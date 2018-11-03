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

#include "CalendarEditor.h"
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDate>
#include <QCalendarWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidget>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <QDateEdit>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#ifdef _HAS_ICS_
#include <Herald/IcsDocument.h>
#endif
#include <Udb/Idx.h>
#include <Udb/Transaction.h>
#include "WorkTreeApp.h"
#include "WtTypeDefs.h"
#include "ObjectHelper.h"
using namespace Wt;

class Wt::_CalendarModel : public QAbstractItemModel
{
public:
    Udb::Obj d_cal;
    QList<Udb::Obj> d_entries;
    int d_year;
    _CalendarModel( QObject* p ):QAbstractItemModel(p),d_year(0)
    {
    }
    void setCalendar( const Udb::Obj& cal )
    {
        d_cal = cal;
        refill();
    }
    void refill()
    {
        d_entries.clear();
        Udb::Idx idx( d_cal.getTxn(), IndexDefs::IdxCalDate );
        if( idx.seek( d_cal ) ) do
        {
            Udb::Obj entry = d_cal.getObject( idx.getOid() );
            if( entry.getType() == TypeCalEntry )
            {
                if( d_year == 0 )
                    d_entries.append( entry );
                else
                {
                    QDate start = entry.getValue( AttrCalDate ).getDate();
                    QDate finish = start.addDays( entry.getValue( AttrCalDuration ).getUInt16() - 1 );
                    if( start.year() == d_year || finish.year() == d_year )
                        d_entries.append( entry );
                }
            }
        }while( idx.nextKey() );
        reset();
    }
    void notify( const QModelIndex& i )
    {
        dataChanged( i, i );
    }
    QModelIndex find( const Udb::Obj& o )
    {
        const int pos = d_entries.indexOf( o );
        if( pos == -1 )
            return QModelIndex();
        else
            return index( pos, 0, QModelIndex() );
    }
    QModelIndex	index ( int row, int column, const QModelIndex & parent ) const
    {
        if( parent.isValid() )
            return QModelIndex();
        if( row < d_entries.size() )
            return createIndex( row, column, row );
        else
            return QModelIndex();
    }
    QModelIndex parent ( const QModelIndex & index ) const
    {
        return QModelIndex();
    }
    int	columnCount( const QModelIndex & parent = QModelIndex() ) const
    {
        return 4;
    }
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const
    {
        if( !parent.isValid() )
            return d_entries.size();
        else
            return 0;
    }
    QVariant data ( const QModelIndex & index, int role ) const
    {
        if( d_entries[ index.row() ].isNull(true) )
            return QVariant();
        if( role == Qt::DisplayRole || role == Qt::EditRole )
        {
            switch( index.column() )
            {
            case 0:
                return d_entries[ index.row() ].getValue( AttrCalDate ).getDate();
            case 1:
                {
                    quint16 d = d_entries[ index.row() ].getValue( AttrCalDuration ).getUInt16();
                    if( d == 1 )
                        return QVariant();
                    else
                        return d_entries[ index.row() ].getValue( AttrCalDate ).getDate().addDays( d - 1 );
                }
            case 3:
                return d_entries[ index.row() ].getValue( AttrText ).toString();
            default:
                return QVariant();
            }
        }else if( index.column() == 2 && role == Qt::CheckStateRole )
        {
            if( d_entries[ index.row() ].getValue( AttrNonWorking ).getBool() )
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }else
            return QVariant();
    }
    QVariant headerData ( int section, Qt::Orientation orientation, int role ) const
    {
        if( role != Qt::DisplayRole )
            return QAbstractItemModel::headerData( section, orientation, role );

        if( orientation == Qt::Horizontal )
        {
            switch( section )
            {
            case 0:
                return tr("Start");
            case 1:
                return tr("Finish");
            case 2:
                return tr("Nonworking");
            case 3:
                return tr("What");
            default:
                return tr("?");
            }
        }else
            return section + 1;
    }
    Qt::ItemFlags flags ( const QModelIndex & index ) const
    {
        Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if( index.column() == 2 )
            f |= Qt::ItemIsUserCheckable;
        return f;
    }
};

