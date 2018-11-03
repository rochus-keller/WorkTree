#ifndef __Wt_PdmItems__
#define __Wt_PdmItems__

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

#include <QAbstractGraphicsShapeItem>

namespace Wt
{
	class LineSegment;

	class PdmNode : public QGraphicsItem
	{
        // Repräsentiert Tasks und Milestones; Überbegriff von Node und Line ist Item
	public:
		static const float s_penWidth;
		static const float s_selPenWidth;
		static const float s_radius;
		static const float s_boxWidth;
		static const float s_boxHeight;
		static const float s_boxInset;
		static const float s_circleDiameter;
		static const float s_textMargin;
        static const float s_rasterX;
        static const float s_rasterY;

		enum Type { Task = UserType + 1, Milestone, Link, LinkHandle };
        PdmNode( quint32 item, quint32 orig, int type ):
            d_itemOid(item),d_origOid(orig),d_alias(false),d_hasSubtasks(false),d_type(type),
            d_code(0),d_critical(false) { setFlags(ItemIsSelectable ); }
        ~PdmNode();

		QString getText() const { return d_text; }
		QString getId() const { return d_id; }
		quint32 getItemOid() const { return d_itemOid; }
        quint32 getOrigOid() const { return d_origOid; }
		bool isAlias() const { return d_alias; }
		void setAlias( bool on ) { d_alias = on; }
        void setCritical( bool on ) { d_critical = on; }
        bool isCritical() const { return d_critical; }
        void setSubtasks( bool on ) { d_hasSubtasks = on; }
        bool hasSubtasks() const { return d_hasSubtasks; }
		bool hasItemText() const { return type() == Task || type() == Milestone; }
        void setType( int type );

        // Interface für Model
		void addLink(LineSegment*, bool start);
		void removeLink(LineSegment*);
		void setText( const QString& t ) { d_text = t; }
		void setId( const QString& t ) { d_id = t; }
        void setCode( quint8 c ) { d_code = c; }
		const QList<LineSegment*>& getLinks() const { return d_links; }

        // Folgendes für LinkHandles
		LineSegment* getFirstInSegment() const;
		LineSegment* getFirstOutSegment() const;
        LineSegment* getFirstSegment() const;
		LineSegment* getLastSegment() const;

		// overrides
		virtual QRectF boundingRect () const;
		virtual QPainterPath shape () const;
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
		virtual QPolygonF toPolygon() const;
		virtual void adjustSize( qreal dx, qreal dy ) {}
        virtual int type () const { return d_type; }
	protected:
        void paintMilestone( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        void paintTask( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        void paintHandle( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        QRectF handleShapeRect() const;
		// overrides
		QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	private:
        friend class LineSegment;
		QList<LineSegment*> d_links;
		quint32 d_itemOid; // PdmItem
        quint32 d_origOid; // OrigObject
        QString d_text;
		QString d_id;  // OrigObject
        int d_type;
        // Text stammt ebenfalls von OrigObject
		bool d_alias; // true: OrigObject.getParent() != PdmItem.getParent()
        bool d_hasSubtasks;
        bool d_critical;
        quint8 d_code; // TaskType, MsType
	};

	class LineSegment : public QGraphicsLineItem
	{
	public:
		LineSegment( quint32 item, quint32 orig);
        ~LineSegment();

		PdmNode* getStartItem() const { return d_start; }
		PdmNode* getUltimateStartItem() const;
		LineSegment* getFirstSegment() const;
		PdmNode* getEndItem() const { return d_end; }
		PdmNode* getUltimateEndItem() const;
		LineSegment* getLastSegment() const;
        QList<LineSegment*> getChain() const;
		quint32 getItemOid() const { return d_itemOid; }
        quint32 getOrigOid() const { return d_origOid; }
		void selectAllSegments();
        QString getTypeCode() const { return d_code; } // FS, FF, SF, SS
        void setTypeCode( const QString& s ) { d_code = s; }
        void setCritical( bool on ) { d_critical = on; }
        bool isCritical() const { return d_critical; }

		// Overrides
		QRectF boundingRect() const;
		QPainterPath shape() const;
		virtual int type () const { return PdmNode::Link; }
	public slots:
		void updatePosition();

	protected:
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
				   QWidget *widget = 0);

	private:
		//friend class PdmItemMdl;
        friend class PdmNode; // Wegen Destructor
		PdmNode* d_start;
		PdmNode* d_end;
		QPolygonF d_arrowHead;
        QString d_code;
		quint32 d_itemOid; // PdmItem
        quint32 d_origOid; // OrigObject
        bool d_critical;
	};
}

#endif
