/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TOCMODEL_H
#define TOCMODEL_H

#include <qabstractitemmodel.h>
#include <QVector>

namespace Okular {
class Document;
class DocumentSynopsis;
class DocumentViewport;
}

class TOCModelPrivate;

class TOCModel : public QAbstractItemModel
{
    Q_OBJECT
    /**
     * How many items are in this model, useful for QML
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    public:
        explicit TOCModel( Okular::Document *document, QObject *parent = Q_NULLPTR );
        virtual ~TOCModel();

        // reimplementations from QAbstractItemModel
        QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
        int columnCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
        bool hasChildren( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
        QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;
        int rowCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

        void fill( const Okular::DocumentSynopsis *toc );
        void clear();
        void setCurrentViewport( const Okular::DocumentViewport &viewport );

        bool isEmpty() const;
        bool equals( const TOCModel *model ) const;
        void setOldModelData( TOCModel *model, const QVector<QModelIndex> &list );
        bool hasOldModelData() const;
        TOCModel *clearOldModelData() const;

        QString externalFileNameForIndex( const QModelIndex &index ) const;
        Okular::DocumentViewport viewportForIndex( const QModelIndex &index ) const;
        QString urlForIndex( const QModelIndex &index ) const;

        int count() const
        {
            return rowCount();
        }

    Q_SIGNALS:
        void countChanged();

    private:
        // storage
        friend class TOCModelPrivate;
        TOCModelPrivate *const d;
        bool checkequality( const TOCModel *model, const QModelIndex &parentA = QModelIndex(), const QModelIndex &parentB = QModelIndex() ) const;
};

#endif
