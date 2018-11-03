#ifndef OBSCTRL_H
#define OBSCTRL_H

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

namespace Wt
{
    class ObsCtrl : public GenericCtrl
    {
        Q_OBJECT
    public:
        explicit ObsCtrl(QTreeView *tree, GenericMdl* mdl );
        static ObsCtrl* create(QWidget* parent, const Udb::Obj& root );
        void addCommands( Gui2::AutoMenu* );
        bool edit( Udb::Obj& );
    public slots:
        void onAddOrgUnit();
        void onAddTypeRole();
        void onAddHuman();
        void onAddTypeResource();
        void onFind();
        void onEditSelected();
        void onDblClick(const QModelIndex&);
        void onAddNext();
    protected:
        void add( quint32 type );
    };
}

#endif // OBSCTRL_H