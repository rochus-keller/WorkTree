#ifndef MSPIMPORTER_H
#define MSPIMPORTER_H

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
#include <QStringList>
#include <Udb/Obj.h>
#include <QHash>

class QAxObject;

namespace Wt
{
    class MspImporter : public QObject
    {
        Q_OBJECT
    public:
        struct Counts
        {
            quint32 tasks;
            quint32 milestones;
            quint32 links;
            quint32 leads;
            quint32 deadLinks;
            quint32 lags;
            Counts():tasks(0),milestones(0),links(0),leads(0),deadLinks(0),lags(0){}
        };
        struct Link
        {
            QString d_file;
            int d_id;
            float d_lag; // +lag, -lead
            QString d_kind; // EA, AA, EE, AE
            QString d_unit; // m, h, t, w, prefix f (d, dy, w, wk, h, hr, m, min, mo, mon, prefix e)
            Link():d_id(0),d_lag(0) {}
            void parseId( const QString& );
            QString renderId() const;
            int getLinkType() const;
            int getLagDays() const;
        };
        static Link parseLink( const QString& );

        explicit MspImporter(QObject *parent = 0);
        ~MspImporter();
        bool prepareEngine( QWidget* );
        bool importMsp( QWidget*, Udb::Transaction* );
        QAxObject* openProject(const QString &path);
        const Counts& getCounts() const { return d_counts; }
        const QStringList& getErrors() const { return d_errors; }
    protected slots:
        void onException(int,const QString &,const QString &,const QString &);
    private:
        struct Task
        {
            Task(Task* p = 0):d_parent(p),d_task(0),d_id(0),d_pro(0)
            { if( p ) p->d_children.append(this); }
            ~Task();
            QString getName() const;
            int d_id;
            Udb::Obj d_obj;
            QAxObject* d_task;
            QString d_path; // Wenn der Task ein Subprojekt ist, ist hier der Pfad
            Task* d_pro; // Zeigt auf das Projekt, zu dem der Task gehört
            QList<Task*> d_children;
            Task* d_parent;
        };

        bool importTasksImp( QAxObject* tasks, Task* super, Task* project );
        void fetchTaskAttributes( Task*, bool ms );
        bool importLinksImp( Task* );
        const Task * findLinkTarget( const Task * sourceNode, const Link& link, bool* ok );

        QAxObject* d_mspApp;
        QStringList d_errors;
        Task* d_top;
        Counts d_counts;
        QHash<QString,Task*> d_projects; // CompleteBaseName -> ProjektTask
        QHash< QPair<Task*,int>, Task* > d_tasks; // ProjectTask/ID -> Task; darin sind alle Tasks ausser ExternalTask
    };
}

#endif // MSPIMPORTER_H
