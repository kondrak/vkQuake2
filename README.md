# vkQuake2
Vulkan port of id Software's Quake 2

Setup:
===
- install Visual Studio 2017 Community (because of MFC and resources)
- install Windows SDK 10.0.17134.0 and MFC in Visual Studio installer

Features:
===
- using Vulkan 1.1
- using VK_EXT_debug_utils instead of VK_EXT_debug_report
- added custom debug console for debug builds
- updated with Knightmare software renderer with color
- fixed original compiler warnings
- vk_validation command to enable layers (0 - off, 1 - warnings and errors, 2 - full validation, defaults to 2 in debug builds and 0 in release)
