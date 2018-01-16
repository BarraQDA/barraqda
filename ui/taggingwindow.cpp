/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "taggingwindow.h"

// qt/kde includes
#include <qapplication.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qsizegrip.h>
#include <qstyle.h>
#include <qtoolbutton.h>
#include <KLocalizedString>
#include <ktextedit.h>
#include <QtCore/QDebug>
#include <qaction.h>
#include <kstandardaction.h>
#include <qmenu.h>

// local includes
#include "core/tagging.h"
#include "core/document.h"
#include "latexrenderer.h"
#include <core/utils.h>
#include <KMessageBox>

// HACK: Use a different class name, otherwise duplicate class is in annotationwindow.cpp

class tagCloseButton
  : public QPushButton
{
    Q_OBJECT

public:
    tagCloseButton( QWidget * parent = Q_NULLPTR )
      : QPushButton( parent )
    {
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QSize size = QSize( 14, 14 ).expandedTo( QApplication::globalStrut() );
        setFixedSize( size );
        setIcon( style()->standardIcon( QStyle::SP_DockWidgetCloseButton ) );
        setIconSize( size );
        setToolTip( i18n( "Close this note" ) );
    }
};


class tagMovableTitle
  : public QWidget
{
    Q_OBJECT

public:
    tagMovableTitle( QWidget * parent )
      : QWidget( parent )
    {
        QVBoxLayout * mainlay = new QVBoxLayout( this );
        mainlay->setMargin( 0 );
        mainlay->setSpacing( 0 );
        // close button row
        QHBoxLayout * buttonlay = new QHBoxLayout();
        mainlay->addLayout( buttonlay );
        titleLabel = new QLabel( this );
        QFont f = titleLabel->font();
        f.setBold( true );
        titleLabel->setFont( f );
        titleLabel->setCursor( Qt::SizeAllCursor );
        buttonlay->addWidget( titleLabel );
        dateLabel = new QLabel( this );
        dateLabel->setAlignment( Qt::AlignTop | Qt::AlignRight );
        f = dateLabel->font();
        f.setPointSize( QFontInfo( f ).pointSize() - 2 );
        dateLabel->setFont( f );
        dateLabel->setCursor( Qt::SizeAllCursor );
        buttonlay->addWidget( dateLabel );
        tagCloseButton * close = new tagCloseButton( this );
        connect( close, &QAbstractButton::clicked, parent, &QWidget::close );
        buttonlay->addWidget( close );
        // option button row
        QHBoxLayout * optionlay = new QHBoxLayout();
        mainlay->addLayout( optionlay );
        authorLabel = new QLabel( this );
        authorLabel->setCursor( Qt::SizeAllCursor );
        authorLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
        optionlay->addWidget( authorLabel );
        optionButton = new QToolButton( this );
        QString opttext = i18n( "Options" );
        optionButton->setText( opttext );
        optionButton->setAutoRaise( true );
        QSize s = QFontMetrics( optionButton->font() ).boundingRect( opttext ).size() + QSize( 8, 8 );
        optionButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        optionButton->setFixedSize( s );
        optionlay->addWidget( optionButton );
        // ### disabled for now
        optionButton->hide();
        latexButton = new QToolButton( this );
        QHBoxLayout * latexlay = new QHBoxLayout();
        QString latextext = i18n ( "This tagging may contain LaTeX code.\nClick here to render." );
        latexButton->setText( latextext );
        latexButton->setAutoRaise( true );
        s = QFontMetrics( latexButton->font() ).boundingRect(0, 0, this->width(), this->height(), 0, latextext ).size() + QSize( 8, 8 );
        latexButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        latexButton->setFixedSize( s );
        latexButton->setCheckable( true );
        latexButton->setVisible( false );
        latexlay->addSpacing( 1 );
        latexlay->addWidget( latexButton );
        latexlay->addSpacing( 1 );
        mainlay->addLayout( latexlay );
        connect(latexButton, SIGNAL(clicked(bool)), parent, SLOT(renderLatex(bool)));
        connect(parent, SIGNAL(containsLatex(bool)), latexButton, SLOT(setVisible(bool)));

        titleLabel->installEventFilter( this );
        dateLabel->installEventFilter( this );
        authorLabel->installEventFilter( this );
    }

    bool eventFilter( QObject * obj, QEvent * e ) override
    {
        if ( obj != titleLabel && obj != authorLabel && obj != dateLabel )
            return false;

        QMouseEvent * me = 0;
        switch ( e->type() )
        {
            case QEvent::MouseButtonPress:
                me = (QMouseEvent*)e;
                mousePressPos = me->pos();
                break;
            case QEvent::MouseButtonRelease:
                mousePressPos = QPoint();
                break;
            case QEvent::MouseMove:
                me = (QMouseEvent*)e;
                parentWidget()->move( me->pos() - mousePressPos + parentWidget()->pos() );
                break;
            default:
                return false;
        }
        return true;
    }

    void setTitle( const QString& title )
    {
        titleLabel->setText( QStringLiteral( " " ) + title );
    }

    void setDate( const QDateTime& dt )
    {
        dateLabel->setText( QLocale().toString( dt, QLocale::ShortFormat ) + QLatin1Char(' ') );
    }

    void setAuthor( const QString& author )
    {
        authorLabel->setText( QStringLiteral( " " ) + author );
    }

    void connectOptionButton( QObject * recv, const char* method )
    {
        connect( optionButton, SIGNAL(clicked()), recv, method );
    }

    void uncheckLatexButton()
    {
        latexButton->setChecked( false );
    }

private:
    QLabel * titleLabel;
    QLabel * dateLabel;
    QLabel * authorLabel;
    QPoint mousePressPos;
    QToolButton * optionButton;
    QToolButton * latexButton;
};


