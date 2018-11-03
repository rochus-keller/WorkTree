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

#include "ImpCtrl.h"
#include "ImpMdl.h"
#include "Funcs.h"
#include <QtGui/QTreeView>
#include <QtCore/QMimeData>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QProgressDialog>
#include <Udb/Transaction.h>
#include <Oln2/OutlineStream.h>
#include <Oln2/OutlineItem.h>
#include "WtTypeDefs.h"
#include "ObjectHelper.h"
#include "WorkTreeApp.h"
#include "TaskAttrDlg.h"
#include "PdmItemObj.h"
using namespace Wt;

const char* ImpCtrl::s_mimeImp = "application/worktree/imp-data";

ImpCtrl::ImpCtrl(QTreeView * tree, GenericMdl *mdl) :
    GenericCtrl(tree, mdl)
{
}

ImpCtrl *ImpCtrl::create(QWidget *parent, const Udb::Obj &root)
{
    QTreeView* tree = new QTreeView( parent );
    tree->setHeaderHidden( true );
	tree->setSelectionMode( QAbstractItemView::ExtendedSelection );
	tree->setEditTriggers( QAbstractItemView::SelectedClicked );
	tree->setDragDropMode( QAbstractItemView::DragDrop );
	tree->setDragEnabled( true );
	tree->setDragDropOverwriteMode( false );
	tree->setAlternatingRowColors(true);
	tree->setIndentation( 15 );
	tree->setExpandsOnDoubleClick( false );

    ImpMdl* mdl = new ImpMdl( tree );
    tree->setModel( mdl );

    ImpCtrl* ctrl = new ImpCtrl(tree, mdl);
    ctrl->setRoot( root );

    connect( mdl, SIGNAL(signalNameEdited(QModelIndex,QVariant)),
             ctrl, SLOT(onNameEdited(QModelIndex,QVariant) ) );
    connect( mdl, SIGNAL(signalMoveTo(QList<Udb::Obj>,QModelIndex,int)),
             ctrl, SLOT(onMoveTo(QList<Udb::Obj>,QModelIndex,int)) );
    connect( tree->selectionModel(), SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
        ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(clicked(QModelIndex)), ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(doubleClicked(QModelIndex)), ctrl, SLOT(onDoubleClicked(QModelIndex)) );

    return ctrl;
}

