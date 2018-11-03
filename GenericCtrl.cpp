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

#include "GenericCtrl.h"
#include "GenericMdl.h"
#include <QtGui/QTreeView>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <Udb/Transaction.h>
#include <Udb/ContentObject.h>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include "Funcs.h"
using namespace Wt;

GenericCtrl::GenericCtrl(QTreeView * tree, Wt::GenericMdl *mdl) :
    QObject(tree), d_mdl( mdl )
{
    Q_ASSERT( mdl != 0 );
    Q_ASSERT( tree != 0 );
}

static void _expand( QTreeView* v, const QModelIndex& index )
{
	v->expand( index );
	const int max = index.model()->rowCount( index );
	for( int row = 0; row < max; row++ )
		_expand( v, index.child( row, 0 ) );
}

void GenericCtrl::onExpandSelected()
{
	ENABLED_IF( getTree()->currentIndex().isValid() );

	_expand( getTree(), getTree()->currentIndex() );
}

QTreeView *GenericCtrl::getTree() const
{
    return dynamic_cast<QTreeView*>( parent() );
}

void GenericCtrl::setRoot(const Udb::Obj &root)
{
    d_mdl->setRoot( root );
}

Udb::Obj GenericCtrl::getSelectedObject(bool rootOtherwise) const
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
	Udb::Obj doc;
    if( sr.size() == 1 )
        doc = d_mdl->getObject( sr.first() );
    else if( rootOtherwise )
        doc = d_mdl->getRoot();
    return doc;
}

void GenericCtrl::showObject(const Udb::Obj & o)
{
    QModelIndex i = d_mdl->getIndex( o );
    focusOn( o );
}

void GenericCtrl::add( quint32 type )
{
    Udb::Obj parentObj = getSelectedObject();
    if( parentObj.isNull() )
        parentObj = d_mdl->getRoot();
	ENABLED_IF( !d_mdl->isReadOnly() && Funcs::isValidAggregate( parentObj.getType(), type ) );

    Udb::Obj f = Funcs::createObject( type, parentObj, Udb::Obj() );
    f.commit();
    focusOn( f, true );
	emit signalSelected( f );
}

void GenericCtrl::focusOn(const Udb::Obj & o, bool edit )
{
    const QModelIndex i = d_mdl->getIndex( o );
    // unnötig wegen scrollto: getTree()->setExpanded( i.parent(), true );
    getTree()->selectionModel()->clearSelection();
	getTree()->setCurrentIndex( i );
	getTree()->scrollTo( i );
    if( edit )
        getTree()->edit( i );
}

void GenericCtrl::onEditName()
{
    Udb::Obj doc = getSelectedObject();
	ENABLED_IF( !d_mdl->isReadOnly() && !doc.isNull() );

    QString res = QInputDialog::getText( getTree(), tr( "Edit Name" ),
                                         tr("Enter a non-empty string:"), QLineEdit::Normal,
                                         Funcs::getText( doc, false ) ).trimmed();
    if( res.isEmpty() )
        return;
    Funcs::setText( doc, res );
    doc.commit();
}

void GenericCtrl::onNameEdited(const QModelIndex &index, const QVariant &value)
{
    Udb::Obj o = d_mdl->getObject( index );
    if( o.isNull() )
        return;
    const QString str = value.toString().trimmed();
    if( Funcs::getText( o, false ) != str )
    {
        Funcs::setText( o, str );
        o.commit();
        focusOn( o );
    }
}

void GenericCtrl::onMoveTo(const QList<Udb::Obj> &what, const QModelIndex &toParent, int beforeRow)
{
    Udb::Obj parentObj = d_mdl->getObject( toParent );
    Udb::Obj before;
    if( beforeRow != -1 )
    {
        QModelIndex temp = d_mdl->index( beforeRow, 0, toParent );
        if( temp.isValid() ) // nötig, da getObject root zurückgibt bei invalid
            before = d_mdl->getObject( temp );
    }
    QList<Udb::Obj> winner;
    foreach( Udb::Obj o, what )
    {
        Q_ASSERT( !o.isNull() );
        if( Funcs::isValidAggregate( parentObj.getType(), o.getType() ) )
        {
            Funcs::moveTo( o, parentObj, before );
            winner.append( o );
        }
    }
    d_mdl->getRoot().getTxn()->commit();
    getTree()->selectionModel()->clearSelection();
    getTree()->setExpanded( toParent, true );
    foreach( Udb::Obj o, winner )
        getTree()->selectionModel()->select( d_mdl->getIndex( o ), QItemSelectionModel::SelectCurrent );
    getTree()->scrollTo( getTree()->currentIndex() );
}

