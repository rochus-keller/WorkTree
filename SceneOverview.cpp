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

#include "SceneOverview.h"
#include <QPainter>
#include <QtDebug>
#include <QScrollBar>
#include <QMouseEvent>
using namespace Wt;

SceneOverview::SceneOverview( QWidget* p ):QGraphicsView(p),d_subject( 0 ),d_mode(Idle)
{
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setInteractive( false );
	// TEST setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
	setBackgroundBrush( Qt::NoBrush );
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
}

void SceneOverview::resizeEvent ( QResizeEvent * e )
{
	onSceneRectChanged();
}

void SceneOverview::setObserved( QGraphicsView* s )
{
	if( d_subject != s )
	{
		if( s )
		{
			connect( s->horizontalScrollBar(), SIGNAL( rangeChanged ( int, int ) ), this, SLOT( onScrolled() ) );
			connect( s->horizontalScrollBar(), SIGNAL( valueChanged ( int ) ), this, SLOT( onScrolled() ) );
			connect( s->verticalScrollBar(), SIGNAL( rangeChanged ( int, int ) ), this, SLOT( onScrolled() ) );
			connect( s->verticalScrollBar(), SIGNAL( valueChanged ( int ) ), this, SLOT( onScrolled() ) );
			connect( s->scene(), SIGNAL( sceneRectChanged ( const QRectF & ) ), this, SLOT( onSceneRectChanged() ) );
		}
		if( d_subject )
		{
			disconnect( d_subject->horizontalScrollBar(), SIGNAL( rangeChanged ( int, int ) ), this, SLOT( onScrolled() ) );
			disconnect( d_subject->horizontalScrollBar(), SIGNAL( valueChanged ( int ) ), this, SLOT( onScrolled() ) );
			disconnect( d_subject->verticalScrollBar(), SIGNAL( rangeChanged ( int, int ) ), this, SLOT( onScrolled() ) );
			disconnect( d_subject->verticalScrollBar(), SIGNAL( valueChanged ( int ) ), this, SLOT( onScrolled() ) );
			disconnect( d_subject->scene(), SIGNAL( sceneRectChanged ( const QRectF & ) ), this, SLOT( onSceneRectChanged() ) );
		}
		d_subject = s;
		if( d_subject )
			QGraphicsView::setScene( d_subject->scene() );
		else
			QGraphicsView::setScene( 0 );
		onSceneRectChanged();
	}
}

void SceneOverview::onSceneRectChanged()
{
	if( scene() )
	{
		fitInView( scene()->sceneRect(), Qt::KeepAspectRatio );
		d_scene = QRectF( mapFromScene( scene()->sceneRect().topLeft() ), 
			mapFromScene( scene()->sceneRect().bottomRight() ) );
	}
}

void SceneOverview::paintEvent ( QPaintEvent * e )
{
	if( scene() )
	{
        const bool big = scene()->items().size() > 1000;
        setRenderHint( QPainter::Antialiasing, !big );
        setRenderHint( QPainter::TextAntialiasing, !big );

		QPainter p( viewport() );
		p.fillRect( viewport()->rect(), Qt::gray );
		p.fillRect( d_scene, Qt::white );
	}
	QGraphicsView::paintEvent( e );
	if( scene() )
	{
		QPainter p( viewport() );
		p.setPen( Qt::blue );
		p.drawRect( getSubjectRect() );
		if( d_mode == Select )
		{
			p.setPen( Qt::darkYellow );
			p.drawRect( d_rect.normalized().adjusted( 0, 0, -1, -1 ) );
		}
	}
}

QRectF SceneOverview::getSubjectRect() const
{
	if( d_subject == 0 || d_subject->scene() == 0 )
		return QRect();

	QRectF sr = d_subject->mapFromScene( d_subject->scene()->sceneRect() ).boundingRect();
	QRectF vr = d_subject->viewport()->rect();
	const qreal dx = d_scene.width() / sr.width();
	const qreal dy = d_scene.height() / sr.height();
	return QRectF( d_scene.x() - sr.x() * dx, d_scene.y() - sr.y() * dy,
		vr.width() * dx, vr.height() * dy );
}

void SceneOverview::onScrolled()
{
	viewport()->update();
}

void SceneOverview::mousePressEvent ( QMouseEvent * e )
{
	if( e->buttons() == Qt::LeftButton )
	{
		d_rect = QRect( e->pos(), QSize( 1, 1 ) );
		d_mode = PrepareSelect;
	}else
		QGraphicsView::mousePressEvent( e );
}

void SceneOverview::mouseMoveEvent ( QMouseEvent * e )
{
	if( d_mode == PrepareSelect )
	{
		QPoint pos = e->pos() - d_rect.topLeft();
		if( pos.manhattanLength() > 3 )
		{
			d_rect.setBottomRight( e->pos() );
			d_mode = Select;
			viewport()->update( d_rect.normalized() );
		}
	}else if( d_mode == Select )
	{
		viewport()->update( d_rect.normalized() );
		d_rect.setBottomRight( e->pos() );
		viewport()->update( d_rect.normalized() );
	}else
		QGraphicsView::mouseMoveEvent( e );
}

static void _scrollBy( QGraphicsView* view, int dx, int dy )
{
    QScrollBar* hBar = view->horizontalScrollBar();
    QScrollBar* vBar = view->verticalScrollBar();
    hBar->setValue(hBar->value() - dx );
    vBar->setValue(vBar->value() - dy );
}

void SceneOverview::mouseReleaseEvent ( QMouseEvent * e )
{
	if( d_mode == Select && d_subject )
	{
		viewport()->update( d_rect.normalized() );
		QRect r = d_rect.normalized();
		d_subject->fitInView( QRectF( mapToScene( r.topLeft() ), 
			mapToScene( r.bottomRight() ) ), Qt::KeepAspectRatio );
	}else if( d_mode == PrepareSelect && d_subject )
	{
		// Verschiebe den Mittelpunkt.
		QRectF sr = getSubjectRect();
		const qreal fx = d_subject->viewport()->rect().width() / sr.width();
		const qreal fy = d_subject->viewport()->rect().height() / sr.height();
		QPoint center;
		center.setX( sr.x() + sr.width() / 2.0 + 0.5 );
		center.setY( sr.y() + sr.height() / 2.0 + 0.5 );
		const int dx = ( center.x() - d_rect.x() ) * fx;
		const int dy = ( center.y() - d_rect.y() ) * fy;
		_scrollBy( d_subject, dx, dy );
	}else
		QGraphicsView::mouseReleaseEvent( e );
	d_mode = Idle;
}

void SceneOverview::mouseDoubleClickEvent ( QMouseEvent * e )
{
	if( e->button() == Qt::LeftButton && d_subject )
	{
		d_subject->setMatrix( QMatrix() );
		d_mode = PrepareSelect;
		mouseReleaseEvent( e );
	}
}
