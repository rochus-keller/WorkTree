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

#include "AssigViewCtrl.h"
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include "WtTypeDefs.h"
#include "ObjectTitleFrame.h"
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QMimeData>
#include <Udb/Idx.h>
#include <QtGui/QHBoxLayout>
#include "ObjectHelper.h"
#include "AssigViewMdl.h"
#include "PersonListView.h"
#include "TextViewCtrl.h" // Wegen Pin
using namespace Wt;

const char* AssigViewCtrl::s_mimeAssignments = "application/worktree/assignments";

AssigViewCtrl::AssigViewCtrl(QWidget *parent) :
    QObject(parent)
{
}

AssigViewCtrl *AssigViewCtrl::create(QWidget *parent, Udb::Transaction *txn)
{
    Q_ASSERT( txn != 0 );
    QWidget* pane = new QWidget( parent );
    QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->setSpacing( 2 );
    hbox->setMargin( 0 );
    vbox->addLayout( hbox );

    ObjectTitleFrame* title = new ObjectTitleFrame( pane );
    hbox->addWidget( title );

    CheckLabel* pin = new CheckLabel( pane );
	pin->setPixmap( QPixmap( ":/images/pin.png" ) );
    pin->setToolTip( tr("Pin to current object") );
    hbox->addWidget( pin );

    QTreeView* tree = new QTreeView( pane );
    tree->setHeaderHidden( true );
	tree->setSelectionMode( QAbstractItemView::ExtendedSelection );
    tree->setRootIsDecorated( false );
	tree->setEditTriggers( QAbstractItemView::NoEditTriggers );
	tree->setDragDropMode( QAbstractItemView::DropOnly );
	tree->setDragDropOverwriteMode( false );
	tree->setAlternatingRowColors(true);
    vbox->addWidget( tree );

    AssigViewMdl* mdl = new AssigViewMdl( tree );
    tree->setModel( mdl );

    AssigViewCtrl* ctrl = new AssigViewCtrl( pane );
    ctrl->d_title = title;
    ctrl->d_tree = tree;
    ctrl->d_mdl = mdl;
    ctrl->d_pin = pin;

    connect( tree->selectionModel(), SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
        ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(clicked(QModelIndex)), ctrl, SLOT(onClick(QModelIndex)) );
    connect( tree, SIGNAL(doubleClicked(QModelIndex)), ctrl, SLOT(onDblClick(QModelIndex)) );
    connect( mdl, SIGNAL(signalLinkTo(QList<Udb::Obj>,QModelIndex,int)),
             ctrl, SLOT(onLinkTo(QList<Udb::Obj>,QModelIndex,int)) );
    connect( title, SIGNAL(signalClicked()), ctrl, SLOT(onTitleClick()) );

    txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

    return ctrl;
}

void AssigViewCtrl::addCommands(Gui2::AutoMenu * pop)
{
    pop->addCommand( tr("New RASCI Assignment..."), this, SLOT( onAddRasciAssig() ) );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Set RASCI" ), pop );
    pop->addMenu( sub );
    QAction* a;
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Unknown ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Unknown );
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Responsible ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Responsible );
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Accountable ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Accountable );
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Supportive ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Supportive );
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Consulted ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Consulted );
    a = sub->addCommand( WtTypeDefs::formatRasciRole( Rasci_Informed ), this, SLOT(onSetRasciAssig()) );
    a->setCheckable(true);
    a->setData( Rasci_Informed );
    pop->addSeparator();
    pop->addCommand( tr("Cut..."), this, SLOT( onCut() ), tr("CTRL+X"), true );
    pop->addCommand( tr("Copy"), this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addCommand( tr("Paste"), this, SLOT( onPaste() ), tr("CTRL+V"), true );
    pop->addSeparator();
    pop->addCommand( tr("Delete..."), this, SLOT( onDelete() ), tr("CTRL+D"), true );

}

