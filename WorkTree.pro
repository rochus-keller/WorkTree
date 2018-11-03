#/*
#* Copyright 2012-2018 Rochus Keller <mailto:me@rochus-keller.info>
#*
#* This file is part of the WorkTree application.
#*
#* The following is the license that applies to this copy of the
#* application. For a license to use the application under conditions
#* other than those described here, please email to me@rochus-keller.info.
#*
#* GNU General Public License Usage
#* This file may be used under the terms of the GNU General Public
#* License (GPL) versions 2.0 or 3.0 as published by the Free Software
#* Foundation and appearing in the file LICENSE.GPL included in
#* the packaging of this file. Please review the following information
#* to ensure GNU General Public Licensing requirements will be met:
#* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#* http://www.gnu.org/copyleft/gpl.html.
#*/

QT += core gui
QT += network svg

TARGET = WorkTree
TEMPLATE = app
INCLUDEPATH += ../NAF ./.. ../../Libraries

win32 {
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/Qt
	RC_FILE = WorkTree.rc
	DEFINES -= UNICODE
	CONFIG += qaxcontainer
	CONFIG(debug, debug|release) {
		DEFINES += _DEBUG
		LIBS += -lQtCLucened
	} else {
	   LIBS += -lQtCLucene
	}
	SOURCES += MspImporter.cpp
	HEADERS += MspImporter.h
 }else {
	INCLUDEPATH += /home/me/Programme/Qt-4.4.3/include/Qt
	DESTDIR = ./tmp
	OBJECTS_DIR = ./tmp
	RCC_DIR = ./tmp
	UI_DIR = ./tmp
	MOC_DIR = ./tmp
	CONFIG(debug, debug|release) {
		DESTDIR = ./tmp-dbg
		OBJECTS_DIR = ./tmp-dbg
		RCC_DIR = ./tmp-dbg
		UI_DIR = ./tmp-dbg
		MOC_DIR = ./tmp-dbg
		DEFINES += _DEBUG
	}
	LIBS += -lQtCLucene
	DEFINES += LUA_USE_LINUX
	QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter
 }

SOURCES += main.cpp\
        MainWindow.cpp \
    WorkTreeApp.cpp \
    WtTypeDefs.cpp \
    ObjectHelper.cpp \
    ObjectTitleFrame.cpp \
    AttrViewCtrl.cpp \
    TextViewCtrl.cpp \
    PdmCtrl.cpp \
    PdmItemMdl.cpp \
    PdmItemObj.cpp \
    PdmItems.cpp \
    PdmOverview.cpp \
    PdmItemView.cpp \
    SceneOverview.cpp \
    PdmLinkViewCtrl.cpp \
    ImpMdl.cpp \
    ImpCtrl.cpp \
    GenericMdl.cpp \
    ObsMdl.cpp \
    GenericCtrl.cpp \
    PersonListView.cpp \
    HumanPropsDlg.cpp \
    RolePropsDlg.cpp \
    ObsCtrl.cpp \
    AssigViewMdl.cpp \
    AssigViewCtrl.cpp \
    Indexer.cpp \
    ../WorkTree/SearchView.cpp \
    WbsMdl.cpp \
    WbsCtrl.cpp \
    PdmLayouter.cpp \
    FolderCtrl.cpp \
    WpViewCtrl.cpp \
    TaskAttrDlg.cpp \
    CalendarEditor.cpp \
    Funcs.cpp \
    RefByViewCtrl.cpp \
    WtLuaBinding.cpp


HEADERS  += MainWindow.h \
    WorkTreeApp.h \
    WtTypeDefs.h \
    ObjectHelper.h \
    ObjectTitleFrame.h \
    AttrViewCtrl.h \
    TextViewCtrl.h \
    PdmCtrl.h \
    PdmItemMdl.h \
    PdmItemObj.h \
    PdmItems.h \
    PdmItemView.h \
    PdmOverview.h \
    SceneOverview.h \
    PdmLinkViewCtrl.h \
    ImpMdl.h \
    ImpCtrl.h \
    GenericMdl.h \
    ObsMdl.h \
    GenericCtrl.h \
    PersonListView.h \
    HumanPropsDlg.h \
    RolePropsDlg.h \
    ObsCtrl.h \
    AssigViewMdl.h \
    AssigViewCtrl.h \
    Indexer.h \
    SearchView.h \
    WbsMdl.h \
    WbsCtrl.h \
    PdmLayouter.h \
    FolderCtrl.h \
    WpViewCtrl.h \
    TaskAttrDlg.h \
    CalendarEditor.h \
    Funcs.h \
    RefByViewCtrl.h \
    WtLuaBinding.h

UseIcs {
	SOURCES += ../Herald/IcsDocument.cpp
	HEADERS  += ../Herald/IcsDocument.h
	DEFINES += _HAS_ICS_
}


include(Libraries.pri)

RESOURCES += \
    WorkTree.qrc

