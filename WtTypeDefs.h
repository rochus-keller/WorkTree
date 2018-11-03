#ifndef WTTYPEDEFS_H
#define WTTYPEDEFS_H

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

#include <QObject>
#include <Udb/Obj.h>

class QMimeData;

namespace Wt
{
    enum WtNumbers
	{
		WtStart = 0x20000,
		WtMax = WtStart + 99,
		WtEnd = WtStart + 1000 // Ab hier werden dynamische Atome angelegt
	};

    struct IndexDefs
	{
		static const char* IdxIdent; // AttrIdent
		static const char* IdxAltIdent; // AttrAltIdent
		static const char* IdxItemLink; // AttrItemLink
		static const char* IdxOrigObject; // AttrOrigObject
        static const char* IdxPred; // AttrPred
        static const char* IdxSucc; // AttrSucc
        static const char* IdxPrincipalFirstName;
		static const char* IdxRoleAssig; // AttrRoleAssig
		static const char* IdxRoleDeputy; // AttrRoleDeputy
        static const char* IdxAssigObject; // AttrAssigObject
        static const char* IdxAssigPrincipal; // AttrAssigPrincipal
        static const char* IdxWbsRef; // AttrWbsRef
        static const char* IdxCalDate; // Parent, AttrCalDate
	};

    enum TypeDef_Object // abstract
	{
		AttrCreatedOn = WtStart + 1, // DateTime
		AttrModifiedOn = WtStart + 2, // DateTime
		AttrText = WtStart + 3, // String, RTXT oder HTML, optional; anzeigbarer Titel/Text des Objekts
		AttrInternalId = WtStart + 4, // String, wird bei einigen Klassen mit automatischer ID gesetzt
		AttrCustomId = WtStart + 5, // String, der User kann seine eigene ID verwenden, welche die interne übersteuert

        // NOTE: Alte AttrIdent werden für Retyping als Cell gespeichert in der Form type -> id
	};

    enum TypeDef_Project
    {
        AttrProjStartDate = WtStart + 82, // QDate; ab hier wird Forward Pass berechnet
        AttrDataDate = WtStart + 83, // QDate
        // AttrStartMs und AttrFinishMs zeigen auf Projektstart und -ende
	};

	enum TypeDef_SchedObj // inherits Object
	{
		AttrEarlyStart = WtStart + 6, // QDate, ES
        AttrEarlyFinish = WtStart + 7, // QDate, EF
        AttrLateStart = WtStart + 8, // QDate, LS
        AttrLateFinish = WtStart + 9, // QDate, LF
        AttrDuration = WtStart + 10, // uint16; Number of Work Days; manuell eingegeben oder berechnet (für Summaries)
        // NOTE: elapsed bzw. fortlaufende durations werden über den 24x7-Kalender abgebildet
        AttrOptimisticDur = WtStart + 33, // uint16
        AttrPessimisticDur = WtStart + 34, // uint16
        AttrMostLikelyDur = WtStart + 35, // uint16

        AttrPlannedValue = WtStart + 88, // uint32
        AttrEarnedValue = WtStart + 89, // uint32
        AttrActualCost = WtStart + 90, // uint32

        // TODO: Baselines, Actuals
	};

    enum EnumDef_TaskType
    {
        TaskType_Discrete = 0,
        TaskType_LOE = 1, // Level of Effort
        TaskType_SVT = 2, // Schedule Visibility Task (statt Lags)
    };

	enum TypeDef_Task // inherits SchedObj
	{
		TypeTask = WtStart + 11,
		AttrTaskType = WtStart + 12, // uint8, EnumDef_TaskType
        AttrSubTMSCount = WtStart + 43, // uint32: Anzahl direkt enthaltener Tasks und Meilensteine
        AttrWbsRef = WtStart + 75, // OID: Ref. auf WBS
        AttrCriticalPath = WtStart + 78, // bool: true wenn Task/MS/Link auf kritischem Pfad
        AttrCalendar = WtStart + 81, // OID: Referenz auf Kalender, null für Default-Kalender
        // TODO: Flag Use Resource Calender wenn nicht der dem Task zugeordnete Kalender verwendet werden soll