void ImpCtrl::addCommands( Gui2::AutoMenu * pop )
{
	pop->addAction( tr("Expand All"), getTree(), SLOT( expandAll() ) );
	pop->addAction( tr("Expand Selected"), this, SLOT( onExpandSelected() ) );
	pop->addAction( tr("Collapse All"), getTree(), SLOT( collapseAll() ) );
    pop->addSeparator();
    pop->addCommand( tr("Move up"), this, SLOT(onMoveUp()), tr("SHIFT+Up"), true );
    pop->addCommand( tr("Move down"), this, SLOT(onMoveDown()), tr("SHIFT+Down"), true );
	pop->addSeparator();
    pop->addCommand( tr("New Item (same type)"), this, SLOT(onAddNext()), tr("CTRL+N"), true );
	pop->addCommand( tr("New Task"), this, SLOT(onAddTask()) );
    pop->addCommand( tr("New Milestone"), this, SLOT(onAddMilestone()) );
    pop->addCommand( tr("New Event"), this, SLOT(onAddEvent()) );
    pop->addCommand( tr("New Accomplishment"), this, SLOT(onAddAccomplishment()) );
    pop->addCommand( tr("New Criterion"), this, SLOT(onAddCriterion()) );
    pop->addSeparator();
    pop->addCommand( tr("Toggle Task/Milestone"), this, SLOT(onConvertTask()) );
    pop->addCommand( tr("To Criterion"), this, SLOT(onConvertToCriterion() ) );

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

    pop->addSeparator();
    pop->addCommand( tr("Copy"), this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addCommand( tr("Paste"), this, SLOT( onPaste() ), tr("CTRL+V"), true );
    pop->addSeparator();
	pop->addCommand( tr("Edit Name"), this, SLOT( onEditName() ) );
    pop->addCommand( tr("Edit Attributes..."), this, SLOT(onEditAttrs()), tr("CTRL+E"), true );
    pop->addCommand( tr("Recreate Diagrams..."), this, SLOT( onRecreateDiagrams() ) );
    pop->addCommand( tr("Delete Items..."), this, SLOT( onDeleteItems() ), tr("CTRL+D"), true );
}

void ImpCtrl::writeMimeImp(QMimeData *data, const QList<Udb::Obj> & objs)
{
    // Schreibe Values
    Stream::DataWriter out2;
    out2.startFrame( Stream::NameTag( "imp" ) );
    foreach( Udb::Obj o, objs)
	{
        writeTo( o, out2 );
	}
    out2.endFrame();
    data->setData( QLatin1String( s_mimeImp ), out2.getStream() );
    getMdl()->getRoot().getTxn()->commit(); // RISK: wegen getUuid in OutlineStream
}

QList<Udb::Obj> ImpCtrl::readMimeImp(const QMimeData *data, Udb::Obj &parent)
{
    QList<Udb::Obj> res;
    Stream::DataReader in( data->data(QLatin1String( s_mimeImp )) );

    Stream::DataReader::Token t = in.nextToken();
    if( t == Stream::DataReader::BeginFrame && in.getName().getTag().equals( "imp" ) )
    {
        Stream::DataReader::Token t = in.nextToken();
        while( t == Stream::DataReader::BeginFrame )
        {
            Udb::Obj o = readImpElement( in, parent );
            if( !o.isNull() )
                res.append( o );
            t = in.nextToken();
        }
        getMdl()->getRoot().getTxn()->commit();
    }
    return res;
}

void ImpCtrl::onAddTask()
{
    add( TypeTask );
}

void ImpCtrl::onAddMilestone()
{
    add( TypeMilestone );
}

void ImpCtrl::onAddEvent()
{
    add( TypeImpEvent );
}

void ImpCtrl::onAddAccomplishment()
{
    add( TypeAccomplishment );
}

void ImpCtrl::onAddCriterion()
{
    add( TypeCriterion );
}

void ImpCtrl::onConvertTask()
{
    Udb::Obj doc = getSelectedObject();
    ENABLED_IF( WtTypeDefs::canConvert( doc, TypeTask ) ||
                WtTypeDefs::canConvert( doc, TypeMilestone ) );
    switch( doc.getType() )
    {
    case TypeTask:
        ObjectHelper::retypeObject( doc, TypeMilestone );
        doc.commit();
        break;
    case TypeMilestone:
    case TypeCriterion:
        ObjectHelper::retypeObject( doc, TypeTask );
        doc.commit();
        break;
    }
}

void ImpCtrl::onConvertToCriterion()
{
    Udb::Obj doc = getSelectedObject();
    ENABLED_IF( WtTypeDefs::canConvert( doc, TypeCriterion ) );
    ObjectHelper::retypeObject( doc, TypeCriterion );
    doc.commit();
}

void ImpCtrl::onCopy()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
    ENABLED_IF( !sr.isEmpty() );

    QList<Udb::Obj> objs;
    QMimeData* mime = new QMimeData();
    foreach( QModelIndex i, sr )
	{
		Udb::Obj o = getMdl()->getObject( i );
		Q_ASSERT( !o.isNull() );
        objs.append( o );
	}
    writeMimeImp( mime, objs );
    Udb::Obj::writeObjectRefs( mime, objs );
    QApplication::clipboard()->setMimeData( mime );
}

void ImpCtrl::onPaste()
{
    Udb::Obj doc = getSelectedObject( true );
    ENABLED_IF( !doc.isNull() && QApplication::clipboard()->mimeData()->hasFormat( s_mimeImp ) );

    readMimeImp( QApplication::clipboard()->mimeData(), doc );
}

void ImpCtrl::onSetMsType()
{
    Udb::Obj o = getSelectedObject();
    CHECKED_IF( o.getType() == TypeMilestone,
                Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrMsType ).getUInt8() );

    if( Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrMsType ).getUInt8() )
        return;
    ObjectHelper::setMsType( o, Gui2::UiFunction::me()->data().toInt() );
    o.commit();
}

void ImpCtrl::onSetTaskType()
{
    Udb::Obj o = getSelectedObject();
    CHECKED_IF( o.getType() == TypeTask,
                Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrTaskType ).getUInt8() );
    if( Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrTaskType ).getUInt8() )
        return;
    o.setValue( AttrTaskType, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
    o.commit();
}

void ImpCtrl::onEditAttrs()
{
    Udb::Obj doc = getSelectedObject();
    ENABLED_IF( doc.getType() == TypeTask || doc.getType() == TypeMilestone );

    TaskAttrDlg dlg( getTree() );
    dlg.edit( doc );
}

