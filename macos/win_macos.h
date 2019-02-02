#ifndef __WIN_MACOS_H__
#define __WIN_MACOS_H__

#include "../ref_vk/vk_local.h"
#include "../macos/vk_macos.h"

VkResult MacOSCreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface);
void MacOSHandleEvents();
void MacOSCreateWindow(int x, int y, int w, int h);
void MacOSDestroyWindow();
#endif
