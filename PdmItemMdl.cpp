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

#include "PdmItemMdl.h"
#include "PdmItems.h"
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <math.h>
#include <QtDebug>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include "PdmItemObj.h"
#include <Udb/Database.h>
#include <QPainter>
#include <QPrinter>
#include <QSvgGenerator>
#include "WtTypeDefs.h"
#include <Oln2/OutlineMdl.h>
#include <Oln2/OutlineItem.h>
#include <QTextDocument>
#include <QTextStream>
#include <Oln2/OutlineToHtml.h>
#include <QFile>
#include "WtTypeDefs.h"
using namespace Wt;

const float PdmItemMdl::s_cellWidth = PdmNode::s_boxWidth * 1.25;
const float PdmItemMdl::s_cellHeight = PdmNode::s_boxHeight * 1.25;
const char* PdmItemMdl::s_mimeEvent = "application/flowline/event-ref";

PdmItemMdl::PdmItemMdl( QObject *p ):
	QGraphicsScene(p),d_mode(Idle),d_tempLine(0),d_tempBox(0),d_lastHitItem(0),
    d_readOnly(false),d_toEnlarge(false)
{
	QDesktopWidget dw;
	setSceneRect( dw.screenGeometry() ); 
	// setItemIndexMethod( QGraphicsScene::NoIndex ); // RISK
}

void PdmItemMdl::fetchAttributes( PdmNode* i, const Udb::Obj& orig ) const
{
	if( i == 0 || orig.isNull() )
		return;
    Q_ASSERT( orig.getType() != TypePdmItem ); // Hier wird das Original erwartet
	i->setText( orig.getValue( AttrText ).toString() );
	i->setId( WtTypeDefs::formatObjectId( orig ) );
    i->setToolTip( WtTypeDefs::formatObjectTitle( orig ) );
    switch( i->type() )
    {
    case PdmNode::Task:
        i->setSubtasks( orig.getValue( AttrSubTMSCount ).getUInt32() > 0 );
        i->setCode( orig.getValue( AttrTaskType ).getUInt8() );
        i->setAlias( !d_doc.equals( orig.getParent() ) );
        i->setCritical( orig.getValue( AttrCriticalPath ).getBool() );
        break;
    case PdmNode::Milestone:
        i->setCode( orig.getValue( AttrMsType ).getUInt8() );
        i->setAlias( !d_doc.equals( orig.getParent() ) );
        i->setCritical( orig.getValue( AttrCriticalPath ).getBool() );
        break;
    }
}

QGraphicsItem* PdmItemMdl::fetchItemFromDb( const Udb::Obj& obj, bool links, bool vertices )
{
	PdmItemObj pdmItem = obj;
    Q_ASSERT( pdmItem.getType() == TypePdmItem );
    const Udb::Obj orig = pdmItem.getValueAsObj( AttrOrigObject );
    if( orig.isNull( true ) )
    {
        d_orphans.append( obj );
        return 0; // Orphan
    }
	const quint32 type = orig.getType();
	if( type == TypeTask && vertices )
	{
        if( d_cache.contains( orig.getOid() ) )
        {
            // Das kann vorkommen, wenn setDiagram vor commit aufgerufen wird.
            qDebug() << "fetchItemFromDb Task already in diagram:" << WtTypeDefs::formatObjectTitle(orig);
            return 0; // Ein Object kann nur genau einmal auf einem Diagramm vorhanden sein
        }
        PdmNode* i = new PdmNode( pdmItem.getOid(), orig.getOid(), PdmNode::Task );
		i->setPos( pdmItem.getPos() );
		fetchAttributes( i, orig );
		d_cache[pdmItem.getOid()] = i;
        d_cache[orig.getOid()] = i;
		addItem( i );
		return i;
	}else if( type == TypeMilestone && vertices )
	{
        if( d_cache.contains( orig.getOid() ) )
        {
            // Das kann vorkommen, wenn setDiagram vor commit aufgerufen wird.
            qDebug() << "fetchItemFromDb MS already in diagram:" << WtTypeDefs::formatObjectTitle(orig);
            return 0; // Ein Object kann nur genau einmal auf einem Diagramm vorhanden sein
        }
        PdmNode* i = new PdmNode( pdmItem.getOid(), orig.getOid(), PdmNode::Milestone );
		i->setPos( pdmItem.getPos() );
		fetchAttributes( i, orig );
		d_cache[pdmItem.getOid()] = i;
        d_cache[orig.getOid()] = i;
		addItem( i );
		return i;
    }else if( type == TypeLink && links )
    {
        if( d_cache.contains( orig.getOid() ) )
        {
            // Das kann vorkommen, wenn setDiagram vor commit aufgerufen wird.
            qDebug() << "fetchItemFromDb Link already in diagram:" << WtTypeDefs::formatObjectTitle(orig);
            return 0; // Ein Object kann nur genau einmal auf einem Diagramm vorhanden sein
        }
		PdmNode* start = dynamic_cast<PdmNode*>(
                             d_cache.value( orig.getValue( AttrPred ).getOid() ) );
		PdmNode* end = dynamic_cast<PdmNode*>(
                           d_cache.value( orig.getValue( AttrSucc ).getOid() ) );
		if( start != 0 && end != 0 )
		{
			QPolygonF nl = pdmItem.getNodeList();
			for( int j = 0; j < nl.size(); j++ )
			{
				PdmNode* n = new PdmNode(0,0,PdmNode::LinkHandle);
				n->setPos( nl[j] );
				addItem( n );
				LineSegment* s = addSegment( start, n );
                s->setToolTip( WtTypeDefs::formatObjectTitle( orig ) );
                s->setCritical( orig.getValue( AttrCriticalPath ).getBool() );
				start = n;
			}
			LineSegment* lastSegment = addSegment( start, end, pdmItem );
            lastSegment->setTypeCode(
                        WtTypeDefs::formatLinkType( orig.getValue( AttrLinkType ).getUInt8(), true ) );
            lastSegment->setToolTip( WtTypeDefs::formatObjectTitle( orig ) );
            lastSegment->setCritical( orig.getValue( AttrCriticalPath ).getBool() );
            return lastSegment;
		}else
            d_orphans.append( obj ); // Der Link existiert zwar, aber nicht auf diesem Diagramm
	}
	return 0;
}