CalendarEditor::CalendarEditor(QWidget *parent) :
    QWidget(parent),d_loadLock(false)
{
    QHBoxLayout* hbox = new QHBoxLayout( this );

    d_list = new QTreeView( this );
    d_list->setAlternatingRowColors( true );
    d_list->setAllColumnsShowFocus( true );
    d_list->setRootIsDecorated( false );
    d_list->setSelectionMode( QAbstractItemView::ExtendedSelection );
    d_list->header()->setStretchLastSection( true );
    hbox->addWidget( d_list );
    d_mdl = new _CalendarModel( this );
    d_list->setModel( d_mdl );
    connect( d_list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(onSelectionChanged()));

    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setMargin(0);
    hbox->addLayout( vbox );

    QHBoxLayout* h2 = new QHBoxLayout();
    h2->setMargin(0);
    vbox->addLayout(h2);
    d_year = new QSpinBox(this);
    d_year->setRange(1, 9999);
    d_year->setValue( QDate::currentDate().year() );
    d_year->setEnabled(false);
    connect( d_year, SIGNAL(valueChanged(int)), this, SLOT(onFilter()) );
    h2->addWidget(d_year);
    QCheckBox* cb = new QCheckBox( tr("Filter by year"), this );
    h2->addWidget( cb );
    connect( cb, SIGNAL(toggled(bool)), d_year,SLOT(setEnabled(bool)) );
    connect( cb, SIGNAL(toggled(bool)), this, SLOT(onFilter()) );

    vbox->addSpacing( 15 );

    vbox->addWidget( new QLabel( tr("Set parent calendar:" ) ) );
    d_parentCal = new QComboBox( this );
    vbox->addWidget( d_parentCal );
    connect( d_parentCal,SIGNAL(currentIndexChanged(int)),this,SLOT(onParentCal(int)) );
    d_parentCal->setInsertPolicy( QComboBox::InsertAlphabetically );

    QGroupBox* gb = new QGroupBox( tr("Set nonworking days:"), this );
    vbox->addWidget( gb );
    QVBoxLayout* vbox2 = new QVBoxLayout();
    gb->setLayout( vbox2 );

    cb = new QCheckBox( tr("Monday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Tuesday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Wednesday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Thursday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Friday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Saturday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    cb = new QCheckBox( tr("Sunday"),this );
    cb->setTristate( true );
    vbox2->addWidget( cb );
    connect( cb, SIGNAL(stateChanged(int)),this,SLOT(onNonWorkingDays()));
    d_nonWorkingDays.append( cb );

    vbox->addStretch();

	QPushButton* b;

#ifdef _HAS_ICS_
	b = new QPushButton( tr("Import..."), this );
    vbox->addWidget( b );
    connect( b, SIGNAL(clicked()), this, SLOT(onImportIcs()) );
#endif

    b = new QPushButton( tr("New..."), this );
    vbox->addWidget( b );
    connect( b, SIGNAL(clicked()), this, SLOT(onNew()) );

    d_edit = new QPushButton( tr("Edit..."), this );
    d_edit->setEnabled( false );
    vbox->addWidget( d_edit );
    connect( d_edit, SIGNAL(clicked()), this, SLOT(onEdit()) );

    d_move = new QPushButton( tr("Copy..."), this );
    d_move->setEnabled( false );
    vbox->addWidget( d_move );
    connect( d_move, SIGNAL(clicked()), this, SLOT(onCopyTo()) );

    d_delete = new QPushButton( tr("Delete..."), this );
    d_delete->setEnabled(false);
    vbox->addWidget( d_delete );
    connect( d_delete, SIGNAL(clicked()), this, SLOT(onDelete()) );
}

static Qt::CheckState _toState( char c )
{
    if( c == '1' )
        return Qt::Checked;
    else if( c == '0' )
        return Qt::Unchecked;
    else
        return Qt::PartiallyChecked;
}

void CalendarEditor::setCalendar(const Udb::Obj & cal)
{
    d_loadLock = true;
    Q_ASSERT( !cal.isNull() );
    d_parentCal->clear();
    Udb::Obj cals = WtTypeDefs::getCalendars( cal.getTxn() );
    d_parentCal->addItem( tr("<none>"), 0 );
    Udb::Obj c = cals.getFirstObj();
    if( !c.isNull() ) do
    {
        if( c.getType() == TypeCalendar && !c.equals( cal ) )
            d_parentCal->addItem( c.getString( AttrText ), c.getOid() );
    }while( c.next() );
    d_mdl->setCalendar( cal );
    d_parentCal->setCurrentIndex( d_parentCal->findData( cal.getValue( AttrParentCalendar ).getOid() ) );
    QByteArray nwd = cal.getValue(AttrNonWorkingDays).getArr();
    for( int i = 0; i < d_nonWorkingDays.size(); i++ )
    {
        Qt::CheckState s = Qt::PartiallyChecked;
        if( i < nwd.size() )
            s = _toState(nwd[i]);
        d_nonWorkingDays[i]->setCheckState(s);
    }
    d_loadLock = false;
    const int cw = d_list->fontMetrics().width( QDate::currentDate().toString( Qt::ISODate ) + "   " );
    d_list->setColumnWidth( 0, cw );
    d_list->setColumnWidth( 1, cw );
    d_list->setColumnWidth( 2, cw );
}

static char _toChar( Qt::CheckState s )
{
    switch( s )
    {
    case Qt::Unchecked:
        return '0';
    case Qt::PartiallyChecked:
        return ' ';
    case Qt::Checked:
        return '1';
    }
    return '?';
}

void CalendarEditor::onNonWorkingDays()
{
    if( d_loadLock )
        return;
    QByteArray nwd(7,'0');
    for( int i = 0; i < d_nonWorkingDays.size(); i++ )
    {
        nwd[i] = _toChar(d_nonWorkingDays[i]->checkState());
    }
    d_mdl->d_cal.setValue( AttrNonWorkingDays, Stream::DataCell().setAscii(nwd) );
    d_mdl->d_cal.commit();
}

void CalendarEditor::onParentCal(int index)
{
    if( d_loadLock )
        return;
    d_mdl->d_cal.setValue( AttrParentCalendar, Stream::DataCell().setOid(
                               d_parentCal->itemData( index ).toULongLong() ) );
    d_mdl->d_cal.commit();
}

void CalendarEditor::onImportIcs()
{
#ifdef _HAS_ICS_
    QString path = QFileDialog::getOpenFileName( this, tr("Import Nonworking Days"), QDir::currentPath(),
                                                 tr("iCalendar File (*.ics)") );
    if( path.isEmpty() )
        return;

    QDir::setCurrent( QFileInfo(path).absolutePath() );
    He::IcsDocument ics;
    if( !ics.loadFrom(path) )
    {
        QMessageBox::critical( this, tr("Import iCalendar File"), ics.getError() );
        return; // RISK
    }
    foreach( const He::IcsDocument::Component& c, ics.getComps() )
    {
        if( c.d_type == He::IcsDocument::VEVENT )
        {
            const He::IcsDocument::Property& subject = c.findProp( He::IcsDocument::SUMMARY );
            const He::IcsDocument::Property& start = c.findProp( He::IcsDocument::DTSTART );
            const He::IcsDocument::Property& end = c.findProp( He::IcsDocument::DTEND );
            const He::IcsDocument::Property& dur = c.findProp( He::IcsDocument::DURATION );
            if( !start.isValid() )
                continue;
            Udb::Obj e = ObjectHelper::createObject( TypeCalEntry, d_mdl->d_cal );
            e.setValue( AttrCalDate, Stream::DataCell().setDate( start.d_value.toDate() ) );
            if( end.isValid() )
            {
                e.setValue( AttrCalDuration, Stream::DataCell().setUInt16(
                                start.d_value.toDate().daysTo(start.d_value.toDate()) + 1 ) );
            }else if( dur.isValid() )
            {
                e.setValue( AttrCalDuration, Stream::DataCell().setUInt16(
                                dur.d_value.toULongLong() / ( 60 * 60 * 24 )) );

            }else
                e.setValue( AttrCalDuration, Stream::DataCell().setUInt16(1) );
            e.setValue( AttrNonWorking, Stream::DataCell().setBool(true) );
            e.setString( AttrText, subject.d_value.toString() );
        }
    }
    d_mdl->d_cal.commit();
    d_mdl->refill();
#endif
}

void CalendarEditor::onSelectionChanged()
{
    const int count = d_list->selectionModel()->selectedRows().size();
    d_delete->setEnabled( count > 0 );
    d_edit->setEnabled( count == 1 );
    d_move->setEnabled( count > 0 );
}

void CalendarEditor::onDelete()
{
    QModelIndexList l = d_list->selectionModel()->selectedRows();
    if( QMessageBox::question( this, tr("Deleting Calendar Entries - WorkTree"),
                               tr("Do you really want to delete %1 entry? This cannot be undone.").arg(l.size()),
                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok ) == QMessageBox::Cancel )
        return;
    foreach( QModelIndex i, l )
        ObjectHelper::erase( d_mdl->d_entries[i.row()] );
    d_mdl->d_cal.commit();
    d_mdl->refill();
}

void CalendarEditor::onNew()
{
    CalEntryDlg dlg( this );
    dlg.setWindowTitle( tr("New Calendar Entry - WorkTree") );
    while( dlg.exec() == QDialog::Accepted )
    {
        if( dlg.getWhat().isEmpty() )
        {
            QMessageBox::critical( this, dlg.windowTitle(), tr("'What' cannot be empty!") );
            continue;
        }
        if( dlg.getFinish() < dlg.getStart() )
        {
            QMessageBox::critical( this, dlg.windowTitle(), tr("'Finish' cannot be earlier than 'Start'!") );
            continue;
        }
        Udb::Obj entry = ObjectHelper::createObject( TypeCalEntry, d_mdl->d_cal );
        entry.setString( AttrText, dlg.getWhat() );
        entry.setValue( AttrCalDate, Stream::DataCell().setDate( dlg.getStart() ) );
        entry.setValue( AttrCalDuration, Stream::DataCell().setUInt16(
                            dlg.getStart().daysTo( dlg.getFinish() ) + 1 ) );
        entry.setBool( AttrNonWorking, dlg.getNonWorking() );
        entry.commit();
        d_mdl->refill();
        QModelIndex i = d_mdl->find( entry );
        d_list->selectionModel()->select( i, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                          | QItemSelectionModel::Current );
        d_list->scrollTo( i );
        d_list->setFocus();
        return;
    }
}

void CalendarEditor::onEdit()
{
    QModelIndexList l = d_list->selectionModel()->selectedRows();
    Udb::Obj entry = d_mdl->d_entries[l.first().row()];
    CalEntryDlg dlg( this );
    dlg.setWindowTitle( tr("Edit Calendar Entry - WorkTree") );
    dlg.setWhat( entry.getString( AttrText ) );
    dlg.setStart( entry.getValue( AttrCalDate ).getDate() );
    dlg.setFinish( dlg.getStart().addDays( entry.getValue( AttrCalDuration ).getUInt16() - 1 ) );
    dlg.setNonWorking( entry.getValue( AttrNonWorking ).getBool() );
    while( dlg.exec() == QDialog::Accepted )
    {
        if( dlg.getWhat().isEmpty() )
        {
            QMessageBox::critical( this, dlg.windowTitle(), tr("'What' cannot be empty!") );
            continue;
        }
        if( dlg.getFinish() < dlg.getStart() )
        {
            QMessageBox::critical( this, dlg.windowTitle(), tr("'Finish' cannot be earlier than 'Start'!") );
            continue;
        }
        entry.setString( AttrText, dlg.getWhat() );
        entry.setValue( AttrCalDate, Stream::DataCell().setDate( dlg.getStart() ) );
        entry.setValue( AttrCalDuration, Stream::DataCell().setUInt16(
                            dlg.getStart().daysTo( dlg.getFinish() ) + 1 ) );
        entry.setBool( AttrNonWorking, dlg.getNonWorking() );
        entry.commit();
        d_mdl->notify( l.first() );
        d_list->setFocus();
        return;
    }
}

void CalendarEditor::onFilter()
{
    if( d_year->isEnabled() )
        d_mdl->d_year = d_year->value();
    else
        d_mdl->d_year = 0;
    d_mdl->refill();
}

void CalendarEditor::onCopyTo()
{
    QModelIndexList l = d_list->selectionModel()->selectedRows();

    CalendarSelectorDlg dlg( tr("Select the calendar where you want the %1\n"
                                "selected entries to copy to:").arg(l.size()), this );
    dlg.setWindowTitle( tr("Copy Calendar Entries - WorkTree" ) );
    Udb::Obj cal = dlg.select( d_mdl->d_cal.getTxn() );
    if( cal.isNull() )
        return;
    foreach( QModelIndex i, l )
    {
        Udb::Obj entry = ObjectHelper::createObject( TypeCalEntry, cal );
        entry.setValue(AttrText, d_mdl->d_entries[i.row()].getValue(AttrText));
        entry.setValue(AttrCalDate, d_mdl->d_entries[i.row()].getValue(AttrCalDate));
        entry.setValue(AttrCalDuration, d_mdl->d_entries[i.row()].getValue(AttrCalDuration));
        entry.setValue(AttrNonWorking, d_mdl->d_entries[i.row()].getValue(AttrNonWorking));
        entry.setValue(AttrAvaility, d_mdl->d_entries[i.row()].getValue(AttrAvaility));
    }
    cal.commit();
}

CalEntryDlg::CalEntryDlg(QWidget *p):QDialog(p),d_lock(false)
{
    QFormLayout* form = new QFormLayout( this );
    d_what = new QLineEdit( this );
    d_what->setMinimumWidth( 200 );
    form->addRow( tr("What:"), d_what );
    d_start = new QDateEdit( this );
    d_start->setDisplayFormat("yyyy-MM-dd");
    d_start->setCalendarPopup( true );
    d_start->setDate( QDate::currentDate() );
    form->addRow( tr("Start:"), d_start );
    d_finish = new QDateEdit( this );
    d_finish->setDisplayFormat("yyyy-MM-dd");
    d_finish->setCalendarPopup( true );
    d_finish->setDate( QDate::currentDate() );
    form->addRow( tr("Finish:"), d_finish );
    d_dur = new QSpinBox(this);
    d_dur->setMinimum( 1 );
    form->addRow( tr("Duration (days):"), d_dur );
    d_nonWorking = new QCheckBox( this );
    d_nonWorking->setChecked( true );
    form->addRow( tr("Nonworking:"), d_nonWorking );

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	form->addRow( bb );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    connect( d_start, SIGNAL(dateChanged(QDate)), this, SLOT(onStart()) );
    connect( d_finish, SIGNAL(dateChanged(QDate)), this, SLOT(onFinish()) );
    connect( d_dur, SIGNAL(valueChanged(int)), this, SLOT(onDur()) );
}

void CalEntryDlg::setStart(const QDate & d)
{
    d_start->setDate( d );
}

QDate CalEntryDlg::getStart() const
{
    return d_start->date();
}

void CalEntryDlg::setFinish(const QDate & d)
{
    d_finish->setDate( d );
}

QDate CalEntryDlg::getFinish() const
{
    return d_finish->date();
}

void CalEntryDlg::setWhat(const QString & s)
{
    d_what->setText( s );
}

QString CalEntryDlg::getWhat() const
{
    return d_what->text();
}

void CalEntryDlg::setNonWorking(bool b)
{
    d_nonWorking->setChecked( b );
}

bool CalEntryDlg::getNonWorking() const
{
    return d_nonWorking->isChecked();
}

void CalEntryDlg::onStart()
{
    d_finish->setDate( d_start->date().addDays( d_dur->value() - 1 ) );
}

void CalEntryDlg::onFinish()
{
    if( d_lock )
        return;
    d_lock = true;
    d_dur->setValue( d_start->date().daysTo( d_finish->date() ) + 1 );
    d_lock = false;
}

void CalEntryDlg::onDur()
{
    if( d_lock )
        return;
    d_lock = true;
    d_finish->setDate( d_start->date().addDays( d_dur->value() - 1 ) );
    d_lock = false;
}

CalendarEditorDlg::CalendarEditorDlg(QWidget * p):QDialog(p)
{
    QVBoxLayout* vbox = new QVBoxLayout( this );
    vbox->setMargin(0);
    vbox->setSpacing(0);

    d_cal = new CalendarEditor( this );
    vbox->addWidget( d_cal );

    QHBoxLayout* hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this );
	hbox->addWidget( bb );
    hbox->setMargin( 10 );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    setMinimumWidth( 640 );
}

void CalendarEditorDlg::setCalendar(const Udb::Obj & cal)
{
    Q_ASSERT( !cal.isNull() );
    d_cal->setCalendar( cal );
    setWindowTitle( tr("Calendar '%1' - WorkTree" ).arg( cal.getString( AttrText ) ) );
}

CalendarListDlg::CalendarListDlg(Udb::Transaction *txn, QWidget * p):QDialog(p),d_txn(txn)
{
    setWindowTitle( tr("Calendars - WorkTree") );
    QVBoxLayout* vbox = new QVBoxLayout( this );
    QHBoxLayout* hbox = new QHBoxLayout();
    vbox->addLayout( hbox );

    d_list = new QTreeWidget( this );
    d_list->setRootIsDecorated( false );
    d_list->setAllColumnsShowFocus( true );
    d_list->setAlternatingRowColors( true );
    d_list->setHeaderLabels( QStringList() << tr("Calendar") << tr("Default") );
    d_list->setSortingEnabled( true );
    hbox->addWidget( d_list );

    QVBoxLayout* v2 = new QVBoxLayout();
    hbox->addLayout( v2 );

    QPushButton* b = new QPushButton( tr("New..."), this );
    v2->addWidget( b );
    connect( b, SIGNAL(clicked()), this, SLOT(onNew()) );

    d_edit = new QPushButton( tr("Edit..."), this );
    d_edit->setEnabled(false);
    v2->addWidget( d_edit );
    connect( d_edit, SIGNAL(clicked()), this, SLOT(onEdit()) );

    d_rename = new QPushButton( tr("Rename..."), this );
    d_rename->setEnabled(false);
    v2->addWidget( d_rename );
    connect( d_rename, SIGNAL(clicked()), this, SLOT(onRename()) );

    d_setDefault = new QPushButton( tr("Set Default"), this );
    d_setDefault->setEnabled(false);
    v2->addWidget( d_setDefault );
    connect( d_setDefault, SIGNAL(clicked()), this, SLOT(onDefault()) );

    d_delete = new QPushButton( tr("Delete..."), this );
    d_delete->setEnabled(false);
    v2->addWidget( d_delete );
    connect( d_delete, SIGNAL(clicked()), this, SLOT(onDelete()) );

    v2->addStretch();

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this );
	vbox->addWidget( bb );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));

    connect( d_list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(onSelected()) );
    connect( d_list, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onEdit()) );
}