        AttrStartMs = WtStart + 76, // OID: Referenz auf ersten Meilenstein im Projekt
        AttrFinishMs = WtStart + 77, // OID: Referenz auf letzten Meilenstein im Projekt
        // NOTE: auch alle Owner der Tasks (Criteria, etc.) verfügen über Start/FinishMS.
    };

    // TODO: CheckPoint, unterteilt Leaf-Tasks mit EVM Weighted Milestones, Prozentsatz, Fälligkeitsdatum
    // Wie Steps in Primavera

    enum EnumDef_MsType
    {
        MsType_Intermediate = 0,
        MsType_TaskStart = 1,
        MsType_TaskFinish = 2,
        MsType_ProjStart = 3,
        MsType_ProjFinish = 4,
    };

	enum TypeDef_Milestone // inherits SchedObj
	{
		TypeMilestone = WtStart + 13,
        AttrMsType = WtStart + 14, // uint8, EnumDef_MsType
	};

    enum EnumDef_LinkType
    {
        LinkType_FS = 0, // Finish-to-Start (FS), Default
        LinkType_FF = 1, // Finish-to-Finish (FF)
        LinkType_SS = 2, // Start-to-Start (SS)
        LinkType_SF = 3  // Start-to-Finish (SF)
    };

    enum TypeDef_Link // inherits Object
	{
        // Dependency, Relationship, Link
        TypeLink = WtStart + 15,
		AttrPred = WtStart + 16, // OID, Predecessor Task or Milestone
		AttrSucc = WtStart + 17, // OID, Successor Task or Milestone
        AttrLinkType = WtStart + 18, // uint8, LinkType
        // TODO: ev. summary Links, welche mehrere untergeordnete repräsentieren
	};

    enum TypeDef_ImpEvent // inherits Object
    {
        TypeImpEvent = WtStart + 19,
    };

    enum TypeDef_Accomplishment // inherits Object
    {
        TypeAccomplishment = WtStart + 20,
    };

    enum TypeDef_Criterion // inherits Object
    {
        TypeCriterion = WtStart + 21,
        // TODO: Entrance vs. Exit
    };

	enum TypeDef_IMP // inherits Object
	{
		TypeIMP = WtStart + 22,
	};

    enum TypeDef_Calendar // inherits Object
    {
        TypeCalendar = WtStart + 84,
        // Im wesentlichen eine sortierte Liste von Sperrtagen
        // Statt boolsche Sperren könnten auch prozentuale verwendet werden, um zu zeigen,
        // an welchen Tagen eine Ressource zu wieviel % zur Verfügung steht.
        AttrParentCalendar = WtStart + 86, // oid, optional, TODO: index
        AttrNonWorkingDays = WtStart + 85, // ascii: [0]..Montag bis [6] Sonntag, Wert '0' Working, '1' Nonworking, ' ' undefined
        AttrCalEntryCount = WtStart + 98, // uint32: Anzahl enthaltener CalendarItems
    };
    enum TypeDef_CalEntry // inherits Object
    {
        // Parent ist Calendar oder Leaf Task
        TypeCalEntry = WtStart + 91,
        AttrCalDate = WtStart + 92, // QDate, Anfangsdatum
        AttrCalDuration = WtStart + 93, // quint16, Default 1
        AttrNonWorking = WtStart + 94, // bool (wirkt mit true/false oder don't care wenn nicht vorhanden)
        AttrAvaility = WtStart + 95, // quint16: 0..0xffff %, xor mit AttrNonWorking
        AttrCalUid = WtStart + 96, // QString, für Replication aus iCal
        AttrCalFileRef = WtStart + 97, // oid, Referenz auf iCal-Datei
        // AttrText geerbt
    };
    enum TypeDef_Calendars
    {
        AttrDefaultCal = WtStart + 87, // oid
    };

    enum TypeDef_Deliverable // inherits Object
	{
        // WBS Element: Product, Data or Service
        // MIL-STD-881: hardware, software, services, data, and facilities
		TypeDeliverable = WtStart + 23,
	};

    // Ein Leaf-Element einer WBS; kann mehrstufig sein
	enum TypeDef_Work // inherits Object
	{
		TypeWork = WtStart + 24,
	};

