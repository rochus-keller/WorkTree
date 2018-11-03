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

#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Udb/DatabaseException.h>
#include <Udb/BtreeCursor.h>
#include <Udb/BtreeStore.h>
#include <Udb/ContentObject.h>
#include <Oln2/OutlineUdbMdl.h>
#include <QtGui/QApplication>
#include <QtApp/QtSingleApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QIcon>
#include <QtGui/QSplashScreen>
#include <QtCore/QtDebug>
#include "WorkTreeApp.h"
#include "WtTypeDefs.h"
#include "MainWindow.h"
using namespace Wt;

int main(int argc, char *argv[])
{
    QtSingleApplication app( WorkTreeApp::s_appName, argc, argv);

    QIcon icon;
	icon.addFile( ":/images/mu16.png" );
	icon.addFile( ":/images/mu32.png" );
	icon.addFile( ":/images/mu48.png" );
	icon.addFile( ":/images/mu64.png" );
	icon.addFile( ":/images/mu128.png" );
	app.setWindowIcon( icon );

    Oln::OutlineUdbMdl::registerPixmap( TypeOutlineItem, QString( ":/images/outline_item.png" ) );
	Oln::OutlineUdbMdl::registerPixmap( TypeOutline, QString( ":/images/outline.png" ) );
    //Oln::OutlineUdbMdl::registerPixmap( TypeIMP, QString( ":/images/project.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeTask, QString( ":/images/task.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeMilestone, QString( ":/images/milestone.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeImpEvent, QString( ":/images/event.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeAccomplishment, QString( ":/images/accomplishment.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeCriterion, QString( ":/images/criterion.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeOutline, QString( ":/images/outline.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeLink, QString( ":/images/chain.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeHuman, QString( ":/images/person.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeResource, QString( ":/images/equipment.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeRole, QString( ":/images/role.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeOrgUnit, QString( ":/images/orgunit.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeUnitAssig, QString( ":/images/unitassig.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeRasciAssig, QString( ":/images/wheel.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeDeliverable, QString( ":/images/deliverable.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeWork, QString( ":/images/gear.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypePdmDiagram, QString( ":/images/pdm-diagram.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeFolder, QString( ":/images/folder.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeCalendar, QString( ":/images/calendar.png" ) );
    Oln::OutlineUdbMdl::registerPixmap( TypeCalEntry, QString( ":/images/calendar-day.png" ) );
	Oln::OutlineUdbMdl::registerPixmap( Udb::ScriptSource::TID, QString( ":/images/lua-script.png" ) );

#ifndef _DEBUG
	try
	{
#endif
		WorkTreeApp ctx;
        QObject::connect( &app, SIGNAL( messageReceived(const QString&)),
                              &ctx, SLOT( onOpen(const QString&) ) );

		QString path;

		if( ctx.getSet()->contains( "App/Font" ) )
		{
			QFont f1 = QApplication::font();
			QFont f2 = ctx.getSet()->value( "App/Font" ).value<QFont>();
			f1.setFamily( f2.family() );
			f1.setPointSize( f2.pointSize() );
			QApplication::setFont( f1 );
            ctx.setAppFont( f1 );
		}else
            ctx.setAppFont( QApplication::font() );

		QStringList args = QCoreApplication::arguments();
		for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
		{
			if( !args[ i ].startsWith( '-' ) )
			{
				path = args[ i ];
				break;
			}
		}

		if( path.isEmpty() )
			path = QFileDialog::getSaveFileName( 0, WorkTreeApp::tr("Create/Open Repository - WorkTree"),
				QString(), QString( "*%1" ).arg( QLatin1String( WorkTreeApp::s_extension ) ),
				0, QFileDialog::DontConfirmOverwrite | QFileDialog::DontUseNativeDialog );
		if( path.isEmpty() )
			return 0;

        if( !path.toLower().endsWith( QLatin1String( WorkTreeApp::s_extension ) ) )
            path += QLatin1String( WorkTreeApp::s_extension );

#ifndef _DEBUG
        if( app.sendMessage( path ) )
                 return 0; // Eine andere Instanz ist bereits offen
#endif

        QSplashScreen splash( QPixmap( ":/images/mu.png" ) );
		QFont f = splash.font();
		f.setBold( true );
		// f.setPointSize( f.pointSize() * 1.5 );
		splash.setFont( f );
		splash.show();
		splash.showMessage( WorkTreeApp::tr("Loading WorkTree %1...").arg( WorkTreeApp::s_version ),
			Qt::AlignHCenter | Qt::AlignBottom, Qt::blue );
		app.processEvents();

        if( !ctx.open( path ) )
			return -1;

        splash.finish( ctx.getDocs().first() );

        const int res = app.exec();
		//ctx.getTxn()->getDb()->dump();
		return res;
#ifndef _DEBUG
	}catch( Udb::DatabaseException& e )
	{
		QMessageBox::critical( 0, WorkTreeApp::tr("WorkTree Error"),
			QString("Database Error: [%1] %2").arg( e.getCodeString() ).arg( e.getMsg() ), QMessageBox::Abort );
		return -1;
	}catch( std::exception& e )
	{
		QMessageBox::critical( 0, WorkTreeApp::tr("WorkTree Error"),
			QString("Generic Error: %1").arg( e.what() ), QMessageBox::Abort );
		return -1;
	}catch( ... )
	{
		QMessageBox::critical( 0, WorkTreeApp::tr("WorkTree Error"),
			QString("Generic Error: unexpected internal exception"), QMessageBox::Abort );
		return -1;
	}
#endif
	return 0;
}
