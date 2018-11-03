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

#include "GenericMdl.h"
#include <Udb/Transaction.h>
#include <Udb/Idx.h>
#include <Udb/Database.h>
#include <Udb/ContentObject.h>
#include <Oln2/OutlineUdbMdl.h>
#include <QtCore/QMimeData>
#include <QtCore/QtDebug>
#include "Funcs.h"
using namespace Wt;

GenericMdl::GenericMdl(QObject *parent) :
    QAbstractItemModel(parent)
{
}

void GenericMdl::setRoot(const Udb::Obj &root)
{
    if( d_root.d_obj.equals( root ) )
		return;
	foreach( Slot* s, d_root.d_children )
		delete s;
	d_root.d_children.clear();
	reset();
	if( !d_root.d_obj.isNull() )
		d_root.d_obj.getDb()->removeObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );
	d_root.d_obj = root;
	fillSubs( &d_root );
	reset();
	if( !d_root.d_obj.isNull() )
		d_root.d_obj.getDb()->addObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ), false );
}

void GenericMdl::fillSubs( Slot* p )
{
	if( p->d_obj.isNull() )
		return;
	Udb::Obj e = p->d_obj.getFirstObj();
	if( !e.isNull() ) do
	{
		if( isSupportedType( e.getType() ) )
		{
            Slot* s = new Slot();
            s->d_parent = p;
            p->d_children.append( s );
            s->d_obj = e;
            d_cache[ e.getOid() ] = s;
            fillSubs( s );
		}
	}while( e.next() );
}

QModelIndex GenericMdl::parent ( const QModelIndex & index ) const
{
	if( index.isValid() )
	{
		Slot* s = static_cast<Slot*>( index.internalPointer() );
		Q_ASSERT( s != 0 );
		if( s->d_parent == &d_root )
			return QModelIndex();
		// else
		Q_ASSERT( s->d_parent != 0 );
		Q_ASSERT( s->d_parent->d_parent != 0 );
		return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
	}else
		return QModelIndex();
}

int GenericMdl::rowCount ( const QModelIndex & parent ) const
{
	if( parent.isValid() )
	{
		Slot* s = static_cast<Slot*>( parent.internalPointer() );
		Q_ASSERT( s != 0 );
		return s->d_children.size();
	}else
		return d_root.d_children.size();
}

QModelIndex GenericMdl::index ( int row, int column, const QModelIndex & parent ) const
{
    const Slot* s = &d_root;
	if( parent.isValid() )
	{
		s = static_cast<Slot*>( parent.internalPointer() );
		Q_ASSERT( s != 0 );
    }
	if( row < s->d_children.size() && column < columnCount( parent ) )
        return createIndex( row, column, s->d_children[row] );
	else
        return QModelIndex();
}

static QString _number( const QModelIndex & index )
{
	if( !index.isValid() )
		return QString();
	QString lhs = _number( index.parent() );
	if( !lhs.isEmpty() )
		return lhs + QLatin1Char( '.' ) + QString::number( index.row() + 1 );
	else
		return QString::number( index.row() + 1 );
}

QVariant GenericMdl::data ( const QModelIndex & index, int role ) const
{
	if( !index.isValid() || d_root.d_obj.isNull() )
		return QVariant();
	Slot* s = static_cast<Slot*>( index.internalPointer() );
	Q_ASSERT( s != 0 );
	return data( s->d_obj, index.column(), role );
}

QModelIndex GenericMdl::getIndex( Slot* s ) const
{
	if( s == 0 || s->d_parent == 0 )
		return QModelIndex();
	else
		return createIndex( s->d_parent->d_children.indexOf( s ), 0, s );
}

Udb::Obj GenericMdl::getObject( const QModelIndex& i ) const
{
	if( i.isValid() )
	{
		Slot* s = static_cast<Slot*>( i.internalPointer() );
		Q_ASSERT( s != 0 );
		return s->d_obj;
	}else
		return d_root.d_obj;
}

QModelIndex GenericMdl::getIndex( const Udb::Obj& o ) const
{
	if( o.isNull() )
		return QModelIndex();
	Slot* s = d_cache.value( o.getOid() );
	if( s )
		return getIndex( s );
	else
		return QModelIndex();
}

