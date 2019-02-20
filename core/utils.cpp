/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "utils.h"
#include "utils_p.h"

#include "debug_p.h"
#include "settings_core.h"

#include <QRect>
#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QIODevice>
#include <QWindow>
#include <QScreen>



using namespace Okular;

QRect Utils::rotateRect( const QRect & source, int width, int height, int orientation )
{
    QRect ret;

    // adapt the coordinates of the boxes to the rotation
    switch ( orientation )
    {
        case 1:
            ret = QRect( width - source.y() - source.height(), source.x(),
                         source.height(), source.width() );
            break;
        case 2:
            ret = QRect( width - source.x() - source.width(), height - source.y() - source.height(),
                         source.width(), source.height() );
            break;
        case 3:
            ret = QRect( source.y(), height - source.x() - source.width(),
                         source.height(), source.width() );
            break;
        case 0:  // no modifications
        default: // other cases
            ret = source;
    }

    return ret;
}

QSizeF Utils::realDpi(QWidget* widgetOnScreen)
{
    const QScreen* screen = widgetOnScreen && widgetOnScreen->window() && widgetOnScreen->window()->windowHandle()
                          ? widgetOnScreen->window()->windowHandle()->screen()
                          : qGuiApp->primaryScreen();

    if (screen)
    {
        const QSizeF res(screen->physicalDotsPerInchX(), screen->physicalDotsPerInchY());
        if (res.width() > 0 && res.height() > 0) {
            if (qAbs(res.width() - res.height()) / qMin(res.height(), res.width()) < 0.05) {
                return res;
            } else {
                qCDebug(OkularCoreDebug) << "QScreen calculation returned a non square dpi." << res << ". Falling back";
            }
        }
    }
    return QSizeF(72, 72);
}

inline static bool isPaperColor( QRgb argb, QRgb paperColor ) {
    return ( argb & 0xFFFFFF ) == ( paperColor & 0xFFFFFF); // ignore alpha
}

NormalizedRect Utils::imageBoundingBox( const QImage * image )
{
    if ( !image )
        return NormalizedRect();

    const int width = image->width();
    const int height = image->height();
    const QRgb paperColor = SettingsCore::paperColor().rgb();
    int left, top, bottom, right, x, y;

#ifdef BBOX_DEBUG
    QTime time;
    time.start();
#endif

    // Scan pixels for top non-white
    for ( top = 0; top < height; ++top )
        for ( x = 0; x < width; ++x )
            if ( !isPaperColor( image->pixel( x, top ), paperColor ) )
                goto got_top;
    return NormalizedRect( 0, 0, 0, 0 ); // the image is blank
got_top:
    left = right = x;

    // Scan pixels for bottom non-white
    for ( bottom = height-1; bottom >= top; --bottom )
        for ( x = width-1; x >= 0; --x )
            if ( !isPaperColor( image->pixel( x, bottom ), paperColor ) )
                goto got_bottom;
    Q_ASSERT( 0 ); // image changed?!
got_bottom:
    if ( x < left )
        left = x;
    if ( x > right )
        right = x;

    // Scan for leftmost and rightmost (we already found some bounds on these):
    for ( y = top; y <= bottom && ( left > 0 || right < width-1 ); ++y )
    {
        for ( x = 0; x < left; ++x )
            if ( !isPaperColor( image->pixel( x, y ), paperColor ) )
                left = x;
        for ( x = width-1; x > right+1; --x )
            if ( !isPaperColor( image->pixel( x, y ), paperColor ) )
                right = x;
    }

    NormalizedRect bbox( QRect( left, top, ( right - left + 1), ( bottom - top + 1 ) ),
                         image->width(), image->height() );

#ifdef BBOX_DEBUG
    qCDebug(OkularCoreDebug) << "Computed bounding box" << bbox << "in" << time.elapsed() << "ms";
#endif

    return bbox;
}

void Okular::copyQIODevice( QIODevice *from, QIODevice *to )
{
    QByteArray buffer( 65536, '\0' );
    qint64 read = 0;
    qint64 written = 0;
    while ( ( read = from->read( buffer.data(), buffer.size() ) ) > 0 )
    {
        written = to->write( buffer.constData(), read );
        if ( read != written )
            break;
    }
}

QTransform Okular::buildRotationMatrix(Rotation rotation)
{
    QTransform matrix;
    matrix.rotate( (int)rotation * 90 );

    switch ( rotation )
    {
        case Rotation90:
            matrix.translate( 0, -1 );
            break;
        case Rotation180:
            matrix.translate( -1, -1 );
            break;
        case Rotation270:
            matrix.translate( -1, 0 );
            break;
        default: ;
    }

    return matrix;
}

/* kate: replace-tabs on; indent-width 4; */
