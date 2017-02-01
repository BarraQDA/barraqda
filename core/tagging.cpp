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

#include <QtCore/QDebug>

// local includes
#include "debug_p.h"
#include "document.h"
#include "document_p.h"
#include "page.h"
#include "page_p.h"
#include "textpage.h"
// #include "ui/pageview.h"

using namespace Okular;

//BEGIN TaggingUtils implementation
Tagging * TaggingUtils::createTagging( Document *doc, const QDomElement & tagElement )
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
            tagging = new TextTagging( doc, tagElement );
            break;
        case Tagging::TBox:
            tagging = new BoxTagging( doc, tagElement );
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
    return parentNode.firstChildElement( name );
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
TaggingPrivate::TaggingPrivate( Tagging *q )
    : m_page( 0 ), m_flags( 0 ), m_disposeFunc( 0 ),
      m_head( 0 ), m_next( 0 ), m_node( 0 ), m_linkNode( 0 ),
      m_pageNum( 0 ), m_doc( 0 ),
      q_ptr( q )
{
}

TaggingPrivate::~TaggingPrivate()
{
}

Tagging::Tagging( TaggingPrivate &dd )
    : d_ptr( &dd )
{
}

Tagging::Tagging( Tagging *head, TaggingPrivate &dd )
    : d_ptr( &dd )
{
    if ( head )
    {
        d_ptr->m_head = head;
        Tagging *tagIt = head;
        Tagging *nextTag = tagIt->next();
        while ( nextTag )
        {
            tagIt = nextTag;
            nextTag = tagIt->next();
        }
        tagIt->setNext( this );
    }
}