QUrl GenericMdl::objToUrl(const Udb::Obj &o)
{
	if( o.isNull() )
		return QUrl();
	// URI-Schema gem. http://en.wikipedia.org/wiki/URI_scheme und http://tools.ietf.org/html/rfc3986 bzw. http://tools.ietf.org/html/rfc6068
	// Format: xoid:49203@348c2483-206a-40a6-835a-9b9753b60188?txt=MO1425 Dies ist ein Text
	QUrl url = Udb::Obj::objToUrl( o );
	url.setQueryDelimiters( '=', ';' );
	QList<QPair<QString, QString> > items;
	items.append( qMakePair( QString("txt"), Funcs::getText( o, true ) ) );
	url.setQueryItems( items );
	return url;
}

void GenericMdl::onDbUpdate( Udb::UpdateInfo info )
{
	if( d_root.d_obj.isNull() )
		return;
	switch( info.d_kind )
	{
	case Udb::UpdateInfo::Aggregated:
		addItem( info.d_id, info.d_parent, info.d_before );
		break;
	case Udb::UpdateInfo::Deaggregated:
		removeItem( info.d_id );
		break;
	case Udb::UpdateInfo::ValueChanged:
    case Udb::UpdateInfo::TypeChanged:
        {
            Slot* s = d_cache.value( info.d_id );
            if( s != 0 )
            {
                QModelIndex i = getIndex( s );
                emit dataChanged( i, i );
            }
        }
		break;
	}
}

Qt::ItemFlags GenericMdl::flags( const QModelIndex & index ) const
{
	if( !index.isValid() )
		return Qt::ItemIsDropEnabled;
	Slot* s = static_cast<Slot*>( index.internalPointer() );
	Q_ASSERT( s != 0 );
	const Qt::ItemFlags basics = Qt::ItemIsSelectable | Qt::ItemIsEnabled
			| Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	if( index.column() == 0 )
		return basics | Qt::ItemIsEditable;
	else
		return basics;
}

bool GenericMdl::setData ( const QModelIndex & index, const QVariant & value, int role )
{
	if( !d_root.d_obj.isNull() && index.isValid() && index.column() == 0 &&  role == Qt::EditRole )
	{
        emit signalNameEdited( index, value );
        return true;
	}else
        return false;
}

Qt::DropActions GenericMdl::supportedDragActions() const
{
    return Qt::DropActions( Qt::MoveAction | Qt::LinkAction ); // Qt::CopyAction );
}

Qt::DropActions GenericMdl::supportedDropActions() const
{
    return Qt::DropActions( Qt::MoveAction );
}

QMimeData * GenericMdl::mimeData ( const QModelIndexList & idx ) const
{
    if( idx.isEmpty() || d_root.d_obj.isNull() )
        return 0;

	QMimeData* mimeData = new QMimeData();

    QList<Udb::Obj> objs;
    QStringList text;
	for( int i = 0; i < idx.size(); i++ )
	{
		Udb::Obj o = getObject( idx[i] );
		Q_ASSERT( !o.isNull() );
        objs.append( o );
        text.append(idx[i].data().toString() );
        //writeTo( o, out2 );
	}
    Udb::Obj::writeObjectRefs( mimeData, objs );
	writeObjectUrls( mimeData, objs );
    mimeData->setText( text.join("\n") );
	return mimeData;
}

bool GenericMdl::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parentIndex )
{
    if( action == Qt::MoveAction )
    {
        QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( data, d_root.d_obj.getTxn() );
        if( objs.isEmpty() )
            return false;
        emit signalMoveTo( objs, parentIndex, row );
        return true;
    }else if( action == Qt::CopyAction )
    {
//        if( row != -1 || column != -1 )
//            return false;
//        Slot* parentSlot = 0;
//        if( !parentIndex.isValid() )
//            parentSlot = &d_root;
//        else
//            parentSlot = static_cast<Slot*>( parentIndex.internalPointer() );
//        Q_ASSERT( parentSlot != 0 );
//        Stream::DataReader r( data->data(QLatin1String( s_mimeWbs )) );
//        Stream::DataReader::Token t = r.nextToken();
//        if( t == Stream::DataReader::BeginFrame && r.getName().getTag().equals( "wbs" ) )
//        {
//            Stream::DataReader::Token t = r.nextToken();
//            while( t == Stream::DataReader::BeginFrame )
//            {
//                readDispatch( r, parentSlot->d_obj );
//                t = r.nextToken();
//            }
//            d_root.d_obj.commit();
//            focusOnLastHit();
//            return true;
//        }else
            return false;
    }else
        return false;
}

QStringList GenericMdl::mimeTypes () const
{
    return QStringList() << QLatin1String( Udb::Obj::s_mimeObjectRefs );// << QLatin1String( s_mimeWbs );
}

