/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include "../core/annotations.h"
#include "../core/form.h"
#include "../core/page.h"
#include "../part.h"
#include "../ui/toc.h"
#include "../ui/pageview.h"

#include "../generators/poppler/config-okular-poppler.h"

#include <KActionCollection>
#include <KConfigDialog>
#include <KParts/OpenUrlArguments>

#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTreeView>
#include <QUrl>
#include <QDesktopServices>
#include <QMenu>

class CloseDialogHelper : public QObject
{
    Q_OBJECT

public:
    CloseDialogHelper(Okular::Part *p, QDialogButtonBox::StandardButton b) : m_part(p), m_button(b), m_clicked(false)
    {
        QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
    }

    ~CloseDialogHelper()
    {
        QVERIFY(m_clicked);
    }

private slots:
    void closeDialog()
    {
        QDialog *dialog = m_part->widget()->findChild<QDialog*>();
        if (!dialog) {
            QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
            return;
        }
        QDialogButtonBox *buttonBox = dialog->findChild<QDialogButtonBox*>();
        buttonBox->button(m_button)->click();
        m_clicked = true;
    }

private:
    Okular::Part *m_part;
    QDialogButtonBox::StandardButton m_button;
    bool m_clicked;
};

namespace Okular
{
class PartTest
    : public QObject
{
    Q_OBJECT

        static bool openDocument(Okular::Part *part, const QString &filePath);

    signals:
        void urlHandler(const QUrl &url);

    private slots:
        void testReload();
        void testCanceledReload();
        void testTOCReload();
        void testForwardPDF();
        void testForwardPDF_data();
        void testGeneratorPreferences();
        void testSelectText();
        void testClickInternalLink();
        void testOpenUrlArguments();
        void test388288();
        void testSaveAs();
        void testSaveAs_data();
        void testSaveAsUndoStackAnnotations();
        void testSaveAsUndoStackAnnotations_data();
        void testSaveAsUndoStackForms();
        void testSaveAsUndoStackForms_data();
        void testMouseMoveOverLinkWhileInSelectionMode();
        void testClickUrlLinkWhileInSelectionMode();
        void testeTextSelectionOverAndAcrossLinks_data();
        void testeTextSelectionOverAndAcrossLinks();
        void testClickUrlLinkWhileLinkTextIsSelected();
        void testRClickWhileLinkTextIsSelected();
        void testRClickOverLinkWhileLinkTextIsSelected();
        void testRClickOnSelectionModeShoulShowFollowTheLinkMenu();
        void testClickAnywhereAfterSelectionShouldUnselect();
        void testeRectSelectionStartingOnLinks();
        void testCheckBoxReadOnly();
        void testCrashTextEditDestroy();
        void testAnnotWindow();
        void testAdditionalActionTriggers();

    private:
        void simulateMouseSelection(double startX, double startY, double endX, double endY, QWidget *target);
};

class PartThatHijacksQueryClose : public Okular::Part
{
    public:
        PartThatHijacksQueryClose(QWidget* parentWidget, QObject* parent,
                                  const QVariantList& args)
        : Okular::Part(parentWidget, parent, args),
          behavior(PassThru)
        {}

        enum Behavior { PassThru, ReturnTrue, ReturnFalse };

        void setQueryCloseBehavior(Behavior new_behavior)
        {
            behavior = new_behavior;
        }

        bool queryClose() override
        {
             if (behavior == PassThru)
                 return Okular::Part::queryClose();
             else // ReturnTrue or ReturnFalse
                 return (behavior == ReturnTrue);
        }
    private:
        Behavior behavior;
};


bool PartTest::openDocument(Okular::Part *part, const QString &filePath)
{
    part->openDocument( filePath );
    return part->m_document->isOpened();
}

// Test that Okular doesn't crash after a successful reload
void PartTest::testReload()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/file1.pdf")) );
    part.reload();
    qApp->processEvents();
}

// Test that Okular doesn't crash after a canceled reload
void PartTest::testCanceledReload()
{
    QVariantList dummyArgs;
    PartThatHijacksQueryClose part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/file1.pdf")) );

    // When queryClose() returns false, the reload operation is canceled (as if
    // the user had chosen Cancel in the "Save changes?" message box)
    part.setQueryCloseBehavior(PartThatHijacksQueryClose::ReturnFalse);

    part.reload();

    qApp->processEvents();
}

void PartTest::testTOCReload()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/tocreload.pdf")) );
    QCOMPARE(part.m_toc->expandedNodes().count(), 0);
    part.m_toc->m_treeView->expandAll();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
    part.reload();
    qApp->processEvents();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
}

void PartTest::testForwardPDF()
{
    QFETCH(QString, dir);

    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);

    // Create temp dir named like this: ${system temp dir}/${random string}/${dir}
    const QTemporaryDir tempDir;
    const QDir workDir(QDir(tempDir.path()).filePath(dir));
    workDir.mkpath(QStringLiteral("."));

    QFile f(QStringLiteral(KDESRCDIR "data/synctextest.tex"));
    const QString texDestination = workDir.path() + QStringLiteral("/synctextest.tex");
    QVERIFY(f.copy(texDestination));
    QProcess process;
    process.setWorkingDirectory(workDir.path());

    const QString pdflatexPath(QStandardPaths::findExecutable("pdflatex"));
    if (pdflatexPath.isEmpty()) {
        QFAIL("pdflatex executable not found, but needed for the test. Try installing the respective TeXLive packages.");
    }
    process.start(pdflatexPath, QStringList() << QStringLiteral("-synctex=1") << QStringLiteral("-interaction=nonstopmode") << texDestination);
    bool started = process.waitForStarted();
    if (!started) {
        qDebug() << "start error:" << process.error();
        qDebug() << "start stdout:" << process.readAllStandardOutput();
        qDebug() << "start stderr:" << process.readAllStandardError();
    }
    QVERIFY(started);

    process.waitForFinished();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << "exit error:" << process.error() << "status" << process.exitStatus() << "code" << process.exitCode();
        qDebug() << "exit stdout:" << process.readAllStandardOutput();
        qDebug() << "exit stderr:" << process.readAllStandardError();
    }

    const QString pdfResult = workDir.path() + QStringLiteral("/synctextest.pdf");

    QVERIFY(QFile::exists(pdfResult));

    QVERIFY( openDocument(&part, pdfResult) );
    part.m_document->setViewportPage(0);
    QCOMPARE(part.m_document->currentPage(), 0u);
    part.closeUrl();

    QUrl u(QUrl::fromLocalFile(pdfResult));
    u.setFragment(QStringLiteral("src:100") + texDestination);
    part.openUrl(u);
    QCOMPARE(part.m_document->currentPage(), 1u);
}