void GenericCtrl::onDoubleClicked(const QModelIndex &i)
{
	emit signalDblClicked( d_mdl->getObject( i ) );
}

bool GenericCtrl::canDelete(const QList<Udb::Obj> &)
{
	return true;
}

void GenericCtrl::onDeleteItems()
{
	QModelIndexList sr = getTree()->selectionModel()->selectedRows();
	ENABLED_IF( !d_mdl->isReadOnly() && sr.size() > 0 );

	QList<Udb::Obj> objs;
	for( int i = 0; i < sr.size(); i++ )
		objs.append( d_mdl->getObject( sr[i] ) );

	if( !canDelete(objs) )
		return;

    const int res = QMessageBox::warning( getTree(), tr("Delete Items"),
		tr("Do you really want to permanently delete the %1 selected items? This cannot be undone." ).
		arg( sr.size() ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( res == QMessageBox::No )
		return;

	for( int i = 0; i < objs.size(); i++ )
    {
		Funcs::erase( objs[i] );
    }
    d_mdl->getRoot().getTxn()->commit();
}

void GenericCtrl::onMoveUp()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
    if( sr.isEmpty() )
        return;
    qSort( sr );
    QModelIndex parent = sr.first().parent();
    for( int i = 1; i < sr.size(); i++ )
        if( sr[i].parent() != parent )
            return;
	ENABLED_IF( !d_mdl->isReadOnly() && sr.first().row() > 0 );

    Udb::Obj before = d_mdl->getObject( sr.first().sibling( sr.first().row() - 1, 0 ) );
    QList<Udb::Obj> objs;
    foreach( QModelIndex index, sr )
    {
        Udb::Obj o = d_mdl->getObject( index );
        Q_ASSERT( !o.isNull() );
        o.aggregateTo( o.getParent(), before );
        objs.append( o );
    }
    d_mdl->getRoot().getTxn()->commit();
    getTree()->selectionModel()->clearSelection();
    foreach( Udb::Obj o, objs )
        // TODO: funktioniert noch nicht richtig; es wird nur einer selektiert und Current falsch
        getTree()->selectionModel()->select( d_mdl->getIndex( o ), QItemSelectionModel::SelectCurrent );
    getTree()->scrollTo( getTree()->currentIndex() );
    // NOTE: hier AttrSubTMSCount nicht relevant, da Parent gleich bleibt
}

void GenericCtrl::onMoveDown()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
    if( sr.isEmpty() )
        return;
    qSort( sr );
    QModelIndex parent = sr.first().parent();
    for( int i = 1; i < sr.size(); i++ )
        if( sr[i].parent() != parent )
            return;
	ENABLED_IF( !d_mdl->isReadOnly() && sr.last().row() < ( d_mdl->rowCount( parent ) - 1 ) );

    Udb::Obj before;
    QModelIndex beforeIndex = sr.last().sibling( sr.last().row() + 2, 0 );
    if( beforeIndex.isValid() )
        before = d_mdl->getObject( beforeIndex );
    QList<Udb::Obj> objs;
    foreach( QModelIndex index, sr )
    {
        Udb::Obj o = d_mdl->getObject( index );
        Q_ASSERT( !o.isNull() );
        o.aggregateTo( o.getParent(), before );
        before = o.getNext();
        objs.append( o );
    }
    d_mdl->getRoot().getTxn()->commit();
    getTree()->selectionModel()->clearSelection();
    foreach( Udb::Obj o, objs )
        // TODO: funktioniert noch nicht richtig; es wird nur einer selektiert und Current falsch
        getTree()->selectionModel()->select( d_mdl->getIndex( o ), QItemSelectionModel::SelectCurrent );
    getTree()->scrollTo( getTree()->currentIndex() );
    // NOTE: hier AttrSubTMSCount nicht relevant, da Parent gleich bleibt
}

void GenericCtrl::onAddNext()
{
    Udb::Obj doc = getSelectedObject();
	ENABLED_IF( !d_mdl->isReadOnly() && !doc.isNull() );

    Udb::Obj o = Funcs::createObject( doc.getType(), doc.getParent(), doc.getNext() );
    o.commit();
    focusOn( o, true );
}

void GenericCtrl::onSelectionChanged()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
	if( sr.size() == 1 )
		emit signalSelected( d_mdl->getObject( sr.first() ) );
}

void GenericCtrl::onCopy()
{
    QModelIndexList sr = getTree()->selectionModel()->selectedRows();
    ENABLED_IF( !sr.isEmpty() );

    QMimeData* mime = d_mdl->mimeData( sr );
    QApplication::clipboard()->setMimeData( mime );
}

