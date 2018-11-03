#ifndef WT_MAINWINDOW_H
#define WT_MAINWINDOW_H

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

#include <QtGui/QMainWindow>
#include <Udb/Transaction.h>
#include <Gui2/AutoMenu.h>
#include <QtCore/QList>

namespace Oln
{
	class DocTabWidget;
}

namespace Wt
{
    class ImpCtrl;
    class AttrViewCtrl;
    class TextViewCtrl;
    class SceneOverview;
    class PdmCtrl;
    class PdmLinkViewCtrl;
    class ObsCtrl;
    class AssigViewCtrl;
    class SearchView;
    class WbsCtrl;
    class MspImporter;
    class FolderCtrl;
    class WpViewCtrl;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(Udb::Transaction*);
        ~MainWindow();

        Udb::Transaction* getTxn() const { return d_txn; }
    signals:
        void closing();
    protected slots:
        void onFullScreen();
        void onImpSelected( const Udb::Obj&);
        void onSetFont();
        void onAbout();
        void onOpenPdmDiagram();
        void onPdmSelection();
        void onTabChanged( int i );
        void onImpDblClicked(const Udb::Obj&);
        void onShowSuperTask();
        void onShowSubTask();
        void onFollowAlias();
        void onLinkSelected( const Udb::Obj&, bool);
        void onAssigSelected( const Udb::Obj&, bool);
        void onObsSelected( const Udb::Obj&);
        void onWbsSelected( const Udb::Obj&);
        void onSearch();
        void onFollowObject( const Udb::Obj&);
        void onGoBack();
        void onGoForward();
        void onImportMsp();
        void onOpenDocument();
        void onFldrSelected( const Udb::Obj&);
        void onFldrDblClicked(const Udb::Obj&);
        void onUrlActivated(const QUrl& url);
        void onFollowOID( quint64 );
        void onWpSelected( const Udb::Obj& );
        void onCalendars();
		void saveEditor();
		void handleExecute();
		void onSetScriptFont();
		void onAutoStart();
	protected:
        void setupImp();
        void setupObs();
        void setupWbs();
        void setupAttrView();
        void setupTextView();
        void setupOverview();
        void setupLinkView();
        void setupAssigView();
        void setupSearchView();
        void setupFolders();
        void setupWbView();
		void setupTerminal();
        void setCaption();
        void addTopCommands( Gui2::AutoMenu* );
        void openPdmDiagram( const Udb::Obj& diagram, const Udb::Obj& select = Udb::Obj() );
        void showInPdmDiagram( const Udb::Obj& select, bool checkAllOpenDiagrams );
        void openOutline( const Udb::Obj& doc, const Udb::Obj& select = Udb::Obj() );
		void openScript( const Udb::Obj& doc );
		PdmCtrl* getCurrentPdmDiagram() const;
        void pushBack( const Udb::Obj& );
        static void toFullScreen( QMainWindow* );
        // Overrides
		void closeEvent ( QCloseEvent * event );
    private:
        ImpCtrl* d_imp;
        ObsCtrl* d_obs;
        AttrViewCtrl* d_atv;
        TextViewCtrl* d_tv;
        SceneOverview* d_ov;
        PdmLinkViewCtrl* d_lv;
        AssigViewCtrl* d_asv;
        SearchView* d_sv;
        WbsCtrl* d_wbs;
        FolderCtrl* d_fldr;
        WpViewCtrl* d_wpv;
        Udb::Transaction* d_txn;
		Oln::DocTabWidget* d_tab;
        QList<Udb::OID> d_backHisto; // d_backHisto.last() ist aktuell angezeigtes Objekt
		QList<Udb::OID> d_forwardHisto;
#ifdef _WIN32
        MspImporter* d_msp;
#endif
        bool d_pushBackLock;
        bool d_selectLock;
        bool d_fullScreen;
    };
}

#endif // WT_MAINWINDOW_H
