/***************************************************************************
 *   Copyright (C) 2016 by Jonathan Schultz <jonathan@imatix.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "qdanodes.h"

// qt/kde includes
#include <QtCore/QUuid>
#include <QtGui/QColor>

// local includes
#include "debug_p.h"
#include "document_p.h"
#include "page.h"
#include "tagging.h"

using namespace Okular;

//BEGIN QDANode implementation

// From http://godsnotwheregodsnot.blogspot.ru/2013/11/kmeans-color-quantization-seeding.html
QRgb QDANodeUtils::tagColors [] = {

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

QList< QDANode * > QDANodeUtils::QDANodes = QList< QDANode * > ();

QDANode * QDANodeUtils::retrieve( QString m_uniqueName )
{
    QList< QDANode * >::const_iterator nIt = QDANodeUtils::QDANodes.constBegin(), nEnd = QDANodeUtils::QDANodes.constEnd();
    for ( ; nIt != nEnd; ++nIt )
    {
        if ( (*nIt)->uniqueName() == m_uniqueName )
            return *nIt;
    }
    return 0;
}

void QDANodeUtils::storeQDANodes( Document * doc, QDomElement & domElement, QDomDocument & domDocument )
{
    QList< QDANode * >::const_iterator nIt = QDANodeUtils::QDANodes.constBegin(), nEnd = QDANodeUtils::QDANodes.constEnd();
    for ( ; nIt != nEnd; ++nIt )
    {
        (*nIt)->store( doc, domElement, domDocument );
    }
}

void QDANodeUtils::load( DocumentPrivate *doc_p, const QDomNode& node )
{
    QDomElement e = node.firstChildElement( QStringLiteral("node") );

    while (! e.isNull() )
    {
        //  If same QDANode is already loaded, then more recent one replaces the other.
        QDateTime oldCreationDate, oldModifyDate, newCreationDate, newModifyDate;

        QDANode *qdaNode;
        if ( e.hasAttribute( QStringLiteral("uniqueName") ) )
        {
            QString uniqueName = e.attribute( QStringLiteral("uniqueName") );
            qdaNode = QDANodeUtils::retrieve( uniqueName );

            if ( qdaNode )
            {
                oldCreationDate = qdaNode->creationDate();
                oldModifyDate   = qdaNode->modificationDate();
            }
            else
                qdaNode = new QDANode( uniqueName );
        }
        else
            qdaNode = new QDANode();

        if ( e.hasAttribute( QStringLiteral("creationDate") ) )
            newCreationDate = QDateTime::fromString( e.attribute(QStringLiteral("creationDate")), Qt::ISODate );
        if ( e.hasAttribute( QStringLiteral("modifyDate") ) )
            newModifyDate = QDateTime::fromString( e.attribute(QStringLiteral("modifyDate")), Qt::ISODate );
        newModifyDate = std::max ( newCreationDate, newModifyDate );

        //  If node being loaded is more recently modified then overwrite existing node fields
        //  and attributes.
        if ( newModifyDate >= std::max ( oldCreationDate, oldModifyDate ) )
        {
            if (! newCreationDate.isNull() )
                qdaNode->setModificationDate( newCreationDate );
            if (! newModifyDate.isNull() )
                qdaNode->setModificationDate( newModifyDate );
            if ( e.hasAttribute( QStringLiteral("author") ) )
                qdaNode->setAuthor( e.attribute(QStringLiteral("author")) );
            if ( e.hasAttribute( QStringLiteral("name") ) )
                qdaNode->setName( e.attribute(QStringLiteral("name")) );

            QDomElement attrElement = e.firstChildElement( QStringLiteral("attribute") );
            qdaNode->attributes = QList < QPair< QString, QString> >();
            while (! attrElement.isNull() )
            {
                QString attrName  = attrElement.attribute(QStringLiteral("name"));
                QString attrValue = attrElement.attribute(QStringLiteral("value"));
                if (! attrName.isNull() && ! attrValue.isNull() )
                    qdaNode->attributes.append( QPair< QString, QString>( attrName, attrValue) );

                attrElement = attrElement.nextSiblingElement( QStringLiteral("attribute") );
            }
        }
        QDomElement tagElement = e.firstChildElement( QStringLiteral("tagging") );
        while (! tagElement.isNull() )
        {
            Tagging * tagging = TaggingUtils::createTagging( doc_p->m_parent, tagElement );
            // append tagging to the list or show warning
            if ( tagging )
            {
                if ( tagging->node() && tagging->node() != qdaNode )
                {
                    qCWarning(OkularCoreDebug) << "QDANodeUtils::load tagging node inconsistent in tagging: " << tagging->uniqueName();
                    delete tagging;
                }
                else if ( tagging->subType() == Tagging::TText && static_cast<TextTagging *>(tagging)->reference().isNull() )
                {
                    qCWarning(OkularCoreDebug) << "QDANodeUtils::load tagging has null reference in tagging: " << tagging->uniqueName();
                    delete tagging;
                }
                else
                {
                    Tagging *tagIt = tagging;
                    while ( tagIt )
                    {
                        doc_p->performAddPageTagging(tagIt->pageNum(), tagIt);
                        tagIt = tagIt->next();
                    }
                    qCDebug(OkularCoreDebug) << "restored tagot:" << tagging->uniqueName();
                }
            }
            else
                qCWarning(OkularCoreDebug).nospace() << "Can't restore a tag tagging from XML.";

            tagElement = tagElement.nextSiblingElement( QStringLiteral("tagging") );
        }

        e = e.nextSiblingElement( QStringLiteral("node") );
    }
}

QDANode::QDANode()
    : QDANode( "okular-" + QUuid::createUuid().toString() )
{
}

QDANode::QDANode( QString uniqueName )
    : attributes ( QList < QPair< QString, QString> >() ),
      m_uniqueName( uniqueName ),
      m_name ( QString() ),
      m_color ( QDANodeUtils::tagColors[ QDANodeUtils::QDANodes.length() ] ),
      m_author ( QString() ),
      m_creationDate (),
      m_modifyDate (),
      m_taggings( QList<Tagging *> () )
{
    QDANodeUtils::QDANodes.append(this);
}

QDANode::~QDANode()
{
}

void QDANode::store( Document * doc, QDomElement & domElement, QDomDocument & domDocument ) const
{
    QList< Tagging * >::const_iterator tagIt = m_taggings.constBegin(), tagEnd = m_taggings.constEnd();
    bool nodeStored = false;
    QDomElement e;
    for ( ; tagIt != tagEnd; ++tagIt )
    {
        if ( (*tagIt)->document() == doc && ! nodeStored )
        {
            nodeStored = true;

            e = domDocument.createElement( QStringLiteral("node") );
            domElement.appendChild( e );

            if ( !this->m_name.isEmpty() )
                e.setAttribute( QStringLiteral("name"), this->m_name );
            if ( !this->m_uniqueName.isEmpty() )
                e.setAttribute( QStringLiteral("uniqueName"), this->m_uniqueName );
            if ( !this->m_author.isEmpty() )
                e.setAttribute( QStringLiteral("author"), this->m_author );
            if ( this->m_modifyDate.isValid() )
                e.setAttribute( QStringLiteral("modifyDate"), this->m_modifyDate.toString(Qt::ISODate) );
            if ( this->m_creationDate.isValid() )
                e.setAttribute( QStringLiteral("creationDate"), this->m_creationDate.toString(Qt::ISODate) );

            QList < QPair< QString, QString> >::const_iterator attrIt = this->attributes.constBegin(), attrEnd = this->attributes.constEnd();
            for ( ; attrIt != attrEnd; ++attrIt )
            {
                QDomElement attrElement = domDocument.createElement( QStringLiteral("attribute") );
                e.appendChild( attrElement );
                attrElement.setAttribute( QStringLiteral("name"),  attrIt->first );
                attrElement.setAttribute( QStringLiteral("value"), attrIt->second );
            }

            QDomElement tagElement = domDocument.createElement( QStringLiteral("tagging") );
            tagElement.setAttribute( QStringLiteral("type"), (*tagIt)->subType() );
            e.appendChild( tagElement );
            (*tagIt)->store( tagElement, domDocument );
            qCDebug(OkularCoreDebug) << "save tagging:" << (*tagIt)->uniqueName();
        }
    }
}

QString QDANode::uniqueName() const
{
    return m_uniqueName;
}

void QDANode::setName( QString name )
{
    m_name = name;
}

QString QDANode::name() const
{
    return m_name;
}

void QDANode::setColor( QRgb color )
{
    m_color = color;
}

QRgb QDANode::color() const
{
    return m_color;
}

void QDANode::setAuthor( QString author )
{
    m_author = author;
}

QString QDANode::author() const
{
    return m_author;
}

void QDANode::setCreationDate( QDateTime creationDate )
{
    m_creationDate = creationDate;
}

QDateTime QDANode::creationDate() const
{
    return m_creationDate;
}

void QDANode::setModificationDate( QDateTime modificationDate )
{
    m_modifyDate = modificationDate;
}

QDateTime QDANode::modificationDate() const
{
    return m_modifyDate;
}

void QDANode::addTagging( Tagging *tag )
{
    if ( m_taggings.indexOf( tag ) != -1 )
        qCWarning(OkularCoreDebug) << "QDANode::addTagging tagging already in node tagging: " << tag->uniqueName();
    else
    {
        m_taggings.append( tag );
        tag->setPrevNode ( this );
    }
}

void QDANode::removeTagging( Tagging *tag )
{
    if ( m_taggings.indexOf( tag ) == -1 )
        qCWarning(OkularCoreDebug) << "QDANode::removeTagging tagging not in node tagging: " << tag->uniqueName();
    else
    {
        m_taggings.removeAll( tag );
        tag->setPrevNode ( 0 );
    }
}

//END QDANode implementation
