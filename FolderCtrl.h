#ifndef FOLDERCTRL_H
#define FOLDERCTRL_H

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
#include "GenericMdl.h"

namespace Wt
{
    class FolderCtrl : public GenericCtrl
    {
        Q_OBJECT
    public:
        explicit FolderCtrl(QTreeView *tree, GenericMdl* mdl );
        static FolderCtrl* create(QWidget* parent, const Udb::Obj& root );
        void addCommands( Gui2::AutoMenu* );
    public slots:
        void onAddFolder();
        void onAddPdmDiagram();
        void onAddOutline();
		void onAddScript();
    };

    class FolderMdl : public GenericMdl
    {
    public:
        explicit FolderMdl(QObject *parent = 0);
    protected:
        virtual bool isSupportedType( quint32 );
    };
}

#endif // FOLDERCTRL_H
