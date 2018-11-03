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

#include "PersonListView.h"
#include "WtTypeDefs.h"
#include <Gui2/AutoMenu.h>
#include <Oln2/OutlineTree.h>
#include <Oln2/OutlineDeleg.h>
#include <Oln2/OutlineCtrl.h>
#include <Oln2/OutlineUdbMdl.h>
#include <Udb/Extent.h>
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QListView>
#include <QStackedWidget>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QHeaderView>
#include <QtDebug>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QToolTip>
#include <QInputDialog>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
using namespace Wt;

// Von MasterPlan übernommen; 1:1, CRUD-Funktionen entfernt

class _PersListViewTabBar : public QTabBar
{
public:
	_PersListViewTabBar( QWidget* p ):QTabBar(p){}
	virtual QSize tabSizeHint ( int index ) const
	{
		return QSize( fontMetrics().height() + 2, fontMetrics().width("W") + 1 );
	}
};

class _PersListTree : public QTreeView
{
public:
	_PersListTree( QWidget* p ):QTreeView(p){}
	void mousePressEvent ( QMouseEvent * e )
	{
		QTreeView::mousePressEvent( e );
		if( e->buttons() == Qt::LeftButton && currentIndex().isValid() )
			QToolTip::showText(e->globalPos(), model()->data( currentIndex(), PersonListMdl::ToolTipRole ).toString(), this );
	}
	void mouseReleaseEvent ( QMouseEvent * e )
	{
		QToolTip::hideText();
		QTreeView::mouseReleaseEvent( e );
	}
};

PersonListView::PersonListView( QWidget* p, Flags showOption, Flags checkOption,
                                Udb::Transaction * txn ):QWidget(p)
{
	QHBoxLayout* hbox = new QHBoxLayout( this );
	hbox->setMargin( 2 );
	hbox->setSpacing( 1 );

	QTabBar* tab = new _PersListViewTabBar( this );
	tab->setShape( QTabBar::RoundedWest );
	tab->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
	tab->addTab( QChar( '*' ) );
	for( char ch = 'A'; ch <= 'Z'; ch++ )
		tab->addTab( QChar( ch ) );
	hbox->addWidget( tab );
	connect( tab, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)) );

	QVBoxLayout* vbox = new QVBoxLayout();
	vbox->setMargin(0);
	vbox->setSpacing(1);
	hbox->addLayout( vbox );

	QHBoxLayout* hbox2 = new QHBoxLayout();
	hbox2->setMargin( 0 );
	hbox2->setSpacing( 1 );
	vbox->addLayout( hbox2 );

	d_search = new QLineEdit( this );
	hbox2->addWidget( d_search );
	connect( d_search, SIGNAL(returnPressed ()), this, SLOT(onSearch()) );

	QPushButton* pb = new QPushButton( tr("&Find"), this );
	pb->setMinimumHeight( 1 );
	hbox2->addWidget( pb );
	connect( pb, SIGNAL(clicked()), this, SLOT( onSearch() ) );

	hbox2 = new QHBoxLayout();
	hbox2->setMargin( 0 );
	hbox2->setSpacing( 1 );
	vbox->addLayout( hbox2 );

	d_mdl = new PersonListMdl( this, txn );

	QCheckBox* cb;
	if( showOption == Nothing )
	{
		d_mdl->addType( TypeHuman );
	}
	if( showOption & Human )
	{
		cb = new QCheckBox( tr("People"), this );
		cb->setToolTip( tr("Show Human Resources") );
		cb->setChecked( checkOption & Human );
		if( checkOption & Human ) 
			d_mdl->addType( TypeHuman );
		connect( cb, SIGNAL( toggled(bool) ), this, SLOT( onPersonChecked(bool) ) );
		hbox2->addWidget( cb );
	}
	if( showOption & Role )
	{
		cb = new QCheckBox( tr("Roles"), this );
		cb->setToolTip( tr("Show Roles") );
		cb->setChecked( checkOption & Role );
		if( checkOption & Role )
			d_mdl->addType( TypeRole );
		connect( cb, SIGNAL( toggled(bool) ), this, SLOT( onRoleChecked(bool) ) );
		hbox2->addWidget( cb );
	}
	if( showOption & OrgUnit )
	{
		cb = new QCheckBox( tr("OrgUnits"), this );
		cb->setToolTip( tr("Show OrgUnits") );
		cb->setChecked( checkOption & OrgUnit );
		if( checkOption & OrgUnit )
			d_mdl->addType( TypeOrgUnit );
		connect( cb, SIGNAL( toggled(bool) ), this, SLOT( onUnitChecked(bool) ) );
		hbox2->addWidget( cb );
	}
	if( showOption & Other )
	{
		cb = new QCheckBox( tr("Others"), this );
		cb->setToolTip( tr("Show Other Resources") );
		cb->setChecked( checkOption & Other );
		if( checkOption & Other )
			d_mdl->addType( TypeResource );
		connect( cb, SIGNAL( toggled(bool) ), this, SLOT( onResourceChecked(bool) ) );
		hbox2->addWidget( cb );
	}
	QTreeView* v = new _PersListTree( this );
	v->setHeaderHidden( true );
	v->setDragDropOverwriteMode( false );
	v->setRootIsDecorated( false );
	v->setAlternatingRowColors(true);
	v->setIndentation( 15 );
	v->setExpandsOnDoubleClick( false );
	d_list = v;
	d_list->setModel( d_mdl );

	connect( d_list->selectionModel(), SIGNAL( currentChanged ( const QModelIndex &, const QModelIndex & ) ), 
		this, SLOT( onItemChanged() ) );
	connect( d_list, SIGNAL( clicked ( const QModelIndex & ) ), this, SLOT( onItemChanged() ) );
	vbox->addWidget( d_list );
}

