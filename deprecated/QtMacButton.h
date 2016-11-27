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

#ifndef GC_QtMacButton_h
#define GC_QtMacButton_h

#include <QWidget>
#include <QVBoxLayout>
#include <QPointer>
#include <QMacCocoaViewContainer>


// Qocoa already dit this, so re-used their code, saved a bit of effort!
class QtMacButtonWidget;
class QtMacButton : public QWidget
{
    Q_OBJECT
public:
    // Matches NSBezelStyle
    enum BezelStyle {
       Rounded           = 1,
       RegularSquare     = 2,
       Disclosure        = 5,
       ShadowlessSquare  = 6,
       Circular          = 7,
       TexturedSquare    = 8,
       HelpButton        = 9,
       SmallSquare       = 10,
       TexturedRounded   = 11,
       RoundRect         = 12,
       Recessed          = 13,
       RoundedDisclosure = 14,
#ifdef __MAC_10_7
       Inline            = 15
#endif
    };

    explicit QtMacButton(QWidget *parent, BezelStyle bezelStyle = Rounded);
    explicit QtMacButton(QString text, QWidget *parent);

public slots:
    void setText(const QString &text);
    void setToolTip(const QString &text);
    void setImage(const QPixmap *image);
    void setChecked(bool checked);
    void setWidth(int x);
    void setSelected(bool x);
    void setIconAndText();

public:
    void setCheckable(bool checkable);
    bool isChecked();

signals:
    void clicked(bool checked = false);

private:
    friend class QtMacButtonWidget;
    QPointer<QtMacButtonWidget> qtw;
    int width;
};

#endif
