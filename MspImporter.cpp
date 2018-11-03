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

#include "MspImporter.h"
#ifdef _WIN32
#include <QAxObject>
#endif
#include <QMessageBox>
#include <QtGui/QFileDialog>
#include "WtTypeDefs.h"
#include "WorkTreeApp.h"
#include <Udb/Transaction.h>
#include "ObjectHelper.h"
#include <QtDebug>
using namespace Wt;

MspImporter::MspImporter(QObject *parent) :
    QObject(parent),d_mspApp(0),d_top(0)
{
}

MspImporter::~MspImporter()
{
    if( d_top )
        delete d_top;
    if( d_mspApp )
        delete d_mspApp;
}

bool MspImporter::prepareEngine(QWidget * parent)
{
    d_mspApp = new QAxObject( this );
    const bool res = d_mspApp->setControl( "MSProject.Application" );
    if( !res )
    {
        delete d_mspApp;
        d_mspApp = 0;
        QMessageBox::critical( parent, tr("Loading COM Object" ), tr("Cannot load MSProject.Application" ) );
        return false;
    }
    connect( d_mspApp, SIGNAL( exception ( int, const QString &, const QString &, const QString & ) ),
             this, SLOT(onException(int,QString,QString,QString)) );

    d_mspApp->setProperty( "Visible", true );
    // Die Dialoge und Fehlermeldungen von MSP kommen auch durch, wenn false.
    return true;
}

bool MspImporter::importMsp(QWidget * parent, Udb::Transaction * txn)
{
    Q_ASSERT( txn != 0 );
    d_errors.clear();
    d_counts = Counts();
    if( !prepareEngine( parent ) )
        return false;

    QString path = QFileDialog::getOpenFileName( parent, tr("Select Project File to Import - WorkTree"),
                                                 "C:/Temp", // TEST
                                                 tr("Project (*.mpp)") );
    if( path.isEmpty() )
        return true;
    QAxObject* pro = openProject( path );
    if( pro == 0 )
    {
        d_errors.append( tr("Could not open project: %1" ).arg( path ) );
        return false;
    }
    Udb::Obj imp = txn->getOrCreateObject( QUuid(WorkTreeApp::s_imp), TypeIMP );

    d_top = new Task();
    d_top->d_obj = ObjectHelper::createObject( TypeTask, imp );
    d_top->d_obj.setString( AttrText, pro->property("Name").toString() );
    d_top->d_obj.setString( imp.getAtom("SourceFile"), pro->property("FullName").toString() );
    d_top->d_path = pro->property("FullName").toString();

    const QString name = QFileInfo( d_top->d_path ).completeBaseName();
    Q_ASSERT( !d_projects.contains( name ) );
    d_projects[ name ] = d_top;

    QAxObject* tasks = pro->querySubObject( "OutlineChildren" );
    Q_ASSERT( tasks != 0 );
    bool res = importTasksImp( tasks, d_top, d_top );
    if( res )
        res = importLinksImp( d_top );

    delete tasks;
    delete pro;
    delete d_top;
    d_top = 0;
    d_projects.clear();
    d_tasks.clear();

    if( res )
        txn->commit();
    else
        txn->rollback();
    return res;
}

void MspImporter::onException(int code, const QString &source, const QString &desc, const QString &help)
{
    d_errors.append( tr("Exception in %1: %2" ).arg( source ).arg( desc ) );
}

