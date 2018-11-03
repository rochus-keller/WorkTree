#ifndef WBSCTRL_H
#define WBSCTRL_H

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

#include "GenericCtrl.h"

class QMimeData;

namespace Wt
{
    class WbsCtrl : public GenericCtrl
    {
        Q_OBJECT
    public:
        static const char* s_mimeWbs;
        explicit WbsCtrl(QTreeView *tree, GenericMdl* mdl );
        static WbsCtrl* create(QWidget* parent, const Udb::Obj& root );
        void addCommands( Gui2::AutoMenu* );
        void writeMimeWbs( QMimeData *data, const QList<Udb::Obj>& );
        QList<Udb::Obj> readMimeWbs(const QMimeData *data, Udb::Obj& parent );
    public slots:
        void onAddDeliverable();
        void onAddWork();
        void onCopy();
        void onPaste();
    protected:
        void writeTo(const Udb::Obj & o, Stream::DataWriter &out) const;
        Udb::Obj readWbsElement(Stream::DataReader & in, Udb::Obj &parent );
    };
}

#endif // WBSCTRL_H
