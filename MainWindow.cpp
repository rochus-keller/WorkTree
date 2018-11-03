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

#include "MainWindow.h"
#include "WtLuaBinding.h"
#include <QtGui/QDockWidget>
#include <QtGui/QCloseEvent>
#include <QtGui/QTreeView>
#include <QtGui/QApplication>
#include <QtGui/QFontDialog>
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>
#include <Oln2/OutlineUdbCtrl.h>
#include "ImpCtrl.h"
#include "WorkTreeApp.h"
#include "WtTypeDefs.h"
#include "AttrViewCtrl.h"
#include "TextViewCtrl.h"
#include <CrossLine/DocTabWidget.h>
#include <Gui2/AutoShortcut.h>
#include <Udb/Database.h>
#include "PdmItemMdl.h"
#include "PdmItemView.h"
#include "PdmCtrl.h"
#include "SceneOverview.h"
#include "PdmLinkViewCtrl.h"
#include "ObsCtrl.h"
#include "AssigViewCtrl.h"
#include "SearchView.h"
#include "WbsCtrl.h"
#include "MspImporter.h"
#include "FolderCtrl.h"
#include "WpViewCtrl.h"
#include "CalendarEditor.h"
#include <QtDebug>
#include <Script/CodeEditor.h>
#include <Script/Terminal2.h>
#include <Udb/ContentObject.h>
using namespace Wt;

class _MyDockWidget : public QDockWidget
{
public:
	_MyDockWidget( const QString& title, QWidget* p ):QDockWidget(title,p){}
	virtual void setVisible( bool visible )
	{
		QDockWidget::setVisible( visible );
		if( visible )
			raise();
	}
};

static inline QDockWidget* createDock( QMainWindow* parent, const QString& title, quint32 id, bool visi )
{
	QDockWidget* dock = new _MyDockWidget( title, parent );
	if( id )
		dock->setObjectName( QString( "{%1" ).arg( id, 0, 16 ) );
	else
		dock->setObjectName( QChar('[') + title );
	dock->setAllowedAreas( Qt::AllDockWidgetAreas );
	dock->setFeatures( QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
		QDockWidget::DockWidgetFloatable );
	dock->setFloating( false );
	dock->setVisible( visi );
	return dock;
}

MainWindow::MainWindow(Udb::Transaction * txn):d_txn(txn),d_pushBackLock(false),d_selectLock(false),d_fullScreen(false)
{
    Q_ASSERT( txn != 0 );

	d_tab = new Oln::DocTabWidget( this, false );
	d_tab->setCloserIcon( ":/images/close.png" );
	setCentralWidget( d_tab );
    connect( d_tab, SIGNAL( currentChanged(int) ), this, SLOT(onTabChanged(int) ) );
	connect( d_tab, SIGNAL(closing(int)), this, SLOT(saveEditor()) );

	setDockNestingEnabled(true);
    setCorner( Qt::BottomRightCorner, Qt::BottomDockWidgetArea );
	setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
	setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
	setCorner( Qt::TopLeftCorner, Qt::TopDockWidgetArea );

    setupAttrView();
    setupTextView();
    setupImp();
    setupOverview();
    setupLinkView();
    setupAssigView();
    setupObs();
    setupWbs();
    setupSearchView();
    setupFolders();
    setupWbView();
	setupTerminal();
#ifdef _WIN32
    d_msp = new MspImporter( this );
#endif


    setAttribute( Qt::WA_DeleteOnClose );

    QVariant state = WorkTreeApp::inst()->getSet()->value( "MainFrame/State/" +
		d_txn->getDb()->getDbUuid().toString() ); // Da DB-individuelle Docks
	if( !state.isNull() )
		restoreState( state.toByteArray() );

    if( WorkTreeApp::inst()->getSet()->value( "MainFrame/State/FullScreen" ).toBool() )
    {
        toFullScreen( this );
        d_fullScreen = true;
    }else
    {
        showNormal();
        d_fullScreen = false;
    }


    setCaption();
    setWindowIcon( qApp->windowIcon() ); // Ist nötig, da ansonsten auf Linux das Fenster keine Ikone hat!

    connect( d_tab, SIGNAL( currentChanged ( int  ) ), this, SLOT( onTabChanged( int ) ) );
    // zuletzt, das sonst d_ov nicht bereit


    // Tab Menu
	Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_tab, true );
    pop->addCommand( tr("Forward Tab"), d_tab, SLOT(onDocSelect()), tr("CTRL+TAB"), true );
    pop->addCommand( tr("Backward Tab"), d_tab, SLOT(onDocSelect()), tr("CTRL+SHIFT+TAB"), true );
    pop->addCommand( tr("Close Tab"), d_tab, SLOT(onCloseDoc()), tr("CTRL+W"), true );
    pop->addCommand( tr("Close All"), d_tab, SLOT(onCloseAll()) );
    pop->addCommand( tr("Close All Others"), d_tab, SLOT(onCloseAllButThis()) );
    addTopCommands( pop );
    new Gui2::AutoShortcut( tr("F11"), this, this, SLOT(onFullScreen()) );
    new Gui2::AutoShortcut( tr("CTRL+F"), this, this, SLOT(onSearch()) );
    new Gui2::AutoShortcut( tr("CTRL+Q"), this, this, SLOT(close()) );
    new Gui2::AutoShortcut( tr("ALT+LEFT"), this,  this, SLOT(onGoBack()) );
    new Gui2::AutoShortcut( tr("ALT+RIGHT"), this,  this, SLOT(onGoForward()) );
    new Gui2::AutoShortcut( tr("ALT+UP"), this, this,  SLOT(onShowSuperTask()) );
    new Gui2::AutoShortcut( tr("ALT+DOWN"), this,  this, SLOT(onShowSubTask()) );
    new Gui2::AutoShortcut( tr("ALT+HOME"), this,  this, SLOT(onFollowAlias()) );

	onFollowObject( WtTypeDefs::getRoot(d_txn).getValueAsObj(AttrAutoOpen) );
}

