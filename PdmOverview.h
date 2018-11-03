#ifndef __Wt_PdmOverview__
#define __Wt_PdfOverview__

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
	class PdmItemView;

	class PdmOverview : public QGraphicsView
	{
		Q_OBJECT
	public:
		enum Mode { Idle, PrepareSelect, Select };
		PdmOverview( QWidget* );
		void setObserved( PdmItemView* );
	protected slots:
		void onSceneRectChanged();
		void onScrolled();
	protected:
		QRectF getSubjectRect() const;
		//QPointF myMapFromScene(const QPointF &point) const;
		// overrides
		void mousePressEvent ( QMouseEvent * );
		void mouseMoveEvent ( QMouseEvent * );
		void mouseReleaseEvent ( QMouseEvent * );
		void mouseDoubleClickEvent ( QMouseEvent * );
		void resizeEvent ( QResizeEvent * );
		void paintEvent ( QPaintEvent * );
	private:
		QRectF d_scene;
		PdmItemView* d_subject;
		Mode d_mode;
		QRect d_rect;
		//Q_DECLARE_PRIVATE(QGraphicsView)
	};
}

#endif
