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

//KDE
#include <KLocalizedString>

//Taken from KDE playground/sysadmin/apper "as is"
static QString statusToString(PackageKit::Enum::Status status)
{
    switch (status) {
    case PackageKit::Enum::LastStatus:
    case PackageKit::Enum::UnknownStatus:
        return i18nc("This is when the transaction status is not known",
                     "Unknown state");
    case PackageKit::Enum::StatusSetup:
        return i18nc("transaction state, the daemon is in the process of starting",
                     "Waiting for service to start");
    case PackageKit::Enum::StatusWait:
        return i18nc("transaction state, the transaction is waiting for another to complete",
                     "Waiting for other tasks");
    case PackageKit::Enum::StatusRunning:
        return i18nc("transaction state, just started",
                     "Running task");
    case PackageKit::Enum::StatusQuery:
        return i18nc("transaction state, is querying data",
                     "Querying");
    case PackageKit::Enum::StatusInfo:
        return i18nc("transaction state, getting data from a server",
                     "Getting information");
    case PackageKit::Enum::StatusRemove:
        return i18nc("transaction state, removing packages",
                     "Removing packages");
    case PackageKit::Enum::StatusDownload:
        return i18nc("transaction state, downloading package files",
                     "Downloading packages");
    case PackageKit::Enum::StatusInstall:
        return i18nc("transaction state, installing packages",
                     "Installing packages");
    case PackageKit::Enum::StatusRefreshCache:
        return i18nc("transaction state, refreshing internal lists",
                     "Refreshing software list");
    case PackageKit::Enum::StatusUpdate:
        return i18nc("transaction state, installing updates",
                     "Updating packages");
    case PackageKit::Enum::StatusCleanup:
        return i18nc("transaction state, removing old packages, and cleaning config files",
                     "Cleaning up packages");
    case PackageKit::Enum::StatusObsolete:
        return i18nc("transaction state, obsoleting old packages",
                     "Obsoleting packages");
    case PackageKit::Enum::StatusDepResolve:
        return i18nc("transaction state, checking the transaction before we do it",
                     "Resolving dependencies");
    case PackageKit::Enum::StatusSigCheck:
        return i18nc("transaction state, checking if we have all the security keys for the operation",
                     "Checking signatures");
    case PackageKit::Enum::StatusRollback:
        return i18nc("transaction state, when we return to a previous system state",
                     "Rolling back");
    case PackageKit::Enum::StatusTestCommit:
        return i18nc("transaction state, when we're doing a test transaction",
                     "Testing changes");
    case PackageKit::Enum::StatusCommit:
        return i18nc("transaction state, when we're writing to the system package database",
                     "Committing changes");
    case PackageKit::Enum::StatusRequest:
        return i18nc("transaction state, requesting data from a server",
                     "Requesting data");
    case PackageKit::Enum::StatusFinished:
        return i18nc("transaction state, all done!",
                     "Finished");
    case PackageKit::Enum::StatusCancel:
        return i18nc("transaction state, in the process of cancelling",
                     "Cancelling");
    case PackageKit::Enum::StatusDownloadRepository:
        return i18nc("transaction state, downloading metadata",
                     "Downloading repository information");
    case PackageKit::Enum::StatusDownloadPackagelist:
        return i18nc("transaction state, downloading metadata",
                     "Downloading list of packages");
    case PackageKit::Enum::StatusDownloadFilelist:
        return i18nc("transaction state, downloading metadata",
                     "Downloading file lists");
    case PackageKit::Enum::StatusDownloadChangelog:
        return i18nc("transaction state, downloading metadata",
                     "Downloading lists of changes");
    case PackageKit::Enum::StatusDownloadGroup:
        return i18nc("transaction state, downloading metadata",
                     "Downloading groups");
    case PackageKit::Enum::StatusDownloadUpdateinfo:
        return i18nc("transaction state, downloading metadata",
                     "Downloading update information");
    case PackageKit::Enum::StatusRepackaging:
        return i18nc("transaction state, repackaging delta files",
                     "Repackaging files");
    case PackageKit::Enum::StatusLoadingCache:
        return i18nc("transaction state, loading databases",
                     "Loading cache");
    case PackageKit::Enum::StatusScanApplications:
        return i18nc("transaction state, scanning for running processes",
                     "Scanning installed applications");
    case PackageKit::Enum::StatusGeneratePackageList:
        return i18nc("transaction state, generating a list of packages installed on the system",
                     "Generating package lists");
    case PackageKit::Enum::StatusWaitingForLock:
        return i18nc("transaction state, when we're waiting for the native tools to exit",
                     "Waiting for package manager lock");
    case PackageKit::Enum::StatusWaitingForAuth:
        return i18nc("waiting for user to type in a password",
                     "Waiting for authentication");
    case PackageKit::Enum::StatusScanProcessList:
        return i18nc("we are updating the list of processes",
                     "Updating the list of running applications");
    case PackageKit::Enum::StatusCheckExecutableFiles:
        return i18nc("we are checking executable files in use",
                     "Checking for applications currently in use");
    case PackageKit::Enum::StatusCheckLibraries:
        return i18nc("we are checking for libraries in use",
                     "Checking for libraries currently in use");
    case PackageKit::Enum::StatusCopyFiles:
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
    PackageKit::Transaction t(QString(), this);
    if (t.error() != PackageKit::Client::NoError) {
        return QStringList();
    }
    m_package.clear();
    QEventLoop loop;
    connect(&t, SIGNAL(finished(PackageKit::Enum::Exit,uint)), &loop, SLOT(quit()));
    connect(&t, SIGNAL(finished(PackageKit::Enum::Exit,uint)), this, SLOT(slotFinished(PackageKit::Enum::Exit,uint)));
    connect(&t, SIGNAL(package(QSharedPointer<PackageKit::Package>)), this, SLOT(slotPackage(QSharedPointer<PackageKit::Package>)));
    t.searchFiles(fileName);
    loop.exec();
    return m_status == PackageKit::Enum::ExitSuccess && !m_package.isNull() ? QStringList() << m_package->name() << m_package->version() : QStringList();
}
void QPkBackend::markForRemoval(const QString &packageName)
{
    if (!m_remove.contains(packageName) && packageExists(packageName)) {
        m_remove.append(m_package->name());
        m_removePtrs.append(m_package);
    }
}
QStringList QPkBackend::markedForRemoval() const
{
    return m_remove;
}
void QPkBackend::removePackages()
{
    m_t = new PackageKit::Transaction(QString(), this);
    if (m_t->error() != PackageKit::Client::NoError) {
        return;
    }
    connect(m_t, SIGNAL(changed()), this, SLOT(slotUpdateProgress()));
    connect(m_t, SIGNAL(finished(PackageKit::Enum::Exit,uint)), this, SLOT(slotFinished(PackageKit::Enum::Exit,uint)));
    m_t->removePackages(m_removePtrs, false, true);
}
void QPkBackend::undoChanges()
{
    m_remove.clear();
    m_removePtrs.clear();
}

void QPkBackend::slotFinished(PackageKit::Enum::Exit status, uint runtime)
{
    Q_UNUSED(runtime)
    m_status = status;
    if (m_t && m_t->role() == PackageKit::Enum::RoleRemovePackages) {
        emit finished(m_status == PackageKit::Enum::ExitSuccess);
    }
}
void QPkBackend::slotPackage(const QSharedPointer<PackageKit::Package> &package)
{
    m_package = package;
}
void QPkBackend::slotUpdateProgress()
{
    emit progress(statusToString(m_t->status()), m_t->percentage());
}

bool QPkBackend::packageExists(const QString &packageName)
{
    PackageKit::Transaction t(QString(), this);
    if (t.error() != PackageKit::Client::NoError) {
        return false;
    }
    m_package.clear();
    QEventLoop loop;
    connect(&t, SIGNAL(finished(PackageKit::Enum::Exit,uint)), &loop, SLOT(quit()));
    connect(&t, SIGNAL(finished(PackageKit::Enum::Exit,uint)), this, SLOT(slotFinished(PackageKit::Enum::Exit,uint)));
    connect(&t, SIGNAL(package(QSharedPointer<PackageKit::Package>)), this, SLOT(slotPackage(QSharedPointer<PackageKit::Package>)));
    t.resolve(packageName);
    loop.exec();
    return m_status == PackageKit::Enum::ExitSuccess && !m_package.isNull();
}
