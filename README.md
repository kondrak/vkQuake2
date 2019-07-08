<p align="center"><img src="vkQuake2.png"></p>

## Build status
| Windows  | [![Build status](https://ci.appveyor.com/api/projects/status/73kmpd5e6hpg8jt5?svg=true)](https://ci.appveyor.com/project/kondrak/vkquake2) |
|----------|:------------:|
| <b>Linux/MacOS</b> | [![Build Status](https://travis-ci.org/kondrak/vkQuake2.svg?branch=master)](https://travis-ci.org/kondrak/vkQuake2) |

Overview
===
This is the official Quake 2 code v3.21 with additional Vulkan renderer and mission packs included. The goal is to maintain as much compatibility as possible with the original game, so there are no fancy visual upgrades here - just pure, vanilla Quake 2 experience as we knew it back in 1997. There are, however, a few notable differences that made the cut for various reasons:

- original compiler warnings have been fixed
- 64 bit support has been added
- `M_DrawTextBox` function is being called slightly differently without resorting to ending the frame prematurely
- debug build comes with additional cmd console for debugging purposes when no visuals are available
- 1366x768, 1920x1080, 2560x1440, and 3840x2160 screen resolutions have been added and the game is now DPI aware
- mouse acceleration has been disabled
- console contents can be scrolled with mouse wheel
- behavior of video menu has been altered so that pressing escape does not restart the rendering system
- HUD elements, menus and console text are now scaled accordingly on higher screen resolutions (can be overridden with `hudscale` console command)
- viewmodel weapons are no longer hidden when FOV > 90
- warped texture effect (lava, water, slime) is now properly drawn (though only with Vulkan and software renderer!)
- player vision is warped when underwater in a similar fashion to GLQuake
- software renderer has been completely replaced with [KolorSoft 1.1](https://github.com/qbism/Quake2-colored-refsoft) - this adds colored lighting and fixes severe instabilities of the original renderer
- on first launch, the game attempts to use Vulkan at 1920x1080 resolution by default
- triangle fans have been replaced with indexed triangle lists due to Metal/MoltenVK limitations
- on Linux, sound is now handled by ALSA instead of OSS

Building
===
For extra challenge I decided to base vkQuake2 on the original id Software code. Because of this, there are no dependencies on external SDL-like libraries and the entire project is mostly self-contained. This also implies that some of the original bugs could be present - those may be fixed in upcoming releases.

## Windows
- download and install [Vulkan SDK](https://vulkan.lunarg.com/) - make sure that the `VULKAN_SDK` environment variable is set afterwards
- install [Visual Studio Community](https://www.visualstudio.com/products/free-developer-offers-vs) with the MFC package
- install Windows Universal CRT SDK and Windows SDK 8.1 or just the latest Windows 10 SDK (the latter will require retargetting the solution)
- open `quake2.sln` and choose the target platform (32/64bit) - it should build with no additional steps required

## Linux
Unfortunately, Linux code for Quake 2 has not aged well and for that reason only the Vulkan renderer is available for use at this time. Build steps assume that Ubuntu is the target distribution:
- install required dependencies:
```
sudo apt install make gcc g++ mesa-common-dev libglu1-mesa-dev libxxf86dga-dev libxxf86vm-dev libasound2-dev libx11-dev libxcb1-dev
```
- install [Vulkan SDK](https://vulkan.lunarg.com/) - the easiest way is to use [LunarG Ubuntu Packages](https://vulkan.lunarg.com/sdk/home#linux) - just follow instructions and there will be no additional steps required. If you decide to manually install the SDK, make sure proper environment variables are set afterwards - the easiest way is to add a section to your `.bashrc` file which may look similar to this:
```
export VULKAN_SDK=/home/user/VulkanSDK/1.1.92.1/x86_64
export PATH=$VULKAN_SDK/bin:$PATH
export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
export VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d
```
- make sure your graphics drivers support Vulkan (run `vulkaninfo` to verify) - if not, you can get them with:
```
sudo apt install mesa-vulkan-drivers
```
- in the main repository, enter the `linux` subfolder and type `make release` or `make debug` depending on which variant you want to build - the output binaries will be placed in `releasex64` and `debugx64` directories respectively

## MacOS
- download and extract the [Vulkan SDK](https://vulkan.lunarg.com/) package
- install XCode 10.1 or later and add the `VULKAN_SDK` environment variable to Locations/Custom Paths - make it point to the downloaded SDK
- open `macos/vkQuake2.xcworkspace` - it should build and run without any additional steps - the output binary will be put in `macos/vkQuake2` subfolder
- alternatively, you can compile the game from command line - modify your `.bash_profile` file and add these entries (replace SDK version and location with the one corresponding to your system):
```
export VULKAN_SDK=/home/user/VulkanSDK/1.1.92.1
export VK_ICD_FILENAMES=$VULKAN_SDK/macOS/etc/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$VULKAN_SDK/macOS/etc/vulkan/explicit_layer.d
```
- enter `macos` directory and run `make debug` or `make release` depending on which variant you want to build - output binaries will be put in `macos/vkQuake2` subfolder

This project uses the Vulkan loader bundled with the SDK, rather than directly linking against `MoltenVK.framework`. This is done so that validation layers are available for debugging. Builds have been tested using MacOS 10.14.2.

Running
===
## Windows
The Visual Studio 2017 C++ Redistributable is required to run the application: [32 bit](https://go.microsoft.com/fwlink/?LinkId=746571) or [64 bit](https://go.microsoft.com/fwlink/?LinkId=746572) depending on the chosen flavor. These are provided automatically if you have Visual Studio 2017 installed.

## All platforms
You'll need proper data files to run the game - the [release packages](https://github.com/kondrak/vkQuake2/releases) come with game data used in the demo version. For full experience, copy retail Quake 2 data paks (`pak0.pak`, `pak1.pak`, `pak2.pak`) into the `baseq2` directory and run the executable. For mission packs, copy necessary data to `rogue` ("Ground Zero") and `xatrix` ("The Reckoning") directories respectively. You can then start the game with `./quake2 +set game rogue` or `./quake2 +set game xatrix`.

Console commands
===

The Vulkan renderer comes with a set of its own additional console commands:

| Command               | Action                                                  |
|-----------------------|:--------------------------------------------------------|
| vk_validation         | Toggle validation layers.<br>0 - disabled (default in Release)<br> 1 - only errors and warnings<br>2 - full validation (default in Debug) |
| vk_strings            | Print some basic Vulkan/GPU information.                                    |
| vk_mem                | Print dynamic vertex/index/uniform/triangle fan buffer memory usage statistics.          |
| vk_device             | Specifiy preferred Vulkan device index on systems with multiple GPUs.<br>-1 - prefer first DISCRETE_GPU (default)<br>0..n - use device #n (full list of devices is returned by vk_strings command) |
| vk_msaa               | Toggle MSAA.<br>0 - off (default)<br>1 - MSAAx2<br>2 - MSAAx4<br>3 - MSAAx8<br>4 - MSAAx16 |
| vk_sampleshading      | Toggle sample shading for MSAA. (default: 1) |
| vk_mode               | Vulkan video mode (default: 11). Setting this to `-1` uses a custom screen resolution defined by `r_customwidth` (default: 1024) and `r_customheight` (default: 768) console variables. |
| vk_flashblend         | Toggle the blending of lights onto the environment. (default: 0)            |
| vk_polyblend          | Blend fullscreen effects: blood, powerups etc. (default: 1)                 |
| vk_skymip             | Toggle the usage of mipmap information for the sky graphics. (default: 0)   |
| vk_finish             | Inserts vkDeviceWaitIdle() on render start (default: 0).<br>Don't use this, it's just for the sake of having a gl_finish equivalent. |
| vk_point_particles    | Use POINT_LIST to render particles, textured triangles otherwise. (default: 1) |
| vk_particle_size      | Rendered particle size. (default: 40)                    |
| vk_particle_att_a     | Intensity of the particle A attribute. (default: 0.01)   |
| vk_particle_att_b     | Intensity of the particle B attribute. (default: 0)      |
| vk_particle_att_c     | Intensity of the particle C attribute. (default: 0.01)   |
| vk_particle_min_size  | The minimum size for a rendered particle. (default: 2)   |
| vk_particle_max_size  | The maximum size for a rendered particle. (default: 40)  |
| vk_lockpvs            | Lock current PVS table. (default: 0)                     |
| vk_clear              | Clear the color buffer each frame. (default: 0)          |
| vk_modulate           | Texture brightness modifier. (default: 1)                |
| vk_shadows            | Draw experimental entity shadows. (default: 0)           |
| vk_picmip             | Shrink factor for the textures. (default: 0)             |
| vk_round_down         | Toggle the rounding of texture sizes. (default: 1)       |
| vk_log                | Log frame validation data to file. (default: 0)          |
| vk_dynamic            | Use dynamic lighting. (default: 1)                       |
| vk_showtris           | Display mesh triangles. (default: 0)                     |
| vk_lightmap           | Display lightmaps. (default: 0)                          |
| vk_aniso              | Toggle anisotropic filtering. (default: 1)               |
| vk_vsync              | Toggle vertical sync. (default: 0)                       |
| vk_mip_nearfilter     | Use nearest neighbour filtering for mipmaps. (default: 0) |
| vk_texturemode        | Change current texture filtering.<br>VK_NEAREST - nearest filter, no mipmaps<br>VK_LINEAR - linear filter, no mipmaps<br>VK_MIPMAP_NEAREST - nearest filter with mipmaps<br>VK_MIPMAP_LINEAR - linear filter with mipmaps (default) |
| vk_lmaptexturemode    | Same as `vk_texturemode` but applied to lightmap textures. |

Acknowledgements
===
- Sascha Willems for his [Vulkan Samples](https://github.com/SaschaWillems/Vulkan) which provided excellent examples of some more complex Vulkan features
- Axel Gneiting for [vkQuake](https://github.com/Novum/vkQuake) which was a great inspiration and a rich source of knowledge
- LunarG team and the Khronos Group for their invaluable help and resources

Known Issues
===
- some Intel UHD GPUs (most notably the 6XX series) may encounter crashes on startup due to faulty drivers - this has been confirmed by Intel to affect dual-GPU setups
