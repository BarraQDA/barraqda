/***************************************************************************
 *   Copyright (C) 2017 by Jonathan Schultz <jonathan@schultz.la>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "taggingpropertiesdialog.h"

// qt/kde includes
#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qheaderview.h>
#include <QtWidgets/qpushbutton.h>
#include <qtextedit.h>
#include <QIcon>
#include <klineedit.h>
#include <KLocalizedString>

// local includes
#include "core/document.h"
#include "core/page.h"
#include "core/tagging.h"
#include "taggingwidgets.h"


TagsPropertiesDialog::TagsPropertiesDialog( QWidget *parent, Okular::Document *document, int docpage, Okular::Tagging *tag )
    : KPageDialog( parent ), m_document( document ), m_page( docpage ), modified( false )
{
    setFaceType( Tabbed );
    m_tag=tag;
    setCaptionTextbyTagType();
    setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel );
    button( QDialogButtonBox::Apply )->setEnabled( false );
    connect( button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &TagsPropertiesDialog::slotapply);
    connect( button( QDialogButtonBox::Ok ), &QPushButton::clicked, this, &TagsPropertiesDialog::slotapply);

    m_tagWidget = TaggingWidgetFactory::widgetFor( tag );

    QLabel* tmplabel;
  //1. Appearance
    //BEGIN tab1
    QWidget *nodeWidget = m_tagWidget->nodeWidget();
    addPage( nodeWidget, i18n( "Node" ) );
    //END tab1

    //BEGIN connections
    connect(m_tagWidget, &TaggingWidget::dataChanged, this, &TagsPropertiesDialog::setModified);
    //END

#if 0
    qCDebug(OkularUiDebug) << "Tagging details:";
    qCDebug(OkularUiDebug).nospace() << " => unique name: '" << tag->uniqueName() << "'";
    qCDebug(OkularUiDebug) << " => flags:" << QString::number( m_tag->flags(), 2 );
#endif

    resize( sizeHint() );
}
TagsPropertiesDialog::~TagsPropertiesDialog()
{
    delete m_tagWidget;
}


void TagsPropertiesDialog::setCaptionTextbyTagType()
{
    Okular::Tagging::SubType type=m_tag->subType();
    QString captiontext;
    switch(type)
    {
        case Okular::Tagging::TText:
            captiontext = i18n( "Text Tag Properties" );
            break;
        case Okular::Tagging::TBox:
            captiontext = i18n( "Box Tag Properties" );
            break;
        default:
            captiontext = i18n( "Tagging Properties" );
            break;
    }
        setWindowTitle( captiontext );
}

void TagsPropertiesDialog::setModified()
{
    modified = true;
    button( QDialogButtonBox::Apply )->setEnabled( true );
}

void TagsPropertiesDialog::slotapply()
{
    if ( !modified )
        return;

    m_document->prepareToModifyTaggingProperties( m_tag );
    m_tag->setModificationDate( QDateTime::currentDateTime() );

    m_tagWidget->applyChanges();

    m_document->modifyPageTaggingProperties( m_page, m_tag );

    modified = false;
    button( QDialogButtonBox::Apply )->setEnabled( false );
}

#include "moc_taggingpropertiesdialog.cpp"

