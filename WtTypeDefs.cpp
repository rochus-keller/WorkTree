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

#include "WtTypeDefs.h"
#include <Udb/Obj.h>
#include <Udb/Transaction.h>
#include <Udb/Database.h>
#include <Txt/TextOutHtml.h>
#include <Oln2/OutlineUdbMdl.h>
#include <Oln2/OutlineUdbCtrl.h>
#include <CrossLine/DocTabWidget.h>
#include <QtCore/QMimeData>
#include <Udb/ContentObject.h>
#include <Oln2/OutlineItem.h>
#include "WorkTreeApp.h"
#include "Funcs.h"
#include "ObjectHelper.h"
#include "Indexer.h"
using namespace Wt;
using namespace Udb;

const char* IndexDefs::IdxIdent = "IdxIdent";
const char* IndexDefs::IdxAltIdent = "IdxAltIdent";
const char* IndexDefs::IdxItemLink = "IdxItemLink";
const char* IndexDefs::IdxOrigObject = "IdxOrigObject";
const char* IndexDefs::IdxPred = "IdxPred";
const char* IndexDefs::IdxSucc = "IdxSucc";
const char* IndexDefs::IdxPrincipalFirstName = "IdxLastNameFirstName";
const char* IndexDefs::IdxRoleAssig = "IdxRoleAssig";
const char* IndexDefs::IdxRoleDeputy = "IdxRoleDeputy";
const char* IndexDefs::IdxAssigObject = "IdxAssigObject";
const char* IndexDefs::IdxAssigPrincipal = "IdxAssigPrincipal";
const char* IndexDefs::IdxWbsRef = "IdxWbsRef";
const char* IndexDefs::IdxCalDate = "IdxCalDate";

static QString _fetchText(const Udb::Obj & o, bool decorate )
{
    if( decorate )
        return WtTypeDefs::formatObjectTitle( o );
    else
        return o.getString( AttrText );
}
static Udb::Obj _getAlias(const Udb::Obj& o) { return o.getValueAsObj( AttrItemLink ); }
static QString _getName( quint32 atom, bool isType, Udb::Transaction* txn ) { return WtTypeDefs::prettyName(atom,txn); }

void WtTypeDefs::init(Udb::Database &db)
{
	Oln::DocTabWidget::attrText = AttrText;
    Funcs::getText = _fetchText;
    Funcs::isValidAggregate = isValidAggregate;
    Funcs::createObject = ObjectHelper::createObject;
    Funcs::setText = ObjectHelper::setText;
    Funcs::moveTo = ObjectHelper::moveTo;
    Funcs::erase = ObjectHelper::erase;
    Funcs::formatValue = formatValue;
	Funcs::getName = _getName;
    Funcs::getAlias = _getAlias;
    Indexer::attrText = AttrText;
    Indexer::attrItemLink = AttrItemLink;
    Indexer::attrInternalId = AttrInternalId;
    Indexer::attrCustomId = AttrCustomId;
    Indexer::attrItemHome = AttrItemHome;
    Indexer::attrItemIsTitle = AttrItemIsTitle;
	Oln::OutlineItem::AttrAlias = AttrItemLink;
	Udb::ContentObject::AttrText = AttrText;
	Oln::OutlineItem::AttrIsTitle = AttrItemIsTitle;
	Oln::OutlineItem::AttrIsReadOnly = AttrItemIsReadOnly;
	Oln::OutlineItem::AttrIsExpanded = AttrItemIsExpanded;
	Oln::OutlineItem::AttrHome = AttrItemHome;
	Udb::ContentObject::AttrCreatedOn = AttrCreatedOn;
	Udb::ContentObject::AttrModifiedOn = AttrModifiedOn;
	Oln::OutlineItem::TID = TypeOutlineItem;
	Oln::Outline::TID = TypeOutline;
	Udb::ContentObject::AttrIdent = AttrCustomId;
	Udb::ContentObject::AttrAltIdent = AttrInternalId;

    Udb::Database::Lock lock( &db);

	db.presetAtom( "WtStart + 1000", WtEnd );

	if( db.findIndex( IndexDefs::IdxIdent ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrInternalId ) );
		db.createIndex( IndexDefs::IdxIdent, def );
	}
	if( db.findIndex( IndexDefs::IdxAltIdent ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrCustomId ) );
		db.createIndex( IndexDefs::IdxAltIdent, def );
	}
    if( db.findIndex( IndexDefs::IdxItemLink ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrItemLink ) );
		db.createIndex( IndexDefs::IdxItemLink, def );
	}
    if( db.findIndex( IndexDefs::IdxOrigObject ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrOrigObject ) );
		db.createIndex( IndexDefs::IdxOrigObject, def );
	}
    if( db.findIndex( IndexDefs::IdxPred ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrPred ) );
		db.createIndex( IndexDefs::IdxPred, def );
	}
    if( db.findIndex( IndexDefs::IdxSucc ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrSucc ) );
		db.createIndex( IndexDefs::IdxSucc, def );
	}
    if( db.findIndex( IndexDefs::IdxPrincipalFirstName ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrPrincipalName, IndexMeta::NFKD_CanonicalBase ) );
		def.d_items.append( IndexMeta::Item( AttrFirstName, IndexMeta::NFKD_CanonicalBase ) );
		db.createIndex( IndexDefs::IdxPrincipalFirstName, def );
	}
	if( db.findIndex( IndexDefs::IdxRoleAssig ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrRoleAssig ) );
		db.createIndex( IndexDefs::IdxRoleAssig, def );
	}
	if( db.findIndex( IndexDefs::IdxRoleDeputy ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrRoleDeputy ) );
		db.createIndex( IndexDefs::IdxRoleDeputy, def );
	}
    if( db.findIndex( IndexDefs::IdxAssigObject ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrAssigObject ) );
		db.createIndex( IndexDefs::IdxAssigObject, def );
	}
    if( db.findIndex( IndexDefs::IdxAssigPrincipal ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrAssigPrincipal ) );
		db.createIndex( IndexDefs::IdxAssigPrincipal, def );
	}
    if( db.findIndex( IndexDefs::IdxWbsRef ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
		def.d_items.append( IndexMeta::Item( AttrWbsRef ) );
		db.createIndex( IndexDefs::IdxWbsRef, def );
	}
    if( db.findIndex( IndexDefs::IdxCalDate ) == 0 )
	{
		IndexMeta def( IndexMeta::Value );
        def.d_items.append( IndexMeta::Item( IndexMeta::AttrParent ) );
		def.d_items.append( IndexMeta::Item( AttrCalDate ) );
		db.createIndex( IndexDefs::IdxCalDate, def );
	}
}