void AssigViewCtrl::writeItems(QMimeData *data, const QList<Udb::Obj> & assigs )
{
    if( assigs.isEmpty() )
        return;
	Stream::DataWriter out1;
	out1.writeSlot( Stream::DataCell().setUuid( assigs.first().getDb()->getDbUuid() ) );
	for( int i = 0; i < assigs.size(); i++ )
	{
		Stream::DataCell obj = assigs[i].getValue( AttrAssigObject);
		Stream::DataCell prin = assigs[i].getValue( AttrAssigPrincipal);

		if( obj.isOid() && prin.isOid() )
		{
			out1.startFrame();
			out1.writeSlot( Stream::DataCell().setAtom( assigs[i].getType() ), Stream::NameTag( "type" ) );
			out1.writeSlot( obj, Stream::NameTag( "obj" ) );
			out1.writeSlot( prin, Stream::NameTag( "prin" ) );
			switch( assigs[i].getType() )
			{
//			case TypeWorkAssig:
//				out1.writeSlot( assigs[i].getValue( AttrPercentage ), Stream::NameTag( "attr" ) );
//				break;
			case TypeRasciAssig:
				out1.writeSlot( assigs[i].getValue( AttrRasciRole ), Stream::NameTag( "attr" ) );
				break;
			}
			out1.endFrame();
		}
	}
    data->setData( QLatin1String( s_mimeAssignments ), out1.getStream() );
}

QList<Udb::Obj> AssigViewCtrl::readItems(const QMimeData *data)
{
    QList<Udb::Obj> res;
    if( !data->hasFormat( QLatin1String( s_mimeAssignments ) ) || d_title->getObj().isNull() )
        return res;
    Stream::DataReader r( QApplication::clipboard()->mimeData()->data( s_mimeAssignments ) );
	Stream::DataReader::Token t = r.nextToken();
	if( t == Stream::DataReader::Slot && r.getValue().getUuid() == d_title->getObj().getDb()->getDbUuid() )
	{
		t = r.nextToken();
		while( t == Stream::DataReader::BeginFrame )
		{
			quint32 type = 0;
			Udb::Obj prin;
			Stream::DataCell attr;
			t = r.nextToken();
			while( t == Stream::DataReader::Slot )
			{
				Stream::NameTag name = r.getName().getTag();
				if( name.equals( "type" ) )
					type = r.getValue().getAtom();
				else if( name.equals( "prin" ) )
					prin = d_title->getObj().getObject( r.getValue().getOid() );
				else if( name.equals( "attr" ) )
					attr = r.getValue();
				t = r.nextToken();
			}
			Q_ASSERT( t == Stream::DataReader::EndFrame );
			Q_ASSERT( type == TypeRasciAssig );
			Q_ASSERT( !prin.isNull() );
			Udb::Obj assig = ObjectHelper::createObject( type, d_title->getObj() );
			res.append( assig );
			assig.setValue( AttrAssigObject, d_title->getObj() );
			assig.setValue( AttrAssigPrincipal, prin );
			switch( type )
			{
//			case TypeWorkAssig:
//				assig.setValue( AttrPercentage, attr );
//				break;
			case TypeRasciAssig:
				assig.setValue( AttrRasciRole, attr );
				break;
			}
			t = r.nextToken();
		}
		return res;
	}else
        return res;
}

void AssigViewCtrl::showAssig(const Udb::Obj & o)
{
    QModelIndex i = d_mdl->getIndex( o );
    if( i.isValid() )
    {
        d_tree->selectionModel()->clearSelection();
        d_tree->setCurrentIndex( i );
        d_tree->scrollTo( i );
    }
}

