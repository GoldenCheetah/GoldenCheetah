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

#include "QtMacPopUpButton.h"

#import "AppKit/NSPopUpButton.h"
#import "AppKit/NSFont.h"

static NSImage *fromQPixmap(const QPixmap &pixmap)
{
    NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage:pixmap.toMacCGImageRef()];
    NSImage *image = [[NSImage alloc] init];
    [image addRepresentation:bitmapRep];
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
static inline void setupLayout(void *cocoaView, QWidget *parent)
{
    parent->setAttribute(Qt::WA_NativeWindow);
    QVBoxLayout *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(new QMacCocoaViewContainer(cocoaView, parent));
}

// Lets wrap up all the NSPopUpButton complexity in this private
class QtMacPopUpButtonWidget : public QObject
{
public:
    QtMacPopUpButtonWidget(QtMacPopUpButton *qPopUpButton, NSPopUpButton *nsPopUpButton, QtMacPopUpButton::BezelStyle bezelStyle)
        : QObject(qPopUpButton), qPopUpButton(qPopUpButton), nsPopUpButton(nsPopUpButton)
    {
        switch(bezelStyle) {
            case QtMacPopUpButton::Disclosure:
            case QtMacPopUpButton::Circular:
#ifdef __MAC_10_7
            case QtMacPopUpButton::Inline:
#endif
            case QtMacPopUpButton::RoundedDisclosure:
            case QtMacPopUpButton::HelpPopUpButton:
                [nsPopUpButton setTitle:@""];
            default:
                break;
        }

        NSFont* font = 0;
        switch(bezelStyle) {
            case QtMacPopUpButton::RoundRect:
                font = [NSFont fontWithName:@"Lucida Grande" size:12];
                break;

            case QtMacPopUpButton::Recessed:
                font = [NSFont fontWithName:@"Lucida Grande Bold" size:12];
                break;

#ifdef __MAC_10_7
            case QtMacPopUpButton::Inline:
                font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]];
                break;
#endif

            default:
                font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSRegularControlSize]];
                break;
        }
        [nsPopUpButton setFont:font];

        switch(bezelStyle) {
            case QtMacPopUpButton::Rounded:
                qPopUpButton->setMinimumWidth(40);
                qPopUpButton->setFixedHeight(24);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacPopUpButton::RegularSquare:
            case QtMacPopUpButton::TexturedSquare:
                qPopUpButton->setMinimumSize(14, 23);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacPopUpButton::ShadowlessSquare:
                qPopUpButton->setMinimumSize(5, 25);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacPopUpButton::SmallSquare:
                qPopUpButton->setMinimumSize(4, 21);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacPopUpButton::TexturedRounded:
                qPopUpButton->setMinimumSize(40, 25);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacPopUpButton::RoundRect:
            case QtMacPopUpButton::Recessed:
                qPopUpButton->setMinimumWidth(16);
                qPopUpButton->setFixedHeight(18);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacPopUpButton::Disclosure:
                qPopUpButton->setMinimumWidth(13);
                qPopUpButton->setFixedHeight(13);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QtMacPopUpButton::Circular:
                qPopUpButton->setMinimumSize(16, 16);
                qPopUpButton->setMaximumHeight(40);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QtMacPopUpButton::HelpPopUpButton:
            case QtMacPopUpButton::RoundedDisclosure:
                qPopUpButton->setMinimumWidth(22);
                qPopUpButton->setFixedHeight(22);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#ifdef __MAC_10_7
            case QtMacPopUpButton::Inline:
                qPopUpButton->setMinimumWidth(10);
                qPopUpButton->setFixedHeight(16);
                qPopUpButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#endif
        }

        [nsPopUpButton setBezelStyle:bezelStyle];
    }

    void clicked()
    {
        emit qPopUpButton->clicked(qPopUpButton->isChecked());
    }

    ~QtMacPopUpButtonWidget() {
        [[nsPopUpButton target] release];
        [nsPopUpButton setTarget:nil];
    }

    QtMacPopUpButton *qPopUpButton;
    NSPopUpButton *nsPopUpButton;
};

@interface QtMacPopUpButtonTarget : NSObject
{
@public
    QPointer<QtMacPopUpButtonWidget> qtw;
}
-(void)clicked;
@end

@implementation QtMacPopUpButtonTarget
-(void)clicked {
    Q_ASSERT(qtw);
    if (qtw)
        qtw->clicked();
}
@end

QtMacPopUpButton::QtMacPopUpButton(QWidget *parent, BezelStyle bezelStyle) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    NSPopUpButton *button = [[NSPopUpButton alloc] init];
    qtw = new QtMacPopUpButtonWidget(this, button, bezelStyle);

    QtMacPopUpButtonTarget *target = [[QtMacPopUpButtonTarget alloc] init];
    target->qtw = qtw;
    [button setTarget:target];

    [button setAction:@selector(clicked)];

    setupLayout(button, this);
}

void QtMacPopUpButton::setToolTip(const QString &text)
{
    [qtw->nsPopUpButton setToolTip: fromQString(text)];
}

void QtMacPopUpButton::setText(const QString &text)
{
    Q_ASSERT(qtw);
    if (!qtw)
        return;

    [qtw->nsPopUpButton setTitle:fromQString(text)];
}

void QtMacPopUpButton::setImage(const QPixmap &image)
{
    Q_ASSERT(qtw);
    if (qtw) {
        [qtw->nsPopUpButton setImage:fromQPixmap(image)];
        [qtw->nsPopUpButton setAlternateImage:fromQPixmap(image)];
    }
}

void QtMacPopUpButton::setChecked(bool checked)
{
    Q_ASSERT(qtw);
    if (qtw)
        [qtw->nsPopUpButton setState:checked];
}

void QtMacPopUpButton::setCheckable(bool checkable)
{
    const NSInteger cellMask = checkable ? NSChangeBackgroundCellMask : NSNoCellMask;

    Q_ASSERT(qtw);
    if (qtw)
        [[qtw->nsPopUpButton cell] setShowsStateBy:cellMask];
}

bool QtMacPopUpButton::isChecked()
{
    Q_ASSERT(qtw);
    if (!qtw)
        return false;

    return [qtw->nsPopUpButton state];
}

void QtMacPopUpButton::setMenu(QMenu *menu)
{
    NSMenu *nsMenu = menu->macMenu(0);
    [qtw->nsPopUpButton setMenu:nsMenu];
    [qtw->nsPopUpButton setPullsDown:true];
}

