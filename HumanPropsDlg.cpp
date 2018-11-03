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

#include "HumanPropsDlg.h"
#include <Gui2/AutoMenu.h>
#include <Udb/Transaction.h>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QInputDialog>
#include <QTextEdit>
#include "WtTypeDefs.h"
using namespace Wt;

static inline QLabel* _label( QWidget* p, quint32 t )
{
	return new QLabel( WtTypeDefs::prettyName( t ) + QChar(':'), p );
}

HumanPropsDlg::HumanPropsDlg( QWidget* p ):QDialog(p)
{
	setWindowTitle( tr("Edit Person - WorkTree") );

	QGridLayout* topGrid = new QGridLayout( this );
	int row = 0;

	topGrid->addWidget( new QLabel( tr("Displayed Name:"), this ), row, 0 );
	d_text = new QLineEdit( this );
	topGrid->addWidget( d_text, row, 1, 1, 2 );

	d_shortHand = new QLineEdit( this );
	topGrid->addWidget( d_shortHand, row, 3 );
	
	row++;

	topGrid->addWidget( _label( this, AttrPrincipalName ), row, 0 );
	d_lastName = new QLineEdit( this );
	topGrid->addWidget( d_lastName, row, 1 );
	
	topGrid->addWidget( _label( this, AttrFirstName ), row, 2 );
	d_firstName = new QLineEdit( this );
	topGrid->addWidget( d_firstName, row, 3 );
	
	row++;

	topGrid->addWidget( _label( this, AttrQualiTitle ), row, 0 );
	d_acaTitle = new QLineEdit( this );
	topGrid->addWidget( d_acaTitle, row, 1 );
	
	topGrid->addWidget( _label( this, AttrJobTitle ), row, 2 );
	d_jobTitle = new QLineEdit( this );
	topGrid->addWidget( d_jobTitle, row, 3 );
	
	row++;

	QTabWidget* tw = new QTabWidget( this );
	tw->setTabPosition( QTabWidget::North );
	tw->setFocusPolicy( Qt::NoFocus );
	topGrid->addWidget( tw, row, 0, 1, 4 );

	// Business Pane
	{
		int row = 0;
		QWidget* tab = new QWidget( tw );
		tw->addTab( tab, tr("&Business") );
		QGridLayout* grid = new QGridLayout( tab );

		grid->addWidget( _label( this, AttrOrganization ), row, 0 );
		d_orgName = new QLineEdit( this );
		grid->addWidget( d_orgName, row, 1 );
		
		grid->addWidget( _label( this, AttrDepartment ), row, 2 );
		d_department = new QLineEdit( this );
		grid->addWidget( d_department, row, 3 );
		
		row++;

		grid->addWidget( _label( this, AttrOffice ), row, 0 );
		d_office = new QLineEdit( this );
		grid->addWidget( d_office, row, 1 );

		grid->addWidget( _label( this, AttrEmail ), row, 2 );
		d_eMail = new QLineEdit( this );
		grid->addWidget( d_eMail, row, 3 );

		row++;

		grid->addWidget( _label( this, AttrPhoneDesk ), row, 0 );
		d_phoneDesk = new QLineEdit( this );
		grid->addWidget( d_phoneDesk, row, 1 );
		
		grid->addWidget( _label( this, AttrPhoneMobile ), row, 2 );
		d_phoneMobile = new QLineEdit( this );
		grid->addWidget( d_phoneMobile, row, 3 );
		
		row++;

		grid->addWidget( _label( this, AttrMailAddr ), row, 0 );
		d_addr = new QTextEdit( this );
		d_addr->setAcceptRichText( false );
		d_addr->setTabChangesFocus( true );
		grid->addWidget( d_addr, row, 1, 1, 3 );

		row++;

		grid->addWidget( _label( this, AttrManager ), row, 0 );
		d_manager = new QLineEdit( this );
		grid->addWidget( d_manager, row, 1 );
		
		grid->addWidget( _label( this, AttrAssistant ), row, 2 );
		d_assistant = new QLineEdit( this );
		grid->addWidget( d_assistant, row, 3 );
	}
	
	// Private Pane
	{
		int row = 0;
		QWidget* tab = new QWidget( tw );
		tw->addTab( tab, tr("&Private") );
		QGridLayout* grid = new QGridLayout( tab );

		grid->addWidget( _label( this, AttrPhonePriv ), row, 0 );
		d_phonePriv = new QLineEdit( this );
		grid->addWidget( d_phonePriv, row, 1 );
		
		grid->addWidget( _label( this, AttrMobilePriv ), row, 2 );
		d_mobilePriv = new QLineEdit( this );
		grid->addWidget( d_mobilePriv, row, 3 );
		
		row++;

		grid->addWidget( _label( this, AttrEmailPriv ), row, 0 );
		d_eMailPriv = new QLineEdit( this );
		grid->addWidget( d_eMailPriv, row, 1 );

		row++;

		grid->addWidget( _label( this, AttrMailAddrPriv ), row, 0 );
		d_addrPriv = new QTextEdit( this );
		d_addrPriv->setAcceptRichText( false );
		d_addrPriv->setTabChangesFocus( true );
		grid->addWidget( d_addrPriv, row, 1, 1, 3 );

		// End Panes
	}
	row++;

	QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	topGrid->addWidget( bb, row, 0, 1, 4 );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

	d_text->setFocus();
}

