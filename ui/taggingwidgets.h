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

class QComboBox;
class QScrollArea;
class QLineEdit;
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

    QWidget * nodeWidget();

    void applyChanges();

Q_SIGNALS:
    void dataChanged();

private Q_SLOTS:
    void nodeChanged();

protected:
    QWidget * createAppearanceWidget();

    virtual QWidget * createNodeWidget();

    Okular::Tagging * m_tag;
    QWidget * m_nodeWidget;

private:
    QComboBox       * m_nodeBox;
    Okular::QDANode * m_QDANode;
    int               m_attrCount;
    QScrollArea     * m_attrArea;
    QLineEdit      ** m_attrName, **m_attrValue;

    void loadAttributes( QWidget *widget );
};

#endif
