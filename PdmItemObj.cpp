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

#include "PdmItemObj.h"
#include <Stream/DataReader.h>
#include <Stream/DataWriter.h>
#include <QtGui/QProgressDialog>
#include <QtGui/QApplication>
#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Udb/Idx.h>
#include "PdmItems.h"
#include "WtTypeDefs.h"
#include "ObjectHelper.h"
#include <QtDebug>
using namespace Wt;

PdmLayouter PdmItemObj::s_layouter;

void PdmItemObj::setPos( const QPointF& p )
{
	setValue( AttrPosX, Stream::DataCell().setFloat( p.x() ) );
	setValue( AttrPosY, Stream::DataCell().setFloat( p.y() ) );
    setTimeStamp( AttrModifiedOn );
}

QPointF PdmItemObj::getPos() const
{
	return QPointF( getValue(AttrPosX).getFloat(), getValue(AttrPosY).getFloat() );
}

void PdmItemObj::setNodeList( const QPolygonF& p )
{
	Stream::DataWriter w;
	for( int i = 0; i < p.size(); i++ )
	{
		w.startFrame();
		w.writeSlot( Stream::DataCell().setFloat( p[i].x() ) );
		w.writeSlot( Stream::DataCell().setFloat( p[i].y() ) );
		w.endFrame();
	}
	setValue( AttrPointList, w.getBml() );
    setTimeStamp( AttrModifiedOn );
}

QPolygonF PdmItemObj::getNodeList() const
{
	QPolygonF res;
	Stream::DataReader r( getValue( AttrPointList ) );
	Stream::DataReader::Token t = r.nextToken();
	while( t == Stream::DataReader::BeginFrame )
	{
		t = r.nextToken();
		QPointF p;
		if( t == Stream::DataReader::Slot )
			p.setX( r.getValue().getFloat() );
		else
			return QPolygonF();
		t = r.nextToken();
		if( t == Stream::DataReader::Slot )
			p.setY( r.getValue().getFloat() );
		else
			return QPolygonF();
		t = r.nextToken();
		if( t != Stream::DataReader::EndFrame )
			return QPolygonF();
		t = r.nextToken();
		res.append( p );
	}
    return res;
}

bool PdmItemObj::hasNodeList() const
{
    return hasValue( AttrPointList );
}

void PdmItemObj::setOrig(const Udb::Obj &orig)
{
    setValue( AttrOrigObject, orig );
    setTimeStamp( AttrModifiedOn );
}

Udb::Obj PdmItemObj::getOrig() const
{
    return getValueAsObj( AttrOrigObject );
}

PdmItemObj PdmItemObj::createLink(Udb::Obj diagram, Obj pred, const Obj &succ)
{
    Q_ASSERT( !diagram.isNull() );
    Q_ASSERT( !pred.isNull() );
    Q_ASSERT( !succ.isNull() );

    Udb:Obj link = ObjectHelper::createObject( TypeLink, pred );
    link.setValue( AttrPred, pred );
    link.setValue( AttrSucc, succ );
    link.setValue( AttrLinkType, Stream::DataCell().setUInt8( LinkType_FS ) ); // RISK
    return createItemObj( diagram, link );
}

PdmItemObj PdmItemObj::createItemObj(Udb::Obj diagram, Udb::Obj other, const QPointF & p)
{
    Q_ASSERT( !diagram.isNull() );
    PdmItemObj item = diagram.createAggregate( TypePdmItem );
    item.setTimeStamp( AttrCreatedOn );
    item.setOrig( other );
    if( !p.isNull() )
        item.setPos( p );
    return item;
}

void PdmItemObj::removeAllItems(const Udb::Obj &diagram)
{
    QList<Udb::Obj> toRemove;
    Udb::Obj sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
            toRemove.append( sub );
    }while( sub.next() );
    foreach( Udb::Obj o, toRemove )
        o.erase();
}

QSet<Udb::OID> PdmItemObj::findAllItemOrigOids( const Udb::Obj& diagram )
{
    QSet<Udb::OID> test;
    Udb::Obj sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
        {
#ifdef _DEBUG
            Udb::Obj orig = sub.getValueAsObj( AttrOrigObject );
            if( test.contains( orig.getOid() ) )
            {
                qWarning() << "WARNING: Diagram" << WtTypeDefs::formatObjectTitle( diagram ) <<
                              "already contains PdmItem for" << WtTypeDefs::prettyName( orig.getType() ) <<
                              WtTypeDefs::formatObjectTitle( orig );
            }
            test.insert( orig.getOid() );
#else
            test.insert( sub.getValue( AttrOrigObject ).getOid() );
#endif
        }
    }while( sub.next() );
    return test;
}

