#ifndef __Fln_FlowChartStream__
#define __Fln_FlowChartStream__

#include <Stream/DataReader.h>
#include <Stream/DataWriter.h>
#include <Udb/Obj.h>
#include <Udb/Transaction.h>

namespace Wt
{
	class PdmStream
	{
	public:
		static const char* s_format;
		/* External Format
			
				Slot <ascii> = "FlowLineStream"
				Slot <ascii> = "0.1"
				Slot <DateTime> = now
				[ Internal Format ]
		*/
		/* Internal Format (names as Tags)

		Sequence of
			Block
				Sequence of
					Frame 'proc' | 'note' | 'fram' | 'func' | 'evt' | 'conn'
						Slot 'oid' = <oid>
						Slot 'id' = <string>
						Slot 'text' = <string>
						Slot 'posx' = <double>
						Slot 'posy' = <double>
						Slot 'w'	= <double>
						Slot 'h'	= <double>
						Slot 'ctyp'	= <uint8>
						[ Slot 'ali' = <uuid>
						[ OutlineStream Internal Format ]

						[ Block ]* // falls proc, rekursiv

				Sequence of 
					Frame 'flow'
						Slot 'from' = <oid>
						Slot 'to' = <oid>
						Slot 'nlst' = <bml>						

		*/

		static void writeListTo( Stream::DataWriter&, const QList<Udb::Obj>& objs ); // kann Uuid erzeugen
		static void writeObjTo( Stream::DataWriter&, const Udb::Obj& objs ); // kann Uuid erzeugen
		static void exportNetwork( Stream::DataWriter&, const Udb::Obj& proc ); // kann Uuid erzeugen
		const QString& getError() const { return d_error; }
		QList<Udb::Obj> readFrom( Stream::DataReader&, Udb::Obj& to ); // Liste enthält Items und Flows!
		Udb::Obj importNetwork( Stream::DataReader&, Udb::Obj& parent );
	private:
		Udb::Obj readItem( Stream::DataReader&, Udb::Obj& parent, quint64* oid = 0 );
		Udb::Obj readFlow( Stream::DataReader&, Udb::Obj& parent, const QHash<quint64,Udb::Obj>& );
		static void writeAtts( Stream::DataWriter&, const Udb::Obj& oln );
		QString d_error;
	};
}

#endif
