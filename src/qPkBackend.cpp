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
#include "qPkBackend.h"

//Qt
#include <QEventLoop>

//KDE
#include <KLocalizedString>

//PackageKit
#include <PackageKit/Daemon>

//Taken from KDE playground/sysadmin/apper "as is"
static QString statusToString(PackageKit::Transaction::Status status)
{
    switch (status) {
    case PackageKit::Transaction::StatusSetup:
        return i18nc("transaction state, the daemon is in the process of starting",
                     "Waiting for service to start");
    case PackageKit::Transaction::StatusWait:
        return i18nc("transaction state, the transaction is waiting for another to complete",
                     "Waiting for other tasks");
    case PackageKit::Transaction::StatusRunning:
        return i18nc("transaction state, just started",
                     "Running task");
    case PackageKit::Transaction::StatusQuery:
        return i18nc("transaction state, is querying data",
                     "Querying");
    case PackageKit::Transaction::StatusInfo:
        return i18nc("transaction state, getting data from a server",
                     "Getting information");
    case PackageKit::Transaction::StatusRemove:
        return i18nc("transaction state, removing packages",
                     "Removing packages");
    case PackageKit::Transaction::StatusDownload:
        return i18nc("transaction state, downloading package files",
                     "Downloading packages");
    case PackageKit::Transaction::StatusInstall:
        return i18nc("transaction state, installing packages",
                     "Installing packages");
    case PackageKit::Transaction::StatusRefreshCache:
        return i18nc("transaction state, refreshing internal lists",
                     "Refreshing software list");
    case PackageKit::Transaction::StatusUpdate:
        return i18nc("transaction state, installing updates",
                     "Updating packages");
    case PackageKit::Transaction::StatusCleanup:
        return i18nc("transaction state, removing old packages, and cleaning config files",
                     "Cleaning up packages");
    case PackageKit::Transaction::StatusObsolete:
        return i18nc("transaction state, obsoleting old packages",
                     "Obsoleting packages");
    case PackageKit::Transaction::StatusDepResolve:
        return i18nc("transaction state, checking the transaction before we do it",
                     "Resolving dependencies");
    case PackageKit::Transaction::StatusSigCheck:
        return i18nc("transaction state, checking if we have all the security keys for the operation",
                     "Checking signatures");
    case PackageKit::Transaction::StatusTestCommit:
        return i18nc("transaction state, when we're doing a test transaction",
                     "Testing changes");
    case PackageKit::Transaction::StatusCommit:
        return i18nc("transaction state, when we're writing to the system package database",
                     "Committing changes");
    case PackageKit::Transaction::StatusRequest:
        return i18nc("transaction state, requesting data from a server",
                     "Requesting data");
    case PackageKit::Transaction::StatusFinished:
        return i18nc("transaction state, all done!",
                     "Finished");
    case PackageKit::Transaction::StatusCancel:
        return i18nc("transaction state, in the process of cancelling",
                     "Cancelling");
    case PackageKit::Transaction::StatusDownloadRepository:
        return i18nc("transaction state, downloading metadata",
                     "Downloading repository information");
    case PackageKit::Transaction::StatusDownloadPackagelist:
        return i18nc("transaction state, downloading metadata",
                     "Downloading list of packages");
    case PackageKit::Transaction::StatusDownloadFilelist:
        return i18nc("transaction state, downloading metadata",
                     "Downloading file lists");
    case PackageKit::Transaction::StatusDownloadChangelog:
        return i18nc("transaction state, downloading metadata",
                     "Downloading lists of changes");
    case PackageKit::Transaction::StatusDownloadGroup:
        return i18nc("transaction state, downloading metadata",
                     "Downloading groups");
    case PackageKit::Transaction::StatusDownloadUpdateinfo:
        return i18nc("transaction state, downloading metadata",
                     "Downloading update information");
    case PackageKit::Transaction::StatusRepackaging:
        return i18nc("transaction state, repackaging delta files",
                     "Repackaging files");
    case PackageKit::Transaction::StatusLoadingCache:
        return i18nc("transaction state, loading databases",
                     "Loading cache");
    case PackageKit::Transaction::StatusScanApplications:
        return i18nc("transaction state, scanning for running processes",
                     "Scanning installed applications");
    case PackageKit::Transaction::StatusGeneratePackageList:
        return i18nc("transaction state, generating a list of packages installed on the system",
                     "Generating package lists");
    case PackageKit::Transaction::StatusWaitingForLock:
        return i18nc("transaction state, when we're waiting for the native tools to exit",
                     "Waiting for package manager lock");
    case PackageKit::Transaction::StatusWaitingForAuth:
        return i18nc("waiting for user to type in a password",
                     "Waiting for authentication");
    case PackageKit::Transaction::StatusScanProcessList:
        return i18nc("we are updating the list of processes",
                     "Updating the list of running applications");
    case PackageKit::Transaction::StatusCheckExecutableFiles:
        return i18nc("we are checking executable files in use",
                     "Checking for applications currently in use");
    case PackageKit::Transaction::StatusCheckLibraries:
        return i18nc("we are checking for libraries in use",
                     "Checking for libraries currently in use");
    case PackageKit::Transaction::StatusCopyFiles:
        return i18nc("we are copying package files to prepare to install",
                     "Copying files");
    }
    return QString();
}

