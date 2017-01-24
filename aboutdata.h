/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ABOUTDATA_H_
#define _ABOUTDATA_H_

#include <KAboutData>

#include "core/version.h"

#include <klocalizedstring.h>

inline KAboutData okularAboutData()
{
    KAboutData about(
        QStringLiteral("okular"),
        i18n("Okular"),
        QStringLiteral(OKULAR_VERSION_STRING),
        i18n("Okular, a universal document viewer"),
        KAboutLicense::GPL,
        i18n("(C) 2002 Wilco Greven, Christophe Devriese\n"
              "(C) 2004-2005 Enrico Ros\n"
              "(C) 2005 Piotr Szymanski\n"
              "(C) 2004-2009 Albert Astals Cid\n"
              "(C) 2006-2009 Pino Toscano"),
        QString(),
        QStringLiteral("http://okular.kde.org")
    );

    about.addAuthor(QStringLiteral("Pino Toscano"), i18n("Former maintainer"), QStringLiteral("pino@kde.org"));
    about.addAuthor(QStringLiteral("Tobias Koenig"), i18n("Lots of framework work, ODT and FictionBook backends"), QStringLiteral("tokoe@kde.org"));
    about.addAuthor(QStringLiteral("Albert Astals Cid"), i18n("Current maintainer"), QStringLiteral("aacid@kde.org"));
    about.addAuthor(QStringLiteral("Piotr Szymanski"), i18n("Created Okular from KPDF codebase"), QStringLiteral("djurban@pld-dc.org"));
    about.addAuthor(QStringLiteral("Enrico Ros"), i18n("KPDF developer"), QStringLiteral("eros.kde@email.it"));
    about.addCredit(QStringLiteral("Eugene Trounev"), i18n("Annotations artwork"), QStringLiteral("eugene.trounev@gmail.com"));
    about.addCredit(QStringLiteral("Jiri Baum - NICTA"), i18n("Table selection tool"), QStringLiteral("jiri@baum.com.au"));
    about.addCredit(QStringLiteral("Fabio D'Urso"), i18n("Annotation improvements"), QStringLiteral("fabiodurso@hotmail.it"));

    return about;
}

#endif
