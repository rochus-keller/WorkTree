#include "PdmStream.h"
#include <QSet>
#include <Oln2/OutlineStream.h>
#include "PdmItemObj.h"
#include "WtTypeDefs.h"
using namespace Wt;
using namespace Stream;

const char* PdmStream::s_format = "application/worktree/pdm-stream";

void PdmStream::exportNetwork( const Oln::AttributeSet* as, Stream::DataWriter& out, const Udb::Obj& proc )
{
	if( proc.isNull() )
		return;
	out.writeSlot( Stream::DataCell().setAscii( "WorkTreeStream" ) );
	out.writeSlot( Stream::DataCell().setAscii( "0.1" ) );
	out.writeSlot( Stream::DataCell().setDateTime( QDateTime::currentDateTime() ) );
	writeObjTo( as, out, proc );
}

Udb::Obj PdmStream::importNetwork( const Oln::AttributeSet* as, Stream::DataReader& in, Udb::Obj& parent )
{
	d_error.clear();
	if( in.nextToken() != DataReader::Slot || in.getValue().getArr() != "FlowLineStream" )
	{
		d_error = "invalid data stream";
		return Udb::Obj();
	}
	if( in.nextToken() != DataReader::Slot || in.getValue().getArr() != "0.1" )
	{
		d_error = "invalid stream version";
		return Udb::Obj();
	}
	if( in.nextToken() != DataReader::Slot || !in.getValue().isDateTime() )
	{
		d_error = "invalid protocol";
		return Udb::Obj();
	}
	if( in.nextToken() != DataReader::BeginFrame || !in.getName().getTag().equals( "proc" ) )
	{
		d_error = "invalid protocol";
		return Udb::Obj();
	}
	return readItem( as, in, parent );
}

void PdmStream::writeAtts( const Oln::AttributeSet* as, Stream::DataWriter& out, const Udb::Obj& oln )
{
	DataCell v;
	out.writeSlot( oln, NameTag( "oid" ) ); // Darauf nimmt dann ControlFlow Bezug

	v = oln.getValue( AttrText );
	if( v.hasValue() )
		out.writeSlot( v, NameTag( "text" ) );
	v = oln.getValue( Atoms::attrIdent );
	if( v.hasValue() )
		out.writeSlot( v, NameTag( "id" ) );
	v = oln.getValue( AttrPosX );
	if( v.hasValue() )
		out.writeSlot( v, NameTag( "posx" ), true ); 
	v = oln.getValue( AttrPosY );
	if( v.hasValue() )
		out.writeSlot( v, NameTag( "posy" ), true ); 
	v = oln.getValue( as->alias );
	if( v.hasValue() )
		out.writeSlot( Stream::DataCell().setUuid( oln.getValueAsObj( as->alias ).getUuid() ), NameTag( "ali" ), true ); 
}

void PdmStream::writeListTo( const Oln::AttributeSet* as, Stream::DataWriter& out, const QList<Udb::Obj>& l )
{
	QSet<Udb::Obj> objs;
	QSet<Udb::Obj> flows;
	for( int i = 0; i < l.size(); i++ )
	{
		const quint32 type = l[i].getType();
		if( type == Atoms::typePdbTask ||
			type == Atoms::typePdbMilestone )
		{
			objs.insert( l[i] );
			// Finde nun alle Flows des Parent
			Udb::Obj sub = l[i].getParent().getFirstObj();
			if( !sub.isNull() ) do
			{
				if( sub.getType() == Atoms::typePdbLink )
					flows.insert( sub );
			}while( sub.next() );
		}
	}
	QSet<Udb::Obj>::const_iterator j;
	for( j = objs.begin(); j != objs.end(); ++j )
		writeObjTo( as, out, *j );

	for( j = flows.begin(); j != flows.end(); ++j )
	{
		Udb::Obj from = (*j).getValueAsObj( Atoms::attrPredecessor );
		Udb::Obj to = (*j).getValueAsObj( Atoms::attrSuccessor );
		if( objs.contains( from ) && objs.contains( to ) )
		{
			out.startFrame( NameTag( "flow" ) );
			out.writeSlot( from, NameTag( "from" ) );
			out.writeSlot( to, NameTag( "to" ) );
			out.writeSlot( (*j).getValue( Atoms::attrPointList ), NameTag( "nlst" ) );
			out.endFrame();
		}
	}
}

