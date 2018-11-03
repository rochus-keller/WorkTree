#ifndef __Wt_PdmCtrl__
#define __Wt_PdmCtrl__

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
#include <QtCore/QSet>
#include <Udb/Obj.h>
#include <Gui2/AutoMenu.h>

class QMimeData;

namespace Wt
{
	class PdmItemView;
    class PdmItemMdl;

	class PdmCtrl : public QObject
	{
		Q_OBJECT
	public:
        static const char* s_mimePdmItems;

		PdmCtrl( PdmItemView*, PdmItemMdl* );
		PdmItemView* getView() const;
		static PdmCtrl* create( QWidget* parent, const Udb::Obj& doc );
        void addCommands( Gui2::AutoMenu * pop );
        Udb::Obj getSingleSelection() const; // Gibt Task/Milestone/Link zurück, nicht PdmItem
        static void writeItems(QMimeData *data, const QList<Udb::Obj>& );
        QList<Udb::Obj> readItems(const QMimeData *data ); // gibt PdmItem zurück!
        const Udb::Obj& getDiagram() const;
        bool focusOn( const Udb::Obj& );
    signals:
        void signalSelectionChanged();
	public slots:
		void onAddTask();
		void onAddMilestone();
		void onSelectAll();
        void onRemoveItems();
		void onInsertHandle();
		void onExportToFile();
		void onExportToClipboard();
		void onSelectRightward();
		void onSelectUpward();
		void onSelectLeftward();
		void onSelectDownward();
		void onLink();
		void onCopy();
		void onPaste();
		void onCut();
		void onShowId();
        void onSetLinkType();
        void onSelectHiddenSchedObjs();
        void onSelectHiddenLinks();
        void onSetTaskType();
        void onMarkAlias();
        void onDeleteItems();
        void onLayout();
        void onSetMsType();
        void onToggleTaskMs();
        void onEditAttrs();
        void onExtendDiagram();
        void onShowShortestPath();
        void onRemoveAllAliasses();
    protected slots:
        void onDblClick( const QPoint& );
        void onCreateLink( const Udb::Obj& pred, const Udb::Obj& succ, const QPolygonF& path );
        void onSelectionChanged();
        void onDrop( QByteArray, QPointF );
    protected:
        void onAddItem( quint32 type );
        void pasteItemRefs(const QMimeData *data, const QPointF &where );
        void doLayout( bool ortho );
        static void adjustTo( const QList<Udb::Obj>&, const QPointF& to ); // erwartet PdmItems
    private:
        PdmItemMdl* d_mdl;
	};
}

#endif
