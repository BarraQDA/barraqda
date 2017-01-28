/***************************************************************************
 *   Copyright (C) 2017 by Jonathan Schultz <jonathan@schultz.la>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TAGGINGPOPUP_H
#define TAGGINGPOPUP_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QPoint>

namespace Okular {
class Tagging;
class Document;
}

class TaggingPopup : public QObject
{
    Q_OBJECT

    public:
        /**
         * Describes the structure of the popup menu.
         */
        enum MenuMode
        {
            SingleTaggingMode, ///< The menu shows only entries to manipulate a single tagging, or multiple taggings as a group.
            MultiTaggingMode   ///< The menu shows entries to manipulate multiple taggings.
        };

        TaggingPopup( Okular::Document *document, MenuMode mode, QWidget *parent = Q_NULLPTR );

        void addTagging( Okular::Tagging* tagging, int pageNumber );

        void exec( const QPoint &point = QPoint() );

    Q_SIGNALS:
        void openTaggingWindow( Okular::Tagging *tagging, int pageNumber );

    public:
        struct TagPagePair {
            TagPagePair() : tagging( 0 ),  pageNumber( -1 )
            { }

            TagPagePair( Okular::Tagging *a, int pn ) : tagging( a ),  pageNumber( pn )
            { }

            TagPagePair( const TagPagePair & pair ) : tagging( pair.tagging ),  pageNumber( pair.pageNumber )
            { }

            bool operator==( const TagPagePair & pair ) const
            { return tagging == pair.tagging && pageNumber == pair.pageNumber; }

            Okular::Tagging* tagging;
            int pageNumber;
        };

    private:
        QWidget *mParent;

        QList< TagPagePair > mTaggings;
        Okular::Document *mDocument;
        MenuMode mMenuMode;
};


#endif
