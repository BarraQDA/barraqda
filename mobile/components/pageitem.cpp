/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "pageitem.h"
#include "documentitem.h"

#include <QPainter>
#include <QTimer>
#include <QStyleOptionGraphicsItem>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include <core/bookmarkmanager.h>
#include <core/document.h>
#include <core/generator.h>
#include <core/page.h>

#include "ui/pagepainter.h"
#include "ui/priorities.h"
#include "settings.h"

#define REDRAW_TIMEOUT 250

PageItem::PageItem(QQuickItem *parent)
    : QQuickItem(parent),
      Okular::View( QLatin1String( "PageView" ) ),
      m_page(nullptr),
      m_smooth(false),
      m_bookmarked(false),
      m_isThumbnail(false)
{
    setFlag(QQuickItem::ItemHasContents, true);

    m_viewPort.rePos.enabled = true;

    m_redrawTimer = new QTimer(this);
    m_redrawTimer->setInterval(REDRAW_TIMEOUT);
    m_redrawTimer->setSingleShot(true);
    connect(m_redrawTimer, &QTimer::timeout, this, &PageItem::paint);
    connect(this, &QQuickItem::windowChanged, m_redrawTimer, [this]() {m_redrawTimer->start(); });
}


PageItem::~PageItem()
{
}

void PageItem::setFlickable(QQuickItem *flickable)
{
    if (m_flickable.data() == flickable) {
        return;
    }

    //check the object can act as a flickable
    if (!flickable->property("contentX").isValid() ||
        !flickable->property("contentY").isValid()) {
        return;
    }

    if (m_flickable) {
        disconnect(m_flickable.data(), nullptr, this, nullptr);
    }

    //check the object can act as a flickable
    if (!flickable->property("contentX").isValid() ||
        !flickable->property("contentY").isValid()) {
        m_flickable.clear();
        return;
    }

    m_flickable = flickable;

    if (flickable) {
        connect(flickable, SIGNAL(contentXChanged()), this, SLOT(contentXChanged()));
        connect(flickable, SIGNAL(contentYChanged()), this, SLOT(contentYChanged()));
    }

    emit flickableChanged();
}

QQuickItem *PageItem::flickable() const
{
    return m_flickable.data();
}

DocumentItem *PageItem::document() const
{
    return m_documentItem.data();
}

void PageItem::setDocument(DocumentItem *doc)
{
    if (doc == m_documentItem.data() || !doc) {
        return;
    }

    m_page = nullptr;
    disconnect(doc, nullptr, this, nullptr);
    m_documentItem = doc;
    Observer *observer = m_isThumbnail ? m_documentItem.data()->thumbnailObserver() : m_documentItem.data()->pageviewObserver();
    connect(observer, &Observer::pageChanged, this, &PageItem::pageHasChanged);
    connect(doc->document()->bookmarkManager(), &Okular::BookmarkManager::bookmarksChanged,
            this, &PageItem::checkBookmarksChanged);
    setPageNumber(0);
    emit documentChanged();
    m_redrawTimer->start();

    connect(doc, &DocumentItem::urlChanged, this, &PageItem::refreshPage);
}

int PageItem::pageNumber() const
{
    return m_viewPort.pageNumber;
}

void PageItem::setPageNumber(int number)
{
    if ((m_page && m_viewPort.pageNumber == number) ||
        !m_documentItem ||
        !m_documentItem.data()->isOpened() ||
        number < 0) {
        return;
    }

    m_viewPort.pageNumber = number;
    refreshPage();
    emit pageNumberChanged();
    checkBookmarksChanged();
}

void PageItem::refreshPage()
{
    if (uint(m_viewPort.pageNumber) < m_documentItem.data()->document()->pages()) {
        m_page = m_documentItem.data()->document()->page(m_viewPort.pageNumber);
    } else {
        m_page = nullptr;
    }

    emit implicitWidthChanged();
    emit implicitHeightChanged();

    m_redrawTimer->start();
}

int PageItem::implicitWidth() const
{
    if (m_page) {
        return m_page->width();
    }
    return 0;
}