bool WtTypeDefs::isValidAggregate(quint32 parent, quint32 child)
{
    switch( parent )
    {
    case TypeIMP:
        switch( child )
        {
        case TypeImpEvent:
            return true;
        // Joker: bei engerer Validierung verbieten
        case TypeTask:
        case TypeMilestone:
            return true;
        }
        break;
    case TypeImpEvent:
        switch( child )
        {
        case TypeAccomplishment:
            return true;
        // Joker: bei engerer Validierung verbieten
        case TypeTask:
        case TypeMilestone:
            return true;
        }
        break;
    case TypeAccomplishment:
        switch( child )
        {
        case TypeCriterion:
            return true;
        // Joker: bei engerer Validierung verbieten
        case TypeTask:
        case TypeMilestone:
            return true;
        }
        break;
    case TypeCriterion:
        switch( child )
        {
        case TypeTask:
        case TypeMilestone:
        case TypePdmItem:
        case TypeRasciAssig:
            return true;
        }
        break;
    case TypeTask:
        switch( child )
        {
        case TypeTask:
        case TypeMilestone:
        case TypePdmItem:
        case TypeLink:
        case TypeRasciAssig:
            return true;
        }
        break;
    case TypeMilestone:
        switch( child )
        {
        case TypeLink:
        case TypeRasciAssig:
            return true;
        }
        break;
    case TypeOBS:
        switch( child )
        {
        case TypeOrgUnit:
        case TypeRole:
        case TypeHuman:
        case TypeUnitAssig:
        case TypeResource:
            return true;
        }
        break;
    case TypeOrgUnit:
        switch( child )
        {
        case TypeOrgUnit:
        case TypeRole:
        case TypeHuman:
        case TypeUnitAssig:
        case TypeResource:
            return true;
        }
        break;
    case TypeWBS:
        switch( child )
        {
        case TypeDeliverable:
        case TypeWork:
            return true;
        }
        break;
    case TypeDeliverable:
        switch( child )
        {
        case TypeDeliverable:
        case TypeWork:
        case TypeRasciAssig:
            return true;
        }
        break;
    case TypeWork:
        switch( child )
        {
        case TypeWork:
        case TypeRasciAssig:
            return true;
        }
        break;
    case TypeRootFolder:
    case TypeFolder:
        switch( child )
        {
        case TypeFolder:
        case TypePdmDiagram:
        case TypeOutline:
            return true;
        }
		if( child == Udb::ScriptSource::TID )
			return true;
        break;
    }
    return false;
}

