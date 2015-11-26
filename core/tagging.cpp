/***************************************************************************
 *   Copyright (C) 2015 by Jonathan Schultz <jonathan@imatix.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tagging.h"
#include "tagging_p.h"

#include <kdebug.h>

// local includes
#include "document.h"
#include "document_p.h"
#include "page_p.h"

using namespace Okular;

//BEGIN TaggingUtils implementation
Tagging * TaggingUtils::createTagging( const QDomElement & tagElement )
{
    // safety check on tagging element
    if ( !tagElement.hasAttribute( "type" ) )
        return 0;

    // build tagging of given type
    Tagging * tagging = 0;
    int typeNumber = tagElement.attribute( "type" ).toInt();
    switch ( typeNumber )
    {
        case Tagging::TText:
            tagging = new TextTagging( tagElement );
            break;
        case Tagging::TBox:
            tagging = new BoxTagging( tagElement );
            break;
    }

    // return created tagging
    return tagging;
}

void TaggingUtils::storeTagging( const Tagging * tag, QDomElement & tagElement,
    QDomDocument & document )
{
    // save tagging's type as element's attribute
    tagElement.setAttribute( "type", (uint)tag->subType() );

    // append all tagging data as children of this node
    tag->store( tagElement, document );
}


QRect TaggingUtils::taggingGeometry( const Tagging * tag,
    double scaledWidth, double scaledHeight )
{
    const QRect rect = tag->transformedBoundingRectangle().geometry( (int)scaledWidth, (int)scaledHeight );
    if ( tag->subType() == Tagging::TText )
    {
        // To be honest i have no clue of why the 24,24 is here, maybe to make sure it's not too small?
        // But why only for linked text?
        const QRect rect24 = QRect( (int)( tag->transformedBoundingRectangle().left * scaledWidth ),
                                    (int)( tag->transformedBoundingRectangle().top * scaledHeight ), 24, 24 );
        return rect24.united(rect);
    }

    return rect;
}
//END TaggingUtils implementation

//BEGIN Tagging implementation
TaggingPrivate::TaggingPrivate()
{
}

TaggingPrivate::~TaggingPrivate()
{
}

Tagging::Tagging( TaggingPrivate &dd )
    : d_ptr( &dd )
{
}

Tagging::Tagging( TaggingPrivate &dd, const QDomNode & tagNode )
    : d_ptr( &dd )
{
    d_ptr->setTaggingProperties( tagNode );
}

Tagging::~Tagging()
{
    if ( d_ptr->m_disposeFunc )
        d_ptr->m_disposeFunc( this );

    delete d_ptr;
}

void Tagging::setAuthor( const QString &author )
{
    Q_D( Tagging );
    d->m_author = author;
}

QString Tagging::author() const
{
    Q_D( const Tagging );
    return d->m_author;
}

void Tagging::setModificationDate( const QDateTime &date )
{
    Q_D( Tagging );
    d->m_modifyDate = date;
}

QDateTime Tagging::modificationDate() const
{
    Q_D( const Tagging );
    return d->m_modifyDate;
}

void Tagging::setCreationDate( const QDateTime &date )
{
    Q_D( Tagging );
    d->m_creationDate = date;
}

QDateTime Tagging::creationDate() const
{
    Q_D( const Tagging );
    return d->m_creationDate;
}

void Tagging::setFlags( int flags )
{
    Q_D( Tagging );
    d->m_flags = flags;
}

int Tagging::flags() const
{
    Q_D( const Tagging );
    return d->m_flags;
}

void Tagging::setBoundingRectangle( const NormalizedRect &rectangle )
{
    Q_D( Tagging );
    d->m_boundary = rectangle;
    d->resetTransformation();
    if ( d->m_page )
    {
        d->transform( d->m_page->rotationMatrix() );
    }
}

NormalizedRect Tagging::boundingRectangle() const
{
    Q_D( const Tagging );
    return d->m_boundary;
}

NormalizedRect Tagging::transformedBoundingRectangle() const
{
    Q_D( const Tagging );
    return d->m_transformedBoundary;
}

void Tagging::translate( const NormalizedPoint &coord )
{
    Q_D( Tagging );
    d->translate( coord );
    d->resetTransformation();
    if ( d->m_page )
    {
        d->transform( d->m_page->rotationMatrix() );
    }
}

void Tagging::setDisposeDataFunction( DisposeDataFunction func )
{
    Q_D( Tagging );
    d->m_disposeFunc = func;
}

void Tagging::store( QDomNode & tagNode, QDomDocument & document ) const
{
    Q_D( const Tagging );
    // create [base] element of the tagging node
    QDomElement e = document.createElement( "base" );
    tagNode.appendChild( e );

    // store -contents- attributes
    if ( !d->m_author.isEmpty() )
        e.setAttribute( "author", d->m_author );
    if ( d->m_modifyDate.isValid() )
        e.setAttribute( "modifyDate", d->m_modifyDate.toString(Qt::ISODate) );
    if ( d->m_creationDate.isValid() )
        e.setAttribute( "creationDate", d->m_creationDate.toString(Qt::ISODate) );

    // store -other- attributes
    if ( d->m_flags ) // Strip internal flags
        e.setAttribute( "flags", d->m_flags );

    // Sub-Node-1 - boundary
    QDomElement bE = document.createElement( "boundary" );
    e.appendChild( bE );
    bE.setAttribute( "l", QString::number( d->m_boundary.left ) );
    bE.setAttribute( "t", QString::number( d->m_boundary.top ) );
    bE.setAttribute( "r", QString::number( d->m_boundary.right ) );
    bE.setAttribute( "b", QString::number( d->m_boundary.bottom ) );
}

void Tagging::setTaggingProperties( const QDomNode& node )
{
    // Save off internal properties that aren't contained in node
    Okular::PagePrivate *p = d_ptr->m_page;
    QVariant nativeID = d_ptr->m_nativeId;
    int internalFlags = d_ptr->m_flags;
    Tagging::DisposeDataFunction disposeFunc = d_ptr->m_disposeFunc;

    // Replace TaggingPrivate object with a fresh copy
    TaggingPrivate *new_d_ptr = d_ptr->getNewTaggingPrivate();
    delete( d_ptr );
    d_ptr = new_d_ptr;

    // Set the annotations properties from node
    d_ptr->setTaggingProperties(node);

    // Restore internal properties
    d_ptr->m_page = p;
    d_ptr->m_nativeId = nativeID;
    d_ptr->m_flags = d_ptr->m_flags | internalFlags;
    d_ptr->m_disposeFunc = disposeFunc;

    // Transform annotation to current page rotation
    d_ptr->transform( d_ptr->m_page->rotationMatrix() );
}

void TaggingPrivate::translate( const NormalizedPoint &coord )
{
    m_boundary.left = m_boundary.left + coord.x;
    m_boundary.right = m_boundary.right + coord.x;
    m_boundary.top = m_boundary.top + coord.y;
    m_boundary.bottom = m_boundary.bottom + coord.y;
}

void TaggingPrivate::transform( const QTransform &matrix )
{
    m_transformedBoundary.transform( matrix );
}

void TaggingPrivate::baseTransform( const QTransform &matrix )
{
    m_boundary.transform( matrix );
}

void TaggingPrivate::resetTransformation()
{
    m_transformedBoundary = m_boundary;
}

void TaggingPrivate::setTaggingProperties( const QDomNode& node )
{
}

//END Tagging implementation

/** TextTagging [Tagging] */

