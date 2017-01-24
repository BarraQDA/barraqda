/***************************************************************************
 *   Copyright (C) 2004 by Duncan Mac-Vicar Prett <duncan@kde.org>         *
 *   Copyright (C) 2004-2005 by Olivier Goffart <ogoffart@kde.org>         *
 *   Copyright (C) 2011 by Niels Ole Salscheider                           *
 *                         <niels_ole@salscheider-online.de>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "latexrenderer.h"

#include <QtCore/QDebug>

#include <kprocess.h>

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <qtemporaryfile.h>
#include <QTextStream>
#include <QStandardPaths>

#include "debug_ui.h"

namespace GuiUtils
{

LatexRenderer::LatexRenderer()
{
}

LatexRenderer::~LatexRenderer()
{
    foreach(const QString &file, m_fileList)
    {
        QFile::remove(file);
    }
}

LatexRenderer::Error LatexRenderer::renderLatexInHtml( QString& html, const QColor& textColor, int fontSize, int resolution, QString& latexOutput )
{
    if( !html.contains(QStringLiteral("$$")))
        return NoError;

    // this searches for $$formula$$ 
    QRegExp rg(QStringLiteral("\\$\\$.+\\$\\$"));
    rg.setMinimal(true);

    int pos = 0;

    QMap<QString, QString> replaceMap;
    while (pos >= 0 && pos < html.length())
    {
        pos = rg.indexIn(html, pos);

        if (pos >= 0 )
        {
            const QString match = rg.cap(0);
            pos += rg.matchedLength();

            QString formul=match;
            // first remove the $$ delimiters on start and end
            formul.remove(QStringLiteral("$$"));
            // then trim the result, so we can skip totally empty/whitespace-only formulas
            formul = formul.trimmed();
            if (formul.isEmpty() || !securityCheck(formul))
                continue;

            //unescape formula
            formul.replace(QLatin1String("&gt;"),QLatin1String(">")).replace(QLatin1String("&lt;"),QLatin1String("<")).replace(QLatin1String("&amp;"),QLatin1String("&")).replace(QLatin1String("&quot;"),QLatin1String("\"")).replace(QLatin1String("&apos;"),QLatin1String("\'")).replace(QLatin1String("<br>"),QLatin1String(" "));

            QString fileName;
            Error returnCode = handleLatex(fileName, formul, textColor, fontSize, resolution, latexOutput);
            if (returnCode != NoError)
                return returnCode;

            replaceMap[match] = fileName;
        }
    }
    
    if(replaceMap.isEmpty()) //we haven't found any LaTeX strings
        return NoError;
    
    int imagePxWidth,imagePxHeight;
    for (QMap<QString,QString>::ConstIterator it = replaceMap.constBegin(); it != replaceMap.constEnd(); ++it)
    {
        QImage theImage(*it);
        if(theImage.isNull())
            continue;
        imagePxWidth = theImage.width();
        imagePxHeight = theImage.height();
        QString escapedLATEX=it.key().toHtmlEscaped().replace(QLatin1Char('"'),QLatin1String("&quot;"));  //we need  the escape quotes because that string will be in a title="" argument, but not the \n
        html.replace(it.key(), QStringLiteral(" <img width=\"") + QString::number(imagePxWidth) + QStringLiteral("\" height=\"") + QString::number(imagePxHeight) + QStringLiteral("\" align=\"middle\" src=\"") + (*it) + QStringLiteral("\"  alt=\"") + escapedLATEX + QStringLiteral("\" title=\"") + escapedLATEX + QStringLiteral("\"  /> "));
    }
    return NoError;
}

bool LatexRenderer::mightContainLatex (const QString& text)
{
    if( !text.contains(QStringLiteral("$$")))
        return false;

    // this searches for $$formula$$ 
    QRegExp rg(QStringLiteral("\\$\\$.+\\$\\$"));
    rg.setMinimal(true);
    if( rg.lastIndexIn(text) == -1 )
        return false;

    return true;
}

LatexRenderer::Error LatexRenderer::handleLatex( QString& fileName, const QString& latexFormula, const QColor& textColor, int fontSize, int resolution, QString& latexOutput )
{
    KProcess latexProc;
    KProcess dvipngProc;

    QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/okular_kdelatex-XXXXXX.tex"));
    tempFile->open();
    QString tempFileName = tempFile->fileName();
    QFileInfo *tempFileInfo = new QFileInfo(tempFileName);
    QString tempFileNameNS = tempFileInfo->absolutePath() + QDir::separator() + tempFileInfo->baseName();
    QString tempFilePath = tempFileInfo->absolutePath();
    delete tempFileInfo;
    QTextStream tempStream(tempFile);

    tempStream << "\
\\documentclass[" << fontSize << "pt]{article} \
\\usepackage{color} \
\\usepackage{amsmath,latexsym,amsfonts,amssymb,ulem} \
\\pagestyle{empty} \
\\begin{document} \
{\\color[rgb]{" << textColor.redF() << "," << textColor.greenF() << "," << textColor.blueF() << "} \
\\begin{eqnarray*} \
" << latexFormula << " \
\\end{eqnarray*}} \
\\end{document}";

    tempFile->close();
    QString latexExecutable = QStandardPaths::findExecutable(QStringLiteral("latex"));
    if (latexExecutable.isEmpty())
    {
        qCDebug(OkularUiDebug) << "Could not find latex!";
        delete tempFile;
        fileName = QString();
        return LatexNotFound;
    }
    latexProc << latexExecutable << QStringLiteral("-interaction=nonstopmode") << QStringLiteral("-halt-on-error") << QStringLiteral("-output-directory=%1").arg(tempFilePath) << tempFile->fileName();
    latexProc.setOutputChannelMode( KProcess::MergedChannels );
    latexProc.execute();
    latexOutput = QString::fromLocal8Bit(latexProc.readAll());
    tempFile->remove();

    QFile::remove(tempFileNameNS + QStringLiteral(".log"));
    QFile::remove(tempFileNameNS + QStringLiteral(".aux"));
    delete tempFile;

    if (!QFile::exists(tempFileNameNS + QStringLiteral(".dvi")))
    {
        fileName = QString();
        return LatexFailed;
    }

    QString dvipngExecutable = QStandardPaths::findExecutable(QStringLiteral("dvipng"));
    if (dvipngExecutable.isEmpty())
    {
        qCDebug(OkularUiDebug) << "Could not find dvipng!";
        fileName = QString();
        return DvipngNotFound;
    }

    dvipngProc << dvipngExecutable << QStringLiteral("-o%1").arg(tempFileNameNS + QStringLiteral(".png")) << QStringLiteral("-Ttight") << QStringLiteral("-bgTransparent") << QStringLiteral("-D %1").arg(resolution) << QStringLiteral("%1").arg(tempFileNameNS + QStringLiteral(".dvi"));
    dvipngProc.setOutputChannelMode( KProcess::MergedChannels );
    dvipngProc.execute();

    QFile::remove(tempFileNameNS + QStringLiteral(".dvi"));
    
    if (!QFile::exists(tempFileNameNS + QStringLiteral(".png")))
    {
        fileName = QString();
        return DvipngFailed;
    }

    fileName = tempFileNameNS + QStringLiteral(".png");
    m_fileList << fileName;
    return NoError;
}

bool LatexRenderer::securityCheck( const QString &latexFormula )
{
    return !latexFormula.contains(QRegExp(QString::fromLatin1("\\\\(def|let|futurelet|newcommand|renewcommand|else|fi|write|input|include"
    "|chardef|catcode|makeatletter|noexpand|toksdef|every|errhelp|errorstopmode|scrollmode|nonstopmode|batchmode"
    "|read|csname|newhelp|relax|afterground|afterassignment|expandafter|noexpand|special|command|loop|repeat|toks"
    "|output|line|mathcode|name|item|section|mbox|DeclareRobustCommand)[^a-zA-Z]")));
}

}