void GenericMdl::addItem( quint64 oid, quint64 parent, quint64 before )
{
	Slot* parentSlot = 0;
	if( parent == d_root.d_obj.getOid() )
		parentSlot = &d_root;
	else
		parentSlot = d_cache.value( parent, 0 );
	if( parentSlot != 0 )
	{
        if( d_cache.contains( oid ) )
            return;
        // Wenn durch Paste ein ganzer Baum eingefgt wird, landet man hier pro rekursivem Element
        // obwohl fillSubs bereits alle eingefgt hat.
		Udb::Obj objToInsert = d_root.d_obj.getObject( oid );
		if( isSupportedType( objToInsert.getType() ) )
		{
            int beforeRow = parentSlot->d_children.size(); // Default: am Ende anfgen
            if( before != 0 )
            {
                Udb::Obj beforeObj = objToInsert.getObject( before );
                // Da beliebige Typen in Aggregat versammelt sind, spuhle vor bis zu einem brauchbaren.
                while( !beforeObj.isNull() && !isSupportedType( beforeObj.getType() ) )
                    beforeObj = beforeObj.getNext();
                Slot* beforeSlot = d_cache.value( beforeObj.getOid() );
                if( beforeSlot )
                {
                    Q_ASSERT( beforeSlot->d_parent == parentSlot );
                    beforeRow = parentSlot->d_children.indexOf( beforeSlot );
                }
            } // else am Ende einfgen
            beginInsertRows( getIndex( parentSlot ), beforeRow, beforeRow );
            Slot* s = new Slot();
            s->d_parent = parentSlot;
            parentSlot->d_children.insert( beforeRow, s );
            s->d_obj = objToInsert;
            d_cache[ oid ] = s;
            fillSubs( s );
            endInsertRows();
		}
	}
}

void GenericMdl::recursiveRemove( Slot* s )
{
	if( s == 0 )
		return;
    d_cache.remove( s->d_obj.getOid() );
	for( int i = 0; i < s->d_children.size(); i++ )
		recursiveRemove( s->d_children[i] );
}

void GenericMdl::removeItem( quint64 oid )
{
	Slot* s = d_cache.value( oid );
	if( s != 0 )
	{
		const int row = s->d_parent->d_children.indexOf( s );
        Q_ASSERT( row != -1 );
		beginRemoveRows( getIndex( s->d_parent ), row, row );
		s->d_parent->d_children.removeAt( row );
        recursiveRemove( s );
		delete s;
		endRemoveRows();
    }
}

bool GenericMdl::isSupportedType(quint32 type)
{
	return false;
}

QVariant GenericMdl::data(const Udb::Obj & o, int section, int role) const
{
	switch( section )
	{
	case 0:
		switch( role )
		{
		case Qt::DisplayRole:
			return Funcs::getText( o, true );
		case Qt::ToolTipRole:
			return formatHierarchy( o );
		case Qt::EditRole:
			return Funcs::getText( o, false );
		case Qt::DecorationRole:
			return Oln::OutlineUdbMdl::getPixmap( o.getType() );
		}
		break;
	}
	return QVariant();
}

QString GenericMdl::formatHierarchy(Udb::Obj o, bool fragment) const
{
    QList<Udb::Obj> path;
	while( !o.equals( d_root.d_obj ) && !o.getParent().isNull() )
    {
		path.prepend( o );
		o = o.getParent();
    }
    // formatObjectTitle( s->d_obj )
	QString html;
	if( !fragment )
		html = "<html>";
    for( int i = 0; i < path.size(); i++ )
    {
		html += tr("%1: <img src=\"%2\"/> ").
                arg(i+1,2,10,QChar('0')).
				arg(Oln::OutlineUdbMdl::getPixmapPath(path[i].getType()));
		if( i == path.size() - 1 )
			html += "<b>";
		html += Funcs::getText( path[i], true );
        if( i == path.size() - 1 )
			html += "</b>";
        else
            html += "<br>";
    }
	if( !fragment )
		html += "</html>";
	return html;
}

void GenericMdl::writeObjectUrls(QMimeData *data, const QList<Udb::Obj> &objs)
{
	if( objs.isEmpty() )
		return;
	QList<QUrl> urls;
	foreach( Udb::Obj o, objs )
		urls.append( objToUrl( o ) );
	if( !urls.isEmpty() )
		data->setUrls( urls );
}

bool GenericMdl::isReadOnly() const
{
	return d_root.d_obj.isNull() || d_root.d_obj.getTxn()->isReadOnly();
}
