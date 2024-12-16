# 2.5D Lighting Prototype

Tiny isometric lighting prototype.
Combines an isometric and top-down orthographic view to support point lights with shadows.
Relies on raycasting instead of rendering from the light source to reduce the number of renders.

![](image.png)

Steps:
1. Render the scene from the player view
2. Render the same scene but from a top-down orthographic view
3. Convert fragments from the player view to the top-down orthographic view
4. Raycast from each converted fragment to every light in range. If the raycast hits an object above the light, the fragment is in shadow
5. Composite and upscale (optional but reduces number of raycasts)

This solution provides a crude approximation of lighting in a real-scene (sketchy lighting beneath trees, side faces having the same lighting).
However, it can support a large number of point lights and massively reduces memory usage (~30 point lights running 768x432 at 60fps on integrated Radeon Graphics).