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

#include "AssigViewMdl.h"
#include "WtTypeDefs.h"
#include <Udb/Idx.h>
#include <Udb/Extent.h>
#include <Udb/Database.h>
#include <Udb/Transaction.h>
#include <Oln2/OutlineUdbMdl.h>
using namespace Wt;

AssigViewMdl::AssigViewMdl(QObject *parent) :
    QAbstractItemModel(parent)
{
}

AssigViewMdl::~AssigViewMdl()
{
	foreach( Slot* s, d_slots )
		delete s;
}

void AssigViewMdl::setObject( const Udb::Obj& r )
{
	//if( !d_target.isNull() )
	//	d_target.getDb()->removeObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ) );
	d_object= r;
	refill();
	//if( !d_target.isNull() )
	//	d_target.getDb()->addObserver( this, SLOT( onDbUpdate( Udb::UpdateInfo ) ), false );

}

void AssigViewMdl::refill()
{
	foreach( Slot* s, d_slots )
		delete s;
	d_slots.clear();
	reset();
	if( d_object.isNull() )
		return;
	Udb::Idx idx( d_object.getTxn(), IndexDefs::IdxAssigObject );
	if( idx.seek( d_object ) ) do
	{
		Udb::Obj assig = d_object.getObject( idx.getOid() );
		Udb::Obj subject = assig.getValueAsObj( AttrAssigPrincipal );
		if( !subject.isNull() )
		{
			const int to = findLowerBound( d_slots, subject.getString( AttrText ) );
			Slot* s = new Slot();
			s->d_assig = assig;
			s->d_principal = subject;
			d_slots.insert( to, s );
		}// TODO: else sollte assig gleich geoelscht werden!
	}while( idx.nextKey() );
	reset();
	// TODO getTree()->resizeColumnToContents(0);
}

QModelIndex AssigViewMdl::parent ( const QModelIndex & index ) const
{
	return QModelIndex();
}

int AssigViewMdl::rowCount ( const QModelIndex & parent ) const
{
	if( !parent.isValid() )
		return d_slots.size();
	else
		return 0;
}

QModelIndex AssigViewMdl::index ( int row, int column, const QModelIndex & parent ) const
{
	if( !parent.isValid() && !d_slots.isEmpty() )
	{
		Q_ASSERT( row < d_slots.size() );
		return createIndex( row, column, d_slots[row] );
	}else
		return QModelIndex();
}

QVariant AssigViewMdl::data ( const QModelIndex & index, int role ) const
{
	if( !index.isValid() || d_object.isNull() )
		return QVariant();
	Slot* s = static_cast<Slot*>( index.internalPointer() );
	Q_ASSERT( s != 0 );
	switch( role )
	{
	case Qt::DisplayRole:
	case Qt::ToolTipRole:
		switch( index.column() )
		{
		case 0:
			if( role == Qt::ToolTipRole )
				return WtTypeDefs::prettyName( s->d_assig.getType() );
			else
				switch( s->d_assig.getType() )
				{
				case TypeRasciAssig:
					return WtTypeDefs::formatRasciRole( s->d_assig.getValue( AttrRasciRole ).getUInt8() );
//				case TypeWorkAssig:
//					return QString("%1%").arg(s->d_assig.getValue( AttrPercentage ).getUInt8() );
				}
			break;
		case 1:
			return s->d_principal.getString( AttrText );
		}
		break;
	case Qt::DecorationRole:
		switch( index.column() )
		{
		case 0:
			return Oln::OutlineUdbMdl::getPixmap( s->d_assig.getType() );
		case 1:
			return Oln::OutlineUdbMdl::getPixmap( s->d_principal.getType() );
		}
		break;
	}
	return QVariant();
}

QModelIndex AssigViewMdl::getIndex( Slot* s ) const
{
	if( s == 0 )
		return QModelIndex();
	else
		return createIndex( d_slots.indexOf( s ), 0, s );
}

Udb::Obj AssigViewMdl::getObject( const QModelIndex& i, bool useCol ) const
{
	if( i.isValid() )
	{
		Slot* s = static_cast<Slot*>( i.internalPointer() );
		Q_ASSERT( s != 0 );
		if( useCol && i.column() > 0 )
			return s->d_principal;
		else
			return s->d_assig;
	}else
		return d_object;
}

int AssigViewMdl::findLowerBound(const QList<Slot*>& l, const QString& value )
{
    int middle;
    int n = l.size();
	int begin = 0;
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
		if ( l[middle]->d_principal.getString( AttrText ).compare( value, Qt::CaseInsensitive ) < 0 )
		{
            begin = middle + 1;
            n -= half + 1;
        } else {
            n = half;
        }
    }
    return begin;
}

QModelIndex AssigViewMdl::getIndex( const Udb::Obj& o ) const
{
	for( int i = 0; i < d_slots.size(); i++ )
		if( d_slots[i]->d_assig.equals( o ) || d_slots[i]->d_principal.equals( o ) )
			return index( i, 0 );
    return QModelIndex();
}

Qt::DropActions AssigViewMdl::supportedDropActions() const
{
    return Qt::DropActions( Qt::MoveAction | Qt::CopyAction );
}

QStringList AssigViewMdl::mimeTypes () const
{
    return QStringList() << QLatin1String( Udb::Obj::s_mimeObjectRefs );
}

bool AssigViewMdl::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parentIndex )
{
    if( action == Qt::MoveAction || action == Qt::CopyAction )
    {
        QList<Udb::Obj> objs = Udb::Obj::readObjectRefs( data, d_object.getTxn() );
        if( objs.isEmpty() )
            return false;
        emit signalLinkTo( objs, parentIndex, row );
        return true;
    }
    return false;
}

Qt::ItemFlags AssigViewMdl::flags( const QModelIndex & index ) const
{
	if( !index.isValid() )
		return Qt::ItemIsDropEnabled;
	Slot* s = static_cast<Slot*>( index.internalPointer() );
	Q_ASSERT( s != 0 );
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
}