QList<PdmItemObj> PdmItemObj::findAllAliasses(const Udb::Obj &diagram)
{
    QList<PdmItemObj> res;
    Udb::Obj sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
        {
            Udb::Obj o = sub.getValueAsObj( AttrOrigObject );
            if( WtTypeDefs::isSchedObj( o.getType() ) && !o.getParent().equals( diagram ) )
                res.append( sub );
        }
    }while( sub.next() );
    return res;
}

QList<Udb::Obj> PdmItemObj::findAllItemOrigObjs(const Udb::Obj &diagram, bool schedObjs, bool links)
{
    QList<Udb::Obj> res;
    Udb::Obj sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
        {
            Udb::Obj o = sub.getValueAsObj( AttrOrigObject );
            if( schedObjs && WtTypeDefs::isSchedObj( o.getType() ) )
                res.append( o );
            else if( links && o.getType() == TypeLink )
                res.append( o );
        }
    }while( sub.next() );
    return res;
}

QList<Udb::Obj> PdmItemObj::findExtendedSchedObjs(QList<Udb::Obj> startset,
                                                  quint8 levels, bool toSucc, bool toPred, bool onlyCritical )
{
    // Diese Methode garantiert nicht, dass die Ergebnisse nicht schon im Diagramm sind
    // RISK: diese Funktion ist bei grossen Diagrammen zu langsam!
    QList<Udb::Obj> res;
    QSet<Udb::OID> visited;
    while( levels > 0 )
    {
        QList<Udb::Obj> objs;
        foreach( Udb::Obj o, startset )
        {
            if( !visited.contains( o.getOid() ) )
            {
                visited.insert( o.getOid() );
                if( toSucc )
                {
					QList<Udb::Obj> tmp = ObjectHelper::findSuccessors( o );
                    // NOTE: tmp wäre unnötig; ich habe mit Qt4.4 verifiziert, dass foreach die Funktion
                    // nur einmal aufruft.
                    foreach( Udb::Obj s, tmp )
                    {
                        if( !onlyCritical || s.getValue( AttrCriticalPath ).getBool() )
                        {
                            objs.append( s );
                        }
                    }
                }
                if( toPred )
                {
					QList<Udb::Obj> tmp = ObjectHelper::findPredecessors( o );
                    foreach( Udb::Obj p, tmp )
                    {
                        if( !onlyCritical || p.getValue( AttrCriticalPath ).getBool() )
                        {
                            objs.append( p );
                        }
                    }
                }
            }
        }
        res += objs;
        startset = objs;
        levels--;
    }
    return res;
}

