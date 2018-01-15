/***************************************************************************
 *   Copyright (C) 2007,2010 by John Layt <john@layt.net>                  *
 *                                                                         *
 *   FilePrinterPreview based on KPrintPreview (originally LGPL)           *
 *   Copyright (c) 2007 Alex Merry <huntedhacker@tiscali.co.uk>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "fileprinter.h"

#include <QtPrintSupport/QPrinter>
#include <QPrintEngine>
#include <QStringList>
#include <QSize>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QLabel>
#include <QtGui/QShowEvent>
#include <QtNetwork/QTcpSocket>

#include <KProcess>
#include <KShell>
#include <QtCore/QDebug>
#include <QStandardPaths>

#include "debug_p.h"

using namespace Okular;

int FilePrinter::printFile( QPrinter &printer, const QString file,
                            QPrinter::Orientation documentOrientation, FileDeletePolicy fileDeletePolicy,
                            PageSelectPolicy pageSelectPolicy, const QString &pageRange )
{
    FilePrinter fp;
    return fp.doPrintFiles( printer, QStringList( file ), fileDeletePolicy, pageSelectPolicy, pageRange, 
                            documentOrientation );
}

int FilePrinter::doPrintFiles( QPrinter &printer, QStringList fileList, FileDeletePolicy fileDeletePolicy,
                               PageSelectPolicy pageSelectPolicy, const QString &pageRange,
                               QPrinter::Orientation documentOrientation )
{

    if ( fileList.size() < 1 ) {
        return -8;
    }

    for (QStringList::ConstIterator it = fileList.constBegin(); it != fileList.constEnd(); ++it) {
        if (!QFile::exists(*it)) {
            return -7;
        }
    }

    if ( printer.printerState() == QPrinter::Aborted || printer.printerState() == QPrinter::Error ) {
        return -6;
    }

    QString exe;
    QStringList argList;
    int ret;

    // Print to File if a filename set, assumes there must be only 1 file
    if ( !printer.outputFileName().isEmpty() ) {

        if ( QFile::exists( printer.outputFileName() ) ) {
            QFile::remove( printer.outputFileName() );
        }

        QFileInfo inputFileInfo = QFileInfo( fileList[0] );
        QFileInfo outputFileInfo = QFileInfo( printer.outputFileName() );

        bool doDeleteFile = (fileDeletePolicy == FilePrinter::SystemDeletesFiles);
        if ( inputFileInfo.suffix() == outputFileInfo.suffix() ) {
            if ( doDeleteFile ) {
                bool res = QFile::rename( fileList[0], printer.outputFileName() );
                if ( res ) {
                    doDeleteFile = false;
                    ret = 0;
                } else {
                    ret = -5;
                }
            } else {
                bool res = QFile::copy( fileList[0], printer.outputFileName() );
                if ( res ) {
                    ret = 0;
                } else {
                    ret = -5;
                }
            }
        } else if ( inputFileInfo.suffix() == QLatin1String("ps") && printer.outputFormat() == QPrinter::PdfFormat && ps2pdfAvailable() ) {
            exe = QStringLiteral("ps2pdf");
            argList << fileList[0] << printer.outputFileName();
            qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;
            ret = KProcess::execute( exe, argList );
        } else if ( inputFileInfo.suffix() == "pdf" && printer.outputFormat() == QPrinter::NativeFormat && pdf2psAvailable() ) {
            exe = "pdf2ps";
            argList << fileList[0] << printer.outputFileName();
            qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;
            ret = KProcess::execute( exe, argList );
        } else {
            ret = -5;
        }

        if ( doDeleteFile ) {
            QFile::remove( fileList[0] );
        }


    } else {  // Print to a printer via lpr command

        //Decide what executable to use to print with, need the CUPS version of lpr if available
        //Some distros name the CUPS version of lpr as lpr-cups or lpr.cups so try those first 
        //before default to lpr, or failing that to lp

        if ( !QStandardPaths::findExecutable(QStringLiteral("lpr-cups")).isEmpty() ) {
            exe = QStringLiteral("lpr-cups");
        } else if ( !QStandardPaths::findExecutable(QStringLiteral("lpr.cups")).isEmpty() ) {
            exe = QStringLiteral("lpr.cups");
        } else if ( !QStandardPaths::findExecutable(QStringLiteral("lpr")).isEmpty() ) {
            exe = QStringLiteral("lpr");
        } else if ( !QStandardPaths::findExecutable(QStringLiteral("lp")).isEmpty() ) {
            exe = QStringLiteral("lp");
        } else {
            return -9;
        }

        bool useCupsOptions = cupsAvailable();
        argList = printArguments( printer, fileDeletePolicy, pageSelectPolicy, 
                                  useCupsOptions, pageRange, exe, documentOrientation ) << fileList;
        qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;

        ret = KProcess::execute( exe, argList );

    }

    return ret;
}

QList<int> FilePrinter::pageList( QPrinter &printer, int lastPage, const QList<int> &selectedPageList )
{
    return pageList( printer, lastPage, 0, selectedPageList );
}

QList<int> FilePrinter::pageList( QPrinter &printer, int lastPage,
                                  int currentPage, const QList<int> &selectedPageList )
{
    if ( printer.printRange() == QPrinter::Selection) {
        return selectedPageList;
    }

    int startPage, endPage;
    QList<int> list;

    if ( printer.printRange() == QPrinter::PageRange ) {
        startPage = printer.fromPage();
        endPage = printer.toPage();
    } else if ( printer.printRange() == QPrinter::CurrentPage) {
        startPage = currentPage;
        endPage = currentPage;
    } else { //AllPages
        startPage = 1;
        endPage = lastPage;
    }

    for (int i = startPage; i <= endPage; i++ ) {
        list << i;
    }

    return list;
}

QString FilePrinter::pageRange( QPrinter &printer, int lastPage, const QList<int> &selectedPageList )
{
    if ( printer.printRange() == QPrinter::Selection) {
        return pageListToPageRange( selectedPageList );
    }

    if ( printer.printRange() == QPrinter::PageRange ) {
        return QStringLiteral("%1-%2").arg(printer.fromPage()).arg(printer.toPage());
    }

    return QStringLiteral("1-%2").arg( lastPage );
}

QString FilePrinter::pageListToPageRange( const QList<int> &pageList )
{
    QString pageRange;
    int count = pageList.count();
    int i = 0;
    int seqStart = i;
    int seqEnd;

    while ( i != count ) {

        if ( i + 1 == count || pageList[i] + 1 != pageList[i+1] ) {

            seqEnd = i;

            if ( !pageRange.isEmpty() ) {
                pageRange.append(QLatin1Char(','));
            }

            if ( seqStart == seqEnd ) {
                pageRange.append(pageList[i]);
            } else {
                pageRange.append(QStringLiteral("%1-%2").arg(seqStart).arg(seqEnd));
            }

            seqStart = i + 1;
        }

        i++;
    }

    return pageRange;
}

bool FilePrinter::ps2pdfAvailable()
{
    return ( !QStandardPaths::findExecutable(QStringLiteral("ps2pdf")).isEmpty() );
}

bool FilePrinter::pdf2psAvailable()
{
    return ( !QStandardPaths::findExecutable(QStringLiteral("pdf2ps")).isEmpty() );
}

bool FilePrinter::cupsAvailable()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
    // Ideally we would have access to the private Qt method
    // QCUPSSupport::cupsAvailable() to do this as it is very complex routine.
    // However, if CUPS is available then QPrinter::numCopies() will always return 1
    // whereas if CUPS is not available it will return the real number of copies.
    // This behaviour is guaranteed never to change, so we can use it as a reliable substitute.
    QPrinter testPrinter;
    testPrinter.setNumCopies( 2 );
    return ( testPrinter.numCopies() == 1 );
#else
    return false;
#endif
}

bool FilePrinter::detectCupsService()
{
    QTcpSocket qsock;
    qsock.connectToHost(QStringLiteral("localhost"), 631);
    bool rtn = qsock.waitForConnected() && qsock.isValid();
    qsock.abort();
    return rtn;
}

bool FilePrinter::detectCupsConfig()
{
    if ( QFile::exists(QStringLiteral("/etc/cups/cupsd.conf")) ) return true;
    if ( QFile::exists(QStringLiteral("/usr/etc/cups/cupsd.conf")) ) return true;
    if ( QFile::exists(QStringLiteral("/usr/local/etc/cups/cupsd.conf")) ) return true;
    if ( QFile::exists(QStringLiteral("/opt/etc/cups/cupsd.conf")) ) return true;
    if ( QFile::exists(QStringLiteral("/opt/local/etc/cups/cupsd.conf")) ) return true;
    return false;
}

QSize FilePrinter::psPaperSize( QPrinter &printer )
{
    QSize size = printer.pageLayout().pageSize().sizePoints();

    if ( printer.pageSize() == QPrinter::Custom )
    {
        return QSize( (int) printer.widthMM() * ( 25.4 / 72 ),
                      (int) printer.heightMM() * ( 25.4 / 72 ) );
    }

    if ( printer.orientation() == QPrinter::Landscape ) {
        size.transpose();
    }

    return size;
}

Generator::PrintError FilePrinter::printError( int c )
{
    Generator::PrintError pe;
    if ( c >= 0 )
    {
        pe = Generator::NoPrintError;
    } 
    else {
        switch ( c )
        {
            case -1:
                pe = Generator::PrintingProcessCrashPrintError;
                break;
            case -2:
                pe = Generator::PrintingProcessStartPrintError;
                break;
            case -5:
                pe = Generator::PrintToFilePrintError;
                break;
            case -6:
                pe = Generator::InvalidPrinterStatePrintError;
                break;
            case -7:
                pe = Generator::UnableToFindFilePrintError;
                break;
            case -8:
                pe = Generator::NoFileToPrintError;
                break;
            case -9:
                pe = Generator::NoBinaryToPrintError;
                break;
            default:
                pe = Generator::UnknownPrintError;
        }
    }
    return pe;
}



QStringList FilePrinter::printArguments( QPrinter &printer, FileDeletePolicy fileDeletePolicy,
                                         PageSelectPolicy pageSelectPolicy, bool useCupsOptions,
                                         const QString &pageRange, const QString &version, 
                                         QPrinter::Orientation documentOrientation )
{
    QStringList argList;

    if ( ! destination( printer, version ).isEmpty() ) {
        argList << destination( printer, version );
    }

    if ( ! copies( printer, version ).isEmpty() ) {
        argList << copies( printer, version );
    }

    if ( ! jobname( printer, version ).isEmpty() ) {
        argList << jobname( printer, version );
    }

    if ( ! pages( printer, pageSelectPolicy, pageRange, useCupsOptions, version ).isEmpty() ) {
        argList << pages( printer, pageSelectPolicy, pageRange, useCupsOptions, version );
    }

    if ( useCupsOptions && ! cupsOptions( printer, documentOrientation ).isEmpty() ) {
        argList << cupsOptions( printer, documentOrientation);
    }

    if ( ! deleteFile( printer, fileDeletePolicy, version ).isEmpty() ) {
        argList << deleteFile( printer, fileDeletePolicy, version );
    }

    if ( version == QLatin1String("lp") ) {
        argList << QStringLiteral("--");
    }

    return argList;
}

QStringList FilePrinter::destination( QPrinter &printer, const QString &version )
{
    if ( version == QLatin1String("lp") ) {
        return QStringList(QStringLiteral("-d")) << printer.printerName();
    }

    if ( version.startsWith( QLatin1String("lpr") ) ) {
        return QStringList(QStringLiteral("-P")) << printer.printerName();
    }

    return QStringList();
}

QStringList FilePrinter::copies( QPrinter &printer, const QString &version )
{
    int cp = printer.actualNumCopies();

    if ( version == QLatin1String("lp") ) {
        return QStringList(QStringLiteral("-n")) << QStringLiteral("%1").arg( cp );
    }

    if ( version.startsWith( QLatin1String("lpr") ) ) {
        return QStringList() << QStringLiteral("-#%1").arg( cp );
    }

    return QStringList();
}

QStringList FilePrinter::jobname( QPrinter &printer, const QString &version )
{
    if ( ! printer.docName().isEmpty() ) {

        if ( version == QLatin1String("lp") ) {
            return QStringList(QStringLiteral("-t")) << printer.docName();
        }

        if ( version.startsWith( QLatin1String("lpr") ) ) {
            const QString shortenedDocName = QString::fromUtf8(printer.docName().toUtf8().left(255));
            return QStringList(QStringLiteral("-J")) << shortenedDocName;
        }
    }

    return QStringList();
}

QStringList FilePrinter::deleteFile( QPrinter &, FileDeletePolicy fileDeletePolicy, const QString &version )
{
    if ( fileDeletePolicy == FilePrinter::SystemDeletesFiles && version.startsWith( QLatin1String("lpr") ) ) {
        return QStringList(QStringLiteral("-r"));
    }

    return QStringList();
}

QStringList FilePrinter::pages( QPrinter &printer, PageSelectPolicy pageSelectPolicy, const QString &pageRange,
                                    bool useCupsOptions, const QString &version )
{
    if ( pageSelectPolicy == FilePrinter::SystemSelectsPages ) {

        if ( printer.printRange() == QPrinter::Selection && ! pageRange.isEmpty() ) {

            if ( version == QLatin1String("lp") ) {
                return QStringList(QStringLiteral("-P")) << pageRange ;
            }

            if ( version.startsWith( QLatin1String("lpr") ) && useCupsOptions ) {
                return QStringList(QStringLiteral("-o")) << QStringLiteral("page-ranges=%1").arg( pageRange );
            }

        }

        if ( printer.printRange() == QPrinter::PageRange ) {

            if ( version == QLatin1String("lp") ) {
                return QStringList(QStringLiteral("-P")) << QStringLiteral("%1-%2").arg( printer.fromPage() )
                                                            .arg( printer.toPage() );
            }

            if ( version.startsWith( QLatin1String("lpr") ) && useCupsOptions ) {
                return QStringList(QStringLiteral("-o")) << QStringLiteral("page-ranges=%1-%2").arg( printer.fromPage() )
                                                                        .arg( printer.toPage() );
            }

        }

    }

    return QStringList(); // AllPages
}

QStringList FilePrinter::cupsOptions( QPrinter &printer, QPrinter::Orientation documentOrientation )
{
    QStringList optionList;

    if ( ! optionMedia( printer ).isEmpty() ) {
        optionList << optionMedia( printer );
    }

    if ( ! optionOrientation( printer, documentOrientation ).isEmpty() ) {
        optionList << optionOrientation( printer, documentOrientation );
    }

    if ( ! optionDoubleSidedPrinting( printer ).isEmpty() ) {
        optionList << optionDoubleSidedPrinting( printer );
    }

    if ( ! optionPageOrder( printer ).isEmpty() ) {
        optionList << optionPageOrder( printer );
    }

    if ( ! optionCollateCopies( printer ).isEmpty() ) {
        optionList << optionCollateCopies( printer );
    }

    if ( ! optionPageMargins( printer ).isEmpty() ) {
        optionList << optionPageMargins( printer );
    }

    optionList << optionCupsProperties( printer );

    return optionList;
}

QStringList FilePrinter::optionMedia( QPrinter &printer )
{
    if ( ! mediaPageSize( printer ).isEmpty() && 
         ! mediaPaperSource( printer ).isEmpty() ) {
        return QStringList(QStringLiteral("-o")) <<
                QStringLiteral("media=%1,%2").arg( mediaPageSize( printer ), mediaPaperSource( printer ) );
    }

    if ( ! mediaPageSize( printer ).isEmpty() ) {
        return QStringList(QStringLiteral("-o")) <<
                QStringLiteral("media=%1").arg( mediaPageSize( printer ) );
    }

    if ( ! mediaPaperSource( printer ).isEmpty() ) {
        return QStringList(QStringLiteral("-o")) <<
                QStringLiteral("media=%1").arg( mediaPaperSource( printer ) );
    }

    return QStringList();
}

QString FilePrinter::mediaPageSize( QPrinter &printer )
{
    switch ( printer.pageSize() ) {
    case QPrinter::A0:         return QStringLiteral("A0");
    case QPrinter::A1:         return QStringLiteral("A1");
    case QPrinter::A2:         return QStringLiteral("A2");
    case QPrinter::A3:         return QStringLiteral("A3");
    case QPrinter::A4:         return QStringLiteral("A4");
    case QPrinter::A5:         return QStringLiteral("A5");
    case QPrinter::A6:         return QStringLiteral("A6");
    case QPrinter::A7:         return QStringLiteral("A7");
    case QPrinter::A8:         return QStringLiteral("A8");
    case QPrinter::A9:         return QStringLiteral("A9");
    case QPrinter::B0:         return QStringLiteral("B0");
    case QPrinter::B1:         return QStringLiteral("B1");
    case QPrinter::B10:        return QStringLiteral("B10");
    case QPrinter::B2:         return QStringLiteral("B2");
    case QPrinter::B3:         return QStringLiteral("B3");
    case QPrinter::B4:         return QStringLiteral("B4");
    case QPrinter::B5:         return QStringLiteral("B5");
    case QPrinter::B6:         return QStringLiteral("B6");
    case QPrinter::B7:         return QStringLiteral("B7");
    case QPrinter::B8:         return QStringLiteral("B8");
    case QPrinter::B9:         return QStringLiteral("B9");
    case QPrinter::C5E:        return QStringLiteral("C5");     //Correct Translation?
    case QPrinter::Comm10E:    return QStringLiteral("Comm10"); //Correct Translation?
    case QPrinter::DLE:        return QStringLiteral("DL");     //Correct Translation?
    case QPrinter::Executive:  return QStringLiteral("Executive");
    case QPrinter::Folio:      return QStringLiteral("Folio");
    case QPrinter::Ledger:     return QStringLiteral("Ledger");
    case QPrinter::Legal:      return QStringLiteral("Legal");
    case QPrinter::Letter:     return QStringLiteral("Letter");
    case QPrinter::Tabloid:    return QStringLiteral("Tabloid");
    case QPrinter::Custom:     return QStringLiteral("Custom.%1x%2mm")
                                            .arg( printer.widthMM() )
                                            .arg( printer.heightMM() );
    default:                   return QString();
    }
}

// What about Upper and MultiPurpose?  And others in PPD???
QString FilePrinter::mediaPaperSource( QPrinter &printer )
{
    switch ( printer.paperSource() ) {
    case QPrinter::Auto:            return QString();
    case QPrinter::Cassette:        return QStringLiteral("Cassette");
    case QPrinter::Envelope:        return QStringLiteral("Envelope");
    case QPrinter::EnvelopeManual:  return QStringLiteral("EnvelopeManual");
    case QPrinter::FormSource:      return QStringLiteral("FormSource");
    case QPrinter::LargeCapacity:   return QStringLiteral("LargeCapacity");
    case QPrinter::LargeFormat:     return QStringLiteral("LargeFormat");
    case QPrinter::Lower:           return QStringLiteral("Lower");
    case QPrinter::MaxPageSource:   return QStringLiteral("MaxPageSource");
    case QPrinter::Middle:          return QStringLiteral("Middle");
    case QPrinter::Manual:          return QStringLiteral("Manual");
    case QPrinter::OnlyOne:         return QStringLiteral("OnlyOne");
    case QPrinter::Tractor:         return QStringLiteral("Tractor");
    case QPrinter::SmallFormat:     return QStringLiteral("SmallFormat");
    default:                        return QString();
    }
}

QStringList FilePrinter::optionOrientation( QPrinter &printer, QPrinter::Orientation documentOrientation )
{
    // portrait and landscape options rotate the document according to the document orientation
    // If we want to print a landscape document as one would expect it, we have to pass the
    // portrait option so that the document is not rotated additionally
    if ( printer.orientation() == documentOrientation ) {
        // the user wants the document printed as is
        return QStringList(QStringLiteral("-o")) << QStringLiteral("portrait");
    } else {
        // the user expects the document being rotated by 90 degrees
        return QStringList(QStringLiteral("-o")) << QStringLiteral("landscape");
    }
}

QStringList FilePrinter::optionDoubleSidedPrinting( QPrinter &printer )
{
    switch ( printer.duplex() ) {
    case QPrinter::DuplexNone:       return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=one-sided");
    case QPrinter::DuplexAuto:       if ( printer.orientation() == QPrinter::Landscape ) {
                                         return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-short-edge");
                                     } else {
                                         return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-long-edge");
                                     }
    case QPrinter::DuplexLongSide:   return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-long-edge");
    case QPrinter::DuplexShortSide:  return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-short-edge");
    default:                         return QStringList();  //Use printer default
    }
}

QStringList FilePrinter::optionPageOrder( QPrinter &printer )
{
    if ( printer.pageOrder() == QPrinter::LastPageFirst ) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("outputorder=reverse");
    }
    return QStringList(QStringLiteral("-o")) << QStringLiteral("outputorder=normal");
}

QStringList FilePrinter::optionCollateCopies( QPrinter &printer )
{
    if ( printer.collateCopies() ) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("Collate=True");
    }
    return QStringList(QStringLiteral("-o")) << QStringLiteral("Collate=False");
}

QStringList FilePrinter::optionPageMargins( QPrinter &printer )
{
    if (printer.printEngine()->property(QPrintEngine::PPK_PageMargins).isNull()) {
        return QStringList();
    } else {
        qreal l, t, r, b;
        printer.getPageMargins( &l, &t, &r, &b, QPrinter::Point );
        return QStringList(QStringLiteral("-o")) << QStringLiteral("page-left=%1").arg(l)
                       <<  QStringLiteral("-o")  << QStringLiteral("page-top=%1").arg(t)
                       <<  QStringLiteral("-o")  << QStringLiteral("page-right=%1").arg(r)
                       <<  QStringLiteral("-o")  << QStringLiteral("page-bottom=%1").arg(b) << QStringLiteral("-o") << QStringLiteral("fit-to-page");
    }
}

QStringList FilePrinter::optionCupsProperties( QPrinter &printer )
{
    QStringList dialogOptions = printer.printEngine()->property(QPrintEngine::PrintEnginePropertyKey(0xfe00)).toStringList();
    QStringList cupsOptions;

    for ( int i = 0; i < dialogOptions.count(); i = i + 2 ) {
        if ( dialogOptions[i+1].isEmpty() ) {
            cupsOptions << QStringLiteral("-o") << dialogOptions[i];
        } else {
            cupsOptions << QStringLiteral("-o") << dialogOptions[i] + QLatin1Char('=') + dialogOptions[i+1];
        }
    }

    return cupsOptions;
}

/* kate: replace-tabs on; indent-width 4; */
