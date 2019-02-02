#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include "win_macos.h"
#include "../client/keys.h"
#include "../linux/rw_linux.h"
// for the keycodes
#include <Carbon/Carbon.h>

// note that it has to be a .m file instead of .mm - .m symbols are visible in .c while .mm are visible in .cpp only ¯\_(ツ)_/¯

//@interface Quake2Application : NSApplication
//@end

extern in_state_t *in_state;
extern int mouse_active;
extern int mx, my;

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

NSWindow *window = nil;
NSView *contentView;



@interface Quake2AppDelegate : NSObject <NSApplicationDelegate>
- (id)init;
@end

@implementation Quake2AppDelegate : NSObject
- (id)init
{
    self = [super init];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(focusSomeWindow:) name:NSApplicationDidBecomeActiveNotification object:nil];
    
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSApplicationDidBecomeActiveNotification object:nil];
    //[super dealloc];
}

- (void)windowWillClose:(NSNotification *)notification
{
}

- (void)focusSomeWindow:(NSNotification *)notification
{
}
@end

enum ePendingOperation {
    PENDING_NULL,
    PENDING_GOFULLSCREEN,
    PENDING_EXITFULLSCREEN,
    PENDING_MINIMIZE,
    PENDING_MAXIMIZE
};

@interface Quake2WindowListener : NSResponder<NSWindowDelegate> {
    enum ePendingOperation pendingWindowOperation;
    BOOL observingVisible;
    BOOL wasCtrlLeft;
    BOOL wasVisible;
    BOOL isFullscreenSpace;
    BOOL inFullscreenTransition;
    BOOL isMoving;
    int pendingWindowWarpX;
    int pendingWindowWarpY;
}

-(void) listen:(NSApplication *) data;
-(void) pauseVisibleObservation;
-(void) resumeVisibleObservation;
-(BOOL) setFullscreenSpace:(BOOL) state;
-(BOOL) isInFullscreenSpace;
-(BOOL) isInFullscreenSpaceTransition;
-(void) addPendingWindowOperation:(enum ePendingOperation) operation;
-(void) close;

-(BOOL) isMoving;
-(void) setPendingMoveX:(int)x Y:(int)y;
-(void) windowDidFinishMoving;

