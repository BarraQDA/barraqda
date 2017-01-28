/***************************************************************************
 *   Copyright (C) 2017 by Jonathan Schultz <jonathan@schultz.la>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _TAGGINGPROPERTIESDIALOG_H_
#define _TAGGINGPROPERTIESDIALOG_H_

#include <kpagedialog.h>

class QLabel;
class QLineEdit;
class TaggingWidget;

namespace Okular {
class Tagging;
class Document;
}

class TagsPropertiesDialog : public KPageDialog
{
    Q_OBJECT
public:
    TagsPropertiesDialog( QWidget *parent, Okular::Document *document, int docpage, Okular::Tagging *tag );
    ~TagsPropertiesDialog();

private:
    Okular::Document *m_document;
    int m_page;
    bool modified;
    Okular::Tagging* m_tag;    //source tagging
    //dialog widgets:
    QLineEdit *AuthorEdit;
    TaggingWidget *m_tagWidget;
    QLabel *m_modifyDateLabel;

    void setCaptionTextbyTagType();

private Q_SLOTS:
    void setModified();
    void slotapply();
};


#endif
