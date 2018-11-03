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

#include "PdmCtrl.h"
#include "PdmItemView.h"
#include "PdmItemMdl.h"
#include "WtTypeDefs.h"
#include "PdmItems.h"
#include "Funcs.h"
#include <Gui2/UiFunction.h>
#include <QtGui/QGraphicsItem>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>
#include <QtGui/QGroupBox>
#include "PdmItems.h"
#include <QtGui/QTextEdit>
#include <QtGui/QKeyEvent>
#include "PdmStream.h"
#include "ObjectHelper.h"
#include "PdmItemObj.h"
#include <QtCore/QSet>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <Udb/Database.h>
#include <Udb/Idx.h>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtDebug>
#include "PdmLayouter.h"
#include "WorkTreeApp.h"
#include "TaskAttrDlg.h"
using namespace Wt;

const char* PdmCtrl::s_mimePdmItems = "application/worktree/pdm-items";

class _FlowChartViewTextEdit : public QTextEdit
{
public:
	bool d_ok;
	PdmNode* d_item;

	_FlowChartViewTextEdit(QWidget* p, PdmNode* item ):QTextEdit(p),d_item(item),d_ok(true)
	{
		setWindowFlags( Qt::Popup );
		setText( item->getText() );
		selectAll();
		setAcceptRichText( false );
		//setLineWrapMode( QTextEdit::NoWrap );
	}
	void hideEvent ( QHideEvent * e )
	{
		if( d_ok )
		{
			PdmItemMdl* mdl = dynamic_cast<PdmItemMdl*>( d_item->scene() );
			if( mdl )
				mdl->setItemText( d_item, toPlainText() );
		}
		QTextEdit::hideEvent( e );
		deleteLater();
	}
	void keyPressEvent ( QKeyEvent * e )
	{
		if( e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return )
			close();
		else if( e->key() == Qt::Key_Escape )
		{
			d_ok = false;
			close();
		}else
			QTextEdit::keyPressEvent( e );
	}
};

PdmCtrl::PdmCtrl( PdmItemView* view, PdmItemMdl * mdl ):
    QObject( view ),d_mdl(mdl)
{
    Q_ASSERT( view != 0 );
    Q_ASSERT( mdl != 0 );
}

PdmItemView* PdmCtrl::getView() const
{
    return static_cast<PdmItemView*>( parent() );
}

PdmCtrl *PdmCtrl::create(QWidget *parent, const Udb::Obj &doc)
{
    PdmItemView* view = new PdmItemView( parent );
	PdmItemMdl* mdl = new PdmItemMdl( view );
    //mdl->setChartFont( d_chartFont );
    mdl->setDiagram( doc );
    view->setScene( mdl );
    PdmCtrl* ctrl = new PdmCtrl( view, mdl );
    connect( view, SIGNAL(signalDblClick(QPoint)), ctrl, SLOT(onDblClick(QPoint)) );
    connect( mdl, SIGNAL(signalCreateLink(Udb::Obj,Udb::Obj,QPolygonF)),
             ctrl, SLOT(onCreateLink(Udb::Obj,Udb::Obj,QPolygonF)));
    connect( mdl, SIGNAL( selectionChanged() ), ctrl, SLOT( onSelectionChanged() ) );
    connect( view, SIGNAL(signalClick(QPoint)), ctrl, SLOT(onSelectionChanged()) );
    connect( mdl, SIGNAL(signalDrop(QByteArray,QPointF)),
             ctrl,SLOT(onDrop(QByteArray,QPointF)), Qt::QueuedConnection );
    return ctrl;
}

