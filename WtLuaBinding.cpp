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

#include "WtLuaBinding.h"
#include "WorkTreeApp.h"
#include "ObjectHelper.h"
#include "WtTypeDefs.h"
#include <Udb/LuaBinding.h>
#include <Udb/ContentObject.h>
#include <Oln2/OutlineItem.h>
#include <Script2/QtValue.h>
#include <Script/Engine2.h>
#include <Udb/Idx.h>
#include <Udb/Transaction.h>
using namespace Wt;

typedef bool (*TypeFilter)( quint32 type );

static int getSubsOfType(lua_State *L, TypeFilter filter )
{
	Udb::ContentObject* obj = Udb::CoBin<Udb::ContentObject>::check( L, 1 );
	QList<Udb::Obj> items;
	Udb::Obj sub = obj->getFirstObj();
	if( !sub.isNull() ) do
	{
		if( filter == 0 || filter( sub.getType() ) )
			items.append(sub);
	}while( sub.next() );
	lua_createtable(L, items.size(), 0 );
	const int table = lua_gettop(L);
	for( int i = 0; i < items.size(); i++ )
	{
		Udb::LuaBinding::pushObject( L, items[i] );
		lua_rawseti( L, table, i + 1 );
	}
	return 1;
}


struct _Imp : public Udb::ContentObject
{
	_Imp( const Udb::Obj& o ):Udb::ContentObject(o){}
	_Imp(){}
	static bool findTask( quint32 type ) { return type == TypeTask; }
	static int getChildren(lua_State *L){ return getSubsOfType( L, WtTypeDefs::isImpType ); }
	static int getTasks(lua_State *L){ return getSubsOfType( L, findTask ); }
	static int addType( lua_State *L, quint32 type )
	{
		_Imp* obj = Udb::CoBin<_Imp>::check( L, 1 );
		if( !WtTypeDefs::isValidAggregate( obj->getType(), type ) )
			luaL_argerror( L, 2, "invalid aggregate type in this context" );

		Udb::LuaBinding::pushObject( L, ObjectHelper::createObject( type, *obj, Udb::Obj() ) );
		return 1;
	}
	static int addTask(lua_State *L) { return addType( L, TypeTask ); }
	static int addMilestone(lua_State *L) { return addType( L, TypeMilestone ); }
	static int addEvent(lua_State *L) { return addType( L, TypeImpEvent ); }
	static int addAccomplishment(lua_State *L) { return addType( L, TypeAccomplishment ); }
	static int addCriterion(lua_State *L) { return addType( L, TypeCriterion ); }
};

static const luaL_reg _Imp_reg[] =
{
	{ "getChildren", _Imp::getChildren },
	{ "getTasks", _Imp::getTasks },
	{ "addTask", _Imp::addTask },
	{ "addMilestone", _Imp::addMilestone },
	{ "addEvent", _Imp::addEvent },
	{ "addAccomplishment", _Imp::addAccomplishment },
	{ "addCriterion", _Imp::addCriterion },
	{ 0, 0 }
};

static int _getPrettyTitle(lua_State *L)
{
	Udb::ContentObject* obj = Udb::CoBin<Udb::ContentObject>::check( L, 1 );
	*Lua::QtValue<QString>::create(L) = WtTypeDefs::formatObjectTitle( *obj );
	return 1;
}

static int _erase(lua_State *L)
{
	Udb::ContentObject* obj = Udb::CoBin<Udb::ContentObject>::check( L, 1 );
	ObjectHelper::erase( *obj );
	// *obj = Udb::Obj(); // Nein, es koennte ja danach Rollback stattfinden
	return 1;
}

static int _moveTo(lua_State *L)
{
	Udb::ContentObject* obj = Udb::CoBin<Udb::ContentObject>::check( L, 1 );
	Udb::ContentObject* parent = Udb::CoBin<Udb::ContentObject>::check( L, 2 );
	Udb::ContentObject* before = Udb::CoBin<Udb::ContentObject>::cast( L, 3 );
	if( before && !before->getParent().equals( *parent ) )
		luaL_argerror( L, 3, "'before' must be a child of 'parent'" );
	if( !WtTypeDefs::isValidAggregate( parent->getType(), obj->getType() ) )
		luaL_argerror( L, 2, "invalid 'parent' for object" );
	if( before )
		ObjectHelper::moveTo( *obj, *parent, *before );
	else
		ObjectHelper::moveTo( *obj, *parent, Udb::Obj() );
	return 0;
}

static const luaL_reg _ContentObject_reg[] =
{
	{ "getPrettyTitle", _getPrettyTitle },
	{ "delete", _erase },
	{ "moveTo", _moveTo },
	{ 0, 0 }
};

