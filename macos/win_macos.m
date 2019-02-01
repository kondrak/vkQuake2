#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include "win_macos.h"


// note that it has to be a .m file instead of .mm - .m symbols are visible in .c while .mm are visible in .cpp only ¯\_(ツ)_/¯

//@interface Quake2Application : NSApplication
//@end

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
    //    SDL_SendWindowEvent(_data->window, SDL_WINDOWEVENT_CLOSE, 0, 0);
    return NO;
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

void MacOSHandleEvents()
{
    if(window == nil) return;
    for (;;) {
        NSEvent *pEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES ];
        if (!pEvent) {
            break;
        }
        
        switch ([pEvent type]) {
            case NSEventTypeLeftMouseDown:
            case NSEventTypeOtherMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeLeftMouseUp:
            case NSEventTypeOtherMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged: /* usually middle mouse dragged */
            case NSEventTypeMouseMoved:
            case NSEventTypeScrollWheel:
                // if (pApp->m_pMouse) {
                //    Cocoa_HandleMouseEvent(_this, event);
                // }
                break;
            case NSEventTypeKeyDown:
            case NSEventTypeKeyUp:
            case NSEventTypeFlagsChanged:
              /*  [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
                [window setFrame:[[NSScreen mainScreen] frame] display:YES];
                NSRect r = [window contentRectForFrameRect:[window frame]];
                contentView = [[NSView alloc] initWithFrame:r];
                [window setContentView:contentView];
                [window toggleFullScreen:nil]; */
                //if (pApp->m_pKeyboard) {
                //   pApp->m_pKeyboard->ProcessEvent(pEvent);
                // }
                break;
            default:
                break;
        }
        // Send the event to the operating system
        [NSApp sendEvent:pEvent];
    }
}

void MacOSCreateWindow(int x, int y, int w, int h)
{ //@autoreleasepool {
    if(window != nil) {
        return;
    }
    appDelegate = [[Quake2AppDelegate alloc] init];
    windowListener = [[Quake2WindowListener alloc] init];
    NSApplication *app = [NSApplication sharedApplication];
    [app setDelegate:appDelegate];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    NSUInteger windowStyle = NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable;
    NSRect r;

    CGFloat s = [[NSScreen mainScreen] backingScaleFactor];
    window = [[Quake2Window alloc] initWithContentRect:NSMakeRect(x, y, w/s, h/s) styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
    
    r = [window contentRectForFrameRect:[window frame]];
    contentView = [[NSView alloc] initWithFrame:r];
    
    [window setContentView:contentView];
    [window setTitle:@"Quake 2 (Vulkan) "CPUSTRING];
   // [contentView release];
    [window makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];
    [windowListener listen:app];
//}
}

MetalView *AddMetalView()
{
    NSView *view = contentView;
    MetalView *mv = [[MetalView alloc] initWithFrame:view.frame];
    [view addSubview:mv];
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