QList<Udb::Obj> PdmItemObj::findHiddenLinks(const Udb::Obj &diagram, QList<Udb::Obj> startset)
{
    if( startset.isEmpty() )
    {
        Udb::Obj sub = diagram.getFirstObj();
        if( !sub.isNull() ) do
        {
            if( sub.getType() == TypePdmItem )
            {
                Udb::Obj orig = sub.getValueAsObj( AttrOrigObject );
                if( WtTypeDefs::isSchedObj( orig.getType() ) )
                    startset.append( orig );
            }
        }while( sub.next() );
    }
    QList<Udb::Obj> res;
    QSet<Udb::OID> existingItems = findAllItemOrigOids( diagram );
    foreach( Udb::Obj o, startset )
    {
        Udb::Idx predIdx( o.getTxn(), IndexDefs::IdxPred );
        if( predIdx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
        {
            Udb::Obj link = o.getObject( predIdx.getOid() );
            if( !link.isNull() && !existingItems.contains( link.getOid() ) &&
                    existingItems.contains( link.getValue( AttrSucc ).getOid() ) )
            {
                // Obj in Startset ist Pred von einem Link, dessen Succ im Diagramm ist, und
                // der Link selber ist im Diagramm noch nicht enthalten.
                res.append( link );
                existingItems.insert( link.getOid() ); // vorher war "&& !res.contains( link )" in Bedingung
            }
        }while( predIdx.nextKey() );
        Udb::Idx succIdx( o.getTxn(), IndexDefs::IdxSucc );
        if( succIdx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
        {
            Udb::Obj link = o.getObject( succIdx.getOid() );
            if( !link.isNull() && !existingItems.contains( link.getOid() ) &&
                    existingItems.contains( link.getValue( AttrPred ).getOid() ) )
            {
                // Obj in Startset ist Succ von einem Link, dessen Pred im Diagramm ist, und
                // der Link selber ist im Diagramm noch nicht enthalten.
                res.append( link );
                existingItems.insert( link.getOid() );
            }
        }while( succIdx.nextKey() );
    }
    return res;
}

QSet<Udb::Obj> PdmItemObj::findHiddenSchedObjs(const Udb::Obj &diagram)
{
    Udb::Obj sub = diagram.getFirstObj();
    QSet<Udb::Obj> hiddens;
    if( !sub.isNull() ) do
    {
        if( WtTypeDefs::isSchedObj( sub.getType() ) )
            hiddens.insert( sub );
    }while( sub.next() );
    sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
            hiddens.remove( sub.getValueAsObj( AttrOrigObject ) );
    }while( sub.next() );
    return hiddens;
}

QRectF PdmItemObj::getBoundingRect() const
{
	QPointF pos = getPos();
	const qreal bwh = PdmNode::s_boxWidth / 2.0;
	const qreal bhh = PdmNode::s_boxHeight / 2.0;
	const qreal pwhh = PdmNode::s_circleDiameter / 2.0;
	QRectF res;
	if( !hasNodeList() )
	{
		res = QRectF( - bwh, - bhh, PdmNode::s_boxWidth, PdmNode::s_boxHeight );
        //qDebug() << res;
		res.moveTo( pos );
        //qDebug() << "move to" << pos << res;
		return res;
	}else
    {
        QPolygonF p = getNodeList();
		return p.boundingRect();
    }
}

QList<Udb::Obj> PdmItemObj::readItems(Udb::Obj diagram, Stream::DataReader& r)
{
    Q_ASSERT( !diagram.isNull() );
    QList<Udb::Obj> res;
    Stream::DataReader::Token t = r.nextToken();
    if( t != Stream::DataReader::Slot || r.getValue().getUuid() != diagram.getDb()->getDbUuid() )
        return res; // Objekte leben in anderer Datenbank; kein Paste möglich.
    QSet<Udb::OID> existingItems = PdmItemObj::findAllItemOrigOids( diagram );
    t = r.nextToken();
    QList<Udb::Obj> done;
    while( t == Stream::DataReader::BeginFrame && r.getName().getTag().equals( "item" ) )
    {
        Stream::DataCell x;
        Stream::DataCell y;
        Stream::DataCell obj;
        t = r.nextToken();
        while( t == Stream::DataReader::Slot )
        {
            if( r.getName().getTag().equals("x") )
                x = r.getValue();
            else if( r.getName().getTag().equals("y") )
                y = r.getValue();
            else if( r.getName().getTag().equals("obj") )
                obj = r.getValue();
            t = r.nextToken();
        }
        Q_ASSERT( t == Stream::DataReader::EndFrame );
        Udb::Obj orig = diagram.getObject( obj.getOid() );
        if( !orig.isNull() && !existingItems.contains( obj.getOid() ) )
        {
            // Lege nur Diagrammelemente für Objekte an, die im Diagramm noch nicht vorhanden sind
            Udb::Obj current = PdmItemObj::createItemObj( diagram, orig );
            current.setValue( AttrPosX, x );
            current.setValue( AttrPosY, y );
            existingItems.insert( orig.getOid() );
            res.append( current );
            done.append( orig );
        }
        t = r.nextToken();
    }
    while( t == Stream::DataReader::BeginFrame && r.getName().getTag().equals( "link" ) )
    {
        Stream::DataCell path;
        Stream::DataCell obj;
        t = r.nextToken();
        while( t == Stream::DataReader::Slot )
        {
            if( r.getName().getTag().equals("path") )
                path = r.getValue();
            else if( r.getName().getTag().equals("obj") )
                obj = r.getValue();
            t = r.nextToken();
        }
        Q_ASSERT( t == Stream::DataReader::EndFrame );
        Udb::Obj link = diagram.getObject( obj.getOid() );
        if( !link.isNull() && !existingItems.contains( link.getOid() ) &&
                existingItems.contains( link.getValue( AttrPred ).getOid() ) &&
                existingItems.contains( link.getValue( AttrSucc ).getOid() ) )
        {
            // Lege nur Links für Objekte an, die im Diagramm vorhanden sind
            Udb::Obj current = PdmItemObj::createItemObj( diagram, link );
            current.setValue( AttrPointList, path );
            existingItems.insert( link.getOid() );
            res.append( current );
        }
        t = r.nextToken();
    }
    QList<Udb::Obj> links = PdmItemObj::findHiddenLinks( diagram, done );
    foreach( Udb::Obj link, links )
        res.append( PdmItemObj::createItemObj( diagram, link ) );
    return res;
}

QList<Udb::Obj> PdmItemObj::writeItems(const QList<Udb::Obj> &items, Stream::DataWriter & out1)
{
    out1.writeSlot( Stream::DataCell().setUuid( items.first().getDb()->getDbUuid() ) );
    // Wir müssen das Format auf die lokale DB beschränken, da es Referenzen auf Objekte gibt
    // Gehe zweimal durch, dass sicher die Tasks/Milestones zuerst kopiert werden
    QList<Udb::Obj> origs;
    foreach( Udb::Obj o, items )
	{
        if( o.getValueAsObj(AttrOrigObject).getType() != TypeLink )
        {
            out1.startFrame( Stream::NameTag( "item" ) );
            out1.writeSlot( o.getValue( AttrPosX ), Stream::NameTag("x") );
            out1.writeSlot( o.getValue( AttrPosY ), Stream::NameTag("y") );
            Udb::Obj orig = o.getValueAsObj( AttrOrigObject );
            out1.writeSlot( orig, Stream::NameTag("obj") );
            out1.endFrame();
            origs.append( orig );
        }
	}
    foreach( Udb::Obj o, items )
	{
        if( o.getValueAsObj(AttrOrigObject).getType() == TypeLink )
        {
            out1.startFrame( Stream::NameTag( "link" ) );
            out1.writeSlot( o.getValue( AttrPointList ), Stream::NameTag("path") );
            Udb::Obj orig = o.getValueAsObj( AttrOrigObject );
            out1.writeSlot( orig, Stream::NameTag("obj") );
            out1.endFrame();
            origs.append( orig );
        }
	}
    return origs;
}

QList<Udb::Obj> PdmItemObj::addItemsToDiagram(Udb::Obj diagram, const QList<Udb::Obj> &items, QPointF where)
{
    QSet<Udb::OID> existingItems = findAllItemOrigOids( diagram );
    QList<Udb::Obj> done;
    foreach( Udb::Obj o, items )
    {
        if( WtTypeDefs::isSchedObj( o.getType() ) &&
                !existingItems.contains( o.getOid() ) )
        {
            createItemObj( diagram, o, where );
            where += QPointF( PdmNode::s_rasterX, PdmNode::s_rasterY ); // TODO: unschön
            done.append( o );
            existingItems.insert( o.getOid() );
        }else if( o.getType() == TypeLink && !existingItems.contains( o.getOid() ) )
        {
            createItemObj( diagram, o );
            existingItems.insert( o.getOid() );
        }
    }
    return done;
}

QList<Udb::Obj> PdmItemObj::addItemLinksToDiagram(Udb::Obj diagram, const QList<Udb::Obj> &items)
{
    QList<Udb::Obj> links = findHiddenLinks( diagram, items );
    foreach( Udb::Obj link, links )
        createItemObj( diagram, link );
    return links;
}

bool PdmItemObj::createDiagram(Udb::Obj diagram, bool recreate, bool layout,
							   bool recursive, quint8 levels, bool toSucc, bool toPred, QProgressDialog* pg )
{
	const QString title = WtTypeDefs::formatObjectTitle( diagram );
	if( pg )
	{
		QApplication::processEvents();
		pg->setLabelText( QObject::tr("Emptying '%1'").arg(title));
		pg->setValue( pg->value() + 1 );
		QApplication::processEvents();
		if( pg->wasCanceled() )
			return false;
	}
	if( recreate )
		removeAllItems( diagram );
	QList<Udb::Obj> objs;
	QList<Udb::Obj> diags;
	Udb::Obj sub = diagram.getFirstObj();
	if( !sub.isNull() ) do
	{
		if( WtTypeDefs::isSchedObj( sub.getType() ) )
			objs.append( sub );
		if( WtTypeDefs::isPdmDiagram( sub.getType() ) )
			diags.append( sub );
	}while( sub.next() );
	if( pg )
	{
		QApplication::processEvents();
		pg->setLabelText( QObject::tr("Analyzing '%1'").arg(title));
		pg->setValue( pg->value() + 1 );
		QApplication::processEvents();
		if( pg->wasCanceled() )
			return false;
	}
	objs += findExtendedSchedObjs( objs, levels, toSucc, toPred, false );
	if( pg )
	{
		QApplication::processEvents();
		pg->setLabelText( QObject::tr("Creating '%1'").arg(title));
		pg->setValue( pg->value() + 1 );
		QApplication::processEvents();
		if( pg->wasCanceled() )
			return false;
	}
	QList<Udb::Obj> done = addItemsToDiagram( diagram, objs, QPointF(0,0) );
	addItemLinksToDiagram( diagram, done );
	if( layout )
	{
		if( pg )
		{
			QApplication::processEvents();
			pg->setLabelText( QObject::tr("Layouting '%1'").arg(title));
			pg->setValue( pg->value() + 1 );
			QApplication::processEvents();
			if( pg->wasCanceled() )
				return false;
		}
		s_layouter.layoutDiagram( diagram, false, false );
	}
	if( recursive )
	{
		foreach( Udb::Obj doc, diags )
		{
			if( !createDiagram( doc, recreate, layout, recursive, levels, toSucc, toPred, pg ) )
				return false;
		}
	}
	return true;
}