template<class T,int attr>
static int _getValue(lua_State *L)
{
	T* obj = Udb::CoBin<T>::check( L, 1 );
	Udb::LuaBinding::pushValue( L, obj->getValue(attr), obj->getTxn() );
	return 1;
}

struct _SchedObj : public _Imp
{
	_SchedObj( const Udb::Obj& o ):_Imp(o){}
	_SchedObj(){}

	static int getEarlyStart(lua_State *L) { return _getValue<_SchedObj,AttrEarlyStart>(L); }
	static int getEarlyFinish(lua_State *L) { return _getValue<_SchedObj,AttrEarlyFinish>(L); }
	static int getLateStart(lua_State *L) { return _getValue<_SchedObj,AttrLateStart>(L); }
	static int getLateFinish(lua_State *L) { return _getValue<_SchedObj,AttrLateFinish>(L); }
	static int getDuration(lua_State *L) { return _getValue<_SchedObj,AttrDuration>(L); }
	static int getOptimisticDur(lua_State *L) { return _getValue<_SchedObj,AttrOptimisticDur>(L); }
	static int getPessimisticDur(lua_State *L) { return _getValue<_SchedObj,AttrPessimisticDur>(L); }
	static int getMostLikelyDur(lua_State *L) { return _getValue<_SchedObj,AttrMostLikelyDur>(L); }
	static int getPlannedValue(lua_State *L) { return _getValue<_SchedObj,AttrPlannedValue>(L); }
	static int getEarnedValue(lua_State *L) { return _getValue<_SchedObj,AttrEarnedValue>(L); }
	static int getActualCost(lua_State *L) { return _getValue<_SchedObj,AttrActualCost>(L); }
	static int getSuccessors(lua_State *L)
	{
		_SchedObj* obj = Udb::CoBin<_SchedObj>::check( L, 1 );
		QList<Udb::Obj> links = ObjectHelper::findSuccessors(*obj);
		lua_createtable(L, links.size(), 0 );
		const int table = lua_gettop(L);
		for( int i = 0; i < links.size(); i++ )
		{
			Udb::LuaBinding::pushObject( L, links[i] );
			lua_rawseti( L, table, i + 1 );
		}
		return 1;
	}
	static int getPredecessors(lua_State *L)
	{
		_SchedObj* obj = Udb::CoBin<_SchedObj>::check( L, 1 );
		QList<Udb::Obj> links = ObjectHelper::findPredecessors(*obj);
		lua_createtable(L, links.size(), 0 );
		const int table = lua_gettop(L);
		for( int i = 0; i < links.size(); i++ )
		{
			Udb::LuaBinding::pushObject( L, links[i] );
			lua_rawseti( L, table, i + 1 );
		}
		return 1;
	}
	static int linkTo(lua_State *L)
	{
		_SchedObj* pred = Udb::CoBin<_SchedObj>::check( L, 1 );
		_SchedObj* succ = Udb::CoBin<_SchedObj>::check( L, 2 );

		Udb:Obj link = ObjectHelper::createObject( TypeLink, *pred );
		link.setValue( AttrPred, *pred );
		link.setValue( AttrSucc, *succ );
		link.setValue( AttrLinkType, Stream::DataCell().setUInt8( LinkType_FS ) ); // RISK

		Udb::LuaBinding::pushObject( L, link );
		return 1;
	}
};

static const luaL_reg _SchedObj_reg[] =
{
	{ "getEarlyStart", _SchedObj::getEarlyStart },
	{ "getEarlyFinish", _SchedObj::getEarlyFinish },
	{ "getLateStart", _SchedObj::getLateStart },
	{ "getLateFinish", _SchedObj::getLateFinish },
	{ "getDuration", _SchedObj::getDuration },
	{ "getOptimisticDur", _SchedObj::getOptimisticDur },
	{ "getPessimisticDur", _SchedObj::getPessimisticDur },
	{ "getMostLikelyDur", _SchedObj::getMostLikelyDur },
	{ "getPlannedValue", _SchedObj::getPlannedValue },
	{ "getEarnedValue", _SchedObj::getEarnedValue },
	{ "getActualCost", _SchedObj::getActualCost },
	{ "getSuccessors", _SchedObj::getSuccessors },
	{ "getPredecessors", _SchedObj::getPredecessors },
	{ 0, 0 }
};

struct _Task : public _SchedObj
{
	_Task( const Udb::Obj& o ):_SchedObj(o){}
	_Task(){}

