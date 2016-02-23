/***************************************************************************
 *   Copyright (C) 2016 by Jonathan Schultz <jonathan@imatix.com>          *
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
#include "page.h"
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
    // save annotation's type as element's attribute
    tagElement.setAttribute( "type", (uint)tag->subType() );

    // append all annotation data as children of this node
    tag->store( tagElement, document );
}

QDomElement TaggingUtils::findChildElement( const QDomNode & parentNode,
    const QString & name )
{
    // loop through the whole children and return a 'name' named element
    QDomNode subNode = parentNode.firstChild();
    while( subNode.isElement() )
    {
        QDomElement element = subNode.toElement();
        if ( element.tagName() == name )
            return element;
        subNode = subNode.nextSibling();
    }
    // if the name can't be found, return a dummy null element
    return QDomElement();
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

Tagging::Tagging( )
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

void Tagging::setNode ( Node *node )
{
    Q_D( Tagging );
    d->m_node = node;
}

Node *Tagging::node() const
{
    Q_D( const Tagging );
    return d->m_node;
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

void Tagging::setUniqueName( const QString &name )
{
    Q_D( Tagging );
    d->m_uniqueName = name;
}

QString Tagging::uniqueName() const
{
    Q_D( const Tagging );
    return d->m_uniqueName;
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
    
    e.setAttribute( "node", this->node()->id() );

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
//    Okular::PagePrivate *p = d_ptr->m_page;
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
//    d_ptr->m_page = p;
    d_ptr->m_nativeId = nativeID;
    d_ptr->m_flags = d_ptr->m_flags | internalFlags;
    d_ptr->m_disposeFunc = disposeFunc;

    // Transform annotation to current page rotation
   d_ptr->transform( d_ptr->m_page->rotationMatrix() );
}

double TaggingPrivate::distanceSqr( double x, double y, double xScale, double yScale )
{
    return m_transformedBoundary.distanceSqr( x, y, xScale, yScale );
}

void TaggingPrivate::taggingTransform( const QTransform &matrix )
{
    resetTransformation();
    transform( matrix );
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

void TaggingPrivate::translate( const NormalizedPoint &coord )
{
    m_boundary.left = m_boundary.left + coord.x;
    m_boundary.right = m_boundary.right + coord.x;
    m_boundary.top = m_boundary.top + coord.y;
    m_boundary.bottom = m_boundary.bottom + coord.y;
}

void TaggingPrivate::setTaggingProperties( const QDomNode& node )
{
    // get the [base] element of the annotation node
    QDomElement e = TaggingUtils::findChildElement( node, "base" );
    if ( e.isNull() )
        return;

    // parse -contents- attributes
    if ( e.hasAttribute( "node" ) )
        m_node = NodeUtils::retrieveNode( e.attribute( "node" ).toInt() );
    else
        m_node = NodeUtils::newNode ();

    // parse -the-subnodes- (describing Style, Window, Revision(s) structures)
    // Note: all subnodes if present must be 'attributes complete'
    QDomNode eSubNode = e.firstChild();
    while ( eSubNode.isElement() )
    {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if ( ee.tagName() == "boundary" )
        {
            m_boundary=NormalizedRect(ee.attribute( "l" ).toDouble(),
                ee.attribute( "t" ).toDouble(),
                ee.attribute( "r" ).toDouble(),
                ee.attribute( "b" ).toDouble());
        }
    }
    m_transformedBoundary = m_boundary;    
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
        
        void setcoords ( NormalizedRect *rect );
	
        NormalizedPoint m_inplaceCallout[3];
	
        NormalizedRect* m_rect;
};

BoxTagging::BoxTagging()
    : Tagging( *new BoxTaggingPrivate() )
{
}

BoxTagging::BoxTagging( NormalizedRect *rect )
    : Tagging( *new BoxTaggingPrivate() )
{
    setcoords(rect);
}

BoxTagging::BoxTagging( const QDomNode &description )
    : Tagging( *new BoxTaggingPrivate(), description )
{
}

BoxTagging::~BoxTagging()
{
}

Tagging::SubType BoxTagging::subType() const
{
    return TBox;
}

void BoxTagging::setcoords( NormalizedRect *rect )
{
    Q_D( BoxTagging );
    d->setcoords( rect );
}

void BoxTagging::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const BoxTagging );
    // recurse to parent objects storing properties
    Tagging::store( node, document );
}

void BoxTaggingPrivate::setcoords( NormalizedRect *rect )
{
    kDebug() << "Setting tagging coordinates";
    m_rect = rect;
    m_boundary = *rect;
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

//BEGIN Node implementation

// From http://godsnotwheregodsnot.blogspot.ru/2013/11/kmeans-color-quantization-seeding.html
static QRgb tagColors [] = {

        /* 0xFF000000, */ 0xFFFFFF00, 0xFF1CE6FF, 0xFFFF34FF, 0xFFFF4A46, 0xFF008941, 0xFF006FA6, 0xFFA30059,

        0xFFFFDBE5, 0xFF7A4900, 0xFF0000A6, 0xFF63FFAC, 0xFFB79762, 0xFF004D43, 0xFF8FB0FF, 0xFF997D87,
        0xFF5A0007, 0xFF809693, 0xFFFEFFE6, 0xFF1B4400, 0xFF4FC601, 0xFF3B5DFF, 0xFF4A3B53, 0xFFFF2F80,
        0xFF61615A, 0xFFBA0900, 0xFF6B7900, 0xFF00C2A0, 0xFFFFAA92, 0xFFFF90C9, 0xFFB903AA, 0xFFD16100,
        0xFFDDEFFF, 0xFF000035, 0xFF7B4F4B, 0xFFA1C299, 0xFF300018, 0xFF0AA6D8, 0xFF013349, 0xFF00846F,
        0xFF372101, 0xFFFFB500, 0xFFC2FFED, 0xFFA079BF, 0xFFCC0744, 0xFFC0B9B2, 0xFFC2FF99, 0xFF001E09,
        0xFF00489C, 0xFF6F0062, 0xFF0CBD66, 0xFFEEC3FF, 0xFF456D75, 0xFFB77B68, 0xFF7A87A1, 0xFF788D66,
        0xFF885578, 0xFFFAD09F, 0xFFFF8A9A, 0xFFD157A0, 0xFFBEC459, 0xFF456648, 0xFF0086ED, 0xFF886F4C,
        
        0xFF34362D, 0xFFB4A8BD, 0xFF00A6AA, 0xFF452C2C, 0xFF636375, 0xFFA3C8C9, 0xFFFF913F, 0xFF938A81,
        0xFF575329, 0xFF00FECF, 0xFFB05B6F, 0xFF8CD0FF, 0xFF3B9700, 0xFF04F757, 0xFFC8A1A1, 0xFF1E6E00,
        0xFF7900D7, 0xFFA77500, 0xFF6367A9, 0xFFA05837, 0xFF6B002C, 0xFF772600, 0xFFD790FF, 0xFF9B9700,
        0xFF549E79, 0xFFFFF69F, 0xFF201625, 0xFF72418F, 0xFFBC23FF, 0xFF99ADC0, 0xFF3A2465, 0xFF922329,
        0xFF5B4534, 0xFFFDE8DC, 0xFF404E55, 0xFF0089A3, 0xFFCB7E98, 0xFFA4E804, 0xFF324E72, 0xFF6A3A4C,
        0xFF83AB58, 0xFF001C1E, 0xFFD1F7CE, 0xFF004B28, 0xFFC8D0F6, 0xFFA3A489, 0xFF806C66, 0xFF222800,
        0xFFBF5650, 0xFFE83000, 0xFF66796D, 0xFFDA007C, 0xFFFF1A59, 0xFF8ADBB4, 0xFF1E0200, 0xFF5B4E51,
        0xFFC895C5, 0xFF320033, 0xFFFF6832, 0xFF66E1D3, 0xFFCFCDAC, 0xFFD0AC94, 0xFF7ED379, 0xFF012C58,
        
        0xFF7A7BFF, 0xFFD68E01, 0xFF353339, 0xFF78AFA1, 0xFFFEB2C6, 0xFF75797C, 0xFF837393, 0xFF943A4D,
        0xFFB5F4FF, 0xFFD2DCD5, 0xFF9556BD, 0xFF6A714A, 0xFF001325, 0xFF02525F, 0xFF0AA3F7, 0xFFE98176,
        0xFFDBD5DD, 0xFF5EBCD1, 0xFF3D4F44, 0xFF7E6405, 0xFF02684E, 0xFF962B75, 0xFF8D8546, 0xFF9695C5,
        0xFFE773CE, 0xFFD86A78, 0xFF3E89BE, 0xFFCA834E, 0xFF518A87, 0xFF5B113C, 0xFF55813B, 0xFFE704C4,
        0xFF00005F, 0xFFA97399, 0xFF4B8160, 0xFF59738A, 0xFFFF5DA7, 0xFFF7C9BF, 0xFF643127, 0xFF513A01,
        0xFF6B94AA, 0xFF51A058, 0xFFA45B02, 0xFF1D1702, 0xFFE20027, 0xFFE7AB63, 0xFF4C6001, 0xFF9C6966,
        0xFF64547B, 0xFF97979E, 0xFF006A66, 0xFF391406, 0xFFF4D749, 0xFF0045D2, 0xFF006C31, 0xFFDDB6D0,
        0xFF7C6571, 0xFF9FB2A4, 0xFF00D891, 0xFF15A08A, 0xFFBC65E9, 0xFFFFFFFE, 0xFFC6DC99, 0xFF203B3C,

        0xFF671190, 0xFF6B3A64, 0xFFF5E1FF, 0xFFFFA0F2, 0xFFCCAA35, 0xFF374527, 0xFF8BB400, 0xFF797868,
        0xFFC6005A, 0xFF3B000A, 0xFFC86240, 0xFF29607C, 0xFF402334, 0xFF7D5A44, 0xFFCCB87C, 0xFFB88183,
        0xFFAA5199, 0xFFB5D6C3, 0xFFA38469, 0xFF9F94F0, 0xFFA74571, 0xFFB894A6, 0xFF71BB8C, 0xFF00B433,
        0xFF789EC9, 0xFF6D80BA, 0xFF953F00, 0xFF5EFF03, 0xFFE4FFFC, 0xFF1BE177, 0xFFBCB1E5, 0xFF76912F,
        0xFF003109, 0xFF0060CD, 0xFFD20096, 0xFF895563, 0xFF29201D, 0xFF5B3213, 0xFFA76F42, 0xFF89412E,
        0xFF1A3A2A, 0xFF494B5A, 0xFFA88C85, 0xFFF4ABAA, 0xFFA3F3AB, 0xFF00C6C8, 0xFFEA8B66, 0xFF958A9F,
        0xFFBDC9D2, 0xFF9FA064, 0xFFBE4700, 0xFF658188, 0xFF83A485, 0xFF453C23, 0xFF47675D, 0xFF3A3F00,
        0xFF061203, 0xFFDFFB71, 0xFF868E7E, 0xFF98D058, 0xFF6C8F7D, 0xFFD7BFC2, 0xFF3C3E6E, 0xFFD83D66,
        
        0xFF2F5D9B, 0xFF6C5E46, 0xFFD25B88, 0xFF5B656C, 0xFF00B57F, 0xFF545C46, 0xFF866097, 0xFF365D25,
        0xFF252F99, 0xFF00CCFF, 0xFF674E60, 0xFFFC009C, 0xFF92896B
};

static unsigned int lastNode = 0;

QList< Node * > * NodeUtils::Nodes = 0;

Node * NodeUtils::retrieveNode ( int id )
{
    if ( !NodeUtils::Nodes )
        NodeUtils::Nodes = new QList< Node * >();
    
    QList< Node * >::const_iterator nIt = NodeUtils::Nodes->constBegin(), nEnd = NodeUtils::Nodes->constEnd();
    for ( ; nIt != nEnd; ++nIt )
    {
        if ( (*nIt)->m_id == id )
            return *nIt;
    }

    Node * node = new Node();
    node->m_id = id;
    if ( id <= lastNode )
        lastNode = id + 1;
    
    return node;
}

Node * NodeUtils::newNode()
{
    Node * node = new Node();
    
    if ( !NodeUtils::Nodes )
        NodeUtils::Nodes = new QList< Node * >();

    node->m_id = lastNode++;
    
    NodeUtils::Nodes-> append(node);
    
    return node;
}

Node::Node()
{
}

Node::~Node()
{
}

int Node::id() const
{
    return this->m_id;
}

unsigned int Node::color() const
{
    return tagColors[ this->m_id ];
}

//END Node implementation
