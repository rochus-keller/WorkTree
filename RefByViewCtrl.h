#ifndef REFBYVIEWCTRL_H
#define REFBYVIEWCTRL_H

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
#include <Oln2/RefByItemMdl.h>

namespace Wt
{
	class ObjectTitleFrame;
	class CheckLabel;

	class RefByViewCtrl : public QObject
	{
		Q_OBJECT
	public:
		explicit RefByViewCtrl(QWidget *parent = 0);

		static RefByViewCtrl* create(QWidget* parent, Udb::Transaction* txn );
		QWidget* getWidget() const;
		void setObj( const Udb::Obj& );
		void addCommands(Gui2::AutoMenu*);
	signals:
		void signalFollowObject( const Udb::Obj& );
	protected slots:
		void onDbUpdate( Udb::UpdateInfo info );
		void onTitleClick();
		void onFollowObject( const Udb::Obj& );
		void onShowItem();
	private:
		ObjectTitleFrame* d_title;
		Oln::RefByItemMdl* d_mdl;
		CheckLabel* d_pin;
	};
}

#endif // REFBYVIEWCTRL_H