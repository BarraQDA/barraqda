/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _TAGGINGWINDOW_H_
#define _TAGGINGWINDOW_H_

#include <qcolor.h>
#include <qframe.h>

namespace Okular {
class Tagging;
class Document;
}

namespace GuiUtils {
class LatexRenderer;
}

class KTextEdit;
class tagMovableTitle;
class QMenu;

class TaggingWindow : public QFrame
{
    Q_OBJECT
    public:
        TaggingWindow( QWidget * parent, Okular::Tagging * tagging, Okular::Document * document, int page );
        ~TaggingWindow();

        void reloadInfo();

    private:
        tagMovableTitle * m_title;
        KTextEdit *textEdit;
        QColor m_color;
        GuiUtils::LatexRenderer *m_latexRenderer;
        Okular::Tagging* m_tagging;
        Okular::Document* m_document;
        int m_page;
        int m_prevCursorPos;
        int m_prevAnchorPos;

    protected:
        void showEvent( QShowEvent * event ) Q_DECL_OVERRIDE;
        bool eventFilter( QObject * obj, QEvent * event ) Q_DECL_OVERRIDE;

    private Q_SLOTS:
        void slotUpdateUndoAndRedoInContextMenu(QMenu *menu);
        void slotOptionBtn();
        void slotsaveWindowText();
        void renderLatex( bool render );
        void slotHandleContentsChangedByUndoRedo( Okular::Tagging* tagging, QString contents, int cursorPos, int anchorPos);

    Q_SIGNALS:
        void containsLatex( bool );
};


#endif
