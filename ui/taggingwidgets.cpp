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

#include "core/document.h"
#include "guiutils.h"



TaggingWidget * TaggingWidgetFactory::widgetFor( Okular::Tagging * tag )
{
    switch ( tag->subType() )
    {
        case Okular::Tagging::TText:
            return new TextTaggingWidget( tag );
            break;
        case Okular::Tagging::TBox:
            return new BoxTaggingWidget( tag );
            break;
        // shut up gcc
        default:
            ;
    }
    // cases not covered yet: return a generic widget
    return new TaggingWidget( tag );
}


TaggingWidget::TaggingWidget( Okular::Tagging * tag )
    : QObject(), m_tag( tag ), m_appearanceWidget( 0 ), m_extraWidget( 0 )
{
}

TaggingWidget::~TaggingWidget()
{
}

Okular::Tagging::SubType TaggingWidget::taggingType() const
{
    return m_tag->subType();
}

QWidget * TaggingWidget::appearanceWidget()
{
    if ( m_appearanceWidget )
        return m_appearanceWidget;

    m_appearanceWidget = createAppearanceWidget();
    return m_appearanceWidget;
}

QWidget * TaggingWidget::extraWidget()
{
    if ( m_extraWidget )
        return m_extraWidget;

    m_extraWidget = createExtraWidget();
    return m_extraWidget;
}

void TaggingWidget::applyChanges()
{
}

QWidget * TaggingWidget::createAppearanceWidget()
{
    QWidget * widget = new QWidget();

    return widget;
}

QWidget * TaggingWidget::createStyleWidget()
{
    return 0;
}

QWidget * TaggingWidget::createExtraWidget()
{
    return 0;
}


TextTaggingWidget::TextTaggingWidget( Okular::Tagging * tag )
    : TaggingWidget( tag )
{
    m_textTag = static_cast< Okular::TextTagging * >( tag );
}

QWidget * TextTaggingWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();

    return widget;
}

void TextTaggingWidget::applyChanges()
{
    TaggingWidget::applyChanges();
}

BoxTaggingWidget::BoxTaggingWidget( Okular::Tagging * tag )
    : TaggingWidget( tag )
{
    m_boxTag = static_cast< Okular::BoxTagging * >( tag );
}

QWidget * BoxTaggingWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();

    return widget;
}

void BoxTaggingWidget::applyChanges()
{
    TaggingWidget::applyChanges();
}

#include "taggingwidgets.moc"
