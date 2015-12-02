/*******************************************************************************
 * Copyright (C) 2008-2013 Konstantinos Smanis <konstantinos.smanis@gmail.com> *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the Free  *
 * Software Foundation, either version 3 of the License, or (at your option)   *
 * any later version.                                                          *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
 * more details.                                                               *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 *******************************************************************************/

//Own
#include "qaptBackend.h"

//Qt
#include <QEventLoop>

QAptBackend::QAptBackend(QObject *parent) : QObject(parent)
{
    m_backend = new QApt::Backend;
    m_backend->init();
    m_error = QApt::UnknownError;
}
QAptBackend::~QAptBackend()
{
    delete m_backend;
}

QStringList QAptBackend::ownerPackage(const QString &fileName)
{
    QApt::Package *package;
    return (package = m_backend->packageForFile(fileName)) ? QStringList() << package->name() << package->version() : QStringList();
}
void QAptBackend::markForRemoval(const QString &packageName)
{
    Q_FOREACH(const QApt::Package *package, m_backend->markedPackages()) {
        if (packageName.compare(package->name()) == 0) {
            return;
        }
    }
    QApt::Package *package;
    if ((package = m_backend->package(packageName))) {
        package->setRemove();
    }
}
QStringList QAptBackend::markedForRemoval() const
{
    QStringList marked;
    Q_FOREACH(const QApt::Package *package, m_backend->markedPackages()) {
        marked.append(package->name());
    }
    return marked;
}
void QAptBackend::removePackages()
{
    connect(m_backend, SIGNAL(commitProgress(QString,int)), this, SIGNAL(progress(QString,int)));
    connect(m_backend, SIGNAL(workerEvent(QApt::WorkerEvent)), this, SLOT(slotWorkerEvent(QApt::WorkerEvent)));
    connect(m_backend, SIGNAL(errorOccurred(QApt::ErrorCode,QVariantMap)), this, SLOT(slotErrorOccurred(QApt::ErrorCode,QVariantMap)));
    m_backend->commitChanges();
}
void QAptBackend::undoChanges()
{
    m_backend->init();
}

void QAptBackend::slotWorkerEvent(QApt::WorkerEvent event)
{
    if (event == QApt::CommitChangesFinished) {
        emit finished(m_error == QApt::UnknownError);
    }
}
void QAptBackend::slotErrorOccurred(QApt::ErrorCode error, const QVariantMap &details)
{
    Q_UNUSED(details)
    m_error = error;
}