QPkBackend::QPkBackend(QObject *parent) : QObject(parent)
{
    m_t = 0;
}
QPkBackend::~QPkBackend()
{
}

QStringList QPkBackend::ownerPackage(const QString &fileName)
{
    auto *t = PackageKit::Daemon::searchFiles(fileName);
    m_packageId.clear();
    QEventLoop loop;
    connect(t, &PackageKit::Transaction::finished, &loop, &QEventLoop::quit);
    connect(t, &PackageKit::Transaction::finished, this, &QPkBackend::slotFinished);
    connect(t, &PackageKit::Transaction::package, this, &QPkBackend::slotPackage);
    loop.exec();
    return m_status == PackageKit::Transaction::ExitSuccess && !m_packageId.isNull() ?
                QStringList() << PackageKit::Transaction::packageName(m_packageId) << PackageKit::Transaction::packageVersion(m_packageId) : QStringList();
}
void QPkBackend::markForRemoval(const QString &packageName)
{
    if (!m_remove.contains(packageName) && packageExists(packageName)) {
        m_remove.append(PackageKit::Transaction::packageName(m_packageId));
        m_removeIds.append(m_packageId);
    }
}
QStringList QPkBackend::markedForRemoval() const
{
    return m_remove;
}
void QPkBackend::removePackages()
{
    m_t = PackageKit::Daemon::removePackages(m_removeIds, false, true);
    connect(m_t, &PackageKit::Transaction::percentageChanged, this, &QPkBackend::slotUpdateProgress);
    connect(m_t, &PackageKit::Transaction::statusChanged, this, &QPkBackend::slotUpdateProgress);
    connect(m_t, &PackageKit::Transaction::finished, this, &QPkBackend::slotFinished);
}
void QPkBackend::undoChanges()
{
    m_remove.clear();
    m_removeIds.clear();
}

void QPkBackend::slotFinished(PackageKit::Transaction::Exit status, uint runtime)
{
    Q_UNUSED(runtime)
    m_status = status;
    if (m_t && m_t->role() == PackageKit::Transaction::RoleRemovePackages) {
        Q_EMIT finished(m_status == PackageKit::Transaction::ExitSuccess);
    }
}
void QPkBackend::slotPackage(PackageKit::Transaction::Info, const QString &packageId, const QString &)
{
    m_packageId = packageId;
}
void QPkBackend::slotUpdateProgress()
{
    Q_EMIT progress(statusToString(m_t->status()), m_t->percentage());
}

bool QPkBackend::packageExists(const QString &packageName)
{
    auto *t = PackageKit::Daemon::resolve(packageName);
    m_packageId.clear();
    QEventLoop loop;
    connect(t, &PackageKit::Transaction::finished, &loop, &QEventLoop::quit);
    connect(t, &PackageKit::Transaction::finished, this, &QPkBackend::slotFinished);
    connect(t, &PackageKit::Transaction::package, this, &QPkBackend::slotPackage);
    loop.exec();
    return m_status == PackageKit::Transaction::ExitSuccess && !m_packageId.isNull();
}