    enum TypeDef_WBS // inherits Object
	{
		TypeWBS = WtStart + 74,
	};

    enum TypeDef_OutlineItem // inherits Object
	{
		TypeOutlineItem = WtStart + 25,
		TypeOutline = WtStart + 26,
		AttrItemIsExpanded = WtStart + 27, // bool: offen oder geschlossen
		AttrItemIsTitle = WtStart + 28, // bool: stellt das Item einen Title dar oder einen Text
		AttrItemIsReadOnly = WtStart + 29, // bool: ist Item ein Fixtext oder kann es bearbeitet werden
		AttrItemHome = WtStart + 30, // OID: Referenz auf Root des Outlines vom Typ TypeOutline
		AttrItemLink = WtStart + 31, // OID: optionale Referenz auf Irgend ein Object, dessen Body statt des eigenen angezeigt wird.
	};

    enum TypeDef_OBS // inherits Object
	{
		TypeOBS = WtStart + 69,
	};

    enum TypeDef_GenericPerson // inherits Object
	{
		AttrPrincipalName = WtStart + 45, // String
	};

    // Eine Körperschaft, z.B. Unternehmen, OE, Teams, etc.
	enum TypeDef_OrgUnit // inherits GenericPerson
	{
		TypeOrgUnit = WtStart + 32,
		// AttrText wird für Bezeichnung verwendet
        // Enthält sich selber, Rollen, Personen, Personenaliasse und Betriebsmittel
        // Ev. Enum Linie, Stab oder Team
	};

    enum TypeDef_Role // inherits GenericPerson
	{
		TypeRole = WtStart + 46,
		AttrRoleAssig = WtStart + 47, // OID
		AttrRoleDeputy = WtStart + 48, // OID
        // AttrText wird für Bezeichnung verwendet

        // Zu einem Work auch den Zweck bzw. die Rolle angeben
        // Unterscheiden zwischen RASCI, RAS/CA und normalem Work Assignment
        // Unterscheiden zw. Rollentypen und Instanzen (sonst müsste man dieselbe Rolle mehrfach definieren)
        // ev. Role vs. Job
	};

    enum TypeDef_Human // inherits GenericPerson, Schedule
	{
		TypeHuman = WtStart + 49,
		// AttrText wird für Pretty-Darstellung der Form "Dr. Rochus Keller" verwendet
		AttrFirstName = WtStart + 50, // String
		// LastName ist in GenericPerson
		AttrQualiTitle = WtStart + 51, // String: Dr., Hptm., Dipl. Ing. etc.
		AttrOrganization = WtStart + 52, // String
		AttrDepartment = WtStart + 53, // String
		AttrOffice = WtStart + 54, // String: Bürokoordinaten
		AttrJobTitle = WtStart + 55, // String
		AttrManager = WtStart + 56,	// String: Name des Vorgesetzten
		AttrAssistant = WtStart + 57,	// String: Name des Assistenten, Sekretärin etc.
		AttrPhoneDesk = WtStart + 58, // String
		AttrPhoneMobile = WtStart + 59, // String
		AttrEmail = WtStart + 60, // String
		AttrMailAddr = WtStart + 61, // String, multiline, Kasernenstrasse 19, CH-3001 Bern
		AttrPhonePriv = WtStart + 62, // String
		AttrMobilePriv = WtStart + 63, // String
		AttrMailAddrPriv = WtStart + 64, // String
		AttrEmailPriv = WtStart + 65, // String
	};

    enum TypeDef_UnitAssig // inherits Object
	{
        // Ziel: eine Person, die zu einer OrgUnit gehört, einer weiteren zuzuordnen (z.B. zu einem Team)
		TypeUnitAssig = WtStart + 67,
		// AttrMember = WtStart + 68, // OID zu TypeHuman
        // Verwendet stattdessen AttrItemLink als Member
        // NOTE: UnitAssig ist wie GenericPerson ein Aggregat von GenericPerson
	};

    enum TypeDef_Resource // inherits GenericPerson
	{
		// Alles, was nicht Human Resource ist: Alle Betriebsmittel wie Equipment, Services, Facilities, Materials, etc.
		TypeResource = WtStart + 66,
		// AttrText wird für Bezeichnung der Ressource verwendet
	};