void PartTest::testForwardPDF_data()
{
    QTest::addColumn<QString>("dir");

    QTest::newRow("non-utf8") << QString::fromUtf8("synctextest");
    QTest::newRow("utf8")     << QString::fromUtf8("ßðđđŋßðđŋ");
}

void PartTest::testGeneratorPreferences()
{
    KConfigDialog * dialog;
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);

    // Test that we don't crash while opening the dialog
    dialog = part.slotGeneratorPreferences();
    qApp->processEvents();
    delete dialog; // closes the dialog and recursively destroys all widgets

    // Test that we don't crash while opening a new instance of the dialog
    // This catches attempts to reuse widgets that have been destroyed
    dialog = part.slotGeneratorPreferences();
    qApp->processEvents();
    delete dialog;
}

void PartTest::testSelectText()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/file2.pdf")));
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const int mouseY = height * 0.052;
    const int mouseStartX = width * 0.12;
    const int mouseEndX = width * 0.7;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("Hola que tal\n"));
}

void PartTest::testClickInternalLink()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/file2.pdf")));
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseNormal");

    QCOMPARE(part.m_document->currentPage(), 0u);
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.17, height * 0.05));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * 0.17, height * 0.05));
    QTRY_COMPARE(part.m_document->currentPage(), 1u);
}

// cursor switches to Hand when hovering over link in TextSelect mode.
void PartTest::testMouseMoveOverLinkWhileInSelectionMode()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // move mouse over link
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.250, height * 0.127));

    // check if mouse icon changed to proper icon
    QTRY_COMPARE(part.m_pageView->cursor().shape(), Qt::PointingHandCursor);
}

// clicking on hyperlink jumps to destination in TextSelect mode.
void PartTest::testClickUrlLinkWhileInSelectionMode()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // overwrite urlHandler for 'mailto' urls
    QDesktopServices::setUrlHandler("mailto", this, "urlHandler");
    QSignalSpy openUrlSignalSpy(this, SIGNAL(urlHandler(QUrl)));

    // click on url
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.250, height * 0.127));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * 0.250, height * 0.127));

    // expect that the urlHandler signal was called
    QTRY_COMPARE(openUrlSignalSpy.count(), 1);
    QList<QVariant> arguments = openUrlSignalSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<QUrl>(), QUrl("mailto:foo@foo.bar"));
}

void PartTest::testeTextSelectionOverAndAcrossLinks_data()
{
    QTest::addColumn<double>("mouseStartX");
    QTest::addColumn<double>("mouseEndX");
    QTest::addColumn<QString>("expectedResult");

    // can text-select "over and across" hyperlink.
    QTest::newRow("start selection before link") << 0.1564 << 0.2943 << QStringLiteral(" a link: foo@foo.b");
    // can text-select starting at text and ending selection in middle of hyperlink.
    QTest::newRow("start selection in the middle of the link") << 0.28 << 0.382 << QStringLiteral("o.bar\n");
    // text selection works when selecting left to right or right to left
    QTest::newRow("start selection after link") << 0.40 << 0.05 << QStringLiteral("This is a link: foo@foo.bar\n");
}

// can text-select "over and across" hyperlink.
void PartTest::testeTextSelectionOverAndAcrossLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.127;
    QFETCH(double, mouseStartX);
    QFETCH(double, mouseEndX);

    mouseStartX = width * mouseStartX;
    mouseEndX = width * mouseEndX;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    QFETCH(QString, expectedResult);
    QCOMPARE(QApplication::clipboard()->text(), expectedResult);
}

// can jump to link while there's an active selection of text.
void PartTest::testClickUrlLinkWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.127;
    const double mouseStartX = width * 0.13;
    const double mouseEndX = width * 0.40;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // overwrite urlHandler for 'mailto' urls
    QDesktopServices::setUrlHandler("mailto", this, "urlHandler");
    QSignalSpy openUrlSignalSpy(this, SIGNAL(urlHandler(QUrl)));

    // click on url
    const double mouseClickX = width * 0.2997;
    const double mouseClickY = height * 0.1293;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // expect that the urlHandler signal was called
    QTRY_COMPARE(openUrlSignalSpy.count(), 1);
    QList<QVariant> arguments = openUrlSignalSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<QUrl>(), QUrl("mailto:foo@foo.bar"));
}

// r-click on the selected text gives the "Go To:" content menu option
void PartTest::testRClickWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    bool menuClosed = false;
    QTimer::singleShot(2000, [view, &menuClosed]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains go-to link action
        QAction *goToAction = qobject_cast<QAction*>(menu->findChild<QAction*>("GoToAction"));
        QVERIFY(goToAction);

        // check if the "follow this link" action is not visible
        QAction *processLinkAction = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(!processLinkAction);

        // check if the "copy link address" action is not visible
        QAction *copyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(!copyLinkLocation);

        // close menu to continue test
        menu->close();
        menuClosed = true;
    });

    // click on url
    const double mouseClickX = width * 0.425;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // will continue after pop-menu get closed
    QTRY_VERIFY(menuClosed);
}