	static int getTaskType(lua_State *L) { return _getValue<_Task,AttrTaskType>(L); }
	static int getSubTMSCount(lua_State *L) { return _getValue<_Task,AttrSubTMSCount>(L); }
	static int getWbsRef(lua_State *L) { return _getValue<_Task,AttrWbsRef>(L); }
	static int getCriticalPath(lua_State *L) { return _getValue<_Task,AttrCriticalPath>(L); }
	static int getCalendar(lua_State *L) { return _getValue<_Task,AttrCalendar>(L); }
	static int getStartMs(lua_State *L) { return _getValue<_Task,AttrStartMs>(L); }
	static int getFinishMs(lua_State *L) { return _getValue<_Task,AttrFinishMs>(L); }
};

static const luaL_reg _Task_reg[] =
{
	{ "getTaskType", _Task::getTaskType },
	{ "getSubTMSCount", _Task::getSubTMSCount },
	{ "getWbsRef", _Task::getWbsRef },
	{ "getCriticalPath", _Task::getCriticalPath },
	{ "getCalendar", _Task::getCalendar },
	{ "getStartMs", _Task::getStartMs },
	{ "getFinishMs", _Task::getFinishMs },
	{ 0, 0 }
};

struct _Milestone : public _SchedObj
{
	_Milestone( const Udb::Obj& o ):_SchedObj(o){}
	_Milestone(){}

	static int getMsType(lua_State *L) { return _getValue<_Milestone,AttrMsType>(L); }
};

static const luaL_reg _Milestone_reg[] =
{
	{ "getMsType", _Milestone::getMsType },
	{ 0, 0 }
};

struct _Link : public Udb::ContentObject
{
	_Link( const Udb::Obj& o ):Udb::ContentObject(o){}
	_Link(){}

	static int getPred(lua_State *L) { return _getValue<_Link,AttrPred>(L); }
	static int getSucc(lua_State *L) { return _getValue<_Link,AttrSucc>(L); }
	static int getLinkType(lua_State *L) { return _getValue<_Link,AttrLinkType>(L); }
};

static const luaL_reg _Link_reg[] =
{
	{ "getPred", _Link::getPred },
	{ "getSucc", _Link::getSucc },
	{ "getLinkType", _Link::getLinkType },
	{ 0, 0 }
};

struct _Event : public _Imp
{
	_Event( const Udb::Obj& o ):_Imp(o){}
	_Event(){}
};

struct _Accomplishment : public _Imp
{
	_Accomplishment( const Udb::Obj& o ):_Imp(o){}
	_Accomplishment(){}
};

struct _Criterion : public _Imp
{
	_Criterion( const Udb::Obj& o ):_Imp(o){}
	_Criterion(){}
};

struct _Repository
{
	Udb::Transaction* d_txn;
	_Repository():d_txn(0) {}

	static int getImp(lua_State *L)
	{
		_Repository* obj = Lua::ValueBinding<_Repository>::check( L, 1 );
		Udb::LuaBinding::pushObject( L, obj->d_txn->getObject( QUuid(WorkTreeApp::s_imp) ) );
		return 1;
	}
	static int commit(lua_State *L)
	{
		_Repository* obj = Lua::ValueBinding<_Repository>::check( L, 1 );
		obj->d_txn->commit();
		return 0;
	}
	static int rollback(lua_State *L)
	{
		_Repository* obj = Lua::ValueBinding<_Repository>::check( L, 1 );
		obj->d_txn->rollback();
		return 0;
	}
};

static const luaL_reg _Repository_reg[] =
{
	{ "getImp", _Repository::getImp },

	//{ "getRootFolder", _Repository::getRootFolder },
	//{ "getRootFunction", _Repository::getRootFunction },
	//{ "getRootElement", _Repository::getRootElement },
	{ "commit", _Repository::commit },
	{ "rollback", _Repository::rollback },
	{ 0, 0 }
};

void Wt::LuaBinding::setRepository(lua_State *L, Udb::Transaction * txn)
{
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Repository" );
	const int table = lua_gettop(L);
	Lua::ValueBinding<_Repository>::create(L)->d_txn = txn;
	lua_pushvalue( L, -1 );
	lua_setfield( L, table, "instance" );
	lua_setfield( L, LUA_GLOBALSINDEX, "wt" );
	lua_pop(L, 1); // table
}

void Wt::LuaBinding::setCurrentObject(lua_State *L, const Udb::Obj & o)
{
	Lua::StackTester test(L, 0 );
	lua_getfield( L, LUA_GLOBALSINDEX, "Repository" );
	const int table = lua_gettop(L);
	Udb::LuaBinding::pushObject( L, o );
	lua_setfield( L, table, "current" );
	lua_pop(L, 1); // table
}

