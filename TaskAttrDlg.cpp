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

#include "TaskAttrDlg.h"
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpinBox>
#include <Udb/Transaction.h>
#include <QtGui/QMessageBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QComboBox>
#include <QtDebug>
#include "WtTypeDefs.h"
using namespace Wt;

TaskAttrDlg::TaskAttrDlg(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle( tr("Set Task/Milestone Attributes - WorkTree") );
    QGridLayout* grid = new QGridLayout( this );

    int row = 0;
    grid->addWidget( new QLabel( tr("Duration (workdays):"), this ), row, 0 );
    d_duration = new QSpinBox( this );
    grid->addWidget( d_duration, row, 1 );
    row++;

    grid->addWidget( new QLabel( tr("Calendar:"), this ), row, 0 );
    d_cals = new QComboBox( this );
    d_cals->setInsertPolicy( QComboBox::InsertAlphabetically );
    grid->addWidget( d_cals, row, 1 );
    row++;

    grid->addWidget( new QLabel( tr("Planned Value (PV):"), this ), row, 0 );
    d_pv = new QLineEdit( this );
    grid->addWidget( d_pv, row, 1 );
    row++;

    grid->addWidget( new QLabel( tr("Earned Value (EV):"), this ), row, 0 );
    d_ev = new QLineEdit( this );
    grid->addWidget( d_ev, row, 1 );
    row++;

    grid->addWidget( new QLabel( tr("Actual Cost (AC):"), this ), row, 0 );
    d_ac = new QLineEdit( this );
    grid->addWidget( d_ac, row, 1 );
    row++;

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	grid->addWidget( bb, row, 0, 1, 2 );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
}

static bool _setValue( QLineEdit* e, Udb::Obj & o, quint32 attr )
{
    if( e->isEnabled() )
    {
        if( e->text().trimmed().isEmpty() )
        {
            o.clearValue( attr );
            return true;
        }
        bool ok = true;
        quint32 v = e->text().toUInt( &ok );
        if( ok )
        {
            o.setValue( attr, Stream::DataCell().setUInt32(v) );
            return true;
        }else
            return false;
    }
    return true;
}

bool TaskAttrDlg::edit(Udb::Obj & o)
{
    if( o.isNull() )
        return false;
    d_pv->clear();
    d_ev->clear();
    d_ev->clear();
    d_duration->setValue( 0 );
    d_pv->setEnabled( true );
    d_ev->setEnabled( true );
    d_ac->setEnabled( true );
    d_duration->setEnabled( true );
    d_cals->clear();
    Udb::Obj cals = WtTypeDefs::getCalendars( o.getTxn() );
    d_cals->addItem( tr("<default>"), 0 );
    Udb::Obj cal = cals.getFirstObj();
    if( !cal.isNull() ) do
    {
        if( cal.getType() == TypeCalendar )
            d_cals->addItem( WtTypeDefs::formatObjectTitle( cal ), cal.getOid() );
    }while( cal.next() );
    d_cals->setEnabled( true );

    switch( o.getType() )
    {
    case TypeTask:
        d_duration->setMinimum( 1 );
        d_duration->setMaximum( 0xffff );
        if( o.getValue( AttrSubTMSCount ).getUInt32() > 0 )
        {
            d_duration->setEnabled( false );
            d_ev->setEnabled( false );
            d_ac->setEnabled( false );
            d_cals->setEnabled( false );
        }
        if( o.hasValue( AttrDuration ) )
            d_duration->setValue( o.getValue( AttrDuration ).getUInt16() );
        break;
    case TypeMilestone:
        d_duration->setMinimum( 0 );
        d_duration->setEnabled( false );
        break;
    }
    if( o.hasValue( AttrPlannedValue ) )
        d_pv->setText( QString::number( o.getValue( AttrPlannedValue ).getUInt32() ) );
    if( o.hasValue( AttrEarnedValue ) )
        d_ev->setText( QString::number( o.getValue( AttrEarnedValue ).getUInt32() ) );
    if( o.hasValue( AttrActualCost ) )
        d_ac->setText( QString::number( o.getValue( AttrActualCost ).getUInt32() ) );
    d_cals->setCurrentIndex( d_cals->findData( o.getValue( AttrCalendar ).getOid() ) );

    while( exec() == QDialog::Accepted )
    {
        if( d_duration->isEnabled() )
            o.setValue( AttrDuration, Stream::DataCell().setUInt16( d_duration->value() ) );

        if( d_cals->isEnabled() )
        {
            if( d_cals->currentIndex() == -1 ||
                    d_cals->itemData( d_cals->currentIndex() ).toULongLong() == 0 )
                o.clearValue( AttrCalendar );
            else
                o.setValue( AttrCalendar, Stream::DataCell().setOid(
                                d_cals->itemData( d_cals->currentIndex() ).toULongLong() ) );
        }
        if( _setValue( d_pv, o, AttrPlannedValue ) && _setValue( d_ev, o, AttrEarnedValue ) &&
                _setValue( d_ac, o, AttrActualCost ) )
        {
            o.commit();
            return true;
        }else
        {
            QMessageBox::warning( this, windowTitle(), tr("Invalid value in PV, EV or AC; enter a positive "
                                                          "number or an empty string!" ) );
        }
    }
    o.getTxn()->rollback();
    return false;
}
