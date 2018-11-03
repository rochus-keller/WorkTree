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

#include "SearchView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QShortcut>
#include <QLabel>
#include <QSettings>
#include <QResizeEvent>
#include <QHeaderView>
#include <Gui2/UiFunction.h>
#include <Oln2/OutlineUdbMdl.h>
#include <Udb/Transaction.h>
#include "Indexer.h"
#include "Funcs.h"
//#include "Funcs.h"
using namespace Wt;

static const int s_objectCol = 0;
static const int s_contextCol = 1;
static const int s_scoreCol = 2;

struct _SearchViewItem : public QTreeWidgetItem
{
	_SearchViewItem( QTreeWidget* w ):QTreeWidgetItem( w ) {}

	bool operator<(const QTreeWidgetItem &other) const
	{
		const int col = treeWidget()->sortColumn();
		switch( col )
		{
		case s_scoreCol:
			return text(s_scoreCol) < other.text(s_scoreCol);
		case s_contextCol:
			return text(s_contextCol) + text(s_scoreCol) < other.text(s_contextCol) + other.text(s_scoreCol );
		case s_objectCol:
			return text(s_objectCol) < other.text(s_objectCol);
		}
		return text(col) < other.text(col);
	}
	QVariant data(int column, int role) const
	{
		if( role == Qt::ToolTipRole )
			role = Qt::DisplayRole;
		return QTreeWidgetItem::data( column, role );
	}
};

SearchView::SearchView(QWidget* p, Udb::Transaction* txn):QWidget( p )
{
	setWindowTitle( tr("WorkTree Search") );

	d_idx = new Indexer( txn, this );

	QVBoxLayout* vbox = new QVBoxLayout( this );
	vbox->setMargin( 2 );
	vbox->setSpacing( 1 );
	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setMargin( 1 );
	hbox->setSpacing( 1 );
	vbox->addLayout( hbox );

	hbox->addWidget( new QLabel( tr("Enter query:"), this ) );

	d_query = new QLineEdit( this );
	connect( d_query, SIGNAL( returnPressed() ), this, SLOT( onSearch() ) );
	hbox->addWidget( d_query );

	QPushButton* doit = new QPushButton( tr("&Search"), this );
	doit->setDefault(true);
	connect( doit, SIGNAL( clicked() ), this, SLOT( onSearch() ) );
	hbox->addWidget( doit );

	d_result = new QTreeWidget( this );
	d_result->header()->setStretchLastSection( false );
	d_result->setAllColumnsShowFocus( true );
	d_result->setRootIsDecorated( false );
	d_result->setHeaderLabels( QStringList() << tr("Match") << tr("Context") << tr("Score") ); // s_item, s_doc, s_score
	d_result->setSortingEnabled(true);
	QPalette pal = d_result->palette();
	pal.setColor( QPalette::AlternateBase, QColor::fromRgb( 245, 245, 245 ) );
	d_result->setPalette( pal );
	d_result->setAlternatingRowColors( true );
    connect( d_result, SIGNAL( itemClicked ( QTreeWidgetItem *, int ) ), this, SLOT( onGoto() ) );
    connect( d_result, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ), this, SLOT( onOpen() ) );
    vbox->addWidget( d_result );
}

SearchView::~SearchView()
{
}

