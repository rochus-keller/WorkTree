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

#include "RefByViewCtrl.h"
#include "TextViewCtrl.h"
#include "ObjectTitleFrame.h"
#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <QVBoxLayout>
#include <QTreeView>
using namespace Wt;

RefByViewCtrl::RefByViewCtrl(QWidget *parent) :
	QObject(parent)
{
}

Wt::RefByViewCtrl *Wt::RefByViewCtrl::create(QWidget *parent, Udb::Transaction *txn)
{
	Q_ASSERT( txn != 0 );

	QWidget* pane = new QWidget( parent );
	QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

	RefByViewCtrl* ctrl = new RefByViewCtrl( pane );
	txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setSpacing( 2 );
	hbox->setMargin( 0 );
	vbox->addLayout( hbox );

	ctrl->d_title = new ObjectTitleFrame( pane );
	hbox->addWidget( ctrl->d_title );

	ctrl->d_pin = new CheckLabel( pane );
	ctrl->d_pin->setPixmap( QPixmap(TextViewCtrl::s_pin) );
	ctrl->d_pin->setToolTip( tr("Pin to current object") );
	hbox->addWidget( ctrl->d_pin );

	QTreeView* tree = new QTreeView( pane );
	vbox->addWidget( tree );
	tree->setAlternatingRowColors( true );
	tree->setAllColumnsShowFocus(true);
	tree->setHeaderHidden(true);

	ctrl->d_mdl = new Oln::RefByItemMdl( tree );
	tree->setModel( ctrl->d_mdl );

	connect( ctrl->d_title, SIGNAL(signalClicked()), ctrl, SLOT(onTitleClick()) );
	connect( ctrl->d_mdl,SIGNAL(sigFollowObject(Udb::Obj)), ctrl,SLOT(onFollowObject(Udb::Obj)) );

	return ctrl;
}

QWidget *RefByViewCtrl::getWidget() const
{
	return static_cast<QWidget*>( parent() );
}

void RefByViewCtrl::setObj(const Udb::Obj & o)
{
	if( d_pin->isChecked() )
		return;
	if( !d_mdl->getObj().equals(o) )
	{
		d_mdl->setObj( o );
		d_title->setObj( o );
	}
}

void RefByViewCtrl::addCommands(Gui2::AutoMenu * pop)
{
	pop->addCommand( tr("Show Item"), this, SLOT(onShowItem()));
}

void RefByViewCtrl::onTitleClick()
{
	if( !d_title->getObj().isNull() )
		emit signalFollowObject( d_title->getObj() );
}

void RefByViewCtrl::onFollowObject(const Udb::Obj & o)
{
	if( !o.isNull() )
		emit signalFollowObject( o );
}

void RefByViewCtrl::onShowItem()
{
	ENABLED_IF( d_mdl->getTree()->currentIndex().isValid() );
	onFollowObject( d_mdl->getObject( d_mdl->getTree()->currentIndex() ) );
}

void RefByViewCtrl::onDbUpdate(Udb::UpdateInfo info)
{
	switch( info.d_kind )
	{
	case Udb::UpdateInfo::ObjectErased:
		if( info.d_id == d_title->getObj().getOid() )
		{
			d_pin->setChecked( false );
			setObj( Udb::Obj() );
		}
		break;
	}
}
