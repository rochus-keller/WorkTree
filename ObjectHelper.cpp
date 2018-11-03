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

#include "ObjectHelper.h"
#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Udb/Idx.h>
#include "WorkTreeApp.h"
#include "WtTypeDefs.h"
using namespace Wt;

Udb::Obj ObjectHelper::createObject(quint32 type, Udb::Obj parent, const Udb::Obj& before )
{
    Q_ASSERT( !parent.isNull() );
    Udb::Obj o = parent.createAggregate( type, before );
    if( WtTypeDefs::canHaveText( type ) )
        o.setString( AttrText, WtTypeDefs::prettyName( type ) );
	o.setTimeStamp( AttrCreatedOn );
    const QString id = getNextIdString( o.getTxn(), type );
    if( !id.isEmpty() )
        o.setString( AttrInternalId, id );
    if( type == TypeTask || type == TypeMilestone )
    {
        Q_ASSERT( !parent.isNull() );
        parent.incCounter( AttrSubTMSCount );
    }else if( type == TypeCalEntry )
    {
        Q_ASSERT( !parent.isNull() );
        parent.incCounter( AttrCalEntryCount );
    }
    return o;
}

quint32 ObjectHelper::getNextId(Udb::Transaction * txn, quint32 type)
{
    Udb::Obj root = WtTypeDefs::getRoot( txn );
    Q_ASSERT( !root.isNull() );
    return root.incCounter( type );
}

QString ObjectHelper::getNextIdString(Udb::Transaction * txn, quint32 type)
{
    QString prefix;
    switch( type )
    {
    case TypeTask:
        prefix = QLatin1String("T");
        break;
    case TypeMilestone:
        prefix = QLatin1String("M");
        break;
    case TypeImpEvent:
        prefix = QLatin1String("E");
        break;
    case TypeAccomplishment:
        prefix = QLatin1String("A");
        break;
    case TypeCriterion:
        prefix = QLatin1String("C");
        break;
    case TypeLink:
        prefix = QLatin1String("L");
        break;
    case TypeDeliverable:
        prefix = QLatin1String("D");
        break;
    case TypeWork:
        prefix = QLatin1String("W");
        break;
    }
    if( !prefix.isEmpty() )
        return QString("%1%2").arg( prefix ).
                     arg( getNextId( txn, type ), 3, 10, QLatin1Char('0' ) );
    else
        return QString();
}

void ObjectHelper::retypeObject(Udb::Obj &o, quint32 type)
{
    Q_ASSERT( !o.isNull() );
    if( o.getType() == type )
        return;
    o.setCell( Udb::Obj::KeyList() << Stream::DataCell().setAtom( o.getType() ), o.getValue( AttrInternalId ) );
    QString id = o.getCell( Udb::Obj::KeyList() << Stream::DataCell().setAtom( type ) ).toString();
    if( id.isEmpty() )
        id = getNextIdString( o.getTxn(), type );
    if( !id.isEmpty() )
        o.setString( AttrInternalId, id );
    o.setTimeStamp( AttrModifiedOn );
    if( o.getType() == TypeTask || o.getType() == TypeMilestone )
    {
        Q_ASSERT( !o.getParent().isNull() );
        o.getParent().decCounter( AttrSubTMSCount );
        if( o.getParent().getValueAsObj( AttrStartMs ).equals( o ) )
            o.getParent().clearValue( AttrStartMs );
        if( o.getParent().getValueAsObj( AttrFinishMs ).equals( o ) )
            o.getParent().clearValue( AttrFinishMs );
        o.clearValue( AttrMsType );
    }
    o.setType( type );
    if( o.getType() == TypeTask || o.getType() == TypeMilestone )
    {
        Q_ASSERT( !o.getParent().isNull() );
        o.getParent().incCounter( AttrSubTMSCount );
    }
}

void ObjectHelper::moveTo(Udb::Obj &o, Udb::Obj &newParent, const Udb::Obj &before)
{
    Q_ASSERT( !o.getParent().isNull() );
    // Cleanup old parent
    o.getParent().decCounter( AttrSubTMSCount );
    if( o.getParent().getValueAsObj( AttrStartMs ).equals( o ) )
        o.getParent().clearValue( AttrStartMs );
    if( o.getParent().getValueAsObj( AttrFinishMs ).equals( o ) )
        o.getParent().clearValue( AttrFinishMs );
    o.clearValue( AttrMsType );
    // Install new parent
    o.aggregateTo( newParent, before );
    Q_ASSERT( !newParent.isNull() );
    newParent.incCounter( AttrSubTMSCount );
}

static void _eraseDiagItems( const Udb::Obj& orig )
{
    if( orig.isNull() )
        return;
    Udb::Idx idx( orig.getTxn(), IndexDefs::IdxOrigObject );
    if( idx.seek( orig ) ) do
    {
        Udb::Obj item = orig.getObject( idx.getOid() );
        if( item.getType() == TypePdmItem )
            item.erase();
    }while( idx.nextKey() );
}

