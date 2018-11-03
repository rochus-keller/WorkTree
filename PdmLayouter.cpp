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

#include "PdmLayouter.h"
#include <QHash>
#include "PdmItems.h"
#include "WtTypeDefs.h"
#include "PdmItemObj.h"
#include <Udb/Transaction.h>
#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>
#include <QtDebug>
#include <QApplication>
#include "WorkTreeApp.h"
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopServices>
#include <QProcess>
using namespace Wt;

typedef GVC_t *(*GvContext)(void);
typedef const char * (*GvcVersion)(GVC_t*);
typedef int (*GvFreeContext)(GVC_t *gvc);
typedef int (*GvLayout)(GVC_t *gvc, graph_t *g, const char *engine);
typedef int (*GvFreeLayout)(GVC_t *gvc, graph_t *g);
typedef int (*GvRenderFilename)(GVC_t *gvc, graph_t *g, const char *format, const char *filename);
typedef void (*Attach_attrs)(graph_t *g);
typedef int (*Agclose)(Agraph_t * g);
typedef Agraph_t* (*Agread)(void *chan, Agdisc_t * disc);
typedef Agraph_t* (*Agopen)(const char *name, Agdesc_t desc, Agdisc_t * disc);
typedef char * (*Aglasterr)(void);
typedef Agraph_t* (*Agmemread)(const char *cp);
typedef Agnode_t* (*Agidnode)(Agraph_t * g, unsigned long id, int createflag);
typedef Agedge_t* (*Agidedge)(Agraph_t * g, Agnode_t * t, Agnode_t * h,
                           unsigned long id, int createflag);
// Agidnode und Agidedge funktionieren nicht
typedef Agnode_t* (*Agnode)(Agraph_t * g, const char *name, int createflag);
typedef Agedge_t* (*Agedge)(Agraph_t * g, Agnode_t * t, Agnode_t * h,
                         const char *name, int createflag);
typedef const char* (*Agget)(void *obj, const char *name);
typedef int (*Agset)(void *obj, const char *name, const char *value);
typedef Agsym_t* (*Agattr)(Agraph_t * g, int kind, const char *name, const char *value);
typedef const char* (*Agnameof)(void *);

namespace Wt
{
    struct _Gvc
    {
        GvContext gvContext;
        GvcVersion gvcVersion;
        GvFreeContext gvFreeContext;
        GvLayout gvLayout;
        GvFreeLayout gvFreeLayout;
        GvRenderFilename gvRenderFilename;
        Attach_attrs attach_attrs;
        Agclose agclose;
        Agread agread;
        Agopen agopen;
        Aglasterr aglasterr;
        Agmemread agmemread;
        Agidnode agidnode;
        Agidedge agidedge;
        Agnode agnode;
        Agedge agedge;
        Agget agget;
        Agset agset;
        Agattr agattr;
        Agnameof agnameof;
        _Gvc():gvContext(0),gvcVersion(0),gvFreeContext(0),gvLayout(0),gvFreeLayout(0),
            agclose(0),gvRenderFilename(0),agread(0),agopen(0),aglasterr(0),
            agmemread(0),agidnode(0),agidedge(0),agnode(0),agedge(0),agget(0),
            agset(0),agattr(0),agnameof(0),attach_attrs(0){}
        bool allResolved() const { return gvContext && gvcVersion && gvFreeContext &&
                    gvLayout && gvFreeLayout && gvRenderFilename && agclose &&
                    agread && agopen && aglasterr && agmemread && agidnode &&
                    agidedge && agnode && agedge && agget && agset && agattr &&
                    agnameof && attach_attrs; }
    };
}

// aus https://github.com/ellson/graphviz/blob/master/lib/cgraph/graph.c
Agdesc_t Agdirected = { 1, 0, 0, 1 };
Agdesc_t Agstrictdirected = { 1, 1, 0, 1 };
Agdesc_t Agundirected = { 0, 0, 0, 1 };
Agdesc_t Agstrictundirected = { 0, 1, 0, 1 };

PdmLayouter::PdmLayouter(QObject *parent) :
    QObject(parent), d_ctx(0)
{
}

PdmLayouter::~PdmLayouter()
{
    if( d_ctx )
        delete d_ctx;
}