void PdmItemMdl::setShowId( bool on ) 
{ 
	if( !d_doc.isNull() && !d_readOnly )
	{
		d_doc.setValue( AttrShowIds, Stream::DataCell().setBool( on ) );
		d_doc.commit();
    }
}

bool PdmItemMdl::isShowId() const
{
    if( d_doc.isNull() )
        return false;
    Stream::DataCell v = d_doc.getValue( AttrShowIds );
    if( v.isNull() )
        return true; // RISK
    else
        return v.getBool();
}

bool PdmItemMdl::isMarkAlias() const
{
    if( d_doc.isNull() )
        return false;
    Stream::DataCell v = d_doc.getValue( AttrMarkAlias );
    if( v.isNull() )
        return d_doc.getType() != TypePdmDiagram; // RISK
    else
        return v.getBool();
}

void PdmItemMdl::setMarkAlias(bool on)
{
    if( !d_doc.isNull() && !d_readOnly )
	{
		d_doc.setValue( AttrMarkAlias, Stream::DataCell().setBool( on ) );
		d_doc.commit();
    }
}

void PdmItemMdl::setDiagram( const Udb::Obj& doc )
{
	if( d_doc.equals( doc ) )
		return;
	if( !d_doc.isNull() )
		d_doc.getDb()->removeObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );
	clear();
	d_cache.clear();
	d_doc = doc;
	if( !d_doc.isNull() )
	{
		// Erzeuge zuerst die EpkItems ohne Handle und Segments
        Udb::Obj pdmItem = d_doc.getFirstObj();
		if( !pdmItem.isNull() ) do
		{
            if( pdmItem.getType() == TypePdmItem )
                fetchItemFromDb( pdmItem, false, true );
		}while( pdmItem.next() );

		// Erzeuge nun die Handle und Segments
        pdmItem = d_doc.getFirstObj();
		if( !pdmItem.isNull() ) do
		{
            if( pdmItem.getType() == TypePdmItem )
                fetchItemFromDb( pdmItem, true, false );
		}while( pdmItem.next() );

        if( !d_orphans.isEmpty() )
        {
            foreach( Udb::Obj o, d_orphans )
                o.erase();
            d_orphans.clear();
            d_doc.commit();
        }
		d_doc.getDb()->addObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ), false ); // neu synchron

        fitSceneRect();
        // qDebug() << "#Items" << items().size();
	}
}

void PdmItemMdl::keyPressEvent ( QKeyEvent * e )
{
	if( e->key() == Qt::Key_Escape && e->modifiers() == Qt::NoModifier && d_mode == AddingLink )
	{
		Q_ASSERT( d_tempLine != 0 );
        //removeItem( d_tempLine );
        delete d_tempLine;
		d_tempLine = 0;
		if( d_lastHitItem )
			removeLink( d_lastHitItem );
		d_lastHitItem = 0;
        d_startItem = 0;
		d_mode = Idle;
    }
}

void PdmItemMdl::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if( event->mimeData()->hasFormat(Udb::Obj::s_mimeObjectRefs ) )
    {
		// Dieser Umweg ist noetig, damit onDrop asynchron aufgerufen werden kann.
        // Bei synchronem Aufruf gibt es eine Exception in QMutex::lock bzw. QMetaObject::activate
        // Auf Windows blockiert QDrag::exec laut Dokumentation die Event Queue, was die Ursache sein
        // knnte, zumal ja signals auch synchron irgendwie via Events versendet werden.
        emit signalDrop( event->mimeData()->data( Udb::Obj::s_mimeObjectRefs ), event->scenePos() );
        event->acceptProposedAction();
    }else
        event->ignore();
}

void PdmItemMdl::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if( event->mimeData()->hasFormat(Udb::Obj::s_mimeObjectRefs ) )
        event->acceptProposedAction();
    else
        event->ignore();
}

void PdmItemMdl::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    // NOP
}

void PdmItemMdl::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    // NOP
}

void PdmItemMdl::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[], QWidget *widget)
{
    if( d_toEnlarge )
    {
        d_toEnlarge = false;
        enlargeSceneRect();
        //return;
    }
    QGraphicsScene::drawItems( painter, numItems, items, options, widget );
}

static inline bool _canAddHandle( const QList<QGraphicsItem *>& l )
{
	if( l.isEmpty() )
		return true;
	// Wenn sich darunter nur Handles befinden, knnen wir weitere draufsetzen
	for( int i = 0; i < l.size(); i++ )
	{
		switch( l[i]->type() )
		{
		case PdmNode::LinkHandle:
		case PdmNode::Link:
			break;
		default:
			return false;
		}
	}
	return true;
}

