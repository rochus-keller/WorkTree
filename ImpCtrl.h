#ifndef WORKTREECTRL_H
#define WORKTREECTRL_H

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
    class ImpCtrl : public GenericCtrl
    {
        Q_OBJECT
    public:
        static const char* s_mimeImp;
        explicit ImpCtrl(QTreeView *tree, GenericMdl* mdl );
        static ImpCtrl* create(QWidget* parent, const Udb::Obj& root );
        void addCommands( Gui2::AutoMenu* );
        void writeMimeImp( QMimeData *data, const QList<Udb::Obj>& );
        QList<Udb::Obj> readMimeImp(const QMimeData *data, Udb::Obj& parent );
    public slots:
        void onAddTask();
        void onAddMilestone();
        void onAddEvent();
        void onAddAccomplishment();
        void onAddCriterion();
        void onConvertTask();
        void onConvertToCriterion();
        void onCopy();
        void onPaste();
        void onSetMsType();
        void onSetTaskType();
        void onEditAttrs();
        void onRecreateDiagrams();
    protected:
        void writeTo(const Udb::Obj & o, Stream::DataWriter &out) const;
        Udb::Obj readImpElement(Stream::DataReader & in, Udb::Obj &parent );
    };
}

#endif // WORKTREECTRL_H
