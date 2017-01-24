/***************************************************************************
 *   Copyright (C) 2007,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sourcereference.h"
#include "sourcereference_p.h"

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <KLocalizedString>

using namespace Okular;

class SourceReference::Private
{
    public:
        Private()
            : row( 0 ), column( 0 )
        {
        }

        QString filename;
        int row;
        int column;
};

SourceReference::SourceReference( const QString &fileName, int row, int column )
    : d( new Private )
{
    d->filename = fileName;
    d->row = row;
    d->column = column;
}

SourceReference::~SourceReference()
{
    delete d;
}

QString SourceReference::fileName() const
{
    return d->filename;
}

int SourceReference::row() const
{
    return d->row;
}

int SourceReference::column() const
{
    return d->column;
}

bool Okular::extractLilyPondSourceReference( const QUrl &url, QString *file, int *row, int *col )
{
    // Example URL is: textedit:///home/foo/bar.ly:42:42:42
    // The three numbers are apparently: line:beginning of column:end of column

    if ( url.scheme() != QStringLiteral("textedit") )
        return false;

    // There can be more, in case the filename contains :
    if (url.fileName().count(':') < 3) {
        return false;
    }

    QStringList parts(url.path().split(':'));

    bool ok;
    // Take out the things we need
    int columnEnd = parts.takeLast().toInt(&ok); // apparently we don't use this
    Q_UNUSED(columnEnd);
    if (!ok) {
        return false;
    }

    *col = parts.takeLast().toInt(&ok);
    if (!ok) {
        return false;
    }

    *row = parts.takeLast().toInt(&ok);
    if (!ok) {
        return false;
    }

    // In case the path itself contains :, we need to reconstruct it after removing all the numbers
    *file = parts.join(':');
    return (!file->isEmpty());
}

QString Okular::sourceReferenceToolTip( const QString &source, int row, int col )
{
    Q_UNUSED( row );
    Q_UNUSED( col );
    return i18nc( "'source' is a source file", "Source: %1", source );
}
