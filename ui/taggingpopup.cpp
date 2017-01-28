/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "taggingpopup.h"

#include <KLocalizedString>
#include <QMenu>
#include <QIcon>

#include "taggingpropertiesdialog.h"

#include "core/taggings.h"
#include "core/document.h"
#include "guiutils.h"
#include "okmenutitle.h"

Q_DECLARE_METATYPE( TaggingPopup::TagPagePair )

namespace {

TaggingPopup::TaggingPopup( Okular::Document *document, MenuMode mode,
                                  QWidget *parent )
    : mParent( parent ), mDocument( document ), mMenuMode( mode )
{
}

void TaggingPopup::addTagging( Okular::Tagging* tagging, int pageNumber )
{
    TagPagePair pair( tagging, pageNumber );
    if ( !mTaggings.contains( pair ) )
      mTaggings.append( pair );
}

void TaggingPopup::exec( const QPoint &point )
{
    if ( mTaggings.isEmpty() )
        return;

    QMenu menu( mParent );

    QAction *action = 0;

    const char *actionTypeId = "actionType";

    const QString openId = QStringLiteral( "open" );
    const QString deleteId = QStringLiteral( "delete" );
    const QString deleteAllId = QStringLiteral( "deleteAll" );
    const QString propertiesId = QStringLiteral( "properties" );
    const QString saveId = QStringLiteral( "save" );

    if ( mMenuMode == SingleTaggingMode )
    {
        const bool onlyOne = (mTaggings.count() == 1);

        const TagPagePair &pair = mTaggings.at(0);

        menu.addAction( new OKMenuTitle( &menu, i18np( "Tagging", "%1 Taggings", mTaggings.count() ) ) );

        action = menu.addAction( QIcon::fromTheme( QStringLiteral("comment") ), i18n( "&Open Pop-up Note" ) );
        action->setData( QVariant::fromValue( pair ) );
        action->setEnabled( onlyOne );
        action->setProperty( actionTypeId, openId );

        action = menu.addAction( QIcon::fromTheme( QStringLiteral("list-remove") ), i18n( "&Delete" ) );
        action->setEnabled( mDocument->isAllowed( Okular::AllowNotes ) );
        action->setProperty( actionTypeId, deleteAllId );

        action = menu.addAction( QIcon::fromTheme( QStringLiteral("configure") ), i18n( "&Properties" ) );
        action->setData( QVariant::fromValue( pair ) );
        action->setEnabled( onlyOne );
        action->setProperty( actionTypeId, propertiesId );

        if ( onlyOne && taggingHasFileAttachment( pair.tagging ) )
        {
            const Okular::EmbeddedFile *embeddedFile = embeddedFileFromTagging( pair.tagging );
            if ( embeddedFile )
            {
                const QString saveText = i18nc( "%1 is the name of the file to save", "&Save '%1'...", embeddedFile->name() );

                menu.addSeparator();
                action = menu.addAction( QIcon::fromTheme( QStringLiteral("document-save") ), saveText );
                action->setData( QVariant::fromValue( pair ) );
                action->setProperty( actionTypeId, saveId );
            }
        }
    }
    else
    {
        foreach ( const TagPagePair& pair, mTaggings )
        {
            menu.addAction( new OKMenuTitle( &menu, GuiUtils::captionForTagging( pair.tagging ) ) );

            action = menu.addAction( QIcon::fromTheme( QStringLiteral("comment") ), i18n( "&Open Pop-up Note" ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, openId );

            action = menu.addAction( QIcon::fromTheme( QStringLiteral("list-remove") ), i18n( "&Delete" ) );
            action->setEnabled( mDocument->isAllowed( Okular::AllowNotes ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, deleteId );

            action = menu.addAction( QIcon::fromTheme( QStringLiteral("configure") ), i18n( "&Properties" ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, propertiesId );

            if ( taggingHasFileAttachment( pair.tagging ) )
            {
                const Okular::EmbeddedFile *embeddedFile = embeddedFileFromTagging( pair.tagging );
                if ( embeddedFile )
                {
                    const QString saveText = i18nc( "%1 is the name of the file to save", "&Save '%1'...", embeddedFile->name() );

                    menu.addSeparator();
                    action = menu.addAction( QIcon::fromTheme( QStringLiteral("document-save") ), saveText );
                    action->setData( QVariant::fromValue( pair ) );
                    action->setProperty( actionTypeId, saveId );
                }
            }
        }
    }

    QAction *choice = menu.exec( point.isNull() ? QCursor::pos() : point );

    // check if the user really selected an action
    if ( choice ) {
        const TagPagePair pair = choice->data().value<TagPagePair>();

        const QString actionType = choice->property( actionTypeId ).toString();
        if ( actionType == openId ) {
            emit openTaggingWindow( pair.tagging, pair.pageNumber );
        } else if( actionType == deleteId ) {
            if ( pair.pageNumber != -1 )
                mDocument->removePageTagging( pair.pageNumber, pair.tagging );
        } else if( actionType == deleteAllId ) {
            Q_FOREACH ( const TagPagePair& pair, mTaggings )
            {
                if ( pair.pageNumber != -1 )
                    mDocument->removePageTagging( pair.pageNumber, pair.tagging );
            }
        } else if( actionType == propertiesId ) {
            if ( pair.pageNumber != -1 ) {
                AnnotsPropertiesDialog propdialog( mParent, mDocument, pair.pageNumber, pair.tagging );
                propdialog.exec();
            }
        } else if( actionType == saveId ) {
            Okular::EmbeddedFile *embeddedFile = embeddedFileFromTagging( pair.tagging );
            GuiUtils::saveEmbeddedFile( embeddedFile, mParent );
        }
    }
}

#include "moc_taggingpopup.cpp"
