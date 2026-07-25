# SkyGL

![OpenGL](https://img.shields.io/badge/OpenGL-4.x-blue?style=flat-square) ![Status](https://img.shields.io/badge/Status-In%20Development-orange?style=flat-square)

Project written with OpenGL focused on creating and exploring in-game sky including clouds and atmosphere
rendering.

![preview](res/preview.jpg)

## Core
core idea is raymarching through the clouds with density accumulation, plus light scattering. Light scattering is a single-scatter, energy-conserving integral along the view ray using a dual-lobe Henyey-Greenstein phase function, with a 4-octave attenuation trick faking multiple scattering. also threw in some cheap 2d high-altitude clouds, no raymarch cost, they just keep the sky from looking empty. sun and sky light get sampled at every raymarch point too, and since that's built on the Bruneton atmosphere it's just texture lookups, basically free.

instead of a single static weather map there are six procedural weather states (clear to storm), each built from its own noise parameters. a scheduler picks the next state using weighted random selection, and the sky blends smoothly between current and next state over time. since the states themselves are procedural, blending between them keeps producing new in-between weather that never quite repeats.

## Build & Run
### Requirements

- CMake 4.0+
- C++20
- OpenGL >= 4.5
- GLM, ImGui, GLFW3, GLAD, STB

```bash
git clone https://github.com/kotivas/skygl.git
cd skygl
cmake -B build
cmake --build build
```

Copy the `res` folder next to the executable, then run it

## Planned

- [ ] Temporal reprojection
- [ ] Night sky

## References

- Bruneton, E. (2017) — Precomputed Atmospheric Scattering: a New Implementation
  https://ebruneton.github.io/precomputed_atmospheric_scattering/

- Guerrilla Games (2015) — The Real-Time Volumetric Cloudscapes of Horizon Zero Dawn
  https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn

- SimonDev (2022) — How Big Budget AAA Games Render Clouds
  https://youtu.be/Qj_tK_mdRcA?si=YALyvCXU96xTZG0H
