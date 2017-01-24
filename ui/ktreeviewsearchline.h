/*
   Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>
   Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
   Copyright (c) 2006 Hamish Rodda <rodda@kde.org>
   Copyright 2007 Pino Toscano <pino@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTREEVIEWSEARCHLINE_H
#define KTREEVIEWSEARCHLINE_H

#include <klineedit.h>

class QModelIndex;
class QTreeView;

/**
 * This class makes it easy to add a search line for filtering the items in
 * listviews based on a simple text search.
 *
 * No changes to the application other than instantiating this class with
 * appropriate QTreeViews should be needed.
 */

class KTreeViewSearchLine : public KLineEdit
{
    Q_OBJECT

    Q_PROPERTY( Qt::CaseSensitivity caseSensitity READ caseSensitivity WRITE setCaseSensitivity )


  public:
    /**
     * Constructs a KTreeViewSearchLine with \a treeView being the QTreeView to
     * be filtered.
     *
     * If \a treeView is null then the widget will be disabled until listview
     * are set with setTreeView().
     */
    explicit KTreeViewSearchLine( QWidget *parent = Q_NULLPTR, QTreeView *treeView = 0 );

    /**
     * Destroys the KTreeViewSearchLine.
     */
    virtual ~KTreeViewSearchLine();

    /**
     * Returns true if the search is case sensitive.  This defaults to false.
     *
     * @see setCaseSensitive()
     */
    Qt::CaseSensitivity caseSensitivity() const;

    /**
     * Returns true if the search is a regular expression search.  This defaults to false.
     *
     * @see setRegularExpression()
     */
    bool regularExpression() const;

    /**
     * Returns the listview that is currently filtered by the search.
     *
     * @see setTreeView()
     */
    QTreeView *treeView() const;

  public Q_SLOTS:
    /**
     * Updates search to only make visible the items that match \a pattern.  If
     * \a s is null then the line edit's text will be used.
     */
    virtual void updateSearch( const QString &pattern = QString() );

    /**
     * Make the search case sensitive or case insensitive.
     *
     * @see caseSenstivity()
     */
    void setCaseSensitivity( Qt::CaseSensitivity caseSensitivity );

    /**
     * Make the search a regular expression search or not.
     *
     * @see regularExpression()
     */
    void setRegularExpression( bool value );

    /**
     * Sets the QTreeView that is filtered by this search line, replacing any
     * previously filtered listviews.  If \a treeView is null then the widget will be
     * disabled.
     *
     * @see treeView()
     */
    void setTreeView( QTreeView *treeView );

  Q_SIGNALS:
    /**
     * This signal is emitted when search options have been changed. It is emitted so
     * that users of this class can choose to save the search options to the settings.
     */
    void searchOptionsChanged();

  protected:
    /**
     * Returns true if \a item matches the search \a pattern.  This will be evaluated
     * based on the value of caseSensitive().  This can be overridden in
     * subclasses to implement more complicated matching schemes.
     */
    virtual bool itemMatches( const QModelIndex &item, int row, const QString &pattern ) const;

    /**
    * Re-implemented for internal reasons.  API not affected.
    */
    virtual void contextMenuEvent( QContextMenuEvent* );

    /**
     * Updates search to only make visible appropriate items in \a treeView.  If
     * \a treeView is null then nothing is done.
     */
    virtual void updateSearch( QTreeView *treeView );

    /**
     * Connects signals of this listview to the appropriate slots of the search
     * line.
     */
    virtual void connectTreeView( QTreeView* );

    /**
     * Disconnects signals of a listviews from the search line.
     */
    virtual void disconnectTreeView( QTreeView* );

  protected Q_SLOTS:
    /**
     * When keys are pressed a new search string is created and a timer is
     * activated.  The most recent search is activated when this timer runs out
     * if another key has not yet been pressed.
     *
     * This method makes @param search the most recent search and starts the
     * timer.
     *
     * Together with activateSearch() this makes it such that searches are not
     * started until there is a short break in the users typing.
     *
     * @see activateSearch()
     */
    void queueSearch( const QString &search );

    /**
     * When the timer started with queueSearch() expires this slot is called.
     * If there has been another timer started then this slot does nothing.
     * However if there are no other pending searches this starts the list view
     * search.
     *
     * @see queueSearch()
     */
    void activateSearch();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void rowsInserted( const QModelIndex&, int, int ) const )
    Q_PRIVATE_SLOT( d, void treeViewDeleted( QObject* ) )
    Q_PRIVATE_SLOT( d, void slotCaseSensitive() )
    Q_PRIVATE_SLOT( d, void slotRegularExpression() )
};

/**
 * Creates a widget featuring a KTreeViewSearchLine, a label with the text
 * "Search" and a button to clear the search.
 */
class KTreeViewSearchLineWidget : public QWidget
{
    Q_OBJECT

  public:
    /**
     * Creates a KTreeViewSearchLineWidget for \a treeView with \a parent as the
     * parent.
     */
    explicit KTreeViewSearchLineWidget( QWidget *parent = Q_NULLPTR, QTreeView *treeView = 0 );

    /**
     * Destroys the KTreeViewSearchLineWidget
     */
    ~KTreeViewSearchLineWidget();

    /**
     * Returns a pointer to the search line.
     */
    KTreeViewSearchLine *searchLine() const;

  protected Q_SLOTS:
    /**
     * Creates the widgets inside of the widget.  This is called from the
     * constructor via a single shot timer so that it it guaranteed to run
     * after construction is complete.  This makes it suitable for overriding in
     * subclasses.
     */
    virtual void createWidgets();

  protected:
    /**
     * Creates the search line.  This can be useful to reimplement in cases where
     * a KTreeViewSearchLine subclass is used.
     *
     * It is const because it is be called from searchLine(), which to the user
     * doesn't conceptually alter the widget.
     */
    virtual KTreeViewSearchLine *createSearchLine( QTreeView *treeView ) const;

  private:
    class Private;
    Private* const d;
};

#endif
