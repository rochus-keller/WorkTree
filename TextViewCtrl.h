#ifndef TEXTVIEWCTRL_H
#define TEXTVIEWCTRL_H

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

#include <QObject>
#include <Udb/Obj.h>
#include <Gui2/AutoMenu.h>
#include <Udb/UpdateInfo.h>
#include <QtGui/QLabel>

namespace Oln
{
    class OutlineUdbCtrl;
}
namespace Wt
{
    class ObjectTitleFrame;
    class CheckLabel;

    class TextViewCtrl : public QObject
    {
        Q_OBJECT
    public:
        static const char* s_pin;
        explicit TextViewCtrl(QWidget *parent = 0);
        QWidget* getWidget() const;
		static TextViewCtrl* create(QWidget* parent, Udb::Transaction* txn );
        void setObj( const Udb::Obj& );
        Gui2::AutoMenu* createPopup();
    signals:
		void signalFollowObject( const Udb::Obj& );  // Alias oder Link wurden aktiviert
		void signalItemActivated( const Udb::Obj& ); // Anderes Outline Item wurde angewählt
	protected slots:
        void onDbUpdate( Udb::UpdateInfo info );
        void onFollowAlias();
        void onTitleClick();
        void onAnchorActivated(QByteArray data, bool url);
		void onCurrentChanged(quint64);
    private:
        ObjectTitleFrame* d_title;
        Oln::OutlineUdbCtrl* d_oln;
        CheckLabel* d_pin;
    };

    class CheckLabel : public QLabel
    {
    public:
        CheckLabel( QWidget* w):QLabel(w),d_checked(false){}
        bool isChecked() const { return d_checked; }
        void setChecked( bool );
    protected:
        void mousePressEvent ( QMouseEvent * event );
    private:
        bool d_checked;
    };
}

#endif // TEXTVIEWCTRL_H