    enum TypeDef_Assig // inherits Object
	{
		AttrAssigObject = WtStart + 70, // OID: OrgUnit, Deliverable, Work, Task, Criterion
		AttrAssigPrincipal = WtStart + 71, // OID: Human, Role, OrgUnit
        // NOTE: Assig ist immer Aggregat von Object
	};
    enum EnumDef_RasciRole // Key Responsibility Roles
	{
		Rasci_Unknown = 0,
		Rasci_Responsible = 1, // Those who do the work to achieve the task and are responsible for the task, ensuring that it is done as per the Approver.
		Rasci_Accountable = 2, // also Approver or final Approving authority. Those who are ultimately accountable for the correct and thorough completion of the deliverable or task.
		Rasci_Supportive = 3, // Resources allocated to Responsible. Unlike Consulted, who may provide input to the task, Support will assist in completing the task.
		Rasci_Consulted = 4, // Those whose opinions are sought and with whom there is two-way communication.
		Rasci_Informed = 5, // Those who are kept up-to-date on progress, often only on completion of the task or deliverable and with whom there is just one-way communication.
	};

    enum TypeDef_RasciAssig // inherits Assig
	{
        TypeRasciAssig = WtStart + 72,
        AttrRasciRole = WtStart + 73, // UInt8, RasciRole
        // TODO: Control Account? ist das immer Rasci_Responsible?
        // Ist Aggregat von AssigObject
	};

    enum TypeDef_PdmDiagram // inherits Object
    {
        TypePdmDiagram = WtStart + 36,
        AttrShowIds = WtStart + 37, // bool
        AttrMarkAlias = WtStart + 44, // bool

        // Aggregat für PdmItems
        // Diagramm bezieht sich auf dessen Owner
    };

    enum TypeDef_Folder // inherits Object
    {
        TypeFolder = WtStart + 79,
        TypeRootFolder = WtStart + 80,
    };

    enum TypeDef_PdmItem
    {
        TypePdmItem = WtStart + 38, // Für alle Diagrammelemente, Nodes und Lines
        AttrPosX = WtStart + 39, // float
        AttrPosY = WtStart + 40, // float
        AttrPointList = WtStart + 41, // frame ( xFloat, yFloat )* end
        AttrOrigObject = WtStart + 42, // OID; zeigt auf Task, MS oder Link, die durch das PdmItem dargestellt werden
    };

	enum TypeDef_Root
	{
		AttrAutoOpen = WtStart + 99   // OID: optionale Referenz auf ein Outline, das beim Start geöffnet wird
	};

    struct WtTypeDefs : public QObject // wegen tr
    {
        static void init( Udb::Database& db );
        static bool isValidAggregate( quint32 parent, quint32 child );
        static bool canConvert( const Udb::Obj&, quint32 toType );
        static QString prettyName( quint32 type );
        static QString prettyName( quint32 atom, Udb::Transaction* txn );
        static QString formatObjectTitle( const Udb::Obj& ); // plain String
        static QString formatObjectId( const Udb::Obj&, bool useOid = false );
        static QString formatValue( Udb::Atom, const Udb::Obj&, bool useIcons = true );
        static QString formatLinkType( quint8 , bool codeOnly = false );
        static QString formatRasciRole( quint8 , bool codeOnly = false );
        static QString formatMsType( quint8 );
        static bool isRasciAssignable( quint32 type );
        static bool isRasciPrincipal( quint32 type );
        static bool isImpType( quint32 type );
        static bool isObsType( quint32 type );
        static bool isWbsType( quint32 type );
        static bool isSchedObj( quint32 type );
        static bool isPdmDiagram( quint32 type );
        static QString formatTaskType( quint8 );
        static QString prettyDate( const QDate& d, bool includeDay = true, bool includeWeek = true );
        static QString prettyDateTime( const QDateTime& t );
        static bool canHaveText( quint32 type );
        static Udb::Obj getCalendars( Udb::Transaction* txn );
        static Udb::Obj getProject( Udb::Transaction* txn );
        static Udb::Obj getRoot( Udb::Transaction * );
    };
}

#endif // WTTYPEDEFS_H
