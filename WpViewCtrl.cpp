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

#include "WpViewCtrl.h"
#include "WtTypeDefs.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include "ObjectTitleFrame.h"
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QMimeData>
#include "ObjectHelper.h"
#include <Udb/Idx.h>
#include <Oln2/OutlineUdbMdl.h>
using namespace Wt;

class _WpViewList : public QListWidget
{
public:
    _WpViewList( QWidget* p, ObjectTitleFrame *title ):QListWidget(p),d_title(title) {}
    ObjectTitleFrame* d_title;
    QStringList mimeTypes () const
    {
        return QStringList() << QLatin1String( Udb::Obj::s_mimeObjectRefs );
    }
    bool dropMimeData ( int index, const QMimeData * data, Qt::DropAction action )
    {
        if( d_title->getObj().isNull() )
            return false;
        clearSelection();
        if( data->hasFormat( Udb::Obj::s_mimeObjectRefs ) )
        {
            QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( data, d_title->getObj().getTxn() );
            foreach( Udb::Obj imp, objs )
            {
                if( WtTypeDefs::isImpType( imp.getType() ) &&
                        imp.getValue( AttrWbsRef ).getOid() != d_title->getObj().getOid() )
                {
                    QListWidgetItem* lwi = new QListWidgetItem( this );
                    lwi->setSelected( true );
                    _WpViewList::formatItem( lwi, imp );
                    imp.setValueAsObj( AttrWbsRef, d_title->getObj() );
                }
            }
            sortItems();
            d_title->getObj().getTxn()->commit();
            return true;
        }else
            return false;
    }

    static void formatItem( QListWidgetItem* lwi, const Udb::Obj& imp )
    {
        lwi->setData( Qt::UserRole, imp.getOid() );
        lwi->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        lwi->setText( WtTypeDefs::formatObjectTitle( imp ) );
        lwi->setToolTip( lwi->text() );
        lwi->setIcon( Oln::OutlineUdbMdl::getPixmap( imp.getType() ) );
    }
};

WpViewCtrl::WpViewCtrl(QWidget *parent) :
    QObject(parent)
{
}

WpViewCtrl *WpViewCtrl::create(QWidget *parent, Udb::Transaction *txn)
{
    QWidget* pane = new QWidget( parent );
    QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

    WpViewCtrl* ctrl = new WpViewCtrl( pane );

    ctrl->d_title = new ObjectTitleFrame( pane );
    vbox->addWidget( ctrl->d_title );

    ctrl->d_list = new _WpViewList( pane, ctrl->d_title );
    ctrl->d_list->setAlternatingRowColors( true );
    ctrl->d_list->setSelectionMode( QAbstractItemView::ExtendedSelection );
    ctrl->d_list->setDragEnabled( true );
    ctrl->d_list->setDragDropMode( QAbstractItemView::DropOnly );

	vbox->addWidget( ctrl->d_list );

    connect( ctrl->d_list, SIGNAL(itemPressed(QListWidgetItem*)), ctrl,SLOT(onClicked(QListWidgetItem*)) );
    connect( ctrl->d_title, SIGNAL(signalClicked()), ctrl, SLOT(onTitleClick()) );

    txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

    return ctrl;
}

void WpViewCtrl::addCommands(Gui2::AutoMenu * pop)
{
    pop->addCommand( tr("Copy"),  this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addCommand( tr("Paste"),  this, SLOT( onPaste() ), tr("CTRL+V"), true );
    pop->addSeparator();
    pop->addCommand( tr("Remove Reference"),  this, SLOT( onRemove() ), tr("CTRL+D"), true );
}

void WpViewCtrl::setObj(const Udb::Obj & o)
{
    if( !WtTypeDefs::isWbsType( o.getType() ) )
        return;

    d_title->setObj( o );
    d_list->clear();
    if( o.isNull() )
        return;

    Udb::Idx wbsIdx( o.getTxn(), IndexDefs::IdxWbsRef );
    if( wbsIdx.seek( Stream::DataCell().setOid( o.getOid() ) ) ) do
    {
        Udb::Obj imp = o.getObject( wbsIdx.getOid() );
        QListWidgetItem* lwi = new QListWidgetItem( d_list );
        _WpViewList::formatItem( lwi, imp );
    }while( wbsIdx.nextKey() );

    d_list->sortItems();
}

QWidget *WpViewCtrl::getWidget() const
{
    return static_cast<QWidget*>( parent() );
}

void WpViewCtrl::focusOn(const Udb::Obj &)
{
}

void WpViewCtrl::clear()
{
    d_title->setObj( Udb::Obj() );
    d_list->clear();
}

void WpViewCtrl::onDbUpdate(Udb::UpdateInfo info)
{
    switch( info.d_kind )
	{
    case Udb::UpdateInfo::ObjectErased:
        if( info.d_id == d_title->getObj().getOid() )
            clear();
        else if( WtTypeDefs::isImpType( info.d_name ) )
        {
            for( int i = 0; i < d_list->count(); i++ )
                if( d_list->item(i)->data(Qt::UserRole).toULongLong() == info.d_id )
                {
                    delete d_list->item(i);
                    return;
                }
        }
        break;
    }
}

void WpViewCtrl::onTitleClick()
{
    if( !d_title->getObj().isNull() )
        emit signalSelect( d_title->getObj() );
}

void WpViewCtrl::onCopy()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( !sel.isEmpty() && !d_title->getObj().isNull() );

    QList<Udb::Obj> objs;
    foreach( QListWidgetItem* i, sel )
        objs.append( d_title->getObj().getObject( i->data(Qt::UserRole).toULongLong() ) );
    QMimeData* data = new QMimeData();
    Udb::Obj::writeObjectRefs( data, objs );
    QApplication::clipboard()->setMimeData( data );
}

void WpViewCtrl::onPaste()
{
    ENABLED_IF( ( QApplication::clipboard()->mimeData()->hasFormat( Udb::Obj::s_mimeObjectRefs ) &&
                !d_title->getObj().isNull() ) );

    QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( QApplication::clipboard()->mimeData(),
                                                       d_title->getObj().getTxn() );
    d_list->clearSelection();
    foreach( Udb::Obj imp, objs )
    {
        if( WtTypeDefs::isImpType( imp.getType() ) &&
                imp.getValue( AttrWbsRef ).getOid() != d_title->getObj().getOid() )
        {
            QListWidgetItem* lwi = new QListWidgetItem( d_list );
            lwi->setSelected( true );
            _WpViewList::formatItem( lwi, imp );
            imp.setValueAsObj( AttrWbsRef, d_title->getObj() );
        }
    }
    d_list->sortItems();
    d_title->getObj().getTxn()->commit();
}

void WpViewCtrl::onRemove()
{
    QList<QListWidgetItem *> sel = d_list->selectedItems();
    ENABLED_IF( !sel.isEmpty() && !d_title->getObj().isNull() );

    foreach( QListWidgetItem* i, sel )
    {
        Udb::Obj imp = d_title->getObj().getObject( i->data(Qt::UserRole).toULongLong() );
        imp.clearValue( AttrWbsRef );
        delete i;
    }
    d_title->getObj().getTxn()->commit();
}

void WpViewCtrl::onClicked(QListWidgetItem * item)
{
    if( item == 0 || d_title->getObj().isNull() )
        return;
    emit signalSelect( d_title->getObj().getObject( item->data(Qt::UserRole).toULongLong() ) );
}

