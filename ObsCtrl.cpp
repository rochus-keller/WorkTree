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

#include "ObsCtrl.h"
#include "ObsMdl.h"
#include <QtGui/QTreeView>
#include <QtGui/QInputDialog>
#include "WtTypeDefs.h"
#include "ObjectHelper.h"
#include "PersonListView.h"
#include "HumanPropsDlg.h"
#include "RolePropsDlg.h"
#include "Funcs.h"
using namespace Wt;

ObsCtrl::ObsCtrl(QTreeView * tree, GenericMdl *mdl) :
    GenericCtrl(tree, mdl)
{
}

ObsCtrl *ObsCtrl::create(QWidget *parent, const Udb::Obj &root)
{
    QTreeView* tree = new QTreeView( parent );
    tree->setHeaderHidden( true );
	tree->setSelectionMode( QAbstractItemView::ExtendedSelection );
	tree->setEditTriggers( QAbstractItemView::NoEditTriggers );
	tree->setDragDropMode( QAbstractItemView::DragDrop );
	tree->setDragEnabled( true );
	tree->setDragDropOverwriteMode( false );
	tree->setAlternatingRowColors(true);
	tree->setIndentation( 15 );
	tree->setExpandsOnDoubleClick( false );

    ObsMdl* mdl = new ObsMdl( tree );
    tree->setModel( mdl );

    ObsCtrl* ctrl = new ObsCtrl(tree, mdl);
    ctrl->setRoot( root );

    connect( mdl, SIGNAL(signalNameEdited(QModelIndex,QVariant)),
             ctrl, SLOT(onNameEdited(QModelIndex,QVariant) ) );
    connect( mdl, SIGNAL(signalMoveTo(QList<Udb::Obj>,QModelIndex,int)),
             ctrl, SLOT(onMoveTo(QList<Udb::Obj>,QModelIndex,int)) );
    connect( tree->selectionModel(), SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
        ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(clicked(QModelIndex)), ctrl, SLOT( onSelectionChanged() ) );
    connect( tree, SIGNAL(doubleClicked(QModelIndex)), ctrl, SLOT(onDblClick(QModelIndex)) );

    return ctrl;
}

void ObsCtrl::addCommands( Gui2::AutoMenu * pop )
{
	pop->addAction( tr("Expand All"), getTree(), SLOT( expandAll() ) );
	pop->addAction( tr("Expand Selected"), this, SLOT( onExpandSelected() ) );
	pop->addAction( tr("Collapse All"), getTree(), SLOT( collapseAll() ) );
    pop->addCommand( tr("Find..."), this, SLOT(onFind()), tr("CTRL+F"), true );
    pop->addSeparator();
    pop->addCommand( tr("Move up"), this, SLOT(onMoveUp()), tr("SHIFT+Up"), true );
    pop->addCommand( tr("Move down"), this, SLOT(onMoveDown()), tr("SHIFT+Down"), true );
	pop->addSeparator();
    pop->addCommand( tr("New Item (same type)"), this, SLOT(onAddNext()), tr("CTRL+N"), true );
    pop->addCommand( tr("New Person"), this, SLOT(onAddHuman()), tr("CTRL+H"), true );
    pop->addCommand( tr("New OrgUnit"), this, SLOT(onAddOrgUnit()), tr("CTRL+U"), true );
    pop->addCommand( tr("New Role"), this, SLOT(onAddTypeRole()), tr("CTRL+R"), true );
    pop->addCommand( tr("New Resource"), this, SLOT(onAddTypeResource()) );
	pop->addSeparator();
    pop->addCommand( tr("Copy"), this, SLOT( onCopy() ), tr("CTRL+C"), true );
    pop->addCommand( tr("Edit..."), this, SLOT( onEditSelected() ), tr("CTRL+E"), true );
    pop->addCommand( tr("Delete Items"), this, SLOT( onDeleteItems() ), tr("CTRL+D"), true );
}

bool ObsCtrl::edit(Udb::Obj & o)
{
    switch( o.getType() )
    {
    case TypeHuman:
        {
            HumanPropsDlg dlg( getTree() );
            if( dlg.edit( o ) )
                o.commit();
            return true;
        }
        break;
    case TypeRole:
        {
            RolePropsDlg dlg( getTree(), getMdl()->getRoot().getTxn() );
            if( dlg.edit( o ) )
                o.commit();
            return true;
        }
        break;
    case TypeOrgUnit:
    case TypeResource:
        {
            bool ok;
            QString res = QInputDialog::getText( getTree(), tr("Edit Object"), tr("Principal Name:"), QLineEdit::Normal,
				o.getString( AttrText ), &ok );
			if( ok )
			{
				o.setString( AttrText, res );
                o.setString( AttrPrincipalName, res );
				o.setTimeStamp( AttrModifiedOn );
				o.commit();
			}

        }
        break;
    }
    return false;
}

void ObsCtrl::onAddOrgUnit()
{
    add( TypeOrgUnit );
}

void ObsCtrl::onAddTypeRole()
{
    add( TypeRole );
}

void ObsCtrl::onAddHuman()
{
    add( TypeHuman );
}

void ObsCtrl::onAddTypeResource()
{
    add( TypeResource );
}

void ObsCtrl::add( quint32 type )
{
    Udb::Obj parentObj = getSelectedObject();
    if( parentObj.isNull() )
        parentObj = getMdl()->getRoot();
	ENABLED_IF( Funcs::isValidAggregate( parentObj.getType(), type ) );

	Udb::Obj f = ObjectHelper::createObject( type, parentObj );
    f.commit();
    focusOn( f, false );
    edit( f );
    emit signalSelected( f );
}

void ObsCtrl::onAddNext()
{
    Udb::Obj doc = getSelectedObject();
	ENABLED_IF( !doc.isNull() );

    Udb::Obj o = ObjectHelper::createObject( doc.getType(), doc.getParent(), doc.getNext() );
    o.commit();
    focusOn( o, false );
    edit( o );
}

void ObsCtrl::onFind()
{
    ENABLED_IF(true);

    PersonListView::Flags f = PersonListView::Human | PersonListView::OrgUnit |
            PersonListView::Role | PersonListView::Other;
    PersonSelectorDlg dlg( getTree(), getMdl()->getRoot().getTxn(), f, f );
    dlg.select();
}

void ObsCtrl::onEditSelected()
{
    Udb::Obj doc = getSelectedObject();
	ENABLED_IF( !doc.isNull() );

    edit( doc );
}

void ObsCtrl::onDblClick(const QModelIndex &i)
{
    Udb::Obj o = getMdl()->getObject( i );
    edit( o );
}