void PdmStream::writeObjTo( const Oln::AttributeSet* as, Stream::DataWriter& out, const Udb::Obj& obj )
{
	const quint32 type = obj.getType();
	if( type == Atoms::typePdbTask )
		out.startFrame( NameTag( "func" ) );
	else if( type == Atoms::typePdbMilestone )
		out.startFrame( NameTag( "evt" ) );
	else
		return;
	writeAtts( as, out, obj );

	Oln::OutlineStream olnout;
	//Oln::AttributeSet attr;
	//TypeDefs::fill( &attr );

	Udb::Obj sub = obj.getFirstObj();
	if( !sub.isNull() ) do
	{
		if( sub.getType() == as->typeItem )
			olnout.writeTo( as, out, sub );
		else
			writeObjTo( as, out, sub );
	}while( sub.next() );
	sub = obj.getFirstObj();
	if( !sub.isNull() ) do
	{
		if( sub.getType() == Atoms::typePdbLink )
		{
			out.startFrame( NameTag( "flow" ) );
			out.writeSlot( sub.getValue( Atoms::attrPredecessor ), NameTag( "from" ) );
			out.writeSlot( sub.getValue( Atoms::attrSuccessor ), NameTag( "to" ) );
			out.writeSlot( sub.getValue( Atoms::attrPointList ), NameTag( "nlst" ) );
			out.endFrame();
		}
	}while( sub.next() );

	out.endFrame();
}

static inline Udb::Obj _create( const NameTag& n, Udb::Obj& p )
{
	quint32 type = 0;
	if( n.equals( "func" ) )
		type = Atoms::typePdbTask;
	else if( n.equals( "evt" ) )
		type = Atoms::typePdbMilestone;
	else
		return Udb::Obj();
	return PdmItemObj::create( p, type );
}

Udb::Obj PdmStream::readFlow( Stream::DataReader& in, Udb::Obj& parent, const QHash<quint64,Udb::Obj>& objs )
{
	Q_ASSERT( in.getCurrentToken() == Stream::DataReader::BeginFrame && in.getName().getTag().equals("flow") );
	Q_ASSERT( !parent.isNull() );
	// Voraussetzung: 'in' sitzt auf einem BeginFrame, das nicht ein Flow ist. 
	Stream::DataReader::Token t = in.nextToken();
	Udb::Obj from;
	Udb::Obj to;
	Stream::DataCell bml;
	while( t == Stream::DataReader::Slot )
	{
		const NameTag n = in.getName().getTag();
		if( n.equals( "from" ) )
			from = objs.value( in.getValue().getOid() );
		else if( n.equals( "to" ) )
			to = objs.value( in.getValue().getOid() );
		else if( n.equals( "nlst" ) )
			bml = in.getValue();
		t = in.nextToken();
	}
	if( t != Stream::DataReader::EndFrame )
	{
		d_error = QString( "invalid format" );
		return Udb::Obj();
	}
	if( !from.isNull() && !to.isNull() )
	{
		Udb::Obj obj = PdmItemObj::create( parent, Atoms::typePdbLink );
		obj.setValue( Atoms::attrPredecessor, from );
		obj.setValue( Atoms::attrSuccessor, to );
		obj.setValue( Atoms::attrPointList, bml );
		return obj;
	}
	return Udb::Obj();
}