void PdmCtrl::addCommands(Gui2::AutoMenu *pop)
{
    pop->addCommand( tr( "New Task" ), this, SLOT( onAddTask() ), tr("CTRL+T"), true );
    pop->addCommand( tr( "New Milestone" ), this, SLOT( onAddMilestone() ), tr("CTRL+M"), true );
    pop->addCommand( tr("Toggle Task/Milestone"), this, SLOT(onToggleTaskMs()) );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Set Task Type" ), pop );
    pop->addMenu( sub );
    QAction* a;
    a = sub->addCommand( WtTypeDefs::formatTaskType( TaskType_Discrete ), this, SLOT(onSetTaskType()) );
    a->setCheckable(true);
    a->setData( TaskType_Discrete );
    a = sub->addCommand( WtTypeDefs::formatTaskType( TaskType_LOE ), this, SLOT(onSetTaskType()) );
    a->setCheckable(true);
    a->setData( TaskType_LOE );
    a = sub->addCommand( WtTypeDefs::formatTaskType( TaskType_SVT ), this, SLOT(onSetTaskType()) );
    a->setCheckable(true);
    a->setData( TaskType_SVT );

    sub = new Gui2::AutoMenu( tr("Set Milestone Type" ), pop );
    pop->addMenu( sub );
    a = sub->addCommand( WtTypeDefs::formatMsType( MsType_Intermediate ), this, SLOT(onSetMsType()) );
    a->setCheckable(true);
    a->setData( MsType_Intermediate );
    a = sub->addCommand( WtTypeDefs::formatMsType( MsType_TaskStart ), this, SLOT(onSetMsType()) );
    a->setCheckable(true);
    a->setData( MsType_TaskStart );
    a = sub->addCommand( WtTypeDefs::formatMsType( MsType_TaskFinish ), this, SLOT(onSetMsType()) );
    a->setCheckable(true);
    a->setData( MsType_TaskFinish );
    sub->addSeparator();
    a = sub->addCommand( WtTypeDefs::formatMsType( MsType_ProjStart ), this, SLOT(onSetMsType()) );
    a->setCheckable(true);
    a->setData( MsType_ProjStart );
    a = sub->addCommand( WtTypeDefs::formatMsType( MsType_ProjFinish ), this, SLOT(onSetMsType()) );
    a->setCheckable(true);
    a->setData( MsType_ProjFinish );
    pop->addCommand( tr("Edit Attributes..."), this, SLOT(onEditAttrs()), tr("CTRL+Return"), true );

    pop->addSeparator();
    pop->addCommand( tr( "Start Link" ), this, SLOT( onLink() ) );
    pop->addCommand( tr( "New Handle" ), this, SLOT( onInsertHandle() ), tr("Insert"), true );
    sub = new Gui2::AutoMenu( tr("Set Link Type" ), pop );
    pop->addMenu( sub );
    a = sub->addCommand( WtTypeDefs::formatLinkType( LinkType_FS ), this, SLOT(onSetLinkType()) );
    a->setCheckable(true);
    a->setData( LinkType_FS );
    a = sub->addCommand( WtTypeDefs::formatLinkType( LinkType_SS ), this, SLOT(onSetLinkType()) );
    a->setCheckable(true);
    a->setData( LinkType_SS );
    a = sub->addCommand( WtTypeDefs::formatLinkType( LinkType_FF ), this, SLOT(onSetLinkType()) );
    a->setCheckable(true);
    a->setData( LinkType_FF );
    a = sub->addCommand( WtTypeDefs::formatLinkType( LinkType_SF ), this, SLOT(onSetLinkType()) );
    a->setCheckable(true);
    a->setData( LinkType_SF );

    pop->addSeparator();
    sub = new Gui2::AutoMenu( tr("Add Existing" ), pop );
    pop->addMenu( sub );
    sub->addCommand( tr( "Hidden Tasks/Milestones..."), this, SLOT(onSelectHiddenSchedObjs()) );
    sub->addCommand( tr( "Linked Tasks/Milestones..."), this, SLOT(onExtendDiagram()), tr("CTRL+E"), true );
    sub->addCommand( tr( "Connecting Tasks/Milestones..."), this, SLOT(onShowShortestPath()), tr("CTRL+SHIFT+E"), true );
    sub->addCommand( tr( "Hidden Links..."), this, SLOT(onSelectHiddenLinks()) );

    sub = new Gui2::AutoMenu( tr("Select" ), pop );
    pop->addMenu( sub );
    sub->addCommand( tr( "All" ), this, SLOT( onSelectAll() ), tr("CTRL+A"), true );
    sub->addCommand( tr( "Rightwards" ), this, SLOT( onSelectRightward() ), tr("CTRL+SHIFT+Right"), true );
    sub->addCommand( tr( "Leftwards" ), this, SLOT( onSelectLeftward() ), tr("CTRL+SHIFT+Left"), true );
    sub->addCommand( tr( "Upwards" ), this, SLOT( onSelectUpward() ), tr("CTRL+SHIFT+Up"), true );
    sub->addCommand( tr( "Downwards" ), this, SLOT( onSelectDownward() ), tr("CTRL+SHIFT+Down"), true );

    pop->addCommand( tr( "Layout..." ), this, SLOT( onLayout() ), tr("CTRL+SHIFT+L"), true );

    pop->addSeparator();
    pop->addCommand( tr( "Cut..." ), this, SLOT( onCut() ), tr( "CTRL+X" ), true );
    pop->addCommand( tr( "Copy" ), this, SLOT( onCopy() ), tr( "CTRL+C" ), true );
    pop->addCommand( tr( "Paste" ), this, SLOT( onPaste() ), tr( "CTRL+V" ), true );

    sub = new Gui2::AutoMenu( tr("Export" ), pop );
    pop->addMenu( sub );
    sub->addCommand( tr( "To File..." ), this, SLOT( onExportToFile() ) );
    sub->addCommand( tr( "To Clipboard" ), this, SLOT( onExportToClipboard() ) );

    pop->addSeparator();

    pop->addCommand( tr( "Remove from diagram..." ), this, SLOT( onRemoveItems() ), tr("CTRL+D"), true );
    pop->addCommand( tr( "Remove all Aliasses..." ), this, SLOT( onRemoveAllAliasses() ) );
    pop->addCommand( tr( "Delete permanently..." ), this, SLOT( onDeleteItems() ), tr("CTRL+SHIFT+D"), true );

    pop->addSeparator();

    pop->addCommand( tr("Show Ident."), this, SLOT(onShowId()) )->setCheckable(true);
    pop->addCommand( tr("Mark Alias"), this, SLOT(onMarkAlias()) )->setCheckable(true);
}

