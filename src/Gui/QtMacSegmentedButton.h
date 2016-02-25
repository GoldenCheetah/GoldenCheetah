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

#ifndef GC_QtMacSegmentedButton_h
#define GC_QtMacSegmentedButton_h

#include <QMacCocoaViewContainer>

// macros for compile time, depending if included in an obj-c
// or a c++ source file. Changes declaration of class types.
#ifdef __OBJC__
# define ADD_COCOA_NATIVE_REF(CocoaClass) \
    @class CocoaClass; \
    typedef CocoaClass *Native##CocoaClass##Ref
#else /* __OBJC__ */
# define ADD_COCOA_NATIVE_REF(CocoaClass) typedef void *Native##CocoaClass##Ref
#endif /* __OBJC__ */

ADD_COCOA_NATIVE_REF (NSAutoreleasePool);
class CocoaInitializer
{
    public:
        CocoaInitializer();
        ~CocoaInitializer();

    private:
    NativeNSAutoreleasePoolRef pool;
};

ADD_COCOA_NATIVE_REF (NSSegmentedControl);
class QtMacSegmentedButton : public QMacCocoaViewContainer
{
    Q_OBJECT;

public:
    QtMacSegmentedButton (int aCount, QWidget *aParent = 0);
    QSize sizeHint() const;

    void setTitle (int aSegment, const QString &aTitle);
    void setToolTip (int aSegment, const QString &aTip);
    void setEnabled (int aSegment, bool fEnabled);
    void setSelected(int index, bool value) const;
    void animateClick (int aSegment);
    void onClicked (int aSegment);
    void setImage(int index, const QPixmap *image, int swidth = 35);
    void setNoSelect();

    void setWidth(int);
signals:
    void clicked (int aSegment, bool aChecked = false);

private:
    /* Private member vars */
    NativeNSSegmentedControlRef mNativeRef;
    int segments;
    bool icons;
    int width;
};

#endif
