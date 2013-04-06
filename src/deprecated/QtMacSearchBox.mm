/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "QtMacSearchBox.h"
#import <Cocoa/Cocoa.h>
#include <QtGui>
#include <QDebug>

#include <Carbon/Carbon.h>

// util to convert darwin to QString
static QString darwintoQString(NSString *string)
{
    if (!string) return QString();
    return QString::fromUtf8([string UTF8String]);
}
//
// We have a search delegate that gets notifed by the
// native search widget when the user does something
//
@interface QSearchDelegate : NSObject<NSTextFieldDelegate>
{
    @public
        // The QT widget that needs to emit signals
        QPointer<SearchWidget> qtw;
}

-(void)controlTextDidChange:(NSNotification*)notification;
-(void)controlTextDidEndEditing:(NSNotification*)notification;
@end

@implementation QSearchDelegate
-(void)controlTextDidChange:(NSNotification*)notification {
    if (qtw) qtw->textChanged(darwintoQString([[notification object] stringValue]));
}

-(void)controlTextDidEndEditing:(NSNotification*)notification {
    Q_UNUSED(notification);

    if (qtw) qtw->editFinished();

    if ([[[notification userInfo] objectForKey:@"NSTextMovement"] intValue] == NSReturnTextMovement)
        qtw->searchSubmit();
}
@end


SearchWidget::SearchWidget(QtMacSearchBox *parent)
    : QMacCocoaViewContainer(0, parent), parent(parent)
{
    setContentsMargins(0,0,0,0);

    // Setup the delegate
    QSearchDelegate *delegate = [[QSearchDelegate alloc] init];
    delegate->qtw = this;

    // Create the NSSearchField, set it on the QCocoaViewContainer.
    NSSearchField *search = [[NSSearchField alloc] init];
    [search setDelegate:delegate];
    setCocoaView(search);

    // Use a Qt menu for the search field menu.
    // We will leave that for now
    //QMenu *qtMenu = createMenu(this);
    //NSMenu *nsMenu = qtMenu->macMenu(0);
    //[[search cell] setSearchMenuTemplate:nsMenu];
}

SearchWidget::~SearchWidget()
{
}

QSize SearchWidget::sizeHint() const
{
    return QSize(250, 30);
}

void SearchWidget::textChanged(QString text)
{
    emit parent->textChanged(text);
}

void SearchWidget::searchSubmit()
{
    emit parent->searchSubmit();
}

void SearchWidget::editFinished()
{
    emit parent->editFinished();
}

QMenu *createMenu(QWidget *parent)
{
    QMenu *searchMenu = new QMenu(parent);
    
    QAction * indexAction = searchMenu->addAction("Index Search");
    indexAction->setCheckable(true);
    indexAction->setChecked(true);

    QAction * fulltextAction = searchMenu->addAction("Full Text Search");
    fulltextAction->setCheckable(true);

    QActionGroup *searchActionGroup = new QActionGroup(parent);
    searchActionGroup->addAction(indexAction);
    searchActionGroup->addAction(fulltextAction);
    searchActionGroup->setExclusive(true);
    
    return searchMenu;
}

QtMacSearchBox::QtMacSearchBox(QWidget *parent)
:QWidget(parent)
{
    s = new SearchWidget(this);
    setContentsMargins(0,0,0,0);
    //s->move(2,2);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QSize QtMacSearchBox::sizeHint() const
{
    return s->sizeHint() + QSize(6, 2);
}

