/***************************************************************************
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "event_p.h"

#include "../form.h"

using namespace Okular;

class Event::Private
{
    public:
        Private( EventType eventType ): m_target( nullptr ),
                                        m_targetPage( nullptr ),
                                        m_source( nullptr ),
                                        m_sourcePage( nullptr ),
                                        m_eventType( eventType )
        {
        }

        void *m_target;
        Page *m_targetPage;
        FormField *m_source;
        Page *m_sourcePage;
        EventType m_eventType;
        QString m_targetName;
        QVariant m_value;
};

Event::Event(): d( new Private( UnknownEvent ) )
{
}

Event::Event( EventType eventType ): d( new Private( eventType ) )
{

}

Event::EventType Event::eventType() const
{
    return d->m_eventType;
}

QString Event::name() const
{
    switch ( d->m_eventType )
    {
        case ( FieldCalculate ):
            return QStringLiteral( "Calculate" );
        case ( UnknownEvent ):
        default:
            return QStringLiteral( "Unknown" );
    }
}

QString Event::type() const
{
    switch ( d->m_eventType )
    {
        case ( FieldCalculate ):
            return QStringLiteral( "Field" );
        case ( UnknownEvent ):
        default:
            return QStringLiteral( "Unknown" );
    }
}

QString Event::targetName() const
{
    if ( !d->m_targetName.isNull() )
    {
        return d->m_targetName;
    }

    return QStringLiteral( "JavaScript for: " ) + type() + name();
}

void Event::setTargetName( const QString &val )
{
    d->m_targetName = val;
}

FormField *Event::source() const
{
    return d->m_source;
}

void Event::setSource( FormField *val )
{
    d->m_source = val;
}

Page *Event::sourcePage() const
{
    return d->m_sourcePage;
}

void Event::setSourcePage( Page *val )
{
    d->m_sourcePage = val;
}

void *Event::target() const
{
    return d->m_target;
}

void Event::setTarget( void *target )
{
    d->m_target = target;
}

Page *Event::targetPage() const
{
    return d->m_targetPage;
}

void Event::setTargetPage( Page *val )
{
    d->m_targetPage = val;
}

QVariant Event::value() const
{
    return d->m_value;
}

void Event::setValue( const QVariant &val )
{
    d->m_value = val;
}

// static
std::shared_ptr<Event> Event::createFormCalculateEvent( FormField *target,
                                                        Page *targetPage,
                                                        FormField *source,
                                                        Page *sourcePage,
                                                        const QString &targetName )
{
    std::shared_ptr<Event> ret( new Event( Event::FieldCalculate ) );
    ret->setSource( source );
    ret->setSourcePage( sourcePage );
    ret->setTarget( target );
    ret->setTargetPage( targetPage );
    ret->setTargetName( targetName );

    FormFieldText *fft = dynamic_cast< FormFieldText * >(target);
    if ( fft )
    {
        ret->setValue( QVariant( fft->text() ) );
    }
    return ret;
}
