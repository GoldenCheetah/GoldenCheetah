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

#include "QtMacButton.h"

#if QT_VERSION > 0x050000
// mac specials
#include <QtWidgets>
#include <qmacfunctions.h>
#endif

#include <Cocoa/Cocoa.h>
#include <QMacCocoaViewContainer>

#import "AppKit/NSButton.h"
#import "AppKit/NSFont.h"

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

static inline NSString* fromQString(const QString &string)
{
    const QByteArray utf8 = string.toUtf8();
    const char* cString = utf8.constData();
    return [[NSString alloc] initWithUTF8String:cString];
}

// nice little trick from qocoa
static inline void setupLayout(NSView *cocoaView, QWidget *parent)
{
    //parent->setAttribute(Qt::WA_NativeWindow);
    QVBoxLayout *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    QMacCocoaViewContainer *cont = new QMacCocoaViewContainer(0, parent);
    cont->setCocoaView(cocoaView);
    layout->addWidget(cont);
}

// Lets wrap up all the NSButton complexity in this private
class QtMacButtonWidget : public QObject
{
public:
    QtMacButtonWidget(QtMacButton *qButton, NSButton *nsButton, QtMacButton::BezelStyle bezelStyle)
        : QObject(qButton), qButton(qButton), nsButton(nsButton)
    {
        switch(bezelStyle) {
            case QtMacButton::Disclosure:
            case QtMacButton::Circular:
#ifdef __MAC_10_7
            case QtMacButton::Inline:
#endif
            case QtMacButton::RoundedDisclosure:
            case QtMacButton::HelpButton:
                [nsButton setTitle:@""];
            default:
                break;
        }

        NSFont* font = 0;
        switch(bezelStyle) {
            case QtMacButton::RoundRect:
                font = [NSFont fontWithName:@"Lucida Grande" size:10];
                break;

            case QtMacButton::Recessed:
                font = [NSFont fontWithName:@"Lucida Grande Bold" size:10];
                break;

#ifdef __MAC_10_7
            case QtMacButton::Inline:
                font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]];
                break;
#endif

            default:
                font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSRegularControlSize]];
                break;
        }
        [nsButton setFont:font];

        switch(bezelStyle) {
            case QtMacButton::Rounded:
                qButton->setMinimumWidth(40);
                qButton->setFixedHeight(24);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacButton::RegularSquare:
            case QtMacButton::TexturedSquare:
                qButton->setMinimumSize(14, 23);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacButton::ShadowlessSquare:
                qButton->setMinimumSize(5, 25);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacButton::SmallSquare:
                qButton->setMinimumSize(4, 21);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacButton::TexturedRounded:
                qButton->setMinimumSize(40, 25);
                qButton->setMaximumSize(40, 25);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacButton::RoundRect:
            case QtMacButton::Recessed:
                qButton->setFixedSize(50,18);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacButton::Disclosure:
                qButton->setMinimumWidth(13);
                qButton->setFixedHeight(13);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacButton::Circular:
                qButton->setMinimumSize(16, 16);
                qButton->setMaximumHeight(40);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacButton::HelpButton:
            case QtMacButton::RoundedDisclosure:
                qButton->setMinimumWidth(22);
                qButton->setFixedHeight(22);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#ifdef __MAC_10_7
            case QtMacButton::Inline:
                qButton->setMinimumWidth(10);
                qButton->setFixedHeight(16);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#endif
        }

        switch(bezelStyle) {
            case QtMacButton::Recessed:
                [nsButton setButtonType:NSOnOffButton];
                [[nsButton cell] setGradientType:NSGradientConvexStrong ];
                [nsButton setShowsBorderOnlyWhileMouseInside:true ];
                [[nsButton cell] setBackgroundStyle:NSBackgroundStyleRaised];
                //[nsButton setButtonType:NSPushOnPushOffButton];
                break;
            case QtMacButton::Disclosure:
                [nsButton setButtonType:NSOnOffButton];
                break;
            default:
                [nsButton setButtonType:NSMomentaryPushInButton];
                break;
        }

        [nsButton setBezelStyle:static_cast<NSBezelStyle>(bezelStyle)];
        [nsButton setState:false ];
    }

    void clicked()
    {
        emit qButton->clicked(qButton->isChecked());
    }

    ~QtMacButtonWidget() {
        [[nsButton target] release];
        [nsButton setTarget:nil];
    }

    QtMacButton *qButton;
    NSButton *nsButton;
};

@interface QtMacButtonTarget : NSObject
{
@public
    QPointer<QtMacButtonWidget> qtw;
}
-(void)clicked;
@end

@implementation QtMacButtonTarget
-(void)clicked {
    Q_ASSERT(qtw);
    if (qtw)
        qtw->clicked();
}
@end

QtMacButton::QtMacButton(QString text, QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    NSButton *button = [[[NSButton alloc] init] autorelease];
    qtw = new QtMacButtonWidget(this, button, QtMacButton::TexturedRounded);
    setText(text);
    QtMacButtonTarget *target = [[QtMacButtonTarget alloc] init];
    target->qtw = qtw;

    [button setShowsBorderOnlyWhileMouseInside:true ];
    [button setFont:[NSFont fontWithName:@"Helvetica" size:12]];
    [button setTarget:target];

    [button setAction:@selector(clicked)];
    setupLayout(button, this);
}

QtMacButton::QtMacButton(QWidget *parent, BezelStyle bezelStyle) : QWidget(parent), width(-1)
{
    setContentsMargins(0,0,0,0);

    NSButton *button = [[[NSButton alloc] init] autorelease];
    qtw = new QtMacButtonWidget(this, button, bezelStyle);

    QtMacButtonTarget *target = [[QtMacButtonTarget alloc] init];
    target->qtw = qtw;
    [button setTarget:target];

    [button setAction:@selector(clicked)];
    setupLayout(button, this);
}

void QtMacButton::setWidth(int x)
{
    setFixedWidth(x);
}

void QtMacButton::setIconAndText()
{
    [[qtw->nsButton cell] setImagePosition:NSImageLeft ];
}

void QtMacButton::setToolTip(const QString &text)
{
    [qtw->nsButton setToolTip: fromQString(text)];
}

void QtMacButton::setText(const QString &text)
{
    Q_ASSERT(qtw);
    if (!qtw)
        return;

    [qtw->nsButton setTitle:fromQString(text)];
}

void QtMacButton::setSelected(bool x)
{
    Q_ASSERT(qtw);
    if (qtw) {
        [qtw->nsButton setButtonType:NSOnOffButton];
        [qtw->nsButton setState:(x ? NSOnState : NSOffState)];
    }
}

void QtMacButton::setImage(const QPixmap *image)
{
    Q_ASSERT(qtw);
    if (qtw) {
        [qtw->nsButton setImage:fromQPixmap(image)];
        [qtw->nsButton setAlternateImage:fromQPixmap(image)];
        [qtw->nsButton setButtonType:NSMomentaryPushButton];
    }
}

void QtMacButton::setChecked(bool checked)
{
    Q_ASSERT(qtw);
    if (qtw)
        [qtw->nsButton setState:checked];
}

void QtMacButton::setCheckable(bool checkable)
{
    const NSInteger cellMask = checkable ? NSChangeBackgroundCellMask : NSNoCellMask;

    Q_ASSERT(qtw);
    if (qtw)
        [[qtw->nsButton cell] setShowsStateBy:cellMask];
}

bool QtMacButton::isChecked()
{
    Q_ASSERT(qtw);
    if (!qtw)
        return false;

    return [qtw->nsButton state];
}