// r-click on the link gives the "follow this link" content menu option
void PartTest::testRClickOverLinkWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    bool menuClosed = false;
    QTimer::singleShot(2000, [view, &menuClosed]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains "follow this link" action
        QAction *processLinkAction = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(processLinkAction);

        // check if the menu contains "copy link address" action
        QAction *copyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(copyLinkLocation);

        // close menu to continue test
        menu->close();
        menuClosed = true;
    });

    // click on url
    const double mouseClickX = width * 0.593;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // will continue after pop-menu get closed
    QTRY_VERIFY(menuClosed);
}

void PartTest::testRClickOnSelectionModeShoulShowFollowTheLinkMenu()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    bool menuClosed = false;
    QTimer::singleShot(2000, [view, &menuClosed]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains "Follow this link" action
        QAction *processLink = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(processLink);

        // chek if the menu contains  "Copy Link Address" action
        QAction *actCopyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(actCopyLinkLocation);

        // close menu to continue test
        menu->close();
        menuClosed = true;
    });

    // r-click on url
    const double mouseClickX = width * 0.604;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // will continue after pop-menu get closed
    QTRY_VERIFY(menuClosed);
}

void PartTest::testClickAnywhereAfterSelectionShouldUnselect()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // click on url
    const double mouseClickX = width * 0.10;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(mouseClickX, mouseY), 1000);

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    // check if copied text is empty what means no text selected
    QVERIFY(QApplication::clipboard()->text().isEmpty());
}

void PartTest::testeRectSelectionStartingOnLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseSelect"));

    const double mouseStartY = height * 0.127;
    const double mouseEndY = height * 0.127;
    const double mouseStartX = width *  0.28;
    const double mouseEndX = width *  0.382;

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    bool menuClosed = false;
    QTimer::singleShot(2000, [view, &menuClosed]() {
        QApplication::clipboard()->clear();

        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the copy selected text to clipboard is present
        QAction *copyAct = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyTextToClipboard"));
        QVERIFY(copyAct);

        menu->close();
        menuClosed = true;
    });

    simulateMouseSelection(mouseStartX, mouseStartY, mouseEndX, mouseEndY, part.m_pageView->viewport());

    // wait menu get closed
    QTRY_VERIFY(menuClosed);
}


void PartTest::simulateMouseSelection(double startX, double startY, double endX, double endY, QWidget *target)
{
    const int steps = 5;
    const double diffX = endX - startX;
    const double diffY = endY - startY;
    const double diffXStep = diffX / steps;
    const double diffYStep = diffY / steps;

    QTestEventList events;
    events.addMouseMove(QPoint(startX, startY));
    events.addMousePress(Qt::LeftButton, Qt::NoModifier, QPoint(startX, startY));
    for (int i = 0; i < steps - 1; ++i) {
        events.addMouseMove(QPoint(startX + i * diffXStep, startY + i * diffYStep));
        events.addDelay(100);
    }
    events.addMouseMove(QPoint(endX, endY));
    events.addDelay(100);
    events.addMouseRelease(Qt::LeftButton, Qt::NoModifier, QPoint(endX, endY));

    events.simulate(target);
}

void PartTest::testSaveAs()
{
    QFETCH(QString, file);
    QFETCH(QString, extension);
    QFETCH(bool, nativelySupportsAnnotations);
    QFETCH(bool, canSwapBackingFile);

    QScopedPointer<CloseDialogHelper> closeDialogHelper;

    QString annotName;
    QTemporaryFile archiveSave( QString( "%1/okrXXXXXX.okular" ).arg( QDir::tempPath() ) );
    QTemporaryFile nativeDirectSave( QString( "%1/okrXXXXXX.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QTemporaryFile nativeFromArchiveFile( QString( "%1/okrXXXXXX.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QVERIFY( archiveSave.open() );
    archiveSave.close();
    QVERIFY( nativeDirectSave.open() );
    nativeDirectSave.close();
    QVERIFY( nativeFromArchiveFile.open() );
    nativeFromArchiveFile.close();

    qDebug() << "Open file, add annotation and save both natively and to .okular";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( file );

        QCOMPARE(part.m_document->canSwapBackingFile(), canSwapBackingFile);

        Okular::Annotation *annot = new Okular::TextAnnotation();
        annot->setBoundingRectangle( Okular::NormalizedRect( 0.1, 0.1, 0.15, 0.15 ) );
        annot->setContents( "annot contents" );
        part.m_document->addPageAnnotation( 0, annot );
        annotName = annot->uniqueName();

        if ( canSwapBackingFile )
        {
            if ( !nativelySupportsAnnotations )
            {
                closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
            }
            QVERIFY( part.saveAs( QUrl::fromLocalFile( nativeDirectSave.fileName() ), Part::NoSaveAsFlags ) );
            // For backends that don't support annotations natively we mark the part as still modified
            // after a save because we keep the annotation around but it will get lost if the user closes the app
            // so we want to give her a last chance to save on close with the "you have changes dialog"
            QCOMPARE( part.isModified(), !nativelySupportsAnnotations );
            QVERIFY( part.saveAs( QUrl::fromLocalFile( archiveSave.fileName() ), Part::SaveAsOkularArchive ) );
        }
        else
        {
            // We need to save to archive first otherwise we lose the annotation

            closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::Yes )); // this is the "you're going to lose the undo/redo stack" dialog
            QVERIFY( part.saveAs( QUrl::fromLocalFile( archiveSave.fileName() ), Part::SaveAsOkularArchive ) );

            if ( !nativelySupportsAnnotations )
            {
                closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
            }
            QVERIFY( part.saveAs( QUrl::fromLocalFile( nativeDirectSave.fileName() ), Part::NoSaveAsFlags ) );
        }

        part.closeUrl();
    }

    qDebug() << "Open the .okular, check that the annotation is present and save to native";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( archiveSave.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), 1 );
        QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        if ( !nativelySupportsAnnotations )
        {
            closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
        }
        QVERIFY( part.saveAs( QUrl::fromLocalFile( nativeFromArchiveFile.fileName() ), Part::NoSaveAsFlags ) );

        if ( canSwapBackingFile && !nativelySupportsAnnotations )
        {
            // For backends that don't support annotations natively we mark the part as still modified
            // after a save because we keep the annotation around but it will get lost if the user closes the app
            // so we want to give her a last chance to save on close with the "you have changes dialog"
            closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "do you want to save or discard" dialog
        }

        part.closeUrl();
    }

    qDebug() << "Open the native file saved directly, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( nativeDirectSave.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        part.closeUrl();
    }

    qDebug() << "Open the native file saved from the .okular, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( nativeFromArchiveFile.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        part.closeUrl();
    }
}

