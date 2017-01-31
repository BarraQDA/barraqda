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
#include "textpage.h"
// #include "ui/pageview.h"

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

QDomElement TaggingUtils::findChildElement( const QDomNode & parentNode,
    const QString & name )
{
    return parentNode->fistChildElement( name );
//     // loop through the whole children and return a 'name' named element
//     QDomNode subNode = parentNode.firstChild();
//     while( subNode.isElement() )
//     {
//         QDomElement element = subNode.toElement();
//         if ( element.tagName() == name )
//             return element;
//         subNode = subNode.nextSibling();
//     }
//     // if the name can't be found, return a dummy null element
//     return QDomElement();
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

class Tagging::Window::Private
{
public:
    Private()
    : m_flags( -1 ), m_width( 0 ), m_height( 0 )
    {
    }

    int m_flags;
    NormalizedPoint m_topLeft;
    int m_width;
    int m_height;
    QString m_title;
    QString m_summary;
};

Tagging::Window::Window()
: d( new Private )
{
}

Tagging::Window::~Window()
{
    delete d;
}

Tagging::Window::Window( const Window &other )
: d( new Private )
{
    *d = *other.d;
}

Tagging::Window& Tagging::Window::operator=( const Window &other )
{
    if ( this != &other )
        *d = *other.d;

    return *this;
}

void Tagging::Window::setFlags( int flags )
{
    d->m_flags = flags;
}

int Tagging::Window::flags() const
{
    return d->m_flags;
}

void Tagging::Window::setTopLeft( const NormalizedPoint &point )
{
    d->m_topLeft = point;
}

NormalizedPoint Tagging::Window::topLeft() const
{
    return d->m_topLeft;
}

void Tagging::Window::setWidth( int width )
{
    d->m_width = width;
}

int Tagging::Window::width() const
{
    return d->m_width;
}

void Tagging::Window::setHeight( int height )
{
    d->m_height = height;
}

int Tagging::Window::height() const
{
    return d->m_height;
}

void Tagging::Window::setTitle( const QString &title )
{
    d->m_title = title;
}

QString Tagging::Window::title() const
{
    return d->m_title;
}

void Tagging::Window::setSummary( const QString &summary )
{
    d->m_summary = summary;
}

QString Tagging::Window::summary() const
{
    return d->m_summary;
}


//BEGIN Tagging implementation
TaggingPrivate::TaggingPrivate()
    : m_page( 0 ), m_flags( 0 ), m_disposeFunc( 0 ),
      m_head( 0 ), m_next( 0 ), m_node( 0 ), m_linkNode( 0 ),
      m_pageNum( 0 ), m_doc( 0 )
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

void Tagging::setNode( QDANode *node )
{
    Q_D( Tagging );
    Tagging *head = this->head();
    if ( head == this )
        d->m_node = node;
    else
        head->setNode( node );
}

void Tagging::setPrevNode( QDANode *node )
{
    Q_D( Tagging );
    Tagging *head = this->head();
    if ( head == this )
        d->m_linkNode = node;
    else
        head->setPrevNode( node );
}

Node *Tagging::node() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_node;
    else
        return head->node();
}

void Tagging::setAuthor( const QString &author )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_author = author;
    else
        head->setAuthor( author );
}

QString Tagging::author() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_author;
    else
        return head->author();
}

void Tagging::setContents( const QString &contents )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_contents = contents;
    else
        head->setContents( author );
}

QString Tagging::contents() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_contents;
    else
        return head->contents();
}

void Tagging::setUniqueName( const QString &name )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_uniqueName = name;
    else
        head->setUniqueName( name );
}

QString Tagging::uniqueName() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_uniqueName;
    else
        return head->uniqueName();
}

void Tagging::setModificationDate( const QDateTime &date )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_modifyDate = date;
    else
        head->setModificationDate( date );
}

QDateTime Tagging::modificationDate() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_modifyDate;
    else
        return head->modificationDate();
}

void Tagging::setCreationDate( const QDateTime &date )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_creationDate = date;
    else
        head->setCreationDate( date );
}

QDateTime Tagging::creationDate() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_creationDate;
    else
        return head->creationDate();
}

