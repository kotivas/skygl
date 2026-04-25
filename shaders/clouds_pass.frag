#version 450 core
#include <atmosphere/atmosphere.glsl>
#include <utility.glsl>

out vec4 FragColor;
in vec3 vRay;

uniform sampler3D baseNoise;
uniform sampler3D detailNoise;
uniform sampler2D weatherMap;
uniform sampler2D highCloudsMap;

uniform vec3 uCameraPos;
uniform float uTime;
uniform vec3 uSunDir;
uniform float uWindSpeed;// in m/s

layout (std140, binding = 1) uniform CloudsParameters {
    float uCloudLayerThickness;
    float uCloudLayerBottom;
    float uHighCloudsHeight;

    float uCirrusDensity;
    float uAltoDensity;

    float uMaxDistance;
    float uCoverage;

    float sigma;
    float uSigmaA;

    float uHighCloudsScale;
    float uWeatherMapScale;
    float uBaseNoiseScale;
    float uDetailNoiseScale;
};

#define   CLOUD_RAY_STEPS 160
#define   LIGHT_RAY_STEPS 8

const float fogFactor = uMaxDistance / 3.0;

const float uSigmaS = 0.01f * 1000.0;

const float sigmaT = uSigmaA + uSigmaS;

const float cirrusSpeedFactor = 3.5;
const float altoSpeedFactor = 2.0;
const float detailStrength = 0.1;

const vec3 earth_center = vec3(0, -bottom_radius, 0);

#define EPS 1e-5

float HenyeyGreenstein(float g, float costh) {
    return (1.0 - g * g) / (4.0 * PI * pow(1.0 + g * g - 2.0 * g * costh, 1.5));
}

float HeightGradient(float h, float cloudType) {
    float stratus = smoothstep(0.0, 0.07, h) * smoothstep(0.2, 0.07, h);
    float cumulus = smoothstep(0.0, 0.2, h) * smoothstep(0.65, 0.35, h);
    float cumulonimbus = smoothstep(0.0, 0.1, h) * smoothstep(0.95, 0.4, h);

    float a = 1.0 - cloudType;
    float wS  = a * a;
    float wCu = 2.0 * cloudType * a;
    float wCb = cloudType * cloudType;

    return wS * stratus + wCu * cumulus + wCb * cumulonimbus;
}

float yFalloff(vec3 position) {
    float falloffDist = uCloudLayerThickness * 0.1;
    float distY = min(falloffDist, min(position.y - uCloudLayerBottom, uCloudLayerBottom + uCloudLayerThickness - position.y));
    return distY / falloffDist;
}

float SampleDensity(vec3 pos){
    float heightFraction = (length(pos - earth_center) - bottom_radius - uCloudLayerBottom) / uCloudLayerThickness;
    if (heightFraction < 0.0 || heightFraction > 1.0) return 0.0;

    // === weather map ===
    //    vec2 weather_uv = pos.xz / uWeatherMapScale + uTime * uWindSpeed / uWeatherMapScale;
    //    vec3 weather = texture(weatherMap, weather_uv).rgb;

    vec3 n = normalize(pos - earth_center);
    // triplanar fixes 2d texture stretching on sphere
    vec3 weather = triplanar(weatherMap, pos + uTime * uWindSpeed, n, 1/ uWeatherMapScale).rgb;

    float coverage = weather.r * uCoverage;
    float precipitation = weather.g;
    float cloudType = weather.b;
    float heightGradient = HeightGradient(heightFraction, cloudType);

    // === base form ===
    vec3 base_uv = pos / uBaseNoiseScale + uTime * uWindSpeed / uBaseNoiseScale;
    vec4 baseN = texture(baseNoise, base_uv);
    float lowFBM = baseN.g * 0.625 + baseN.b * 0.25 + baseN.a * 0.125;
    float baseCloud = remap(baseN.r, -(1.0-lowFBM), 1.0, 0.0, 1.0);

    // === compose ===
    baseCloud = remap(baseCloud, 1.0-coverage, 1.0, 0.0, 1.0);
    baseCloud *= coverage;
    baseCloud *= heightGradient;
    //        baseCloud *= yFalloff(pos);

    if (baseCloud < 0) return 0;

    // === details ===
    vec3 detail_uv = pos / uDetailNoiseScale + uTime * uWindSpeed / uDetailNoiseScale;
    vec3 detailN = texture(detailNoise, detail_uv).rgb;
    float detailFBM = dot(detailN.rgb, vec3(0.625, 0.25, 0.125));
    float detailCloud = remap(baseCloud, detailFBM * detailStrength, 1.0, 0.0, 1.0);

    return saturate(detailCloud);
}