void PdmItemMdl::mousePressEvent(QGraphicsSceneMouseEvent * e)
{
    // Migrated
	d_startPos = e->scenePos();
	d_lastPos = d_startPos;
    if (e->button() != Qt::LeftButton)
        return;

	if( d_mode == AddingLink )
	{
		Q_ASSERT( d_tempLine != 0 );
		Q_ASSERT( d_lastHitItem != 0 );
        Q_ASSERT( d_startItem != 0 );
        QList<QGraphicsItem *> endItems = items( d_tempLine->line().p2() );
        if( endItems.count() && endItems.first() == d_tempLine )
            endItems.removeFirst();

		PdmNode* to = 0;
		if( !endItems.isEmpty() )
			to = dynamic_cast<PdmNode*>( endItems.first() );
		if( _canAddHandle( endItems ) || to == 0 )
		{
			// Erzeuge Punkt
			PdmNode* n = addHandle();
			addSegment( d_lastHitItem, n );
			d_lastHitItem = n;
			d_tempLine->setLine( QLineF( n->pos(), d_startPos ) );
		}else
		{
			if( to && d_startItem != to && canEndLink( to ) )
			{
                Q_ASSERT( !d_doc.isNull() );
                Udb::Obj predObj = d_doc.getObject( d_startItem->getOrigOid() );
                Udb::Obj succObj = d_doc.getObject( to->getOrigOid() );
                LineSegment* f = addSegment( d_lastHitItem, to );
                QPolygonF path = getNodeList( f );
                removeLink( f );
                Q_ASSERT( d_tempLine != 0 );
				removeItem( d_tempLine );
				delete d_tempLine;
				d_tempLine = 0;
                d_lastHitItem = 0;
                d_startItem = 0;
                d_mode = Idle;
                emit signalCreateLink( predObj, succObj, path );
            }
		}
	}else
	{
		Q_ASSERT( d_mode == Idle );
		QGraphicsItem* i = itemAt( d_startPos );
		if( i == 0 )
		{
			QGraphicsScene::mousePressEvent(e);
		}else
		{
			if( e->modifiers() == Qt::ShiftModifier )
			{
				// Es geht um Selektionen
				if( i->isSelected() )
					i->setSelected( false );
				else
					i->setSelected( true );
				if( !d_readOnly )
					d_mode = PrepareMove;
			}else
			{
				if( !i->isSelected() )
				{
					clearSelection();
					i->setSelected( true );
				}
				if( !d_readOnly )
				{
					d_startItem = dynamic_cast<PdmNode*>( i );
                    d_lastHitItem = d_startItem;
					if( d_startItem && e->modifiers() == Qt::ControlModifier && canStartLink( d_startItem ) )
					{
						// Beginne zeichnen eines ControlFlows
						d_mode = AddingLink;
						d_tempLine = new QGraphicsLineItem( QLineF(d_startItem->scenePos(), e->scenePos() ) );
						d_tempLine->setZValue( 1000 );
						d_tempLine->setPen(QPen(Qt::gray, 1));
						addItem(d_tempLine);
					}else if( d_startItem && canScale( d_startItem ) && e->modifiers() == Qt::AltModifier )
						d_mode = Scaling; // Figuren skalieren
					else if( e->modifiers() == Qt::NoModifier )
						d_mode = PrepareMove;
				}
			}
		}
	}
}

bool PdmItemMdl::canScale( PdmNode* i ) const
{
	return false;
}

void PdmItemMdl::mouseMoveEvent(QGraphicsSceneMouseEvent * e)
{
    // migrated
    if( d_mode == AddingLink ) 
	{
		if( d_tempLine != 0 )
			d_tempLine->setLine( QLineF( d_tempLine->line().p1(), e->scenePos() ) );
	}else if( d_mode == Moving )
	{
		Q_ASSERT( d_tempBox != 0 );
		d_lastPos = e->scenePos();
		d_tempBox->setPos( rastered( d_lastPos - d_startPos ) );
	}else if( d_mode == Scaling )
	{
		Q_ASSERT( d_lastHitItem != 0 );
		QPointF p = e->scenePos() - e->lastScenePos();
		d_lastHitItem->adjustSize( p.x(), p.y() );
	}else if( d_mode == PrepareMove )
	{
		QPoint pos = QPointF( e->scenePos() - d_startPos ).toPoint();
		if( pos.manhattanLength() > 5 )
		{
			QPainterPath path;
			QList<QGraphicsItem*> l = selectedItems();
			for( int i = 0; i < l.size(); i++ )
			{
				if( dynamic_cast<PdmNode*>( l[i] ) )
				{
					QPainterPath s = l[i]->shape();
					QPointF off = l[i]->pos();
					for( int j = 0; j < s.elementCount(); j++ )
					{
						QPointF pos = s.elementAt( j );
						s.setElementPositionAt( j, pos.x() + off.x(), pos.y() + off.y() );
					}
					path.addPath( s );
				}
			}
			d_lastPos = e->scenePos();
			d_tempBox = new QGraphicsPathItem( path );
			d_tempBox->setPos( rastered( d_lastPos - d_startPos ) );
			d_tempBox->setZValue( 1000 );
			d_tempBox->setPen(QPen(Qt::gray, 1));
			addItem(d_tempBox);
			d_mode = Moving;
		}
	}else
	{
		QGraphicsScene::mouseMoveEvent( e );
	}
}

void PdmItemMdl::enlargeSceneRect()
{
    // migrated
	QRectF sr = sceneRect();
	const QRectF br = itemsBoundingRect();
	QDesktopWidget dw;
	const QRect screen = dw.screenGeometry();

	if( br.top() < sr.top() )
		sr.adjust( 0, - screen.height(), 0, 0 );
	if( br.bottom() > sr.bottom() )
		sr.adjust( 0, 0, 0, screen.height() );
	if( br.left() < sr.left() )
		sr.adjust( - screen.width(), 0, 0, 0 );
	if( br.right() > sr.right() )
		sr.adjust( 0, 0, screen.width(), 0 );

    setSceneRect( sr );
}

void PdmItemMdl::fitSceneRect(bool forceFit)
{
    QRectF r = itemsBoundingRect().adjusted(
        -PdmNode::s_boxWidth * 0.5, -PdmNode::s_boxHeight * 0.5,
        PdmNode::s_boxWidth * 0.5, PdmNode::s_boxHeight * 0.5 );
    if( !forceFit )
    {
        qreal diff = sceneRect().width() - r.width();
        if( diff > 0.0 )
            r.adjust( -diff * 0.5, 0, diff * 0.5, 0 );
        diff = sceneRect().height() - r.height();
        if( r.height() < sceneRect().height() )
            r.adjust( 0, -diff * 0.5, 0, diff * 0.5 );
    }
    setSceneRect( r );
}