// Qt::SubWindow is needed to make QSizeGrip work
TaggingWindow::TaggingWindow( QWidget * parent, Okular::Tagging * tagging, Okular::Document *document, int page )
    : QFrame( parent, Qt::SubWindow ), m_tagging( tagging ), m_document( document ), m_page( page )
{
    setAutoFillBackground( true );
    setFrameStyle( Panel | Raised );
    setAttribute( Qt::WA_DeleteOnClose );

    textEdit = new KTextEdit( this );
    textEdit->setAcceptRichText( false );
    textEdit->setPlainText( m_tagging->contents() );
    textEdit->installEventFilter( this );
    textEdit->setUndoRedoEnabled( false );

    m_prevCursorPos = textEdit->textCursor().position();
    m_prevAnchorPos = textEdit->textCursor().anchor();

    connect(textEdit, &KTextEdit::textChanged, this, &TaggingWindow::slotsaveWindowText);
    connect(textEdit, &KTextEdit::cursorPositionChanged, this, &TaggingWindow::slotsaveWindowText);
    connect(textEdit, &KTextEdit::aboutToShowContextMenu, this, &TaggingWindow::slotUpdateUndoAndRedoInContextMenu);
    connect(m_document, &Okular::Document::taggingContentsChangedByUndoRedo, this, &TaggingWindow::slotHandleContentsChangedByUndoRedo);

    QVBoxLayout * mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 2 );
    mainlay->setSpacing( 0 );
    m_title = new tagMovableTitle( this );
    mainlay->addWidget( m_title );
    mainlay->addWidget( textEdit );
    QHBoxLayout * lowerlay = new QHBoxLayout();
    mainlay->addLayout( lowerlay );
    lowerlay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    QSizeGrip * sb = new QSizeGrip( this );
    lowerlay->addWidget( sb );

    m_latexRenderer = new GuiUtils::LatexRenderer();
    emit containsLatex( GuiUtils::LatexRenderer::mightContainLatex( m_tagging->contents() ) );

    m_title->setTitle( m_tagging->window().summary() );
    m_title->connectOptionButton( this, SLOT(slotOptionBtn()) );

    setGeometry(10,10,300,300 );

    reloadInfo();
}

TaggingWindow::~TaggingWindow()
{
    delete m_latexRenderer;
}

Okular::Tagging * TaggingWindow::tagging() const
{
    return m_tagging;
}

void TaggingWindow::reloadInfo()
{
    m_title->setAuthor( m_tagging->author() );
    m_title->setDate( m_tagging->modificationDate() );
}

void TaggingWindow::showEvent( QShowEvent * event )
{
    QFrame::showEvent( event );

    // focus the content area by default
    textEdit->setFocus();
}

bool TaggingWindow::eventFilter(QObject *, QEvent *e)
{
    if ( e->type () == QEvent::ShortcutOverride )
    {
        QKeyEvent * keyEvent = static_cast< QKeyEvent * >( e );
        if ( keyEvent->key() == Qt::Key_Escape )
        {
            close();
            return true;
        }
    }
    else if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent == QKeySequence::Undo)
        {
            m_document->undo();
            return true;
        }
        else if (keyEvent == QKeySequence::Redo)
        {
            m_document->redo();
            return true;
        }
    }
    return false;
}