void MainWindow::setCaption()
{
	QFileInfo info( d_txn->getDb()->getFilePath() );
    setWindowTitle( tr("%1 - WorkTree").arg( info.baseName() ) );
}

void MainWindow::toFullScreen(QMainWindow * w)
{
#ifdef _WIN32
    w->showFullScreen();
#else
    w->setWindowFlags( Qt::Window | Qt::FramelessWindowHint  );
    w->showMaximized();
    //w->setWindowIcon( qApp->windowIcon() );
#endif
}

void MainWindow::addTopCommands(Gui2::AutoMenu * pop)
{
    Q_ASSERT( pop != 0 );
    pop->addSeparator();
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Go" ), pop );
    pop->addMenu( sub );
    sub->addCommand( tr("Back"),  this, SLOT(onGoBack()), tr("ALT+LEFT") );
    sub->addCommand( tr("Forward"), this, SLOT(onGoForward()), tr("ALT+RIGHT") );
    pop->addCommand( tr("Search..."),  this, SLOT(onSearch()), tr("CTRL+F") );
    pop->addSeparator();
    QMenu* sub2 = createPopupMenu();
	sub2->setTitle( tr("Show Window") );
	pop->addMenu( sub2 );
    sub = new Gui2::AutoMenu( tr("Configuration" ), pop );
    pop->addMenu( sub );
	sub->addCommand( tr("Set Application Font..."), this, SLOT(onSetFont()) );
	sub->addCommand( "Set Script Font...", this, SLOT(onSetScriptFont()) );
	sub->addCommand( tr("Calendars..."), this, SLOT(onCalendars()) );
    sub->addCommand( tr("Full Screen"), this, SLOT(onFullScreen()), tr("F11") )->setCheckable(true);

    pop->addCommand( tr("About WorkTree..."), this, SLOT(onAbout()) );
    pop->addSeparator();
    pop->addAction( tr("Quit"), this, SLOT(close()), tr("CTRL+Q") );

}

void MainWindow::showInPdmDiagram( const Udb::Obj &select, bool checkAllOpenDiagrams )
{
    if( !select.isNull() )
    {
        // Wenn das Item im aktuell angezeigten Diagramm vorhanden ist, nehme das
        PdmCtrl* c = getCurrentPdmDiagram();
        if( c && c->focusOn( select ) )
            return;
    }
    if( !checkAllOpenDiagrams )
        return;
    const int pos = d_tab->findDoc( (select.getType()==TypeLink)?
                                        select.getParent().getParent():
                                        select.getParent() );
    if( pos != -1 )
    {
        // Wenn der Parent des Items aktuell offen ist, nehme das
        if( PdmItemView* v = dynamic_cast<PdmItemView*>( d_tab->widget(pos) ) )
        {
            if( v->getCtrl()->focusOn( select ) )
                d_tab->setCurrentIndex( pos );
            return;
        }
    }
    // else schaue in allen anderen offenen Diagrammen
    for( int i = 0; i < d_tab->count(); i++ )
    {
        if( PdmItemView* v = dynamic_cast<PdmItemView*>( d_tab->widget(i) ) )
        {
            if( v->getCtrl()->focusOn( select ) )
                d_tab->setCurrentIndex( i );
            return;
        }
    }
}