void PdmItemMdl::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
    // migrated
    if( d_mode == AddingLink ) 
	{
		// NOP
    }else if( d_mode == Moving )
	{
		Q_ASSERT( d_tempBox != 0 );
		QPointF off = d_lastPos - d_startPos;
		QList<QGraphicsItem*> l = selectedItems();
		for( int i = 0; i < l.size(); i++ )
		{
			if( PdmNode* ei = dynamic_cast<PdmNode*>( l[i] ) )
			{
				rasteredMoveBy( ei, off.x(), off.y() );
			}
		}
		if( !d_doc.isNull() )
			d_doc.commit();
		delete d_tempBox;
		enlargeSceneRect();
		d_tempBox = 0;
		d_mode = Idle;
	}else if( d_mode == Scaling )
	{
		d_lastHitItem = 0;
        d_startItem = 0;
		d_mode = Idle;
	}else
	{
		d_mode = Idle;
		QGraphicsScene::mouseReleaseEvent( e );
	}
}

QPointF PdmItemMdl::toCellPos( const QPointF& pos ) const
{
	return QPointF( s_cellWidth * ::floor( pos.x() / s_cellWidth ), 
			s_cellHeight * ::floor( pos.y() / s_cellHeight ) );
}

QPolygonF PdmItemMdl::getNodeList( QGraphicsItem* i ) const
{
	if( i == 0 )
		return QPolygonF();
	else if( i->type() == PdmNode::LinkHandle )
	{
		return getNodeList( static_cast<PdmNode*>( i )->getFirstInSegment() );
		// Die Points werden immer ausgehend von einem ControlFlow ermittelt. Wenn das Objekt also
		// ein Handle ist, wird der Flow verwendet, der in den Handle reinmndet. Da ein Handle nie alleine
		// stehen kann und immer genau ein Flow einmndet, drfte getInFlow() nie null zurckgeben.
	}else if( i->type() == PdmNode::Link )
	{
		LineSegment* f = static_cast<LineSegment*>( i );
		if( f->getStartItem()->type() == PdmNode::LinkHandle )
			// f ist noch nicht das erste Segment im Flow.
			return getNodeList( f->getStartItem() );
		else
		{
			// f ist nun das erste Segment im Flow. Gehe nun bis zum Ende und sammle die Handle-Punkte ein.
			QPolygonF res;
			while( f && f->getEndItem()->type() == PdmNode::LinkHandle )
			{
				PdmNode* n = f->getEndItem();
				res.append( n->pos() );
				f = n->getFirstOutSegment();
			}
			return res;
		}
	}//else
	return QPolygonF();
}

PdmNode* PdmItemMdl::addHandle()
{
	if( d_readOnly )
		return 0;
	// Handle hat keine Oid. Stattdessen liegt die Oid auf dem letzten Link-Segment vor dem Target.
	PdmNode* i = new PdmNode(0,0,PdmNode::LinkHandle);
	i->setPos( rastered( d_startPos ) );
	addItem( i );
	return i;
}

LineSegment* PdmItemMdl::addSegment( PdmNode* from, PdmNode* to, const Udb::Obj &pdmItem )
{
    // migrated
    const quint32 origID = ( !pdmItem.isNull() )?pdmItem.getValue( AttrOrigObject ).getOid():0;
    LineSegment* segment = new LineSegment( pdmItem.getOid(), origID );
    from->addLink(segment, true);
    to->addLink(segment, false);
    addItem(segment);
    segment->updatePosition();
	if( !pdmItem.isNull() )
    {
        d_cache[ segment->getItemOid() ] = segment;
        d_cache[ origID ] = segment;
    }
	return segment;
}

QPointF PdmItemMdl::rastered( const QPointF& p )
{
	QPointF res = p;
	res.setX( ::floor( res.x() / PdmNode::s_rasterX + 0.5 ) * PdmNode::s_rasterX );
	res.setY( ::floor( res.y() / PdmNode::s_rasterY + 0.5 ) * PdmNode::s_rasterY );
    return res;
}

void PdmItemMdl::removeFromCache(QGraphicsItem * i)
{
    if( LineSegment* link = dynamic_cast<LineSegment*>( i ) )
    {
        d_cache.remove( link->getOrigOid() );
        d_cache.remove( link->getItemOid() );
    }else if( PdmNode* item = dynamic_cast<PdmNode*>( i ) )
    {
        d_cache.remove( item->getOrigOid() );
        d_cache.remove( item->getItemOid() );
    }
}

void PdmItemMdl::rasteredMoveBy( PdmNode* i, qreal dx, qreal dy )
{
    // migrated
	QPointF pos = i->pos() + QPointF( dx, dy );
	i->setPos( rastered( pos ) );
	if( !d_doc.isNull() )
	{
		if( i->type() == PdmNode::LinkHandle )
		{
			LineSegment* f = i->getLastSegment();
			Q_ASSERT( f != 0 && f->getItemOid() != 0 );
			PdmItemObj o = d_doc.getObject( f->getItemOid() );
			o.setNodeList( getNodeList( f ) );
		}else
		{
			Q_ASSERT( i->getItemOid() != 0 );
			PdmItemObj o = d_doc.getObject( i->getItemOid() );
			o.setPos( i->pos() );
		}
	}
}

bool PdmItemMdl::canStartLink( PdmNode* i ) const
{
	if( i->type() == PdmNode::Task  || i->type() == PdmNode::Milestone )
	{
		return true;
	}else
		return false;
}

