#ifndef __Wt_PdmItemView__
#define __Wt_PdmItemView__

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

#include <QGraphicsView>

namespace Wt
{
	class PdmItemMdl;
    class PdmCtrl;

	class PdmItemView : public QGraphicsView
	{
        Q_OBJECT
	public:
		enum Mode { Idle, Selecting, Extending, Scrolling, Clicking };
		PdmItemView( QWidget* );
		void scrollBy( int dx, int dy );
        PdmCtrl* getCtrl() const;
        PdmItemMdl* getMdl() const;
        Mode getMode() const { return d_mode; }
        bool isIdle() const { return d_mode == Idle; }
    signals:
        void signalDblClick( const QPoint& );
        void signalClick( const QPoint& );
	protected:
		// overrides
		void mousePressEvent ( QMouseEvent * );
		void mouseMoveEvent ( QMouseEvent * );
		void mouseReleaseEvent ( QMouseEvent * );
		void mouseDoubleClickEvent ( QMouseEvent * e );
		void paintEvent ( QPaintEvent * );
	private:
		QRect d_rubberRect;
		Mode d_mode;
	};
}

#endif