void PartTest::testSaveAs_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("nativelySupportsAnnotations");
    QTest::addColumn<bool>("canSwapBackingFile");

    QTest::newRow("pdf") << KDESRCDIR "data/file1.pdf" << "pdf" << true << true;
    QTest::newRow("pdf.gz") << KDESRCDIR "data/file1.pdf.gz" << "pdf" << true << true;
    QTest::newRow("epub") << KDESRCDIR "data/contents.epub" << "epub" << false << false;
    QTest::newRow("jpg") << KDESRCDIR "data/potato.jpg" << "jpg" << false << true;
}

void PartTest::testSaveAsUndoStackAnnotations()
{
    QFETCH(QString, file);
    QFETCH(QString, extension);
    QFETCH(bool, nativelySupportsAnnotations);
    QFETCH(bool, canSwapBackingFile);
    QFETCH(bool, saveToArchive);

    const Part::SaveAsFlag saveFlags = saveToArchive ? Part::SaveAsOkularArchive : Part::NoSaveAsFlags;

    QScopedPointer<CloseDialogHelper> closeDialogHelper;

    // closeDialogHelper relies on the availability of the "Continue" button to drop changes
    // when saving to a file format not supporting those. However, this button is only sensible
    // and available for "Save As", but not for "Save". By alternately saving to saveFile1 and
    // saveFile2 we always force "Save As", so closeDialogHelper keeps working.
    QTemporaryFile saveFile1( QString( "%1/okrXXXXXX_1.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QVERIFY( saveFile1.open() );
    saveFile1.close();
    QTemporaryFile saveFile2( QString( "%1/okrXXXXXX_2.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QVERIFY( saveFile2.open() );
    saveFile2.close();

    Okular::Part part(nullptr, nullptr, QVariantList());
    part.openDocument( file );

    QCOMPARE(part.m_document->canSwapBackingFile(), canSwapBackingFile);

    Okular::Annotation *annot = new Okular::TextAnnotation();
    annot->setBoundingRectangle( Okular::NormalizedRect( 0.1, 0.1, 0.15, 0.15 ) );
    annot->setContents( "annot contents" );
    part.m_document->addPageAnnotation( 0, annot );
    QString annotName = annot->uniqueName();

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }

    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );

    if (!canSwapBackingFile) {
        // The undo/redo stack gets lost if you can not swap the backing file
        QVERIFY( !part.m_document->canUndo() );
        QVERIFY( !part.m_document->canRedo() );
        return;
    }

    // Check we can still undo the annot add after save
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( !part.m_document->canUndo() );

    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->page( 0 )->annotations().isEmpty() );

    // Check we can redo the annot add after save
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( !part.m_document->canRedo() );

    if ( nativelySupportsAnnotations ) {
        // If the annots are provived by the backend we need to refetch the pointer after save
        annot = part.m_document->page( 0 )->annotation( annotName );
        QVERIFY( annot );
    }



    // Remove the annotation, creates another undo command
    QVERIFY( part.m_document->canRemovePageAnnotation( annot ) );
    part.m_document->removePageAnnotation( 0, annot );
    QVERIFY( part.m_document->page( 0 )->annotations().isEmpty() );

    // Check we can still undo the annot remove after save
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.m_document->canUndo() );
    QCOMPARE( part.m_document->page( 0 )->annotations().count(), 1 );

    // Check we can still undo the annot add after save
    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile2.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( !part.m_document->canUndo() );
    QVERIFY( part.m_document->page( 0 )->annotations().isEmpty() );


    // Redo the add annotation
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.m_document->canUndo() );
    QVERIFY( part.m_document->canRedo() );

    if ( nativelySupportsAnnotations ) {
        // If the annots are provived by the backend we need to refetch the pointer after save
        annot = part.m_document->page( 0 )->annotation( annotName );
        QVERIFY( annot );
    }


    // Add translate, adjust and modify commands
    part.m_document->translatePageAnnotation( 0, annot, Okular::NormalizedPoint( 0.1, 0.1 ) );
    part.m_document->adjustPageAnnotation( 0, annot, Okular::NormalizedPoint( 0.1, 0.1 ), Okular::NormalizedPoint( 0.1, 0.1 ) );
    part.m_document->prepareToModifyAnnotationProperties( annot );
    part.m_document->modifyPageAnnotationProperties( 0, annot );

    // Now check we can still undo/redo/save at all the intermediate states and things still work
    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile2.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.m_document->canUndo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.m_document->canUndo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile2.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.m_document->canUndo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( !part.m_document->canUndo() );
    QVERIFY( part.m_document->canRedo() );
    QVERIFY( part.m_document->page( 0 )->annotations().isEmpty() );

    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.m_document->canRedo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile2.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.m_document->canRedo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile1.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.m_document->canRedo() );

    if ( !nativelySupportsAnnotations && !saveToArchive ) {
        closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "you're going to lose the annotations" dialog
    }
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile2.fileName() ), saveFlags ) );
    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( !part.m_document->canRedo() );

    closeDialogHelper.reset(new CloseDialogHelper( &part, QDialogButtonBox::No  )); // this is the "do you want to save or discard" dialog
    part.closeUrl();
}