int PageItem::implicitHeight() const
{
    if (m_page) {
        return m_page->height();
    }
    return 0;
}

void PageItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool PageItem::smooth() const
{
    return m_smooth;
}

bool PageItem::isBookmarked()
{
    return m_bookmarked;
}

void PageItem::setBookmarked(bool bookmarked)
{
    if (!m_documentItem) {
        return;
    }

    if (bookmarked == m_bookmarked) {
        return;
    }

    if (bookmarked) {
        m_documentItem.data()->document()->bookmarkManager()->addBookmark(m_viewPort);
    } else {
        m_documentItem.data()->document()->bookmarkManager()->removeBookmark(m_viewPort.pageNumber);
    }
    m_bookmarked = bookmarked;
    emit bookmarkedChanged();
}

QStringList PageItem::bookmarks() const
{
    QStringList list;
    foreach(const KBookmark &bookmark, m_documentItem.data()->document()->bookmarkManager()->bookmarks(m_viewPort.pageNumber)) {
        list << bookmark.url().toString();
    }
    return list;
}

void PageItem::goToBookmark(const QString &bookmark)
{
    Okular::DocumentViewport viewPort(QUrl::fromUserInput(bookmark).fragment(QUrl::FullyDecoded));
    setPageNumber(viewPort.pageNumber);

    //Are we in a flickable?
    if (m_flickable) {
        //normalizedX is a proportion, so contentX will be the difference between document and viewport times normalizedX
        m_flickable.data()->setProperty("contentX", qMax((qreal)0, width() - m_flickable.data()->width()) * viewPort.rePos.normalizedX);

        m_flickable.data()->setProperty("contentY", qMax((qreal)0, height() - m_flickable.data()->height()) * viewPort.rePos.normalizedY);
    }
}

QPointF PageItem::bookmarkPosition(const QString &bookmark) const
{
    Okular::DocumentViewport viewPort(QUrl::fromUserInput(bookmark).fragment(QUrl::FullyDecoded));

    if (viewPort.pageNumber != m_viewPort.pageNumber) {
        return QPointF(-1, -1);
    }

    return QPointF(qMax((qreal)0, width() - m_flickable.data()->width()) * viewPort.rePos.normalizedX,
                   qMax((qreal)0, height() - m_flickable.data()->height()) * viewPort.rePos.normalizedY);
}

void PageItem::setBookmarkAtPos(qreal x, qreal y)
{
    Okular::DocumentViewport viewPort(m_viewPort);
    viewPort.rePos.normalizedX = x;
    viewPort.rePos.normalizedY = y;

    m_documentItem.data()->document()->bookmarkManager()->addBookmark(viewPort);

    if (!m_bookmarked) {
        m_bookmarked = true;
        emit bookmarkedChanged();
    }

    emit bookmarksChanged();
}

void PageItem::removeBookmarkAtPos(qreal x, qreal y)
{
    Okular::DocumentViewport viewPort(m_viewPort);
    viewPort.rePos.enabled = true;
    viewPort.rePos.normalizedX = x;
    viewPort.rePos.normalizedY = y;

    m_documentItem.data()->document()->bookmarkManager()->addBookmark(viewPort);

    if (m_bookmarked && m_documentItem.data()->document()->bookmarkManager()->bookmarks(m_viewPort.pageNumber).count() == 0) {
        m_bookmarked = false;
        emit bookmarkedChanged();
    }

    emit bookmarksChanged();
}

void PageItem::removeBookmark(const QString &bookmark)
{
    m_documentItem.data()->document()->bookmarkManager()->removeBookmark(bookmark);
    emit bookmarksChanged();
}

//Reimplemented
void PageItem::geometryChanged(const QRectF &newGeometry,
                               const QRectF &oldGeometry)
{
    if (newGeometry.size().isEmpty()) {
        return;
    }

    bool changed = false;
    if (newGeometry.size() != oldGeometry.size()) {
        changed = true;
        m_redrawTimer->start();
    }

    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (changed) {
        //Why aren't they automatically emuitted?
        emit widthChanged();
        emit heightChanged();
    }
}