bool MspImporter::importTasksImp(QAxObject *mspTasks, Task *superTask, Task *project)
{
    if( mspTasks == 0 )
        return true;
    for( int i = 1; i <= mspTasks->property("Count").toInt(); i++ )
    {
        QAxObject* mspTask = mspTasks->querySubObject( "Item(int)", i );
        Q_ASSERT( mspTask != 0 );
//        qDebug() << "Handling " << mspTask->property( "ID" ).toInt() <<
//                    mspTask->property( "Name" ).toString(); // TEST
        if( mspTask->property("ExternalTask").toBool() )
        {
            // Wir ignorieren hier die Platzhalter-Tasks
            delete mspTask;
        }else
        {
            Task* newTask = new Task( superTask );
            newTask->d_path = mspTask->property( "Subproject" ).toString();
            newTask->d_task = mspTask;
            newTask->d_id = mspTask->property( "ID" ).toInt();
            Q_ASSERT( project != 0 );
            newTask->d_pro = project;
            d_tasks[ qMakePair( project, newTask->d_id ) ] = newTask;

            bool ms = mspTask->property( "Milestone" ).toBool();
            newTask->d_obj = ObjectHelper::createObject( (ms)?TypeMilestone:TypeTask, superTask->d_obj );
            if( ms )
                d_counts.milestones++;
            else
                d_counts.tasks++;
            if( superTask->d_obj.getType() == TypeMilestone )
                superTask->d_obj.setType( TypeTask );
            // NOTE: AttrSubTMSCount wird in createObject gesetzt
            // NOTE: wir korrigieren hier counts nicht, da sonst Abweichung zu Health Check
            fetchTaskAttributes( newTask, ms );

            if( !newTask->d_path.isEmpty() )
            {
                // Es ist ein Subproject. Schauen wir mal, ob wir die Datei finden.
                QAxObject* pro = openProject( newTask->d_path );
                if( pro )
                {
                    pro->setParent( mspTasks ); // ansonsten wird mit delete app pro gelöscht
                    QString name = QFileInfo( newTask->d_path ).completeBaseName();
                    Q_ASSERT( !d_projects.contains( name ) );
                    d_projects[ name ] = newTask;
                    if( !importTasksImp( pro->querySubObject( "OutlineChildren" ), newTask, newTask ) )
                        return false;
                }else
                {
                    d_errors.append( tr("Cannot expand to project file '%1'").arg( newTask->d_path ) );
                    return false;
                }
            }else
            {
                if( !importTasksImp( mspTask->querySubObject( "OutlineChildren" ), newTask, project ) )
                    return false;
            }
        }
    }
    return true;
}

static bool _isElapsed( const QString& str )
{
    // TODO: berücksichtigen von elapsed bzw. fortlaufende durations: 1 Tag vs 1 fTag
    const int spacePos = str.indexOf( QChar(' ') );
    if( spacePos != -1 )
        return str[ spacePos + 1 ] == QChar('e') || str[ spacePos + 1 ] == QChar('f');
    return false;
}

void MspImporter::fetchTaskAttributes(MspImporter::Task * task, bool ms)
{
    Q_ASSERT( task != 0 && !task->d_obj.isErased() );

    task->d_obj.setString( AttrText, task->d_task->property("Name").toString() );
    task->d_obj.setString( AttrCustomId, task->d_task->property("Text15").toString() ); // TEST, RISK
    const QDate earlyStart =  task->d_task->property( "EarlyStart" ).toDate();
    if( earlyStart.isValid() )
        task->d_obj.setValue( AttrEarlyStart, Stream::DataCell().setDate( earlyStart ) );
    const QDate earlyFinish = task->d_task->property( "EarlyFinish" ).toDate();
    if( earlyFinish.isValid() && !ms )
        task->d_obj.setValue( AttrEarlyFinish, Stream::DataCell().setDate( earlyFinish ) );
    const QDate lateStart =  task->d_task->property( "LateStart" ).toDate();
    if( lateStart.isValid() )
        task->d_obj.setValue( AttrLateStart, Stream::DataCell().setDate( lateStart ) );
    const QDate lateFinish = task->d_task->property( "LateFinish" ).toDate();
    if( lateFinish.isValid() && !ms )
        task->d_obj.setValue( AttrLateFinish, Stream::DataCell().setDate( lateFinish ) );
    const int duration = task->d_task->property("Duration").toInt();
    // Task.Duration ist in Minuten geführt; der Tag hat jedoch nicht 24h, sondern nur 8! RISK
    // VORSICHT: 1 t hat 480 Minuten (1 * 8 * 60), 1 ft jedoch 1440 (1 * 24 * 60); verifiziert in ProjectAnalyzer
    // Es ist jedes Feld identisch eines Tasks mit ft oder t; kein Unterschied erkennbar.
    if( !ms )
    {
        // Folgender Wert wird nur ab MSP 2010 angezeigt!
        const bool elapsed = _isElapsed( task->d_task->property("DurationText").toString().trimmed() );
        task->d_obj.setValue( AttrDuration, Stream::DataCell().setUInt16(
                                  duration / ( ( (elapsed)?24:8 ) * 60 ) ) ); // RISK
    }
    if( task->d_task->property("Critical").toBool() )
        task->d_obj.setValue( AttrCriticalPath, Stream::DataCell().setBool( true ) );
}

