/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_fax.h"

#include <QtGui/QPainter>
#include <QtPrintSupport/QPrinter>

#include <KAboutData>
#include <KLocalizedString>

#include <core/document.h>
#include <core/page.h>

OKULAR_EXPORT_PLUGIN(FaxGenerator, "libokularGenerator_fax.json")

FaxGenerator::FaxGenerator( QObject *parent, const QVariantList &args )
    : Generator( parent, args )
{
    setFeature( Threaded );
    setFeature( PrintNative );
    setFeature( PrintToFile );
}

FaxGenerator::~FaxGenerator()
{
}

bool FaxGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    if ( fileName.toLower().endsWith( QLatin1String(".g3") ) )
        m_type = FaxDocument::G3;
    else
        m_type = FaxDocument::G4;

    FaxDocument faxDocument( fileName, m_type );

    if ( !faxDocument.load() )
    {
        emit error( i18n( "Unable to load document" ), -1 );
        return false;
    }

    m_img = faxDocument.image();

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool FaxGenerator::doCloseDocument()
{
    m_img = QImage();

    return true;
}

QImage FaxGenerator::image( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    int width = request->width();
    int height = request->height();
    if ( request->page()->rotation() % 2 == 1 )
        qSwap( width, height );

    return m_img.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}

Okular::DocumentInfo FaxGenerator::generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const
{
    Okular::DocumentInfo docInfo;
    if ( keys.contains( Okular::DocumentInfo::MimeType ) )
    {
        if ( m_type == FaxDocument::G3 )
            docInfo.set( Okular::DocumentInfo::MimeType, QStringLiteral("image/fax-g3") );
        else
            docInfo.set( Okular::DocumentInfo::MimeType, QStringLiteral("image/fax-g4") );
    }
    return docInfo;
}

bool FaxGenerator::print( QPrinter& printer )
{
    QPainter p( &printer );

    QImage image( m_img );

    if ( ( image.width() > printer.width() ) || ( image.height() > printer.height() ) )

        image = image.scaled( printer.width(), printer.height(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation );

    p.drawImage( 0, 0, image );

    return true;
}

#include "generator_fax.moc"

