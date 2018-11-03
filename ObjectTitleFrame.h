#ifndef OBJECTTITLEFRAME_H
#define OBJECTTITLEFRAME_H

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

#include <QFrame>
#include <Udb/Obj.h>

namespace Wt
{
    class ObjectTitleFrame : public QFrame
    {
        Q_OBJECT
    public:
        explicit ObjectTitleFrame(QWidget *parent = 0);
        void adjustFont();
		void setObj( const Udb::Obj& );
		const Udb::Obj& getObj() const { return d_obj; }
    signals:
        void signalDblClicked();
        void signalClicked();
	protected:
		QSize sizeHint () const;
		void paintEvent ( QPaintEvent * event );
        void mouseDoubleClickEvent ( QMouseEvent * event );
        void mousePressEvent ( QMouseEvent * event );
	private:
		Udb::Obj d_obj;
    };
}

#endif // OBJECTTITLEFRAME_H
