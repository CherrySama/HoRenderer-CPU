# HoRenderer-CPU

A physically based offline renderer with supports for path tracing, volume rendering, and multiple material types.

## Main Features

🎯 **Core Rendering Features**

Path Tracing Integrator: Unbiased rendering based on Monte Carlo methods
Volume Rendering: Supports scattering and absorption effects in homogeneous media
Multiple Importance Sampling: Combines BRDF sampling and light sampling
BVH Accelerator: Optimized using SAH (Surface Area Heuristic)
Multi-Threaded Rendering: Parallel computing based on OpenMP

🎨 **Material System**

Diffuse: Diffuse material based on the Oren-Nayar model
Conductor: Metal material with support for complex refraction
Plastic: Plastic material with support for Fresnel reflections
Emission: Emissive material
FrostedGlass: Frosted glass with support for transmission and reflection

🔺 **Geometry Support**

Base Geometries: Sphere, Quad, Cube
Transformations: Rotation, Translation, Scale
Normal Maps: Supports normal textures

💡 **Lighting System**

Area Light: Quad area light
Spherical Light: Spherical area light
Ambient Light: Supports HDR environment maps

🌫️ **Volume Rendering**

Homogeneous Media: Supports scattering and absorption parameters
Phase Functions: Isotropic and Henyey-Greenstein phase functions
Multiple Scattering: Complete volumetric light transport simulation

🎲 **Sampling System**

Sobol Sequence: Low-discrepancy sampling
Importance Sampling: Optimized sampling for BRDFs and light sources
Filters: Uniform, Gaussian, and Tent filters

🖼️ **Texture System**

Solid Color Textures: Monochrome materials
Image Textures: Supports common image formats
HDR Textures: High Dynamic Range environment maps

📷 **Camera System**

Perspective Camera: Adjustable field of view
Depth of Field: Supports focal length and aperture settings

📝 **TODO List**

Model Loader ❎

Spherical Light & Ambient Light ❎

Heterogeneous media ❎

Embree Accelerator ❎

## Requirements📝

**Language:** C++23
**Project Builder:** Xmake
**Mathematical Library:** GLM
**Window Manager:** GLFW
**Graphics API:** OpenGL (GLAD)
**Parallel Computing:** OpenMP
**JSON Parser:** nlohmann_json (optional)
**Ray Tracing Accelerator:** Embree (optional)

## Quick Start

Make sure that you have installed Xmake before running the project.

```
# Clone the repository
git clone https://github.com/yourusername/HoRenderer-CPU.git
cd HoRenderer-CPU

# Configure Project
xmake f -m release

# Compile
xmake

# Run the default Cornell Box scene
xmake run HoRenderer
```



## Gallery

![Cornell Box](examples/CornellBox.png)



![Cornell Box](examples/CornellBox_smoke.png)

## 📚 References

- [Ray Tracing in One Weekend Book Series](https://github.com/RayTracing/raytracing.github.io/tree/release)
- [DreamRender](https://github.com/GraphicsEnthusiast/DreamRender/tree/stage-3)
- [UCSD CSE 272: Advanced Image Synthesis (Winter 2024)](https://cseweb.ucsd.edu/~tzli/cse272/wi2024/)

