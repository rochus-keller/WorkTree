#ifndef PDMLINKVIEWCTRL_H
#define PDMLINKVIEWCTRL_H

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
#include <Udb/UpdateInfo.h>
#include <Gui2/AutoMenu.h>

class QListWidget;
class QListWidgetItem;

namespace Wt
{
    class ObjectTitleFrame;
    class PdmItemView;

    class PdmLinkViewCtrl : public QObject
    {
        Q_OBJECT
    public:
        explicit PdmLinkViewCtrl(QWidget *parent = 0);

        static PdmLinkViewCtrl* create(QWidget* parent,Udb::Transaction *txn );
        void setObj( const Udb::Obj& );
        void clear();
        QWidget* getWidget() const;
        void addCommands( Gui2::AutoMenu* );
        void showLink( const Udb::Obj& );
        void setObserved( PdmItemView* );
    signals:
        void signalSelect( const Udb::Obj&, bool open );
    protected slots:
        void onDbUpdate( Udb::UpdateInfo info );
        void onClicked(QListWidgetItem*);
        void onDblClick(QListWidgetItem*);
        void onShowLink();
        void onShowPeer();
        void onDelete();
        void onTitleClick();
        void onSetLinkType();
        void onCopy();
    private:
        void addLink( const Udb::Obj&, bool inbound );
        void formatItem( QListWidgetItem*, const Udb::Obj&, bool inbound );
        void markObservedLinks();
        ObjectTitleFrame* d_title;
        QListWidget* d_list;
        PdmItemView* d_view;
    };
}

#endif // PDMLINKVIEWCTRL_H
