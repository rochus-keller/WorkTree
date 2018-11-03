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

#include "PdmLinkViewCtrl.h"
#include <QtGui/QVBoxLayout>
#include "WtTypeDefs.h"
#include "ObjectTitleFrame.h"
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <QtGui/QListWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QMimeData>
#include <Udb/Idx.h>
#include "ObjectHelper.h"
#include "PdmItemMdl.h"
#include "PdmItemView.h"
using namespace Wt;

const int LinkRole = Qt::UserRole;
const int PeerRole = Qt::UserRole + 1;

class _PdmLinkList : public QListWidget
{
public:
    _PdmLinkList( QWidget* p, Udb::Transaction *txn ):QListWidget(p),d_txn(txn) {}
    Udb::Transaction* d_txn;
    QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const
    {
        QList<Udb::Obj> objs;
        foreach( QListWidgetItem* i, items )
        {
            objs.append( d_txn->getObject( i->data(PeerRole).toULongLong() ) );
            objs.append( d_txn->getObject( i->data(LinkRole).toULongLong() ) );
        }
        QMimeData* data = new QMimeData();
        Udb::Obj::writeObjectRefs( data, objs );
        return data;
    }
    QStringList mimeTypes () const
    {
        return QStringList() << QLatin1String( Udb::Obj::s_mimeObjectRefs );
    }
};

PdmLinkViewCtrl::PdmLinkViewCtrl(QWidget *parent) :
    QObject(parent),d_view(0)
{
}

Wt::PdmLinkViewCtrl *Wt::PdmLinkViewCtrl::create(QWidget *parent,Udb::Transaction *txn )
{
    Q_ASSERT( txn != 0 );
    QWidget* pane = new QWidget( parent );
    QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

    PdmLinkViewCtrl* ctrl = new PdmLinkViewCtrl( pane );

    ctrl->d_title = new ObjectTitleFrame( pane );
    vbox->addWidget( ctrl->d_title );

    ctrl->d_list = new _PdmLinkList( pane, txn );
    ctrl->d_list->setAlternatingRowColors( true );
    ctrl->d_list->setSelectionMode( QAbstractItemView::ExtendedSelection );
    ctrl->d_list->setDragEnabled( true );
    ctrl->d_list->setDragDropMode( QAbstractItemView::DragOnly );

	vbox->addWidget( ctrl->d_list );

    connect( ctrl->d_list, SIGNAL(itemPressed(QListWidgetItem*)), ctrl,SLOT(onClicked(QListWidgetItem*)) );
    connect( ctrl->d_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), ctrl,SLOT(onDblClick(QListWidgetItem*)));
    connect( ctrl->d_title, SIGNAL(signalClicked()), ctrl, SLOT(onTitleClick()) );

    txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

    return ctrl;
}

void Wt::PdmLinkViewCtrl::setObj(const Udb::Obj & o)
{
    if( o.getType() == TypeLink )
        return;

    d_title->setObj( o );
    d_list->clear();
    if( o.isNull() )
        return;

    Udb::Idx succIdx( o.getTxn(), IndexDefs::IdxSucc );
    if( succIdx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
    {
        addLink( o.getObject( succIdx.getOid() ), true );
    }while( succIdx.nextKey() );
    Udb::Idx predIdx( o.getTxn(), IndexDefs::IdxPred );
    if( predIdx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
    {
        addLink( o.getObject( predIdx.getOid() ), false );
    }while( predIdx.nextKey() );

    d_list->sortItems();
    markObservedLinks();
}

void PdmLinkViewCtrl::clear()
{
    d_title->setObj( Udb::Obj() );
    d_list->clear();
}

QWidget *Wt::PdmLinkViewCtrl::getWidget() const
{
    return static_cast<QWidget*>( parent() );
}

void Wt::PdmLinkViewCtrl::addCommands(Gui2::AutoMenu * pop)
{
    pop->addCommand( tr("Show Link"), this, SLOT( onShowLink() ) );
    pop->addCommand( tr("Show Task/Milestone"),  this, SLOT( onShowPeer() ) );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Set Link Type" ), pop );
    pop->addMenu( sub );
    QAction* a = sub->addCommand( WtTypeDefs::formatLinkType( LinkType_FS ), this, SLOT(onSetLinkType()) );
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
    pop->addCommand( tr("Copy"),  this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addSeparator();
    pop->addCommand( tr("Delete..."),  this, SLOT( onDelete() ), tr("CTRL+D"), true );
}

void PdmLinkViewCtrl::showLink(const Udb::Obj & link)
{
    for( int i = 0; i < d_list->count(); i++ )
    {
        if( d_list->item(i)->data(LinkRole).toULongLong() == link.getOid() )
        {
            d_list->setCurrentItem( d_list->item(i), QItemSelectionModel::SelectCurrent );
            d_list->scrollToItem( d_list->item(i) );
            return;
        }
    }
}

void PdmLinkViewCtrl::setObserved(PdmItemView * v)
{
    d_view = v;
    markObservedLinks();
}

void Wt::PdmLinkViewCtrl::onDbUpdate(Udb::UpdateInfo info)
{
    switch( info.d_kind )
	{
    case Udb::UpdateInfo::ObjectErased:
        if( info.d_id == d_title->getObj().getOid() )
            clear();
        else if( info.d_name == TypeLink )
        {
            for( int i = 0; i < d_list->count(); i++ )
                if( d_list->item(i)->data(LinkRole).toULongLong() == info.d_id )
                {
                    delete d_list->item(i);
                    return;
                }
        }
        break;
    }
}

void PdmLinkViewCtrl::onClicked(QListWidgetItem* item)
{
    if( item == 0 || d_title->getObj().isNull() )
        return;
    emit signalSelect( d_title->getObj().getObject( item->data(LinkRole).toULongLong() ), false );
}

void PdmLinkViewCtrl::onDblClick(QListWidgetItem* item)
{
    if( item == 0 || d_title->getObj().isNull() )
        return;
    Udb::Obj link = d_title->getObj().getObject( item->data(LinkRole).toULongLong() );
    Udb::Obj peer = d_title->getObj().getObject( item->data(PeerRole).toULongLong() );
    emit signalSelect( peer, true );
}

void PdmLinkViewCtrl::onShowLink()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( sel.size() == 1 && !d_title->getObj().isNull() );
    emit signalSelect( d_title->getObj().getObject(
                               sel.first()->data(LinkRole).toULongLong() ), true );
}

void PdmLinkViewCtrl::onShowPeer()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( sel.size() == 1 && !d_title->getObj().isNull() );
    onDblClick( sel.first() );
}

void PdmLinkViewCtrl::onDelete()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( !sel.isEmpty() && !d_title->getObj().isNull() );

    const int res = QMessageBox::warning( getWidget(), tr("Delete Link from Database"),
                                          tr("Do you really want to permanently delete %1 links? This cannot be undone." ).
                                          arg( sel.size() ), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
    if( res == QMessageBox::No )
		return;

    foreach( QListWidgetItem* i, sel )
    {
        Udb::Obj link = d_title->getObj().getObject( i->data(LinkRole).toULongLong() );
        ObjectHelper::erase( link );
    }
    d_title->getObj().getTxn()->commit();
}

void PdmLinkViewCtrl::onTitleClick()
{
    if( !d_title->getObj().isNull() )
        emit signalSelect( d_title->getObj(), false );
}

void PdmLinkViewCtrl::onSetLinkType()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    Udb::Obj link;
    if( sel.size() == 1 && !d_title->getObj().isNull() )
        link = d_title->getObj().getObject( sel.first()->data(LinkRole).toULongLong() );
    CHECKED_IF( !link.isNull(), !link.isNull() &&
                Gui2::UiFunction::me()->data().toInt() == link.getValue( AttrLinkType ).getUInt8() );
    link.setValue( AttrLinkType, Stream::DataCell().setUInt8( Gui2::UiFunction::me()->data().toInt() ) );
    link.commit();
    formatItem( sel.first(), link, link.getValueAsObj(AttrSucc).equals(d_title->getObj()) );
}