void ImpCtrl::onRecreateDiagrams()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
    QList<Udb::Obj> docs;
    foreach( QModelIndex i, sr )
    {
        Udb::Obj doc = getMdl()->getObject( i );
        if( WtTypeDefs::isPdmDiagram( doc.getType() ) )
            docs.append( doc );
    }
	ENABLED_IF( !docs.isEmpty() );

    QDialog dlg( getTree() );
    dlg.setWindowTitle( tr("Recreate Diagrams - WorkTree") );
    QVBoxLayout vbox( &dlg );
    vbox.addWidget( new QLabel( tr("Do you really want to recreate the diagrams \n"
                                   "of the %1 selected diagram items?\n"
                                   "This cannot be undone and may take some time!").arg(docs.size())));
    QCheckBox recursive( tr("Recursive (also do it for all subordinate items)"), &dlg );
    vbox.addWidget( &recursive );
    QHBoxLayout hbox;
    vbox.addLayout( &hbox );
    QSpinBox spin( &dlg );
    spin.setRange(0,255);
    spin.setValue(1);
    hbox.addWidget( &spin );
    hbox.addWidget( new QLabel( tr("How many levels to extend"), &dlg ) );
    QCheckBox succ( tr("Extend successors"), &dlg );
    succ.setChecked( true );
    vbox.addWidget( &succ );
    QCheckBox pred( tr("Extend predecessors"), &dlg );
    pred.setChecked( true );
    vbox.addWidget( &pred );
    QCheckBox layout( tr("Layout diagrams"), &dlg );
    layout.setChecked( true );
    vbox.addWidget( &layout );
    QDialogButtonBox bb(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, &dlg );
	vbox.addWidget( &bb );
    connect( &bb, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect( &bb, SIGNAL(rejected()), &dlg, SLOT(reject()));
    if( dlg.exec() == QDialog::Rejected )
        return;

    if( layout.isChecked() && !PdmItemObj::s_layouter.prepareEngine( getTree() ) )
        return;
    QProgressDialog progress( getTree() );
    progress.setWindowTitle( dlg.windowTitle() );
    progress.setAutoClose(false);
    progress.setValue( 1 );
    QApplication::setOverrideCursor( Qt::WaitCursor );
    foreach( Udb::Obj doc, docs )
    {
        if( !PdmItemObj::createDiagram( doc, true, layout.isChecked(), recursive.isChecked(),
                                   spin.value(), succ.isChecked(), pred.isChecked(), &progress ) )
            break;
    }
    getMdl()->getRoot().getTxn()->commit();
    QApplication::restoreOverrideCursor();
}

void ImpCtrl::writeTo(const Udb::Obj &o, Stream::DataWriter &out) const
{
    switch( o.getType() )
    {
    case TypeTask:
        out.startFrame( Stream::NameTag( "task" ) );
        break;
    case TypeMilestone:
        out.startFrame( Stream::NameTag( "mlst" ) );
        break;
    case TypeImpEvent:
        out.startFrame( Stream::NameTag( "evt" ) );
        break;
    case TypeAccomplishment:
        out.startFrame( Stream::NameTag( "acc" ) );
        break;
    case TypeCriterion:
        out.startFrame( Stream::NameTag( "crit" ) );
        break;
//    case TypePdmItem:
//        out.startFrame( Stream::NameTag( "pdmi" ) );
//        // TODO: im Moment zeigen gepastete auf die ursprünglichen Objekte, was keinen Sinn macht
//        break;
//    case TypePdmDiagram:
//        out.startFrame( Stream::NameTag( "pdmd" ) );
//        break;
//    case TypeLink:
//        out.startFrame( Stream::NameTag( "link" ) );
//        // TODO: im Moment zeigen gepastete auf die ursprünglichen Objekte, was keinen Sinn macht
//        break;
    case TypeRasciAssig:
        out.startFrame( Stream::NameTag( "rasc" ) );
        break;
    default:
        return;
    }
    Stream::DataCell v;
    Udb::Obj ref;
    v = o.getValue( AttrText );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "text" ) );
    v = o.getValue( AttrTaskType );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "taty" ) );
    v = o.getValue( AttrSubTMSCount );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "tmsc" ) );
    v = o.getValue( AttrMsType );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "mst" ) );
    ref = o.getValueAsObj( AttrPred );
    if( !ref.isNull() )
        out.writeSlot( Stream::DataCell().setUuid( ref.getUuid() ), Stream::NameTag( "pred" ) );
    ref = o.getValueAsObj( AttrSucc );
    if( !ref.isNull() )
        out.writeSlot( Stream::DataCell().setUuid( ref.getUuid() ), Stream::NameTag( "succ" ) );
    v = o.getValue( AttrLinkType );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "ltyp" ) );
    ref = o.getValueAsObj( AttrAssigObject );
    if( !ref.isNull() )
        out.writeSlot( Stream::DataCell().setUuid( ref.getUuid() ), Stream::NameTag( "asso" ) );
    ref = o.getValueAsObj( AttrAssigPrincipal );
    if( !ref.isNull() )
        out.writeSlot( Stream::DataCell().setUuid( ref.getUuid() ), Stream::NameTag( "assp" ) );
    v = o.getValue( AttrRasciRole );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "rasr" ) );
    v = o.getValue( AttrShowIds );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "shid" ) );
    v = o.getValue( AttrMarkAlias );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "mara" ) );
    v = o.getValue( AttrPosX );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "posx" ) );
    v = o.getValue( AttrPosY );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "posy" ) );
    v = o.getValue( AttrPointList );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "poil" ) );
    ref = o.getValueAsObj( AttrOrigObject );
    if( !ref.isNull() )
        out.writeSlot( Stream::DataCell().setUuid( ref.getUuid() ), Stream::NameTag( "orig" ) );

    Udb::Obj sub = o.getFirstObj();
	if( !sub.isNull() ) do
	{
		if( sub.getType() == Oln::OutlineItem::TID )
			Oln::OutlineStream::writeTo( out, sub );
        else
            writeTo( sub, out );
	}while( sub.next() );
    out.endFrame();
}

