#ifndef GenericMDL_H
#define GenericMDL_H

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

#include <QtCore/QAbstractItemModel>
#include <QtCore/QHash>
#include <Udb/Obj.h>

namespace Wt
{
    class GenericMdl : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit GenericMdl(QObject *parent = 0);
        void setRoot( const Udb::Obj& root );
        const Udb::Obj& getRoot() const { return d_root.d_obj; }
        Udb::Obj getObject( const QModelIndex& ) const;
		QModelIndex getIndex( const Udb::Obj& ) const;
		static QUrl objToUrl(const Udb::Obj & o);
		static void writeObjectUrls(QMimeData *data, const QList<Udb::Obj> & objs );
		bool isReadOnly() const;

        // overrides
		int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 1; }
		QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
		QModelIndex parent ( const QModelIndex & index ) const;
		int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
		Qt::ItemFlags flags ( const QModelIndex & index ) const;
		bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
		Qt::DropActions supportedDragActions () const;
		Qt::DropActions supportedDropActions () const;
		QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		QStringList mimeTypes () const;
		bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    signals:
        void signalNameEdited( const QModelIndex & index, const QVariant & value );
        void signalMoveTo( const QList<Udb::Obj>& what, const QModelIndex & to, int row );
    protected slots:
        void onDbUpdate( Udb::UpdateInfo );
    protected:
        void addItem( quint64 oid, quint64 parent, quint64 before );
		void removeItem( quint64 oid );
        void focusOnLastHit( bool edit = false );
		QString formatHierarchy( Udb::Obj, bool fragment = false ) const;
		// to override
        virtual bool isSupportedType( quint32 );
		virtual QVariant data( const Udb::Obj&, int section, int role ) const;
    private:
        struct Slot
		{
			Udb::Obj d_obj;
			QList<Slot*> d_children;
			Slot* d_parent;
			Slot(Slot* p = 0):d_parent(p){ if( p ) p->d_children.append(this); }
			~Slot() { foreach( Slot* s, d_children ) delete s; }
			void fillSubs();
		};
        void fillSubs( Slot* );
        void recursiveRemove( Slot* s );
		QModelIndex getIndex( Slot* ) const;
        QHash<quint32,Slot*> d_cache;
        Slot d_root;
    };
}

#endif // GenericMDL_H
