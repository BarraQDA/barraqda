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

#include "ktreeviewsearchline.h"

#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtCore/QRegExp>
#include <QtWidgets/QApplication>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>

#include <QtCore/QDebug>
#include <kiconloader.h>
#include <KLocalizedString>
#include <ktoolbar.h>

class KTreeViewSearchLine::Private
{
  public:
    Private( KTreeViewSearchLine *_parent )
      : parent( _parent ),
        treeView( nullptr ),
        caseSensitive( Qt::CaseInsensitive ),
        regularExpression( false ),
        activeSearch( false ),
        queuedSearches( 0 )
    {
    }

    KTreeViewSearchLine *parent;
    QTreeView * treeView;
    Qt::CaseSensitivity caseSensitive;
    bool regularExpression;
    bool activeSearch;
    QString search;
    int queuedSearches;

    void rowsInserted(const QModelIndex & parent, int start, int end) const;
    void treeViewDeleted( QObject *treeView );
    void slotCaseSensitive();
    void slotRegularExpression();

    void checkItemParentsNotVisible(QTreeView *treeView);
    bool checkItemParentsVisible(QTreeView *treeView, const QModelIndex &index);
};

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void KTreeViewSearchLine::Private::rowsInserted( const QModelIndex & parentIndex, int start, int end ) const
{
  QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>( parent->sender() );
  if ( !model )
    return;

  QTreeView* widget = nullptr;
  if ( treeView->model() == model ) {
    widget = treeView;
  }

  if ( !widget )
    return;

  for ( int i = start; i <= end; ++i ) {
     widget->setRowHidden( i, parentIndex, !parent->itemMatches( parentIndex, i, parent->text() ) );
  }
}

void KTreeViewSearchLine::Private::treeViewDeleted( QObject *object )
{
  if ( object == treeView ) {
    treeView = nullptr;
    parent->setEnabled( false );
  }
}

void KTreeViewSearchLine::Private::slotCaseSensitive()
{
  if ( caseSensitive == Qt::CaseSensitive)
    parent->setCaseSensitivity( Qt::CaseInsensitive );
  else
    parent->setCaseSensitivity( Qt::CaseSensitive );

  parent->updateSearch();
}