void CalendarListDlg::refill()
{
    d_list->clear();
    Udb::Obj cals = WtTypeDefs::getCalendars( d_txn );
    Udb::Obj def = cals.getValueAsObj( AttrDefaultCal );
    Udb::Obj cal = cals.getFirstObj();
    if( !cal.isNull() ) do
    {
        if( cal.getType() == TypeCalendar )
        {
            QTreeWidgetItem* i = new QTreeWidgetItem( d_list );
            i->setText(0, cal.getString( AttrText ) );
            i->setData(0, Qt::UserRole, cal.getOid() );
            if( cal.equals( def ) )
                i->setText(1, tr("x") );
            else
                i->setText( 1, QString() );
            i->setTextAlignment(1, Qt::AlignHCenter );
        }
    }while( cal.next() );
    d_list->resizeColumnToContents(0);
    d_list->sortItems( 0, Qt::AscendingOrder );
}

void CalendarListDlg::onEdit()
{
    QTreeWidgetItem* i = d_list->currentItem();
    Q_ASSERT( i != 0 );
    Udb::Obj cal = d_txn->getObject( i->data( 0, Qt::UserRole ).toLongLong() );
    Q_ASSERT( !cal.isNull() );

    CalendarEditorDlg dlg( this );
    dlg.setCalendar( cal );
    dlg.exec();
}

