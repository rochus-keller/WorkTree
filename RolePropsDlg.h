#ifndef __WtRolePropsDlg__
#define __WtRolePropsDlg__

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

class QLineEdit;
class QPushButton;
class QLabel;

namespace Wt
{
	class RolePropsDlg : public QDialog
	{
		Q_OBJECT
	public:
		RolePropsDlg( QWidget*, Udb::Transaction* txn );
		void loadFrom( Udb::Obj& );
		void saveTo( Udb::Obj& ) const;
		bool edit( Udb::Obj& );
	protected slots:
		void onSelAssig();
		void onClearAssig();
		void onSelDeputy();
		void onClearDeputy();
	private:
		QLineEdit* d_title;
		QLabel* d_assig;
		QLabel* d_deputy;
		QPushButton* d_selAssig;
		QPushButton* d_clearAssig;
		QPushButton* d_selDeputy;
		QPushButton* d_clearDeputy;
		Udb::Obj d_pa;
		Udb::Obj d_pd;
        Udb::Transaction* d_txn;
	};
}

#endif
