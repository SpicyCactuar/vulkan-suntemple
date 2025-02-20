# Vulkan Suntemple

![vulkan-suntemple](https://github.com/user-attachments/assets/e4271763-e0f2-4cd7-9482-32bad0d7cf10)

Vulkan application that renders the popular [Suntemple](https://developer.nvidia.com/ue4-sun-temple) scene.
The renderer showcase different techniques and features:

* Render-to-texture pipeline
* Physically-based rendering (_roughnes-metalness_ workflow)
* Shadow mapping - Single point light
* Normal mapping
* Alpha masking - Opaque and transparent objects go through separate rendering passes
* Tone mapping - Operator applied as post-processing step to the rendered scene texture

## Project Structure

```plaintext
vulkan-suntemple/
├── assets-bake/           # Asset baking source code
├── assets-src/            # Static assets (to be baked)
├── third-party/           # Bundled third party libraries
├── util/glslc.lua         # Compile-time utility to compile shaders with google/shaderc 
├── vksuntemple/           # Application source code
    ├── shaders/           # GLSL shaders
├── vkutils/               # Application source code
├── premake5.lua           # Premake 5 configuration
├── premake5(.*)           # Bundled Premake 5 executables
├── third-party.md         # Third party libraries' licenses
└── README.md              # Project README
```

## Build - Make

```shell
./premake5 gmake2
make [config={debug_x64|release_x64}]
```

The `config` parameter defaults to `debug_x64`.

## Build - Visual Studio

```shell
./premake5.exe vs2022
```

Open generated `.sln` project file.

## Build - Xcode

```shell
./premake5.apple xcode4
```

Open generated Xcode project.

## Run

```shell
bin/assets-bake-{target}.exe
bin/vksuntemple-{target}.exe
```

Executables have `.exe` extension for all platforms, but binaries are platform-specific.

Baking is required to be run successfully before application.

## Controls

| Key(s)                  | Action                                                                 |
|-------------------------|------------------------------------------------------------------------|
| `Right Click`           | Toggle camera rotation with mouse                                      |
| `W` `A` `S` `D` `E` `Q` | Move camera around                                                     |
| `I` / `L`               | Reset camera to initial/light position                                 |
| `Shift` / `Ctrl`        | Slow/fast speed modifiers for camera controls                          |                                              |
| `1 - 8`                 | Display different visualisation modes (see `state::VisualisationMode`) |
| `Alt` + `1 - 7`         | Display different PBR terms (see `state::PBRTerm`)                     |
| `N` / `O` / `P`         | Toggle normal mapping, shadows, PCF (see `state::ShadingDetails`)      |
| `T`                     | Toggle Reinhard tone mapping                                           |
| `Esc`                   | Close application                                                      |

## Technologies

* **C++**: `>= C++20`
* **Premake**: `5.0.0` (Bundled)
* **Vulkan**: `>= 1.2.0` (Bundled headers)
* **Volk**: `1.3.295` (Bundled)
* **Vulkan Memory Allocator (VMA)**: `3.1.0` (Bundled)
* **GLSL**: `460`
* **GLM**: `0.9.9` (Bundled)
* **GLFW**: `3.4` (Bundled)
* **stb_image**: `v2.29` (Bundled)
* **stb_image_write**: `v1.16` (Bundled)

A few additional supporting libraries are leveraged. Files were selectively bundled as needed.

See `third-party.md` for licensing and attributions.

## TODOs

* [ ] Implement emission and [bloom](https://learnopengl.com/Advanced-Lighting/Bloom)
* [ ] Implement [mesh data optimisations](https://github.com/niklasfrykholm/blog/blob/master/2009/the-bitsquid-low-level-animation-system.md)