void CalendarListDlg::onSelected()
{
    d_edit->setEnabled( d_list->selectionModel()->selectedRows().size() == 1 );
    d_delete->setEnabled( d_edit->isEnabled() );
    d_setDefault->setEnabled( d_edit->isEnabled() );
    d_rename->setEnabled( d_edit->isEnabled() );
}

void CalendarListDlg::onRename()
{
    QTreeWidgetItem* item = d_list->currentItem();
    Q_ASSERT( item != 0 );
    Udb::Obj cal = d_txn->getObject( item->data( 0, Qt::UserRole ).toLongLong() );
    Q_ASSERT( !cal.isNull() );

    bool ok = true;
    const QString title = tr("Rename Calendar - WorkTree" );
    QString name = cal.getString( AttrText );
    while( true )
    {
        name = QInputDialog::getText( this, title, tr("Name (not empty):"), QLineEdit::Normal, name, &ok );
        if( !ok )
            return;
        if( name.isEmpty() )
        {
            QMessageBox::critical( this, title, tr("Name cannot be empty!") );
            continue;
        }
        if( !d_list->findItems( name, Qt::MatchFixedString ).isEmpty() )
        {
            QMessageBox::critical( this, title, tr("Name is not unique!") );
            continue;
        }
        cal.setString( AttrText, name );
        item->setText( 0, name );
        cal.commit();
        return;
    }
}

