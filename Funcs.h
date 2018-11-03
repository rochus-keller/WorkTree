#ifndef STATICS_H
#define STATICS_H

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
    struct Funcs
    {
		static QString (*getName)( quint32 atom, bool isType, Udb::Transaction* txn );
        static QString (*formatValue)( Udb::Atom, const Udb::Obj&, bool useIcons );
        static Udb::Obj (*getAlias)(const Udb::Obj&);
        static QString (*getText)(const Udb::Obj & o, bool decorate );
        static void (*setText)(Udb::Obj &o, const QString &str);
        static bool (*isValidAggregate)( quint32 parent, quint32 child );
        static Udb::Obj (*createObject)(quint32 type, Udb::Obj parent, const Udb::Obj& before );
        static void (*moveTo)(Udb::Obj &o, Udb::Obj &newParent, const Udb::Obj &before);
        static void (*erase)(Udb::Obj &o);

    };
}

#endif // STATICS_H
