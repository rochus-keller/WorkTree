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

#include "Funcs.h"
#include <Udb/Transaction.h>
#include <Udb/ContentObject.h>
#include <Udb/Database.h>
#include <Oln2/OutlineItem.h>
using namespace Wt;

static QString _dummyGetText( const Udb::Obj & o, bool decorate )
{
    Q_UNUSED(decorate);
    return QString("<text>");
}

QString (*Funcs::getText)(const Udb::Obj & o, bool decorate) = _dummyGetText;

static bool _dummyIsAggregate( quint32 parent, quint32 child ) { return true; }

bool (*Funcs::isValidAggregate)( quint32 parent, quint32 child ) = _dummyIsAggregate;

static Udb::Obj _create(quint32 type, Udb::Obj parent, const Udb::Obj& before )
{
    return parent.createAggregate( type, before );
}

Udb::Obj (*Funcs::createObject)(quint32 type, Udb::Obj parent, const Udb::Obj& before ) = _create;

static void _setText(Udb::Obj &o, const QString &str) { o.setString( Udb::ContentObject::AttrText, str ); }

void (*Funcs::setText)(Udb::Obj &o, const QString &str) = _setText;

static void _moveTo(Udb::Obj &o, Udb::Obj &newParent, const Udb::Obj &before) { o.aggregateTo( newParent, before ); }

void (*Funcs::moveTo)(Udb::Obj &o, Udb::Obj &newParent, const Udb::Obj &before) = _moveTo;

static void _erase(Udb::Obj &o) { o.erase(); }

void (*Funcs::erase)(Udb::Obj &o) = _erase;

static QString _getName( quint32 atom, bool, Udb::Transaction* txn ) { return txn->getDb()->getAtomString( atom ); }

QString (*Funcs::getName)( quint32 atom, bool isType, Udb::Transaction* txn ) = _getName;

static QString _formatValue( Udb::Atom a, const Udb::Obj& o, bool ) { return o.getString(a); }

QString (*Funcs::formatValue)( Udb::Atom, const Udb::Obj&, bool useIcons ) = _formatValue;

static Udb::Obj _getAlias(const Udb::Obj& o) { return o.getValueAsObj( Oln::OutlineItem::AttrAlias ); }

Udb::Obj (*Funcs::getAlias)(const Udb::Obj&) = _getAlias;

