/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "taggingwidgets.h"

// qt/kde includes

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qscrollarea.h>
#include <KLocalizedString>

#include "core/document.h"
#include "guiutils.h"

TaggingWidget * TaggingWidgetFactory::widgetFor( Okular::Tagging * tag )
{
    return new TaggingWidget( tag );
}


TaggingWidget::TaggingWidget( Okular::Tagging * tag )
    : QObject(), m_tag( tag ), m_nodeWidget( 0 ), m_nodeBox( 0 ), m_QDANode( 0 ),
      m_attrCount( 0 ), m_attrArea( 0 ), m_attrName( 0 ), m_attrValue( 0 )
{
}

TaggingWidget::~TaggingWidget()
{
}

Okular::Tagging::SubType TaggingWidget::taggingType() const
{
    return m_tag->subType();
}

QWidget * TaggingWidget::nodeWidget()
{
    if (! m_nodeWidget )
        m_nodeWidget = createNodeWidget();

    return m_nodeWidget;
}

QWidget * TaggingWidget::createNodeWidget()
{
    QWidget * widget = new QWidget();
    widget->setWindowTitle( i18nc( "QDA Node name", "Node" ) );

    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    lay->setAlignment( Qt::AlignTop );
    QHBoxLayout * nodeLay = new QHBoxLayout();
    lay->addLayout( nodeLay );

    QLabel * tmplabel = new QLabel( i18n( "&Name:" ), widget );
    nodeLay->addWidget( tmplabel, 0, Qt::AlignRight );
    m_nodeBox = new QComboBox( widget );
    m_nodeBox->setEditable( true );
    tmplabel->setBuddy( m_nodeBox );
    nodeLay->addWidget( m_nodeBox, 0 );

    QList< Okular::QDANode * >::const_iterator nIt = Okular::QDANodeUtils::QDANodes.constBegin(), nEnd = Okular::QDANodeUtils::QDANodes.constEnd();
    int i = 0;
    for ( ; nIt != nEnd; ++nIt )
    {
        QPixmap pixmap(100,100);
        pixmap.fill((*nIt)->color());
        m_nodeBox->addItem( pixmap, (*nIt)->name() );
        if ( (*nIt)->uniqueName() == m_tag->node()->uniqueName() )
            m_nodeBox->setCurrentIndex( i );
        i++;
    }

    m_attrArea = new QScrollArea( widget );
    m_attrArea->setWidgetResizable( true );

    lay->addWidget( m_attrArea );

    this->loadAttributes( widget );

    connect( m_nodeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(nodeChanged()) );
    connect( m_nodeBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dataChanged()) );
    connect( m_nodeBox, SIGNAL(currentTextChanged(const QString &)), this, SIGNAL(dataChanged()) );

    return widget;
}

void TaggingWidget::applyChanges()
{
    Okular::QDANode * node = Okular::QDANodeUtils::QDANodes.at( m_nodeBox->currentIndex() );
    m_tag->setNode( node );
    node->setName( m_nodeBox->currentText() );

    //  Recreate the attribute hash table. Note extra line for potential new attribute.
    m_tag->node()->attributes = QList < QPair< QString, QString> >();
    for ( int i = 0; i < m_attrCount; i++ )
    {
        QString attrName  = m_attrName [i]->text();
        QString attrValue = m_attrValue[i]->text();
        if ( ! attrName.isEmpty() && ! attrValue.isEmpty() )
            m_tag->node()->attributes.append( QPair< QString, QString>( attrName, attrValue) );
    }
}

void TaggingWidget::loadAttributes( QWidget *widget )
{
    if ( m_attrCount )
    {
        for ( int i = 0; i < m_attrCount; i++ )
        {
            disconnect( m_attrName[i],  0, 0, 0 );
            disconnect( m_attrValue[i], 0, 0, 0 );
            delete m_attrName[i];
            delete m_attrValue[i];

        }
        delete m_attrName;
        delete m_attrValue;
    }

    m_QDANode = Okular::QDANodeUtils::QDANodes.at( m_nodeBox->currentIndex() );

    QWidget *attrWidget = new QWidget( widget );
    QGridLayout *attrLay = new QGridLayout( attrWidget );
    attrLay->setAlignment( Qt::AlignTop );
    attrWidget->setLayout( attrLay );
    m_attrArea->setWidget( attrWidget );

    m_attrCount = m_QDANode->attributes.count() + 1;

    m_attrName  = new QLineEdit *[ m_attrCount ];
    m_attrValue = new QLineEdit *[ m_attrCount ];
    QList < QPair< QString, QString> >::const_iterator attrIt = m_QDANode->attributes.constBegin(), attrEnd = m_QDANode->attributes.constEnd();
    int i = 0;
    for ( ; attrIt != attrEnd; ++attrIt )
    {
        m_attrName[i]  = new QLineEdit( attrIt->first,  widget );
        m_attrValue[i] = new QLineEdit( attrIt->second, widget );
        attrLay->addWidget( m_attrName[i],  i, 0) ;
        attrLay->addWidget( m_attrValue[i], i, 1) ;
        connect( m_attrName [i], SIGNAL(textChanged(const QString &)), this, SIGNAL(dataChanged()) );
        connect( m_attrValue[i], SIGNAL(textChanged(const QString &)), this, SIGNAL(dataChanged()) );
        i++;
    }

    //  Add blank space for new attribute
    m_attrName[i]  = new QLineEdit( QString(), widget );
    m_attrValue[i] = new QLineEdit( QString(), widget );
    attrLay->addWidget( m_attrName[i],  i, 0) ;
    attrLay->addWidget( m_attrValue[i], i, 1) ;
    connect( m_attrName [i], SIGNAL(textChanged(const QString &)), this, SIGNAL(dataChanged()) );
    connect( m_attrValue[i], SIGNAL(textChanged(const QString &)), this, SIGNAL(dataChanged()) );
}

void TaggingWidget::nodeChanged()
{
    this->loadAttributes( m_nodeWidget );
}

#include "taggingwidgets.moc"
