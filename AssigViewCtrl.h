#ifndef ASSIGVIEWCTRL_H
#define ASSIGVIEWCTRL_H

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

#include <QObject>
#include <Udb/Obj.h>
#include <Gui2/AutoMenu.h>

class QTreeView;
class QModelIndex;
class QMimeData;

namespace Wt
{
    class AssigViewMdl;
    class ObjectTitleFrame;
    class CheckLabel;

    class AssigViewCtrl : public QObject
    {
        Q_OBJECT
    public:
        static const char* s_mimeAssignments;

        explicit AssigViewCtrl(QWidget *parent );
        QWidget* getWidget() const;

        void setObj( const Udb::Obj& root );
        static AssigViewCtrl* create(QWidget* parent, Udb::Transaction *txn );
        void addCommands( Gui2::AutoMenu* );
        static void writeItems(QMimeData *data, const QList<Udb::Obj>& );
        QList<Udb::Obj> readItems(const QMimeData *data );
        void showAssig( const Udb::Obj& );
    signals:
        void signalSelect( const Udb::Obj&, bool open );
    public slots:
        void onAddRasciAssig();
        void onSetRasciAssig();
        void onDelete();
        void onPaste();
        void onCopy();
        void onCut();
    protected slots:
        void onDblClick(const QModelIndex& );
        void onClick(const QModelIndex& );
        void onSelectionChanged();
        void onTitleClick();
        void onLinkTo( const QList<Udb::Obj> &what, const QModelIndex & toParent, int beforeRow );
        void onDbUpdate(Udb::UpdateInfo info);
    private:
        int selectRasciRole( int = 0 );
        AssigViewMdl* d_mdl;
        ObjectTitleFrame* d_title;
        QTreeView* d_tree;
        CheckLabel* d_pin;
    };
}

#endif // ASSIGVIEWCTRL_H
