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
#include "qapt2Backend.h"

QApt2Backend::QApt2Backend(QObject *parent) : QObject(parent)
{
    m_backend = new QApt::Backend;
    m_backend->init();
    m_exitStatus = QApt::ExitSuccess;
}
QApt2Backend::~QApt2Backend()
{
    delete m_backend;
}

QStringList QApt2Backend::ownerPackage(const QString &fileName)
{
    QApt::Package *package;
    return (package = m_backend->packageForFile(fileName)) ? QStringList() << package->name() << package->version() : QStringList();
}
void QApt2Backend::markForRemoval(const QString &packageName)
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
QStringList QApt2Backend::markedForRemoval() const
{
    QStringList marked;
    Q_FOREACH(const QApt::Package *package, m_backend->markedPackages()) {
        marked.append(package->name());
    }
    return marked;
}
void QApt2Backend::removePackages()
{
    m_trans = m_backend->commitChanges();

    connect(m_trans, SIGNAL(progressChanged(int)), this, SLOT(slotUpdateProgress()));
    connect(m_trans, SIGNAL(finished(QApt::ExitStatus)), this, SLOT(slotTransactionFinished(QApt::ExitStatus)));
    m_trans->run();
}
void QApt2Backend::undoChanges()
{
    m_backend->init();
}

void QApt2Backend::slotUpdateProgress()
{
    emit progress(m_trans->statusDetails(), m_trans->progress());
}
void QApt2Backend::slotTransactionFinished(QApt::ExitStatus status)
{
    m_exitStatus = status;
    emit finished(status == QApt::ExitSuccess);
}
