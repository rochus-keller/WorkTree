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

#include "PdmItems.h"
#include <QPainter>
#include <QGraphicsTextItem>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <math.h>
#include "PdmItemMdl.h"
#include "WtTypeDefs.h"
using namespace Wt;

const float PdmNode::s_penWidth = 1.0;
const float PdmNode::s_selPenWidth = 3.0;
const float PdmNode::s_radius = 5.0;
const float PdmNode::s_boxWidth = 100.0;
const float PdmNode::s_boxHeight = 60.0; // 72, 18, 12
const float PdmNode::s_boxInset = 12; // Ergebnis sollte ganzzahliger Teiler sein von s_boxHeight für bestes Aussehen
const float PdmNode::s_circleDiameter = PdmNode::s_boxHeight / 2.0;
const float PdmNode::s_textMargin = 2.5;
const float PdmNode::s_rasterX = 0.125 * s_boxWidth;
const float PdmNode::s_rasterY = 0.125 * s_boxHeight;

static inline PdmItemMdl* _mdl( const QGraphicsItem* i )
{
	PdmItemMdl* mdl = dynamic_cast<PdmItemMdl*>( i->scene() );
	Q_ASSERT( mdl );
	return mdl;
}

static inline bool _showId( const QGraphicsItem* i )
{
	PdmItemMdl* m = _mdl( i );
	return m && m->isShowId();
}

static inline bool _markAlias( const QGraphicsItem* i )
{
	PdmItemMdl* m = _mdl( i );
	return m && m->isMarkAlias();
}

PdmNode::~PdmNode()
{
    // Sorge dafür, dass Querverweise gelöscht werden, da ansonten Scene::clear crasht
    foreach( LineSegment* s, d_links )
    {
        if( s->d_start == this )
            s->d_start = 0;
        if( s->d_end == this )
            s->d_end = 0;
    }
    if( d_itemOid || d_origOid )
    {
        PdmItemMdl* mdl = dynamic_cast<PdmItemMdl*>( scene() );
        if( mdl )
            mdl->removeFromCache( this );
    }
}

void PdmNode::setType(int type)
{
    prepareGeometryChange();
    d_type = type;
}

LineSegment* PdmNode::getFirstInSegment() const
{
	for( int i = 0; i < d_links.size(); i++ )
		if( d_links[i]->getEndItem() == this )
			return d_links[i];
	return 0;
}

LineSegment* PdmNode::getFirstOutSegment() const
{
	for( int i = 0; i < d_links.size(); i++ )
		if( d_links[i]->getStartItem() == this )
			return d_links[i];
	return 0;
}

QRectF PdmNode::boundingRect() const
{
    if( type() == LinkHandle )
    {
        const float pwh = s_penWidth / 2.0;
        return handleShapeRect().adjusted( -pwh, -pwh, pwh, pwh );
    }else
    {
        // Wird für Redraw verwendet. Sollte tendenziell grösser sein als die Shape.
        const qreal pwh = s_penWidth / 2.0;
        const qreal bwh = s_boxWidth / 2.0;
        const qreal bhh = s_boxHeight / 2.0;
        QRectF r( - bwh - pwh, - bhh - pwh, s_boxWidth + pwh, s_boxHeight + pwh );
        if( _showId( this ) )
        {
            QFontMetricsF f( scene()->font() );
            r.adjust( 0, - f.height(), 0, 0 );
        }
        return r;
    }
}

