/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_TAGGINGS_P_H
#define OKULAR_TAGGINGS_P_H

#include "area.h"
#include "document.h"
#include "tagging.h"

namespace Okular {

class PagePrivate;

class TaggingPrivate
{
    public:
        TaggingPrivate();

        virtual ~TaggingPrivate();

        /**
         * Transforms the tagging coordinates with the transformation
         * defined by @p matrix.
         */
        void taggingTransform( const QTransform &matrix );

        virtual void transform( const QTransform &matrix );
        virtual void baseTransform( const QTransform &matrix );
        virtual void resetTransformation();
        virtual void translate( const NormalizedPoint &coord );
        virtual void setTaggingProperties( const QDomNode& node );
        virtual TaggingPrivate* getNewTaggingPrivate() = 0;

        /**
         * Determines the distance of the closest point of the annotation to the
         * given point @p x @p y @p xScale @p yScale
         * @since 0.17
         */
        virtual double distanceSqr( double x, double y, double xScale, double yScale );
	
        PagePrivate * m_page;
        Node * m_node;

        QString m_author;
        QString m_contents;
        QString m_uniqueName;
        QDateTime m_modifyDate;
        QDateTime m_creationDate;

        int m_flags;
        NormalizedRect m_boundary;
        NormalizedRect m_transformedBoundary;

        Tagging::DisposeDataFunction m_disposeFunc;
        QVariant m_nativeId;
};

}

#endif
