/*
* Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

//avoid including these in the main headers since all the
//Objective-C syntax borks the Qt Meta Compiler (moc)
//we also include them before the QT headers to avoid conflicts
//between QT keywords 'slots' and a header definition in CALayer.h
#include <QTKit/QTKit.h>
#include <QTkit/QTMovie.h>
#include <QTkit/QTMovieView.h>

#include "QtMacVideoWindow.h"

VideoWindow::VideoWindow(MainWindow *parent, const QDir &home)  :
GcWindow(parent), home(home), main(parent), hasMovie(false)
{

    setControls(NULL);
    setInstanceName("Video Window");
    setProperty("color", Qt::black);

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);

    movie = NULL; // no movie allocated at present
    player = new QtMacMovieView(this);
    player->show();

    layout->addWidget(player);

    connect(main, SIGNAL(stop()), this, SLOT(stopPlayback()));
    connect(main, SIGNAL(start()), this, SLOT(startPlayback()));
    connect(main, SIGNAL(pause()), this, SLOT(pausePlayback()));
    connect(main, SIGNAL(unpause()), this, SLOT(resumePlayback()));
    connect(main, SIGNAL(seek(long)), this, SLOT(seekPlayback(long)));
    connect(main, SIGNAL(mediaSelected(QString)), this, SLOT(mediaSelected(QString)));

}

VideoWindow::~VideoWindow()
{
    stopPlayback();
}

void VideoWindow::resizeEvent(QResizeEvent * )
{
}

void VideoWindow::startPlayback()
{
    if (!hasMovie) return; // no movie selected

    [movie gotoBeginning];
    [movie play];
}

void VideoWindow::stopPlayback()
{
    if (!hasMovie) return; // no movie selected

    [movie stop];
    [movie gotoBeginning];
}

void VideoWindow::pausePlayback()
{
    if (!hasMovie) return; // no movie selected

    [movie stop];
}

void VideoWindow::resumePlayback()
{
    if (!hasMovie) return; // no movie selected

    [movie play];
}

void VideoWindow::seekPlayback(long ms)
{
    if (!hasMovie) return;

    QTTime newTime = QTMakeTime(ms,(long)1000);
    [movie setCurrentTime:newTime];
}


static inline NSString *darwinQStringToNSString (const QString &aString)
{
    return [reinterpret_cast<const NSString *> (CFStringCreateWithCharacters
            (0, reinterpret_cast<const UniChar *> (aString.unicode()), aString.length())) autorelease];
}

void VideoWindow::mediaSelected(QString filename)
{
    NativeQTMovieRef old = movie; // so we can invalidate once View has been reset

    // stop any current playback
    if (hasMovie) {
        stopPlayback();
        hasMovie = false;
        movie = NULL;
    }

    // open the movie file
    if (filename != "" && QFile(filename).exists()) {

        // open async to use Quickime X - major performance improvment
        NSError *error = nil;
        NSNumber *num = [NSNumber numberWithBool:YES];
        NSString *file = ::darwinQStringToNSString(filename);
        NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
            file, QTMovieFileNameAttribute,
            num, QTMovieLoopsAttribute,
#ifdef GC_HAVE_LION
            num, QTMovieOpenForPlaybackAttribute,
#endif
            nil];

        movie = [[QTMovie alloc] initWithAttributes:attributes error:&error];
        player->setMovie(movie);

        hasMovie = true;
    }

    if (old) [old invalidate];
}

MediaHelper::MediaHelper() { }

MediaHelper::~MediaHelper() { }

// convert an NSString to a QString
static QString qt_mac_NSStringToQString(const NSString *nsstr)
{
    NSRange range;
    range.location = 0;
    range.length = [nsstr length];
    QString result(range.length, QChar(0));
 
    unichar *chars = new unichar[range.length];
    [nsstr getCharacters:chars range:range];
    result = QString::fromUtf16(chars, range.length);
    delete[] chars;
    return result;
}

QStringList 
MediaHelper::listMedia(QDir dir)
{
    QStringList supported;
    QStringList returning;

    // get a QTMove object to get the extensions we need
    NSArray *types = [QTMovie movieFileTypes:QTIncludeCommonTypes];
    for (unsigned int i=0; i<[types count]; ++i) {
        QString type = qt_mac_NSStringToQString([types objectAtIndex:i]);

        if (type.startsWith("'")) continue; // skip 'xxx ' types

        supported << QString(".%1").arg(type); // .xxx added
    }

    // whizz through every file in the directory
    // if it has the right extension then we are happy
    foreach(QString name, dir.entryList()) {

        foreach(QString extension, supported) {
            if (name.endsWith(extension, Qt::CaseInsensitive)) {
                returning << name;
                break;
            }
        }
    }

    return returning;
}

QtMacMovieView::QtMacMovieView (QWidget *parent) : QMacCocoaViewContainer (0, parent)
{
#if QT_VERSION >= 0x040800 // see QT-BUG 22574, QMacCocoaContainer on 4.8 is "broken"
    setAttribute(Qt::WA_NativeWindow);
#endif

    NSRect frame;
    // allocate the player
    player = [[QTMovieView alloc] initWithFrame:frame];
    [player setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    [player setPreservesAspectRatio:YES];
    [player setControllerVisible:NO];

    setCocoaView (player);
}

void
QtMacMovieView::setMovie(NativeQTMovieRef movie)
{
    [player setMovie:movie];
}
