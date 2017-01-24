/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationpropertiesdialog.h"

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
#include "core/annotations.h"
#include "annotationwidgets.h"


AnnotsPropertiesDialog::AnnotsPropertiesDialog( QWidget *parent, Okular::Document *document, int docpage, Okular::Annotation *ann )
    : KPageDialog( parent ), m_document( document ), m_page( docpage ), modified( false )
{
    setFaceType( Tabbed );
    m_annot=ann;
    const bool canEditAnnotations = m_document->canModifyPageAnnotation( ann );
    setCaptionTextbyAnnotType();
    if ( canEditAnnotations )
    {
        setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel );
        button( QDialogButtonBox::Apply )->setEnabled( false );
        connect( button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &AnnotsPropertiesDialog::slotapply);
        connect( button( QDialogButtonBox::Ok ), &QPushButton::clicked, this, &AnnotsPropertiesDialog::slotapply);
    }
    else
    {
        setStandardButtons( QDialogButtonBox::Close );
        button( QDialogButtonBox::Close )->setDefault( true );
    }

    m_annotWidget = AnnotationWidgetFactory::widgetFor( ann );

    QLabel* tmplabel;
  //1. Appearance
    //BEGIN tab1
    QWidget *appearanceWidget = m_annotWidget->appearanceWidget();
    appearanceWidget->setEnabled( canEditAnnotations );
    addPage( appearanceWidget, i18n( "&Appearance" ) );
    //END tab1

    //BEGIN tab 2
    QFrame* page = new QFrame( this );
    addPage( page, i18n( "&General" ) );
//    m_tabitem[1]->setIcon( QIcon::fromTheme( "fonts" ) );
    QGridLayout* gridlayout = new QGridLayout( page );
    tmplabel = new QLabel( i18n( "&Author:" ), page );
    AuthorEdit = new KLineEdit( ann->author(), page );
    AuthorEdit->setEnabled( canEditAnnotations );
    tmplabel->setBuddy( AuthorEdit );
    gridlayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    gridlayout->addWidget( AuthorEdit, 0, 1 );

    tmplabel = new QLabel( page );
    tmplabel->setText( i18n( "Created: %1", QLocale().toString( ann->creationDate(), QLocale::LongFormat ) ) );
    tmplabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
    gridlayout->addWidget( tmplabel, 1, 0, 1, 2 );

    m_modifyDateLabel = new QLabel( page );
    m_modifyDateLabel->setText( i18n( "Modified: %1", QLocale().toString( ann->modificationDate(), QLocale::LongFormat ) ) );
    m_modifyDateLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
    gridlayout->addWidget( m_modifyDateLabel, 2, 0, 1, 2 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ), 3, 0 );
    //END tab 2

    QWidget * extraWidget = m_annotWidget->extraWidget();
    if ( extraWidget )
    {
        addPage( extraWidget, extraWidget->windowTitle() );
    }

    //BEGIN connections
    connect(AuthorEdit, &QLineEdit::textChanged, this, &AnnotsPropertiesDialog::setModified);
    connect(m_annotWidget, &AnnotationWidget::dataChanged, this, &AnnotsPropertiesDialog::setModified);
    //END

#if 0
    qCDebug(OkularUiDebug) << "Annotation details:";
    qCDebug(OkularUiDebug).nospace() << " => unique name: '" << ann->uniqueName() << "'";
    qCDebug(OkularUiDebug) << " => flags:" << QString::number( m_annot->flags(), 2 );
#endif

    resize( sizeHint() );
}
AnnotsPropertiesDialog::~AnnotsPropertiesDialog()
{
    delete m_annotWidget;
}


void AnnotsPropertiesDialog::setCaptionTextbyAnnotType()
{
    Okular::Annotation::SubType type=m_annot->subType();
    QString captiontext;
    switch(type)
    {
        case Okular::Annotation::AText:
            if(((Okular::TextAnnotation*)m_annot)->textType()==Okular::TextAnnotation::Linked)
                captiontext = i18n( "Pop-up Note Properties" );
            else
                captiontext = i18n( "Inline Note Properties" );
            break;
        case Okular::Annotation::ALine:
            if ( ((Okular::LineAnnotation*)m_annot)->linePoints().count() == 2 )
                captiontext = i18n( "Straight Line Properties" );
            else
                captiontext = i18n( "Polygon Properties" );
            break;
        case Okular::Annotation::AGeom:
            captiontext = i18n( "Geometry Properties" );
            break;
        case Okular::Annotation::AHighlight:
            captiontext = i18n( "Text Markup Properties" );
            break;
        case Okular::Annotation::AStamp:
            captiontext = i18n( "Stamp Properties" );
            break;
        case Okular::Annotation::AInk:
            captiontext = i18n( "Freehand Line Properties" );
            break;
        case Okular::Annotation::ACaret:
            captiontext = i18n( "Caret Properties" );
            break;
        case Okular::Annotation::AFileAttachment:
            captiontext = i18n( "File Attachment Properties" );
            break;
        case Okular::Annotation::ASound:
            captiontext = i18n( "Sound Properties" );
            break;
        case Okular::Annotation::AMovie:
            captiontext = i18n( "Movie Properties" );
            break;
        default:
            captiontext = i18n( "Annotation Properties" );
            break;
    }
        setWindowTitle( captiontext );
}

void AnnotsPropertiesDialog::setModified()
{
    modified = true;
    button( QDialogButtonBox::Apply )->setEnabled( true );
}

void AnnotsPropertiesDialog::slotapply()
{
    if ( !modified )
        return;

    m_document->prepareToModifyAnnotationProperties( m_annot );
    m_annot->setAuthor( AuthorEdit->text() );
    m_annot->setModificationDate( QDateTime::currentDateTime() );

    m_annotWidget->applyChanges();

    m_document->modifyPageAnnotationProperties( m_page, m_annot );

    m_modifyDateLabel->setText( i18n( "Modified: %1", QLocale().toString( m_annot->modificationDate(), QLocale::LongFormat ) ) );

    modified = false;
    button( QDialogButtonBox::Apply )->setEnabled( false );
}

#include "moc_annotationpropertiesdialog.cpp"

