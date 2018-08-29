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

#ifndef QPKBACKEND_H
#define QPKBACKEND_H

//Qt
#include <QStringList>

//PackageKit
#include <PackageKit/Transaction>

class QPkBackend : public QObject
{
    Q_OBJECT
public:
    explicit QPkBackend(QObject *parent = 0);
    virtual ~QPkBackend();

    QStringList ownerPackage(const QString &fileName);
    void markForRemoval(const QString &packageName);
    QStringList markedForRemoval() const;
    void removePackages();
    void undoChanges();
Q_SIGNALS:
    void finished(bool success);
    void progress(const QString &status, int percentage);
private Q_SLOTS:
    void slotFinished(PackageKit::Transaction::Exit status, uint runtime);
    void slotPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
    void slotUpdateProgress();
private:
    bool packageExists(const QString &packageName);

    PackageKit::Transaction *m_t;
    PackageKit::Transaction::Exit m_status;
    QString m_packageId;
    QStringList m_remove;
    QStringList m_removeIds;
};

#endif
