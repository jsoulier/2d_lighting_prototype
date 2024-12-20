# 2.5D Lighting Prototype

Tiny isometric lighting prototype.
Combines an isometric and top-down orthographic view to support a large number of point lights with shadows.
Relies on raycasting instead of rendering from each light source to reduce the number of renders.

![](image.png)

Steps:
1. Render the scene from the player view
2. Render the same scene from a topdown orthographic view using back face culling
3. Render the same scene from a topdown orthographic view using front face culling
4. Sample the world space position (ray origin) for each fragment
5. Walk each ray to each light and cull if between the front and back face
6. (optional) Add directional shadows, SSAO, and apply PCF

See the shader implementation [here](shaders/light.frag)

### Building

```bash
git clone https://github.com/jsoulier/2.5d_lighting_prototype --recurse-submodules
cd 2.5d_lighting_prototype
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 8 --config Release
cd bin
./prototype.exe
```

### Known Bugs

- The screen will be entirely black if there's no lights in the scene
- There's no depth peeling so models with multiple back faces will have incorrect lighting
