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

class Page;

class TaggingPrivate
{
    public:
        TaggingPrivate( Tagging *q );

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

        Page * m_page;

        QString m_author;
        QString m_contents;
        QString m_uniqueName;
        QDateTime m_modifyDate;
        QDateTime m_creationDate;

        Tagging *m_head, *m_next;               //  Structures to link multi-page taggings
        QDANode *m_node;                        //  This pointer can be updated
        QDANode *m_linkNode;                    //  This pointer must point to a QDA Node
        uint m_pageNum;                         //  Unlike m_page, m_pageNum is always assigned.
        Document *m_doc;

        int m_flags;
        NormalizedRect m_boundary;
        NormalizedRect m_transformedBoundary;

        Okular::Tagging::Window m_window;

        Tagging::DisposeDataFunction m_disposeFunc;
        QVariant m_nativeId;

    protected:
        Q_DECLARE_PUBLIC( Tagging )
        Tagging * q_ptr;
};

}

#endif