Udb::Obj PdmCtrl::getSingleSelection() const
{
    PdmItemObj o = d_mdl->getSingleSelection();
    if( !o.isNull() )
        return o.getOrig();
    else
        return Udb::Obj();
}

void PdmCtrl::onAddItem( quint32 type )
{
    ENABLED_IF( !d_mdl->isReadOnly() && d_mdl->getDiagram().getType() != TypePdmDiagram &&
				   Funcs::isValidAggregate( d_mdl->getDiagram().getType(), type ) );
    // Wir erlauben nur Erzeugen von Tasks in IMP-Elementen; Diagramme sind nur zur Zusammenfassung

    Udb::Obj diag = d_mdl->getDiagram();
    Udb::Obj obj = ObjectHelper::createObject( type, diag );
    PdmItemObj::createItemObj( diag, obj, d_mdl->getStart( true ) );
    obj.commit();
}

void PdmCtrl::pasteItemRefs(const QMimeData *data, const QPointF& where )
{
    if( !data->hasFormat( Udb::Obj::s_mimeObjectRefs ) )
        return;
    Udb::Obj diagram = d_mdl->getDiagram();
    QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( data, diagram.getTxn() );
    QList<Udb::Obj> done = PdmItemObj::addItemsToDiagram( diagram, objs, where );
    diagram.commit();
    PdmItemObj::addItemLinksToDiagram( diagram, done );
    diagram.commit();
}

void PdmCtrl::onAddTask()
{
    onAddItem( TypeTask );
}

void PdmCtrl::onAddMilestone()
{
    onAddItem( TypeMilestone );
}

void PdmCtrl::onSelectAll()
{
	ENABLED_IF( true );
    QPainterPath selectionArea;
	selectionArea.addRect( d_mdl->sceneRect() );
	d_mdl->setSelectionArea( selectionArea );
}

void PdmCtrl::onSelectRightward()
{
	ENABLED_IF( true );
	QRectF r = d_mdl->sceneRect();
	QPointF p = d_mdl->getStart();
	r.setLeft( p.x() );
    QPainterPath pp;
	pp.addRect( r );
	d_mdl->setSelectionArea( pp, Qt::ContainsItemShape );
}

void PdmCtrl::onSelectUpward()
{
	ENABLED_IF( true );
	QRectF r = d_mdl->sceneRect();
	QPointF p = d_mdl->getStart();
	r.setBottom( p.y() );
    QPainterPath pp;
	pp.addRect( r );
	d_mdl->setSelectionArea( pp, Qt::ContainsItemShape );
}

void PdmCtrl::onSelectLeftward()
{
	ENABLED_IF( true );
	QRectF r = d_mdl->sceneRect();
	QPointF p = d_mdl->getStart();
	r.setRight( p.x() );
    QPainterPath pp;
	pp.addRect( r );
	d_mdl->setSelectionArea( pp, Qt::ContainsItemShape );
}

void PdmCtrl::onSelectDownward()
{
	ENABLED_IF( true );
	QRectF r = d_mdl->sceneRect();
	QPointF p = d_mdl->getStart();
	r.setTop( p.y() );
    QPainterPath pp;
	pp.addRect( r );
	d_mdl->setSelectionArea( pp, Qt::ContainsItemShape );
}