void PdmLinkViewCtrl::onCopy()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( !sel.isEmpty() && !d_title->getObj().isNull() );

    QList<Udb::Obj> objs;
    foreach( QListWidgetItem* i, sel )
    {
        objs.append( d_title->getObj().getObject( i->data(PeerRole).toULongLong() ) );
        objs.append( d_title->getObj().getObject( i->data(LinkRole).toULongLong() ) );
    }
    QMimeData* data = new QMimeData();
    Udb::Obj::writeObjectRefs( data, objs );
    QApplication::clipboard()->setMimeData( data );
}

static inline QString _formatLink( const Udb::Obj& link, const Udb::Obj& peer )
{
    return QString("%1 %2 / %3").arg( WtTypeDefs::formatObjectId( link ) ).
            arg( WtTypeDefs::formatLinkType( link.getValue( AttrLinkType ).getUInt8(),true ) ).
            arg( WtTypeDefs::formatObjectTitle( peer ) );
}

void PdmLinkViewCtrl::addLink(const Udb::Obj & link, bool inbound )
{
    if( link.isNull() )
        return;
    QListWidgetItem* lwi = new QListWidgetItem( d_list );
    formatItem( lwi, link, inbound );
}

void PdmLinkViewCtrl::formatItem(QListWidgetItem * lwi, const Udb::Obj & link, bool inbound)
{
    Udb::Obj peer;
    if( inbound )
    {
        peer = link.getValueAsObj( AttrPred );
        lwi->setText( _formatLink( link, peer ) );
        lwi->setToolTip( QString( "Inbound Link/Predecessor: %1" ).arg( lwi->text() ) );
        lwi->setIcon( QIcon( ":/images/pred.png" ) );
    }else
    {
        peer = link.getValueAsObj( AttrSucc );
        lwi->setText( _formatLink( link, peer ) );
        lwi->setToolTip( QString( "Outbound Link/Successor: %1" ).arg( lwi->text() ) );
        lwi->setIcon( QIcon( ":/images/succ.png" ) );
    }
    lwi->setData( LinkRole, link.getOid() );
    lwi->setData( PeerRole, peer.getOid() );
    lwi->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled );
}

void PdmLinkViewCtrl::markObservedLinks()
{
    if( d_view == 0 )
        for( int i = 0; i < d_list->count(); i++ )
            d_list->item(i)->setFont( d_list->font() );
    else
    {
        PdmItemMdl* mdl = d_view->getMdl();
        QFont bold = d_list->font();
        bold.setBold( true );
        for( int i = 0; i < d_list->count(); i++ )
        {
            if( mdl->contains( d_list->item(i)->data(LinkRole).toULongLong() ) &&
                    mdl->contains( d_list->item(i)->data(PeerRole).toULongLong() ) )
                d_list->item(i)->setFont( bold );
            else
                d_list->item(i)->setFont( d_list->font() );
        }
    }
}
