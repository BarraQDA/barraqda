/***************************************************************************
 *   Copyright (C) 2015 by Alex Richardson <arichardson.kde@gmail.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest/QTest>
#include <QDirIterator>
#include <QDebug>
#include <QStringList>
#include <KPluginFactory>
#include <KPluginLoader>

#include "../generator.h"

class GeneratorsTest : public QObject
{
    Q_OBJECT
private slots:
    void testLoadsCorrectly();
};

void GeneratorsTest::testLoadsCorrectly()
{
    QCoreApplication::setLibraryPaths(QStringList());
    QVERIFY2(QDir(QStringLiteral(GENERATORS_BUILD_DIR)).exists(), GENERATORS_BUILD_DIR);
    // find all possible generators in $CMAKE_BINARY_DIR/generators
    // We can't simply hardcore the list of generators since some might not be built
    // depending on which dependencies were found by CMake
    QStringList generatorLibs;
    QDirIterator it(QStringLiteral(GENERATORS_BUILD_DIR), QDir::Files | QDir::Executable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (QLibrary::isLibrary(it.fileName())) {
            if (it.fileName().startsWith(QLatin1String("kio_"))) {
                continue; // don't check kio_msits.so
            }
            generatorLibs <<  it.fileInfo().absoluteFilePath();
        }
    }
    int failures = 0;
    int successful = 0;
    foreach (const QString& lib, generatorLibs) {
        KPluginLoader loader(lib);
        QVERIFY2(!loader.fileName().isEmpty(), qPrintable(lib));
        qDebug() << loader.fileName();
        auto factory = loader.factory();
        if (!factory) {
            qWarning() << "Could not get KPluginFactory for" << lib;
            failures++;
            continue;
        }
        Okular::Generator* generator = factory->create<Okular::Generator>();
        if (!generator) {
            qWarning() << "Failed to cast" << lib << "to Okular::Generator";
            // without the necessary Q_INTERFACES() qobject_cast fails!
            auto obj = factory->create<QObject>();
            qDebug() << "Object is of type " << obj->metaObject()->className();
            qDebug() << "dynamic_cast:" << dynamic_cast<Okular::Generator*>(obj);
            qDebug() << "qobject_cast:" << qobject_cast<Okular::Generator*>(obj);
            failures++;
            continue;
        }
        successful++;
    }
    qDebug() << "Successfully loaded" << successful << "generators";
    QCOMPARE(failures, 0);
}

QTEST_MAIN(GeneratorsTest)

#include "generatorstest.moc"