void PartTest::testSaveAsUndoStackAnnotations_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("nativelySupportsAnnotations");
    QTest::addColumn<bool>("canSwapBackingFile");
    QTest::addColumn<bool>("saveToArchive");

    QTest::newRow("pdf") << KDESRCDIR "data/file1.pdf" << "pdf" << true << true << false;
    QTest::newRow("epub") << KDESRCDIR "data/contents.epub" << "epub" << false << false << false;
    QTest::newRow("jpg") << KDESRCDIR "data/potato.jpg" << "jpg" << false << true << false;
    QTest::newRow("pdfarchive") << KDESRCDIR "data/file1.pdf" << "okular" << true << true << true;
    QTest::newRow("jpgarchive") << KDESRCDIR "data/potato.jpg" << "okular" << false << true << true;
}

void PartTest::testSaveAsUndoStackForms()
{
    QFETCH(QString, file);
    QFETCH(QString, extension);
    QFETCH(bool, saveToArchive);

    const Part::SaveAsFlag saveFlags = saveToArchive ? Part::SaveAsOkularArchive : Part::NoSaveAsFlags;

    QTemporaryFile saveFile( QString( "%1/okrXXXXXX.%2" ).arg( QDir::tempPath(), extension ) );
    QVERIFY( saveFile.open() );
    saveFile.close();

    Okular::Part part(nullptr, nullptr, QVariantList());
    part.openDocument( file );

    for ( FormField *ff : part.m_document->page( 0 )->formFields() )
    {
        if ( ff->id() == 65537 )
        {
            QCOMPARE( ff->type(), FormField::FormText );
            FormFieldText *fft = static_cast<FormFieldText *>( ff );
            part.m_document->editFormText( 0, fft, "BlaBla", 6, 0, 0 );
        }
        else if ( ff->id() == 65538 )
        {
            QCOMPARE( ff->type(), FormField::FormButton );
            FormFieldButton *ffb = static_cast<FormFieldButton *>( ff );
            QCOMPARE( ffb->buttonType(), FormFieldButton::Radio );
            part.m_document->editFormButtons( 0, QList< FormFieldButton* >() << ffb, QList< bool >() << true );
        }
        else if ( ff->id() == 65542 )
        {
            QCOMPARE( ff->type(), FormField::FormChoice );
            FormFieldChoice *ffc = static_cast<FormFieldChoice *>( ff );
            QCOMPARE( ffc->choiceType(), FormFieldChoice::ListBox );
            part.m_document->editFormList( 0, ffc, QList< int >() << 1 );
        }
        else if ( ff->id() == 65543 )
        {
            QCOMPARE( ff->type(), FormField::FormChoice );
            FormFieldChoice *ffc = static_cast<FormFieldChoice *>( ff );
            QCOMPARE( ffc->choiceType(), FormFieldChoice::ComboBox );
            part.m_document->editFormCombo( 0, ffc, "combo2", 3, 0, 0);
        }
    }

    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );
    QVERIFY( !part.m_document->canUndo() );

    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );

    QVERIFY( part.m_document->canRedo() );
    part.m_document->redo();
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), saveFlags ) );
}

void PartTest::testSaveAsUndoStackForms_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("saveToArchive");

    QTest::newRow("pdf") << KDESRCDIR "data/formSamples.pdf" << "pdf" << false;
    QTest::newRow("pdfarchive") << KDESRCDIR "data/formSamples.pdf" << "okular" << true;
}

void PartTest::testOpenUrlArguments()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs);

    KParts::OpenUrlArguments args;
    args.setMimeType(QStringLiteral("text/rtf"));

    part.setArguments(args);

    part.openUrl(QUrl::fromLocalFile(QStringLiteral(KDESRCDIR "data/file1.pdf")));

    QCOMPARE( part.arguments().mimeType(), QStringLiteral("text/rtf") );
}

void PartTest::test388288()
{
    Okular::Part part(nullptr, nullptr, QVariantList());

    part.openUrl(QUrl::fromLocalFile(QStringLiteral(KDESRCDIR "data/file1.pdf")));

    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    QMetaObject::invokeMethod(part.m_pageView, "slotToggleAnnotator", Q_ARG( bool, true ));

    auto annot = new Okular::HighlightAnnotation();
    annot->setHighlightType( Okular::HighlightAnnotation::Highlight );
    const Okular::NormalizedRect r(0.36, 0.16, 0.51, 0.17);
    annot->setBoundingRectangle( r );
    Okular::HighlightAnnotation::Quad q;
    q.setCapStart( false );
    q.setCapEnd( false );
    q.setFeather( 1.0 );
    q.setPoint( Okular::NormalizedPoint( r.left, r.bottom ), 0 );
    q.setPoint( Okular::NormalizedPoint( r.right, r.bottom ), 1 );
    q.setPoint( Okular::NormalizedPoint( r.right, r.top ), 2 );
    q.setPoint( Okular::NormalizedPoint( r.left, r.top ), 3 );
    annot->highlightQuads().append( q );

    part.m_document->addPageAnnotation( 0, annot );

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.5, height * 0.5));
    QTRY_COMPARE(part.m_pageView->cursor().shape(), Qt::OpenHandCursor);

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.4, height * 0.165));
    QTRY_COMPARE(part.m_pageView->cursor().shape(), Qt::ArrowCursor);

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.1, height * 0.165));

    part.m_document->undo();

    annot = new Okular::HighlightAnnotation();
    annot->setHighlightType( Okular::HighlightAnnotation::Highlight );
    annot->setBoundingRectangle( r );
    annot->highlightQuads().append( q );

    part.m_document->addPageAnnotation( 0, annot );

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.5, height * 0.5));
    QTRY_COMPARE(part.m_pageView->cursor().shape(), Qt::OpenHandCursor);
}