MspImporter::Link MspImporter::parseLink(const QString & s)
{
    if( s.isEmpty() )
        return Link();
    // "2;C:\TEMP\PM-Test\P2.mpp\3": Die angegebenen Nummern sind ID
    // (nicht UniqueID). Auch die Formen "<>\P-1012-MALN - MCPs\13", "209EA+9 W", "214EA+1 t"
    // kommen vor (<> wenn Datei nicht gefunden); oder "C:\TEMP\P-1012-MALN - MCPs.mpp\115EA+4 t"
    Link l;
    Q_ASSERT( s.indexOf( ';' ) == -1 ); // s muss bereits aufgeteilt sein
    if( s[0].isDigit() )
    {
        // Einfaches Format; ID kommt direkt, "209EA+9 W", "214EA+1 t"
        l.parseId( s );
    }else
    {
        // Vollständiger Pfad, "<>\P-1012-MALN - MCPs\13", "C:\TEMP\P-1012-MALN - MCPs.mpp\115EA+4 t"
        const int pos2 = s.lastIndexOf( QChar('\\') );
        Q_ASSERT( pos2 != -1 );
        const int pos1 = s.lastIndexOf( QChar('\\'), pos2 - 1 );
        Q_ASSERT( pos1 != -1 );
        QString fileName = s.mid( pos1 + 1, pos2 - pos1 - 1 );
        l.d_file = QFileInfo( fileName ).completeBaseName();
        l.parseId( s.mid( pos2 + 1 ) );
    }
    return l;
}

void MspImporter::Link::parseId(const QString & str)
{
    d_id = 0;
    d_kind.clear();
    d_lag = 0;
    d_unit.clear();
    // "209", "209EA+9 W", "214EA-1 t"
    int idLen = 0;
    for( int i = 0; i < str.size(); i++ )
        if( str[i].isDigit() )
            idLen++;
        else
            break;
    Q_ASSERT( idLen > 0 );
    d_id = str.left( idLen ).toInt();

    int kindLen = 0;
    for( int i = idLen; i < str.size(); i++ )
        if( str[i].isLetter() )
            kindLen++;
        else
            break;
    if( kindLen == 0 )
    {
        d_kind = "EA"; // RISK: das ist in MSP anscheinend der implizite Default, wenn nichts steht
        return;
    }
    d_kind = str.mid( idLen, kindLen );

    int lagLen = 0;
    for( int i = idLen+kindLen+1; i < str.size(); i++ )
        if( str[i].isDigit() || str[i] == QChar('.') )
            lagLen++;
        else
            break;
    if( lagLen == 0 )
        return;
    Q_ASSERT( str[idLen+kindLen]==QChar('+') || str[idLen+kindLen]==QChar('-') );
    d_lag = str.mid( idLen+kindLen, lagLen + 1).toFloat();
    QString unit = str.mid( idLen+kindLen+lagLen + 1 ).simplified();
    if( !unit.isEmpty() )
        d_unit = unit;
}

const MspImporter::Task *MspImporter::findLinkTarget(const Task *sourceNode, const Link &link, bool* ok )
{
    if( ok )
        *ok = true;
    Task* target = 0;
    if( link.d_file.isEmpty() )
    {
        // Der Task müsste lokal bekannt sein
        Q_ASSERT( sourceNode->d_pro );
        target = d_tasks.value( qMakePair( sourceNode->d_pro, link.d_id ) );
        if( target == 0 && ok )
            *ok = false;
    }else
    {
        // Der Task gehört in eine andere Datei
        Task* project = d_projects.value( link.d_file );
        if( project )
        {
            // Projekt bekannt, also müsste auch Task bekannt sein
            target = d_tasks.value( qMakePair( project, link.d_id ) );
            if( target == 0 && ok )
                *ok = false;
        }// else Projekt unbekannt
    }
    return target;
}

QString MspImporter::Task::getName() const
{
    return QFileInfo( d_path ).completeBaseName();
}

