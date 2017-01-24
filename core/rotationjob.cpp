/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "rotationjob_p.h"

#include <QtGui/QTransform>

using namespace Okular;

RotationJob::RotationJob( const QImage &image, Rotation oldRotation, Rotation newRotation, DocumentObserver *observer )
    : ThreadWeaver::QObjectDecorator( new RotationJobInternal( image, oldRotation, newRotation ) )
    , mObserver( observer ), m_pd( 0 )
    , mRect( NormalizedRect() )
{
}

void RotationJob::setPage( PagePrivate * pd )
{
    m_pd = pd;
}

void RotationJob::setRect( const NormalizedRect &rect )
{
    mRect = rect;
}

DocumentObserver * RotationJob::observer() const
{
    return mObserver;
}

PagePrivate * RotationJob::page() const
{
    return m_pd;
}

NormalizedRect RotationJob::rect() const
{
    return mRect;
}

QTransform RotationJob::rotationMatrix( Rotation from, Rotation to )
{
    QTransform matrix;

    if ( from == Rotation0 ) {
        if ( to == Rotation90 )
            matrix.rotate( 90 );
        else if ( to == Rotation180 )
            matrix.rotate( 180 );
        else if ( to == Rotation270 )
            matrix.rotate( 270 );
    } else if ( from == Rotation90 ) {
        if ( to == Rotation180 )
            matrix.rotate( 90 );
        else if ( to == Rotation270 )
            matrix.rotate( 180 );
        else if ( to == Rotation0 )
            matrix.rotate( 270 );
    } else if ( from == Rotation180 ) {
        if ( to == Rotation270 )
            matrix.rotate( 90 );
        else if ( to == Rotation0 )
            matrix.rotate( 180 );
        else if ( to == Rotation90 )
            matrix.rotate( 270 );
    } else if ( from == Rotation270 ) {
        if ( to == Rotation0 )
            matrix.rotate( 90 );
        else if ( to == Rotation90 )
            matrix.rotate( 180 );
        else if ( to == Rotation180 )
            matrix.rotate( 270 );
    }

    return matrix;
}

RotationJobInternal::RotationJobInternal( const QImage &image, Rotation oldRotation, Rotation newRotation )
    : mImage( image ), mOldRotation( oldRotation ), mNewRotation( newRotation )
{
}

QImage RotationJobInternal::image() const
{
    return mRotatedImage;
}

Rotation RotationJobInternal::rotation() const
{
    return mNewRotation;
}

void RotationJobInternal::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);

    if ( mOldRotation == mNewRotation ) {
        mRotatedImage = mImage;
        return;
    }

    const QTransform matrix = RotationJob::rotationMatrix( mOldRotation, mNewRotation );

    mRotatedImage = mImage.transformed( matrix );
}

#include "moc_rotationjob_p.cpp"
