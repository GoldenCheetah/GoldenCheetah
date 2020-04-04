#include "NewMainWindow.h"
#include <Cocoa/Cocoa.h>

void
NewMainWindow::macNativeSettings()
{
    NSView *nativeView = reinterpret_cast<NSView *>(winId());
    NSWindow* nativeWindow = [nativeView window];

    [nativeWindow setStyleMask: [nativeWindow styleMask] | NSFullSizeContentViewWindowMask | NSWindowTitleHidden];
    [nativeWindow setTitlebarAppearsTransparent:YES];
    [nativeWindow setMovableByWindowBackground:YES];
}