void MainWindow::openPdmDiagram(const Udb::Obj & doc, const Udb::Obj &select )
{
    if( doc.isNull() || !WtTypeDefs::isPdmDiagram( doc.getType() ) )
        return;
    if( d_tab->showDoc( doc ) != -1 )
    {
        // Wenn der Parent des Items aktuell offen ist, nehme das
        PdmCtrl* c = getCurrentPdmDiagram();
        if( c )
            c->focusOn( select );
        return;
    }
    // Ansonsten öffne den Parent des Diagramms
    QApplication::setOverrideCursor( Qt::WaitCursor );

	PdmCtrl* ctrl = PdmCtrl::create( d_tab, doc );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( ctrl->getView(), true );
    pop->addCommand( tr( "Show Subtask" ), this, SLOT( onShowSubTask() ), tr("ALT+DOWN") );
    pop->addCommand( tr( "Show Supertask" ), this, SLOT( onShowSuperTask() ), tr("ALT+UP") );
    pop->addCommand( tr( "Follow Alias" ), this, SLOT( onFollowAlias() ), tr("ALT+HOME") );
    pop->addSeparator();
    ctrl->addCommands( pop );
    addTopCommands( pop );
    connect( ctrl, SIGNAL(signalSelectionChanged()), this, SLOT(onPdmSelection()) );

    d_tab->addDoc( ctrl->getView(), doc, WtTypeDefs::formatObjectTitle( doc ) );
    ctrl->focusOn( select );

    QApplication::restoreOverrideCursor();
}

void MainWindow::openOutline(const Udb::Obj &doc, const Udb::Obj &select)
{
    if( doc.isNull() )
        return;
    const int pos = d_tab->showDoc( doc );
    if( pos != -1 )
    {
        const QObjectList& ol = d_tab->widget(pos)->children();
        for( int i = 0; i < ol.size(); i++ )
        {
            if( Oln::OutlineUdbCtrl* ctrl = dynamic_cast<Oln::OutlineUdbCtrl*>( ol[i] ) )
            {
                if( ctrl->gotoItem( select.getOid() ) )
                    ctrl->getTree()->expand( ctrl->getMdl()->getIndex( select.getOid() ) );
                return;
            }
        }
        return;
    }
    QApplication::setOverrideCursor( Qt::WaitCursor );

	Oln::OutlineUdbCtrl* oln = Oln::OutlineUdbCtrl::create( d_tab, d_txn );
	oln->getDeleg()->setShowIcons( true );

    connect( oln, SIGNAL(sigLinkActivated(quint64) ), this, SLOT(onFollowOID(quint64) ) );
    connect( oln, SIGNAL(sigUrlActivated(QUrl) ), this, SLOT(onUrlActivated(QUrl)) );

    Gui2::AutoMenu* pop = new Gui2::AutoMenu( oln->getTree(), true );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Item" ), pop );
	pop->addMenu( sub );
    oln->addItemCommands( sub );
	sub = new Gui2::AutoMenu( tr("Text" ), pop );
	pop->addMenu( sub );
    oln->addTextCommands( sub );
	pop->addCommand( tr("Open on Start"), this, SLOT(onAutoStart()) );
	pop->addSeparator();
    oln->addOutlineCommands( pop );
    addTopCommands( pop );

    oln->setOutline( doc );

    d_tab->addDoc( oln->getTree(), doc, WtTypeDefs::formatObjectTitle( doc ) );
    oln->gotoItem( select.getOid() );
	QApplication::restoreOverrideCursor();
}

void MainWindow::openScript(const Udb::Obj &doc)
{
	if( doc.isNull() )
		return;
	const int pos = d_tab->showDoc( doc );
	if( pos != -1 )
	{
		return;
	}
	Udb::ScriptSource script(doc);

	Lua::CodeEditor* e = new Lua::CodeEditor( d_tab );

	Gui2::AutoMenu* pop = new Gui2::AutoMenu( e, true );
	pop->addCommand( "&Execute",this, SLOT(handleExecute()), tr("CTRL+E"), true );
	pop->addSeparator();
	pop->addCommand( "Undo", e, SLOT(handleEditUndo()), tr("CTRL+Z"), true );
	pop->addCommand( "Redo", e, SLOT(handleEditRedo()), tr("CTRL+Y"), true );
	pop->addSeparator();
	pop->addCommand( "Cut", e, SLOT(handleEditCut()), tr("CTRL+X"), true );
	pop->addCommand( "Copy", e, SLOT(handleEditCopy()), tr("CTRL+C"), true );
	pop->addCommand( "Paste", e, SLOT(handleEditPaste()), tr("CTRL+V"), true );
	pop->addCommand( "Select all", e, SLOT(handleEditSelectAll()), tr("CTRL+A"), true  );
	pop->addSeparator();
	pop->addCommand( "Find...", e, SLOT(handleFind()), tr("CTRL+F"), true );
	pop->addCommand( "Find again", e, SLOT(handleFindAgain()), tr("F3"), true );
	pop->addCommand( "Replace...", e, SLOT(handleReplace()), tr("CTRL+R"), true );
	pop->addCommand( "&Goto...", e, SLOT(handleGoto()), tr("CTRL+G"), true );
	pop->addSeparator();
	pop->addCommand( "Indent", e, SLOT(handleIndent()) );
	pop->addCommand( "Unindent", e, SLOT(handleUnindent()) );
	pop->addCommand( "Set Indentation Level...", e, SLOT(handleSetIndent()) );
	pop->addSeparator();
	pop->addCommand( "Print...", e, SLOT(handlePrint()) );
	pop->addCommand( "Export PDF...", e, SLOT(handleExportPdf()) );
	addTopCommands( pop );

	QSettings set;
	const QVariant v = set.value( "LuaEditor/Font" );
	if( !v.isNull() )
		e->setFont( v.value<QFont>() );

	e->setText( script.getSource() );
	e->setName( script.getText() );
	e->document()->setModified( false );
	d_tab->addDoc( e, doc, script.getText() );
	connect( e, SIGNAL(blockCountChanged(int)), this, SLOT(saveEditor() ) );
}

