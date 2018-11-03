#ifndef OBJECTHELPER_H
#define OBJECTHELPER_H

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

#include <Udb/Obj.h>

namespace Wt
{
    struct ObjectHelper
    {
        static Udb::Obj createObject( quint32 type, Udb::Obj parent, const Udb::Obj &before = Udb::Obj() );
        static quint32 getNextId( Udb::Transaction *, quint32 type );
        static QString getNextIdString( Udb::Transaction *, quint32 type );
        static void retypeObject( Udb::Obj& o, quint32 type ); // Prüft nicht, ob zulässig!
        static void moveTo( Udb::Obj& o, Udb::Obj& newParent, const Udb::Obj& before ); // Prüft nicht, ob zulässig!
        static void erase( Udb::Obj& o );
        static void setText( Udb::Obj& o, const QString& str );
		static void setMsType( Udb::Obj& ms, int msType );

		enum ShortestPathMethod { SpmNodeCount, SpmDuration, SpmOptimisticDur, SpmPessimisticDur,
								  SpmMostLikelyDur, SpmCriticalPath };
		static QList<Udb::Obj> findShortestPath( const Udb::Obj& start, const Udb::Obj& goal, ShortestPathMethod meth );
		static QList<Udb::Obj> findSuccessors( const Udb::Obj& item );
		static QList<Udb::Obj> findPredecessors( const Udb::Obj& item );
	};
}

#endif // OBJECTHELPER_H