Tagging::Tagging( Document *doc, TaggingPrivate &dd, const QDomElement & tagElement )
    : d_ptr( &dd )
{
    d_ptr->m_doc = doc;
    d_ptr->setTaggingProperties( tagElement );
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

QDANode *Tagging::node() const
{
    Q_D( const Tagging );
    const Tagging *head = this->head();
    if ( head == this )
        return d->m_node;
    else
        return head->node();
}
const Document * Tagging::document() const
{
    Q_D( const Tagging );
    return d->m_page->m_doc->m_parent;
}

uint Tagging::pageNum() const
{
    Q_D( const Tagging );
    return d->m_pageNum;
}

void Tagging::setAuthor( const QString &author )
{
    Q_D( Tagging );
    Tagging *head = this->head();
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
    Tagging *head = this->head();
    if ( head == this )
        d->m_contents = contents;
    else
        head->setContents( contents );
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
    Tagging *head = this->head();
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
    Tagging *head = this->head();
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
    Tagging *head = this->head();
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
    Tagging *head = this->head();
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

const Tagging * Tagging::head() const
{
    Q_D( const Tagging );
    return d->m_head ? d->m_head : this;
}

Tagging * Tagging::head()
{
    Q_D( const Tagging );
    return d->m_head ? d->m_head : this;
}

Tagging * Tagging::next() const
{
    Q_D( const Tagging );
    return d->m_next;
}

void Tagging::setNext( Tagging *next )
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
    // QDA node
    e.setAttribute( QStringLiteral("node"), this->node()->uniqueName() );

    // Sub-Node-1 - boundary
    if ( this->subType() != TText )
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
        d_ptr->m_linkNode->removeTagging( this );

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
    d_ptr->m_node->addTagging( this );
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
        m_node = QDANodeUtils::retrieve( e.attribute( QStringLiteral("node") ) );
    if (! m_node )
        m_node = new QDANode ();

    //  m_doc can be set either when loading the annotation, or from the attached
    //  structure.
    if (! m_doc )
    {
        if ( m_page)
            m_doc = m_page->m_doc->m_parent;
        else
            //  This should not happen
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

class Okular::TextTaggingPrivate : public Okular::TaggingPrivate
{
public:
    TextTaggingPrivate( Tagging *q )
        : TaggingPrivate( q ),
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

    void setTextArea( const RegularAreaRect * textArea );

    TextReference m_ref;
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
    Q_Q ( Tagging );

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
                head = q;
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
    return new TextTaggingPrivate( this->q_ptr );
}

TextTagging::TextTagging( Document *doc, const QDomElement & tagElement )
    : Tagging( doc, *new TextTaggingPrivate( this ), tagElement )
{
}

TextTagging::TextTagging( Tagging * head, const Page * page, TextReference ref )
    : Tagging( head, *new TextTaggingPrivate( this ) )
{
    Q_D( TextTagging );

    d->m_pageNum = page->number();
    d->m_ref = ref;
    d->m_textArea = page->TextReferenceArea( ref );
    NormalizedRect rect = NormalizedRect();;
    int end = d->m_textArea->count();
    if ( end == 0 )
        qCWarning(OkularCoreDebug) << __func__ << " text reference area is null: " << d->m_uniqueName;

    for (int i = 0; i < end; i++ )
        rect |= d->m_textArea->at( i );

    d->m_boundary            = rect;
    d->m_transformedBoundary = rect;
}

TextTagging::TextTagging( const Page * page, TextReference ref )
    : TextTagging( 0, page, ref )
{
}

TextTagging::~TextTagging()
{
}

Tagging::SubType TextTagging::subType() const
{
    return TText;
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
        QDomElement e = document.createElement( "textref" );
        node.appendChild( e );

        e.setAttribute( QStringLiteral("o"), d->m_ref.offset + d->m_page->m_page->textOffset() );
        e.setAttribute( QStringLiteral("l"), d->m_ref.length );

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

const TextReference TextTagging::reference () const
{
    Q_D( const TextTagging );

    return d->m_ref;
}

/** BoxTagging [Tagging] */

class Okular::BoxTaggingPrivate : public Okular::TaggingPrivate
{
public:
    BoxTaggingPrivate( Tagging *q )
        : TaggingPrivate( q )
    {
    }

    void translate( const NormalizedPoint &coord );
    TaggingPrivate* getNewTaggingPrivate();

    void setcoords ( const NormalizedRect *rect );
};

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
    return new BoxTaggingPrivate( this->q_ptr );
}

BoxTagging::BoxTagging( Document *doc, const QDomElement & tagElement )
    : Tagging( doc, *new BoxTaggingPrivate( this ), tagElement )
{
}

BoxTagging::BoxTagging( Tagging * head, const Page * page, const NormalizedRect *rect )
    : Tagging( head, *new BoxTaggingPrivate( this ) )
{
    Q_D( BoxTagging );
    d->m_pageNum = page->number();
    d->m_boundary = *rect;
}

BoxTagging::BoxTagging( const Page * page, const NormalizedRect *rect )
    : BoxTagging( 0, page, rect )
{
}

BoxTagging::~BoxTagging()
{
}

Tagging::SubType BoxTagging::subType() const
{
    return TBox;
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
        QDomElement e = document.createElement( "imageref" );
        node.appendChild( e );

        e.setAttribute( QStringLiteral("l"), QString::number(d->m_boundary.left ) );
        e.setAttribute( QStringLiteral("r"), QString::number(d->m_boundary.right ) );
        e.setAttribute( QStringLiteral("t"), QString::number(d->m_boundary.top + d->m_page->m_page->verticalOffset() ) );
        e.setAttribute( QStringLiteral("b"), QString::number(d->m_boundary.bottom + d->m_page->m_page->verticalOffset() ) );

        tagIt = tagIt->next();
    }
}

// QPixmap BoxTagging::pixmap() const
// {
//         Q_D( const BoxTagAnnotation );
//
//         const QVector< PageViewItem * > items = pageView->items();
//         QVector< PageViewItem * >::const_iterator iIt = items.constBegin(), iEnd = items.constEnd();
//         for ( ; iIt != iEnd; ++iIt )
//         {
//             PageViewItem * item = *iIt;
//             const Okular::Page *okularPage = item->page();
//             if ( okularPage->number() != pageItem->page()->number()
//             ||  !item->isVisible() )
//                 continue;
//
//             QRect tagRect   = this->transformedBoundingRectangle().geometry( item->uncroppedWidth(), item->uncroppedHeight() ).translated( item->uncroppedGeometry().topLeft() );
//             QRect itemRect  = item->croppedGeometry();
//             QRect intersect = tagRect.intersect (itemRect);
//             if ( !intersect.isNull() )
//             {
//                 // renders page into a pixmap
//                 QPixmap copyPix( tagRect.width(), tagRect.height() );
//                 QPainter copyPainter( &copyPix );
//                 copyPainter.translate( -tagRect.left(), -tagRect.top() );
//                 pageView->drawDocumentOnPainter( tagRect, &copyPainter );
//                 copyPainter.end();
//                 QClipboard *cb = QApplication::clipboard();
//                 cb->setPixmap( copyPix, QClipboard::Clipboard );
//                 if ( cb->supportsSelection() )
//                     cb->setPixmap( copyPix, QClipboard::Selection );
//
//                 break;
//             }
//         }
//     return QPixmap();
// }