class Okular::TextTaggingPrivate : public Okular::TaggingPrivate
{
    public:
        virtual void translate( const NormalizedPoint &coord );
        virtual TaggingPrivate* getNewTaggingPrivate();
	
        NormalizedPoint m_inplaceCallout[3];
};

TextTagging::TextTagging()
    : Tagging( *new TextTaggingPrivate() )
{
}

TextTagging::TextTagging( const QDomNode & node )
    : Tagging( *new TextTaggingPrivate(), node )
{
}

TextTagging::~TextTagging()
{
}

Tagging::SubType TextTagging::subType() const
{
    return TText;
}

void TextTaggingPrivate::translate( const NormalizedPoint &coord )
{
    TaggingPrivate::translate( coord );

#define ADD_COORD( c1, c2 ) \
{ \
  c1.x = c1.x + c2.x; \
  c1.y = c1.y + c2.y; \
}
    ADD_COORD( m_inplaceCallout[0], coord )
    ADD_COORD( m_inplaceCallout[1], coord )
    ADD_COORD( m_inplaceCallout[2], coord )
#undef ADD_COORD
}

TaggingPrivate* TextTaggingPrivate::getNewTaggingPrivate()
{
    return new TextTaggingPrivate();
}

/** BoxTagging [Tagging] */

class Okular::BoxTaggingPrivate : public Okular::TaggingPrivate
{
    public:
        virtual void translate( const NormalizedPoint &coord );
        virtual TaggingPrivate* getNewTaggingPrivate();
	
        NormalizedPoint m_inplaceCallout[3];
};

BoxTagging::BoxTagging()
    : Tagging( *new BoxTaggingPrivate() )
{
}

BoxTagging::BoxTagging( const QDomNode & node )
    : Tagging( *new BoxTaggingPrivate(), node )
{
}

BoxTagging::~BoxTagging()
{
}

Tagging::SubType BoxTagging::subType() const
{
    return TBox;
}

void BoxTaggingPrivate::translate( const NormalizedPoint &coord )
{
    TaggingPrivate::translate( coord );

#define ADD_COORD( c1, c2 ) \
{ \
  c1.x = c1.x + c2.x; \
  c1.y = c1.y + c2.y; \
}
    ADD_COORD( m_inplaceCallout[0], coord )
    ADD_COORD( m_inplaceCallout[1], coord )
    ADD_COORD( m_inplaceCallout[2], coord )
#undef ADD_COORD
}

TaggingPrivate* BoxTaggingPrivate::getNewTaggingPrivate()
{
    return new BoxTaggingPrivate();
}




    