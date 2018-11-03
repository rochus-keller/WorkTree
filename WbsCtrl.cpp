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

#include "WbsCtrl.h"
#include <QtGui/QTreeView>
#include <QtGui/QInputDialog>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QMimeData>
#include <Udb/Transaction.h>
#include <Oln2/OutlineStream.h>
#include <Oln2/OutlineItem.h>
#include "WtTypeDefs.h"
#include "ObjectHelper.h"
#include "WbsMdl.h"
#include "WorkTreeApp.h"
#include "Funcs.h"
using namespace Wt;

const char* WbsCtrl::s_mimeWbs = "application/worktree/wbs-data";

WbsCtrl::WbsCtrl(QTreeView *tree, GenericMdl* mdl ) :
    GenericCtrl(tree,mdl)
{
}

WbsCtrl *WbsCtrl::create(QWidget *parent, const Udb::Obj &root )
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

    WbsMdl* mdl = new WbsMdl( tree );
    tree->setModel( mdl );

    WbsCtrl* ctrl = new WbsCtrl(tree, mdl);
    ctrl->setRoot( root );

    connect( mdl, SIGNAL(signalNameEdited(QModelIndex,QVariant)),
             ctrl, SLOT(onNameEdited(QModelIndex,QVariant) ) );
    connect( mdl, SIGNAL(signalMoveTo(QList<Udb::Obj>,QModelIndex,int)),
             ctrl, SLOT(onMoveTo(QList<Udb::Obj>,QModelIndex,int)) );
    connect( tree->selectionModel(), SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
        ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(clicked(QModelIndex)), ctrl, SLOT( onSelectionChanged() ) );
    //connect( tree, SIGNAL(doubleClicked(QModelIndex)), ctrl, SLOT(onDblClick(QModelIndex)) );

    return ctrl;
}

void WbsCtrl::addCommands(Gui2::AutoMenu * pop)
{
    pop->addAction( tr("Expand All"), getTree(), SLOT( expandAll() ) );
	pop->addAction( tr("Expand Selected"), this, SLOT( onExpandSelected() ) );
	pop->addAction( tr("Collapse All"), getTree(), SLOT( collapseAll() ) );
    pop->addSeparator();
    pop->addCommand( tr("Move up"), this, SLOT(onMoveUp()), tr("SHIFT+Up"), true );
    pop->addCommand( tr("Move down"), this, SLOT(onMoveDown()), tr("SHIFT+Down"), true );
	pop->addSeparator();
    pop->addCommand( tr("New Item (same type)"), this, SLOT(onAddNext()), tr("CTRL+N"), true );
	pop->addCommand( tr("New Deliverable"), this, SLOT(onAddDeliverable()) );
    pop->addCommand( tr("New Work"), this, SLOT(onAddWork()) );
	pop->addSeparator();
    pop->addCommand( tr("Copy"), this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addCommand( tr("Paste"), this, SLOT( onPaste() ), tr("CTRL+V"), true );
    pop->addSeparator();
	pop->addCommand( tr("Edit Name"), this, SLOT( onEditName() ) );
    pop->addCommand( tr("Delete Items"), this, SLOT( onDeleteItems() ), tr("CTRL+D"), true );
}

void WbsCtrl::writeMimeWbs(QMimeData *data, const QList<Udb::Obj> & objs)
{
    // Schreibe Values
    Stream::DataWriter out2;
    out2.startFrame( Stream::NameTag( "wbs" ) );
    foreach( Udb::Obj o, objs)
	{
        writeTo( o, out2 );
	}
    out2.endFrame();
    data->setData( QLatin1String( s_mimeWbs ), out2.getStream() );
    getMdl()->getRoot().getTxn()->commit(); // RISK: wegen getUuid in OutlineStream
}

QList<Udb::Obj> WbsCtrl::readMimeWbs(const QMimeData *data, Udb::Obj &parent)
{
    QList<Udb::Obj> res;
    Stream::DataReader in( data->data(QLatin1String( s_mimeWbs )) );

    Stream::DataReader::Token t = in.nextToken();
    if( t == Stream::DataReader::BeginFrame && in.getName().getTag().equals( "wbs" ) )
    {
        Stream::DataReader::Token t = in.nextToken();
        while( t == Stream::DataReader::BeginFrame )
        {
            Udb::Obj o = readWbsElement( in, parent );
            if( !o.isNull() )
                res.append( o );
            t = in.nextToken();
        }
        getMdl()->getRoot().getTxn()->commit();
    }
    return res;
}

Udb::Obj WbsCtrl::readWbsElement(Stream::DataReader &in, Udb::Obj &parent )
{
    Q_ASSERT( in.getCurrentToken() == Stream::DataReader::BeginFrame );
    quint32 type = 0;
    Stream::NameTag tag = in.getName().getTag();
    if( tag.equals( "work" ) )
        type = TypeWork;
    else if( tag.equals( "deli" ) )
        type = TypeDeliverable;
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
        while( t == Stream::DataReader::Slot || t == Stream::DataReader::BeginFrame )
        {
            switch( t )
            {
            case Stream::DataReader::Slot:
                {
                    const Stream::NameTag name = in.getName().getTag();
                    if( name.equals( "text" ) )
                    {
                        wbs.setValue( AttrText, in.readValue() );
                    }
                }
                break;
            case Stream::DataReader::BeginFrame:
                readWbsElement( in, wbs );
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

void WbsCtrl::writeTo(const Udb::Obj & o, Stream::DataWriter &out) const
{
    switch( o.getType() )
    {
    case TypeWork:
        out.startFrame( Stream::NameTag( "work" ) );
        break;
    case TypeDeliverable:
        out.startFrame( Stream::NameTag( "deli" ) );
        break;
    default:
        return;
    }
    Stream::DataCell v;
    v = o.getValue( AttrText );
    if( v.hasValue() )
        out.writeSlot( v, Stream::NameTag( "text" ) );

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

void WbsCtrl::onAddDeliverable()
{
    add( TypeDeliverable );
}

void WbsCtrl::onAddWork()
{
    add( TypeWork );
}

void WbsCtrl::onCopy()
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
    writeMimeWbs( mime, objs );
    Udb::Obj::writeObjectRefs( mime, objs );
    QApplication::clipboard()->setMimeData( mime );
}

void WbsCtrl::onPaste()
{
    Udb::Obj doc = getSelectedObject( true );
    ENABLED_IF( !doc.isNull() && QApplication::clipboard()->mimeData()->hasFormat( s_mimeWbs ) );

    readMimeWbs( QApplication::clipboard()->mimeData(), doc );
}