QString WtTypeDefs::prettyName(quint32 type)
{
    switch( type )
    {
    case TypeIMP:
        return tr( "Integrated Master Plan" );
    case TypeTask:
        return tr( "Task" );
    case TypeMilestone:
        return tr( "Milestone" );
    case TypeImpEvent:
        return tr( "Event" );
    case TypeAccomplishment:
        return tr( "Accomplishment" );
    case TypeCriterion:
        return tr( "Criterion" );
	case AttrCreatedOn:
		return tr("Created On");
	case AttrModifiedOn:
		return tr("Modified On");
	case AttrText:
		return tr("Header/Text");
	case AttrInternalId:
		return tr("Internal ID");
	case AttrCustomId:
		return tr("Custom ID");
    case AttrEarlyStart:
        return tr("Early Start");
    case AttrEarlyFinish:
        return tr("Early Finish");
    case AttrLateStart:
        return tr("Late Start");
    case AttrLateFinish:
        return tr("Late Finish");
    case AttrTaskType:
        return tr("Task Type");
    case AttrMsType:
        return tr("Milestone Type");
    case TypeLink:
        return tr("Link");
    case AttrPred:
        return tr("Predecessor");
    case AttrSucc:
        return tr("Successor");
    case AttrLinkType:
        return tr("Link Type");
    case TypeOutlineItem:
        return tr("Outline Item");
    case TypeOutline:
        return tr("Outline");
    case TypeOrgUnit:
        return tr("OrgUnit");
    case TypeRole:
        return tr("Role");
    case TypeHuman:
        return tr("Person");
    case TypeResource:
        return tr("Resource");
    case TypeUnitAssig:
        return tr("Member");
    case TypeOBS:
        return tr("Organizational Breakdown Structure");
    case AttrFirstName:
		return tr("First Name");
	case AttrPrincipalName:
		return tr("Principal Name");
	case AttrQualiTitle:
		return tr("Title");
	case AttrOrganization:
		return tr("Organization");
	case AttrDepartment:
		return tr("Department");
	case AttrOffice:
		return tr("Office");
	case AttrJobTitle:
		return tr("Job Title");
	case AttrManager:
		return tr("Manager");
	case AttrAssistant:
		return tr("Assistant");
	case AttrPhoneDesk:
		return tr("Business Phone");
	case AttrPhoneMobile:
		return tr("Business Mobile");
	case AttrEmail:
		return tr("Business Email");
	case AttrMailAddr:
		return tr("Business Address");
	case AttrPhonePriv:
		return tr("Home Phone");
	case AttrMobilePriv:
		return tr("Private Mobile");
	case AttrMailAddrPriv:
		return tr("Home Address");
	case AttrEmailPriv:
		return tr("Home Email");
    case AttrRoleAssig:
		return tr("Official");
	case AttrRoleDeputy:
		return tr("Deputy");
    case AttrAssigObject:
        return tr("Assigned to");
    case AttrAssigPrincipal:
        return tr("Principal");
    case TypeRasciAssig:
        return tr("RASCI Assignment");
    case AttrRasciRole:
        return tr("RASCI Role");
    case TypeDeliverable:
        return tr("Deliverable");
    case TypeWork:
        return tr("Work");
    case TypeWBS:
        return tr("Works Breakdown Structure");
    case AttrDuration:
        return tr("Duration" );
    case AttrOptimisticDur:
        return tr("Optimistic Dur.");
    case AttrPessimisticDur:
        return tr("Pessimistic Dur.");
    case AttrMostLikelyDur:
        return tr("Most Likely Dur.");
    case AttrSubTMSCount:
        return tr("Summary Task");
    case AttrCriticalPath:
        return tr("Critical Path");
    case TypeFolder:
        return tr("Folder");
    case TypeRootFolder:
        return tr("Document Tree");
    case TypePdmDiagram:
        return tr("PDM Diagram");
    case AttrStartMs:
        return tr("Start Milestone");
    case AttrFinishMs:
        return tr("Finish Milestone");
    case AttrWbsRef:
        return tr("WBS Reference");
    case AttrPlannedValue:
        return tr("Planned Value (PV)");
    case AttrEarnedValue:
        return tr("Earned Value (EV)");
    case AttrActualCost:
        return tr("Actual Cost (AC)");
    case AttrCalendar:
    case TypeCalendar:
        return tr("Calendar");
    case AttrParentCalendar:
        return tr("Parent Calendar");
    case AttrNonWorkingDays:
        return tr("Workday Pattern");
    case TypeCalEntry:
        return tr("Calendar Entry");
    case AttrCalDate:
        return tr("Start Date");
    case AttrCalDuration:
        return tr("Duration");
    case AttrNonWorking:
        return tr("Nonworking");
    case AttrAvaility:
        return tr("Availability");
    case AttrCalEntryCount:
        return tr("Calendar Entries");

        // TEST
    case TypePdmItem:
        return tr("PDM Diagram Item");
    case AttrOrigObject:
        return tr("Original Object" );
    case AttrPosX:
        return tr("X Position");
    case AttrPosY:
        return tr("Y Position");
    case AttrPointList:
        return tr("Point List" );
//    case AttrSubTMSCount:
//        return tr("Summary Task");
    }
	if( type == Udb::ScriptSource::TID )
		return tr("Lua Script");
    return QString();
}

