/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DVI_GENERATOR_H_
#define _DVI_GENERATOR_H_

#include <core/generator.h>

#include <qbitarray.h>

class dviRenderer;
class dviPageInfo;
class Anchor;

namespace Okular {
class DocumentViewport;
class ObjectRect;
}

class DviGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES( Okular::Generator )
    public:
        DviGenerator( QObject *parent, const QVariantList &args );
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector ) override;

        // document information
        Okular::DocumentInfo generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const override;

        // table of contents
        const Okular::DocumentSynopsis *generateDocumentSynopsis() override;

        // list of fonts
        Okular::FontInfo::List fontsForPage( int page ) override;

        bool print( QPrinter &printer ) override;

        QVariant metaData( const QString & key, const QVariant & option ) const override;

    protected:
        bool doCloseDocument() override;
        QImage image( Okular::PixmapRequest * request ) override;
        Okular::TextPage* textPage( Okular::TextRequest *request ) override;

    private:
        double m_resolution;
        bool m_fontExtracted;

        Okular::DocumentSynopsis *m_docSynopsis;

        dviRenderer *m_dviRenderer;
        QBitArray m_linkGenerated;

        void loadPages( QVector< Okular::Page * > & pagesVector );
        Okular::TextPage *extractTextFromPage( dviPageInfo *pageInfo );
        void fillViewportFromAnchor( Okular::DocumentViewport &vp, const Anchor &anch, 
                                     int pW, int pH ) const; 
        void fillViewportFromAnchor( Okular::DocumentViewport &vp, const Anchor &anch,
                                     const Okular::Page *page ) const;
        QLinkedList<Okular::ObjectRect*> generateDviLinks( const dviPageInfo *pageInfo );
};

#endif