bool PdmItemMdl::canEndLink( PdmNode* i ) const
{
	if( i->type() == PdmNode::Task  || i->type() == PdmNode::Milestone )
	{
        // TODO: Zirkulrreferenzen verhindern; ev. auch doppelte Links zwischen zwei Items
		return true;
	}else
		return false;
}

static void _recursiveDelete( LineSegment* segment )
{
    PdmNode* item = segment->getEndItem();
    if( item && item->type() == PdmNode::LinkHandle )
    {
        foreach( LineSegment* nextSegment, item->getLinks() )
        {
            if( nextSegment != segment )
            {
                _recursiveDelete( nextSegment );
                break;
            }
        }
        delete item;
    }
    delete segment;
}

void PdmItemMdl::removeLink( QGraphicsItem* linkOrHandle )
{
	if( linkOrHandle->type() == PdmNode::Link )
	{
		LineSegment* link = static_cast<LineSegment*>( linkOrHandle )->getFirstSegment();
        _recursiveDelete( link );
	}else if( linkOrHandle->type() == PdmNode::LinkHandle )
	{
		PdmNode* handle = static_cast<PdmNode*>( linkOrHandle );
        removeLink( handle->getFirstSegment() );
	}
}

void PdmItemMdl::removeHandle( PdmNode *handle )
{
	// Loesche die Kante davor und verbinde die nachfolgende mit dem Handle davor.
    LineSegment* prev = handle->getFirstInSegment();
    LineSegment* next = handle->getFirstOutSegment();
	LineSegment* last = handle->getLastSegment(); // Hier muesste der OID sein
    PdmNode* start = prev->getStartItem();
    start->removeLink( prev );
    start->addLink( next, true );
    next->updatePosition();
    delete prev;
    delete handle;
    Q_ASSERT( !d_doc.isNull() );
    if( last->getItemOid() )
    {
        PdmItemObj o = d_doc.getObject( last->getItemOid() );
        o.setNodeList( getNodeList( start ) );
    }
}

bool PdmItemMdl::insertHandle()
{
    // migrated
	if( d_readOnly )
		return false;
	if( selectedItems().isEmpty() || selectedItems().first()->type() != PdmNode::Link )
		return false;
	LineSegment* f = static_cast<LineSegment*>( selectedItems().first() );
	PdmNode* start = f->getStartItem();
	start->removeLink( f );
	PdmNode* n = addHandle();
	n->addLink( f, true );
	f->updatePosition();
	addSegment( start, n );
	LineSegment* last = n->getLastSegment(); // Hier muesste der OID sein
	if( !d_doc.isNull() && last->getItemOid() )
	{
		PdmItemObj o = d_doc.getObject( last->getItemOid() );
		o.setNodeList( getNodeList( start ) );
	}
	clearSelection();
	n->setSelected(true);
	n->update();
	return true;
}

void PdmItemMdl::removeTaskMilestone( QGraphicsItem* i )
{
    if( i->type() == PdmNode::Milestone || i->type() == PdmNode::Task )
	{
        PdmNode* item = static_cast<PdmNode*>( i );
		// Lsche zuerst alle Links
		while( !item->getLinks().isEmpty() )
		{
			removeLink( item->getLinks().first() );
		}
		delete item;
	}
}

void PdmItemMdl::setItemText( QGraphicsItem* i, const QString& str )
{
    // migriert
	if( d_readOnly )
		return;
	if( PdmNode* ei = dynamic_cast<PdmNode*>( i ) )
	{
		ei->update();
        Q_ASSERT( !d_doc.isNull() );
        Q_ASSERT( ei->getItemOid() != 0 );
        Q_ASSERT( ei->getOrigOid() != 0 );
        Udb::Obj orig = d_doc.getObject( ei->getOrigOid() );
        if( !orig.isNull(true) )
        {
            // DB WRITE
            orig.setString( AttrText, str );
            orig.setTimeStamp( AttrModifiedOn );
            orig.commit();
        }
	}
}