void SearchView::onSearch()
{
	Indexer::ResultList res;
	if( !d_idx->exists() )
	{
		if( QMessageBox::question( this, tr("WorkTree Search"),
			tr("The index does not yet exist. Do you want to build it? This will take some minutes." ),
			QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
			return;
		if( !d_idx->indexRepository( this ) )
		{
			if( !d_idx->getError().isEmpty() )
				QMessageBox::critical( this, tr("WorkTree Indexer"), d_idx->getError() );
			return;
		}
	}else if( d_idx->hasPendingUpdates() )
	{
		// Immer aktualisieren ohne zu fragen
		if( true ) /*QMessageBox::question( this, tr("WorkTree Search"),
			tr("The index is not up-do-date. Do you want to update it?" ),
			QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok )*/
		{
			if( !d_idx->indexIncrements( this ) )
			{
				if( !d_idx->getError().isEmpty() )
					QMessageBox::critical( this, tr("WorkTree Indexer"), d_idx->getError() );
				return;
			}
		}
	}
	QApplication::setOverrideCursor( Qt::WaitCursor );
	if( !d_idx->query( d_query->text(), res ) )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, tr("WorkTree Search"), d_idx->getError() );
		return;
	}
	d_result->clear();
	for( int i = 0; i < res.size(); i++ )
	{
		QTreeWidgetItem* item = new _SearchViewItem( d_result );
        const QString text = Funcs::getText( res[i].d_object, true );
		item->setText( s_objectCol, text );
		item->setData( s_objectCol, Qt::UserRole, res[i].d_object.getOid() );
		Udb::Obj doc = res[i].d_context;
		if( doc.isNull() )
			doc = res[i].d_object.getParent();
		if( doc.isNull() )
		{
			item->setText( s_contextCol, text );
			item->setIcon( s_contextCol, Oln::OutlineUdbMdl::getPixmap( res[i].d_object.getType() ) );
		}else
		{
			item->setIcon( s_objectCol, Oln::OutlineUdbMdl::getPixmap( res[i].d_object.getType() ) );
            item->setText( s_contextCol, Indexer::fetchText( doc, Indexer::attrText ) );
			item->setIcon( s_contextCol, Oln::OutlineUdbMdl::getPixmap( doc.getType() ) );
			item->setData( s_contextCol, Qt::UserRole, doc.getOid() );
		}
		item->setText( s_scoreCol, QString::number( res[i].d_score, 'f', 1 ) );
		QFont f = item->font( s_objectCol );
		if( res[i].d_isTitle )
		{
			f.setBold(true);
			item->setFont( s_objectCol, f );
		}
		if( res[i].d_isAlias )
		{
			f.setItalic(true);
			item->setFont( s_objectCol, f );
		}
	}
    d_result->header()->setResizeMode( s_objectCol, QHeaderView::Stretch );
	d_result->header()->setResizeMode( s_contextCol, QHeaderView::Interactive );
    d_result->header()->setResizeMode( s_scoreCol, QHeaderView::ResizeToContents );
	d_result->resizeColumnToContents( s_scoreCol );
	d_result->sortByColumn( s_scoreCol, Qt::DescendingOrder );
	QApplication::restoreOverrideCursor();
}

void SearchView::onNew()
{
	d_query->selectAll();
	d_query->setFocus();
}

void SearchView::onGoto()
{
    QTreeWidgetItem* cur = d_result->currentItem();
    ENABLED_IF( cur );
    emit signalShowItem( d_idx->getTxn()->getObject( cur->data( s_objectCol,Qt::UserRole).toULongLong() ) );
}

void SearchView::onOpen()
{
    QTreeWidgetItem* cur = d_result->currentItem();
    ENABLED_IF( cur );
    emit signalOpenItem( d_idx->getTxn()->getObject( cur->data( s_objectCol,Qt::UserRole).toULongLong() ) );
}

void SearchView::onCopyRef()
{
	ENABLED_IF( d_result->currentItem() );

    QMimeData* mimeData = new QMimeData();
    QList<Udb::Obj> objs;
    objs.append( d_idx->getTxn()->
                 getObject( d_result->currentItem()->data( s_objectCol,Qt::UserRole).toULongLong() ) );
    Udb::Obj::writeObjectRefs( mimeData, objs );
}

Udb::Obj SearchView::getItem() const
{
	QTreeWidgetItem* cur = d_result->currentItem();
	if( cur == 0 )
        return Udb::Obj();
	else
		return d_idx->getTxn()->getObject( cur->data( s_objectCol,Qt::UserRole).toULongLong() );
}

void SearchView::onRebuildIndex()
{
	ENABLED_IF( true );

	if( !d_idx->indexRepository( this ) )
	{
		if( !d_idx->getError().isEmpty() )
			QMessageBox::critical( this, tr("WorkTree Indexer"), d_idx->getError() );
		return;
	}
}

void SearchView::onUpdateIndex()
{
	ENABLED_IF( d_idx->exists() && d_idx->hasPendingUpdates() );

	if( !d_idx->indexIncrements( this ) )
	{
		if( !d_idx->getError().isEmpty() )
			QMessageBox::critical( this, tr("WorkTree Indexer"), d_idx->getError() );
	}
}

void SearchView::onClearSearch()
{
	ENABLED_IF( d_result->topLevelItemCount() > 0 );

	d_result->clear();
	d_query->clear();
	d_query->setFocus();
}
