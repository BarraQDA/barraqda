/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "unrarflavours.h"

#include <QRegExp>

ProcessArgs::ProcessArgs()
{

}

ProcessArgs::ProcessArgs( const QStringList &args, bool lsar )
    : appArgs { args }, useLsar { lsar }
{

}

UnrarFlavour::UnrarFlavour()
{
}

UnrarFlavour::~UnrarFlavour()
{
}

void UnrarFlavour::setFileName( const QString &fileName )
{
    mFileName = fileName;
}

QString UnrarFlavour::fileName() const
{
    return mFileName;
}


NonFreeUnrarFlavour::NonFreeUnrarFlavour()
    : UnrarFlavour()
{
}

QStringList NonFreeUnrarFlavour::processListing( const QStringList &data )
{
    // unrar-nonfree just lists the files
    return data;
}

QString NonFreeUnrarFlavour::name() const
{
    return QStringLiteral("unrar-nonfree");
}

ProcessArgs NonFreeUnrarFlavour::processListArgs( const QString &fileName ) const
{
    return ProcessArgs ( QStringList() << QStringLiteral("lb") << fileName, false );
}

ProcessArgs NonFreeUnrarFlavour::processOpenArchiveArgs( const QString &fileName, const QString &path ) const
{
    return ProcessArgs ( QStringList() << QStringLiteral("e") << fileName << path +  QLatin1Char('/'), false );
}

FreeUnrarFlavour::FreeUnrarFlavour()
    : UnrarFlavour()
{
}

QStringList FreeUnrarFlavour::processListing( const QStringList &data )
{
    QRegExp re( QStringLiteral("^ ([^/]+/([^\\s]+))$") );

    QStringList newdata;
    foreach ( const QString &line, data )
    {
        if ( re.exactMatch( line ) )
            newdata.append( re.cap( 1 ) );
    }
    return newdata;
}

QString FreeUnrarFlavour::name() const
{
    return QStringLiteral("unrar-free");
}

ProcessArgs FreeUnrarFlavour::processListArgs( const QString& ) const
{
    return ProcessArgs();
}

ProcessArgs FreeUnrarFlavour::processOpenArchiveArgs( const QString&, const QString& ) const
{
    return ProcessArgs();
}

UnarFlavour::UnarFlavour()
    : UnrarFlavour()
{
}

QStringList UnarFlavour::processListing( const QStringList &data )
{
    QStringList newdata = data;

    newdata.removeFirst();

    return newdata;
}

QString UnarFlavour::name() const
{
    return QStringLiteral("unar");
}

ProcessArgs UnarFlavour::processListArgs( const QString &fileName ) const
{
    return ProcessArgs ( QStringList() << fileName, true );
}

ProcessArgs UnarFlavour::processOpenArchiveArgs( const QString &fileName, const QString &path ) const
{
    return ProcessArgs ( QStringList() << fileName << QStringLiteral("-o") << path +  QLatin1Char('/'), false );
}

