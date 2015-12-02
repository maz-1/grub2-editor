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

#ifndef QAPTBACKEND_H
#define QAPTBACKEND_H

//Qt
#include <QStringList>

//QApt
#include <libqapt/backend.h>

class QAptBackend : public QObject
{
    Q_OBJECT
public:
    explicit QAptBackend(QObject *parent = 0);
    virtual ~QAptBackend();

    QStringList ownerPackage(const QString &fileName);
    void markForRemoval(const QString &packageName);
    QStringList markedForRemoval() const;
    void removePackages();
    void undoChanges();
Q_SIGNALS:
    void finished(bool success);
    void progress(const QString &status, int percentage);
private Q_SLOTS:
    void slotWorkerEvent(QApt::WorkerEvent event);
    void slotErrorOccurred(QApt::ErrorCode error, const QVariantMap &details);
private:
    QApt::Backend *m_backend;
    QApt::ErrorCode m_error;
};

#endif
