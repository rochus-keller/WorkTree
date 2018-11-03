#ifndef CALENDAREDITOR_H
#define CALENDAREDITOR_H

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

#include <QDialog>
#include <Udb/Obj.h>

class QTreeView;
class QComboBox;
class QCheckBox;
class QPushButton;
class QDateEdit;
class QLineEdit;
class QSpinBox;
class QTreeWidget;

namespace Wt
{
    class _CalendarModel;

    class CalendarEditor : public QWidget
    {
        Q_OBJECT
    public:
        explicit CalendarEditor(QWidget *parent );
        void setCalendar( const Udb::Obj& );
    protected slots:
        void onNonWorkingDays();
        void onParentCal(int index);
        void onImportIcs();
        void onSelectionChanged();
        void onDelete();
        void onNew();
        void onEdit();
        void onFilter();
        void onCopyTo();
    private:
        QTreeView* d_list;
        _CalendarModel* d_mdl;
        QComboBox* d_parentCal;
        QList<QCheckBox*> d_nonWorkingDays; // [0]..Montag bis [6] Sonntag
        QPushButton* d_delete;
        QPushButton* d_edit;
        QPushButton* d_move;
        QSpinBox* d_year;
        bool d_loadLock;
    };

    class CalendarEditorDlg : public QDialog
    {
    public:
        CalendarEditorDlg(QWidget* p);
        void setCalendar( const Udb::Obj& );
    private:
        CalendarEditor* d_cal;
    };

    class CalEntryDlg : public QDialog
    {
        Q_OBJECT
    public:
        CalEntryDlg( QWidget* p );
        void setStart( const QDate& );
        QDate getStart() const;
        void setFinish( const QDate& );
        QDate getFinish() const;
        void setWhat( const QString& );
        QString getWhat() const ;
        void setNonWorking( bool );
        bool getNonWorking() const;
    protected slots:
        void onStart();
        void onFinish();
        void onDur();
    private:
        QDateEdit* d_start;
        QDateEdit* d_finish;
        QCheckBox* d_nonWorking;
        QLineEdit* d_what;
        QSpinBox* d_dur;
        bool d_lock;
    };

    class CalendarListDlg : public QDialog
    {
        Q_OBJECT
    public:
        CalendarListDlg( Udb::Transaction * txn, QWidget* );
        void refill();
    protected slots:
        void onEdit();
        void onSelected();
        void onRename();
        void onDefault();
        void onDelete();
        void onNew();
    private:
        Udb::Transaction * d_txn;
        QTreeWidget* d_list;
        QPushButton* d_edit;
        QPushButton* d_delete;
        QPushButton* d_setDefault;
        QPushButton* d_rename;
    };

    class CalendarSelectorDlg : public QDialog
    {
        Q_OBJECT
    public:
        CalendarSelectorDlg( const QString& text, QWidget* );
        Udb::Obj select( Udb::Transaction * txn );
    protected slots:
        void onSelect();
    private:
        QTreeWidget* d_list;
        QPushButton* d_ok;
    };
}

#endif // CALENDAREDITOR_H