void PdmItemMdl::onDbUpdate( Udb::UpdateInfo info )
{
	Q_ASSERT( !d_doc.isNull() );
	switch( info.d_kind )
	{
	case Udb::UpdateInfo::TypeChanged:
        if( info.d_name == TypeTask || info.d_name == TypeMilestone )
        {
            QGraphicsItem* i = d_cache.value( info.d_id );
            if( i!= 0 )
            {
                switch( i->type() )
                {
                case PdmNode::Task:
                case PdmNode::Milestone:
                    {
                        PdmNode* item = static_cast<PdmNode*>( i );
                        switch( info.d_name )
                        {
                        case TypeTask:
                            item->setType( PdmNode::Task );
                            break;
                        case TypeMilestone:
                            item->setType( PdmNode::Milestone );
                            break;
                        }
                        Udb::Obj o = d_doc.getObject( item->getOrigOid() );
                        fetchAttributes( item, o );
                    }
                    break;
                }
            }
        }
		break;
	case Udb::UpdateInfo::ValueChanged:
		if( info.d_name == AttrText || info.d_name == AttrInternalId
                || info.d_name == AttrCustomId )
		{
            QGraphicsItem* gi = d_cache.value( info.d_id );
            PdmNode* pi = dynamic_cast<PdmNode*>( gi );
            LineSegment* ls = dynamic_cast<LineSegment*>( gi );
            if( pi && pi->getOrigOid() == info.d_id )
            {
				Udb::Obj o = d_doc.getObject( info.d_id );
                fetchAttributes( pi, o );
				pi->update();
            }else if( ls && ls->getOrigOid() == info.d_id )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                ls->setToolTip( WtTypeDefs::formatObjectTitle( o ) );
                ls->update();
            }
        }else if( info.d_name == AttrLinkType )
        {
            LineSegment* ls = dynamic_cast<LineSegment*>( d_cache.value( info.d_id ) );
            if( ls && ls->getOrigOid() == info.d_id )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                ls->setTypeCode( WtTypeDefs::formatLinkType( o.getValue( AttrLinkType ).getUInt8(), true ) );
                ls->update();
            }
        }else if( info.d_name == AttrSubTMSCount )
        {
            PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) );
            if( pi )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                pi->setSubtasks( o.getValue( AttrSubTMSCount ).getUInt32() > 0 );
                pi->update();
            }
        }else if( info.d_name == AttrTaskType )
        {
            PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) );
            if( pi && pi->type() == PdmNode::Task )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                pi->setCode( o.getValue( AttrTaskType ).getUInt8() );
                pi->update();
            }
        }else if( info.d_name == AttrMsType )
        {
            PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) );
            if( pi && pi->type() == PdmNode::Milestone )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                pi->setCode( o.getValue( AttrMsType ).getUInt8() );
                pi->update();
            }
        }else if( info.d_name == AttrPosX || info.d_name == AttrPosY )
        {
            // Unntig, da setDiagram nach layout
//            PdmItem* pi = dynamic_cast<PdmItem*>( d_cache.value( info.d_id ) );
//            if( pi )
//            {
//                PdmItemObj o = d_doc.getObject( info.d_id );
//                pi->setPos( o.getPos() );
//                pi->update();
//            }
        }else if( info.d_name == AttrPointList )
        {
            // Unntig, da setDiagram nach layout
        }else if( info.d_name == AttrCriticalPath )
        {
            if( PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) ) )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                pi->setCritical( o.getValue( AttrCriticalPath ).getBool() );
                pi->update();
            }else if( LineSegment* ls = dynamic_cast<LineSegment*>( d_cache.value( info.d_id ) ) )
            {
                Udb::Obj o = d_doc.getObject( info.d_id );
                const bool isCritical = o.getValue( AttrCriticalPath ).getBool();
                QList<LineSegment*> list = ls->getChain();
                foreach( LineSegment* l, list )
                {
                    l->setCritical( isCritical );
                    l->update();
                }
            }
        }
        break;
    case Udb::UpdateInfo::ObjectErased:
        {
            QGraphicsItem* i = d_cache.value( info.d_id );
            if( i!= 0 )
            {
                // TODO: wird hier Item aus Cache entfernt?
                switch( i->type() )
                {
                case PdmNode::Link:
                case PdmNode::LinkHandle:
                    // Wenn Orig zuerst gelscht wird ist das nicht tragisch, da Item beim nchsten
                    // ffnen als Orphan erkannt und gelscht wird.
                    removeLink( i );
                    break;
                case PdmNode::Task:
                case PdmNode::Milestone:
                    // Wenn Orig zuerst gelscht wird ist das nicht tragisch, da Item beim nchsten
                    // ffnen als Orphan erkannt und gelscht wird.
                    removeTaskMilestone( i );
                    break;
                }
            }
        }
        break;
    case Udb::UpdateInfo::Aggregated:
        if( info.d_parent == d_doc.getOid() )
        {
            PdmItemObj pdmItem = d_doc.getObject( info.d_id );
            if( pdmItem.getType() == TypePdmItem )
            {
                fetchItemFromDb( pdmItem, true, true );
                d_toEnlarge = true;
            }else
            {
                Q_ASSERT( pdmItem.getType() != TypePdmItem );
                PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) );
                if( pi && pi->getOrigOid() == info.d_id )
                {
                    pi->setAlias( false );
                    pi->update();
                }
            }
        }
        break;
    case Udb::UpdateInfo::Deaggregated:
        if( info.d_parent == d_doc.getOid() )
        {
            PdmNode* pi = dynamic_cast<PdmNode*>( d_cache.value( info.d_id ) );
            if( pi && pi->getOrigOid() == info.d_id )
            {
                pi->setAlias( true );
                pi->update();
            }
        }
        break;
	}
}

Udb::Obj PdmItemMdl::getSingleSelection() const
{
    // migriert
	if( selectedItems().size() != 1 || d_doc.isNull() )
		return Udb::Obj();
    QList<Udb::Obj> res = getMultiSelection();
    if( !res.isEmpty() )
        return res.first();
    else
        return Udb::Obj();
}

QList<Udb::Obj> PdmItemMdl::getMultiSelection(bool task, bool ms,
                                              bool link, bool handle) const
{
    QList<Udb::Obj> res;
    QList<QGraphicsItem*> sel = selectedItems();
    foreach( QGraphicsItem* i, sel )
    {
        switch( i->type() )
        {
        case PdmNode::Task:
            if( task )
                res.append( d_doc.getObject( static_cast<PdmNode*>( i )->getItemOid() ) );
            break;
        case PdmNode::Milestone:
            if( ms )
                res.append( d_doc.getObject( static_cast<PdmNode*>( i )->getItemOid() ) );
            break;
        case PdmNode::Link:
            if( link )
            {
                LineSegment* f = static_cast<LineSegment*>( i );
                f = f->getLastSegment();
                Udb::Obj link = d_doc.getObject( f->getItemOid() );
                if( !res.contains( link ) )
                    res.append( link );
            }
            break;
        case PdmNode::LinkHandle:
            if( handle )
            {
                PdmNode* n = static_cast<PdmNode*>( i );
                LineSegment* f = n->getLastSegment();
                Udb::Obj link = d_doc.getObject( f->getItemOid() );
                if( !res.contains( link ) )
                    res.append( link );
            }
            break;
        default:
            Q_ASSERT( false );
        }
    }
    return res;
}

QGraphicsItem* PdmItemMdl::selectObject( const Udb::Obj& o, bool clearSel )
{
    // migriert
	QGraphicsItem* i = d_cache.value( o.getOid() );
	if( i != 0 )
    {
        if( clearSel )
            clearSelection();
		i->setSelected( true );
        if( LineSegment* ls = dynamic_cast<LineSegment*>(i) )
        {
            foreach( LineSegment* l, ls->getChain() )
                l->setSelected( true );
        }
    }
    return i;
}

