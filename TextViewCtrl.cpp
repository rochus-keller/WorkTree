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

#include "TextViewCtrl.h"
#include "ObjectTitleFrame.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QDesktopServices>
#include <Oln2/OutlineUdbCtrl.h>
#include <Oln2/OutlineItem.h>
#include <Gui2/AutoShortcut.h>
#include <Udb/Database.h>
#include <QtGui/QToolButton>
#include <QtGui/QCheckBox>
#include "Funcs.h"
using namespace Wt;

TextViewCtrl::TextViewCtrl(QWidget *parent) :
    QObject(parent)
{
}

QWidget *TextViewCtrl::getWidget() const
{
    return static_cast<QWidget*>( parent() );
}

const char* TextViewCtrl::s_pin = ":/images/pin.png";

TextViewCtrl *TextViewCtrl::create(QWidget *parent, Udb::Transaction *txn)
{
    Q_ASSERT( txn != 0 );
    QWidget* pane = new QWidget( parent );
    QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

    TextViewCtrl* ctrl = new TextViewCtrl( pane );

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->setSpacing( 2 );
    hbox->setMargin( 0 );
    vbox->addLayout( hbox );

    ctrl->d_title = new ObjectTitleFrame( pane );
    hbox->addWidget( ctrl->d_title );

    ctrl->d_pin = new CheckLabel( pane );
    ctrl->d_pin->setPixmap( QPixmap(s_pin) );
    ctrl->d_pin->setToolTip( tr("Pin to current object") );
    hbox->addWidget( ctrl->d_pin );

	ctrl->d_oln = Oln::OutlineUdbCtrl::create( pane, txn );
	ctrl->d_oln->getDeleg()->setBiggerTitle( false );
	ctrl->d_oln->getDeleg()->setShowIcons( true );
	ctrl->d_oln->getTree()->setHandleWidth( 7 );
	ctrl->d_oln->getTree()->setIndentation( 20 );
	vbox->addWidget( ctrl->d_oln->getTree() );

    txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

    connect( ctrl->d_title, SIGNAL(signalClicked()), ctrl, SLOT(onTitleClick()) );
    connect( ctrl->d_oln->getTree(), SIGNAL( identDoubleClicked() ), ctrl, SLOT( onFollowAlias() ) );
    connect( ctrl->d_oln->getDeleg()->getEditCtrl(), SIGNAL( anchorActivated( QByteArray, bool ) ),
		ctrl, SLOT( onAnchorActivated(QByteArray, bool) ) );
	connect( ctrl->d_oln, SIGNAL(sigCurrentChanged(quint64)), ctrl, SLOT( onCurrentChanged( quint64 ) ) );

    return ctrl;
}

Gui2::AutoMenu *TextViewCtrl::createPopup()
{
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( d_oln->getTree(), true );

//	pop->addCommand( tr("Goto Object"),  this, SLOT(onGotoAlias()), tr("CTRL+G") );
//	new AutoShortcut( tr("CTRL+G"), d_oln->getTree(), this, SLOT(onGotoAlias()) );
//	pop->addCommand( tr("Open in Browser..."),  d_dv, SLOT(onOpenUrl()), tr("CTRL+O") );
//	new AutoShortcut( tr("CTRL+O"), d_oln->getTree(), d_dv, SLOT(onOpenUrl()) );
//    pop->addSeparator();

//	pop->addCommand( tr("Create Role..."),  this, SLOT(onCreateRole()) );
//	pop->addCommand( tr("Create External..."),  this, SLOT(onCreateExternal()) );
//	pop->addCommand( tr("Create Action Item..."),  this, SLOT(onCreateActionItem()) );
//    pop->addCommand( tr("Create Outline..."),  this, SLOT(onCreateOutline2()) );
//	pop->addCommand( tr("Create Process..."),  this, SLOT(onCreateProcess()) );
//	pop->addCommand( tr("Import Process..."),  this, SLOT(onImportProcess()) );
	Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Item" ), pop );
	pop->addMenu( sub );
    d_oln->addItemCommands( sub );

	sub = new Gui2::AutoMenu( tr("Text" ), pop );
	pop->addMenu( sub );
    d_oln->addTextCommands( sub );

	pop->addSeparator();
    d_oln->addOutlineCommands( pop );
//	pop->addSeparator();
//	pop->addCommand( tr("Export to HTML..."), d_dv, SLOT(onExportHtml()) );
//    pop->addCommand( tr("Outline locked"), d_dv->getCtrl(), SLOT(onDocReadOnly()) )->setCheckable(true);
//    pop->addCommand( tr("Set Properties..."), this, SLOT(onSetOutlineProps()),tr("ALT+Return") );
//	// new AutoShortcut( tr("ALT+Return"), d_oln->getTree(), this, SLOT(onSetOutlineProps()) );
//    pop->addAutoCommand( tr("Dock Outline"), SLOT(onAddDock()) );
//	pop->addAutoCommand( tr("Undock Outline"),  SLOT(onRemoveDock()) );
//    pop->addSeparator();

    return pop;
}

void TextViewCtrl::onDbUpdate(Udb::UpdateInfo info)
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

void TextViewCtrl::onFollowAlias()
{
    QModelIndex i = d_oln->getTree()->currentIndex();
    Udb::Obj o = d_oln->getMdl()->getItem( i );
    if( o.isNull() )
        return;
    o = Funcs::getAlias( o );
    if( !o.isNull() )
        emit signalFollowObject( o );
}

void TextViewCtrl::onTitleClick()
{
    if( !d_title->getObj().isNull() )
        emit signalFollowObject( d_title->getObj() );
}

void TextViewCtrl::setObj(const Udb::Obj & o)
{
    	if( d_pin->isChecked() )
        	return;
	Udb::Obj obj = o;
	if( obj.getType() == Oln::OutlineItem::TID )
		obj = obj.getValueAsObj( Oln::OutlineItem::AttrHome );
	if( obj.getType() == Oln::Outline::TID )
		obj = Udb::Obj();

	if( !d_oln->getOutline().equals(obj) )
	{
        d_oln->setOutline( obj );
        d_title->setObj( obj );
	}
	if( !obj.isNull() && !obj.equals( o ) )
		d_oln->gotoItem( o.getOid() );
}

void TextViewCtrl::onAnchorActivated(QByteArray data, bool url )
{
    if( url )
        QDesktopServices::openUrl( QUrl::fromEncoded( data ) );
    else
    {
        Oln::Link link;
        if( link.readFrom( data ) && d_title->getObj().getDb()->getDbUuid() == link.d_db )
            emit signalFollowObject( d_title->getObj().getObject( link.d_oid ) );
	}
}

void TextViewCtrl::onCurrentChanged(quint64 oid)
{
	emit signalItemActivated( d_title->getObj().getObject( oid ) ); // kann auch Null sein
}

void CheckLabel::setChecked(bool checked)
{
    if( checked )
    {
        setFrameShape( QFrame::Panel );
        setFrameShadow( QFrame::Sunken );
        d_checked = true;
    }else
    {
        setFrameShape( QFrame::NoFrame );
        setFrameShadow( QFrame::Plain );
        d_checked = false;
    }
}

void CheckLabel::mousePressEvent(QMouseEvent *event)
{
    setChecked( !d_checked );
    QLabel::mousePressEvent( event );
}