bool PdmLayouter::renderPdf(const QString &dotFile, const QString &pdfFile)
{
    d_errors.clear();
    if( !loadFunctions() )
        return false;

    GVC_t* gvc = d_ctx->gvContext();
    Q_ASSERT( gvc != 0 );

//    FILE* fp = ::fopen( dotFile.toAscii().data(), "r" );
//    if( fp == 0 )
//    {
//        d_errors.append( tr("Cannot open file for reading: %1").arg( dotFile ) );
//        d_ctx->gvFreeContext( gvc );
//        return false;
//    }

//    Agraph_t* G = d_ctx->agread( fp, 0 ); // Produziert Crash!
//    ::fclose( fp );

    QFile in( dotFile );
    if( !in.open( QIODevice::ReadOnly ) )
    {
        d_errors.append( tr("Cannot open file for reading: %1").arg( dotFile ) );
        d_ctx->gvFreeContext( gvc );
        return false;
    }
    Agraph_t* G = d_ctx->agmemread( in.readAll().data() ); // das funktioniert!

    if( G == 0 )
    {
        d_errors.append( QString::fromAscii( d_ctx->aglasterr() ) );
        d_ctx->gvFreeContext( gvc );
        return false;
    }

    d_ctx->gvLayout( gvc, G, "dot");
    d_ctx->gvRenderFilename( gvc, G, "pdf", pdfFile.toAscii().data() );

    d_ctx->agclose( G );

    d_ctx->gvFreeContext( gvc );
    return true;
}

bool PdmLayouter::layoutDiagram(const Udb::Obj & diagram, bool ortho, bool topToBottom )
{
    d_errors.clear();
    if( !loadFunctions() )
        return false;

    GVC_t* gvc = d_ctx->gvContext();
    Q_ASSERT( gvc != 0 );

    Agraph_t* G = d_ctx->agopen( WtTypeDefs::formatObjectTitle( diagram ).toUtf8().data(), Agdirected, 0 );
    Q_ASSERT( G != 0 );

    d_ctx->agattr( G, AGRAPH, "size", "150,150" );
    d_ctx->agattr( G, AGRAPH, "splines", (ortho)?"ortho":"polyline" ); // line, polyline, ortho
    d_ctx->agattr( G, AGRAPH, "rankdir", (topToBottom)?"TB":"LR" );
    d_ctx->agattr( G, AGRAPH, "dpi", "72" );
    d_ctx->agattr( G, AGRAPH, "nodesep", QByteArray::number( PdmNode::s_boxHeight / 72.0 * 0.5 ) );
    d_ctx->agattr( G, AGRAPH, "ranksep", QByteArray::number( PdmNode::s_boxWidth / 72.0 * 0.5 ) );
    d_ctx->agattr( G, AGNODE, "fixedsize", "true" );
    d_ctx->agattr( G, AGNODE, "shape", "box" );
    d_ctx->agattr( G, AGNODE, "height", QByteArray::number( PdmNode::s_boxHeight / 72.0 ) );
    d_ctx->agattr( G, AGNODE, "width", QByteArray::number( PdmNode::s_boxWidth / 72.0 ) );
    d_ctx->agattr( G, AGEDGE, "label", "" );

    QHash<Udb::OID,Agnode_t*> nodeCache; // orig->node
    QList<Agedge_t*> edgeList;

    Udb::Obj sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
        {
            Udb::Obj orig = sub.getValueAsObj( AttrOrigObject );
            if( orig.getType() != TypeLink )
            {
                Agnode_t* n = d_ctx->agnode( G, QByteArray::number( sub.getOid() ), 1 );
                Q_ASSERT( n != 0 );
                Udb::Obj orig = sub.getValueAsObj( AttrOrigObject );
                nodeCache[ orig.getOid() ] = n;
            }
        }
    }while( sub.next() );
    sub = diagram.getFirstObj();
    if( !sub.isNull() ) do
    {
        if( sub.getType() == TypePdmItem )
        {
            Udb::Obj link = sub.getValueAsObj( AttrOrigObject );
            if( link.getType() == TypeLink )
            {
                Agnode_t* pred = nodeCache.value( link.getValue( AttrPred ).getOid() );
                Agnode_t* succ = nodeCache.value( link.getValue( AttrSucc ).getOid() );
                // Wenn man Nodes löscht, können verwaiste PdmItems übrigbleiben bis zum nächsten Öffnen.
                if( pred && succ )
                {
                    Agedge_t* e = d_ctx->agedge( G, pred, succ, QByteArray::number( sub.getOid() ), 1 );
                    Q_ASSERT( e != 0 );
                    edgeList.append( e );
                    // TEST d_ctx->agset( e, "label", QByteArray::number( sub.getOid() ) );
                }
            }
        }
    }while( sub.next() );

    d_ctx->gvLayout( gvc, G, "dot");
    //d_ctx->gvRenderFilename( gvc, G, "pdf", "out.pdf" ); // TEST
    d_ctx->attach_attrs( G ); // ansonsten steht nichts in pos