void PersonListView::setMultiSelect( bool on )
{
	if( on )
		d_list->setSelectionMode( QAbstractItemView::ExtendedSelection );
	else
        d_list->setSelectionMode( QAbstractItemView::SingleSelection );
}

Udb::Transaction *PersonListView::getTxn() const
{
    return d_mdl->getTxn();
}

void PersonListView::onPersonChecked(bool on)
{
	if( on )
		d_mdl->addType( TypeHuman );
	else
		d_mdl->removeType( TypeHuman );
	d_mdl->refill();
}

void PersonListView::onRoleChecked(bool on)
{
	if( on )
		d_mdl->addType( TypeRole );
	else
		d_mdl->removeType( TypeRole );
	d_mdl->refill();
}

void PersonListView::onUnitChecked(bool on)
{
	if( on )
		d_mdl->addType( TypeOrgUnit );
	else
		d_mdl->removeType( TypeOrgUnit );
	d_mdl->refill();
}

void PersonListView::onResourceChecked(bool on)
{
	if( on )
		d_mdl->addType( TypeResource );
	else
		d_mdl->removeType( TypeResource );
	d_mdl->refill();
}

void PersonListView::onItemChanged()
{
	emit objectSelected( d_list->currentIndex().data( PersonListMdl::OidRole ).toULongLong() );
}

quint64 PersonListView::getCurrent() const
{
	return d_list->currentIndex().data( PersonListMdl::OidRole ).toULongLong();
}

QList<quint64> PersonListView::getSelected() const
{
	QList<quint64> res;
	QModelIndexList l = d_list->selectionModel()->selectedRows();
	for( int i = 0; i < l.size(); i++ )
	{
		quint64 id = l[i].data( PersonListMdl::OidRole ).toULongLong();
		if( id )
			res.append( id );
	}
	return res;
}

void PersonListView::onTabChanged(int i)
{
	QTabBar* tab = (QTabBar*) sender();
	QString str = tab->tabText( i );
	if( str == "*" )
		str = "";
	d_search->setText( str );
	onSearch();
}

void PersonListView::showAll()
{
	d_search->clear();
	onSearch();
}

void PersonListView::onSearch()
{
	QString str = d_search->text();
	if( str == "*" )
		str = "";
	d_mdl->seek( str );
	//emit currentChanged( d_idx->getFirstInList() );

}

PersonListMdl::PersonListMdl( QObject* p, Udb::Transaction * txn ):QAbstractItemModel(p),d_txn(txn)
{
    Q_ASSERT( txn != 0 );
}

QModelIndex PersonListMdl::parent ( const QModelIndex & index ) const
{
	if( index.isValid() )
	{
		Slot* s = static_cast<Slot*>( index.internalPointer() );
		Q_ASSERT( s != 0 );
		if( s->d_parent == &d_root )
			return QModelIndex();
		// else
		Q_ASSERT( s->d_parent != 0 );
		Q_ASSERT( s->d_parent->d_parent != 0 );
		return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
	}else
		return QModelIndex();
}

int PersonListMdl::rowCount ( const QModelIndex & parent ) const
{
	if( parent.isValid() )
	{
		Slot* s = static_cast<Slot*>( parent.internalPointer() );
		Q_ASSERT( s != 0 );
		return s->d_children.size();
	}else
		return d_root.d_children.size();
}