PdmCtrl *MainWindow::getCurrentPdmDiagram() const
{
    PdmItemView* v = dynamic_cast<PdmItemView*>( d_tab->getCurrentTab() );
    if( v )
        return v->getCtrl();
    else
        return 0;
}

void MainWindow::pushBack(const Udb::Obj & o)
{
    if( d_pushBackLock )
        return;
    if( o.isNull( true, true ) )
        return;
	Wt::LuaBinding::setCurrentObject( o );
	if( !d_backHisto.isEmpty() && d_backHisto.last() == o.getOid() )
        return; // o ist bereits oberstes Element auf dem Stack.
    d_backHisto.removeAll( o.getOid() );
    d_backHisto.push_back( o.getOid() );
}

MainWindow::~MainWindow()
{
    if( d_txn )
        delete d_txn;
}

void MainWindow::setupImp()
{
    QDockWidget* dock = createDock( this, tr("Integrated Master Plan"), 0, true );

    Udb::Obj root = d_txn->getOrCreateObject( QUuid(WorkTreeApp::s_imp), TypeIMP );
    root.setString( AttrText, WtTypeDefs::prettyName( TypeIMP ) );
	d_txn->commit();
    d_imp = ImpCtrl::create( dock, root );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_imp->getTree(), true );
    pop->addCommand( tr("Open Diagram"), this, SLOT(onOpenPdmDiagram()) );
    pop->addSeparator();
    d_imp->addCommands( pop );
    pop->addCommand( tr("Import MS Project..."), this, SLOT(onImportMsp() ) );
    addTopCommands( pop );
    connect( d_imp, SIGNAL(signalSelected(Udb::Obj)), this, SLOT(onImpSelected(Udb::Obj)) );
    connect( d_imp, SIGNAL(signalDblClicked(Udb::Obj)), this, SLOT( onImpDblClicked(Udb::Obj)));
    dock->setWidget( d_imp->getTree() );
    addDockWidget( Qt::LeftDockWidgetArea, dock );
}

void MainWindow::setupObs()
{
    QDockWidget* dock = createDock( this, tr("Organizational Breakdown"), 0, true );

    Udb::Obj root = d_txn->getOrCreateObject( QUuid(WorkTreeApp::s_obs), TypeOBS );
    root.setString( AttrText, WtTypeDefs::prettyName( TypeOBS ) );
	d_txn->commit();
    d_obs = ObsCtrl::create( dock, root );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_obs->getTree(), true );
    d_obs->addCommands( pop );
    addTopCommands( pop );
    connect( d_obs, SIGNAL(signalSelected(Udb::Obj)), this, SLOT(onObsSelected(Udb::Obj)) );
    dock->setWidget( d_obs->getTree() );
    addDockWidget( Qt::RightDockWidgetArea, dock );
}

void MainWindow::setupWbs()
{
    QDockWidget* dock = createDock( this, tr("Work Breakdown"), 0, true );

    Udb::Obj root = d_txn->getOrCreateObject( QUuid(WorkTreeApp::s_wbs), TypeWBS );
    root.setString( AttrText, WtTypeDefs::prettyName( TypeWBS ) );
	d_txn->commit();
    d_wbs = WbsCtrl::create( dock, root );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_wbs->getTree(), true );
    d_wbs->addCommands( pop );
    addTopCommands( pop );
    connect( d_wbs, SIGNAL(signalSelected(Udb::Obj)), this, SLOT(onWbsSelected(Udb::Obj)) );
    dock->setWidget( d_wbs->getTree() );
    addDockWidget( Qt::RightDockWidgetArea, dock );
}

void MainWindow::setupAttrView()
{
    QDockWidget* dock = createDock( this, tr("Object Attributes"), 0, true );
    d_atv = AttrViewCtrl::create( dock, d_txn );
    dock->setWidget( d_atv->getWidget() );
    addDockWidget( Qt::BottomDockWidgetArea, dock );
    connect( d_atv,SIGNAL(signalFollowObject(Udb::Obj)), this,SLOT(onFollowObject(Udb::Obj)) );
}