void Tagging::setFlags( int flags )
{
    Q_D( Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        d->m_flags = flags;
    else
        head->setFlags( flags );
}

int Tagging::flags() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_flags;
    else
        return head->flags();
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

const Tagging *head() const
{
    Q_D( const Tagging );
    return d->m_head ? d->m_head : this;
}

Tagging *head()
{
    Q_D( const Tagging );
    return d->m_head ? d->m_head : this;
}

Tagging *next() const
{
    Q_D( const Tagging );
    return d->m_next;
}

void setNext( Tagging *next )
{
    Q_D( Tagging );
    d->m_next = next;
}

Tagging::Window & Tagging::window()
{
    Q_D( Tagging );
    return d->m_window;
}

const Tagging::Window & Tagging::window() const
{
    Q_D( const Tagging );
    return d->m_window;
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
        e.setAttribute( QStringLiteral("author"), d->m_author );
    if ( !d->m_contents.isEmpty() )
        e.setAttribute( QStringLiteral("contents"), d->m_contents );
    if ( d->m_modifyDate.isValid() )
        e.setAttribute( QStringLiteral("modifyDate"), d->m_modifyDate.toString(Qt::ISODate) );
    if ( d->m_creationDate.isValid() )
        e.setAttribute( QStringLiteral("creationDate"), d->m_creationDate.toString(Qt::ISODate) );
    // QSR node
    e.setAttribute( QStringLiteral("node"), this->node()->id() );

    // Sub-Node-1 - boundary
    if ( this->subType() != TTag )
    {
        QDomElement bE = document.createElement( "boundary" );
        e.appendChild( bE );
        bE.setAttribute( QStringLiteral("l"), QString::number( d->m_boundary.left ) );
        bE.setAttribute( QStringLiteral("t"), QString::number( d->m_boundary.top ) );
        bE.setAttribute( QStringLiteral("r"), QString::number( d->m_boundary.right ) );
        bE.setAttribute( QStringLiteral("b"), QString::number( d->m_boundary.bottom ) );
    }
}

QDomNode Tagging::getTaggingPropertiesDomNode() const
{
    QDomDocument doc( QStringLiteral("documentInfo") );
    QDomElement node = doc.createElement( QStringLiteral("tagging") );

    store(node, doc);
    return node;
}

void Tagging::setTaggingProperties( const QDomNode& node )
{
    if ( d_ptr->m_head )
        qCCritical(OkularCoreDebug) << "Tagging::setTaggingProperties called with non-head tagging: " << d_ptr->m_uniqueName;

    //  Save and restore private fields that will be erased.
    Tagging * next = this->next();

    //  Remove annotation from current QDA node
    if ( d_ptr->m_linkNode )
        d_ptr->m_linkNode->removeAnnotation( this );

    // Save off internal properties that aren't contained in node
    Okular::PagePrivate         *p             = d_ptr->m_page;
    QVariant                     nativeID      = d_ptr->m_nativeId;
    int                          internalFlags = d_ptr->m_flags;
    Tagging::DisposeDataFunction disposeFunc   = d_ptr->m_disposeFunc;

    // Replace TaggingPrivate object with a fresh copy
    TaggingPrivate *new_d_ptr = d_ptr->getNewTaggingPrivate();
    delete( d_ptr );
    d_ptr = new_d_ptr;

    // Restore internal properties
    d_ptr->m_page        = p;
    d_ptr->m_nativeId    = nativeID;
    d_ptr->m_flags       = d_ptr->m_flags | internalFlags;
    d_ptr->m_disposeFunc = disposeFunc;

    // Set the private taggings properties from node
    d_ptr->setTaggingProperties(node);

    // Transform tagging to current page rotation
    d_ptr->transform( d_ptr->m_page->rotationMatrix() );

    this->setNext( next );
    d->m_node->addAnnotation( this )
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
    // get the [base] element of the tagging node
    QDomElement e = TaggingUtils::findChildElement( node, "base" );
    if ( e.isNull() )
        return;

    // parse -contents- attributes
    if ( e.hasAttribute( QStringLiteral("author") ) )
        m_author = e.attribute( QStringLiteral("author") );
    if ( e.hasAttribute( QStringLiteral("contents") ) )
        m_contents = e.attribute( QStringLiteral("contents") );
    if ( e.hasAttribute( QStringLiteral("uniqueName") ) )
        m_uniqueName = e.attribute( QStringLiteral("uniqueName") );
    if ( e.hasAttribute( QStringLiteral("modifyDate") ) )
        m_modifyDate = QDateTime::fromString( e.attribute(QStringLiteral("modifyDate")), Qt::ISODate );
    if ( e.hasAttribute( QStringLiteral("creationDate") ) )
        m_creationDate = QDateTime::fromString( e.attribute(QStringLiteral("creationDate")), Qt::ISODate );
    // QDA node
    if ( e.hasAttribute( "node" ) )
        m_node = NodeUtils::retrieveNode( e.attribute( QStringLiteral("node") ).toInt() );
    else
        m_node = new Node ();

    //  m_doc can be set either when loading the annotation, or from the attached
    //  structure.
    if (! m_doc )
    {
        if ( m_page)
            m_doc = m_page->m_doc->m_parent;
        else
            return;
    }

    QDomNode eSubNode = e.firstChild();
    while ( eSubNode.isElement() )
    {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if ( ee.tagName() == QLatin1String("boundary") )
        {
            m_boundary=NormalizedRect(ee.attribute( QStringLiteral("l") ).toDouble(),
                                      ee.attribute( QStringLiteral("t") ).toDouble(),
                                      ee.attribute( QStringLiteral("r") ).toDouble(),
                                      ee.attribute( QStringLiteral("b") ).toDouble());
        }
    }
    m_transformedBoundary = m_boundary;
}

//END Tagging implementation

/** TextTagging [Tagging] */

TextTagging::TextTagging( const QDomNode & node )
: Tagging( *new TextTaggingPrivate(), node )
{
}

TextTagging::TextTagging( const RegularAreaRect * textArea )
: Tagging( *new TextTaggingPrivate() )
{
    Q_D( TextTagging );

    d->setTextArea( textArea );

    NormalizedRect rect = textArea->first();
    int end = textArea->count();
    for (int i = 1; i < end; i++ )
    {
        rect |= textArea->at( i );
    }
    d->m_boundary = rect;
}

TextTagging::~TextTagging()
{
}

Tagging::SubType TextTagging::subType() const
{
    return TText;
}

void TextTagging::storeSection( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const TextTagging );

    QDomElement e = document.createElement( "textref" );
    node.appendChild( e );

    e.setAttribute( QStringLiteral("o"), d->m_ref.offset + d->m_page->m_page->textOffset() );
    e.setAttribute( QStringLiteral("l"), d->m_ref.length );
}