void PartTest::testCheckBoxReadOnly()
{
#ifndef HAVE_POPPLER_0_64
    return;
#endif

    const QString testFile = QStringLiteral( KDESRCDIR "data/checkbox_ro.pdf" );
    Okular::Part part( nullptr, nullptr, QVariantList() );
    part.openDocument( testFile );

    // The test document uses the activation action of checkboxes
    // to update the read only state. For this we need the part so that
    // undo / redo activates the activation action.

    QVERIFY( part.m_document->isOpened() );

    const Okular::Page* page = part.m_document->page( 0 );

    QMap<QString, Okular::FormField *> fields;

    // Field names in test document are:
    // CBMakeRW, CBMakeRO, TargetDefaultRO, TargetDefaultRW

    for ( Okular::FormField *ff: page->formFields() )
    {
        fields.insert( ff->name(), static_cast< Okular::FormField* >( ff ) );
    }

    // First grab all fields and check that the setup is as expected.
    auto cbMakeRW = dynamic_cast< Okular::FormFieldButton* > ( fields[QStringLiteral( "CBMakeRW" )] );
    auto cbMakeRO = dynamic_cast< Okular::FormFieldButton* > ( fields[QStringLiteral( "CBMakeRO" )] );

    auto targetDefaultRW = dynamic_cast< Okular::FormFieldText* > ( fields[QStringLiteral( "TargetDefaultRw" )] );
    auto targetDefaultRO = dynamic_cast< Okular::FormFieldText* > ( fields[QStringLiteral( "TargetDefaultRo" )] );

    QVERIFY( cbMakeRW );
    QVERIFY( cbMakeRO );
    QVERIFY( targetDefaultRW );
    QVERIFY( targetDefaultRO );

    QVERIFY( !cbMakeRW->state() );
    QVERIFY( !cbMakeRO->state() );

    QVERIFY( !targetDefaultRW->isReadOnly() );
    QVERIFY( targetDefaultRO->isReadOnly() );

    QList< Okular::FormFieldButton* > btns;
    btns << cbMakeRW << cbMakeRO;

    // Now check both boxes
    QList< bool > btnStates;
    btnStates << true << true;

    part.m_document->editFormButtons( 0, btns, btnStates );

    // Read only should be inverted
    QVERIFY( targetDefaultRW->isReadOnly() );
    QVERIFY( !targetDefaultRO->isReadOnly() );

    // Test that undo / redo works
    QVERIFY( part.m_document->canUndo() );
    part.m_document->undo();
    QVERIFY( !targetDefaultRW->isReadOnly() );
    QVERIFY( targetDefaultRO->isReadOnly() );

    part.m_document->redo();
    QVERIFY( targetDefaultRW->isReadOnly() );
    QVERIFY( !targetDefaultRO->isReadOnly() );

    btnStates.clear();
    btnStates << false << true;

    part.m_document->editFormButtons( 0, btns, btnStates );
    QVERIFY( targetDefaultRW->isReadOnly() );
    QVERIFY( targetDefaultRO->isReadOnly() );

    // Now set both to checked again and confirm that
    // save / load works.
    btnStates.clear();
    btnStates << true << true;
    part.m_document->editFormButtons( 0, btns, btnStates );

    QTemporaryFile saveFile( QString( "%1/okrXXXXXX.pdf" ).arg( QDir::tempPath() ) );
    QVERIFY( saveFile.open() );
    saveFile.close();

    // Save
    QVERIFY( part.saveAs( QUrl::fromLocalFile( saveFile.fileName() ), Part::NoSaveAsFlags ) );
    part.closeUrl();

    // Load
    part.openDocument( saveFile.fileName() );
    QVERIFY( part.m_document->isOpened() );

    page = part.m_document->page( 0 );

    fields.clear();

    for ( Okular::FormField *ff: page->formFields() )
    {
        fields.insert( ff->name(), static_cast< Okular::FormField* >( ff ) );
    }

    cbMakeRW = dynamic_cast< Okular::FormFieldButton* > ( fields[QStringLiteral( "CBMakeRW" )] );
    cbMakeRO = dynamic_cast< Okular::FormFieldButton* > ( fields[QStringLiteral( "CBMakeRO" )] );

    targetDefaultRW = dynamic_cast< Okular::FormFieldText* > ( fields[QStringLiteral( "TargetDefaultRw" )] );
    targetDefaultRO = dynamic_cast< Okular::FormFieldText* > ( fields[QStringLiteral( "TargetDefaultRo" )] );

    QVERIFY( cbMakeRW->state() );
    QVERIFY( cbMakeRO->state() );
    QVERIFY( targetDefaultRW->isReadOnly() );
    QVERIFY( !targetDefaultRO->isReadOnly() );
}

void PartTest::testCrashTextEditDestroy()
{
    const QString testFile = QStringLiteral( KDESRCDIR "data/formSamples.pdf" );
    Okular::Part part( nullptr, nullptr, QVariantList() );
    part.openDocument( testFile );
    part.widget()->show();
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    part.widget()->findChild<QTextEdit*>()->setText("HOLA");
    part.actionCollection()->action(QStringLiteral("view_toggle_forms"))->trigger();
}