//  Aus dotguide 2010, App F:
//    Coordinate values increase up and to the right. Positions
//    are represented by two integers separated by a comma, representing the X and Y
//    coordinates of the location speci?ed in points (1/72 of an inch). A position refers
//    to the center of its associated object. Lengths are given in inches.

    QHash<Udb::OID,Agnode_t*>::const_iterator j;
    double factor = 0.0;
	for( j = nodeCache.begin(); j != nodeCache.end(); ++j )
	{
		Agnode_t* n = *j;
        PdmItemObj item = diagram.getObject( QByteArray( d_ctx->agnameof( n ) ).toULongLong() );
        Q_ASSERT( !item.isNull() );
        factor = PdmNode::s_boxWidth / QByteArray( d_ctx->agget( n, "width" ) ).toFloat();
//        qDebug() << "Node " << d_ctx->agnameof( n ) << "pos=" << d_ctx->agget( n, "pos" ) <<
//                    "width=" << ( QByteArray( d_ctx->agget( n, "width" ) ).toFloat() * 72.0 ) <<
//                    "height=" << ( QByteArray( d_ctx->agget( n, "height" ) ).toFloat() * 72.0 );
        QStringList pos = QString::fromAscii( d_ctx->agget( n, "pos" ) ).split( QChar(',') );
        Q_ASSERT( pos.size() == 2 );
        item.setPos( QPointF( pos.first().toFloat() * factor / 72.0, -pos.last().toFloat() * factor / 72.0 ) );
	}
    foreach( Agedge_t* e, edgeList )
    {
        PdmItemObj item = diagram.getObject( QByteArray( d_ctx->agnameof( e ) ).toULongLong() );
        Q_ASSERT( !item.isNull() );
//        qDebug() << "Edge" << d_ctx->agnameof( e ) << "pos=" << d_ctx->agget( e, "pos" );
        QStringList points = QString::fromAscii( d_ctx->agget( e, "pos" ) ).split( QChar(' ') );
        Q_ASSERT( points.first().startsWith( QChar('e') ) );
        points.pop_front();
        QPolygonF poly;
        for( int i = (ortho)?0:3; i < ( points.size() - 1 ); i += 3 )
        {
            QStringList pos = points[i].split( QChar(',') );
            Q_ASSERT( pos.size() == 2 );
            poly.append( QPointF( pos.first().toFloat() * factor / 72.0, -pos.last().toFloat() * factor / 72.0 ) );
        }
        item.setNodeList( poly );

//        Every edge is assigned a pos attribute, which consists of a list of 3n + 1
//        locations. These are B-spline control points: points p0; p1; p2; p3 are the ?rst Bezier
//        spline, p3; p4; p5; p6 are the second, etc.
//        In the pos attribute, the list of control points might be preceded by a start
//        point ps and/or an end point pe. These have the usual position representation with a
//        "s," or "e," pre?x, respectively. A start point is present if there is an arrow at p0.
//        In this case, the arrow is from p0 to ps , where ps is actually on the node’s boundary.
    }

    d_ctx->agclose( G );
    d_ctx->gvFreeContext( gvc );
    return true;
}

bool PdmLayouter::loadLibs()
{
    d_errors.clear();
    if( d_ctx )
        return true;

#ifdef _WIN32
    d_libGvc.setFileName( "gvc");
    d_libCgraph.setFileName( "cgraph" );
#else
    d_libGvc.setLoadHints( QLibrary::ResolveAllSymbolsHint );
    d_libCgraph.setLoadHints( QLibrary::ResolveAllSymbolsHint );
    d_libGvc.setFileName( "libgvc.so.6");
    d_libCgraph.setFileName( "libcgraph.so.6" );
#endif
    if( d_libGvc.load() && d_libCgraph.load() )
    {
        return true;
    }else
    {
        d_errors.append( tr("Could not load Graphviz libraries; make sure Graphviz is on the library path.") );
        qDebug() << "PdmLayouter::loadLibs libgvc:" << d_libGvc.errorString();
        d_errors.append( d_libGvc.errorString() );
        qDebug() << "PdmLayouter::loadLibs libcgraph:" << d_libCgraph.errorString();
        d_errors.append( d_libCgraph.errorString() );
        return false;
    }
}