QString WtTypeDefs::prettyName( quint32 atom, Udb::Transaction* txn )
{
	if( atom < WtEnd )
		return prettyName( atom );
	else
	{
        Q_ASSERT( txn != 0 );
		return QString::fromLatin1( txn->getDb()->getAtomString( atom ) );
	}
}

QString WtTypeDefs::formatObjectTitle(const Udb::Obj & o)
{
    if( o.isNull() )
        return tr("<null>");
    Udb::Obj resolvedObj = o.getValueAsObj( AttrItemLink );
    if( resolvedObj.isNull() )
        resolvedObj = o;
    QString id = resolvedObj.getString( AttrCustomId, true );
    if( id.isEmpty() )
        id = resolvedObj.getString( AttrInternalId, true );
    QString name = resolvedObj.getString( AttrText, true );
    if( name.isEmpty() )
        name = prettyName( resolvedObj.getType() );

    if( id.isEmpty() )
#ifdef _DEBUG_GUGUS
        return QString("%1%2 %3").arg( QLatin1String("#" ) ).
            arg( o.getOid(), 3, 10, QLatin1Char('0' ) ).
            arg( name );
#else
        return name;
#endif
    else
        return QString("%1 %3").arg( id ).arg( name );
}

QString WtTypeDefs::formatObjectId(const Obj & o, bool useOid)
{
    if( o.isNull() )
        return tr("<null>");
    QString id = o.getString( AttrCustomId );
    if( id.isEmpty() )
        id = o.getString( AttrInternalId );
    if( id.isEmpty() && useOid )
        id = QString("#%1").arg( o.getOid() );
    return id;
}

QString WtTypeDefs::prettyDate( const QDate& d, bool includeDay, bool includeWeek )
{
	const char* s_dateFormat = "d.M.yy"; // Qt::DefaultLocaleShortDate
	QString res;
	if( includeDay )
		res = d.toString( "ddd, " ) + d.toString( s_dateFormat );
	else
		res = d.toString( s_dateFormat );
	if( includeWeek )
		res += QString( ", W%1" ).arg( d.weekNumber(), 2, 10, QChar( '0' ) );
	return res;
}

QString WtTypeDefs::prettyDateTime( const QDateTime& t )
{
    return t.date().toString( Qt::DefaultLocaleShortDate ) + t.time().toString( " hh:mm" );
}

bool WtTypeDefs::canHaveText(quint32 type)
{
    switch( type )
    {
    case TypeLink:
    case TypePdmItem:
        return false;
    default:
        return true;
    }
}