Udb::Obj PdmStream::readItem( const Oln::AttributeSet* as, Stream::DataReader& in, Udb::Obj& parent, quint64* oid )
{
	Q_ASSERT( !parent.isNull() );
	Q_ASSERT( in.getCurrentToken() == Stream::DataReader::BeginFrame && !in.getName().getTag().equals("flow") );
	Udb::Obj obj = _create( in.getName().getTag(), parent );
	if( obj.isNull() )
	{
		d_error = QString( "cannot create object for frame '%1'" ).arg( in.getName().toString() );
		return Udb::Obj();
	}
	QHash<quint64,Udb::Obj> subs;
	Oln::OutlineStream olns;
	//Oln::AttributeSet attr;
	//TypeDefs::fill( &attr );
	Stream::DataReader::Token t = in.nextToken( true ); // Schaue eine Stelle voraus
	// t kann sein: Slot, EndFrame, Start von Embedded oder Outline
	while( Stream::DataReader::isUseful( t ) )
	{
		switch( t )
		{
		case Stream::DataReader::Slot:
			{
				in.nextToken(); // bestätige nextToken
				const NameTag n = in.getName().getTag();
				if( n.equals( "text" ) )
					obj.setValue( AttrText, in.getValue() );
				else if( n.equals( "id" ) )
				{
					if( !obj.hasValue( Atoms::attrIdent ) )
						obj.setValue( Atoms::attrIdent, in.getValue() );
				}else if( n.equals( "oid" ) && oid )
					*oid = in.getValue().getOid();
				else if( n.equals( "posx" ) )
					obj.setValue( AttrPosX, in.getValue() );
				else if( n.equals( "posy" ) )
					obj.setValue( AttrPosY, in.getValue() );
				else if( n.equals( "ali" ) )
				{
					Udb::Obj a = parent.getTxn()->getObject( in.getValue() );
					// TODO: was wenn a Teil des Streams ist und noch nicht instanziiert?
					obj.setValue( as->alias, a );
				}
			}
			break;
		case Stream::DataReader::EndFrame:
			in.nextToken(); // bestätige nextToken
			return obj;
		case Stream::DataReader::BeginFrame:
			{
				const NameTag n = in.getName().getTag();
				if( n.equals( "oln" ) )
				{
					// olns.readFrom konsumiert final das Token
					Udb::Obj oln = olns.readFrom( as, in, obj.getTxn(), obj.getOid() );
					if( oln.isNull() )
					{
						d_error = QString( "invalid embedded outline" );
						return Udb::Obj();
					}
					oln.aggregateTo( obj );
				}else if( n.equals( "flow" ) )
				{
					in.nextToken(); // bestätige nextToken
					readFlow( in, obj, subs );
				}else
				{
					d_error = QString( "invalid frame '%1' in oid=%2" ).arg( n.toString() ).arg( *oid );
					return Udb::Obj();
				}
			}
			break;
		} // switch
		t = in.nextToken(true);
	} // while( isUseful() )
	d_error = QString( "protocol error" );
	return Udb::Obj();
}

QList<Udb::Obj> PdmStream::readFrom( const Oln::AttributeSet* as, Stream::DataReader& in, Udb::Obj& parent )
{
	d_error.clear();
	Q_ASSERT( !parent.isNull() );
	QHash<quint64,Udb::Obj> subs;
	Stream::DataReader::Token t = in.nextToken();
	QList<Udb::Obj> res;
	while( t == Stream::DataReader::BeginFrame )
	{
		const NameTag n = in.getName().getTag();
		if( n.equals( "flow" ) )
		{
			Udb::Obj flow = readFlow( in, parent, subs );
			if( !flow.isNull() )
				res.append( flow );
		}else  // if( !n.equals( "flow" ) )
		{
			quint64 suboid = 0;
			Udb::Obj sub = readItem( as, in, parent, &suboid );
			if( sub.isNull() || suboid == 0 )
				return QList<Udb::Obj>();
			subs[ suboid ] = sub;
			res.append( sub );
		} // if( n.equals( "flow" ) )
		t = in.nextToken(); 
	} // while( t == Stream::DataReader::BeginFrame )
	return res;
}
