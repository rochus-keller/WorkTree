#ifndef PDMLAYOUTER_H
#define PDMLAYOUTER_H

/*
* Copyright 2012-2018 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the WorkTree application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.info.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QtCore/QObject>
#include <QtCore/QLibrary>
#include <Udb/Obj.h>

namespace Wt
{
    struct _Gvc;

    class PdmLayouter : public QObject
    {
        Q_OBJECT
    public:
        explicit PdmLayouter(QObject *parent = 0);
        ~PdmLayouter();

        bool renderPdf( const QString& dotFile, const QString& pdfFile );
        bool layoutDiagram( const Udb::Obj&, bool ortho, bool topToBottom );
        bool loadLibs();
        bool loadFunctions();
        bool addLibraryPath( const QString& );
        bool prepareEngine( QWidget* );
        const QStringList& getErrors() const { return d_errors; }
    protected:
        QStringList d_errors;
        QLibrary d_libGvc;
        QLibrary d_libCgraph;
        _Gvc* d_ctx;
    };
}

#endif // PDMLAYOUTER_H