void MainWindow::setupTextView()
{
    QDockWidget* dock = createDock( this, tr("Object Text"), 0, true );
	d_tv = TextViewCtrl::create( dock, d_txn );
    Gui2::AutoMenu* pop = d_tv->createPopup();
    addTopCommands( pop );
    dock->setWidget( d_tv->getWidget() );
    addDockWidget( Qt::BottomDockWidgetArea, dock );
    connect( d_tv,SIGNAL(signalFollowObject(Udb::Obj)), this,SLOT(onFollowObject(Udb::Obj)) );
}

void MainWindow::setupOverview()
{
    QDockWidget* dock = createDock( this, tr("Overview"), 0, true );
    d_ov = new SceneOverview( dock );
	dock->setWidget( d_ov );
    addDockWidget( Qt::LeftDockWidgetArea, dock );
}

void MainWindow::setupLinkView()
{
    QDockWidget* dock = createDock( this, tr("Links"), 0, true );
    d_lv = PdmLinkViewCtrl::create( dock, d_txn );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_lv->getWidget(), true );
    d_lv->addCommands( pop );
    connect( d_lv, SIGNAL(signalSelect(Udb::Obj,bool)), this, SLOT(onLinkSelected(Udb::Obj,bool)) );
    addTopCommands( pop );
    dock->setWidget( d_lv->getWidget() );
    addDockWidget( Qt::BottomDockWidgetArea, dock );
}

void MainWindow::setupAssigView()
{
    QDockWidget* dock = createDock( this, tr("Assignments"), 0, true );
    d_asv = AssigViewCtrl::create( dock, d_txn );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_asv->getWidget(), true );
    d_asv->addCommands( pop );
    connect( d_asv, SIGNAL(signalSelect(Udb::Obj,bool)), this, SLOT(onAssigSelected(Udb::Obj,bool)) );
    addTopCommands( pop );
    dock->setWidget( d_asv->getWidget() );
    addDockWidget( Qt::BottomDockWidgetArea, dock );
}

void MainWindow::setupSearchView()
{
    d_sv = new SearchView( this, d_txn );
	QDockWidget* dock = createDock( this, tr("Search" ), 0, false );
	dock->setWidget( d_sv );
	addDockWidget( Qt::RightDockWidgetArea, dock );

    connect( d_sv, SIGNAL(signalShowItem(Udb::Obj) ), this, SLOT(onFollowObject(Udb::Obj) ) );

	Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_sv, true );
	pop->addCommand( tr("Show Item"), d_sv, SLOT(onGoto()), tr("CTRL+G"), true );
	pop->addCommand( tr("Clear"), d_sv, SLOT(onClearSearch()) );
	pop->addSeparator();
    pop->addCommand( tr("Copy"), d_sv, SLOT(onCopyRef()), tr("CTRL+C"), true );
    pop->addSeparator();
	pop->addCommand( tr("Update Index..."), d_sv, SLOT(onUpdateIndex()) );
	pop->addCommand( tr("Rebuild Index..."), d_sv, SLOT(onRebuildIndex()) );
    addTopCommands( pop );
}

void MainWindow::setupFolders()
{
    QDockWidget* dock = createDock( this, tr("Documents"), 0, true );

    Udb::Obj root = d_txn->getOrCreateObject( QUuid(WorkTreeApp::s_folders), TypeRootFolder );
    root.setString( AttrText, WtTypeDefs::prettyName( TypeRootFolder ) );
	d_txn->commit();
    d_fldr = FolderCtrl::create( dock, root );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_fldr->getTree(), true );
    pop->addCommand( tr("Open Document"), this, SLOT(onOpenDocument()) );
    pop->addSeparator();
    d_fldr->addCommands( pop );
    addTopCommands( pop );
    connect( d_fldr, SIGNAL(signalSelected(Udb::Obj)), this, SLOT(onFldrSelected(Udb::Obj)) );
    connect( d_fldr, SIGNAL(signalDblClicked(Udb::Obj)), this, SLOT( onFldrDblClicked(Udb::Obj)));
    dock->setWidget( d_fldr->getTree() );
    addDockWidget( Qt::RightDockWidgetArea, dock );
}

void MainWindow::setupWbView()
{
    QDockWidget* dock = createDock( this, tr("Work Package"), 0, true );

    d_wpv = WpViewCtrl::create( dock, d_txn );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_wpv->getWidget(), true );
    d_wpv->addCommands( pop );
    addTopCommands( pop );
    connect( d_wpv, SIGNAL(signalSelect(Udb::Obj)), this, SLOT(onWpSelected(Udb::Obj)) );
    dock->setWidget( d_wpv->getWidget() );
	addDockWidget( Qt::RightDockWidgetArea, dock );
}

