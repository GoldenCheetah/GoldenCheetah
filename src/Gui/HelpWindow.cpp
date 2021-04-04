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

#include "HelpWindow.h"
#include "MainWindow.h"

HelpWindow::HelpWindow(Context *context) : context(context)
{
    setWindowTitle(tr("Help Overview"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    // prepare the text
    fillPage();

    // layout the widget
    layout = new QVBoxLayout();
    layout->addWidget(textPage);
    setLayout(layout);

}

// prefer a tall slim label not a wide squat one
class TextPage : public QLabel
{
    public:
        TextPage(QWidget *parent) : QLabel(parent) {}
        int heightForWidth(int w) const { 
            return w; 
        }
        QSize sizeHint() const { return QSize(375,375); }
};

void
HelpWindow::fillPage() {

    textPage = new TextPage(this);
    textPage->setContentsMargins(0,0,0,0);
    textPage->setWordWrap(true);
    textPage->setOpenExternalLinks(true);
    textPage->setText(tr("<center>"
                    "<img src=\":images/gc.png\" height=80>"
                    "<h2>Help Options</h2>"
                    "<b><tt>\"Shift\"+\"F1\"</tt></b><br>provides a context specific short description of a feature"
                    " including the link to the Wiki page explaining more details<hr>"
                    "<br><a href=\"https://www.goldencheetah.org\">Golden Cheetah Website</a>"
                    "<br><a href=\"https://www.goldencheetah.org/#section-tutorials\">Golden Cheetah Website - Video Tutorials</a>"
                    "<br><a href=\"https://www.goldencheetah.org/#section-science\">Golden Cheetah Website - Science in Golden Cheetah</a>"
                    "<br><a href=\"https://github.com/GoldenCheetah/GoldenCheetah/wiki\">Wiki Overview</a>"
                    "<br><a href=\"https://github.com/GoldenCheetah/GoldenCheetah/wiki/UG_Main-Page_Users-Guide\">Wiki - User's Guide</a>"
                    "<br><a href=\"https://github.com/GoldenCheetah/GoldenCheetah/wiki/FAQ\">Wiki - Frequently Asked Questions</a>"

                ));
}