void TextTagging::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const TextTagging );

    if ( d->m_head )
        qCWarning(OkularCoreDebug) << "TextTagging::store called with non-head tagging: " << d->m_uniqueName;

    // recurse to parent objects storing properties
    Tagging::store( node, document );

    const Tagging * tagIt = this;
    while ( tagIt )
    {
        tagIt->storeSection( node, document );
        tagIt = tagIt->next();
    }

//     NormalizedRect rect;
//     int end = d->m_textArea->count();
//     for (int i = 0; i < end; i++ )
//     {
//         rect = d->m_textArea->at( i );
//
//         QDomElement e = document.createElement( "rect" );
//         node.appendChild( e );
//
//         e.setAttribute( "l", QString::number( rect.left ) );
//         e.setAttribute( "t", QString::number( rect.top ) );
//         e.setAttribute( "r", QString::number( rect.right ) );
//         e.setAttribute( "b", QString::number( rect.bottom ) );
//     }
}

const RegularAreaRect * TextTagging::transformedTextArea () const
{
    Q_D( const TextTagging );

    return d->m_transformedTextArea;
}

class Okular::TextTaggingPrivate : public Okular::TaggingPrivate
{
    public:
        TextTaggingPrivate()
            : TaggingPrivate(),
              m_textArea( 0 ), m_transformedTextArea( 0 )
        {
        }

        ~TextTaggingPrivate()
        {
            delete m_textArea;
            delete m_transformedTextArea;
        }

        void transform( const QTransform &matrix ) override;
        void resetTransformation() override;
        void translate( const NormalizedPoint &coord ) override;
        TaggingPrivate* getNewTaggingPrivate() override;
        void setTaggingProperties( const QDomNode& node ) override;
        double distanceSqr( double x, double y, double xScale, double yScale );

        ~TextTaggingPrivate();
        void setTextArea( const RegularAreaRect * textArea );

        RegularAreaRect * m_textArea;
        RegularAreaRect * m_transformedTextArea;
};

void TextTaggingPrivate::setTextArea( const RegularAreaRect * textArea )
{
    m_textArea = new RegularAreaRect;
    *m_textArea = *textArea;
}

