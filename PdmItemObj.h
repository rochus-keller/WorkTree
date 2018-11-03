#ifndef __Wt_PdmItemObj__
#define __Wt_PdmItemObj__

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

#include <Udb/Obj.h>
#include <QPolygonF>
#include "PdmLayouter.h"

class QProgressDialog;

namespace Wt
{
	class PdmItemObj : public Udb::Obj
	{
	public:
        static PdmLayouter s_layouter;
		PdmItemObj(){}
		PdmItemObj( const Udb::Obj& o ):Obj( o ) {}
		void setPos( const QPointF& );
		QPointF getPos() const;
		QRectF getBoundingRect() const;
		void setNodeList( const QPolygonF& ); // Methode für ControlFlow
		QPolygonF getNodeList() const;
        bool hasNodeList() const;
        void setOrig( const Udb::Obj& orig );
        Udb::Obj getOrig() const;
        static bool createDiagram( Udb::Obj diagram, bool recreate, bool layout, bool recursive,
                                   quint8 levels, bool toSucc, bool toPred, QProgressDialog* pg );
        static PdmItemObj createLink( Udb::Obj diagram, Udb::Obj pred, const Udb::Obj& succ );
        static PdmItemObj createItemObj( Udb::Obj diagram, Udb::Obj other, const QPointF& = QPointF() );
        static void removeAllItems( const Obj &diagram );
        static QList<Udb::Obj> findHiddenLinks( const Udb::Obj& diagram, QList<Udb::Obj> startset );
        static QSet<Udb::Obj> findHiddenSchedObjs( const Udb::Obj& diagram );
        static QSet<Udb::OID> findAllItemOrigOids( const Udb::Obj& diagram );
        static QList<PdmItemObj> findAllAliasses( const Udb::Obj& diagram );
        static QList<Udb::Obj> findAllItemOrigObjs( const Udb::Obj& diagram, bool schedObjs, bool links );
        static QList<Udb::Obj> findExtendedSchedObjs( QList<Udb::Obj> startset, quint8 levels, bool toSucc, bool toPred, bool onlyCritical );
        static QList<Udb::Obj> readItems(Udb::Obj diagram, Stream::DataReader& );
        static QList<Udb::Obj> writeItems( const QList<Udb::Obj> & items, Stream::DataWriter& );
        static QList<Udb::Obj> addItemsToDiagram( Udb::Obj diagram, const QList<Udb::Obj>& items, QPointF where );
        static QList<Udb::Obj> addItemLinksToDiagram( Udb::Obj diagram, const QList<Udb::Obj>& items );
	};
}

#endif
