/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PRESENTATIONWIDGET_H_
#define _OKULAR_PRESENTATIONWIDGET_H_

#include <QDomElement>
#include <qlist.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <qwidget.h>
#include "core/area.h"
#include "core/observer.h"
#include "core/pagetransition.h"

class QLineEdit;
class QToolBar;
class QTimer;
class QGestureEvent;
class KActionCollection;
class KSelectAction;
class SmoothPathEngine;
struct PresentationFrame;
class PresentationSearchBar;
class DrawingToolActions;

namespace Okular {
class Action;
class Annotation;
class Document;
class MovieAction;
class Page;
class RenditionAction;
}

/**
 * @short A widget that shows pages as fullscreen slides (with transitions fx).
 *
 * This is a fullscreen widget that displays 
 */
class PresentationWidget : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        PresentationWidget( QWidget * parent, Okular::Document * doc, DrawingToolActions * drawingToolActions, KActionCollection * collection );
        ~PresentationWidget();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags ) override;
        void notifyViewportChanged( bool smoothMove ) override;
        void notifyPageChanged( int pageNumber, int changedFlags ) override;
        bool canUnloadPixmap( int pageNumber ) const override;
        void notifyCurrentPageChanged( int previous, int current ) override;

    public Q_SLOTS:
        void slotFind();

    protected:
        // widget events
        bool event( QEvent * e ) override;
        void keyPressEvent( QKeyEvent * e ) override;
        void wheelEvent( QWheelEvent * e ) override;
        void mousePressEvent( QMouseEvent * e ) override;
        void mouseReleaseEvent( QMouseEvent * e ) override;
        void mouseMoveEvent( QMouseEvent * e ) override;
        void paintEvent( QPaintEvent * e ) override;
        void resizeEvent( QResizeEvent * e ) override;
        void leaveEvent( QEvent * e ) override;
        bool gestureEvent (QGestureEvent * e );

    private:
        const void * getObjectRect( Okular::ObjectRect::ObjectType type, int x, int y, QRect * geometry = nullptr ) const;
        const Okular::Action * getLink( int x, int y, QRect * geometry = nullptr ) const;
        const Okular::Annotation * getAnnotation( int x, int y, QRect * geometry = nullptr ) const;
        void testCursorOnLink( int x, int y );
        void overlayClick( const QPoint & position );
        void changePage( int newPage );
        void generatePage( bool disableTransition = false );
        void generateIntroPage( QPainter & p );
        void generateContentsPage( int page, QPainter & p );
        void generateOverlay();
        void initTransition( const Okular::PageTransition *transition );
        const Okular::PageTransition defaultTransition() const;
        const Okular::PageTransition defaultTransition( int ) const;
        QRect routeMouseDrawingEvent( QMouseEvent * );
        void startAutoChangeTimer();
        void recalcGeometry();
        void repositionContent();
        void requestPixmaps();
        void setScreen( int );
        void applyNewScreenSize( const QSize & oldSize );
        void inhibitPowerManagement();
        void allowPowerManagement();
        void showTopBar( bool );
        // create actions that interact with this widget
        void setupActions();
        void setPlayPauseIcon();

        // cache stuff
        int m_width;
        int m_height;
        QPixmap m_lastRenderedPixmap;
        QPixmap m_lastRenderedOverlay;
        QRect m_overlayGeometry;
        const Okular::Action * m_pressedLink;
        bool m_handCursor;
        SmoothPathEngine * m_drawingEngine;
        QRect m_drawingRect;
        int m_screen;
        uint m_screenInhibitCookie;
        int m_sleepInhibitCookie;

        // transition related
        QTimer * m_transitionTimer;
        QTimer * m_overlayHideTimer;
        QTimer * m_nextPageTimer;
        int m_transitionDelay;
        int m_transitionMul;
        int m_transitionSteps;
        QList< QRect > m_transitionRects;
        Okular::PageTransition m_currentTransition;
        QPixmap m_currentPagePixmap;
        QPixmap m_previousPagePixmap;
        double m_currentPixmapOpacity;

        // misc stuff
        QWidget * m_parentWidget;
        Okular::Document * m_document;
        QVector< PresentationFrame * > m_frames;
        int m_frameIndex;
        QStringList m_metaStrings;
        QToolBar * m_topBar;
        QLineEdit *m_pagesEdit;
        PresentationSearchBar *m_searchBar;
        KActionCollection * m_ac;
        KSelectAction * m_screenSelect;
        QDomElement m_currentDrawingToolElement;
        bool m_isSetup;
        bool m_blockNotifications;
        bool m_inBlackScreenMode;
        bool m_showSummaryView;
        bool m_advanceSlides;
        bool m_goToNextPageOnRelease;

    private Q_SLOTS:
        void slotNextPage();
        void slotPrevPage();
        void slotFirstPage();
        void slotLastPage();
        void slotHideOverlay();
        void slotTransitionStep();
        void slotDelayedEvents();
        void slotPageChanged();
        void clearDrawings();
        void screenResized( int );
        void chooseScreen( QAction * );
        void toggleBlackScreenMode( bool );
        void slotProcessMovieAction( const Okular::MovieAction *action );
        void slotProcessRenditionAction( const Okular::RenditionAction *action );
        void slotTogglePlayPause();
        void slotChangeDrawingToolEngine( const QDomElement &doc );
        void slotAddDrawingToolActions();
};

#endif