static void buildTextReferenceArea( TextTaggingPrivate *tTagP, const Page *page )
{
    //  Recreate the text reference area and boundaries.
    tTagP->m_textArea = page->TextReferenceArea( tTagP->m_ref );
    tTagP->m_transformedTextArea = new RegularAreaRect;
    tTagP->m_boundary = Okular::NormalizedRect();
    int end = tTagP->m_textArea->count();
    if ( end == 0 )
        qCWarning(OkularCoreDebug) << __func__ << " text reference area is null: " << tTagP->m_uniqueName;

    for (int i = 0; i < end; i++ )
    {
        NormalizedRect rect = tTagP->m_textArea->at(i);
        tTagP->m_transformedTextArea->append (rect);
        tTagP->m_boundary |= rect;
    }
    tTagP->m_transformedBoundary = tTagP->m_boundary;
}

void TextTaggingPrivate::setTaggingProperties( const QDomNode& node )
{
    Q_Q ( TextTagging );

    Tagging *head = 0;

    Okular::TaggingPrivate::setTaggingProperties(node);

    m_textArea = new Okular::RegularAreaRect();
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        // get tagging element and advance to next annot
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();

        if ( e.tagName() == "rect" )
        {
            NormalizedRect rect = NormalizedRect (e.attribute( "l" ).toDouble(),
                                                  e.attribute( "t" ).toDouble(),
                                                  e.attribute( "r" ).toDouble(),
                                                  e.attribute( "b" ).toDouble());

            m_textArea->append( rect );
        }
        else if ( e.tagName() == "textref" )
        {
            uint remainingLength = e.attribute( "l" ).toInt();
            uint pageOffset = e.attribute( "o" ).toInt();

            //  Find page where annotation starts
            uint pageNum = 0;
            const Page *page = m_doc->page( pageNum );
            const Page *nextPage = m_doc->page( pageNum + 1 );
            while ( nextPage && nextPage->textOffset() <= pageOffset )
            {
                pageNum++;
                page = nextPage;
                nextPage = m_doc->page( pageNum + 1 );
            }

            pageOffset -= page->textOffset();
            uint pageLength = nextPage ? std::min( remainingLength, nextPage->textOffset() - page->textOffset() - pageOffset ) : remainingLength;
            remainingLength -= pageLength;

            if (! head )     //  ie we are building the head annotation object
            {
                m_pageNum = pageNum;
                m_ref = { pageOffset, pageLength };
                buildTextReferenceArea( this, page );
                head = this;
            }
            else
            {
                TextTagging *tag = new TextTagging( head, page, { pageOffset, pageLength } );
                static_cast<TextTaggingPrivate *>(tag->d_ptr)->m_pageNum = pageNum;
            }

            while ( nextPage && remainingLength )
            {
                pageNum++;
                page = nextPage;
                nextPage = m_doc->page( pageNum + 1 );

                uint pageLength = nextPage ? std::min( remainingLength, nextPage->textOffset() - page->textOffset() ) : remainingLength;
                remainingLength -= pageLength;

                TextTagging *tag = new TextTagging( head, page, { 0, pageLength } );
                static_cast<TextTaggingPrivate *>(tag->d_ptr)->m_pageNum = pageNum;

                buildTextReferenceArea( static_cast<TextTaggingPrivate *>(tag->d_ptr), page );
            }
        }
}

double TextTaggingPrivate::distanceSqr( double x, double y, double xScale, double yScale )
{
    NormalizedRect rect = m_transformedTextArea->first();
    double leastdistance = rect.distanceSqr( x, y, xScale, yScale );
    int end = m_transformedTextArea->count();
    for (int i = 1; leastdistance > 0 && i < end; i++ )
    {
        rect = m_transformedTextArea->at( i );
        leastdistance = std::min( leastdistance, rect.distanceSqr( x, y, xScale, yScale ) );
    }

    return leastdistance;
}

void TextTaggingPrivate::resetTransformation()
{
    TaggingPrivate::resetTransformation();

    delete m_transformedTextArea;
    m_transformedTextArea = new RegularAreaRect;
    int end = m_textArea->count();
    for (int i = 0; i < end; i++ )
        m_transformedTextArea->append (m_textArea->at(i));
}

void TextTaggingPrivate::transform( const QTransform &matrix )
{
    TaggingPrivate::transform (matrix);
    m_transformedTextArea->transform( matrix );
}