void PdmItemMdl::selectObjects(const QList<Udb::Obj> &obs, bool clearSel)
{
    bool first = clearSel;
    foreach( Udb::Obj o, obs )
    {
        selectObject( o, first );
        first = false;
    }
}

void PdmItemMdl::removeSelectedItems()
{
    // NOTE: Diese Methode ist eine Ausnahme, da ich keine Lust habe, Internas der PdmItems nach PdmCtrl
    // zu exportieren. Daher wird hier gelscht.
    QList<QGraphicsItem*> sel = selectedItems();
    QList<Udb::Obj> toRemoveObjs;
    QList<PdmNode*> toRemoveHandles;
    foreach( QGraphicsItem * i, sel )
	{
		switch( i->type() )
		{
		case PdmNode::Link:
            {
                LineSegment* s = static_cast<LineSegment*>( i );
                s = s->getLastSegment();
                Udb::Obj o = d_doc.getObject( s->getItemOid() );
                if( !toRemoveObjs.contains( o ) )
                    toRemoveObjs.append( o );
                // Links sind auf mehr als ein Segment abgebildet
            }
			break;
		case PdmNode::LinkHandle:
            toRemoveHandles.append( static_cast<PdmNode*>( i ) );
			break;
		case PdmNode::Task:
		case PdmNode::Milestone:
            {
                PdmNode* p = static_cast<PdmNode*>( i );
                Udb::Obj o = d_doc.getObject( p->getItemOid() );
                Q_ASSERT( !toRemoveObjs.contains( o ) ); // Darf nicht vorkommen
                toRemoveObjs.append( o );
                // Sorge dafür, dass auch die zu- und wegführenden Links ordentlich gelöscht werden
                foreach( LineSegment* s, p->getLinks() )
                {
                    s = s->getLastSegment();
                    o = d_doc.getObject( s->getItemOid() );
					if( !toRemoveObjs.contains( o ) )
						toRemoveObjs.append( o );
                }
            }
			break;
		default:
			Q_ASSERT( false ); // RISK
		}
	}
    foreach( PdmNode* h, toRemoveHandles )
    {
        Udb::Obj o = d_doc.getObject( h->getLastSegment()->getItemOid() );
        if( !toRemoveObjs.contains( o ) ) // Das Handle nur entfernen, wenn nicht gleich der Link gelscht wird
            removeHandle( h );
    }
    foreach( Udb::Obj o, toRemoveObjs )
        o.erase();
    d_doc.commit();
}

bool PdmItemMdl::canStartLink() const
{
	if( d_readOnly )
		return false;
	if( d_mode == Idle )
	{
		QGraphicsItem* i = itemAt( d_startPos );
		if( i != 0 )
		{
			PdmNode* ei = dynamic_cast<PdmNode*>( i );
			if( ei && canStartLink( ei ) )
				return true;
		}
	}
	return false;
}

void PdmItemMdl::startLink()
{
	if( d_readOnly )
		return;
	if( d_mode == Idle )
	{
		QGraphicsItem* i = itemAt( d_startPos );
		if( i != 0 )
		{
			if( !i->isSelected() )
			{
				clearSelection();
				i->setSelected( true );
			}
			d_startItem = dynamic_cast<PdmNode*>( i );
            d_lastHitItem = d_startItem;
			if( d_startItem && canStartLink( d_startItem ) )
			{
				// Wie in mousePressEvent
				d_mode = AddingLink;
				d_tempLine = new QGraphicsLineItem( QLineF(d_startItem->scenePos(), d_startPos ) );
				d_tempLine->setZValue( 1000 );
				d_tempLine->setPen(QPen(Qt::gray, 1));
				addItem(d_tempLine);
			}
		}
	}
}

QPointF PdmItemMdl::getStart(bool rastered) const
{
    if( rastered )
        return this->rastered( d_startPos );
    else
        return d_startPos;
}

// To migrate

void PdmItemMdl::exportPng( const QString& path )
{
	clearSelection();
	QBrush back = backgroundBrush();
	setBackgroundBrush( Qt::white );
	QRectF b = itemsBoundingRect().adjusted( -PdmNode::s_boxWidth * 0.5, -PdmNode::s_boxHeight * 0.5,
		PdmNode::s_boxWidth * 0.5, PdmNode::s_boxHeight * 0.5 );
	QImage img( b.size().toSize(), QImage::Format_RGB32 );
	QPainter painter( &img );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
	render( &painter, QRectF( QPointF(0.0,0.0), b.size() ), b );
	if( !path.isEmpty() )
		img.save( path, "PNG" );
	else
		QApplication::clipboard()->setImage( img );
	setBackgroundBrush( back );
}

static inline bool _hasOutline( const Udb::Obj& o, bool followAlias = false )
{
	if( o.isNull() )
		return false;
	Udb::Obj ali;
	if( followAlias )
		ali = o.getValueAsObj( Oln::OutlineItem::AttrAlias );
	if( ali.isNull() )
		ali = o;
	Udb::Obj s = ali.getFirstObj();
	if( !s.isNull() ) do
	{
		if( s.getType() == Oln::OutlineItem::TID )
			return true;
	}while( s.next() );
	return false;
}

void PdmItemMdl::exportPdf( const QString& path, bool withDetails )
{
	clearSelection();
	QBrush back = backgroundBrush();
	setBackgroundBrush( Qt::white );
	QRectF b = itemsBoundingRect().adjusted( -PdmNode::s_boxWidth * 0.5, -PdmNode::s_boxHeight * 0.5,
		PdmNode::s_boxWidth * 0.5, PdmNode::s_boxHeight * 0.5 );
	QPrinter prn(QPrinter::PrinterResolution);
	prn.setPaperSize(QPrinter::A4);
	if( b.width() > b.height() )
		prn.setOrientation( QPrinter::Landscape );
	prn.setOutputFormat( QPrinter::PdfFormat );
	prn.setOutputFileName( path );
	QPainter painter( &prn );
	render( &painter, QRectF(), b );
	setBackgroundBrush( back );
}

