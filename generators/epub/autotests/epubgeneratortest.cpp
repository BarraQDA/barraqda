/***************************************************************************
 *   Copyright (C) 2017 by Gilbert Assaf <gassaf@gmx.de>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include "core/document.h"
#include "core/page.h"
#include "settings_core.h"
#include "core/textpage.h"


class EpubGeneratorTest
: public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();
        void testDocumentStructure();
        void testDocumentContent();
        void cleanupTestCase();

    private:
        Okular::Document *m_document;
};

void EpubGeneratorTest::initTestCase()
{
    Okular::SettingsCore::instance( QStringLiteral("EpubGeneratorTest") );
    m_document = new Okular::Document( 0 );
    const QString testFile = QStringLiteral(KDESRCDIR "autotests/data/test.epub");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( testFile );
    QCOMPARE( m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess );
}


void EpubGeneratorTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

void EpubGeneratorTest::testDocumentStructure()
{
    unsigned int expectedPageNr = 3;
    QCOMPARE( m_document->pages(), expectedPageNr);
    QCOMPARE( m_document->metaData(QLatin1String("DocumentTitle")).toString(), QStringLiteral("Okular Test") );

    const Okular::DocumentSynopsis *docSyn = m_document->documentSynopsis();
    QDomElement heading1 = docSyn->documentElement();
    QCOMPARE( heading1.tagName(), QStringLiteral("Lorem ipsum Section 1") );

    QDomElement heading2 = heading1.nextSiblingElement();
    QCOMPARE( heading2.tagName(), QStringLiteral("Lorem ipsum Section 2") );
}

void EpubGeneratorTest::testDocumentContent()
{
    const Okular::Page *page0 = m_document->page(0);
    QCOMPARE( page0->number(), 0);
    m_document->requestTextPage( page0->number() );
    QVERIFY( page0->hasTextPage() );
    QCOMPARE( page0->text().trimmed(), QStringLiteral("Lorem ipsum Section 1\n\u2029This is an example Text.\n\uFFFC") );

    const Okular::Page *page1 = m_document->page(1);
    QCOMPARE( page1->number(), 1);
    m_document->requestTextPage( page1->number() );
    QVERIFY( page1->hasTextPage() );
    QCOMPARE( page1->text().trimmed(), QStringLiteral("Lorem ipsum Section 2\n\u2029This is an example Text.") );

}

QTEST_MAIN( EpubGeneratorTest )
#include "epubgeneratortest.moc"

/* kate: replace-tabs on; tab-width 4; */