void TextTaggingPrivate::translate( const NormalizedPoint &coord )
{
    TaggingPrivate::translate( coord );

#define ADD_COORD( c1, c2 ) \
{ \
  c1.x = c1.x + c2.x; \
  c1.y = c1.y + c2.y; \
}
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
        void translate( const NormalizedPoint &coord );
        TaggingPrivate* getNewTaggingPrivate();

        void setcoords ( const NormalizedRect *rect );

};

BoxTagging::BoxTagging()
    : Tagging( *new BoxTaggingPrivate() )
{
}

BoxTagging::BoxTagging( const NormalizedRect *rect )
    : Tagging( *new BoxTaggingPrivate() )
{
    Q_D( BoxTagging );
    d->m_boundary = *rect;
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

void BoxTagging::storeSection( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const BoxTagging );

    QDomElement e = document.createElement( "imageref" );
    node.appendChild( e );

    e.setAttribute( QStringLiteral("l"), QString::number(d->m_boundary.left ) );
    e.setAttribute( QStringLiteral("r"), QString::number(d->m_boundary.right ) );
    e.setAttribute( QStringLiteral("t"), QString::number(d->m_boundary.top + d->m_page->m_page->verticalOffset() ) );
    e.setAttribute( QStringLiteral("b"), QString::number(d->m_boundary.bottom + d->m_page->m_page->verticalOffset() ) );
}

void BoxTagging::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const BoxTagging );

    if ( d->m_head )
        qCWarning(OkularCoreDebug) << "BoxTagging::store called with non-head tagging: " << d->m_uniqueName;

    // recurse to parent objects storing properties
    Tagging::store( node, document );

    //  Dig up the base node to add the QDA Node reference
    QDomElement baseElement = TaggingUtils::findChildElement( node, QStringLiteral("base") );
    baseElement.setAttribute( QStringLiteral("node"), d->m_node->uniqueName() );

    const Tagging * tagIt = this;
    while ( tagIt )
    {
        tagIt->storeSection( node, document );
        tagIt = tagIt->next();
    }
}

void BoxTagging::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const BoxTagging );
    // recurse to parent objects storing properties
    Tagging::store( node, document );
}

QPixmap BoxTagging::pixmap() const
{
    //     Q_D( const BoxTagAnnotation );
    //
    //     const QVector< PageViewItem * > items = pageView->items();
    //     QVector< PageViewItem * >::const_iterator iIt = items.constBegin(), iEnd = items.constEnd();
    //     for ( ; iIt != iEnd; ++iIt )
    //     {
    //         PageViewItem * item = *iIt;
    //         const Okular::Page *okularPage = item->page();
    //         if ( okularPage->number() != pageItem->page()->number()
    //         ||  !item->isVisible() )
    //             continue;
    //
    //         QRect tagRect   = this->transformedBoundingRectangle().geometry( item->uncroppedWidth(), item->uncroppedHeight() ).translated( item->uncroppedGeometry().topLeft() );
    //         QRect itemRect  = item->croppedGeometry();
    //         QRect intersect = tagRect.intersect (itemRect);
    //         if ( !intersect.isNull() )
    //         {
    //             // renders page into a pixmap
    //             QPixmap copyPix( tagRect.width(), tagRect.height() );
    //             QPainter copyPainter( &copyPix );
    //             copyPainter.translate( -tagRect.left(), -tagRect.top() );
    //             pageView->drawDocumentOnPainter( tagRect, &copyPainter );
    //             copyPainter.end();
    //             QClipboard *cb = QApplication::clipboard();
    //             cb->setPixmap( copyPix, QClipboard::Clipboard );
    //             if ( cb->supportsSelection() )
    //                 cb->setPixmap( copyPix, QClipboard::Selection );
    //
    //             break;
    //         }
    //     }
    return QPixmap();
}

void BoxTaggingPrivate::translate( const NormalizedPoint &coord )
{
    TaggingPrivate::translate( coord );

#define ADD_COORD( c1, c2 ) \
{ \
  c1.x = c1.x + c2.x; \
  c1.y = c1.y + c2.y; \
}
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

Node::Node()
{
    this->m_id = lastNode++;

    if ( !NodeUtils::Nodes )
        NodeUtils::Nodes = new QList< Node * >();

    NodeUtils::Nodes-> append(this);
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
