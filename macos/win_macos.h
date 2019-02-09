#ifndef __WIN_MACOS_H__
#define __WIN_MACOS_H__

#include "../ref_vk/vk_local.h"
#include "../macos/vk_macos.h"

// MacOS window creation, destruction and event handling.

void CocoaHandleEvents(void);
void CocoaCreateWindow(int x, int y, int *w, int *h, qboolean fullscreen);
void CocoaDestroyWindow(void);
const void *CocoaAddMetalView(void);
#endif