bool MspImporter::importLinksImp(MspImporter::Task * task )
{
    QStringList successors;
    if( task->d_task )
        successors = task->d_task->property( "Successors" ).
                                   toString().split( QChar(';') );
    foreach( QString str, successors )
    {
        Link succ = parseLink( str.trimmed() );
        const Task* toTask = 0;
        bool ok;
        if( succ.d_id )
        {
            toTask = findLinkTarget( task, succ, &ok );
            if( !ok )
            {
                qDebug() << "Link Error" << task->d_task->property( "Text15" ) << "->" <<  str;
                return false;
            }
            if( toTask )
            {
                Udb::Obj link = ObjectHelper::createObject( TypeLink, task->d_obj );
                d_counts.links++;

                link.setValue( AttrLinkType, Stream::DataCell().setUInt8( succ.getLinkType() ) );
                if( task->d_obj.getValue( AttrCriticalPath ).getBool() &&
                        toTask->d_obj.getValue( AttrCriticalPath ).getBool() )
                    link.setValue( AttrCriticalPath, Stream::DataCell().setBool(true) );

                const int lag = succ.getLagDays();
                if( lag > 0 )
                {
                    d_counts.lags++;
                    Udb::Obj svt = ObjectHelper::createObject( TypeTask, task->d_obj.getParent() );
                    svt.setString( AttrText, tr("Lag %1 to %2").
                                    arg( WtTypeDefs::formatObjectId( task->d_obj ) ).
                                    arg( WtTypeDefs::formatObjectId( toTask->d_obj ) ) );

                    svt.setValue( AttrTaskType, Stream::DataCell().setUInt8( TaskType_SVT ) );
                    svt.setValue( AttrEarlyStart, task->d_obj.getValue( AttrEarlyFinish ) );
                    svt.setValue( AttrEarlyFinish, toTask->d_obj.getValue( AttrEarlyStart ) );
                    svt.setValue( AttrLateStart, task->d_obj.getValue( AttrLateFinish ) );
                    svt.setValue( AttrLateFinish, toTask->d_obj.getValue( AttrLateStart ) );
                    svt.setValue( AttrDuration, Stream::DataCell().setUInt16( lag ) );
                    svt.setValue( AttrCriticalPath, link.getValue( AttrCriticalPath ) );

                    link.setValueAsObj( AttrPred, task->d_obj );
                    link.setValueAsObj( AttrSucc, svt );
                    link = ObjectHelper::createObject( TypeLink, svt );
                    link.setValueAsObj( AttrPred, svt );
                    link.setValueAsObj( AttrSucc, toTask->d_obj );
                    link.setValue( AttrCriticalPath, svt.getValue( AttrCriticalPath ) );
                }else
                {
                    // Ignoring leads
                    if( lag < 0 )
                        d_counts.leads++;
                    link.setValueAsObj( AttrPred, task->d_obj );
                    link.setValueAsObj( AttrSucc, toTask->d_obj );
                }
            }else
            {
                // Target not found
                qDebug() << "Link Error" << task->d_task->property( "Text15" ) << "->" <<  str;
                d_counts.deadLinks++;
                // task->d_obj.incCounter( task->d_obj.getAtom("External Successors") );
            }
        }
    }
    foreach( Task* sub, task->d_children )
        if( !importLinksImp( sub ) )
            return false;
    return true;
}

QAxObject *MspImporter::openProject(const QString &path)
{
    if( d_mspApp == 0 )
        return 0;
    if( QFileInfo( path ).isReadable() )
    {
        if( !d_mspApp->dynamicCall( "FileOpenEx(const QString&, bool )", path, false ).toBool() )
            // wenn true kommen die ganze Zeit blöde Warnungen; man kann stattdessen die Dateien auf ReadOnly setzen
            return 0;
        return d_mspApp->querySubObject( "ActiveProject" );
    }else
        return 0;
}

MspImporter::Task::~Task()
{
    foreach( Task* s, d_children )
        delete s;
    if( d_parent == 0 )
        delete d_task;
    // Wir löschen den obersten Task; die anderen sind Kinder davon
}

int MspImporter::Link::getLinkType() const
{
    if( d_kind == "EE" )
        return LinkType_FF;
    else if( d_kind == "AA" )
        return LinkType_SS;
    else if( d_kind == "AE" )
        return LinkType_SF;
    else
        return LinkType_FS;
}

int MspImporter::Link::getLagDays() const
{
    // RISK: das funktioniert nur in der deutschen und englischen Version richtig.
    QString u = d_unit.toLower().trimmed();
    // Elapsed durations are scheduled 24 hours a day, 7 days a week, until fnished.
    // That is, one day is always considered 24 hours long (rather than 8 hours),
    // and one week is always 7 days (rather than 5 days).
    // TODO
    if( u.startsWith( "f" ) || u.startsWith( "e" ) )
        u = u.mid( 1 );
    if( u == "t" || u == "d" || u == "dy" ) // Tage
        return d_lag + 0.5;
    else if( u == "w" || u == "wk" ) // Wochen
        return d_lag * 5.0 + 0.5;
    else if( u == "h" || u == "hr") // Stunden
        return d_lag / 8.0 + 0.5;
    else if( u == "m" || u == "min" ) // Minuten
        return d_lag / ( 8.0 * 60.0 ) + 0.5;
    else if( u == "mo" || u == "mon" ) // Monat
        return d_lag * 20.0 + 0.5;
    else if( u.isEmpty() )
        return d_lag + 0.5;
    else
        Q_ASSERT( false );
    return 0;
}

QString MspImporter::Link::renderId() const
{
    if( d_file.isEmpty() )
        return QString("?/%1").arg( d_id );
    else
        return QString("%1/%2").arg( d_file ).arg( d_id );
}