Udb::Obj ImpCtrl::readImpElement(Stream::DataReader &in, Udb::Obj &parent)
{
    Q_ASSERT( in.getCurrentToken() == Stream::DataReader::BeginFrame );
    quint32 type = 0;
    Stream::NameTag tag = in.getName().getTag();
    if( tag.equals( "task" ) )
        type = TypeTask;
    else if( tag.equals( "mlst" ) )
        type = TypeMilestone;
    else if( tag.equals( "evt" ) )
        type = TypeImpEvent;
    else if( tag.equals( "acc" ) )
        type = TypeAccomplishment;
    else if( tag.equals( "crit" ) )
        type = TypeCriterion;
    else if( tag.equals( "pdmi" ) )
        type = TypePdmItem;
    else if( tag.equals( "pdmd" ) )
        type = TypePdmDiagram;
    else if( tag.equals( "link" ) )
        type = TypeLink;
    else if( tag.equals( "rasc" ) )
        type = TypeRasciAssig;
    else if( tag.equals( Oln::OutlineStream::s_olnFrameTag ) )
    {
        Oln::OutlineStream str;
		Udb::Obj o = str.readFrom( in, parent.getTxn(), parent.getOid(), false );
        o.aggregateTo( parent );
        return o;
    }
	if( Funcs::isValidAggregate( parent.getType(), type ) )
    {
        Udb::Obj wbs = ObjectHelper::createObject( type, parent );
        Stream::DataReader::Token t = in.nextToken();
        Udb::Transaction* txn = parent.getTxn();
        while( t == Stream::DataReader::Slot || t == Stream::DataReader::BeginFrame )
        {
            switch( t )
            {
            case Stream::DataReader::Slot:
                {
                    const Stream::NameTag name = in.getName().getTag();
                    if( name.equals( "text" ) )
                        wbs.setValue( AttrText, in.readValue() );
                    else if( name.equals( "taty" ) )
                        wbs.setValue( AttrTaskType, in.readValue() );
                    else if( name.equals( "tmsc" ) )
                        wbs.setValue( AttrSubTMSCount, in.readValue() );
                    else if( name.equals( "mst" ) )
                        wbs.setValue( AttrMsType, in.readValue() );
                    else if( name.equals( "pred" ) )
                        wbs.setValueAsObj( AttrPred, txn->getObject(in.readValue()) );
                    else if( name.equals( "succ" ) )
                        wbs.setValueAsObj( AttrSucc, txn->getObject(in.readValue()) );
                    else if( name.equals( "ltyp" ) )
                        wbs.setValue( AttrLinkType, in.readValue() );
                    else if( name.equals( "asso" ) )
                        wbs.setValueAsObj( AttrAssigObject, txn->getObject(in.readValue()) );
                    else if( name.equals( "assp" ) )
                        wbs.setValueAsObj( AttrAssigPrincipal, txn->getObject(in.readValue()) );
                    else if( name.equals( "rasr" ) )
                        wbs.setValue( AttrRasciRole, in.readValue() );
                    else if( name.equals( "shid" ) )
                        wbs.setValue( AttrShowIds, in.readValue() );
                    else if( name.equals( "mara" ) )
                        wbs.setValue( AttrMarkAlias, in.readValue() );
                    else if( name.equals( "posx" ) )
                        wbs.setValue( AttrPosX, in.readValue() );
                    else if( name.equals( "posy" ) )
                        wbs.setValue( AttrPosY, in.readValue() );
                    else if( name.equals( "poil" ) )
                        wbs.setValue( AttrPointList, in.readValue() );
                    else if( name.equals( "orig" ) )
                        wbs.setValueAsObj( AttrOrigObject, txn->getObject(in.readValue()) );
                }
                break;
            case Stream::DataReader::BeginFrame:
                readImpElement( in, wbs );
                break;
            }
            t =in.nextToken();
        }
        Q_ASSERT( t == Stream::DataReader::EndFrame );
        return wbs;
    }else
    {
        // Lese bis nächstes EndFrame auf gleicher Ebene
        in.skipToEndFrame();
        Q_ASSERT( in.getCurrentToken() == Stream::DataReader::EndFrame );
        return Udb::Obj();
    }
}

