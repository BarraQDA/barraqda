/***************************************************************************
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <QMap>
#include <QMimeType>
#include <QMimeDatabase>
#include "../settings_core.h"
#include "core/document.h"
#include <core/page.h>
#include <core/form.h>

#include "../generators/poppler/config-okular-poppler.h"

class VisibilityTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testJavaScriptVisibility();
    void testSaveLoad();
    void testActionVisibility();

private:
    void verifyTargetStates( bool visible );

    Okular::Document *m_document;
    QMap<QString, Okular::FormField*> m_fields;
};

void VisibilityTest::initTestCase()
{
    Okular::SettingsCore::instance( QStringLiteral("visibilitytest") );
    m_document = new Okular::Document( nullptr );

    const QString testFile = QStringLiteral( KDESRCDIR "data/visibilitytest.pdf" );
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( testFile );
    QCOMPARE( m_document->openDocument( testFile, QUrl(), mime), Okular::Document::OpenSuccess );

    // The test document has four buttons:
    // HideScriptButton -> Hides targets with JavaScript
    // ShowScriptButton -> Shows targets with JavaScript
    // HideActionButton -> Hides targets with HideAction
    // ShowActionButton -> Shows targets with HideAction
    //
    // The target fields are:
    // TargetButton TargetText TargetCheck TargetDropDown TargetRadio
    //
    // With two radio buttons named TargetRadio.

    const Okular::Page* page = m_document->page( 0 );
    for ( Okular::FormField *ff: page->formFields() )
    {
        m_fields.insert( ff->name(),  ff );
    }
}

void VisibilityTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

void VisibilityTest::verifyTargetStates( bool visible )
{
    QCOMPARE( m_fields[QStringLiteral( "TargetButton" )]->isVisible(), visible );
    QCOMPARE( m_fields[QStringLiteral( "TargetText" )]->isVisible(), visible );
    QCOMPARE( m_fields[QStringLiteral( "TargetCheck" )]->isVisible(), visible );
    QCOMPARE( m_fields[QStringLiteral( "TargetDropDown" )]->isVisible(), visible );

    // Radios do not properly inherit a name from the parent group so
    // this does not work yet (And would probably need some list handling).
    // QCOMPARE( m_fields[QStringLiteral( "TargetRadio" )].isVisible(), visible );
}

void VisibilityTest::testJavaScriptVisibility()
{
#ifndef HAVE_POPPLER_0_64
    return;
#endif
    auto hideBtn = m_fields[QStringLiteral( "HideScriptButton" )];
    auto showBtn = m_fields[QStringLiteral( "ShowScriptButton" )];

    // We start with all fields visible
    verifyTargetStates( true );

    m_document->processAction( hideBtn->activationAction() );

    // Now all should be hidden
    verifyTargetStates( false );

    // And show again
    m_document->processAction( showBtn->activationAction() );
    verifyTargetStates( true );
}

void VisibilityTest::testSaveLoad()
{
#ifndef HAVE_POPPLER_0_64
    return;
#endif
    auto hideBtn = m_fields[QStringLiteral( "HideScriptButton" )];
    auto showBtn = m_fields[QStringLiteral( "ShowScriptButton" )];

    // We start with all fields visible
    verifyTargetStates( true );

    m_document->processAction( hideBtn->activationAction() );

    // Now all should be hidden
    verifyTargetStates( false );

    // Save the changed states
    QTemporaryFile saveFile( QString( "%1/okrXXXXXX.pdf" ).arg( QDir::tempPath() ) );
    QVERIFY( saveFile.open() );
    saveFile.close();

    QVERIFY( m_document->saveChanges( saveFile.fileName() ) );

    auto newDoc = new Okular::Document( nullptr );

    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( saveFile.fileName() );
    QCOMPARE( newDoc->openDocument( saveFile.fileName(), QUrl(), mime), Okular::Document::OpenSuccess );

    const Okular::Page* page = newDoc->page( 0 );

    bool anyChecked = false; // Saveguard against accidental test passing here ;-)
    for ( Okular::FormField *ff: page->formFields() )
    {
        if ( ff->name().startsWith( QStringLiteral( "Target" ) ) )
        {
            QCOMPARE( ff->isVisible(), false );
            anyChecked = true;
        }
    }
    QVERIFY(anyChecked);

    newDoc->closeDocument();
    delete newDoc;

    // Restore the state of the member document
    m_document->processAction( showBtn->activationAction() );
}

void VisibilityTest::testActionVisibility()
{
#ifndef HAVE_POPPLER_0_64
    return;
#endif
    auto hideBtn = m_fields[QStringLiteral( "HideActionButton" )];
    auto showBtn = m_fields[QStringLiteral( "ShowActionButton" )];

    verifyTargetStates( true );

    m_document->processAction( hideBtn->activationAction() );

    verifyTargetStates( false );

    m_document->processAction( showBtn->activationAction() );

    verifyTargetStates( true );
}

QTEST_MAIN( VisibilityTest )
#include "visibilitytest.moc"