void TaggingWindow::slotUpdateUndoAndRedoInContextMenu(QMenu* menu)
{
    if (!menu) return;

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };

    QAction *kundo = KStandardAction::create( KStandardAction::Undo, m_document, SLOT(undo()), menu);
    QAction *kredo = KStandardAction::create( KStandardAction::Redo, m_document, SLOT(redo()), menu);
    connect(m_document, &Okular::Document::canUndoChanged, kundo, &QAction::setEnabled);
    connect(m_document, &Okular::Document::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(m_document->canUndo());
    kredo->setEnabled(m_document->canRedo());

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction(oldUndo, kundo);
    menu->insertAction(oldRedo, kredo);

    menu->removeAction(oldUndo);
    menu->removeAction(oldRedo);
}

void TaggingWindow::slotOptionBtn()
{
    //TODO: call context menu in pageview
    //emit sig...
}

void TaggingWindow::slotsaveWindowText()
{
    const QString contents = textEdit->toPlainText();
    const int cursorPos = textEdit->textCursor().position();
    if (contents != m_tagging->contents())
    {
        m_document->editPageTaggingContents( m_page, m_tagging, contents, cursorPos, m_prevCursorPos, m_prevAnchorPos);
        emit containsLatex( GuiUtils::LatexRenderer::mightContainLatex( textEdit->toPlainText() ) );
    }
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = textEdit->textCursor().anchor();
}

void TaggingWindow::renderLatex( bool render )
{
    if (render)
    {
        textEdit->setReadOnly( true );
        disconnect(textEdit, &KTextEdit::textChanged, this, &TaggingWindow::slotsaveWindowText);
        disconnect(textEdit, &KTextEdit::cursorPositionChanged, this, &TaggingWindow::slotsaveWindowText);
        textEdit->setAcceptRichText( true );
        QString contents = m_tagging->contents();
        contents = Qt::convertFromPlainText( contents );
        QColor fontColor = textEdit->textColor();
        int fontSize = textEdit->fontPointSize();
        QString latexOutput;
        GuiUtils::LatexRenderer::Error errorCode = m_latexRenderer->renderLatexInHtml( contents, fontColor, fontSize, Okular::Utils::realDpi(nullptr).width(), latexOutput );
        switch ( errorCode )
        {
            case GuiUtils::LatexRenderer::LatexNotFound:
                KMessageBox::sorry( this, i18n( "Ctagging find latex executable." ), i18n( "LaTeX rendering failed" ) );
                m_title->uncheckLatexButton();
                renderLatex( false );
                break;
            case GuiUtils::LatexRenderer::DvipngNotFound:
                KMessageBox::sorry( this, i18n( "Ctagging find dvipng executable." ), i18n( "LaTeX rendering failed" ) );
                m_title->uncheckLatexButton();
                renderLatex( false );
                break;
            case GuiUtils::LatexRenderer::LatexFailed:
                KMessageBox::detailedSorry( this, i18n( "A problem occurred during the execution of the 'latex' command." ), latexOutput, i18n( "LaTeX rendering failed" ) );
                m_title->uncheckLatexButton();
                renderLatex( false );
                break;
            case GuiUtils::LatexRenderer::DvipngFailed:
                KMessageBox::sorry( this, i18n( "A problem occurred during the execution of the 'dvipng' command." ), i18n( "LaTeX rendering failed" ) );
                m_title->uncheckLatexButton();
                renderLatex( false );
                break;
            case GuiUtils::LatexRenderer::NoError:
            default:
                textEdit->setHtml( contents );
                break;
        }
    }
    else
    {
        textEdit->setAcceptRichText( false );
        textEdit->setPlainText( m_tagging->contents() );
        connect(textEdit, &KTextEdit::textChanged, this, &TaggingWindow::slotsaveWindowText);
        connect(textEdit, &KTextEdit::cursorPositionChanged, this, &TaggingWindow::slotsaveWindowText);
        textEdit->setReadOnly( false );
    }
}

void TaggingWindow::slotHandleContentsChangedByUndoRedo(Okular::Tagging* tagging, QString contents, int cursorPos, int anchorPos)
{
    if ( tagging != m_tagging )
    {
        return;
    }

    textEdit->setPlainText(contents);
    QTextCursor c = textEdit->textCursor();
    c.setPosition(anchorPos);
    c.setPosition(cursorPos,QTextCursor::KeepAnchor);
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    textEdit->setTextCursor(c);
    textEdit->setFocus();
    emit containsLatex( GuiUtils::LatexRenderer::mightContainLatex( m_tagging->contents() ) );
}

#include "taggingwindow.moc"
