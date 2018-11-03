#ifndef GENERICCTRL_H
#define GENERICCTRL_H

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

class QTreeView;
class QModelIndex;

namespace Wt
{
    class GenericMdl;

    class GenericCtrl : public QObject
    {
        Q_OBJECT
    public:
        explicit GenericCtrl(QTreeView *tree, GenericMdl* mdl );
        QTreeView* getTree() const;
        void setRoot( const Udb::Obj& root );
        Udb::Obj getSelectedObject( bool rootOtherwise = false ) const;
        void showObject( const Udb::Obj& );
    signals:
		void signalSelected( const Udb::Obj&);
        void signalDblClicked( const Udb::Obj&);
    public slots:
        void onExpandSelected();
        void onEditName();
        void onDeleteItems();
        void onMoveUp();
        void onMoveDown();
        void onAddNext();
        void onSelectionChanged();
        void onCopy();
    protected slots:
        void onNameEdited( const QModelIndex & index, const QVariant & value );
        void onMoveTo( const QList<Udb::Obj> &what, const QModelIndex & toParent, int beforeRow );
        void onDoubleClicked(const QModelIndex&);
		// to override
		virtual bool canDelete( const QList<Udb::Obj>& );
    protected:
        void add( quint32 type );
        void focusOn( const Udb::Obj&, bool edit = false);
		GenericMdl* getMdl() const { return d_mdl; }
    private:
        GenericMdl* d_mdl;
    };
}

#endif // GENERICCTRL_H