QModelIndex PersonListMdl::index ( int row, int column, const QModelIndex & parent ) const
{
    const Slot* s = &d_root;
	if( parent.isValid() )
	{
		s = static_cast<Slot*>( parent.internalPointer() );
		Q_ASSERT( s != 0 );
    }
    if( row < s->d_children.size() )
        return createIndex( row, column, s->d_children[row] );
	else
        return QModelIndex();
}

QVariant PersonListMdl::data ( const QModelIndex & index, int role ) const
{
	if( !index.isValid() )
		return QVariant();
	Slot* s = static_cast<Slot*>( index.internalPointer() );
	Q_ASSERT( s != 0 );
	switch( role )
	{
	case Qt::DisplayRole:
		return s->d_obj.getString( AttrText );
	case Qt::DecorationRole:
		return Oln::OutlineUdbMdl::getPixmap( s->d_obj.getType() );
	case ToolTipRole:
		{
            // TODO
//			ContentObj o = getObject( index );
//			return o.printToolTip();
		}
	case OidRole:
		return getObject( index ).getOid();
	}
	return QVariant();
}

Udb::Obj PersonListMdl::getObject( const QModelIndex& i ) const
{
	if( i.isValid() )
	{
		Slot* s = static_cast<Slot*>( i.internalPointer() );
		Q_ASSERT( s != 0 );
		return s->d_obj;
	}else
        return Udb::Obj();
}

void PersonListMdl::refill()
{
	// TODO: incremental fetch
    d_root.clear();
	Udb::Idx idx( d_txn, IndexDefs::IdxPrincipalFirstName );
	if( idx.seek( Stream::DataCell().setString( d_pattern, false ) ) ) do
	{
		Udb::Obj o = d_txn->getObject( idx.getOid() );
		if( d_types.contains( o.getType() ) )
		{
			Slot* s = new Slot( &d_root );
			s->d_obj = o;
		}
	}while( idx.nextKey() );
	reset();
}

void PersonListMdl::seek( const QString& str )
{
	d_pattern = str;
	refill();
}

void PersonListMdl::addType( quint32 t )
{
	d_types.append( t );
}

void PersonListMdl::removeType( quint32 t )
{
	d_types.removeAll( t );
}

PersonSelectorDlg::PersonSelectorDlg( QWidget* p, Udb::Transaction* txn, PersonListView::Flags showOption,
                           PersonListView::Flags checkOption ):QDialog(p)
{
	setWindowTitle( tr( "Select Persons - WorkTree" ) );
	QVBoxLayout* vbox = new QVBoxLayout( this );

	d_list = new PersonListView( this, showOption, checkOption, txn );
	vbox->addWidget( d_list );

	QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	vbox->addWidget( bb );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

	d_ok = bb->button( QDialogButtonBox::Ok );
	d_ok->setEnabled( false );

	connect( d_list->getList()->selectionModel(), 
		SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
		this, SLOT( onSelect() ) );
	connect( d_list->getList(), SIGNAL( doubleClicked ( const QModelIndex & ) ), this, SLOT( accept() ) );
}

void PersonSelectorDlg::onSelect()
{
	d_ok->setEnabled( d_list->getList()->selectionModel()->hasSelection() );
}

QList<quint64> PersonSelectorDlg::select(bool current)
{
	d_list->setMultiSelect( true );
	d_list->getSearch()->setFocus();
	if( exec() == QDialog::Accepted )
	{
		if( current )
			return QList<quint64>() << d_list->getCurrent();
		else
			return d_list->getSelected();
	}else
		return QList<quint64>();
}

quint64 PersonSelectorDlg::selectOne()
{
	d_list->setMultiSelect( false );
	d_list->getSearch()->setFocus();
	if( exec() == QDialog::Accepted )
	{
		return d_list->getCurrent();
	}else
		return 0;
}

void PersonListView::onRebuildIndex()
{
	ENABLED_IF( true );

	Udb::Idx idx( d_mdl->getTxn(), IndexDefs::IdxPrincipalFirstName );
	idx.rebuildIndex();
}

void PersonListView::onUpdateRoles()
{
	ENABLED_IF( true );
	Udb::Extent e( d_mdl->getTxn() );
	if( e.first() ) do
	{
		Udb::Obj o = e.getObj();
		if( o.getType() == TypeRole )
		{
			o.setValue( AttrPrincipalName, o.getValue( AttrText ) );
		}
	}while( e.next() );
	d_mdl->getTxn()->commit();
}