bool PdmLayouter::loadFunctions()
{
    d_errors.clear();
    if( d_ctx )
        return true;
    if( !d_libGvc.isLoaded() || !d_libCgraph.isLoaded() )
    {
        d_errors.append( tr("Graphviz libraries not loaded.") );
        return false;
    }

    _Gvc ctx;
    // libGv
    ctx.gvContext = (GvContext) d_libGvc.resolve( "gvContext" );
    ctx.gvcVersion = (GvcVersion) d_libGvc.resolve( "gvcVersion" );
    ctx.gvFreeContext = (GvFreeContext) d_libGvc.resolve( "gvFreeContext" );
    ctx.gvLayout = (GvLayout) d_libGvc.resolve( "gvLayout" );
    ctx.gvFreeLayout = (GvFreeLayout) d_libGvc.resolve( "gvFreeLayout" );
    ctx.gvRenderFilename = (GvRenderFilename) d_libGvc.resolve( "gvRenderFilename" );
    ctx.attach_attrs = (Attach_attrs) d_libGvc.resolve( "attach_attrs" );

    // libCgraph
    ctx.agclose = (Agclose) d_libCgraph.resolve( "agclose" );
    ctx.agread = (Agread) d_libCgraph.resolve( "agread" );
    ctx.agopen = (Agopen) d_libCgraph.resolve( "agopen" );
    ctx.aglasterr = (Aglasterr) d_libCgraph.resolve( "aglasterr" );
    ctx.agmemread = (Agmemread) d_libCgraph.resolve( "agmemread" );
    ctx.agidnode = (Agidnode) d_libCgraph.resolve( "agidnode" );
    ctx.agidedge = (Agidedge) d_libCgraph.resolve( "agidedge" );
    ctx.agnode = (Agnode) d_libCgraph.resolve( "agnode" );
    ctx.agedge = (Agedge) d_libCgraph.resolve( "agedge" );
    ctx.agget = (Agget) d_libCgraph.resolve( "agget" );
    ctx.agset = (Agset) d_libCgraph.resolve( "agset" );
    ctx.agattr = (Agattr) d_libCgraph.resolve( "agattr" );
    ctx.agnameof = (Agnameof) d_libCgraph.resolve( "agnameof" );

    if( !ctx.allResolved() )
    {
        d_errors.append( tr("Could not resolve all needed functions.") );
        return false;
    }
    d_ctx = new _Gvc( ctx );
    return true;
}

bool PdmLayouter::prepareEngine(QWidget * parent)
{
    const bool canSetLibPath = addLibraryPath( WorkTreeApp::inst()->getSet()->value(
                                                  "GraphvizBinPath" ).toString() );
    while( !loadLibs() )
    {
        if( canSetLibPath )
        {
            if( QMessageBox::warning( parent, tr("Layout Diagram - WorkTree" ),
                                  tr("<html>Could not find the <a href=\"http://graphviz.org\">"
                                     "Graphviz library</a>. Do you want to locate it?</html>" ),
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel )
                                    == QMessageBox::Cancel )
                return false;
        }else
        {
            QMessageBox::warning( parent, tr("Layout Diagram - WorkTree" ),
                                      tr("<html>Could not find the <a href=\"http://graphviz.org\">"
                                         "Graphviz library</a>. To use this function "
                                         "you have to add the Graphviz 'bin' "
                                         "directory to the PATH variable of your system.</html>" ) );
            return false;
        }
        QString path = QFileDialog::getExistingDirectory( parent,
                              tr("Looking for Graphviz bin directory - WorkTree"),
                              QDesktopServices::storageLocation(
                                  QDesktopServices::ApplicationsLocation ),
                              QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly );
        if( path.isEmpty() )
            return false;
        WorkTreeApp::inst()->getSet()->setValue( "GraphvizBinPath", path );
        addLibraryPath( path );
    }
    return true;
}

bool PdmLayouter::addLibraryPath(const QString & path)
{
#ifdef _WIN32
    // Dieser Code crashed beim Rücksprung aus der Funktion, wenn __stdcall nicht definiert
    QLibrary kernel32( "Kernel32.dll" );
    // aus winbase.h, leider im Compiler nicht verfügbar
    typedef int (__stdcall *SetDllDirectory)( const char* lpPathName );
    SetDllDirectory setDllDirectory = (SetDllDirectory)kernel32.resolve( "SetDllDirectoryA" );
    if( setDllDirectory )
    {
        if( path.isEmpty() )
            return true;
        setDllDirectory( path.toLatin1().data() );
        return true;
    }else
        return false; // QApplication::addLibraryPath hat keine Wirkung auf Windows
#else
    // qDebug() << QProcess::systemEnvironment();
    QApplication::addLibraryPath( path );
    return true;
#endif
}