QSGNode * PageItem::updatePaintNode(QSGNode* node, QQuickItem::UpdatePaintNodeData* /*data*/)
{
    if (!window() || m_buffer.isNull()) {
        delete node;
        return nullptr;
    }
    QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode *>(node);
    if (!n) {
        n = new QSGSimpleTextureNode();
        n->setOwnsTexture(true);
    }

    n->setTexture(window()->createTextureFromImage(m_buffer));
    n->setRect(boundingRect());
    return n;
}

void PageItem::paint()
{
    if (!m_documentItem || !m_page || !window() || width() <= 0 || height() < 0) {
        if (!m_buffer.isNull()) {
            m_buffer = QImage();
            update();
        }
        return;
    }

    Observer *observer = m_isThumbnail ? m_documentItem.data()->thumbnailObserver() : m_documentItem.data()->pageviewObserver();
    const int priority = m_isThumbnail ? THUMBNAILS_PRIO : PAGEVIEW_PRIO;

    qreal dpr = window()->devicePixelRatio();

    {
        auto request = new Okular::PixmapRequest(observer, m_viewPort.pageNumber, width() * dpr, height() * dpr, priority, Okular::PixmapRequest::NoFeature);
        request->setNormalizedRect(Okular::NormalizedRect(0,0,1,1));
        const Okular::Document::PixmapRequestFlag prf = m_isThumbnail ? Okular::Document::NoOption : Okular::Document::RemoveAllPrevious;
        m_documentItem.data()->document()->requestPixmaps({request}, prf);
    }
    const int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
    // Simply using the limits as described by textureSize will, at times, result in the page painter
    // attempting to write outside the data area, unsurprisingly resulting in a crash.


    const QRect limits(QPoint(0, 0), QSize(width()*dpr, height()*dpr));
    QPixmap pix(limits.size());
    pix.setDevicePixelRatio(dpr);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, m_smooth);
    PagePainter::paintPageOnPainter(&p, m_page, observer, flags, width(), height(), limits);
    p.end();

    m_buffer = pix.toImage();

    update();
}

//Protected slots
void PageItem::pageHasChanged(int page, int flags)
{
    if (m_viewPort.pageNumber == page) {
        if (flags == 32) {
            // skip bounding box updates
            //kDebug() << "32" << m_page->boundingBox();
        } else if (flags == Okular::DocumentObserver::Pixmap) {
            // if pixmaps have updated, just repaint .. don't bother updating pixmaps AGAIN
            update();
        } else {
            m_redrawTimer->start();
        }
    }
}

void PageItem::checkBookmarksChanged()
{
    if (!m_documentItem) {
        return;
    }

    bool newBookmarked = m_documentItem.data()->document()->bookmarkManager()->isBookmarked(m_viewPort.pageNumber);
    if (m_bookmarked != newBookmarked) {
        m_bookmarked = newBookmarked;
        emit bookmarkedChanged();
    }

    //TODO: check the page
    emit bookmarksChanged();
}

void PageItem::contentXChanged()
{
    if (!m_flickable || !m_flickable.data()->property("contentX").isValid()) {
        return;
    }

    m_viewPort.rePos.normalizedX = m_flickable.data()->property("contentX").toReal() / (width() - m_flickable.data()->width());
}

void PageItem::contentYChanged()
{
    if (!m_flickable || !m_flickable.data()->property("contentY").isValid()) {
        return;
    }

    m_viewPort.rePos.normalizedY = m_flickable.data()->property("contentY").toReal() / (height() - m_flickable.data()->height());
}

void PageItem::setIsThumbnail(bool thumbnail)
{
    if (thumbnail == m_isThumbnail) {
        return;
    }

    m_isThumbnail = thumbnail;

    if (thumbnail) {
        m_smooth = false;
    }

    /*
    m_redrawTimer->setInterval(thumbnail ? 0 : REDRAW_TIMEOUT);
    m_redrawTimer->setSingleShot(true);
    */
}