void CalendarListDlg::onDefault()
{
    QTreeWidgetItem* item = d_list->currentItem();
    Q_ASSERT( item != 0 );
    Udb::Obj cal = d_txn->getObject( item->data( 0, Qt::UserRole ).toLongLong() );
    Q_ASSERT( !cal.isNull() );
    Udb::Obj cals = WtTypeDefs::getCalendars( d_txn );
    cals.setValue( AttrDefaultCal, cal );
    for( int i = 0; i < d_list->topLevelItemCount(); i++ )
        d_list->topLevelItem( i )->setText( 1, QString() );
    item->setText( 1, tr("x") );
    cal.commit();
}

void CalendarListDlg::onDelete()
{
    QTreeWidgetItem* item = d_list->currentItem();
    Q_ASSERT( item != 0 );
    Udb::Obj cal = d_txn->getObject( item->data( 0, Qt::UserRole ).toLongLong() );
    Q_ASSERT( !cal.isNull() );
    if( QMessageBox::question( this, tr("Deleting Calendar - WorkTree"),
                               tr("Do you really want to delete this calendar? This cannot be undone."),
                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel ) == QMessageBox::Cancel )
        return;
    Udb::Obj cals = WtTypeDefs::getCalendars( d_txn );
    if( cals.getValueAsObj(AttrDefaultCal).equals( cal ) )
        cals.clearValue( AttrDefaultCal );
    ObjectHelper::erase( cal );
    d_txn->commit();
    delete item;
}