QPolygonF PdmNode::toPolygon() const
{
    if( d_type == Milestone )
    {
        QPolygonF p;
        p << QPointF( 0, s_boxHeight * 0.5 ) << // links mitte
             QPointF( s_boxInset, 0 ) << // links oben
             QPointF( s_boxWidth - s_boxInset, 0 ) << // rechts oben
             QPointF( s_boxWidth, s_boxHeight * 0.5 ) << // rechts mitte
             QPointF( s_boxWidth - s_boxInset, s_boxHeight ) << // rechts unten
             QPointF( s_boxInset, s_boxHeight ) << // links unten
             QPointF( 0, s_boxHeight * 0.5 ); // links mitte
        p.translate( -s_boxWidth * 0.5, -s_boxHeight * 0.5 );
        return p;
    }else if( d_type == LinkHandle )
    {
        return shape().toFillPolygon();
    }else
    {
        // Wird für Shape verwendet und damit für Hit-Test etc.
        // Muss daher die genauen Abmessungen repräsentieren.
        const qreal pwh = s_penWidth / 2.0;
        const qreal bwh = s_boxWidth / 2.0;
        const qreal bhh = s_boxHeight / 2.0;
        return QRectF( - bwh - pwh, - bhh - pwh, s_boxWidth + pwh, s_boxHeight + pwh );
    }
}

QPainterPath PdmNode::shape () const
{
	QPainterPath p;
    if( type() == LinkHandle )
        p.addRect( handleShapeRect() );
    else
        p.addPolygon( toPolygon() );
	return p;
}

void PdmNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    switch( d_type )
    {
    case Task:
        paintTask( painter, option, widget );
        break;
    case Milestone:
        paintMilestone( painter, option, widget );
        break;
    case LinkHandle:
        paintHandle( painter, option, widget );
        break;
    default:
        qWarning( "Don't know how to paint PdmItem type=%d", d_type );
    }
}

void PdmNode::removeLink(LineSegment *arrow)
{
    if( arrow == 0 )
        return;
    if( arrow->d_start == this )
        arrow->d_start = 0;
    else if( arrow->d_end == this )
        arrow->d_end = 0;
    else
        Q_ASSERT( false );
    d_links.removeAll( arrow );
}

void PdmNode::addLink(LineSegment *arrow, bool start)
{
    if( start )
    {
        if( arrow->d_start )
            arrow->d_start->removeLink( arrow );
        Q_ASSERT( arrow->d_start == 0 );
        arrow->d_start = this;
    }else
    {
        if( arrow->d_end )
            arrow->d_end->removeLink( arrow );
        Q_ASSERT( arrow->d_end == 0 );
        arrow->d_end = this;
    }
    d_links.append(arrow);
}

QVariant PdmNode::itemChange(GraphicsItemChange change,
                     const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (LineSegment *arrow, d_links) {
            arrow->updatePosition();
        }
    }

    return value;
}

LineSegment* PdmNode::getFirstSegment() const
{
	LineSegment* prev = getFirstInSegment();
	if( prev )
		return prev->getFirstSegment();
	else
		return 0;
}

LineSegment* PdmNode::getLastSegment() const
{
	LineSegment* next = getFirstOutSegment();
	if( next )
		return next->getLastSegment();
	else
		return 0;
}


static const QColor s_lightBrown( 221, 208, 155 );
static const QColor s_darkBrown( 140, 120, 33 );
static const QColor s_lightBlue( 202, 215, 236 );
static const QColor s_darkBlue( 119, 152, 206 );