static void _recursiveEraseLinks( Udb::Obj &o )
{
    // Lösche auch die Links, wo das Object Successor ist (die übrigen sind ja
    // Children des Predecessors
    if( o.isNull() )
        return;
    Udb::Idx idx( o.getTxn(), IndexDefs::IdxSucc );
    if( idx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
    {
        Udb::Obj link = o.getObject( idx.getOid() );
        if( !link.isNull() )
            link.erase();
    }while( idx.nextKey() );
    Udb::Obj sub = o.getFirstObj();
    if( !sub.isNull() )do
    {
        _recursiveEraseLinks( sub );
    }while( sub.next() );
}

void ObjectHelper::erase(Udb::Obj &o)
{
    if( o.isNull( false, true ) )
        return;
    if( o.getType() == TypeTask || o.getType() == TypeMilestone )
    {
        Q_ASSERT( !o.getParent().isNull() );
        // Task löschen mit Löschen der nicht mehr benötigten Links
        _recursiveEraseLinks( o );
        o.getParent().decCounter( AttrSubTMSCount );
        if( o.getParent().getValueAsObj( AttrStartMs ).equals( o ) )
            o.getParent().clearValue( AttrStartMs );
        if( o.getParent().getValueAsObj( AttrFinishMs ).equals( o ) )
            o.getParent().clearValue( AttrFinishMs );
        _eraseDiagItems( o );
    }else if( o.getType() == TypeLink )
        _eraseDiagItems( o );
    else if( o.getType() == TypeCalEntry )
    {
        Q_ASSERT( !o.getParent().isNull() );
        o.getParent().decCounter( AttrCalEntryCount );
    }
    o.erase();
    // Weil this und die untergeordneten eh gelöscht werden ist, auch AttrSubTMSCount nicht mehr relevant
}

void ObjectHelper::setText(Udb::Obj &o, const QString &str)
{
    o.setString( AttrText, str );
    o.setTimeStamp( AttrModifiedOn );
}

void ObjectHelper::setMsType(Udb::Obj & ms, int msType )
{
    if( ms.getType() != TypeMilestone )
        return;
    Udb::Obj oldTaskStart = ms.getParent().getValueAsObj( AttrStartMs );
    if( ms.equals( oldTaskStart ) )
        ms.getParent().clearValue( AttrStartMs );
    Udb::Obj oldTaskFinish = ms.getParent().getValueAsObj( AttrFinishMs );
    if( ms.equals( oldTaskFinish ) )
        ms.getParent().clearValue( AttrFinishMs );
    Udb::Obj proj = WtTypeDefs::getProject( ms.getTxn() );
    Udb::Obj oldProjStart = proj.getValueAsObj( AttrStartMs );
    if( ms.equals( oldProjStart ) )
        proj.clearValue( AttrStartMs );
    Udb::Obj oldProjFinish = proj.getValueAsObj( AttrFinishMs );
    if( ms.equals( oldProjFinish ) )
        proj.clearValue( AttrFinishMs );
    switch( msType )
    {
    case MsType_Intermediate:
        ms.clearValue( AttrMsType );
        break;
    case MsType_TaskStart:
        ms.setValue( AttrMsType, Stream::DataCell().setUInt8( MsType_TaskStart ) );
        ms.getParent().setValueAsObj( AttrStartMs, ms );
        if( !oldTaskStart.isNull() )
            oldTaskStart.clearValue( AttrMsType );
        break;
    case MsType_TaskFinish:
        ms.setValue( AttrMsType, Stream::DataCell().setUInt8( MsType_TaskFinish ) );
        ms.getParent().setValueAsObj( AttrFinishMs, ms );
        if( !oldTaskFinish.isNull() )
            oldTaskFinish.clearValue( AttrMsType );
        break;
    case MsType_ProjStart:
        ms.setValue( AttrMsType, Stream::DataCell().setUInt8( MsType_ProjStart ) );
        proj.setValueAsObj( AttrStartMs, ms );
        if( !oldProjStart.isNull() )
            oldProjStart.clearValue( AttrMsType );
        break;
    case MsType_ProjFinish:
        ms.setValue( AttrMsType, Stream::DataCell().setUInt8( MsType_ProjFinish ) );
        proj.setValueAsObj( AttrFinishMs, ms );
        if( !oldProjFinish.isNull() )
            oldProjFinish.clearValue( AttrMsType );
        break;
    }
}

// TODO: Task und Milestone löschen mit Rückgabe IDs

typedef QPair<Udb::Obj,int> Pred;
typedef QHash<Udb::OID,Pred> Visited; // current -> ( Predecessor, path length )
typedef QMultiHash<Udb::OID,Udb::OID> PredSucc;

static void _search( const Udb::Obj& cur, int val, const Udb::Obj& goal,
					 const PredSucc& predSucc, Visited& visited, ObjectHelper::ShortestPathMethod m )
{
	switch( m )
	{
	case ObjectHelper::SpmNodeCount:
		val += 1;
		break;
	case ObjectHelper::SpmDuration:
		val += cur.getValue( AttrDuration ).getUInt16();
		break;
	case ObjectHelper::SpmOptimisticDur:
		val += cur.getValue( AttrOptimisticDur ).getUInt16();
		break;
	case ObjectHelper::SpmPessimisticDur:
		val += cur.getValue( AttrPessimisticDur ).getUInt16();
		break;
	case ObjectHelper::SpmMostLikelyDur:
		val += cur.getValue( AttrMostLikelyDur ).getUInt16();
		break;
	case ObjectHelper::SpmCriticalPath:
		val = 0xffff; // egal, einfach grosse zahl
		break;
	}
	PredSucc::const_iterator i = predSucc.find( cur.getOid() );
	while( i != predSucc.end() && i.key() == cur.getOid() )
	{
		Udb::Obj next = cur.getObject( i.value() );
		if( m == ObjectHelper::SpmCriticalPath && next.getValue( AttrCriticalPath ).getBool() )
			val = 0;
		Pred& p = visited[ i.value() ];
		if( p.first.isNull() )
		{
			// Not yet visited
			p.first = cur;
			p.second = val;
			if( !next.equals( goal ) && ( m != ObjectHelper::SpmCriticalPath || val == 0 ) )
				_search( next, val, goal, predSucc, visited, m );
		}else if( p.second > val )
		{
			// already visited, but shorter path found
			p.first = cur;
			p.second = val;
		}else
		{
			// already visited
		}
		++i;
	}
}

static QList<Udb::Obj> _unroll( const Udb::Obj &start, const Udb::Obj &goal, const Visited& visited, bool onlyCritical  )
{
	QList<Udb::Obj> path;
	Visited::const_iterator i = visited.find( goal.getOid() );
	while( i != visited.end() )
	{
		if( onlyCritical && i.value().second != 0 )
			return QList<Udb::Obj>(); // der kritische Pfad wurde nicht gefunden
		path.prepend( start.getObject(i.key()) );
		if( i.value().first.equals( start ) )
		{
			path.prepend( start );
			return path;
		}
		i = visited.find( i.value().first.getOid() );
	}
	return QList<Udb::Obj>();
}

QList<Udb::Obj> ObjectHelper::findShortestPath(const Udb::Obj &start, const Udb::Obj &goal, ShortestPathMethod m)
{
	// Diese Methode garantiert nicht, dass die Ergebnisse noch nicht im Diagramm sind!

	Q_ASSERT( !start.isNull() && !goal.isNull() );
	// Es gibt dafür keinen schlauen bzw. etablierten Algorithmus ausser rudimentäre Suche
	Visited visited;
	PredSucc predSucc;
	Udb::Idx predIdx( start.getTxn(), IndexDefs::IdxPred );
	if( predIdx.first() ) do
	{
		Udb::Obj o = start.getObject( predIdx.getOid() );
		Q_ASSERT( !o.isNull() );
		predSucc.insert( o.getValue( AttrPred ).getOid(), o.getValue( AttrSucc ).getOid() );
	}while( predIdx.next() );
	_search( start, 0, goal, predSucc, visited, m );
	if( !visited.contains( goal.getOid() ) )
	{
		// Wir haben keinen Pfad gefunden, also umgekehrte Suche
		visited.clear();
		_search( goal, 0, start, predSucc, visited, m );
		return _unroll( goal, start, visited, m == SpmCriticalPath );
	}else
		return _unroll( start, goal, visited, m == SpmCriticalPath );
}

QList<Udb::Obj> ObjectHelper::findSuccessors(const Udb::Obj &item)
{
	// Diese Funktion garantiert nicht, dass Items nicht schon im Diagramm sind!
	QList<Udb::Obj> successors;
	Udb::Idx predIdx( item.getTxn(), IndexDefs::IdxPred );
	if( predIdx.seek( Stream::DataCell().setOid( item.getOid() ) ) ) do
	{
		Udb::Obj link = item.getObject( predIdx.getOid() );
		if( !link.isNull() )
		{
			Udb::Obj succ = link.getValueAsObj( AttrSucc );
			if( !succ.isNull() )
				successors.append( succ );
		}
	}while( predIdx.nextKey() );
	return successors;
}

QList<Udb::Obj> ObjectHelper::findPredecessors( const Udb::Obj &item)
{
	// Diese Funktion garantiert nicht, dass Items nicht schon im Diagramm sind!
	QList<Udb::Obj> predecessors;
	Udb::Idx succIdx( item.getTxn(), IndexDefs::IdxSucc );
	if( succIdx.seek( Stream::DataCell().setOid( item.getOid() ) ) ) do
	{
		Udb::Obj link = item.getObject( succIdx.getOid() );
		if( !link.isNull() )
		{
			Udb::Obj pred = link.getValueAsObj( AttrPred );
			if( !pred.isNull() )
				predecessors.append( pred );
		}
	}while( succIdx.nextKey() );
	return predecessors;
}
