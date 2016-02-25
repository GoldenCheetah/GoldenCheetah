/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "QtMacSegmentedButton.h"

#if QT_VERSION > 0x050000
// mac specials
#include <qmacfunctions.h>
#endif

#import <AppKit/NSButton.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSSegmentedControl.h>
#import <AppKit/NSSegmentedCell.h>
#import <AppKit/NSBezierPath.h>

CocoaInitializer::CocoaInitializer()
{
    pool = [[NSAutoreleasePool alloc] init];
    NSApplicationLoad();
}

CocoaInitializer::~CocoaInitializer()
{
    [pool release];
}

static inline NSString *darwinQStringToNSString (const QString &aString)
{
    return (NSString*)CFStringCreateWithCharacters
    (0, reinterpret_cast<const UniChar *> (aString.unicode()), aString.length());
}

static NSImage *fromQPixmap(const QPixmap *pixmap)
{
#if QT_VERSION > 0x050000
    CGImageRef cgImage = QtMac::toCGImageRef(*pixmap);
#else
    CGImageRef cgImage = pixmap->toMacCGImageRef();
#endif
    NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
    NSImage *image = [[[NSImage alloc] init] autorelease];
    [image addRepresentation:bitmapRep];
    [bitmapRep release];
    [image setTemplate:true];
    return image;
}

/*----------------------------------------------------------------------
 * QtMacSegmented Button
 *----------------------------------------------------------------------*/

/* Define the interface */
@interface NSSegmentedButtonTarget: NSObject
{
    QtMacSegmentedButton *mRealTarget;
}
-(id)initWithObject1:(QtMacSegmentedButton*)object;
-(IBAction)segControlClicked:(id)sender;
@end

@implementation NSSegmentedButtonTarget
-(id)initWithObject1:(QtMacSegmentedButton*)object
{
    self = [super init];
    mRealTarget = object;
    return self;
}

-(IBAction)segControlClicked:(id)sender
{
    mRealTarget->onClicked([sender selectedSegment]);
}
@end


QtMacSegmentedButton::QtMacSegmentedButton (int aCount, QWidget *aParent /* = 0 */)
  : QMacCocoaViewContainer (0, aParent), segments(aCount), icons(false), width(-1)
{
    setContentsMargins(0,0,0,0);

#if QT_VERSION >= 0x040800 // see QT-BUG 22574, QMacCocoaContainer on 4.8 is "broken"
    //setAttribute(Qt::WA_NativeWindow);
#endif
    mNativeRef = [[[NSSegmentedControl alloc] init] autorelease];
    [mNativeRef setSegmentCount:aCount];
    [mNativeRef setSegmentStyle:NSSegmentStyleTexturedRounded];
    [[mNativeRef cell] setTrackingMode: NSSegmentSwitchTrackingSelectOne];
    [mNativeRef setFont: [NSFont controlContentFontOfSize:
        [NSFont systemFontSizeForControlSize: NSSmallControlSize]]];
    [mNativeRef sizeToFit];

    NSSegmentedButtonTarget *bt = [[[NSSegmentedButtonTarget alloc] initWithObject1:this] autorelease];
    [mNativeRef setTarget:bt];

    [mNativeRef setAction:@selector(segControlClicked:)];

    NSRect frame = [mNativeRef frame];
    resize (frame.size.width, frame.size.height);
    setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCocoaView (mNativeRef);
}

void QtMacSegmentedButton::setWidth(int x)
{
    width = x;
}

QSize QtMacSegmentedButton::sizeHint() const
{
    NSRect frame = [mNativeRef frame];
    return (width == -1 ? QSize (frame.size.width, frame.size.height) :  // +65 is a hack ... XXX fixme soon!
                   QSize (width, frame.size.height));
}

void QtMacSegmentedButton::setNoSelect()
{
    [[mNativeRef cell] setTrackingMode: NSSegmentSwitchTrackingMomentary];
}

void QtMacSegmentedButton::setSelected(int index, bool value) const
{
    [mNativeRef setSelected:value forSegment:index];
}

void QtMacSegmentedButton::setImage(int index, const QPixmap *image, int swidth)
{
    [mNativeRef setImage:fromQPixmap(image) forSegment:index];
    [mNativeRef setWidth:swidth forSegment:index];
    icons = true;
}

void QtMacSegmentedButton::setTitle (int aSegment, const QString &aTitle)
{
    QString s (aTitle);
    [mNativeRef setLabel: ::darwinQStringToNSString (s.remove ('&')) forSegment: aSegment];
    [mNativeRef sizeToFit];
    NSRect frame = [mNativeRef frame];
    resize (frame.size.width, frame.size.height);
}

void QtMacSegmentedButton::setToolTip (int aSegment, const QString &aTip)
{
    [[mNativeRef cell] setToolTip: ::darwinQStringToNSString (aTip) forSegment: aSegment];
}

void QtMacSegmentedButton::setEnabled (int aSegment, bool fEnabled)
{
    [[mNativeRef cell] setEnabled: fEnabled forSegment: aSegment];
}

void QtMacSegmentedButton::animateClick (int aSegment)
{
    [mNativeRef setSelectedSegment: aSegment];
    [[mNativeRef cell] performClick: mNativeRef];
}

void QtMacSegmentedButton::onClicked (int aSegment)
{
    emit clicked (aSegment, false);
}