void AssigViewCtrl::onAddRasciAssig()
{
    ENABLED_IF( !d_title->getObj().isNull() );

    Udb::Obj assignTo = d_title->getObj();
    Q_ASSERT( WtTypeDefs::isRasciAssignable( assignTo.getType() ) );

    PersonSelectorDlg dlg( d_tree, assignTo.getTxn(),
                           PersonListView::Human | PersonListView::Role | PersonListView::OrgUnit,
                           PersonListView::Human | PersonListView::Role | PersonListView::OrgUnit );
	dlg.setWindowTitle( tr("Select Principal - WorkTree") );
	const quint64 id = dlg.selectOne();
	if( id == 0 )
		return;

    const int role = selectRasciRole();
    if( role == -1 )
        return;

	Udb::Obj assig = ObjectHelper::createObject( TypeRasciAssig, assignTo );
	assig.setValue( AttrAssigObject, assignTo );
	assig.setValue( AttrAssigPrincipal, assignTo.getObject( id ) );
    assig.setValue( AttrRasciRole, Stream::DataCell().setUInt8( role ) );
	assig.commit();
	d_mdl->refill();
	d_tree->setCurrentIndex( d_mdl->getIndex( assig ) );
    d_tree->scrollTo( d_tree->currentIndex() );
    d_tree->resizeColumnToContents(0);
}