QString WtTypeDefs::formatValue(Udb::Atom name, const Udb::Obj & obj, bool useIcons)
{
    if( obj.isNull() )
        return tr("<null>");
    const Stream::DataCell v = obj.getValue( name );

    if( name == AttrLinkType )
        return formatLinkType( v.getUInt8(), false );
    else if( name == AttrTaskType )
        return formatTaskType( v.getUInt8() );
    else if( name == AttrRasciRole )
        return formatRasciRole( v.getUInt8(), false );
    else if( name == AttrSubTMSCount || name == AttrCalEntryCount )
        return (v.getUInt32() > 0)?"true":"false";
    else if( name == AttrDuration )
        return QString("%1 workdays").arg( v.getUInt16() );
    else if( name == AttrMsType )
        return formatMsType( v.getUInt8() );

    // TODO: Auflösung von weiteren EnumDefs

	switch( v.getType() )
	{
	case Stream::DataCell::TypeDateTime:
		return WtTypeDefs::prettyDateTime( v.getDateTime() );
	case Stream::DataCell::TypeDate:
		return WtTypeDefs::prettyDate( v.getDate(), false );
	case Stream::DataCell::TypeTime:
		return v.getTime().toString( "hh:mm" );
	case Stream::DataCell::TypeBml:
		{
			Stream::DataReader r( v.getBml() );
			return r.extractString(true);
		}
	case Stream::DataCell::TypeUrl:
		{
			QString host = v.getUrl().host();
			if( host.isEmpty() )
				host = v.getUrl().toString();
			return QString( "<a href=\"%1\">%2</a>" ).arg( QString::fromAscii( v.getArr() ) ).arg( host );
		}
	case Stream::DataCell::TypeHtml:
		return Txt::TextOutHtml::htmlToPlainText( v.getStr() );
	case Stream::DataCell::TypeOid:
		{
			Udb::Obj o = obj.getObject( v.getOid() );
			if( !o.isNull() )
			{
				QString img;
				if( useIcons )
					img = Oln::OutlineUdbMdl::getPixmapPath( o.getType() );
				if( !img.isEmpty() )
					img = QString( "<img src=\"%1\" align=\"middle\"/>&nbsp;" ).arg( img );
                return Txt::TextOutHtml::linkToHtml( Oln::Link( o ).writeTo(),
                                                         img + formatObjectTitle( o ) );
			}
		}
	case Stream::DataCell::TypeTimeSlot:
		{
			Stream::TimeSlot t = v.getTimeSlot();
			if( t.d_duration == 0 )
				return t.getStartTime().toString( "hh:mm" );
			else
				return t.getStartTime().toString( "hh:mm - " ) + t.getEndTime().toString( "hh:mm" );
		}
	case Stream::DataCell::TypeNull:
	case Stream::DataCell::TypeInvalid:
		return QString();
	case Stream::DataCell::TypeAtom:
		return prettyName( v.getAtom() );
	default:
		return v.toPrettyString();
	}
    return QString();
}

QString WtTypeDefs::formatLinkType(quint8 v, bool codeOnly)
{
    if( codeOnly )
        switch( v )
        {
        case LinkType_FS:
            return tr("FS");
        case LinkType_FF:
            return tr("FF");
        case LinkType_SS:
            return tr("SS");
        case LinkType_SF:
            return tr("SF");
        default:
            return tr("?");
        }
    else
        switch( v )
        {
        case LinkType_FS:
            return tr("Finish-to-Start (FS)");
        case LinkType_FF:
            return tr("Finish-to-Finish (FF)");
        case LinkType_SS:
            return tr("Start-to-Start (SS)");
        case LinkType_SF:
            return tr("Start-to-Finish (SF)");
        default:
            return tr("<unknown>");
        }
}

QString WtTypeDefs::formatRasciRole(quint8 v, bool codeOnly)
{
    if( codeOnly )
        switch( v )
        {
        case Rasci_Unknown:
            return tr("-");
        case Rasci_Responsible:
            return tr("R");
        case Rasci_Accountable:
            return tr("A");
        case Rasci_Supportive:
            return tr("S");
        case Rasci_Consulted:
            return tr("C");
        case Rasci_Informed:
            return tr("I");
        default:
            return tr("?");
        }
    else
        switch( v )
        {
        case Rasci_Unknown:
            return tr("<not defined>");
        case Rasci_Responsible:
            return tr("Responsible (R)");
        case Rasci_Accountable:
            return tr("Accountable (A)");
        case Rasci_Supportive:
            return tr("Supportive (S)");
        case Rasci_Consulted:
            return tr("Consulted (C)");
        case Rasci_Informed:
            return tr("Informed (I)");
        default:
            return tr("<unknown>");
        }
}

QString WtTypeDefs::formatMsType(quint8 type)
{
    switch( type )
    {
    case MsType_Intermediate:
        return tr("Intermediate");
    case MsType_TaskStart:
        return tr("Task Start");
    case MsType_TaskFinish:
        return tr("Task Finish");
    case MsType_ProjStart:
        return tr("Project Start");
    case MsType_ProjFinish:
        return tr("Project Finish");
    default:
        return tr("<unknown>");
    }
}

