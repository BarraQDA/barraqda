/***************************************************************************
 *   Copyright (C) 2014 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <qprintdialog.h>
#include <qwidget.h>
#include <qtabwidget.h>
#include <QStandardPaths>
#include <kconfiggroup.h>

#include "../shell/okular_main.h"
#include "../shell/shell.h"
#include "../shell/shellutils.h"
#include "../core/document_p.h"
#include "../ui/presentationwidget.h"
#include "../part.h"
#include "../settings.h"

#include <sys/types.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#else
#include <process.h>
#endif

namespace Okular {
class PartTest
{
public:
    Okular::Document *partDocument(Okular::Part *part) const {
        return part->m_document;
    }
    QWidget *presentationWidget(Okular::Part *part) const {
        return part->m_presentationWidget;
    }
};
}

class ClosePrintDialogHelper : public QObject
{
    Q_OBJECT

public:
    ClosePrintDialogHelper(int expectedTab) : foundDialog(false), m_expectedTab(expectedTab) { }
    bool foundDialog;

private slots:
    void closePrintDialog();

private:
    int m_expectedTab;
};

class MainShellTest : public QObject, public Okular::PartTest
{
    Q_OBJECT

public:
    static QTabWidget* tabWidget(Shell *s)
    {
        return s->m_tabWidget;
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testShell_data();
    void testShell();
    void testFileRemembersPagePosition_data();
    void testFileRemembersPagePosition();
    void test2FilesError_data();
    void test2FilesError();
    void testSessionRestore_data();
    void testSessionRestore();

private:
};

QList<Shell*> getShells()
{
    QList<Shell*> shells;
    foreach( KMainWindow* kmw, KMainWindow::memberList() )
    {
        Shell* shell = qobject_cast<Shell*>( kmw );
        if( shell )
        {
            shells.append( shell );
        }
    }
    return shells;
}

Shell *findShell(Shell *ignore = nullptr)
{
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        Shell *s = qobject_cast<Shell*>(widget);
        if (s && s != ignore)
            return s;
    }
    return nullptr;
}

void MainShellTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Don't pollute people's okular settings
    Okular::Settings::instance( QStringLiteral("mainshelltest") );

    // Register in bus as okular
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    const QString myPid = QString::number( getpid() );
    const QString serviceName = QStringLiteral("org.kde.okular-")+ myPid;
    QVERIFY( bus->registerService(serviceName) == QDBusConnectionInterface::ServiceRegistered );

    // Tell the presentationWidget and queryClose to not be annoying
    KSharedConfigPtr c = KSharedConfig::openConfig();
    KConfigGroup cg = c->group("Notification Messages");
    cg.writeEntry("presentationInfo", false);
    cg.writeEntry("ShowTabWarning", false);
}

void MainShellTest::cleanupTestCase()
{
}

void MainShellTest::init()
{
    // Default settings for every test
    Okular::Settings::self()->setDefaults();

    // Clean docdatas
    QList<QUrl> urls;
    urls << QUrl::fromUserInput(QStringLiteral("file://" KDESRCDIR "data/file1.pdf"));
    urls << QUrl::fromUserInput(QStringLiteral("file://" KDESRCDIR "data/tocreload.pdf"));
    urls << QUrl::fromUserInput(QStringLiteral("file://" KDESRCDIR "data/contents.epub"));

    foreach (const QUrl &url, urls)
    {
        QFileInfo fileReadTest( url.toLocalFile() );
        const QString docDataPath = Okular::DocumentPrivate::docDataFileName(url, fileReadTest.size());
        QFile::remove(docDataPath);
    }
}

void MainShellTest::cleanup()
{
    Shell *s;
    while ((s = findShell()))
    {
        delete s;
    }
}

void MainShellTest::testShell_data()
{
    QTest::addColumn<QStringList>("paths");
    QTest::addColumn<QString>("serializedOptions");
    QTest::addColumn<bool>("useTabs");
    QTest::addColumn<QString>("externalProcessPath");
    QTest::addColumn<uint>("expectedPage");
    QTest::addColumn<bool>("expectPresentation");
    QTest::addColumn<bool>("expectPrintDialog");
    QTest::addColumn<bool>("unique");
    QTest::addColumn<uint>("externalProcessExpectedPage");
    QTest::addColumn<bool>("externalProcessExpectPresentation");
    QTest::addColumn<bool>("externalProcessExpectPrintDialog");

    const QStringList contentsEpub = QStringList(QStringLiteral(KDESRCDIR "data/contents.epub"));
    const QStringList file1 = QStringList(QStringLiteral(KDESRCDIR "data/file1.pdf"));
    QStringList file1AndToc;
    file1AndToc << QStringLiteral(KDESRCDIR "data/file1.pdf");
    file1AndToc << QStringLiteral(KDESRCDIR "data/tocreload.pdf");
    const QString tocReload = QStringLiteral(KDESRCDIR "data/tocreload.pdf");

    const QString optionsPage2 = ShellUtils::serializeOptions(false, false, false, false, QStringLiteral("2"));
    const QString optionsPage2Presentation = ShellUtils::serializeOptions(true, false, false, false, QStringLiteral("2"));
    const QString optionsPrint = ShellUtils::serializeOptions(false, true, false, false, QString());
    const QString optionsUnique = ShellUtils::serializeOptions(false, false, true, false, QString());

    QTest::newRow("just show shell") << QStringList() << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("open file") << file1 << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files no tabs") << file1AndToc << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files with tabs") << file1AndToc << QString() << true << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files sequence no tabs") << file1 << QString() << false << tocReload << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files sequence with tabs") << file1 << QString() << true << tocReload << 0u << false << false << false << 0u << false << false;
    QTest::newRow("open file page number") << contentsEpub << optionsPage2 << false << QString() << 1u << false << false << false << 0u << false << false;
    QTest::newRow("open file page number and presentation") << contentsEpub << optionsPage2Presentation << false << QString() << 1u << true << false << false << 0u << false << false;
    QTest::newRow("open file print") << file1 << optionsPrint << false << QString() << 0u << false << true << false << 0u << false << false;
    QTest::newRow("open two files unique") << file1 << optionsUnique << false << tocReload << 0u << false << false << true << 0u << false << false;
    QTest::newRow("open two files unique tabs") << file1 << optionsUnique << true << tocReload << 0u << false << false << true << 0u << false << false;
    QTest::newRow("page number attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << false << false << 2u << false << false;
    QTest::newRow("presentation attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << false << false << 2u << true << false;
    QTest::newRow("print attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << true << false << 2u << false << true;
    QTest::newRow("page number attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 3u << false << false;
    QTest::newRow("presentation attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 2u << true << false;
    QTest::newRow("print attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 2u << false << true;
    QTest::newRow("page number attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 3u << false << false;
    QTest::newRow("presentation attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 2u << true << false;
    QTest::newRow("print attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 2u << false << true;
}

void MainShellTest::testShell()
{
    QFETCH(QStringList, paths);
    QFETCH(QString, serializedOptions);
    QFETCH(bool, useTabs);
    QFETCH(QString, externalProcessPath);
    QFETCH(uint, expectedPage);
    QFETCH(bool, expectPresentation);
    QFETCH(bool, expectPrintDialog);
    QFETCH(bool, unique);
    QFETCH(uint, externalProcessExpectedPage);
    QFETCH(bool, externalProcessExpectPresentation);
    QFETCH(bool, externalProcessExpectPrintDialog);

    QScopedPointer<ClosePrintDialogHelper> helper;

    Okular::Settings::self()->setShellOpenFileInTabs(useTabs);

    if (expectPrintDialog || externalProcessExpectPrintDialog) {
        const int expectedTab = externalProcessExpectPrintDialog && !unique ? 1 : 0;
        helper.reset(new ClosePrintDialogHelper(expectedTab));
        QTimer::singleShot(0, helper.data(), SLOT(closePrintDialog()));
    }

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);

    if (paths.count() == 1)
    {
        QCOMPARE(s->m_tabs.count(), 1);
        Okular::Part *part = s->findChild<Okular::Part*>();
        QVERIFY(part);
        QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
        QCOMPARE(partDocument(part)->currentPage(), expectedPage);
    }
    else if (paths.count() == 2)
    {
        if (useTabs)
        {
            QSet<QString> openUrls;
            Shell *s = findShell();
            QVERIFY(s);
            Okular::Part *part = dynamic_cast<Okular::Part*>(s->m_tabs[0].part);
            Okular::Part *part2 = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
            QCOMPARE(s->m_tabs.count(), 2);
            QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
            QCOMPARE(part2->url().url(), QStringLiteral("file://%1").arg(paths[1]));
            QCOMPARE(partDocument(part)->currentPage(), expectedPage);
            QCOMPARE(partDocument(part2)->currentPage(), expectedPage);
        }
        else
        {
            QSet<QString> openUrls;
            Shell *s = findShell();
            QVERIFY(s);
            QCOMPARE(s->m_tabs.count(), 1);
            Okular::Part *part = s->findChild<Okular::Part*>();
            QVERIFY(part);
            QCOMPARE(partDocument(part)->currentPage(), expectedPage);
            openUrls << part->url().url();

            Shell *s2 = findShell(s);
            QVERIFY(s2);
            QCOMPARE(s2->m_tabs.count(), 1);
            Okular::Part *part2 = s2->findChild<Okular::Part*>();
            QVERIFY(part2);
            QCOMPARE(partDocument(part2)->currentPage(), expectedPage);
            openUrls << part2->url().url();

            foreach(const QString &path, paths)
            {
                QVERIFY(openUrls.contains(QStringLiteral("file://%1").arg(path)));
            }
        }
    }

    if (!externalProcessPath.isEmpty())
    {
        Okular::Part *part = s->findChild<Okular::Part*>();

        QProcess p;
        QStringList args;
        args << externalProcessPath;
        if (unique)
            args << QStringLiteral("-unique");
        if (externalProcessExpectedPage != 0)
            args << QStringLiteral("-page") << QString::number(externalProcessExpectedPage + 1);
        if (externalProcessExpectPresentation)
            args << QStringLiteral("-presentation");
        if (externalProcessExpectPrintDialog)
            args << QStringLiteral("-print");
        p.start(QStringLiteral(OKULAR_BINARY), args);
        p.waitForStarted();
        QCOMPARE(p.state(), QProcess::Running);

        if (useTabs || unique)
        {
            // It is attaching to us, so will eventually stop
            QTRY_COMPARE_WITH_TIMEOUT(p.state(), QProcess::NotRunning, 20000);
            QCOMPARE(p.exitStatus(), QProcess::NormalExit);
            QCOMPARE(p.exitCode(), 0);

            if (unique)
            {
                // It is unique so part got "overriten"
                QCOMPARE(s->m_tabs.count(), 1);
                QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(externalProcessPath));
                QCOMPARE(partDocument(part)->currentPage(), externalProcessExpectedPage);
            }
            else
            {
                // It is attaching to us so a second tab is there
                QCOMPARE(s->m_tabs.count(), 2);
                Okular::Part *part2 = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
                QCOMPARE(part2->url().url(), QStringLiteral("file://%1").arg(externalProcessPath));
                QCOMPARE(partDocument(part2)->currentPage(), externalProcessExpectedPage);
            }
        }
        else
        {
            QTest::qWait(750);

            // It opened on a new process, so it is still running, we need to kill it
            QCOMPARE(p.state(), QProcess::Running);
            p.terminate();
            p.waitForFinished();
            QVERIFY(p.state() != QProcess::Running);
            // It opened on a new process, so no change for us
            QCOMPARE(s->m_tabs.count(), 1);
            QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
            QCOMPARE(partDocument(part)->currentPage(), externalProcessExpectedPage);
        }
    }

    if (expectPresentation)
    {
        QCOMPARE(paths.count(), 1);
        Okular::Part *part = s->findChild<Okular::Part*>();
        QTRY_VERIFY(presentationWidget(part) != nullptr);
    }

    if (externalProcessExpectPresentation)
    {
        Okular::Part *part;
        if (unique)
        {
            QCOMPARE(s->m_tabs.count(), 1);
            part = dynamic_cast<Okular::Part*>(s->m_tabs[0].part);
        }
        else
        {
            QCOMPARE(s->m_tabs.count(), 2);
            part = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
        }

        QTRY_VERIFY(presentationWidget(part) != nullptr);
    }

    if (helper)
    {
        QVERIFY(helper->foundDialog);
    }
}

void ClosePrintDialogHelper::closePrintDialog()
{
    Shell *s = findShell();
    QPrintDialog *dialog = s->findChild<QPrintDialog*>();
    if (!dialog) {
        QTimer::singleShot(0, this, &ClosePrintDialogHelper::closePrintDialog);
        return;
    }
    QVERIFY(dialog);
    QCOMPARE(MainShellTest::tabWidget(s)->currentIndex(), m_expectedTab);
    dialog->close();
    foundDialog = true;
}

void MainShellTest::testFileRemembersPagePosition_data()
{
    QTest::addColumn<int>("mode");

    QTest::newRow("normal") << 1;
    QTest::newRow("unique") << 2;
    QTest::newRow("tabs") << 3;
}

void MainShellTest::testFileRemembersPagePosition()
{
    QFETCH(int, mode);

    const QStringList paths = QStringList(QStringLiteral(KDESRCDIR "data/contents.epub"));
    QString serializedOptions;
    if (mode == 1 || mode == 3)
        serializedOptions = ShellUtils::serializeOptions(false, false, false, false, QString());
    else
        serializedOptions = ShellUtils::serializeOptions(false, false, true, false, QString());

    Okular::Settings::self()->setShellOpenFileInTabs(mode == 3);

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);
    Okular::Part *part = s->findChild<Okular::Part*>();
    QVERIFY(part);
    QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
    QCOMPARE(partDocument(part)->currentPage(), 0u);
    partDocument(part)->setViewportPage(3);
    QCOMPARE(partDocument(part)->currentPage(), 3u);
    s->closeUrl();
    QCOMPARE(part->url().url(), QString());

    if (mode == 1)
    {
        delete s;
        status = Okular::main(paths, serializedOptions);
        QCOMPARE(status, Okular::Success);
    }
    else
    {
        QProcess p;
        QStringList args;
        args << paths[0];
        if (mode == 2)
            args << QStringLiteral("-unique");
        p.start(QStringLiteral(OKULAR_BINARY), args);
        p.waitForStarted();
        QCOMPARE(p.state(), QProcess::Running);

        // It is attaching to us, so will eventually stop
        QTRY_COMPARE_WITH_TIMEOUT((int)p.state(), (int)QProcess::NotRunning, 20000);
        QCOMPARE((int)p.exitStatus(), (int)QProcess::NormalExit);
        QCOMPARE(p.exitCode(), 0);
    }
    s = findShell();
    QVERIFY(s);
    part = s->findChild<Okular::Part*>();
    QVERIFY(part);
    QCOMPARE(part->url().url(), QStringLiteral("file://%1").arg(paths[0]));
    QCOMPARE(partDocument(part)->currentPage(), 3u);
}

void MainShellTest::test2FilesError_data()
{
    QTest::addColumn<QString>("serializedOptions");

    QTest::newRow("startInPresentation") << ShellUtils::serializeOptions(true, false, false, false, QString());
    QTest::newRow("showPrintDialog") << ShellUtils::serializeOptions(false, true, false, false, QString());
    QTest::newRow("unique") << ShellUtils::serializeOptions(false, false, true, false, QString());
    QTest::newRow("pageNumger") << ShellUtils::serializeOptions(false, false, false, false, QStringLiteral("3"));
}

void MainShellTest::test2FilesError()
{
    QFETCH(QString, serializedOptions);

    QStringList paths;
    paths << QStringLiteral(KDESRCDIR "data/file1.pdf") << QStringLiteral(KDESRCDIR "data/tocreload.pdf");
    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Error);

    QSet<QString> openUrls;
    Shell *s = findShell();
    QVERIFY(!s);
}

void MainShellTest::testSessionRestore_data()
{
    QTest::addColumn<QStringList>("paths");
    QTest::addColumn<QString>("options");
    QTest::addColumn<bool>("useTabsOpen");
    QTest::addColumn<bool>("useTabsRestore");

    QStringList oneDocPaths(QStringLiteral( KDESRCDIR "data/file1.pdf" ) );
    QStringList twoDocPaths( oneDocPaths );
    twoDocPaths << QStringLiteral(KDESRCDIR "data/formSamples.pdf");

    const QString options = ShellUtils::serializeOptions(false, false, false, false, QString());

    QTest::newRow("1 doc, 1 window, tabs")      << oneDocPaths << options << true  << true;
    QTest::newRow("2 docs, 1 window, tabs")     << twoDocPaths << options << true  << true;
    QTest::newRow("2 docs, 2 windows, tabs")    << twoDocPaths << options << false << true;
    QTest::newRow("2 docs, 2 windows, no tabs") << twoDocPaths << options << false << false;
    QTest::newRow("2 docs, 1 window, no tabs")  << twoDocPaths << options << true  << false;
}

void MainShellTest::testSessionRestore()
{
    QFETCH( QStringList, paths );
    QFETCH( QString, options );
    QFETCH( bool, useTabsOpen );
    QFETCH( bool, useTabsRestore );

    Okular::Settings::self()->setShellOpenFileInTabs( useTabsOpen );

    Okular::Status status = Okular::main( paths, options );
    QCOMPARE( status, Okular::Success );

    // Gather some information about the state
    // Verify that the correct number of windows/tabs were opened
    QList<Shell*> shells = getShells();
    QVERIFY( !shells.isEmpty() );
    int numDocs = 0;
    foreach( Shell* shell, shells )
    {
        QTest::qWaitForWindowExposed( shell );
        numDocs += shell->m_tabs.size();
    }

    QCOMPARE( numDocs, paths.size() );
    QCOMPARE( shells.size(), useTabsOpen ? 1 : paths.size() );
    QTest::qWait( 100 );

    // Simulate session shutdown. The actual shutdown path comes through
    // QSessionManager XSMP handlers, then KApplication::commitData/saveState,
    // then KMWSessionManager::commitData/saveState. Without simulating an X
    // session manager, the best we can do here is to make a temporary Config
    // and call KMainWindows save functions directly.
    QTemporaryFile configFile;
    QVERIFY( configFile.open() );

    int numWindows = 0;
    {   // Scope for config so that we can reconstruct from file
        KConfig config( configFile.fileName(), KConfig::SimpleConfig );
        foreach( Shell* shell, shells )
        {
            shell->savePropertiesInternal( &config, ++numWindows );
            // Windows aren't necessarily closed on shutdown, but we'll use
            // this as a way to trigger the destructor code, which is normally
            // connected to the aboutToQuit signal
            shell->close();
        }
    }

    // Wait for shells to delete themselves. QTest::qWait doesn't do deferred
    // deletions so we'll set up a full event loop to do that.
    QEventLoop eventLoop;
    QTimer::singleShot( 100, &eventLoop, &QEventLoop::quit );
    eventLoop.exec( QEventLoop::AllEvents );
    shells = getShells();
    QVERIFY( shells.isEmpty() );

    Okular::Settings::self()->setShellOpenFileInTabs( useTabsRestore );

    // Simulate session restore. We can't call KMainWindow::restore() directly
    // because it asks for info from the session manager, which doesn't know
    // about our temporary config. But the logic here mostly mirrors restore().
    KConfig config( configFile.fileName(), KConfig::SimpleConfig );
    for( int i = 1; i <= numWindows; ++i )
    {
        Shell* shell = new Shell;
        shell->readPropertiesInternal( &config, i );
        shell->show();
    }

    // Verify that the restore state is reasonable
    shells = getShells();
    QVERIFY( !shells.isEmpty() );
    numDocs = 0;
    foreach( Shell* shell, shells )
    {
        QTest::qWaitForWindowExposed( shell );
        numDocs += shell->m_tabs.size();
    }

    QCOMPARE( numDocs, paths.size() );
    QCOMPARE( shells.size(), useTabsRestore ? numWindows : paths.size() );
}

QTEST_MAIN( MainShellTest )
#include "mainshelltest.moc"
