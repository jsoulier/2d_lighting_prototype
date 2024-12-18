# 2.5D Lighting Prototype

Tiny isometric lighting prototype.
Combines an isometric and top-down orthographic view to support a large number of point lights with shadows.
Relies on raycasting instead of rendering from each light source to reduce the number of renders.

![](image.png)

Steps:
1. Render scene (deferred) from player view
2. Render the same scene from a topdown orthographic view using back face culling
3. Render the same scene from a topdown orthographic view using front face culling
4. For each fragment, sample the world space position and convert to topdown orthographic (world space)
5. Walk the topdown orthographic position to each light and cull if any positions are above the current
6. Walk the original world space position to each light and cull if between the front and back face
7. (optional) Combine with directional shadow mapping

See the implementation [here](shaders/light.frag)

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

- If there are no lights in the scene, the screen will be entirely black