void PdmNode::paintTask( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
	QColor borderClr( 71, 179, 0 );
	QColor fillClr( 150, 255, 0 ); // TaskType_Discrete
    QColor textClr = Qt::black;

    switch( d_code )
    {
    case TaskType_LOE:
        fillClr = s_lightBrown;
		borderClr = s_darkBrown;
        break;
    case TaskType_SVT:
        fillClr = s_lightBlue;
		borderClr = s_darkBlue;
        break;
    }
    if( isAlias() && _markAlias( this ) ) // TODO
	{
		borderClr = Qt::lightGray;
        // fillClr = fillClr.lighter(130);
        fillClr = borderClr.lighter(130);
        textClr = Qt::gray;
	}
    if( isCritical() )
        borderClr = Qt::red;

    painter->setPen( QPen( borderClr, isSelected()?s_selPenWidth:s_penWidth ) );
	painter->setBrush( fillClr );
	QRectF r( -s_boxWidth * 0.5, -s_boxHeight * 0.5, s_boxWidth, s_boxHeight );
	painter->drawRoundedRect( r, s_radius, s_radius );
	if( d_hasSubtasks )
	{
		painter->setPen( QPen( borderClr, s_penWidth ) );
		painter->drawRoundedRect( r.adjusted( s_textMargin, s_textMargin, -s_textMargin, -s_textMargin ), 
			s_radius - s_textMargin, s_radius - s_textMargin );
	}
	painter->setPen( textClr );
	painter->setFont( _mdl( this )->getChartFont() );
	r.adjust( s_textMargin, s_textMargin, -s_textMargin, -s_textMargin );
	int flags = Qt::TextWordWrap;
	QString text = getText();
	QFontMetricsF fm( painter->font(), painter->device() );
	const QRectF br = fm.boundingRect( r, Qt::TextWordWrap, text );
	if( br.height() > r.height() )
		flags |= Qt::AlignTop;
	else
		flags |= Qt::AlignVCenter;
	if( br.width() > r.width() )
		flags |= Qt::AlignLeft;
	else
		flags |= Qt::AlignHCenter;
	painter->drawText( r, flags, text );
	if( br.height() - fm.descent() > r.height() )
	{
        // Punkte nach überschüssigem Text
		painter->setPen( QPen( textClr, 2.0 * s_penWidth, Qt::DotLine, Qt::RoundCap ) );
		painter->drawLine( r.bottomRight() - QPointF( s_boxInset, 0 ), r.bottomRight() );
	}
	if( _showId( this ) )
	{
		painter->setPen( Qt::blue );
		QFont f = painter->font();
		f.setBold( true );
		painter->setFont( f );
        QFontMetricsF fm( f );
        QRectF br = fm.boundingRect( getId() );
        br.moveBottomLeft( QPointF( -s_boxWidth * 0.5, -s_boxHeight * 0.5 -s_textMargin -1.0 ) );
        painter->fillRect( br, Qt::white );
		painter->drawText( br, getId() );
	}
}

void PdmNode::paintMilestone( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
	//painter->setRenderHint( QPainter::Antialiasing );
	QColor borderClr = QColor( 179, 98, 5 );
	QColor fillClr = QColor( 255, 178, 7 );
    QColor textClr = Qt::black;
    if( isAlias() && _markAlias( this ) )
	{
		borderClr = Qt::lightGray;
        // fillClr = fillClr.lighter(130);
        fillClr = borderClr.lighter(130);
        textClr = Qt::gray;
	}
    if( isCritical() )
        borderClr = Qt::red;

	painter->setPen( QPen( borderClr, isSelected()?s_selPenWidth:s_penWidth ) );
	painter->setBrush( fillClr );

	QPolygonF p = toPolygon();
	painter->drawPolygon( p );

    QRectF textRect = p.boundingRect().adjusted( s_boxInset, s_textMargin,
                                                       -s_boxInset, -s_textMargin );
    painter->setPen( borderClr );
    switch( d_code )
    {
    case MsType_TaskStart:
        painter->drawLine( p[1], p[5] );
        textRect.adjust( 1, 0, 0, 0 );
        break;
    case MsType_TaskFinish:
        painter->drawLine( p[2], p[4] );
        textRect.adjust( 0, 0, -1, 0 );
        break;
    case MsType_ProjStart:
        painter->setBrush( borderClr );
        painter->drawPolygon( QPolygonF() << p[1] << p[5] << p[6] );
        textRect.adjust( 1, 0, 0, 0 );
        break;
    case MsType_ProjFinish:
        painter->setBrush( borderClr );
        painter->drawPolygon( QPolygonF() << p[2] << p[3] << p[4] );
        textRect.adjust( 0, 0, -1, 0 );
        break;
    }

    painter->setPen( textClr );
	painter->setFont( _mdl( this )->getChartFont() );

    int textFlags = Qt::TextWordWrap;
	QFontMetricsF fm( painter->font(), painter->device() );
	QString text = getText();
	const QRectF textBounding = fm.boundingRect( textRect, Qt::TextWordWrap, text );
	if( textBounding.height() > textRect.height() )
		textFlags |= Qt::AlignTop;
	else
		textFlags |= Qt::AlignVCenter;
	if( textBounding.width() > textRect.width() )
		textFlags |= Qt::AlignLeft;
	else
		textFlags |= Qt::AlignHCenter;
	painter->drawText( textRect, textFlags, text );
	if( textBounding.height() - fm.descent() > textRect.height() )
	{
        // Punkte hinter Text
		painter->setPen( QPen( textClr, 2.0 * s_penWidth, Qt::DotLine, Qt::RoundCap ) );
		painter->drawLine( textRect.bottomRight() - QPointF( s_boxInset, 0 ), textRect.bottomRight() );
	}
	if( _showId( this ) )
	{
		painter->setPen( Qt::blue );
		QFont f = painter->font();
		f.setBold( true );
		painter->setFont( f );
        QFontMetricsF fm( f );
        QRectF br = fm.boundingRect( getId() );
        br.moveBottomLeft( QPointF( -s_boxWidth * 0.5 + s_boxInset, -s_boxHeight * 0.5 -s_textMargin -1.0 ) );
        painter->fillRect( br, Qt::white );
		painter->drawText( br, getId() );
	}
}