void CalendarListDlg::onNew()
{
    bool ok = true;
    const QString title = tr("Create Calendar - WorkTree" );
    QString name;
    while( true )
    {
        name = QInputDialog::getText( this, title, tr("Name (not empty):"), QLineEdit::Normal, name, &ok );
        if( !ok )
            return;
        if( name.isEmpty() )
        {
            QMessageBox::critical( this, title, tr("Name cannot be empty!") );
            continue;
        }
        if( !d_list->findItems( name, Qt::MatchFixedString ).isEmpty() )
        {
            QMessageBox::critical( this, title, tr("Name is not unique!") );
            continue;
        }
        Udb::Obj cals = WtTypeDefs::getCalendars( d_txn );
        Udb::Obj cal = ObjectHelper::createObject( TypeCalendar, cals );
        cal.setString( AttrText, name );
        cal.commit();
        QTreeWidgetItem* i = new QTreeWidgetItem( d_list );
        i->setText(0, name );
        i->setData(0, Qt::UserRole, cal.getOid() );
        i->setTextAlignment(1, Qt::AlignHCenter );
        d_list->sortItems( 0, Qt::AscendingOrder );
        d_list->setCurrentItem( i );
        onEdit();
        return;
    }
}

CalendarSelectorDlg::CalendarSelectorDlg(const QString& text, QWidget * p):QDialog(p)
{
    setWindowTitle( tr("Select Calendar - WorkTree") );
    QVBoxLayout* vbox = new QVBoxLayout( this );

    vbox->addWidget( new QLabel( text, this ) );

    d_list = new QTreeWidget( this );
    d_list->setRootIsDecorated( false );
    d_list->setAllColumnsShowFocus( true );
    d_list->setAlternatingRowColors( true );
    d_list->setHeaderHidden(true);
    vbox->addWidget( d_list );

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel, Qt::Horizontal, this );
	vbox->addWidget( bb );
    d_ok = bb->button( QDialogButtonBox::Ok );
    d_ok->setEnabled( false );
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
    connect( d_list->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(onSelect()) );
    connect( d_list, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(accept()) );
}

Udb::Obj CalendarSelectorDlg::select(Udb::Transaction *txn)
{
    Udb::Obj cals = WtTypeDefs::getCalendars( txn );
    Udb::Obj cal = cals.getFirstObj();
    if( !cal.isNull() ) do
    {
        if( cal.getType() == TypeCalendar )
        {
            QTreeWidgetItem* i = new QTreeWidgetItem( d_list );
            i->setText(0, cal.getString( AttrText ) );
            i->setData(0, Qt::UserRole, cal.getOid() );
        }
    }while( cal.next() );
    d_list->sortItems( 0, Qt::AscendingOrder );
    if( exec() != QDialog::Accepted )
        return Udb::Obj();
    Q_ASSERT( d_list->currentItem() != 0 );
    return txn->getObject( d_list->currentItem()->data(0, Qt::UserRole).toULongLong() );
}

void CalendarSelectorDlg::onSelect()
{
    d_ok->setEnabled( d_list->selectionModel()->selectedRows().size() == 1 );
}

