<p align="center"><img src="vkQuake2.png"></p>

Overview
===
This is the official Quake 2 code v3.21 with additional Vulkan renderer. The goal is to maintain as much compatibility as possible with the original game, so there are no fancy visual upgrades here - just pure, vanilla Quake 2 experience as we knew it back in 1997. There are, however, a few notable differences that made the cut for various reasons:

- original compiler warnings have been fixed
- `M_DrawTextBox` function is being called slightly differently without resorting to ending the frame prematurely
- debug build comes with additional cmd console for debugging purposes when no visuals are available
- 1920x1080, 2560x1440, and 3840x2160 screen resolutions have been added
- warped texture effect (lava, water, slime) is now properly drawn (though only with Vulkan and software renderer!)
- software renderer has been completely replaced with [KolorSoft 1.1](https://github.com/qbism/Quake2-colored-refsoft) - this adds colored lighting and fixes severe instabilities of the original renderer
- on first launch, the game attempts to use Vulkan at 1920x1080 resolution by default and reverts to software renderer on failure

Building
===
- download and install [Vulkan SDK](https://vulkan.lunarg.com/) - make sure that the `VULKAN_SDK` environment variable is set afterwards
- install [Visual Studio Community](https://www.visualstudio.com/products/free-developer-offers-vs) with the MFC package
- install Windows Universal CRT SDK and Windows SDK 8.1 or alternatively the latest Windows 10 SDK (this will require retargetting the solution)

With this setup, the game should build out of the box with no additional dependencies.

Running
===
The Visual Studio 2017 C++ Redistributable [(32 bit)](https://go.microsoft.com/fwlink/?LinkId=746571) is required to run the application.

The release package comes with game data used in the demo version - for full experience, copy all retail Quake 2 data paks (`pak0.pak`, `pak1.pak`, `pak2.pak`) into the `baseq2` folder and run the executable. The mission packs have not been tested but should work just as well.

Console commands
===

The Vulkan renderer comes with a set of its own additional console commands:

| Command               | Action                                                  |
|-----------------------|:--------------------------------------------------------|
| vk_validation         | Toggle validation layers.<br>0 - disabled (default in Release)<br> 1 - only errors and warnings<br>2 - full validation (default in Debug) |
| vk_strings            | Print some basic Vulkan/GPU information.                                    |
| vk_device             | Specifiy preferred Vulkan device index on systems with multiple GPUs.<br>-1 - prefer first DISCRETE_GPU (default)<br>0..n - use device #n (full list of devices is returned by vk_strings command) |
| vk_msaa               | Toggle MSAA.<br>0 - off (default)<br>1 - MSAAx2<br>2 - MSAAx4<br>3 - MSAAx8 |
| vk_flashblend         | Toggle the blending of lights onto the environment. (default: 0)            |
| vk_polyblend          | Blend fullscreen effects: blood, powerups etc. (default: 1)                 |
| vk_skymip             | Toggle the usage of mipmap information for the sky graphics. (default: 0)   |
| vk_finish             | Inserts vkDeviceWaitIdle() on render start (default: 0).<br>Don't use this, it's just for the sake of having a gl_finish equivalent. |
| vk_point_particles    | Use POINT_LIST to render particles, textured triangles otherwise. (default: 1) |
| vk_particle_size      | Rendered particle size. (default: 40)                   |
| vk_particle_att_a     | Intensity of the particle A attribute. (default: 0.01)  |
| vk_particle_att_b     | Intensity of the particle B attribute. (default: 0)     |
| vk_particle_att_c     | Intensity of the particle C attribute. (default: 0.01)  |
| vk_particle_min_size  | The minimum size for a rendered particle. (default: 2)  |
| vk_particle_max_size  | The maximum size for a rendered particle. (default: 40) |
| vk_lockpvs            | Lock current PVS table. (default: 0)                    |
| vk_clear              | Clear the color buffer each frame. (default: 0)         |
| vk_modulate           | Texture brightness modifier. (default: 1)               |
| vk_shadows            | Draw experimental entity shadows. (default: 0)          |
| vk_picmip             | Shrink factor for the textures. (default: 0)            |
| vk_round_down         | Toggle the rounding of texture sizes. (default: 1)      |
| vk_log                | Log frame validation data to file. (default: 0)         |
| vk_dynamic            | Use dynamic lighting. (default: 1)                      |
| vk_showtris           | Display mesh triangles. (default: 0)                    |
| vk_lightmap           | Display lightmaps. (default: 0)                         |
| vk_texturemode        | Change current texture filtering.<br>VK_NEAREST - nearest filter, no mipmaps<br>VK_LINEAR - linear filter, no mipmaps<br>VK_MIPMAP_NEAREST - nearest filter with mipmaps<br>VK_MIPMAP_LINEAR - linear filter with mipmaps (default) |

Known Issues
===
- some Intel UHD GPUs (most notably the 6XX series) may encounter crashes on startup due to faulty drivers

TODO
===
- implement water warp effect
- add music
- use push constants
- Linux and MacOS versions