void PdmNode::paintHandle( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
	if( isSelected() )
	{
		painter->setPen( QPen( Qt::black, s_penWidth ) );
        if( isCritical() )
            painter->setBrush( Qt::red );
        else
            painter->setBrush( Qt::black );
		painter->drawRect( handleShapeRect() );
	}
}

QRectF PdmNode::handleShapeRect() const
{
	qreal r = s_radius * 0.75; // * ()?1.0:0.5;
	return QRectF( -r, -r, 2.0 * r, 2.0 * r );
}

static const qreal Pi = 3.14;

LineSegment::LineSegment( quint32 item, quint32 orig)
    :d_itemOid(item),d_origOid(orig),d_start(0), d_end(0),d_critical(false)
{
    setZValue(-1000.0);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setPen(QPen(Qt::black, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // Gezeichnet wird nur mit einem Punkt, aber dank den 5 kann man besser selektieren.
}

LineSegment::~LineSegment()
{
    if( d_start )
        d_start->removeLink( this );
    Q_ASSERT( d_start == 0 );
    if( d_end )
        d_end->removeLink( this );
    Q_ASSERT( d_end == 0 );
    if( d_itemOid || d_origOid )
    {
        PdmItemMdl* mdl = dynamic_cast<PdmItemMdl*>( scene() );
        if( mdl )
            mdl->removeFromCache( this );
    }
}

QRectF LineSegment::boundingRect() const
{
	return shape().boundingRect();
}

QPainterPath LineSegment::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();
    path.addPolygon(d_arrowHead);
    return path;
}

void LineSegment::updatePosition()
{
    QLineF line(mapFromItem(d_start, 0, 0), mapFromItem(d_end, 0, 0));
    setLine(line);
}

void LineSegment::paint(QPainter *painter, const QStyleOptionGraphicsItem *,QWidget *)
{
	if (d_start->collidesWithItem(d_end))
		return;

	QPen myPen = pen();
	if( isSelected() )
		myPen.setWidth( PdmNode::s_selPenWidth );
	else
		myPen.setWidth( PdmNode::s_penWidth );
    if( isCritical() )
        myPen.setColor( Qt::red );
	const qreal arrowSize = 10;
	painter->setPen(myPen);
    if( isCritical() )
        painter->setBrush(Qt::red);
    else
        painter->setBrush(Qt::black);

	const QLineF centerLine(d_start->pos(), d_end->pos());
	const QPolygonF endPolygon = d_end->toPolygon();
	QPointF p1 = endPolygon.first() + d_end->pos();
	QPointF p2;
	QPointF intersectPoint;
	QLineF polyLine;
	for (int i = 1; i < endPolygon.count(); ++i) 
	{
		p2 = endPolygon.at(i) + d_end->pos();
		polyLine = QLineF(p1, p2);
		QLineF::IntersectType intersectType =
			polyLine.intersect(centerLine, &intersectPoint);
		if (intersectType == QLineF::BoundedIntersection)
			break;
		p1 = p2;
	}

	setLine(QLineF(intersectPoint, d_start->pos()));

	double angle = ::acos(line().dx() / line().length());
	if (line().dy() >= 0)
		angle = (Pi * 2) - angle;

	const QPointF arrowP1 = line().p1() + QPointF(sin(angle + Pi / 3) * arrowSize,
		cos(angle + Pi / 3) * arrowSize);
	const QPointF arrowP2 = line().p1() + QPointF(sin(angle + Pi - Pi / 3) * arrowSize,
		cos(angle + Pi - Pi / 3) * arrowSize);

	d_arrowHead.clear();
	d_arrowHead << line().p1() << arrowP1 << arrowP2;
	painter->drawLine(QLineF(d_end->pos(), d_start->pos()));
	if( d_end->type() != PdmNode::LinkHandle )
        painter->drawPolygon(d_arrowHead);

    if( _showId( this ) )
	{
		painter->setPen( Qt::blue );
		QFont f = painter->font();
		f.setBold( true );
		painter->setFont( f );
        QFontMetricsF fm( f );
        const QLineF l1 = QLineF(d_start->pos(), (d_start->type()==PdmNode::LinkHandle)?
                                     intersectPoint : d_end->pos() );
        const QPointF p1 = l1.pointAt( 0.5 );

        QRectF br = fm.boundingRect( getTypeCode() );
        br.moveCenter( p1 );
        painter->fillRect( br, Qt::white );
        painter->drawText( br, getTypeCode() );
	}
}

PdmNode* LineSegment::getUltimateStartItem() const
{
	LineSegment* f = getFirstSegment();
	Q_ASSERT( f != 0 );
	return f->getStartItem();
}

PdmNode* LineSegment::getUltimateEndItem() const
{
	LineSegment* f = getLastSegment();
	Q_ASSERT( f != 0 );
	return f->getEndItem();
}

LineSegment* LineSegment::getLastSegment() const
{
	if( getEndItem() && getEndItem()->type() == PdmNode::LinkHandle )
	{
		return getEndItem()->getLastSegment();
	}else
        return const_cast<LineSegment*>( this );
}

QList<LineSegment*> LineSegment::getChain() const
{
    QList<LineSegment*> res;
    res.append( const_cast<LineSegment*>( this ) );
    const LineSegment* cur = this;
    while( cur->getStartItem() && cur->getStartItem()->type() == PdmNode::LinkHandle )
    {
        cur = cur->getStartItem()->getFirstInSegment();
        res.append( const_cast<LineSegment*>( cur ) );
    }
    return res;
}

LineSegment* LineSegment::getFirstSegment() const
{
	if( getStartItem() && getStartItem()->type() == PdmNode::LinkHandle )
	{
		return getStartItem()->getFirstSegment();
	}else
		return const_cast<LineSegment*>( this );
}

void LineSegment::selectAllSegments()
{
	LineSegment* cf = getLastSegment();
	while( cf )
	{
		cf->setSelected( true );
		if( cf->getStartItem()->type() == PdmNode::LinkHandle )
		{
			PdmNode* handle = cf->getStartItem();
			handle->setSelected( true );
			cf = handle->getFirstInSegment();
		}else
			cf = 0;
	}
}



