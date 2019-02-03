#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include "win_macos.h"
#include "../client/keys.h"
#include "../linux/rw_linux.h"
// for the keycodes
#include <Carbon/Carbon.h>

// note that it has to be a .m file instead of .mm - .m symbols are visible in .c while .mm are visible in .cpp only ¯\_(ツ)_/¯

extern in_state_t *in_state;
extern int mouse_active;
extern int mx, my;

static NSWindow *window = nil;

@interface MetalView : NSView
- (instancetype)initWithFrame:(NSRect)frame;
@end

@implementation MetalView

+ (Class)layerClass
{
    return NSClassFromString(@"CAMetalLayer");
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (CALayer*)makeBackingLayer
{
    return [self.class.layerClass layer];
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if((self = [super initWithFrame:frame]))
    {
        self.wantsLayer = YES;
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
       
        [self updateDrawableSize];
    }
    
    return self;
}

- (void)updateDrawableSize
{
    CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
    CGSize size = self.bounds.size;
    CGSize backingSize = [self convertSizeToBacking:size];
    
    metalLayer.contentsScale = backingSize.height / size.height;
    metalLayer.drawableSize = backingSize;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
    [super resizeWithOldSuperviewSize:oldSize];
    [self updateDrawableSize];
}

@end

@interface Quake2WindowListener : NSResponder<NSWindowDelegate>

-(void) listen:(NSApplication *) data;
-(BOOL) windowShouldClose:(id) sender;

@end

@implementation Quake2WindowListener

- (void)listen:(NSApplication *)data
{
    if ([window delegate] == nil) {
		[window setDelegate:self];
    }

	[window setAcceptsMouseMovedEvents:YES];
    [window setNextResponder:self];
    [[window contentView] setNextResponder:self];
}

- (BOOL)windowShouldClose:(id)sender
{
    in_state->Quit_fp();
    return YES;
}

// these empty functions are necessary so that there's no beeping sound when pressing keys
- (void)flagsChanged:(NSEvent *)theEvent {}
- (void)keyDown:(NSEvent *)theEvent {}
- (void)keyUp:(NSEvent *)theEvent {}
- (void)doCommandBySelector:(SEL)aSelector {}

@end

@interface Quake2Window : NSWindow
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

@implementation Quake2Window : NSWindow
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end

static Quake2WindowListener *windowListener = nil;

static int KeyLate(NSEvent *keyEvent)
{
    int key = 0;

    switch([keyEvent keyCode])
    {
        case kVK_Escape: key = K_ESCAPE; break;
        case kVK_Return: key = K_ENTER; break;
        case kVK_Delete: key = K_BACKSPACE; break;
        case kVK_UpArrow: key = K_UPARROW; break;
        case kVK_LeftArrow: key = K_LEFTARROW; break;
        case kVK_RightArrow: key = K_RIGHTARROW; break;
        case kVK_DownArrow: key = K_DOWNARROW; break;
        case kVK_PageUp: key = K_PGUP; break;
        case kVK_PageDown: key = K_PGDN; break;
        case kVK_Home: key = K_HOME; break;
        case kVK_End: key = K_END; break;
        case kVK_Tab: key = K_TAB; break;
        case kVK_F1: key = K_F1; break;
        case kVK_F2: key = K_F2; break;
        case kVK_F3: key = K_F3; break;
        case kVK_F4: key = K_F4; break;
        case kVK_F5: key = K_F5; break;
        case kVK_F6: key = K_F6; break;
        case kVK_F7: key = K_F7; break;
        case kVK_F8: key = K_F8; break;
        case kVK_F9: key = K_F9; break;
        case kVK_F10: key = K_F10; break;
        case kVK_F11: key = K_F11; break;
        case kVK_F12: key = K_F12; break;
        case kVK_Space: key = K_SPACE; break;
        case kVK_Shift:
        case kVK_RightShift: key = K_SHIFT; break;
        case kVK_Option:
        case kVK_RightOption: key = K_ALT; break;
        case kVK_Control:
        case kVK_RightControl: key = K_CTRL; break;
		case kVK_Command:
		case kVK_RightCommand: key = K_CMD; break;
        case kVK_ANSI_A: key = 'a'; break;
        case kVK_ANSI_B: key = 'b'; break;
        case kVK_ANSI_C: key = 'c'; break;
        case kVK_ANSI_D: key = 'd'; break;
        case kVK_ANSI_E: key = 'e'; break;
        case kVK_ANSI_F: key = 'f'; break;
        case kVK_ANSI_G: key = 'g'; break;
        case kVK_ANSI_H: key = 'h'; break;
        case kVK_ANSI_I: key = 'i'; break;
        case kVK_ANSI_J: key = 'j'; break;
        case kVK_ANSI_K: key = 'k'; break;
        case kVK_ANSI_L: key = 'l'; break;
        case kVK_ANSI_M: key = 'm'; break;
        case kVK_ANSI_N: key = 'n'; break;
        case kVK_ANSI_O: key = 'o'; break;
        case kVK_ANSI_P: key = 'p'; break;
        case kVK_ANSI_Q: key = 'q'; break;
        case kVK_ANSI_R: key = 'r'; break;
        case kVK_ANSI_S: key = 's'; break;
        case kVK_ANSI_T: key = 't'; break;
        case kVK_ANSI_U: key = 'u'; break;
        case kVK_ANSI_V: key = 'v'; break;
        case kVK_ANSI_W: key = 'w'; break;
        case kVK_ANSI_X: key = 'x'; break;
        case kVK_ANSI_Y: key = 'y'; break;
        case kVK_ANSI_Z: key = 'z'; break;
        case kVK_ANSI_0: key = '0'; break;
        case kVK_ANSI_1: key = '1'; break;
        case kVK_ANSI_2: key = '2'; break;
        case kVK_ANSI_3: key = '3'; break;
        case kVK_ANSI_4: key = '4'; break;
        case kVK_ANSI_5: key = '5'; break;
        case kVK_ANSI_6: key = '6'; break;
        case kVK_ANSI_7: key = '7'; break;
        case kVK_ANSI_8: key = '8'; break;
        case kVK_ANSI_9: key = '9'; break;
        case kVK_ANSI_Minus:
        case kVK_ANSI_KeypadMinus: key = '-'; break;
        case kVK_ANSI_KeypadEquals:
        case kVK_ANSI_Equal: key = '='; break;
        case kVK_ANSI_Period: key ='.'; break;
        case kVK_ANSI_Comma: key = ','; break;
        case kVK_ANSI_Semicolon: key = ';'; break;
        case kVK_ANSI_Slash: key = '/'; break;
        case kVK_ANSI_Backslash: key = '\\'; break;
        case kVK_ANSI_LeftBracket: key = '['; break;
        case kVK_ANSI_RightBracket: key = ']'; break;
        case kVK_ANSI_Quote: key = '\''; break;
        case kVK_ISO_Section:
        case kVK_ANSI_Grave: key = '~'; break;
        default:
            return key;
    }

    return key;
}

void MacOSHandleEvents()
{
    if(window == nil) return;

	qboolean dowarp = false;
	int mwx, mwy;

	if(vid_fullscreen->value)
	{
		mwx = [[NSScreen mainScreen] frame].size.width / 2;
		mwy = [[NSScreen mainScreen] frame].size.height / 2;
	}
	else
	{
		mwx = [window frame].origin.x + [window frame].size.width/2;
		mwy = [[NSScreen mainScreen] frame].size.height - [window frame].size.height - [window frame].origin.y + [window frame].size.height/2;
	}

    for (;;) {
        NSEvent *pEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES ];
        if (!pEvent) {
            break;
        }
        
        switch ([pEvent type]) {
            case NSEventTypeLeftMouseDown:
			case NSEventTypeLeftMouseUp:
				if (in_state && in_state->Key_Event_fp)
					in_state->Key_Event_fp (K_MOUSE1, [pEvent type] == NSEventTypeLeftMouseDown);
				break;
			case NSEventTypeRightMouseDown:
			case NSEventTypeRightMouseUp:
				if (in_state && in_state->Key_Event_fp)
					in_state->Key_Event_fp (K_MOUSE2, [pEvent type] == NSEventTypeRightMouseDown);
				break;
            case NSEventTypeOtherMouseDown:
			case NSEventTypeOtherMouseUp:
				if (in_state && in_state->Key_Event_fp)
					in_state->Key_Event_fp (K_MOUSE3, [pEvent type] == NSEventTypeOtherMouseDown);
				break;
            case NSEventTypeScrollWheel:
				if (in_state && in_state->Key_Event_fp)
				{
					if([pEvent deltaY] != 0)
						in_state->Key_Event_fp ([pEvent deltaY] < 0 ? K_MWHEELDOWN : K_MWHEELUP, true);
					in_state->Key_Event_fp (K_MWHEELUP, false);
					in_state->Key_Event_fp (K_MWHEELDOWN, false);
				}
                break;
			case NSEventTypeLeftMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeOtherMouseDragged:
			case NSEventTypeMouseMoved:
				if (mouse_active)
				{
					CGEventRef event = CGEventCreate(NULL);
					CGPoint cursor = CGEventGetLocation(event);
					CFRelease(event);
					mx += ((int)cursor.x - mwx) * 2;
					my += ((int)cursor.y - mwy) * 2;
					mwx = cursor.x;
					mwy = cursor.y;

					if (mx || my)
						dowarp = true;
				}
				break;
            case NSEventTypeKeyDown:
            case NSEventTypeKeyUp:
                if(in_state && in_state->Key_Event_fp)
                    in_state->Key_Event_fp(KeyLate(pEvent), [pEvent type] == NSEventTypeKeyDown);
                break;
            case NSEventTypeFlagsChanged:
                if(in_state && in_state->Key_Event_fp)
                {
                    int keyCode = KeyLate(pEvent);
                    qboolean ctrlDown = keyCode == K_CTRL && ([pEvent modifierFlags] & NSEventModifierFlagControl);
                    qboolean shiftDown = keyCode == K_SHIFT && ([pEvent modifierFlags] & NSEventModifierFlagShift);
                    qboolean altDown = keyCode == K_ALT && ([pEvent modifierFlags] & NSEventModifierFlagOption);
					qboolean cmdDown = keyCode == K_CMD && ([pEvent modifierFlags] & NSEventModifierFlagCommand);
                    in_state->Key_Event_fp(keyCode, ctrlDown || shiftDown || altDown || cmdDown);
                }
                break;
            default:
                break;
        }

		if(dowarp)
		{
			if(vid_fullscreen->value)
				CGWarpMouseCursorPosition(CGPointMake([[NSScreen mainScreen] frame].size.width/2, [[NSScreen mainScreen] frame].size.height/2));
			else
				CGWarpMouseCursorPosition(CGPointMake([window frame].origin.x + [window frame].size.width/2, [[NSScreen mainScreen] frame].size.height - [window frame].size.height - [window frame].origin.y + [window frame].size.height/2));
			CGAssociateMouseAndMouseCursorPosition(YES);
		}

        // Send the event to the operating system
        [NSApp sendEvent:pEvent];
    }
}