float MultipleOctaveScattering(float density, float mu) {
    float attenuation      = 0.3;
    float contribution     = 0.5;
    float phaseAttenuation = 0.5;

    const float scatteringOctaves = 4.0;

    float a = 1.0;
    float b = 1.0;
    float c = 1.0;

    float luminance = 0.0;

    for (float i = 0.0; i < scatteringOctaves; i++) {
        float phase = mix(HenyeyGreenstein(-0.2 * c, mu), HenyeyGreenstein(0.8 * c, mu), 0.5);
        luminance += b * phase * exp(-sigmaT * density * a);

        a *= attenuation;
        b *= contribution;
        c *= phaseAttenuation;
    }

    return luminance;
}

bool IntersectCloudLayer(vec3 ro, vec3 rd, out float tEnter, out float tExit) {
    float innerRadius = bottom_radius + uCloudLayerBottom;
    float outerRadius = bottom_radius + uCloudLayerBottom + uCloudLayerThickness;

    float tInnerNear = raySphereIntersect(ro, rd, earth_center, innerRadius);
    float tInnerFar  = raySphereIntersectFar(ro, rd, earth_center, innerRadius);
    float tOuterNear = raySphereIntersect(ro, rd, earth_center, outerRadius);
    float tOuterFar  = raySphereIntersectFar(ro, rd, earth_center, outerRadius);

    float distToCenter = length(ro - earth_center);

    if (distToCenter < innerRadius) { // cam below layer
        tEnter = tInnerFar;
        tExit  = tOuterFar;
    } else if (distToCenter < outerRadius) { // cam inside layer
        tEnter = 0.0;
        tExit  = (tInnerNear > 0.0) ? tInnerNear : tOuterFar;
    } else { // cam above layer
        if (tOuterNear < 0.0) return false;
        tEnter = tOuterNear;
        tExit  = (tInnerNear > 0.0) ? tInnerNear : tOuterFar;
    }

    if (tExit < 0.0 || tEnter >= tExit) return false;

    tEnter = max(tEnter, 0.0);
    return true;
}

float ComputeLightVisibility(vec3 pos, vec3 lightDir, float mu) {
    float tLightEnter, tLightExit;
    if (!IntersectCloudLayer(pos, lightDir, tLightEnter, tLightExit)) return 0;

    float dt = tLightExit / float(LIGHT_RAY_STEPS);
    dt *= max(abs(dot(lightDir, vec3(0, 1, 0))), 0.1);
    float d = 0.0;

    for (int i = 0; i < LIGHT_RAY_STEPS; ++i) {
        d += SampleDensity(pos + lightDir * dt * float(i));
    }
    d *= dt;

    float beersLaw = MultipleOctaveScattering(d, mu);
    float powder = 1.0 - exp(-d * 2.0 * sigmaT);

    return beersLaw * mix(1.0, powder * 2.0, 0.5 - 0.5 * mu);
}