void HumanPropsDlg::loadFrom( Udb::Obj& o )
{
	d_text->setText( o.getString( AttrText ) );
	d_lastName->setText( o.getString( AttrPrincipalName ) );
	d_firstName->setText( o.getString( AttrFirstName ) );
	d_acaTitle->setText( o.getString( AttrQualiTitle ) );
	d_orgName->setText( o.getString( AttrOrganization ) );
	d_department->setText( o.getString( AttrDepartment ) );
	d_jobTitle->setText( o.getString( AttrJobTitle ) );
	d_eMail->setText( o.getString( AttrEmail ) );
	d_phoneDesk->setText( o.getString( AttrPhoneDesk ) );
	d_phoneMobile->setText( o.getString( AttrPhoneMobile ) );
	d_office->setText( o.getString( AttrOffice ) );
	d_addr->setText( o.getString( AttrMailAddr ) );
	d_phonePriv->setText( o.getString( AttrPhonePriv ) );
	d_mobilePriv->setText( o.getString( AttrMobilePriv ) );
	d_addrPriv->setText( o.getString( AttrMailAddrPriv ) );
	d_eMailPriv->setText( o.getString( AttrEmailPriv ) );
	d_manager->setText( o.getString( AttrManager ) );
	d_assistant->setText( o.getString( AttrAssistant ) );
	d_shortHand->setText( o.getString( AttrCustomId ) );
}

void HumanPropsDlg::saveTo( Udb::Obj& o ) const
{
	o.setString( AttrText, d_text->text().simplified() );
	o.setString( AttrPrincipalName, d_lastName->text().simplified() );
	o.setString( AttrFirstName, d_firstName->text().simplified() );
	o.setString( AttrQualiTitle, d_acaTitle->text().simplified() );
	o.setString( AttrOrganization, d_orgName->text().simplified() );
	o.setString( AttrDepartment, d_department->text().simplified() );
	o.setString( AttrJobTitle, d_jobTitle->text().simplified() );
	o.setString( AttrEmail, d_eMail->text().simplified() );
	o.setString( AttrPhoneDesk, d_phoneDesk->text().simplified() );
	o.setString( AttrPhoneMobile, d_phoneMobile->text().simplified() );
	o.setString( AttrOffice, d_office->text().simplified() );
	o.setString( AttrMailAddr, d_addr->toPlainText().trimmed() );
	o.setString( AttrPhonePriv, d_phonePriv->text().simplified() );
	o.setString( AttrMobilePriv, d_mobilePriv->text().simplified() );
	o.setString( AttrMailAddrPriv, d_addrPriv->toPlainText().trimmed() );
	o.setString( AttrEmailPriv, d_eMailPriv->text().simplified() );
	o.setString( AttrManager, d_manager->text().simplified() );
	o.setString( AttrAssistant, d_assistant->text().simplified() );
	o.setString( AttrCustomId, d_shortHand->text().simplified() );
}

bool HumanPropsDlg::edit( Udb::Obj& item )
{
	setWindowTitle( tr("Edit Person - WorkTree") );
	loadFrom( item );
	bool run = true;
	while( run )
	{
		if( exec() != QDialog::Accepted )
		{
			item.getTxn()->rollback();
			return false;
		}
		run = false;
		item.setValue( AttrModifiedOn, Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
		saveTo( item );
	}
	item.commit();
	return true;
}