void MacOSCreateWindow(int x, int y, int *w, int *h, qboolean fullscreen)
{ //@autoreleasepool {
    if(window != nil) {
        return;
    }

    windowListener = [[Quake2WindowListener alloc] init];
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

	NSRect windowFrame;
	NSUInteger windowStyle = 0;
	CGFloat s = [[NSScreen mainScreen] backingScaleFactor];

	if(fullscreen)
	{
		windowFrame = [[NSScreen mainScreen] frame];
		[NSMenu setMenuBarVisible:NO];
		*w = windowFrame.size.width * s;
		*h = windowFrame.size.height * s;
	}
	else
	{
		windowStyle = NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable;
		windowFrame = NSMakeRect(x, y, *w/s, *h/s);
		[NSMenu setMenuBarVisible:YES];
	}

    window = [[Quake2Window alloc] initWithContentRect:windowFrame styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];

    [window setContentView:[[NSView alloc] initWithFrame:windowFrame]];
    [window setTitle:@"Quake 2 (Vulkan) "CPUSTRING];
    [window makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];
    [windowListener listen:app];
//}
}

void MacOSDestroyWindow()
{
	[window close];
	window = nil;
}

MetalView *AddMetalView()
{
    MetalView *mv = [[MetalView alloc] initWithFrame:[[window contentView] frame]];
    [[window contentView] addSubview:mv];
    return mv;
}

VkResult MacOSCreateVulkanSurface(VkInstance vk_instance, VkSurfaceKHR *vk_surface)
{
    VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
        .pNext = NULL,
        .flags = 0,
        .pView = (__bridge const void *)(AddMetalView())
    };
    
    return vkCreateMacOSSurfaceMVK(vk_instance, &surfaceCreateInfo, NULL, vk_surface);
}