vec4 RaymarchVolumetricCloud(vec3 ro, vec3 rd) {
    float tEnter;
    float tExit;

    if (!IntersectCloudLayer(ro, rd, tEnter, tExit)) return vec4(0.0);

    float dt = (tExit - tEnter) / float(CLOUD_RAY_STEPS);

    float t = max(tEnter, 0.0);
    vec3 pos = ro + rd * t;
    float horizDist = length(pos - ro);

    vec3 radiance = vec3(0.0);
    vec3 totalTransmittance = vec3(1.0);

    float mu = dot(-rd, uSunDir);

    vec3 trans;
    GetSkyRadiance(pos - earth_center, uSunDir, 0.0, uSunDir, trans);
    vec3 sunLight = trans * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    vec3 skyLight = GetSkyRadiance(pos - earth_center, vec3(0, 1, 0), 0.0, uSunDir, trans);

    for (int i = 0; i < CLOUD_RAY_STEPS; i++) {
        if (t > tExit) break;

        pos = ro + rd * t;

        if (dot(horizDist, horizDist) > uMaxDistance * uMaxDistance) break;

        float density = SampleDensity(pos);

        if (density > 0.0) {
            float sampleSigmaS = uSigmaS * density;
            float sampleSigmaT = sigmaT * density;

            float vis = ComputeLightVisibility(pos, uSunDir, mu);

            float heightFraction = (length(pos - earth_center) - bottom_radius - uCloudLayerBottom) / uCloudLayerThickness;
            vec3 heightAmbient = mix(skyLight*0.1, skyLight*2, heightFraction);

            vec3 inscatter = (sunLight * vis + heightAmbient) * sampleSigmaS;

            float transmittance = exp(-sampleSigmaT * dt);

            // Energy conserving
            radiance += totalTransmittance * (inscatter - inscatter * transmittance) / sampleSigmaT;
            totalTransmittance *= transmittance;

            if (length(totalTransmittance) <= 0.1) {
                totalTransmittance = vec3(0.0);
                break;
            }
        }

        t += dt;
    }

    // Convert transmittance to alpha
    float alpha = 1.0 - dot(totalTransmittance, vec3(1.0 / 3.0));

    float fog = exp(-horizDist / fogFactor);// ! hack probably should use bruneton transmittance

    return vec4(radiance, alpha) * fog;
}

// High-altitude clouds (above the volumetric) 2D
vec4 HighAltitudeCloud(vec3 ro, vec3 rd) {
    if (abs(rd.y) < EPS) return vec4(0.0);

    vec4 result = vec4(0.0);

    float t = raySphereIntersect(ro, rd, earth_center, bottom_radius + uHighCloudsHeight);
    if (t < 0.0) return result;

    vec3 pos = ro + rd * t;
    vec3 normal = normalize(pos - earth_center);
    vec2 wind = vec2(uTime * uWindSpeed);
    float mu    = dot(rd, uSunDir);
    float phase = mix(HenyeyGreenstein(-0.1, mu), HenyeyGreenstein(0.6, mu), 0.6);

    vec3 windPos = pos;
    pos.xz * wind;

    vec3 trans;
    GetSkyRadiance(pos - earth_center, uSunDir, 0.0, uSunDir, trans);
    vec3 sunLight = trans * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    vec3 skyLight = GetSkyRadiance(pos - earth_center, vec3(0, 1, 0), 0.0, uSunDir, trans);

    vec3  scatter = sunLight * phase + skyLight;

    //     === Cirrus (R) ===

    float cirrusDensity = triplanar(highCloudsMap, windPos * cirrusSpeedFactor, normal, 1 / uHighCloudsScale).r;

    if (cirrusDensity > EPS) {
        float alpha = cirrusDensity * uCirrusDensity;
        result.rgb += (1.0 - result.a) * scatter * alpha;
        result.a   += (1.0 - result.a) * alpha;
    }

    //     === Alto (G) ===
    float altoDensity = triplanar(highCloudsMap, windPos * altoSpeedFactor, normal, 1 / uHighCloudsScale).g;

    if (altoDensity > EPS) {
        float alpha = altoDensity * uAltoDensity;
        result.rgb += (1.0 - result.a) * scatter * alpha;
        result.a   += (1.0 - result.a) * alpha;
    }

    float fog = exp(-length((ro + rd * t).xz - ro.xz) / fogFactor);

    return result * fog;
}

void main() {
    vec3 rd = normalize(vRay);
    vec4 volumetric = RaymarchVolumetricCloud(uCameraPos, rd);
    vec4 hightAlt = HighAltitudeCloud(uCameraPos, rd);

    vec4 cloud = volumetric;
    cloud.rgb  += (1.0 - cloud.a) * hightAlt.rgb;
    cloud.a    += (1.0 - cloud.a) * hightAlt.a;

    //    if (raySphereIntersect(uCameraPos, view_ray, earth_center, bottom_radius) > 0.0) cloud = vec4(0.0);

    FragColor = cloud;
}