void AssigViewCtrl::onSetRasciAssig()
{
    QModelIndexList sr = d_tree->selectionModel()->selectedRows();
    if( sr.size() == 1 )
    {
        Udb::Obj o = d_mdl->getObject( sr.first() );
        CHECKED_IF( o.getType() == TypeRasciAssig, !o.isNull() &&
                    Gui2::UiFunction::me()->data().toInt() == o.getValue( AttrRasciRole ).getUInt8() );
        o.setValue( AttrRasciRole, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
        o.commit();
        d_tree->resizeColumnToContents(0);
    }else
    {
        CHECKED_IF( true, false );
        foreach( QModelIndex i, sr )
        {
            Udb::Obj o = d_mdl->getObject( i );
            if( o.getType() == TypeRasciAssig )
                o.setValue( AttrRasciRole, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
        }
        d_title->getObj().getTxn()->commit();
        d_tree->resizeColumnToContents(0);
    }
}

void AssigViewCtrl::onDelete()
{
    QModelIndexList sr = d_tree->selectionModel()->selectedRows();
    ENABLED_IF( !sr.isEmpty() );
    if( QMessageBox::question( d_tree, tr("Delete Assignments - WorkTree"),
		tr("Are you sure you want to delete the %1 selected assignments? This cannot be undone." ).arg( sr.size() ),
		QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok ) == QMessageBox::Cancel )
		return;
    foreach( QModelIndex i, sr )
    {
        Udb::Obj o = d_mdl->getObject( i );
        ObjectHelper::erase( o );
    }
    d_title->getObj().getTxn()->commit();
    d_mdl->refill();
    d_tree->resizeColumnToContents(0);
}

void AssigViewCtrl::onPaste()
{
    ENABLED_IF( ( QApplication::clipboard()->mimeData()->hasFormat( Udb::Obj::s_mimeObjectRefs ) ||
                  QApplication::clipboard()->mimeData()->hasFormat( s_mimeAssignments ) ) &&
                !d_title->getObj().isNull() );

    if( QApplication::clipboard()->mimeData()->hasFormat( s_mimeAssignments ) )
    {
        QList<Udb::Obj> objs = readItems( QApplication::clipboard()->mimeData() );
        if( !objs.isEmpty() )
        {
            d_title->getObj().getTxn()->commit();
            d_mdl->refill();
            d_tree->resizeColumnToContents(0);
        }
    }else if( QApplication::clipboard()->mimeData()->hasFormat( Udb::Obj::s_mimeObjectRefs ) )
    {
        QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( QApplication::clipboard()->mimeData(),
                                                           d_title->getObj().getTxn() );
        if( !objs.isEmpty() )
            onLinkTo( objs, QModelIndex(), -1 );
    }
}

void AssigViewCtrl::onCopy()
{
    QModelIndexList sr = d_tree->selectionModel()->selectedRows();
    ENABLED_IF( !sr.isEmpty() );

    QList<Udb::Obj> sel;
    foreach( QModelIndex i, sr )
        sel.append( d_mdl->getObject( i, false ) );

    QMimeData* mime = new QMimeData();
    writeItems( mime, sel );
    Udb::Obj::writeObjectRefs( mime, sel );
    QApplication::clipboard()->setMimeData( mime );
}

void AssigViewCtrl::onCut()
{
    QModelIndexList sr = d_tree->selectionModel()->selectedRows();
    ENABLED_IF( !sr.isEmpty() );
    if( QMessageBox::question( d_tree, tr("Delete Assignments - WorkTree"),
		tr("Are you sure you want to delete the %1 selected assignments? This cannot be undone." ).arg( sr.size() ),
		QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok ) == QMessageBox::Cancel )
		return;

    QList<Udb::Obj> sel;
    foreach( QModelIndex i, sr )
        sel.append( d_mdl->getObject( i, false ) );

    QMimeData* mime = new QMimeData();
    writeItems( mime, sel );
    QApplication::clipboard()->setMimeData( mime );

    foreach( Udb::Obj o, sel )
        ObjectHelper::erase( o );
    d_title->getObj().getTxn()->commit();
    d_mdl->refill();
}

QWidget *AssigViewCtrl::getWidget() const
{
    return static_cast<QWidget*>( parent() );
}


void AssigViewCtrl::setObj(const Udb::Obj & o)
{
    if( d_pin->isChecked() )
        return;
    if( !WtTypeDefs::isRasciAssignable( o.getType() ) )
        return;

    d_title->setObj( o );
    d_mdl->setObject( o );
    d_tree->resizeColumnToContents(0);
}

void AssigViewCtrl::onDblClick(const QModelIndex & i)
{
    Udb::Obj o = d_mdl->getObject( i, true );
    emit signalSelect( o, true );
}

void AssigViewCtrl::onClick(const QModelIndex & i)
{
    Udb::Obj o = d_mdl->getObject( i, true );
    emit signalSelect( o, false );
}

void AssigViewCtrl::onSelectionChanged()
{
    QModelIndexList sr = d_tree->selectionModel()->selectedRows();
    if( false ) // sr.size() == 1 )
    {
        Udb::Obj o = d_mdl->getObject( d_tree->currentIndex(), true );
        emit signalSelect( o, false );
    }
}

void AssigViewCtrl::onDbUpdate(Udb::UpdateInfo info)
{
    switch( info.d_kind )
	{
    case Udb::UpdateInfo::ObjectErased:
        if( info.d_id == d_title->getObj().getOid() )
        {
            d_pin->setChecked( false );
            d_title->setObj( Udb::Obj() );
            d_mdl->setObject( Udb::Obj() );
        }
        break;
    }
}

int AssigViewCtrl::selectRasciRole(int pre)
{
    QStringList syms;
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Unknown ) );
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Responsible ) );
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Accountable ) );
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Supportive ) );
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Consulted ) );
    syms.append( WtTypeDefs::formatRasciRole( Rasci_Informed ) );
    bool ok;
    const QString str = QInputDialog::getItem( d_tree, tr("RASCI Assignment - WorkTree "),
                     tr("Select:"), syms, pre, false, &ok );
    if( !ok )
        return -1;
    else
        return syms.indexOf( str );
}

void AssigViewCtrl::onLinkTo(const QList<Udb::Obj> &what, const QModelIndex &toParent, int beforeRow)
{
    Udb::Obj assignTo = d_title->getObj();
    foreach( Udb::Obj o, what )
    {
        if( WtTypeDefs::isRasciPrincipal( o.getType() ) )
        {
            Udb::Obj assig = ObjectHelper::createObject( TypeRasciAssig, assignTo );
            assig.setValue( AttrAssigObject, assignTo );
            assig.setValue( AttrAssigPrincipal, o );
        }
    }
	assignTo.commit();
	d_mdl->refill();
    d_tree->resizeColumnToContents(0);
}

void AssigViewCtrl::onTitleClick()
{
    if( !d_title->getObj().isNull() )
        emit signalSelect( d_title->getObj(), false );
}
