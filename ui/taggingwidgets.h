/***************************************************************************
 *   Copyright (C) 2016 by Jonathan Schultz <jonathan@imatix.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _TAGGINGWIDGETS_H_
#define _TAGGINGWIDGETS_H_

#include <qwidget.h>

#include "core/tagging.h"

class TaggingWidget;

/**
 * A factory to create TaggingWidget's.
 */
class TaggingWidgetFactory
{
public:
    static TaggingWidget * widgetFor( Okular::Tagging * tag );
};

class TaggingWidget
  : public QObject
{
    Q_OBJECT

public:
    TaggingWidget( Okular::Tagging * tag );
    virtual ~TaggingWidget();

    virtual Okular::Tagging::SubType taggingType() const;

    QWidget * appearanceWidget();
    QWidget * extraWidget();

    virtual void applyChanges();

signals:
    void dataChanged();

protected:
    QWidget * createAppearanceWidget();

    virtual QWidget * createStyleWidget();
    virtual QWidget * createExtraWidget();

    Okular::Tagging * m_tag;
    QWidget * m_appearanceWidget;
    QWidget * m_extraWidget;
};

class TextTaggingWidget
  : public TaggingWidget
{
    Q_OBJECT

public:
    TextTaggingWidget( Okular::Tagging * tag );

    virtual void applyChanges();

protected:
    virtual QWidget * createStyleWidget();

private:
    Okular::TextTagging * m_textTag;
};

class BoxTaggingWidget
  : public TaggingWidget
{
    Q_OBJECT

public:
    BoxTaggingWidget( Okular::Tagging * tag );

    virtual void applyChanges();

protected:
    virtual QWidget * createStyleWidget();

private:
    Okular::BoxTagging * m_boxTag;
};

#endif