bool WtTypeDefs::isRasciAssignable(quint32 type)
{
    switch( type )
    {
    case TypeTask:
    case TypeMilestone: // RISK
    case TypeCriterion:
    case TypeDeliverable:
    case TypeWork:
        return true;
    }
    return false;
}

bool WtTypeDefs::isRasciPrincipal(quint32 type)
{
    switch( type )
    {
    case TypeOrgUnit:
    case TypeRole:
    case TypeHuman:
        return true;
    }
    return false;
}

bool WtTypeDefs::isImpType(quint32 type)
{
    switch( type )
    {
    case TypeTask:
    case TypeMilestone:
    case TypeImpEvent:
    case TypeAccomplishment:
    case TypeCriterion:
    case TypeIMP:
        return true;
    default:
        return false;
    }
}

bool WtTypeDefs::isObsType(quint32 type)
{
    switch( type )
    {
    case TypeOrgUnit:
    case TypeRole:
    case TypeHuman:
    case TypeUnitAssig:
    case TypeResource:
    case TypeOBS:
        return true;
    default:
        return false;
    }
}

bool WtTypeDefs::isWbsType(quint32 type)
{
    switch( type )
    {
    case TypeDeliverable:
    case TypeWork:
    case TypeWBS:
        return true;
    default:
        return false;
    }
}

bool WtTypeDefs::isSchedObj(quint32 type)
{
    switch( type )
    {
    case TypeTask:
    case TypeMilestone:
        return true;
    default:
        return false;
    }
}

bool WtTypeDefs::isPdmDiagram(quint32 type)
{
    switch( type )
    {
    case TypeIMP:
    case TypeCriterion:
    case TypeTask:
    case TypePdmDiagram:
        return true;
    default:
        return false;
    }
}

QString WtTypeDefs::formatTaskType(quint8 v)
{
    switch( v )
    {
    case TaskType_Discrete:
        return tr("Discrete");
    case TaskType_LOE:
        return tr("Level of Effort (LOE)");
    case TaskType_SVT:
        return tr("Schedule Visibility (SVT)");
    default:
        return tr("?");
    }
}

bool WtTypeDefs::canConvert(const Udb::Obj & o, quint32 toType)
{
    if( o.isNull() )
        return false;
    bool allowed = false;
    switch( o.getType() )
    {
    case TypeTask:
        if( toType == TypeMilestone )
        {
//            Udb::Obj e = o.getFirstObj();
//            if( !e.isNull() ) do
//            {
//                if( isImpType( e.getType() ) )
//                    // Wenn der Task irgendwelche IMP-Elemente enthält, kann man nicht umwandeln
//                    return false;
//            }while( e.next() );
//            allowed = true;
            ;
            allowed = ( o.getValue( AttrSubTMSCount ).getUInt32() == 0 );
        }else if( toType == TypeCriterion )
            allowed = true;
        break;
    case TypeMilestone:
        if( toType == TypeTask )
            allowed = true;
        break;
    case TypeCriterion:
        if( toType == TypeTask )
            allowed = true;
        break;
    }
    if( allowed )
        return isValidAggregate( o.getParent().getType(), toType );
    else
        return false;
}

Udb::Obj WtTypeDefs::getCalendars(Transaction *txn)
{
    Q_ASSERT( txn != 0 );
    Udb::Obj root = txn->getObject( WorkTreeApp::s_calendars );
    if( root.isNull() )
    {
        root = txn->createObject( WorkTreeApp::s_calendars );

        Udb::Obj cal = root.createAggregate( TypeCalendar );
        cal.setTimeStamp( AttrCreatedOn );
        cal.setString( AttrText, tr("7x24 Elapsed Days") );
        cal.setValue( AttrNonWorkingDays, Stream::DataCell().setAscii( "0000000" ) );

        cal = root.createAggregate( TypeCalendar );
        cal.setTimeStamp( AttrCreatedOn );
        cal.setString( AttrText, tr("Standard Five-day Week") );
        cal.setValue( AttrNonWorkingDays, Stream::DataCell().setAscii( "0000011" ) );

        root.setValueAsObj( AttrDefaultCal, cal );

        root.commit();
    }
    return root;
}

Obj WtTypeDefs::getProject(Transaction *txn)
{
    Q_ASSERT( txn != 0 );
    return txn->getOrCreateObject( WorkTreeApp::s_project );
}

Udb::Obj WtTypeDefs::getRoot(Udb::Transaction * txn)
{
    Q_ASSERT( txn != 0 );
    return txn->getOrCreateObject( WorkTreeApp::s_rootUuid );
}