void PartTest::testAnnotWindow()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/file1.pdf")));
    part.widget()->show();
    part.widget()->resize(800, 600);
    QVERIFY(QTest::qWaitForWindowExposed(part.widget()));

    const int width =  part.m_pageView->horizontalScrollBar()->maximum() +
                       part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseNormal");

    QCOMPARE(part.m_document->currentPage(), 0u);

    // Create two distinct text annotations
    Okular::Annotation * annot1 = new Okular::TextAnnotation();
    annot1->setBoundingRectangle( Okular::NormalizedRect( 0.8, 0.1, 0.85, 0.15 ) );
    annot1->setContents( QStringLiteral("Annot contents 111111") );

    Okular::Annotation *annot2 = new Okular::TextAnnotation();
    annot2->setBoundingRectangle( Okular::NormalizedRect(  0.8, 0.3, 0.85, 0.35 ) );
    annot2->setContents( QStringLiteral("Annot contents 222222") );

    // Add annot1 and annot2 to document
    part.m_document->addPageAnnotation( 0, annot1 );
    QTest::qWait(100);
    part.m_document->addPageAnnotation( 0, annot2 );
    QTest::qWait(100);
    QVERIFY( part.m_document->page( 0 )->annotations().size() == 2 );

    // Double click the first annotation to open its window (move mouse for visual feedback)
    const NormalizedPoint annot1pt = annot1->boundingRectangle().center();
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * annot1pt.x, height * annot1pt.y));
    QTest::mouseDClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * annot1pt.x, height * annot1pt.y));
    QTRY_COMPARE( part.m_pageView->findChildren<QFrame *>("AnnotWindow").size(), 1 );
    // Verify that the window is visible
    QFrame* win1 = part.m_pageView->findChild<QFrame *>("AnnotWindow");
    QVERIFY( !win1->visibleRegion().isEmpty() );

    // Double click the second annotation to open its window (move mouse for visual feedback)
    const NormalizedPoint annot2pt = annot2->boundingRectangle().center();
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * annot2pt.x, height * annot2pt.y));
    QTest::mouseDClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * annot2pt.x, height * annot2pt.y));
    QTRY_COMPARE( part.m_pageView->findChildren<QFrame *>("AnnotWindow").size(), 2 );
    // Verify that the first window is hidden covered by the second, which is visible
    QList<QFrame *> lstWin = part.m_pageView->findChildren<QFrame *>("AnnotWindow");
    QFrame * win2;
    if (lstWin[0] == win1) {
        win2 = lstWin[1];
    } else {
        win2 = lstWin[0];
    }
    QVERIFY( win1->visibleRegion().isEmpty() );
    QVERIFY( !win2->visibleRegion().isEmpty() );

    // Double click the first annotation to raise its window (move mouse for visual feedback)
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * annot1pt.x, height * annot1pt.y));
    QTest::mouseDClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * annot1pt.x, height * annot1pt.y));
    // Verify that the second window is hidden covered by the first, which is visible
    QVERIFY( !win1->visibleRegion().isEmpty() );
    QVERIFY( win2->visibleRegion().isEmpty() );

    // Move annotation window 1 to partially show annotation window 2
    win1->move(QPoint(win2->pos().x(),  win2->pos().y() + 50));
    // Verify that both windows are partially visible
    QVERIFY( !win1->visibleRegion().isEmpty() );
    QVERIFY( !win2->visibleRegion().isEmpty() );

    // Click the second annotation window to raise it (move mouse for visual feedback)
    auto widget = win2->window()->childAt(win2->mapTo(win2->window(), QPoint(10,  10)));
    QTest::mouseMove(win2->window(), win2->mapTo(win2->window(), QPoint(10,  10)));
    QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, widget->mapFrom(win2, QPoint(10,  10)));
    QVERIFY( win1->visibleRegion().rects().count() == 3);
    QVERIFY( win2->visibleRegion().rects().count() == 4);
}

// Helper for testAdditionalActionTriggers
static void verifyTargetStates( const QString & triggerName,
                                const QMap<QString, Okular::FormField *> &fields,
                                bool focusVisible, bool cursorVisible, bool mouseVisible,
                                int line)
{
    Okular::FormField *focusTarget = fields.value( triggerName + QStringLiteral ("_focus_target") );
    Okular::FormField *cursorTarget = fields.value( triggerName + QStringLiteral ("_cursor_target") );
    Okular::FormField *mouseTarget = fields.value( triggerName + QStringLiteral ("_mouse_target") );

    QVERIFY( focusTarget );
    QVERIFY( cursorTarget );
    QVERIFY( mouseTarget );

    QTRY_VERIFY2( focusTarget->isVisible() == focusVisible,
                  QStringLiteral ("line: %1 focus for %2 not matched. Expected %3 Actual %4").
                  arg( line ).arg( triggerName ).arg( focusTarget->isVisible() ).arg( focusVisible ).toUtf8().constData() );
    QTRY_VERIFY2( cursorTarget->isVisible() == cursorVisible,
                  QStringLiteral ("line: %1 cursor for %2 not matched. Actual %3 Expected %4").
                  arg( line ).arg( triggerName ).arg( cursorTarget->isVisible() ).arg( cursorVisible ).toUtf8().constData() );
    QTRY_VERIFY2( mouseTarget->isVisible() == mouseVisible,
                  QStringLiteral ("line: %1 mouse for %2 not matched. Expected %3 Actual %4").
                  arg( line ).arg( triggerName ).arg( mouseTarget->isVisible() ).arg( mouseVisible ).toUtf8().constData() );
}