void PdmCtrl::onRemoveItems()
{
    ENABLED_IF( !d_mdl->isReadOnly() &&
		!d_mdl->selectedItems().isEmpty() );
	const int res = QMessageBox::warning( getView(), tr("Remove Items from Diagram"),
		tr("Do you really want to remove the %1 selected items? This cannot be undone." ).
		arg( d_mdl->selectedItems().size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( res == QMessageBox::No )
		return;
    d_mdl->removeSelectedItems();
}

void PdmCtrl::onInsertHandle()
{
	ENABLED_IF( !d_mdl->isReadOnly() &&
		d_mdl->selectedItems().size() == 1 &&
		d_mdl->selectedItems().first()->type() == PdmNode::Link );

	d_mdl->insertHandle();
}

void PdmCtrl::onExportToClipboard()
{
	ENABLED_IF( true );
	d_mdl->exportPng( QString() );
}

void PdmCtrl::onExportToFile()
{
	ENABLED_IF( true );
	QString filter;
	QString path = QFileDialog::getSaveFileName( getView(), tr( "Export Diagram - WorkTree" ),
		d_mdl->getDiagram().getString( AttrText ),
		tr("*.png;;*.pdf;;*.html;;*.flnx;;*.svg"), &filter, QFileDialog::DontUseNativeDialog );
	if( path.isEmpty() )
		return;

	if( filter == "*.png" )
	{
		if( !path.endsWith( ".png" ) )
			path += ".png";
		d_mdl->exportPng( path );
	}else if( filter == "*.pdf" )
	{
		if( !path.endsWith( ".pdf" ) )
			path += ".pdf";
		d_mdl->exportPdf( path );
	}else if( filter == "*.html" )
    {
		if( !path.endsWith( ".html" ) )
			path += ".html";
		d_mdl->exportHtml( path );
	}else if( filter == "*.svg" )
    {
		if( !path.endsWith( ".svg" ) )
			path += ".svg";
		d_mdl->exportSvg( path );
	}else if( filter == "*.flnx" )
	{
		if( !path.endsWith( ".flnx" ) )
			path += ".flnx";
		QFile f( path );
		if( !f.open( QIODevice::WriteOnly ) )
		{
			QMessageBox::critical( getView(), tr("Export Process - WorkTree"),
				tr("Cannot open file for writing!") );
			return;
		}
		Stream::DataWriter out( &f );
		// TODO PdmStream::exportNetwork( d_mdl->getAttr(), out, d_mdl->getDoc() );
		d_mdl->getDiagram().getTxn()->commit();  // Wegen UUID
	}
}

void PdmCtrl::onLink()
{
	ENABLED_IF( d_mdl->canStartLink() );
	d_mdl->startLink();
}

void PdmCtrl::onCopy()
{
	ENABLED_IF( !d_mdl->selectedItems().isEmpty() );

    QList<Udb::Obj> sel = d_mdl->getMultiSelection();

    QMimeData* mime = new QMimeData();
    writeItems( mime, sel );
    QApplication::clipboard()->setMimeData( mime );
}

void PdmCtrl::onCut()
{
    ENABLED_IF( !d_mdl->isReadOnly() && !d_mdl->selectedItems().isEmpty() );

    const int res = QMessageBox::warning( getView(), tr("Cut Items from Diagram"),
		tr("Do you really want to remove the %1 selected items? This cannot be undone." ).
		arg( d_mdl->selectedItems().size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( res == QMessageBox::No )
		return;
    QList<Udb::Obj> sel = d_mdl->getMultiSelection();

    QMimeData* mime = new QMimeData();
    writeItems( mime, sel );
    QApplication::clipboard()->setMimeData( mime );
    d_mdl->removeSelectedItems();
}

void PdmCtrl::onPaste()
{
    ENABLED_IF( QApplication::clipboard()->mimeData()->hasFormat( Udb::Obj::s_mimeObjectRefs ) ||
                QApplication::clipboard()->mimeData()->hasFormat( s_mimePdmItems ) );

    if( QApplication::clipboard()->mimeData()->hasFormat( s_mimePdmItems ) )
    {
        QList<Udb::Obj> objs = readItems( QApplication::clipboard()->mimeData() );
        adjustTo( objs, d_mdl->getStart( true ) );
        d_mdl->getDiagram().getTxn()->commit();
    }else if( QApplication::clipboard()->mimeData()->hasFormat( Udb::Obj::s_mimeObjectRefs ) )
    {
        pasteItemRefs( QApplication::clipboard()->mimeData(), d_mdl->getStart( true ) );
    }
}

void PdmCtrl::onShowId()
{
	CHECKED_IF( true, d_mdl->isShowId() );
	d_mdl->setShowId( !d_mdl->isShowId() );
    getView()->viewport()->update();
}

void PdmCtrl::onSetLinkType()
{
    Udb::Obj o = getSingleSelection();
    CHECKED_IF( !d_mdl->isReadOnly() && o.getType() == TypeLink,
                Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrLinkType ).getUInt8() );
    if( Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrLinkType ).getUInt8() )
        return;
    o.setValue( AttrLinkType, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
    o.commit();
}

void PdmCtrl::onSetTaskType()
{
    Udb::Obj o = getSingleSelection();
    CHECKED_IF( !d_mdl->isReadOnly() && o.getType() == TypeTask,
                Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrTaskType ).getUInt8() );
    if( Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrTaskType ).getUInt8() )
        return;
    o.setValue( AttrTaskType, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
    o.commit();
}

void PdmCtrl::onMarkAlias()
{
    CHECKED_IF( true, d_mdl->isMarkAlias() );
	d_mdl->setMarkAlias( !d_mdl->isMarkAlias() );
    getView()->viewport()->update();
}

void PdmCtrl::onDeleteItems()
{
    ENABLED_IF( !d_mdl->isReadOnly() &&
		!d_mdl->selectedItems().isEmpty() );
	const int res = QMessageBox::warning( getView(), tr("Delete Items from Database"),
		tr("Do you really want to permanently delete the %1 selected items? This cannot be undone." ).
		arg( d_mdl->selectedItems().size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( res == QMessageBox::No )
		return;
    QList<Udb::Obj> sel = d_mdl->getMultiSelection();
    foreach( Udb::Obj o, sel )
    {
        Udb::Obj hidden = o.getValueAsObj( AttrOrigObject );
        ObjectHelper::erase( hidden );
    }
    d_mdl->getDiagram().getTxn()->commit();
}

void PdmCtrl::onLayout()
{
    ENABLED_IF( !d_mdl->isReadOnly() && !d_mdl->getDiagram().isNull() );
    Udb::Obj diagram = d_mdl->getDiagram();

    if( !PdmItemObj::s_layouter.prepareEngine( getView() ) )
        return;

    QMessageBox msg( QMessageBox::Warning, tr("Layout Diagram - WorkTree"),
                     tr("Do you really want to relayout the diagram? This cannot be undone." ),
                     QMessageBox::NoButton);
    msg.setDefaultButton( msg.addButton( tr("&Linear"), QMessageBox::YesRole ) );
    QPushButton* ortho = msg.addButton( tr("&Orthogonal"), QMessageBox::AcceptRole );
    msg.addButton( QMessageBox::Cancel );
    const int res = msg.exec();
	if( res == QMessageBox::Cancel )
		return;

    QApplication::setOverrideCursor( Qt::WaitCursor );
    QList<Udb::Obj> sel = d_mdl->getMultiSelection();
    doLayout( msg.clickedButton() == ortho );
    d_mdl->selectObjects( sel );
    QApplication::restoreOverrideCursor();
    //d_mdl->fitSceneRect(true); // führt zu Crash wenn man anschliessend ein Objekt bewegt
}

void PdmCtrl::doLayout( bool ortho )
{
    // Es ist besser, wenn während dem Layout das Diagramm nicht dargestellt wird.
    // Aus irgendwelchen Gründen werden die neuen Positionen ansonsten nicht angezeigt.
    Udb::Obj diagram = d_mdl->getDiagram();
    d_mdl->setDiagram( Udb::Obj() );
    if( !PdmItemObj::s_layouter.layoutDiagram( diagram, ortho, false ) )
    {
        diagram.getTxn()->rollback();
        d_mdl->setDiagram( diagram );
        QMessageBox::critical( getView(), tr("Layout Diagram Error - WorkTree" ),
                              PdmItemObj::s_layouter.getErrors().join( "; " ) );
        return;
    } // else
    diagram.commit();
    d_mdl->setDiagram( diagram );
}

void PdmCtrl::onSetMsType()
{
    Udb::Obj o = getSingleSelection();
    CHECKED_IF( !d_mdl->isReadOnly() && o.getType() == TypeMilestone,
                Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrMsType ).getUInt8() );

    if( Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrMsType ).getUInt8() )
        return;
    ObjectHelper::setMsType( o, Gui2::UiFunction::me()->data().toInt() );
    o.commit();
}

void PdmCtrl::onToggleTaskMs()
{
    Udb::Obj doc = getSingleSelection();
    ENABLED_IF( WtTypeDefs::canConvert( doc, TypeTask ) ||
                WtTypeDefs::canConvert( doc, TypeMilestone ) );
    switch( doc.getType() )
    {
    case TypeTask:
        ObjectHelper::retypeObject( doc, TypeMilestone );
        doc.commit();
        break;
    case TypeMilestone:
        ObjectHelper::retypeObject( doc, TypeTask );
        doc.commit();
        break;
    }
}

void PdmCtrl::onEditAttrs()
{
    Udb::Obj doc = getSingleSelection();
    ENABLED_IF( WtTypeDefs::isSchedObj( doc.getType() ) );

    TaskAttrDlg dlg( getView() );
    dlg.edit( doc );
}

void PdmCtrl::onExtendDiagram()
{
    ENABLED_IF(true);

    QDialog dlg( getView() );
    dlg.setWindowTitle( tr("Add Linked Tasks/Milestones - WorkTree") );
    QVBoxLayout vbox( &dlg );
    vbox.addWidget( new QLabel(tr("Please set the following parameters to extend the diagram:"), &dlg ) );
    QHBoxLayout hbox;
    vbox.addLayout( &hbox );
    QSpinBox spin( &dlg );
    spin.setRange(1,255);
    spin.setValue(1);
    hbox.addWidget( &spin );
    hbox.addWidget( new QLabel( tr("How many levels to extend"), &dlg ) );
    QCheckBox succ( tr("Extend successors"), &dlg );
    succ.setChecked( true );
    vbox.addWidget( &succ );
    QCheckBox pred( tr("Extend predecessors"), &dlg );
    pred.setChecked( true );
    vbox.addWidget( &pred );
    QCheckBox critical( tr("Critical only"), &dlg );
    vbox.addWidget( &critical );
    QCheckBox layout( tr("Layout diagram"), &dlg );
    layout.setChecked( true );
    vbox.addWidget( &layout );
    QDialogButtonBox bb(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, &dlg );
	vbox.addWidget( &bb );
    connect( &bb, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect( &bb, SIGNAL(rejected()), &dlg, SLOT(reject()));
    if( dlg.exec() == QDialog::Rejected )
        return;

    if( layout.isChecked() && !PdmItemObj::s_layouter.prepareEngine( getView() ) )
        return;

    QList<Udb::Obj> sel = d_mdl->getMultiSelection(); // PdmItemObj

    QList<Udb::Obj> objs;
    foreach( Udb::Obj item, sel )
    {
        Udb::Obj o = item.getValueAsObj( AttrOrigObject );
        if( WtTypeDefs::isSchedObj( o.getType() ) )
            objs.append( o );
    }
    Udb::Obj diagram = d_mdl->getDiagram();
    if( objs.isEmpty() )
        objs = PdmItemObj::findAllItemOrigObjs( diagram, true, false );
    QApplication::setOverrideCursor( Qt::WaitCursor );
    objs = PdmItemObj::findExtendedSchedObjs( objs, spin.value(), succ.isChecked(),
                                              pred.isChecked(), critical.isChecked() );
    objs = PdmItemObj::addItemsToDiagram( diagram, objs, QPointF(0,0) );
    diagram.commit();
    // hier gleich auch die Links einfügen zu dem Zeug, das schon im Diagramm ist.
    PdmItemObj::addItemLinksToDiagram( diagram, objs );
    diagram.commit();
    if( layout.isChecked() )
        doLayout( false );
    d_mdl->selectObjects( objs );
    QApplication::restoreOverrideCursor();
}

void PdmCtrl::onShowShortestPath()
{
    QList<Udb::Obj> sel = d_mdl->getMultiSelection( true, true, false, false );
    ENABLED_IF( sel.size() == 2 );

    Udb::Obj start = sel.first().getValueAsObj( AttrOrigObject );
    Udb::Obj goal = sel.last().getValueAsObj( AttrOrigObject );

    QDialog dlg( getView() );
    dlg.setWindowTitle( tr("Add Connecting Tasks/Milestones - WorkTree") );
    QVBoxLayout vbox( &dlg );
    vbox.addWidget( new QLabel(tr("Please set the following parameters to extend the diagram:"), &dlg ) );

    QGroupBox groupBox(tr("Shortest Path based on"), &dlg );
    QRadioButton r1(tr("Task/Milestone &Count"), &groupBox);
    QRadioButton r2(tr("&Normal Duration"), &groupBox);
    QRadioButton r3(tr("&Optimistic Duration"), &groupBox );
    QRadioButton r4(tr("&Pessimistic Duration"), &groupBox );
    QRadioButton r5(tr("&Most Likely Duration"), &groupBox );
    QRadioButton r6(tr("C&ritical Path"), &groupBox );
    r1.setChecked(true);
    r6.setEnabled( start.getValue( AttrCriticalPath ).getBool() &&
                   goal.getValue( AttrCriticalPath ).getBool() );
    QVBoxLayout vbox2( &groupBox );
    vbox2.addWidget(&r1);
    vbox2.addWidget(&r2);
    vbox2.addWidget(&r3);
    vbox2.addWidget(&r4);
    vbox2.addWidget(&r5);
    vbox2.addWidget(&r6);
    vbox.addWidget( &groupBox );
    QCheckBox layout( tr("Layout diagram"), &dlg );
    layout.setChecked( true );
    vbox.addWidget( &layout );
    QDialogButtonBox bb(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, &dlg );
	vbox.addWidget( &bb );
    connect( &bb, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect( &bb, SIGNAL(rejected()), &dlg, SLOT(reject()));
    if( dlg.exec() == QDialog::Rejected )
        return;

    if( layout.isChecked() && !PdmItemObj::s_layouter.prepareEngine( getView() ) )
        return;

	ObjectHelper::ShortestPathMethod method;
    if( r1.isChecked() )
		method = ObjectHelper::SpmNodeCount;
    else if( r2.isChecked() )
		method = ObjectHelper::SpmDuration;
    else if( r3.isChecked() )
		method = ObjectHelper::SpmOptimisticDur;
    else if( r4.isChecked() )
		method = ObjectHelper::SpmPessimisticDur;
    else if( r5.isChecked() )
		method = ObjectHelper::SpmMostLikelyDur;
    else if( r6.isChecked() )
		method = ObjectHelper::SpmCriticalPath;

    QApplication::setOverrideCursor( Qt::WaitCursor );
	QList<Udb::Obj> objs = ObjectHelper::findShortestPath( start, goal, method );
    if( objs.isEmpty() )
    {
        QApplication::restoreOverrideCursor();
        return;
    }
    Udb::Obj diagram = d_mdl->getDiagram();
    objs = PdmItemObj::addItemsToDiagram( diagram, objs, QPointF(0,0) );
    diagram.commit();
    // hier gleich auch die Links einfügen zu dem Zeug, das schon im Diagramm ist.
    PdmItemObj::addItemLinksToDiagram( diagram, objs );
    diagram.commit();
    if( layout.isChecked() )
        doLayout( false );
    d_mdl->selectObjects( objs );
    QApplication::restoreOverrideCursor();

}

void PdmCtrl::onRemoveAllAliasses()
{
    ENABLED_IF( true );

    QList<PdmItemObj> aliasses = PdmItemObj::findAllAliasses( d_mdl->getDiagram() );
    const int res = QMessageBox::warning( getView(), tr("Remove Aliasses from Diagram"),
		tr("Do you really want to remove the %1 aliasses? This cannot be undone." ).
		arg( aliasses.size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( res == QMessageBox::No )
		return;
    foreach( PdmItemObj o, aliasses )
        o.erase();
    d_mdl->getDiagram().getTxn()->commit();
}

void PdmCtrl::onSelectHiddenSchedObjs()
{
    ENABLED_IF( !d_mdl->getDiagram().isNull() );

    Udb::Obj diagram = d_mdl->getDiagram();
    QSet<Udb::Obj> hiddens = PdmItemObj::findHiddenSchedObjs( diagram );

    QDialog dlg( getView() );
    dlg.setWindowTitle( tr("Add Hidden Tasks/Milestones - WorkTree") );
    QVBoxLayout vbox( &dlg );
    vbox.addWidget( new QLabel( tr("Select one or more Tasks/Milestones:"), &dlg ) );
	QListWidget list( &dlg );
	list.setAlternatingRowColors( true );
    list.setSelectionMode( QAbstractItemView::ExtendedSelection );
	vbox.addWidget( &list );
	QDialogButtonBox bb(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, &dlg );
	vbox.addWidget( &bb );
    connect(&bb, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(&bb, SIGNAL(rejected()), &dlg, SLOT(reject()));
	connect(&list, SIGNAL( itemDoubleClicked ( QListWidgetItem * ) ), &dlg, SLOT( accept() ) );

    foreach( Udb::Obj o, hiddens )
    {
        QListWidgetItem* lwi = new QListWidgetItem( &list );
		lwi->setText( WtTypeDefs::formatObjectTitle( o ) );
		lwi->setData( Qt::UserRole, o.getOid() );
		lwi->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }
    list.sortItems();

    QPointF p = d_mdl->getStart( true );
    if( dlg.exec() == QDialog::Accepted )
    {
        QList<Udb::Obj> done;
        foreach( QListWidgetItem* lwi, list.selectedItems() )
		{
            Udb::Obj hidden = diagram.getObject( lwi->data( Qt::UserRole ).toULongLong() );
            Q_ASSERT( !hidden.isNull() );
            done.append( hidden );
            PdmItemObj::createItemObj( diagram, hidden, p );
            p += QPointF( PdmNode::s_rasterX, PdmNode::s_rasterY );
		}
        diagram.commit();
        // hier gleich auch die Links einfügen zu dem Zeug, das schon im Diagramm ist.
        PdmItemObj::addItemLinksToDiagram( diagram, done );
        diagram.commit();
    }
}

void PdmCtrl::onSelectHiddenLinks()
{
    ENABLED_IF( !d_mdl->getDiagram().isNull() );

    Udb::Obj diagram = d_mdl->getDiagram();
    QList<Udb::Obj> startset;
    foreach( Udb::Obj o, d_mdl->getMultiSelection() )
    {
        if( o.getType() == TypePdmItem )
        {
            Udb::Obj orig = o.getValueAsObj( AttrOrigObject );
            if( WtTypeDefs::isSchedObj( orig.getType() ) )
                startset.append( orig );
        }
    }
    QList<Udb::Obj> hiddenLinks = PdmItemObj::findHiddenLinks( diagram, startset );

    QDialog dlg( getView() );
    dlg.setWindowTitle( tr("Select hidden links - WorkTree") );
    QVBoxLayout vbox( &dlg );
    vbox.addWidget( new QLabel( tr("Select one or more Links:"), &dlg ) );
	QListWidget list( &dlg );
	list.setAlternatingRowColors( true );
    list.setSelectionMode( QAbstractItemView::ExtendedSelection );
	vbox.addWidget( &list );
	QDialogButtonBox bb(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, &dlg );
	vbox.addWidget( &bb );
    connect(&bb, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(&bb, SIGNAL(rejected()), &dlg, SLOT(reject()));
	connect(&list, SIGNAL( itemDoubleClicked ( QListWidgetItem * ) ), &dlg, SLOT( accept() ) );

    foreach( Udb::Obj link, hiddenLinks )
    {
        QListWidgetItem* lwi = new QListWidgetItem( &list );
		lwi->setText( QString( "%1 %2 %3" ).
                      arg( WtTypeDefs::formatObjectId( link.getValueAsObj( AttrPred ) ) ).
                      arg( QChar( 0x2192 ) ).
                      arg( WtTypeDefs::formatObjectId( link.getValueAsObj( AttrSucc ) ) ) );
        lwi->setToolTip( QString( "%1\n%2\n%3" ).
                         arg( WtTypeDefs::formatObjectTitle( link.getValueAsObj( AttrPred ) ) ).
                         arg( QChar( 0x2192 ) ).
                         arg( WtTypeDefs::formatObjectTitle( link.getValueAsObj( AttrSucc ) ) ) );
		lwi->setData( Qt::UserRole, link.getOid() );
		lwi->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }
    list.sortItems();
    if( dlg.exec() == QDialog::Accepted )
    {
        foreach( QListWidgetItem* lwi, list.selectedItems() )
		{
            Udb::Obj link = diagram.getObject( lwi->data( Qt::UserRole ).toULongLong() );
            Q_ASSERT( !link.isNull() );
            PdmItemObj::createItemObj( diagram, link );
		}
        diagram.commit();
    }
}

void PdmCtrl::onDblClick(const QPoint & pos)
{
	if( !d_mdl->isReadOnly() )
	{
		PdmNode* i = dynamic_cast<PdmNode*>( d_mdl->itemAt( getView()->mapToScene( pos ) ) );
		if( i && i->hasItemText() ) // Auch Aliasse änderbar
		{
			_FlowChartViewTextEdit* edit = new _FlowChartViewTextEdit( 0, i );
			edit->resize( PdmNode::s_boxWidth * 1.5, PdmNode::s_boxHeight * 1.5 );
			edit->move( getView()->viewport()->mapToGlobal( pos ) -
				QPoint( PdmNode::s_boxWidth * 0.5, PdmNode::s_boxHeight * 0.5 ) );
			edit->show();
			edit->setFocus();
		}
    }
}

void PdmCtrl::onCreateLink(const Udb::Obj &pred, const Udb::Obj &succ, const QPolygonF &path)
{
    PdmItemObj pdmItem = PdmItemObj::createLink( d_mdl->getDiagram(), pred, succ );
    pdmItem.setNodeList( path );
    pdmItem.commit();
}

void PdmCtrl::onSelectionChanged()
{
    emit signalSelectionChanged();
}

void PdmCtrl::onDrop(QByteArray data, QPointF where)
{
    // Dieser Umweg ist nötig, damit onDrop asynchron aufgerufen werden kann.
    QMimeData mime;
    mime.setData( Udb::Obj::s_mimeObjectRefs, data );
    pasteItemRefs( &mime, where );
}

void PdmCtrl::adjustTo( const QList<Udb::Obj>& pdmItems, const QPointF& to )
{
	QPainterPath bound;
	int i;
	for( i = 0; i < pdmItems.size(); i++ )
	{
		PdmItemObj pdmItem = pdmItems[i];
        if( !pdmItem.hasNodeList() )
            bound.addRect( pdmItem.getBoundingRect() );
	}
	QPointF off = to - bound.boundingRect().topLeft();
	for( i = 0; i < pdmItems.size(); i++ )
	{
		PdmItemObj pdmItem = pdmItems[i];
		if( pdmItem.hasNodeList() )
		{
            // Das ist ein Link
			QPolygonF p = pdmItem.getNodeList();
			for( int j = 0; j < p.size(); j++ )
				p[j] = PdmItemMdl::rastered( p[j] + off );
			pdmItem.setNodeList( p );
		}else
		{
            // Das ist ein Task/Milestone
			pdmItem.setPos( PdmItemMdl::rastered( pdmItem.getPos() + off ) );
		}
	}
}

void PdmCtrl::writeItems(QMimeData *data, const QList<Udb::Obj> & items)
{
    if( items.isEmpty() )
        return;
    Q_ASSERT( !items.first().isNull() );
	Stream::DataWriter out1;
    QList<Udb::Obj> origs = PdmItemObj::writeItems( items, out1 );
	data->setData( QLatin1String( s_mimePdmItems ), out1.getStream() );
    Udb::Obj::writeObjectRefs( data, origs );
}

QList<Udb::Obj> PdmCtrl::readItems(const QMimeData *data )
{
    if( !data->hasFormat( QLatin1String( s_mimePdmItems ) ) )
        return QList<Udb::Obj>();
    Stream::DataReader r( data->data(QLatin1String( s_mimePdmItems )) );
    return PdmItemObj::readItems( d_mdl->getDiagram(), r );
}

const Udb::Obj &PdmCtrl::getDiagram() const
{
    return d_mdl->getDiagram();
}

bool PdmCtrl::focusOn(const Udb::Obj & o)
{
    if( o.isNull() || !getView()->isIdle() )
        return d_mdl->contains( o.getOid() );
    QGraphicsItem* i = d_mdl->selectObject( o );
    if( i )
    {
        getView()->ensureVisible( i );
        return true;
    }else
        return false;
}


