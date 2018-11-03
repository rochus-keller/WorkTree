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

#include "AttrViewCtrl.h"
#include "ObjectTitleFrame.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QDesktopServices>
#include <Oln2/OutlineUdbCtrl.h>
#include <Udb/Database.h>
#include "Funcs.h"
using namespace Wt;

void ObjectDetailPropsMdl::setObject(const Udb::Obj& o)
{
    if( d_obj.equals( o ) )
        return;
    clear();
    d_obj = o;
    Slot* root = new Slot();
    add( root, 0 );
    if( d_obj.isNull() )
        return;
    Udb::Obj::Names n = o.getNames();
    QMap<QString,quint32> dir;
    for( int i = 0; i < n.size(); i++ )
    {
		QString txt = Funcs::getName( n[i], false, o.getTxn() );
        if( !txt.isEmpty() )
            dir[ txt ] = n[i];
    }
    QMap<QString,quint32>::const_iterator j;
    for( j = dir.begin(); j != dir.end(); ++j )
    {
        ObjSlot* s = new ObjSlot();
        s->d_name = j.value();
        add( s, root );
    }
    reset();
}

AttrViewCtrl::AttrViewCtrl(QWidget *parent) :
    QObject(parent)
{
}

Wt::AttrViewCtrl *Wt::AttrViewCtrl::create(QWidget *parent, Udb::Transaction *txn)
{
    Q_ASSERT( txn != 0 );
    QWidget* pane = new QWidget( parent );
    QVBoxLayout* vbox = new QVBoxLayout( pane );
	vbox->setMargin( 2 );
	vbox->setSpacing( 2 );

    AttrViewCtrl* ctrl = new AttrViewCtrl( pane );

    ctrl->d_title = new ObjectTitleFrame( pane );
    vbox->addWidget( ctrl->d_title );


    ctrl->d_props = new ObjectDetailPropsMdl( pane );

	ctrl->d_tree = new Oln::OutlineTree( pane );
	ctrl->d_tree->setAlternatingRowColors( true );
	ctrl->d_tree->setIndentation( 10 ); // RISK
	ctrl->d_tree->setHandleWidth( 7 );
	ctrl->d_tree->setShowNumbers( false );
	ctrl->d_tree->setModel( ctrl->d_props );
	Oln::OutlineCtrl* ctrl2 = new Oln::OutlineCtrl( ctrl->d_tree );
    Oln::OutlineDeleg* deleg = ctrl2->getDeleg();
    deleg->setReadOnly(true);
    connect( deleg->getEditCtrl(), SIGNAL(anchorActivated(QByteArray,bool) ),
		ctrl, SLOT( onAnchorActivated(QByteArray, bool) ) );
	ctrl->d_tree->setAcceptDrops( false );
    vbox->addWidget( ctrl->d_tree );

    ctrl->d_title->adjustFont();
	pane->updateGeometry();

    txn->getDb()->addObserver( ctrl, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );

    return ctrl;
}

void AttrViewCtrl::setObj(const Udb::Obj & o)
{
    d_props->setObject( o );
	d_title->setObj( o );
}

QWidget *AttrViewCtrl::getWidget() const
{
    return static_cast<QWidget*>( parent() );
}

void AttrViewCtrl::onAnchorActivated(QByteArray data, bool url)
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

void AttrViewCtrl::onDbUpdate(Udb::UpdateInfo info)
{
    switch( info.d_kind )
	{
    case Udb::UpdateInfo::ObjectErased:
        if( info.d_id == d_title->getObj().getOid() )
            setObj( Udb::Obj() );
        break;
    }
}

QVariant ObjectDetailPropsMdl::ObjSlot::getData(const Oln::OutlineMdl * m, int role) const
{
    const ObjectDetailPropsMdl* mdl = static_cast<const ObjectDetailPropsMdl*>(m);
	if( role == Qt::DisplayRole )
	{
		QString html = "<html>";
		html += "<b>" + Funcs::getName( d_name, false, mdl->getObj().getTxn() ) + ":</b> ";
        html += Funcs::formatValue( d_name, mdl->getObj(), true );
		html += "</html>";
		return QVariant::fromValue( OutlineMdl::Html( html ) );
	}else
		return QVariant();
}