void MainWindow::setupTerminal()
{
	QDockWidget* dock = createDock( this, tr("Lua Terminal"), 0, false );
	Lua::Terminal2* term = new Lua::Terminal2( dock );
	dock->setWidget( term );
	addDockWidget( Qt::BottomDockWidgetArea, dock );
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    WorkTreeApp::inst()->getSet()->setValue("MainFrame/State/" +
		d_txn->getDb()->getDbUuid().toString(), saveState() );
	QMainWindow::closeEvent( event );
	if( event->isAccepted() )
        emit closing();
}

void MainWindow::onFullScreen()
{
    CHECKED_IF( true, d_fullScreen );
    if( d_fullScreen )
    {
#ifndef _WIN32
        setWindowFlags( Qt::Window );
#endif
        showNormal();
        d_fullScreen = false;
    }else
    {
        toFullScreen( this );
        d_fullScreen = true;
    }
    WorkTreeApp::inst()->getSet()->setValue( "MainFrame/State/FullScreen", d_fullScreen );
}

void MainWindow::onSetFont()
{
	ENABLED_IF( true );
	QFont f1 = QApplication::font();
	bool ok;
	QFont f2 = QFontDialog::getFont( &ok, f1, this, tr("Select Application Font - WorkTree" ) );
	if( !ok )
		return;
	f1.setFamily( f2.family() );
	f1.setPointSize( f2.pointSize() );
	QApplication::setFont( f1 );
	WorkTreeApp::inst()->setAppFont( f1 );
	// TODO d_dv->adjustFont();
	WorkTreeApp::inst()->getSet()->setValue( "App/Font", f1 );
}

void MainWindow::onAbout()
{
	ENABLED_IF( true );

	QMessageBox::about( this, tr("About WorkTree"),
		tr("<html><body><p>Release: <strong>%1</strong>   Date: <strong>%2</strong> </p>"
		   "<p>WorkTree can be used in the project planning and review process.</p>"
		   "<p>Author: Rochus Keller, me@rochus-keller.info</p>"
		   "<p>Copyright (c) 2012-%3</p>"
		   "<h4>Additional Credits:</h4>"
		   "<p>Qt GUI Toolkit 4.4 (c) 1995-2008 Trolltech AS, 2008 Nokia Corporation<br>"
		   "Sqlite 3.5, <a href='http://sqlite.org/copyright.html'>dedicated to the public domain by the authors</a><br>"
		   "<a href='http://www.sourceforge.net/projects/clucene'>CLucene</a> Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team<br>"
		   "<a href='http://code.google.com/p/fugue-icons-src/'>Fugue Icons</a>  2012 by Yusuke Kamiyamane<br>"
		   "<a href='https://www.graphviz.org'>Graphviz</a>  (libGv libCgraph) 1991-2016 by AT&T, John Ellson et al.<br>"
		   "Lua 5.1 by R. Ierusalimschy, L. H. de Figueiredo & W. Celes (c) 1994-2006 Tecgraf, PUC-Rio<p>"
		   "<h4>Terms of use:</h4>"
		   "<p>This version of WorkTree is freeware, i.e. it can be used for free by anyone. "
		   "The software and documentation are provided \"as is\", without warranty of any kind, "
		   "expressed or implied, including but not limited to the warranties of merchantability, "
		   "fitness for a particular purpose and noninfringement. In no event shall the author or "
		   "copyright holders be liable for any claim, damages or other liability, whether in an action "
		   "of contract, tort or otherwise, arising from, out of or in connection with the software or the "
		   "use or other dealings in the software.</p></body></html>" )
		.arg( WorkTreeApp::s_version ).arg( WorkTreeApp::s_date ).arg( QDate::currentDate().year() ) );
}

void MainWindow::onOpenPdmDiagram()
{
    Udb::Obj doc = d_imp->getSelectedObject(true);
    ENABLED_IF( WtTypeDefs::isPdmDiagram( doc.getType() ) );

    openPdmDiagram( doc );
}

void MainWindow::onPdmSelection()
{
    PdmCtrl* ctrl = dynamic_cast<PdmCtrl*>( sender() );
    Q_ASSERT( ctrl != 0 );
    Udb::Obj o = ctrl->getSingleSelection();
    if( !o.isNull( ) )
    {
        if( o.getType() == TypeLink )
            onImpSelected( o );
        else
            d_imp->showObject( o );
    }
}

void MainWindow::onTabChanged(int i)
{
    if( i < 0 ) // bei remove des letzen Tab kommt -1
	{
		d_ov->setObserved( 0 );
        d_lv->setObserved( 0 );
	}else
	{
		if( PdmItemView* v = dynamic_cast<PdmItemView*>( d_tab->widget( i ) ) )
		{
			d_ov->setObserved( v );
            d_lv->setObserved( v );
            Udb::Obj o = v->getCtrl()->getSingleSelection();
            if( o.getType() == TypeLink )
                onImpSelected( o );
            else if( !o.isNull() )
                d_imp->showObject( o );
		}else
		{
			d_ov->setObserved( 0 );
            d_lv->setObserved( 0 );
		}
    }
}

