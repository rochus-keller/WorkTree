#ifndef ASSIGVIEWMDL_H
#define ASSIGVIEWMDL_H

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

#include <QAbstractItemModel>
#include <Udb/Obj.h>

namespace Wt
{
    class AssigViewMdl : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit AssigViewMdl(QObject *parent = 0);
        ~AssigViewMdl();

        void setObject( const Udb::Obj& );
		const Udb::Obj& getObject() const { return d_object; }
		Udb::Obj getObject( const QModelIndex&, bool useCol = false ) const;
		QModelIndex getIndex( const Udb::Obj& ) const;
		void refill();
		//static void copyToClipboard( const QList<Udb::Obj>& );
		//QList<Udb::Obj> pasteFromClipboard();

		// overrides
		int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 2; }
		QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
		QModelIndex parent ( const QModelIndex & index ) const;
		int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
        QStringList mimeTypes () const;
        Qt::DropActions supportedDropActions () const;
        Qt::ItemFlags flags ( const QModelIndex & index ) const;
    signals:
        void signalLinkTo( const QList<Udb::Obj>& what, const QModelIndex & to, int row );
    private:
		struct Slot
		{
			Udb::Obj d_assig;
			Udb::Obj d_principal;
		};
		QModelIndex getIndex( Slot* ) const;
		static int findLowerBound(const QList<Slot*>& l, const QString& value );
		QList<Slot*> d_slots;
		Udb::Obj d_object;
    };
}

#endif // ASSIGVIEWMDL_H
