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

#include "RolePropsDlg.h"
#include <QtDebug>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QPushButton>
#include <Udb/Transaction.h>
#include "PersonListView.h"
#include "WtTypeDefs.h"
using namespace Wt;

RolePropsDlg::RolePropsDlg( QWidget* p, Udb::Transaction *txn ):QDialog(p),d_txn(txn)
{
    Q_ASSERT( txn != 0 );
	setWindowTitle( tr("Edit Role - WorkTree") );

	QVBoxLayout* vbox = new QVBoxLayout( this );

	QGridLayout* grid = new QGridLayout();
	vbox->addLayout( grid );

	d_title = new QLineEdit( this );
	grid->addWidget( new QLabel( tr( "Name:" ), this ), 0, 0 );
	grid->addWidget( d_title, 0, 1, 1, 3 );

	grid->addWidget( new QLabel( tr( "Official:" ), this ), 1, 0 );
	d_assig = new QLabel( this );
	grid->addWidget( d_assig, 1, 1 );
	d_selAssig = new QPushButton( tr("Select"), this );
	grid->addWidget( d_selAssig, 1, 2 );
	connect( d_selAssig, SIGNAL( clicked() ), this, SLOT( onSelAssig() ) );
	d_clearAssig = new QPushButton( tr("Clear"), this );
	grid->addWidget( d_clearAssig, 1, 3 );
	connect( d_clearAssig, SIGNAL( clicked() ), this, SLOT( onClearAssig() ) );

	grid->addWidget( new QLabel( tr( "Deputy:" ), this ), 2, 0 );
	d_deputy = new QLabel( this );
	grid->addWidget( d_deputy, 2, 1 );
	d_selDeputy = new QPushButton( tr("Select"), this );
	grid->addWidget( d_selDeputy, 2, 2 );
	connect( d_selDeputy, SIGNAL( clicked() ), this, SLOT( onSelDeputy() ) );
	d_clearDeputy = new QPushButton( tr("Clear"), this );
	grid->addWidget( d_clearDeputy, 2, 3 );
	connect( d_clearDeputy, SIGNAL( clicked() ), this, SLOT( onClearDeputy() ) );

	QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	vbox->addWidget( bb );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
}

void RolePropsDlg::onSelAssig()
{
	PersonSelectorDlg dlg(this, d_txn);

	quint64 res = dlg.selectOne();
	if( res != 0 )
	{
		d_pa = d_txn->getObject( res );
		d_assig->setText( d_pa.getString( AttrText ) );
	}
}

void RolePropsDlg::onClearAssig()
{
	d_pa = Udb::Obj();
	d_assig->clear();
}

void RolePropsDlg::onSelDeputy()
{
	PersonSelectorDlg dlg(this, d_txn);

	quint64 res = dlg.selectOne();
	if( res != 0 )
	{
		d_pd = d_txn->getObject( res );
		d_deputy->setText( d_pd.getString( AttrText ) );
	}
}

void RolePropsDlg::onClearDeputy()
{
	d_pd = Udb::Obj();
	d_deputy->clear();
}

void RolePropsDlg::loadFrom( Udb::Obj& o )
{
	d_title->setText( o.getString( AttrText ) );
	d_assig->clear();
	d_deputy->clear();
	d_pa = o.getValueAsObj( AttrRoleAssig );
	d_pd = o.getValueAsObj( AttrRoleDeputy );
	if( !d_pd.isNull() )
		d_deputy->setText( d_pd.getString( AttrText ) );
	if( !d_pa.isNull() )
		d_assig->setText( d_pa.getString( AttrText ) );
}

void RolePropsDlg::saveTo( Udb::Obj& o ) const
{
	o.setString( AttrText, d_title->text() );
	o.setString( AttrPrincipalName, d_title->text() ); // Redundant wegen Index. Ok.
	o.setValue( AttrRoleAssig, d_pa );
	o.setValue( AttrRoleDeputy, d_pd );
}

bool RolePropsDlg::edit( Udb::Obj& item )
{
	setWindowTitle( tr("Edit Role - WorkTree") );
	loadFrom( item );
	bool run = true;
	while( run )
	{
		if( exec() != QDialog::Accepted )
		{
			item.getTxn()->rollback();
			return false;
		}
		saveTo( item );
		item.setValue( AttrModifiedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
		run = false;
	}
	item.commit();
	return true;
}