void MainWindow::onImpDblClicked(const Udb::Obj & doc)
{
    openPdmDiagram( doc );
}

void MainWindow::onShowSuperTask()
{
    PdmCtrl* ctrl = getCurrentPdmDiagram();
    if( ctrl == 0 )
        return;
    ENABLED_IF( ctrl && WtTypeDefs::isPdmDiagram( ctrl->getDiagram().getParent().getType() ) );
    d_imp->showObject( ctrl->getDiagram().getParent() );
    openPdmDiagram( ctrl->getDiagram().getParent(), ctrl->getDiagram() );
}

void MainWindow::onShowSubTask()
{
    PdmCtrl* ctrl = getCurrentPdmDiagram();
    if( ctrl == 0 )
        return;
    Udb::Obj sel = ctrl->getSingleSelection();
    ENABLED_IF( sel.getType() == TypeTask );
    d_imp->showObject( sel );
    openPdmDiagram( sel );
}

void MainWindow::onFollowAlias()
{
    PdmCtrl* ctrl = getCurrentPdmDiagram();
    if( ctrl == 0 )
        return;
    Udb::Obj sel = ctrl->getSingleSelection();
    ENABLED_IF( !sel.isNull() && !sel.getParent().equals( ctrl->getDiagram() ) );
    d_imp->showObject( sel );
    openPdmDiagram( sel.getParent(), sel );
}

void MainWindow::onImpSelected(const Udb::Obj & o)
{
    if( d_selectLock )
        return;
    d_selectLock = true;
    d_atv->setObj( o );
    d_tv->setObj( o );
    d_lv->setObj( o );
    d_asv->setObj( o );
    pushBack( o );
    showInPdmDiagram( o, false );
    d_selectLock = false;
}

void MainWindow::onLinkSelected(const Udb::Obj & o, bool open )
{
    if( o.getType() == TypeLink )
    {
        if( open )
            openPdmDiagram( o.getParent().getParent(), o );
        onImpSelected( o );
    }else
    {
        d_imp->showObject( o );
        if( open )
            openPdmDiagram( o.getParent(), o );
    }
}

void MainWindow::onAssigSelected(const Udb::Obj & o, bool open)
{
    d_atv->setObj( o );
    d_tv->setObj( o );
    // TODO: open
    pushBack( o );
}

void MainWindow::onObsSelected(const Udb::Obj & o)
{
    d_atv->setObj( o );
    d_tv->setObj( o );
    pushBack( o );
}

void MainWindow::onWbsSelected(const Udb::Obj & o)
{
    d_atv->setObj( o );
    d_tv->setObj( o );
    d_asv->setObj( o );
    d_wpv->setObj( o );
    pushBack( o );
}

void MainWindow::onSearch()
{
    ENABLED_IF( true );

	d_sv->parentWidget()->show();
	d_sv->parentWidget()->raise();
    d_sv->onNew();
}

void MainWindow::onFollowObject(const Udb::Obj & o)
{
    if( o.isNull( true, true ) )
        return;
    if( WtTypeDefs::isImpType( o.getType() ) )
    {
        d_imp->showObject( o );
        showInPdmDiagram( o, true );
    }else if( WtTypeDefs::isObsType( o.getType() ) )
        d_obs->showObject( o );
    else if( WtTypeDefs::isWbsType( o.getType() ) )
        d_wbs->showObject( o );
    else if( o.getType() == TypeLink )
    {
        onLinkSelected( o, false );
        d_lv->setObj( o.getParent() );
        d_lv->showLink( o );
    }else if( o.getType() == TypePdmDiagram )
    {
        openPdmDiagram( o );
        d_fldr->showObject( o );
    }else if( o.getType() == TypeOutline )
    {
        openOutline( o );
        d_fldr->showObject( o );
    }else if( o.getType() == TypeFolder )
        d_fldr->showObject( o );
	else if( o.getType() == Udb::ScriptSource::TID )
	{
		openScript( o );
		d_fldr->showObject( o );
	}else
	{
        pushBack( o );
        d_atv->setObj( o );
        d_tv->setObj( o );
        if( o.getType() == TypeRasciAssig ) // TODO: WorkAssig
        {
            d_asv->setObj( o.getParent() );
            d_asv->showAssig( o );
        }
    }
}

void MainWindow::onGoBack()
{
    // d_backHisto.last() ist Current
    ENABLED_IF( d_backHisto.size() > 1 );

    d_pushBackLock = true;
    d_forwardHisto.push_back( d_backHisto.last() );
    d_backHisto.pop_back();
    onFollowObject( d_txn->getObject( d_backHisto.last() ) );
    d_pushBackLock = false;
}

void MainWindow::onGoForward()
{
    ENABLED_IF( !d_forwardHisto.isEmpty() );

    Udb::Obj cur = d_txn->getObject( d_forwardHisto.last() );
    d_forwardHisto.pop_back();
    onFollowObject( cur );
}

