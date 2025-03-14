# Terrain Rendering using SDL3 GPU
---
Based on rastertek's Direct3d11 Terrain series. Roughly.
Very roughly. This program attempts to push lot of terrain work to gpu.

## Dependencies
Dependencies are loaded via CPM.CMake.
- SDL3
- GLM
- DDS-KTX

## Toolset
- CMake 3.31+
- CPM.CMake
- Ninja build
- MSVC
- Windows SDK

## Progress
[x] Initialize SDL3, create a window and get GPU device
[x] Create basic plane mesh, shaders and pipeline
[x] Load terrain image
[ ] Use terrain image to generate heightmap
[ ] Terrain cells
[ ] Culling cells
[ ] ... Finish??