/* Window delegate functionality */
-(BOOL) windowShouldClose:(id) sender;
-(void) windowDidExpose:(NSNotification *) aNotification;
-(void) windowDidMove:(NSNotification *) aNotification;
-(void) windowDidResize:(NSNotification *) aNotification;
-(void) windowDidMiniaturize:(NSNotification *) aNotification;
-(void) windowDidDeminiaturize:(NSNotification *) aNotification;
-(void) windowDidBecomeKey:(NSNotification *) aNotification;
-(void) windowDidResignKey:(NSNotification *) aNotification;
-(void) windowWillEnterFullScreen:(NSNotification *) aNotification;
-(void) windowDidEnterFullScreen:(NSNotification *) aNotification;
-(void) windowWillExitFullScreen:(NSNotification *) aNotification;
-(void) windowDidExitFullScreen:(NSNotification *) aNotification;
-(NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions;

/* Window event handling */
-(void) mouseDown:(NSEvent *) theEvent;
-(void) rightMouseDown:(NSEvent *) theEvent;
-(void) otherMouseDown:(NSEvent *) theEvent;
-(void) mouseUp:(NSEvent *) theEvent;
-(void) rightMouseUp:(NSEvent *) theEvent;
-(void) otherMouseUp:(NSEvent *) theEvent;
-(void) mouseMoved:(NSEvent *) theEvent;
-(void) mouseDragged:(NSEvent *) theEvent;
-(void) rightMouseDragged:(NSEvent *) theEvent;
-(void) otherMouseDragged:(NSEvent *) theEvent;
-(void) scrollWheel:(NSEvent *) theEvent;

@end

@implementation Quake2WindowListener

- (void)listen:(NSApplication *)data
{
    NSNotificationCenter *center;
    NSView *view = [window contentView];
    
    observingVisible = YES;
    wasCtrlLeft = NO;
    wasVisible = [window isVisible];
    isFullscreenSpace = NO;
    inFullscreenTransition = NO;
    pendingWindowOperation = PENDING_NULL;
    isMoving = NO;
    
    center = [NSNotificationCenter defaultCenter];
    
    if ([window delegate] != nil) {
        [center addObserver:self selector:@selector(windowDidExpose:) name:NSWindowDidExposeNotification object:window];
        [center addObserver:self selector:@selector(windowDidMove:) name:NSWindowDidMoveNotification object:window];
        [center addObserver:self selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:window];
        [center addObserver:self selector:@selector(windowDidMiniaturize:) name:NSWindowDidMiniaturizeNotification object:window];
        [center addObserver:self selector:@selector(windowDidDeminiaturize:) name:NSWindowDidDeminiaturizeNotification object:window];
        [center addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:window];
        [center addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:window];
        [center addObserver:self selector:@selector(windowWillEnterFullScreen:) name:NSWindowWillEnterFullScreenNotification object:window];
        [center addObserver:self selector:@selector(windowDidEnterFullScreen:) name:NSWindowDidEnterFullScreenNotification object:window];
        [center addObserver:self selector:@selector(windowWillExitFullScreen:) name:NSWindowWillExitFullScreenNotification object:window];
        [center addObserver:self selector:@selector(windowDidExitFullScreen:) name:NSWindowDidExitFullScreenNotification object:window];
    } else {
        [window setDelegate:self];
    }
    
    /* Haven't found a delegate / notification that triggers when the window is
     * ordered out (is not visible any more). You can be ordered out without
     * minimizing, so DidMiniaturize doesn't work. (e.g. -[NSWindow orderOut:])
     */
    [window addObserver:self
             forKeyPath:@"visible"
                options:NSKeyValueObservingOptionNew
                context:NULL];
    
    [window setNextResponder:self];
    [window setAcceptsMouseMovedEvents:YES];
    
    [view setNextResponder:self];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
}

-(void) pauseVisibleObservation
{
    observingVisible = NO;
    wasVisible = [window isVisible];
}

-(void) resumeVisibleObservation
{
    BOOL isVisible = [window isVisible];
    observingVisible = YES;
    if (wasVisible != isVisible) {
        if (isVisible) {
            //            SDL_SendWindowEvent(m_pParent->GetWindow(), SDL_WINDOWEVENT_SHOWN, 0, 0);
        } else {
            //            SDL_SendWindowEvent(m_pParent->GetWindow(), SDL_WINDOWEVENT_HIDDEN, 0, 0);
        }
        
        wasVisible = isVisible;
    }
}

-(BOOL) setFullscreenSpace:(BOOL) state
{
   /* if (state && (videodata->GetFlags() & Burger::Display::FULLSCREEN)) {
        return NO;
    } else if (state == isFullscreenSpace) {
        return YES;
    }*/
    
    if (inFullscreenTransition) {
        if (state) {
            [self addPendingWindowOperation:PENDING_GOFULLSCREEN];
        } else {
            [self addPendingWindowOperation:PENDING_EXITFULLSCREEN];
        }
        return YES;
    }
    inFullscreenTransition = YES;
    
    /* you need to be FullScreenPrimary, or toggleFullScreen doesn't work. Unset it again in windowDidExitFullScreen. */
    [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [window performSelectorOnMainThread: @selector(toggleFullScreen:) withObject:window waitUntilDone:NO];
    return YES;
}

-(BOOL) isInFullscreenSpace
{
    return isFullscreenSpace;
}

-(BOOL) isInFullscreenSpaceTransition
{
    return inFullscreenTransition;
}

-(void) addPendingWindowOperation:(enum ePendingOperation) operation
{
    pendingWindowOperation = operation;
}

- (void)close
{
    NSNotificationCenter *center;
    NSView *view = [window contentView];
    NSArray *windows = nil;
    
    center = [NSNotificationCenter defaultCenter];
    
    if ([window delegate] != self) {
        [center removeObserver:self name:NSWindowDidExposeNotification object:window];
        [center removeObserver:self name:NSWindowDidMoveNotification object:window];
        [center removeObserver:self name:NSWindowDidResizeNotification object:window];
        [center removeObserver:self name:NSWindowDidMiniaturizeNotification object:window];
        [center removeObserver:self name:NSWindowDidDeminiaturizeNotification object:window];
        [center removeObserver:self name:NSWindowDidBecomeKeyNotification object:window];
        [center removeObserver:self name:NSWindowDidResignKeyNotification object:window];
        [center removeObserver:self name:NSWindowWillEnterFullScreenNotification object:window];
        [center removeObserver:self name:NSWindowDidEnterFullScreenNotification object:window];
        [center removeObserver:self name:NSWindowWillExitFullScreenNotification object:window];
        [center removeObserver:self name:NSWindowDidExitFullScreenNotification object:window];
    } else {
        [window setDelegate:nil];
    }
    
    [window removeObserver:self forKeyPath:@"visible"];
    
    if ([window nextResponder] == self) {
        [window setNextResponder:nil];
    }
    if ([view nextResponder] == self) {
        [view setNextResponder:nil];
    }
    
    /* Make the next window in the z-order Key. If we weren't the foreground
     when closed, this is a no-op.
     !!! FIXME: Note that this is a hack, and there are corner cases where
     !!! FIXME:  this fails (such as the About box). The typical nib+RunLoop
     !!! FIXME:  handles this for Cocoa apps, but we bypass all that in SDL.
     !!! FIXME:  We should remove this code when we find a better way to
     !!! FIXME:  have the system do this for us. See discussion in
     !!! FIXME:   http://bugzilla.libsdl.org/show_bug.cgi?id=1825
     */
    windows = [NSApp orderedWindows];
    for (NSWindow *win in windows)
    {
        if (win == window) {
            continue;
        }
        
        [win makeKeyAndOrderFront:self];
        break;
    }
}

- (BOOL)isMoving
{
    return isMoving;
}

-(void) setPendingMoveX:(int)x Y:(int)y
{
    pendingWindowWarpX = x;
    pendingWindowWarpY = y;
}

- (void)windowDidFinishMoving
{
    if ([self isMoving])
    {
        isMoving = NO;
        
        //SDL_Mouse *mouse = SDL_GetMouse();
        if (pendingWindowWarpX >= 0 && pendingWindowWarpY >= 0) {
            //mouse->WarpMouse(_data->window, pendingWindowWarpX, pendingWindowWarpY);
            pendingWindowWarpX = pendingWindowWarpY = -1;
        }
        //        if (mouse->relative_mode && SDL_GetMouseFocus() == _data->window) {
        //            mouse->SetRelativeMouseMode(SDL_TRUE);
        //        }
    }
}

- (BOOL)windowShouldClose:(id)sender
{
    in_state->Quit_fp();
    return YES;
}

- (void)windowDidExpose:(NSNotification *)aNotification
{
}

- (void)windowWillMove:(NSNotification *)aNotification
{
    /*if ([window isKindOfClass:[Quake2Window class]]) {
        pendingWindowWarpX = pendingWindowWarpY = -1;
        isMoving = YES;
    }*/
}

- (void)windowDidMove:(NSNotification *)aNotification
{
}

- (void)windowDidResize:(NSNotification *)aNotification
{
}

- (void)windowDidMiniaturize:(NSNotification *)aNotification
{
}

- (void)windowDidDeminiaturize:(NSNotification *)aNotification
{
}

- (void)windowDidBecomeKey:(NSNotification *)aNotification
{
  /*  if ((isFullscreenSpace) && (m_pParent->GetDisplay()->GetFlags() & Burger::Display::FULLSCREEN)) {
        [NSMenu setMenuBarVisible:NO];
    }*/
}

- (void)windowDidResignKey:(NSNotification *)aNotification
{
    if (isFullscreenSpace) {
        [NSMenu setMenuBarVisible:YES];
    }
}

- (void)windowWillEnterFullScreen:(NSNotification *)aNotification
{
    isFullscreenSpace = YES;
    inFullscreenTransition = YES;
}

- (void)windowDidEnterFullScreen:(NSNotification *)aNotification
{
    inFullscreenTransition = NO;
    
    if (pendingWindowOperation == PENDING_EXITFULLSCREEN) {
        pendingWindowOperation = PENDING_NULL;
        [self setFullscreenSpace:NO];
    } else {
      /*  if (m_pParent->GetDisplay()->GetFlags() & Burger::Display::FULLSCREEN) {
            [NSMenu setMenuBarVisible:NO];
        }*/
        
        pendingWindowOperation = PENDING_NULL;
        /* Force the size change event in case it was delivered earlier
         while the window was still animating into place.
         */
        [self windowDidResize:aNotification];
    }
}

- (void)windowWillExitFullScreen:(NSNotification *)aNotification
{
    isFullscreenSpace = NO;
    inFullscreenTransition = YES;
}

- (void)windowDidExitFullScreen:(NSNotification *)aNotification
{
    inFullscreenTransition = NO;
    
    [window setLevel:kCGNormalWindowLevel];
    
    if (pendingWindowOperation == PENDING_GOFULLSCREEN) {
        pendingWindowOperation = PENDING_NULL;
        [self setFullscreenSpace:YES];
    } else if (pendingWindowOperation == PENDING_MINIMIZE) {
        pendingWindowOperation = PENDING_NULL;
        [window miniaturize:nil];
    } else {
        [window setCollectionBehavior:NSWindowCollectionBehaviorManaged];
        [NSMenu setMenuBarVisible:YES];
        
        pendingWindowOperation = PENDING_NULL;
        /* Force the size change event in case it was delivered earlier
         while the window was still animating into place.
         */
        [self windowDidResize:aNotification];
    }
}

-(NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
  /*  if (m_pParent->GetDisplay()->GetFlags() & Burger::Display::FULLSCREEN) {
        return NSApplicationPresentationFullScreen | NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
    } else */
    {
        return proposedOptions;
    }
}


/* We'll respond to key events by doing nothing so we don't beep.
 * We could handle key messages here, but we lose some in the NSApp dispatch,
 * where they get converted to action messages, etc.
 */
- (void)flagsChanged:(NSEvent *)theEvent
{
    /*Cocoa_HandleKeyEvent(SDL_GetVideoDevice(), theEvent);*/
}
- (void)keyDown:(NSEvent *)theEvent
{
    /*Cocoa_HandleKeyEvent(SDL_GetVideoDevice(), theEvent);*/
}
- (void)keyUp:(NSEvent *)theEvent
{
    /*Cocoa_HandleKeyEvent(SDL_GetVideoDevice(), theEvent);*/
}

/* We'll respond to selectors by doing nothing so we don't beep.
 * The escape key gets converted to a "cancel" selector, etc.
 */
- (void)doCommandBySelector:(SEL)aSelector
{
    /*NSLog(@"doCommandBySelector: %@\n", NSStringFromSelector(aSelector));*/
}

- (void)mouseDown:(NSEvent *)theEvent
{
    int button;
    
    switch ([theEvent buttonNumber]) {
        case 0:
            if (([theEvent modifierFlags] & NSControlKeyMask) /* &&
                                                               GetHintCtrlClickEmulateRightClick() */ ) {
                                                                   wasCtrlLeft = YES;
                                                                   //                button = SDL_BUTTON_RIGHT;
                                                               } else {
                                                                   wasCtrlLeft = NO;
                                                                   //                button = SDL_BUTTON_LEFT;
                                                               }
            break;
        case 1:
            //            button = SDL_BUTTON_RIGHT;
            break;
        case 2:
            //            button = SDL_BUTTON_MIDDLE;
            break;
        default:
            //            button = [theEvent buttonNumber] + 1;
            break;
    }
    //    SDL_SendMouseButton(_data->window, 0, SDL_PRESSED, button);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
    [self mouseDown:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
    [self mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    NSInteger button;
    
    switch ([theEvent buttonNumber]) {
        case 0:
            if (wasCtrlLeft) {
                //                button = SDL_BUTTON_RIGHT;
                wasCtrlLeft = NO;
            } else {
                //                button = SDL_BUTTON_LEFT;
            }
            break;
        case 1:
            //            button = SDL_BUTTON_RIGHT;
            break;
        case 2:
            //            button = SDL_BUTTON_MIDDLE;
            break;
        default:
            button = [theEvent buttonNumber] + 1;
            break;
    }
    //    SDL_SendMouseButton(_data->window, 0, SDL_RELEASED, button);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
    [self mouseUp:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
    [self mouseUp:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    [self mouseMoved:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
    [self mouseMoved:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
    [self mouseMoved:theEvent];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    //    Cocoa_HandleMouseWheel(_data->window, theEvent);
}

@end

@interface Quake2Window : NSWindow
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (void)sendEvent:(NSEvent *)event;
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

- (void)sendEvent:(NSEvent *)event
{
    [super sendEvent:event];
    
    id delegate = [self delegate];
    if(![delegate isKindOfClass:[Quake2WindowListener class]])
    {
        return;
    }

    if ([delegate isMoving])
    {
        [delegate windowDidFinishMoving];
    }
}
@end

static Quake2AppDelegate *appDelegate = nil;
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
    appDelegate = [[Quake2AppDelegate alloc] init];
    windowListener = [[Quake2WindowListener alloc] init];
    NSApplication *app = [NSApplication sharedApplication];
    [app setDelegate:appDelegate];
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

    contentView = [[NSView alloc] initWithFrame:windowFrame];

    [window setContentView:contentView];
    [window setTitle:@"Quake 2 (Vulkan) "CPUSTRING];
   // [contentView release];
    [window makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];
    [windowListener listen:app];
//}
}

void MacOSDestroyWindow()
{
	[window close];
	window = nil;
	contentView = nil;
}

MetalView *AddMetalView()
{
    MetalView *mv = [[MetalView alloc] initWithFrame:contentView.frame];
    [contentView addSubview:mv];
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