void Wt::LuaBinding::setCurrentObject(const Udb::Obj & o)
{
	setCurrentObject( Lua::Engine2::getInst()->getCtx(), o );
}

static void _pushObject(lua_State * L, const Udb::Obj& o )
{
	const Udb::Atom t = o.getType();
	if( o.isNull() )
		lua_pushnil(L);
	else if( t == TypeIMP )
		Udb::CoBin<_Imp>::create( L, o );
	else if( t == TypeMilestone )
		Udb::CoBin<_Milestone>::create( L, o );
	else if( t == TypeTask )
		Udb::CoBin<_Task>::create( L, o );
	else if( t == TypeLink )
		Udb::CoBin<_Link>::create( L, o );
	else if( t == TypeImpEvent )
		Udb::CoBin<_Event>::create( L, o );
	else if( t == TypeAccomplishment )
		Udb::CoBin<_Accomplishment>::create( L, o );
	else if( t == TypeCriterion )
		Udb::CoBin<_Criterion>::create( L, o );
	else if( t == Oln::OutlineItem::TID )
		Udb::CoBin<Oln::OutlineItem>::create( L, o );
	else if( t == Oln::Outline::TID )
		Udb::CoBin<Oln::Outline>::create( L, o );
	else
	{
		lua_pushnil(L);
		// luaL_error hier nicht sinnvoll, da auch aus normalem Code heraus aufgerufen und Lua exit aufruft
		qWarning( "cannot create binding object for type ID %d %s", t, WtTypeDefs::prettyName(t).toUtf8().data() );
	}
}

static bool _isWritable( quint32 atom, const Udb::Obj& )
{
	return  atom >= WtEnd;
}

void Wt::LuaBinding::install(lua_State *L)
{
	Lua::StackTester test(L, 0 );

	Udb::LuaBinding::pushObject = _pushObject;
	Udb::LuaBinding::isWritable = _isWritable;

	Udb::CoBin<Udb::ContentObject>::addMethods( L, _ContentObject_reg );
	Lua::ValueBinding<_Repository>::install( L, "Repository", _Repository_reg );
	Udb::CoBin<_Imp,Udb::ContentObject>::install( L, "IMP", _Imp_reg );
	Udb::CoBin<_SchedObj,_Imp>::install( L, "ScheduleItem", _SchedObj_reg );
	Udb::CoBin<_Event,_Imp>::install( L, "Event" );
	Udb::CoBin<_Accomplishment,_Imp>::install( L, "Accomplishment" );
	Udb::CoBin<_Criterion,_Imp>::install( L, "Criterion" );

	Udb::CoBin<_Task,_SchedObj>::install( L, "Task", _Task_reg );
	lua_getfield( L, LUA_GLOBALSINDEX, "Task" );
	int table = lua_gettop(L);
	lua_pushinteger( L, TaskType_Discrete );
	lua_setfield( L, table, "Discrete" );
	lua_pushinteger( L, TaskType_LOE );
	lua_setfield( L, table, "LOE" );
	lua_pushinteger( L, TaskType_SVT );
	lua_setfield( L, table, "SVT" );
	lua_pop(L, 1); // table

	Udb::CoBin<_Milestone,_SchedObj>::install( L, "Milestone", _Milestone_reg );
	lua_getfield( L, LUA_GLOBALSINDEX, "Milestone" );
	table = lua_gettop(L);
	lua_pushinteger( L, MsType_Intermediate );
	lua_setfield( L, table, "Intermediate" );
	lua_pushinteger( L, MsType_TaskStart );
	lua_setfield( L, table, "TaskStart" );
	lua_pushinteger( L, MsType_TaskFinish );
	lua_setfield( L, table, "TaskFinish" );
	lua_pushinteger( L, MsType_ProjStart );
	lua_setfield( L, table, "ProjStart" );
	lua_pushinteger( L, MsType_ProjFinish );
	lua_setfield( L, table, "ProjFinish" );
	lua_pop(L, 1); // table

	Udb::CoBin<_Link,Udb::ContentObject>::install( L, "Link", _Link_reg );
	lua_getfield( L, LUA_GLOBALSINDEX, "Link" );
	table = lua_gettop(L);
	lua_pushinteger( L, LinkType_FS );
	lua_setfield( L, table, "FS" );
	lua_pushinteger( L, LinkType_FF );
	lua_setfield( L, table, "FF" );
	lua_pushinteger( L, LinkType_SS );
	lua_setfield( L, table, "SS" );
	lua_pushinteger( L, LinkType_SF );
	lua_setfield( L, table, "SF" );
	lua_pop(L, 1); // table

}


