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

#include "WorkTreeApp.h"
#include "WtLuaBinding.h"
#include <QtGui/QApplication>
#include <QtGui/QPlastiqueStyle>
#include <QtGui/QMessageBox>
#include <QDesktopServices>
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <Udb/DatabaseException.h>
#include <Oln2/OutlineItem.h>
#include "MainWindow.h"
#include "WtTypeDefs.h"
#include <Script2/QtObject.h>
#include <Script/Engine2.h>
#include <Oln2/LuaBinding.h>
#include <Qtl2/Objects.h>
#include <Qtl2/Variant.h>
using namespace Wt;

const char* WorkTreeApp::s_company = "Dr. Rochus Keller";
const char* WorkTreeApp::s_domain = "me@rochus-keller.info";
const char* WorkTreeApp::s_appName = "WorkTree";
const QUuid WorkTreeApp::s_rootUuid( "{26EA3150-68B3-4553-A9DB-05807314A12E}" );
const char* WorkTreeApp::s_version = "0.8";
const char* WorkTreeApp::s_date = "2018-10-21";
const char* WorkTreeApp::s_extension = ".wtdb";
const char* WorkTreeApp::s_imp = "{E6153DDB-3A00-49fe-985F-30956E13C0A9}";
const char* WorkTreeApp::s_obs = "{9C5B1BD3-AA3A-4edd-A66B-53C33E83120E}";
const char* WorkTreeApp::s_wbs = "{FA5EC2A6-F3B8-42d7-A02C-710DF6F14F1B}";
const char* WorkTreeApp::s_folders = "{8EB5894A-CC36-458f-9137-A6681F423CAB}";
const QUuid WorkTreeApp::s_calendars( "{26D78124-1623-4fbb-901C-A93BFB0C6619}" );
const QUuid WorkTreeApp::s_project( "{4CBED7F2-0DEC-4a14-8765-8545F4C625A3}" );

static WorkTreeApp* s_inst = 0;

static int type(lua_State * L)
{
	luaL_checkany(L, 1);
	if( lua_type(L,1) == LUA_TUSERDATA )
	{
		Lua::ValueBindingBase::pushTypeName( L, 1 );
		if( !lua_isnil( L, -1 ) )
			return 1;
		lua_pop( L, 1 );
	}
	lua_pushstring(L, luaL_typename(L, 1) );
	return 1;
}
WorkTreeApp::WorkTreeApp()
{
    s_inst = this;

    qApp->setOrganizationName( s_company );
	qApp->setOrganizationDomain( s_domain );
	qApp->setApplicationName( s_appName );

    qApp->setStyle( new QPlastiqueStyle() );

    d_set = new QSettings( s_appName, s_appName, this );

	d_styles = new Txt::Styles( this );
	Txt::Styles::setInst( d_styles );

	QDesktopServices::setUrlHandler( tr("xoid"), this, "onHandleXoid" ); // hier ohne SLOT und ohne Args ntig!

//	if( d_set->contains( "Outliner/Font" ) )
//	{
//		QFont f = d_set->value( "Outliner/Font" ).value<QFont>();
//		d_styles->setFontStyle( f.family(), f.pointSize() );
//	}
	Lua::Engine2::setInst( new Lua::Engine2() );
	Lua::Engine2::getInst()->addStdLibs();
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::PACKAGE );
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::OS );
	Lua::Engine2::getInst()->addLibrary( Lua::Engine2::IO );
	Lua::QtObjectBase::install( Lua::Engine2::getInst()->getCtx() );
	Qtl::Variant::install( Lua::Engine2::getInst()->getCtx() );
	Qtl::Objects::install( Lua::Engine2::getInst()->getCtx() );
	Udb::LuaBinding::install( Lua::Engine2::getInst()->getCtx() );
	Oln::LuaBinding::install( Lua::Engine2::getInst()->getCtx() );
	Wt::LuaBinding::install( Lua::Engine2::getInst()->getCtx() );
	lua_pushcfunction( Lua::Engine2::getInst()->getCtx(), type );
	lua_setfield( Lua::Engine2::getInst()->getCtx(), LUA_GLOBALSINDEX, "type" );
}

WorkTreeApp::~WorkTreeApp()
{
    foreach( MainWindow* w, d_docs )
        w->close(); // da selbst qApp->quit() zuerst alle Fenster schliesst, dürfte das nie vorkommen.
    s_inst = 0;
}

void WorkTreeApp::onOpen(const QString & path)
{
    if( !open( path ) )
    {
        if( d_docs.isEmpty() )
            qApp->quit();
    }
}

void WorkTreeApp::onClose()
{
    MainWindow* w = static_cast<MainWindow*>( sender() );
    if( d_docs.removeAll( w ) > 0 )
    {
       // w->deleteLater(); // Wird mit Qt::WA_DeleteOnClose von MainWindow selber gelöscht
    }
}

bool WorkTreeApp::open(const QString & path)
{
    foreach( MainWindow* w, d_docs )
    {
        if( w->getTxn()->getDb()->getFilePath() == path ) // TODO: Case sensitive paths?
        {
            w->activateWindow();
            w->raise();
            return true;
        }
    }
    Udb::Transaction* txn = 0;
	try
	{
        Udb::Database* db = new Udb::Database( this );
		db->open( path );
		db->setCacheSize( 10000 ); // RISK
        txn = new Udb::Transaction( db, this );
		WtTypeDefs::init( *db );
		txn->commit();
	}catch( Udb::DatabaseException& e )
	{
		QMessageBox::critical( 0, tr("Create/Open Repository"),
			tr("Error <%1>: %2").arg( e.getCodeString() ).arg( e.getMsg() ) );

        // Gib false zurück, wenn ansonsten kein Doc offen ist, damit Anwendung enden kann.
		return d_docs.isEmpty();
	}
    Q_ASSERT( txn != 0 );
	Wt::LuaBinding::setRepository( Lua::Engine2::getInst()->getCtx(), txn );
	MainWindow* w = new MainWindow( txn );
    connect( w, SIGNAL(closing()), this, SLOT(onClose()) );
    if( d_docs.isEmpty() )
    {
        setAppFont( w->font() );
    }
    d_docs.append( w );
    return true;
}

WorkTreeApp *WorkTreeApp::inst()
{
    return s_inst;
}

void WorkTreeApp::setAppFont( const QFont& f )
{
	d_styles->setFontStyle( f.family(), f.pointSize() );
}

void WorkTreeApp::onHandleXoid(const QUrl & url)
{
	const QString oid = url.userInfo();
	const QString uid = url.host();
	Udb::Database::runDatabaseApp( uid, QStringList() << tr("-oid:%1").arg(oid) );
}