void KTreeViewSearchLine::Private::slotRegularExpression()
{
  if ( regularExpression )
    parent->setRegularExpression( false );
  else
    parent->setRegularExpression( true );

  parent->updateSearch();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

/** Check whether \p item, its siblings and their descendents should be shown. Show or hide the items as necessary.
 *
 *  \p item  The list view item to start showing / hiding items at. Typically, this is the first child of another item, or the
 *              the first child of the list view.
 *  \return \c true if an item which should be visible is found, \c false if all items found should be hidden. If this function
 *             returns true and \p highestHiddenParent was not 0, highestHiddenParent will have been shown.
 */
bool KTreeViewSearchLine::Private::checkItemParentsVisible( QTreeView *treeView, const QModelIndex &index )
{
  bool childMatch = false;
  const int rowcount = treeView->model()->rowCount( index );
  for ( int i = 0; i < rowcount; ++i )
    childMatch |= checkItemParentsVisible( treeView, treeView->model()->index( i, 0, index ) );

  // Should this item be shown? It should if any children should be, or if it matches.
  const QModelIndex parentindex = index.parent();
  if ( childMatch || parent->itemMatches( parentindex, index.row(), search ) ) {
    treeView->setRowHidden( index.row(), parentindex, false );
    return true;
  }

  treeView->setRowHidden( index.row(), parentindex, true );

  return false;
}


////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

KTreeViewSearchLine::KTreeViewSearchLine( QWidget *parent, QTreeView *treeView )
  : KLineEdit( parent ), d( new Private( this ) )
{
  connect(this, &KTreeViewSearchLine::textChanged, this, &KTreeViewSearchLine::queueSearch);

  setClearButtonShown( true );
  setTreeView( treeView );

  if ( !treeView ) {
      setEnabled( false );
  }
}

KTreeViewSearchLine::~KTreeViewSearchLine()
{
  delete d;
}

Qt::CaseSensitivity KTreeViewSearchLine::caseSensitivity() const
{
  return d->caseSensitive;
}

bool KTreeViewSearchLine::regularExpression() const
{
  return d->regularExpression;
}

QTreeView *KTreeViewSearchLine::treeView() const
{
  return d->treeView;
}


////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void KTreeViewSearchLine::updateSearch( const QString &pattern )
{
  d->search = pattern.isNull() ? text() : pattern;

  updateSearch( d->treeView );
}

void KTreeViewSearchLine::updateSearch( QTreeView *treeView )
{
  if ( !treeView || !treeView->model()->rowCount() )
    return;


  // If there's a selected item that is visible, make sure that it's visible
  // when the search changes too (assuming that it still matches).

  QModelIndex currentIndex = treeView->currentIndex();

  bool wasUpdateEnabled = treeView->updatesEnabled();
  treeView->setUpdatesEnabled( false );
  for ( int i = 0; i < treeView->model()->rowCount(); ++i )
    d->checkItemParentsVisible( treeView, treeView->rootIndex() );
  treeView->setUpdatesEnabled( wasUpdateEnabled );

  if ( currentIndex.isValid() )
    treeView->scrollTo( currentIndex );
}

void KTreeViewSearchLine::setCaseSensitivity( Qt::CaseSensitivity caseSensitive )
{
  if ( d->caseSensitive != caseSensitive ) {
    d->caseSensitive = caseSensitive;
    updateSearch();
    emit searchOptionsChanged();
  }
}

void KTreeViewSearchLine::setRegularExpression( bool value )
{
  if ( d->regularExpression != value ) {
    d->regularExpression = value;
    updateSearch();
    emit searchOptionsChanged();
  }
}

void KTreeViewSearchLine::setTreeView( QTreeView *treeView )
{
  disconnectTreeView( d->treeView );
  d->treeView = treeView;
  connectTreeView( treeView );

  setEnabled( treeView != nullptr );
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

bool KTreeViewSearchLine::itemMatches( const QModelIndex &parentIndex, int row, const QString &pattern ) const
{
  if ( pattern.isEmpty() )
    return true;

  if ( !parentIndex.isValid() && parentIndex != d->treeView->rootIndex() )
    return false;

  // Contruct a regular expression object with the right options.
  QRegExp expression = QRegExp( pattern,
      d->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive,
      d->regularExpression ? QRegExp::RegExp : QRegExp::FixedString );

  // If the search column list is populated, search just the columns
  // specifified.  If it is empty default to searching all of the columns.

  QAbstractItemModel *model = d->treeView->model();
  const int columncount = model->columnCount( parentIndex );
  for ( int i = 0; i < columncount; ++i) {
    if ( expression.indexIn( model->data( model->index( row, i, parentIndex ), Qt::DisplayRole ).toString() ) >= 0 )
      return true;
  }

  return false;
}

void KTreeViewSearchLine::contextMenuEvent( QContextMenuEvent *event )
{
  QMenu *popup = KLineEdit::createStandardContextMenu();

  popup->addSeparator();
  QMenu *optionsSubMenu = popup->addMenu( i18n("Search Options") );
  QAction* caseSensitiveAction = optionsSubMenu->addAction( i18nc("Enable case sensitive search in the side navigation panels", "Case Sensitive"), this, SLOT(slotCaseSensitive()) );
  caseSensitiveAction->setCheckable( true );
  caseSensitiveAction->setChecked( d->caseSensitive );
  QAction* regularExpressionAction = optionsSubMenu->addAction( i18nc("Enable regular expression search in the side navigation panels", "Regular Expression"), this, SLOT(slotRegularExpression()) );
  regularExpressionAction->setCheckable( true );
  regularExpressionAction->setChecked( d->regularExpression );

  popup->exec( event->globalPos() );
  delete popup;
}

void KTreeViewSearchLine::connectTreeView( QTreeView *treeView )
{
  if ( treeView ) {
    connect( treeView, SIGNAL(destroyed(QObject*)),
             this, SLOT(treeViewDeleted(QObject*)) );

    connect( treeView->model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
             this, SLOT(rowsInserted(QModelIndex,int,int)) );
  }
}

void KTreeViewSearchLine::disconnectTreeView( QTreeView *treeView )
{
  if ( treeView ) {
    disconnect( treeView, SIGNAL(destroyed(QObject*)),
                this, SLOT(treeViewDeleted(QObject*)) );

    disconnect( treeView->model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(rowsInserted(QModelIndex,int,int)) );
  }
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void KTreeViewSearchLine::queueSearch( const QString &search )
{
  d->queuedSearches++;
  d->search = search;

  QTimer::singleShot( 200, this, &KTreeViewSearchLine::activateSearch );
}

void KTreeViewSearchLine::activateSearch()
{
  --(d->queuedSearches);

  if ( d->queuedSearches == 0 )
    updateSearch( d->search );
}

////////////////////////////////////////////////////////////////////////////////
// KTreeViewSearchLineWidget
////////////////////////////////////////////////////////////////////////////////

class KTreeViewSearchLineWidget::Private
{
  public:
    Private()
      : treeView( nullptr ),
        searchLine( nullptr )
    {
    }

    QTreeView *treeView;
    KTreeViewSearchLine *searchLine;
};

KTreeViewSearchLineWidget::KTreeViewSearchLineWidget( QWidget *parent, QTreeView *treeView )
  : QWidget( parent ), d( new Private )
{
  d->treeView = treeView;

  QTimer::singleShot( 0, this, &KTreeViewSearchLineWidget::createWidgets );
}

KTreeViewSearchLineWidget::~KTreeViewSearchLineWidget()
{
  delete d;
}

KTreeViewSearchLine *KTreeViewSearchLineWidget::createSearchLine( QTreeView *treeView ) const
{
  return new KTreeViewSearchLine( const_cast<KTreeViewSearchLineWidget*>(this), treeView );
}

void KTreeViewSearchLineWidget::createWidgets()
{
  QLabel *label = new QLabel( i18n("S&earch:"), this );
  label->setObjectName( QStringLiteral("kde toolbar widget") );

  searchLine()->show();

  label->setBuddy( d->searchLine );
  label->show();

  QHBoxLayout* layout = new QHBoxLayout( this );
  layout->setSpacing( 5 );
  layout->setMargin( 0 );
  layout->addWidget( label );
  layout->addWidget( d->searchLine );
}

KTreeViewSearchLine *KTreeViewSearchLineWidget::searchLine() const
{
  if ( !d->searchLine )
    d->searchLine = createSearchLine( d->treeView );

  return d->searchLine;
}

#include "moc_ktreeviewsearchline.cpp"