void PartTest::testAdditionalActionTriggers()
{
#ifndef HAVE_POPPLER_0_65
    return;
#endif
    const QString testFile = QStringLiteral( KDESRCDIR "data/additionalFormActions.pdf" );
    Okular::Part part( nullptr, nullptr, QVariantList() );
    part.openDocument( testFile );
    part.widget()->resize(800, 600);

    part.widget()->show();
    QVERIFY( QTest::qWaitForWindowExposed( part.widget() ) );

    QMap<QString, Okular::FormField *> fields;
    // Field names in test document are:
    // For trigger fields: tf, cb, rb, dd, pb
    // For target fields: <trigger_name>_focus_target, <trigger_name>_cursor_target,
    // <trigger_name>_mouse_target
    const Okular::Page* page = part.m_document->page( 0 );
    for ( Okular::FormField *ff: page->formFields() )
    {
        fields.insert( ff->name(), static_cast< Okular::FormField* >( ff ) );
    }

    // Verify that everything is set up.
    verifyTargetStates( QStringLiteral( "tf" ), fields, true, true, true, __LINE__ );
    verifyTargetStates( QStringLiteral( "cb" ), fields, true, true, true, __LINE__ );
    verifyTargetStates( QStringLiteral( "rb" ), fields, true, true, true, __LINE__ );
    verifyTargetStates( QStringLiteral( "dd" ), fields, true, true, true, __LINE__ );
    verifyTargetStates( QStringLiteral( "pb" ), fields, true, true, true, __LINE__ );

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage( 0 );

    // wait for pixmap
    QTRY_VERIFY( part.m_document->page( 0 )->hasPixmap( part.m_pageView) );

    part.actionCollection()->action( QStringLiteral( "view_toggle_forms" ) )->trigger();

    QPoint tfPos( width * 0.045, height * 0.05 );
    QPoint cbPos( width * 0.045, height * 0.08 );
    QPoint rbPos( width * 0.045, height * 0.12 );
    QPoint ddPos( width * 0.045, height * 0.16 );
    QPoint pbPos( width * 0.045, height * 0.26 );

    // Test text field
    auto widget = part.m_pageView->viewport()->childAt( tfPos );
    QVERIFY( widget );

    QTest::mouseMove( part.m_pageView->viewport(), QPoint( tfPos ));
    verifyTargetStates( QStringLiteral( "tf" ), fields, true, false, true, __LINE__ );
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "tf" ), fields, false, false, false, __LINE__ );
    QTest::mouseRelease( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "tf" ), fields, false, false, true, __LINE__ );

    // Checkbox
    widget = part.m_pageView->viewport()->childAt( cbPos );
    QVERIFY( widget );

    QTest::mouseMove( part.m_pageView->viewport(), QPoint( cbPos ) );
    verifyTargetStates( QStringLiteral( "cb" ), fields, true, false, true, __LINE__ );
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "cb" ), fields, false, false, false, __LINE__ );
    // Confirm that the textfield no longer has any invisible
    verifyTargetStates( QStringLiteral( "tf" ), fields, true, true, true, __LINE__ );
    QTest::mouseRelease( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "cb" ), fields, false, false, true, __LINE__ );

    // Radio
    widget = part.m_pageView->viewport()->childAt( rbPos );
    QVERIFY( widget );

    QTest::mouseMove( part.m_pageView->viewport(), QPoint( rbPos ) );
    verifyTargetStates( QStringLiteral( "rb" ), fields, true, false, true, __LINE__ );
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "rb" ), fields, false, false, false, __LINE__ );
    QTest::mouseRelease( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "rb" ), fields, false, false, true, __LINE__ );

    // Dropdown
    widget = part.m_pageView->viewport()->childAt( ddPos );
    QVERIFY( widget );

    QTest::mouseMove( part.m_pageView->viewport(), QPoint( ddPos ) );
    verifyTargetStates( QStringLiteral( "dd" ), fields, true, false, true, __LINE__ );
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "dd" ), fields, false, false, false, __LINE__ );
    QTest::mouseRelease( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "dd" ), fields, false, false, true, __LINE__ );

    // Pushbutton
    widget = part.m_pageView->viewport()->childAt( pbPos );
    QVERIFY( widget );

    QTest::mouseMove( part.m_pageView->viewport(), QPoint( pbPos ) );
    verifyTargetStates( QStringLiteral( "pb" ), fields, true, false, true, __LINE__ );
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "pb" ), fields, false, false, false, __LINE__ );
    QTest::mouseRelease( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "pb" ), fields, false, false, true, __LINE__ );

    // Confirm that a mouse release outside does not trigger the show action.
    QTest::mousePress( widget, Qt::LeftButton, Qt::NoModifier, QPoint( 5, 5 ) );
    verifyTargetStates( QStringLiteral( "pb" ), fields, false, false, false, __LINE__ );
    QTest::mouseRelease( part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, tfPos );
    verifyTargetStates( QStringLiteral( "pb" ), fields, false, false, false, __LINE__ );
}

} // namespace Okular

int main(int argc, char *argv[])
{
    // Force consistent locale
    QLocale locale(QStringLiteral("en_US.UTF-8"));
    if (locale == QLocale::c()) { // This is the way to check if the above worked
        locale = QLocale(QLocale::English, QLocale::UnitedStates);
    }

    QLocale::setDefault(locale);
    qputenv("LC_ALL", "en_US.UTF-8"); // For UNIX, third-party libraries

    // Ensure consistent configs/caches
    QTemporaryDir homeDir; // QTemporaryDir automatically cleans up when it goes out of scope
    Q_ASSERT(homeDir.isValid());
    QByteArray homePath = QFile::encodeName(homeDir.path());
    qDebug() << homePath;
    qputenv("USERPROFILE", homePath);
    qputenv("HOME", homePath);
    qputenv("XDG_DATA_HOME", homePath + "/.local");
    qputenv("XDG_CONFIG_HOME", homePath + "/.kde-unit-test/xdg/config");

    // Disable fancy debug output
    qunsetenv("QT_MESSAGE_PATTERN");

    QApplication app( argc, argv );
    app.setApplicationName(QLatin1String("okularparttest"));
    app.setOrganizationDomain(QLatin1String("kde.org"));
    app.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<QUrl>(); /*as done by kapplication*/
    qRegisterMetaType<QList<QUrl>>();

    Okular::PartTest test;

    return QTest::qExec( &test, argc, argv );
}

#include "parttest.moc"