void MainWindow::onImportMsp()
{
#ifdef _WIN32
    ENABLED_IF( true );

    QApplication::setOverrideCursor( Qt::WaitCursor );
    const bool res = d_msp->importMsp( this, d_txn );
    qDebug() << "#tasks:" << d_msp->getCounts().tasks <<
                "#milestones:" << d_msp->getCounts().milestones <<
                "#links:" << d_msp->getCounts().links <<
                "#deadLinks:" << d_msp->getCounts().deadLinks <<
                "#leads:" << d_msp->getCounts().leads <<
                "#lags:" << d_msp->getCounts().lags;
    qDebug() << d_msp->getErrors();
    QApplication::restoreOverrideCursor();
    if( !res && !d_msp->getErrors().isEmpty() )
        QMessageBox::information( this, tr("Import MS Project results - WorkTree" ),
                                  tr("%1 errors have occured. First error:\n%2").
                                  arg( d_msp->getErrors().size() ).
                                  arg( d_msp->getErrors().first() ) );
#endif
}

void MainWindow::onOpenDocument()
{
    Udb::Obj doc = d_fldr->getSelectedObject();
    ENABLED_IF( !doc.isNull() );

    switch( doc.getType() )
    {
    case TypePdmDiagram:
        openPdmDiagram( doc );
        break;
    case TypeOutline:
        openOutline( doc );
        break;
    }

}

void MainWindow::onFldrSelected(const Udb::Obj & o)
{
    if( d_selectLock )
        return;
    d_selectLock = true;
    d_atv->setObj( o );
    d_tv->setObj( o );
    pushBack( o );
    d_selectLock = false;
}

void MainWindow::onFldrDblClicked(const Udb::Obj & doc)
{
	const Udb::Atom type = doc.getType();
	if( type == TypePdmDiagram )
        openPdmDiagram( doc );
	else if( type == TypeOutline )
        openOutline( doc );
	else if( type  == Udb::ScriptSource::TID )
		openScript( doc );
}

void MainWindow::onUrlActivated( const QUrl& url)
{
    QDesktopServices::openUrl( url );
}

void MainWindow::onFollowOID(quint64 oid)
{
    Udb::Obj obj = d_txn->getObject( oid );
    if( !obj.isNull() )
        onFollowObject( obj );
}

void MainWindow::onWpSelected(const Udb::Obj & imp)
{
    if( WtTypeDefs::isImpType( imp.getType() ) )
        d_imp->showObject( imp );
    else if( WtTypeDefs::isWbsType( imp.getType() ) )
        d_wbs->showObject( imp );
}

void MainWindow::onCalendars()
{
    ENABLED_IF(true);

    CalendarListDlg dlg(d_txn,this);
    dlg.refill();
	dlg.exec();
}

void MainWindow::saveEditor()
{
	Lua::CodeEditor* e = dynamic_cast<Lua::CodeEditor*>( d_tab->currentWidget() );
	if( e == 0 || !e->document()->isModified() )
		return;
	Udb::ScriptSource script = d_tab->getCurrentObj();
	if( script.isNull() )
		return;
	script.setSource( e->getText() );
	script.commit();
	e->document()->setModified(false);
}

void MainWindow::handleExecute()
{
	Lua::CodeEditor* e = dynamic_cast<Lua::CodeEditor*>( d_tab->currentWidget() );
	ENABLED_IF( !Lua::Engine2::getInst()->isExecuting() && e != 0 );
	Lua::Engine2::getInst()->executeCmd( e->text().toLatin1(), ( e->getName().isEmpty() ) ?
											 QByteArray("#Editor") : e->getName().toLatin1() );
}

void MainWindow::onSetScriptFont()
{
	ENABLED_IF( true );

	bool ok;
	QFont f = Lua::CodeEditor::defaultFont();
	Lua::CodeEditor* e = dynamic_cast<Lua::CodeEditor*>( d_tab->currentWidget() );
	if( e )
		f = e->font();
	f = QFontDialog::getFont( &ok, f, this );
	if( !ok )
		return;
	QSettings set;
	set.setValue( "LuaEditor/Font", QVariant::fromValue(f) );
	set.sync();
	for( int i = 0; i < d_tab->count(); i++ )
		dynamic_cast<Lua::CodeEditor*>( d_tab->widget( i ) )->setFont(f);
}

void MainWindow::onAutoStart()
{
	Udb::Obj oln = d_tab->getCurrentObj();
	Udb::Obj root = WtTypeDefs::getRoot( d_txn );
	CHECKED_IF( !oln.isNull(), root.getValueAsObj(AttrAutoOpen).equals(oln) );

	if( root.getValueAsObj(AttrAutoOpen).equals(oln) )
		root.clearValue(AttrAutoOpen);
	else
		root.setValue(AttrAutoOpen,oln);
	root.commit();
}