static QString _coded( QString str )
{
	// Ansonten Qt::escape, aber quot wird dort nicht escaped
	str.replace( QChar('"'), "&quot;" );
	return str;
}

void PdmItemMdl::exportHtml( const QString& path, bool withPng )
{
	const qreal off = 10.0;

	if( d_doc.isNull() )
		return;
	QImage img;
	QSet< quint32 > imagemap;
	QRectF bound;
	if( withPng )
	{
		clearSelection();
		QHash<quint32,QGraphicsItem*>::const_iterator i;
		for( i = d_cache.begin(); i != d_cache.end(); ++i )
		{
			if( _hasOutline( d_doc.getObject( i.key() ), true ) )
			{
				i.value()->setSelected( true );
				imagemap.insert( i.key() );
			}
		}
		QBrush back = backgroundBrush();
		setBackgroundBrush( Qt::white );
		bound = itemsBoundingRect();
		bound.adjust( -off, -off, off, off );
		img = QImage( bound.size().toSize(), QImage::Format_RGB32 );
		QPainter painter( &img );
		painter.fillRect( img.rect(), Qt::white ); 
		painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
		render( &painter, QRectF( QPointF(0,0), bound.size() ), bound );
		setBackgroundBrush( back );
	}

	QMap<quint64,Udb::Obj> order;
	order.insert( d_doc.getOid(), d_doc ); // RISK
	Udb::Obj sub = d_doc.getFirstObj();
	if( !sub.isNull() ) do
	{
		const quint32 type = sub.getType(); 
		if( type == TypeMilestone || type == TypeTask )
			order[sub.getOid()] = sub;
	}while( sub.next() );
	QFile f( path );
	f.open( QIODevice::WriteOnly );
	QTextStream out( &f );
	out.setCodec( "latin-1" );
	out << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n";
	out << "<html><META http-equiv=\"Content-Type\" content=\"text/html; charset=Latin-1\">\n";
	QString title = Qt::escape( d_doc.getString( AttrText ) );
    if( isShowId() )
	{
		const QString id = d_doc.getString( AttrInternalId );
		if( !id.isEmpty() ) 
			title = QString( "%1: %2" ).arg( id ).arg( title );
	}
	out << QString( "<head><title>%1</title></head>" ).arg( title );
	out << "<body>\n";
	if( withPng )
	{
		// width=100% funktioniert nicht mit map
		out << "<img usemap=\"#map1\" src=\"data:image/png;base64,";
		Stream::DataCell v;
		v.setImage( img );
		out << v.getArr().toBase64();
		out << "\"";
		out << " >\n";
		out << "<map name=\"map1\">\n";
		QHash<quint32,QGraphicsItem*>::const_iterator i;
		for( i = d_cache.begin(); i != d_cache.end(); ++i )
		{		
			if( i.value()->type() == PdmNode::Task ||
				i.value()->type() == PdmNode::Milestone )
			{
				QRectF b = i.value()->sceneBoundingRect();
				b.moveTo( b.x() - bound.x(), b.y() - bound.y());
				QRect bb = b.toRect();
				out << "<area ";
				Udb::Obj o = d_doc.getObject( i.key() );
				if( o.hasValue( Oln::OutlineItem::AttrAlias ) )
					o = o.getValueAsObj( Oln::OutlineItem::AttrAlias );
				Q_ASSERT( !o.isNull() );
				if( imagemap.contains( i.key() ) )
					out << "href=\"#" << o.getOid() << "\"";
				out << " shape=\"rect\" coords=\"";
				out << QString("%1,%2,%3,%4").arg( bb.left() ).arg( bb.top() ).arg( bb.right() ).arg( bb.bottom() );
				out << "\" title=\"";
				out << _coded( o.getString( AttrText ) );
				out << "\" />\n";
			}
		}
		out << "</map>\n";
		out << "</img>";
	}
	// NOTE: Der Rckweg - vom Titel in Diagramm - funktioniert nicht. Browser zeigt immer obere linke Ecke.
	out << "<table border=0 cellspacing=0 align=\"left\">";
	QMap<quint64,Udb::Obj>::const_iterator i;
	for( i = order.begin(); i != order.end(); ++i )
	{
		if( _hasOutline( i.value() ) )
		{
			title = Qt::escape( i.value().getString( AttrText ) );
            if( isShowId() )
			{
				const QString id = i.value().getString( AttrInternalId );
				if( !id.isEmpty() )
					title = QString( "%1: %2" ).arg( id ).arg( title );
			}
			out << "<tr><td><a name=\"" << i.value().getOid() << "\"><h3>" << title << "</h3></a></td>\n";
			out << "<tr><td>";
			Oln::OutlineToHtml writer;
			writer.writeTo( out, i.value(), QString(), true );
			out << "</td>\n";
		}
	}
    out << "</table></body></html>";
}

void PdmItemMdl::exportSvg(const QString &path )
{
    clearSelection();
	QBrush back = backgroundBrush();
	setBackgroundBrush( Qt::white );
	QRectF b = itemsBoundingRect().adjusted( -PdmNode::s_boxWidth * 0.5, -PdmNode::s_boxHeight * 0.5,
		PdmNode::s_boxWidth * 0.5, PdmNode::s_boxHeight * 0.5 );
	QSvgGenerator prn;
	prn.setFileName( path );
	QPainter painter( &prn );
	render( &painter, QRectF(), b );
	setBackgroundBrush( back );
}

static bool _isProcFuncEvt( quint32 type )
{
	return 	type == TypeTask ||
			type == TypeMilestone;
}
