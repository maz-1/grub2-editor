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

//Krazy
//krazy:excludeall=cpp

//Own
#include "helper.h"

//Qt
#include <QDir>
#include <QtGlobal>

//KDE
//#include <KDebug>
#include <KProcess>
#include <klocalizedstring.h>

//Project
#include "../config.h"
#if HAVE_HD
#undef slots
#include <hd.h>
#endif

//The $PATH environment variable is emptied by D-Bus activation,
//so let's provide a sane default. Needed for os-prober to work.
static const QString path = QLatin1String("/usr/sbin:/usr/bin:/sbin:/bin");

Helper::Helper()
{
    qputenv("PATH", path.toLatin1());
}

ActionReply Helper::executeCommand(const QStringList &command, QHash<QString, QString> &environment)
{
    KProcess process;
    process.setProgram(command);
    process.setOutputChannelMode(KProcess::MergedChannels);

    if (!environment.isEmpty()) {
        QStringList grub_envKeys = environment.keys();
        Q_FOREACH(const QString &envKey, grub_envKeys) {
            process.setEnv(envKey, environment.value(envKey));
        }
    }

    qDebug() << "Executing" << command.join(" ");
    int exitCode = process.execute();

    ActionReply reply;
    if (exitCode != 0) {
        reply = ActionReply::HelperErrorReply();
        //TO BE FIXED
        reply.setErrorCode(ActionReply::Error::InvalidActionError);
    }
    reply.addData("command", command);
    reply.addData("output", process.readAll());
    return reply;
}

ActionReply Helper::initialize(QVariantMap args)
{
    ActionReply reply;
    switch (args.value("actionType").toInt()) {
    case actionLoad:
        reply = load(args);
        break;
    case actionProbe:
        reply = probe(args);
        break;
    case actionProbevbe:
        reply = probevbe(args);
    }
    return reply;
}

ActionReply Helper::executeLongCommand(const QStringList &command, QHash<QString, QString> &environment)
{
    KProcess process;
    process.setProgram(command);
    process.setOutputChannelMode(KProcess::MergedChannels);

    if (!environment.isEmpty()) {
        QStringList grub_envKeys = environment.keys();
        Q_FOREACH(const QString &envKey, grub_envKeys) {
            process.setEnv(envKey, environment.value(envKey));
        }
    }

    qDebug() << "Executing" << command.join(" ");
    process.start();
    bool finished = false; HelperSupport::progressStep(0); int i=0;
    do { 
      finished = process.waitForFinished(10000);
    
      HelperSupport::progressStep(i++); 
       
    } while(!finished);
 
    int exitCode = process.exitCode();

    ActionReply reply;
    if (exitCode != 0) {
        reply = ActionReply::HelperErrorReply();
        reply.setErrorCode(ActionReply::Error::InvalidActionError);
    }
    reply.addData("command", command);
    reply.addData("output", process.readAll());
    return reply;
}

ActionReply Helper::defaults(QVariantMap args)
{
    Q_UNUSED(args)
//Disable security
    QString filePath(QString(GRUB_CONFIGDIR)+QString(GRUB_SECURITY));
    QFile::Permissions permissions = QFile::permissions(filePath);
    permissions &= ~(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    QFile::setPermissions(filePath, permissions);
    
    ActionReply reply;
    QString configFileName = GRUB_CONFIG;
    QString originalConfigFileName = configFileName + ".original";
        
    if (!QFile::exists(originalConfigFileName)) {
        reply = ActionReply::HelperErrorReply();
        reply.addData("errorDescription", i18nc("@info", "Original configuration file <filename>%1</filename> does not exist.", originalConfigFileName));
        return reply;
    }
    if (!QFile::remove(configFileName)) {
        reply = ActionReply::HelperErrorReply();
        reply.addData("errorDescription", i18nc("@info", "Cannot remove current configuration file <filename>%1</filename>.", configFileName));
        return reply;
    }
    if (!QFile::copy(originalConfigFileName, configFileName)) {
        reply = ActionReply::HelperErrorReply();
        reply.addData("errorDescription", i18nc("@info", "Cannot copy original configuration file <filename>%1</filename> to <filename>%2</filename>.", originalConfigFileName, configFileName));
        return reply;
    }
    return reply;
}
ActionReply Helper::install(QVariantMap args)
{
    ActionReply reply;
    QHash<QString, QString> empty_hash;
    QString partition = args.value("partition").toString();
    QString mountPoint = args.value("mountPoint").toString();
    bool mbrInstall = args.value("mbrInstall").toBool();

    if (mountPoint.isEmpty()) {
        for (int i = 0; QDir(mountPoint = QString("%1/kcm-grub2-%2").arg(QDir::tempPath(), QString::number(i))).exists(); i++);
        if (!QDir().mkpath(mountPoint)) {
            reply = ActionReply::HelperErrorReply();
            reply.addData("errorDescription", i18nc("@info", "Failed to create temporary mount point."));
            return reply;
        }
        ActionReply mountReply = executeCommand(QStringList() << "mount" << partition << mountPoint, empty_hash);
        if (mountReply.failed()) {
            return mountReply;
        }
    }

    QStringList grub_installCommand;
    grub_installCommand << GRUB_INSTALL_EXE << "--root-directory" << mountPoint;
    if (mbrInstall) {
        grub_installCommand << partition.remove(QRegExp("\\d+"));
    } else {
        grub_installCommand << "--force" << partition;
    }
    return executeCommand(grub_installCommand, empty_hash);
}
ActionReply Helper::load(QVariantMap args)
{
    QHash<QString, QString> empty_hash;
    ActionReply reply;
    QString fileName;
    switch (args.value("grubFile").toInt()) {
    case GrubMenuFile:
        fileName = GRUB_MENU;
        break;
    case GrubConfigurationFile:
        fileName = GRUB_CONFIG;
        break;
    case GrubEnvironmentFile:
        fileName = GRUB_ENV;
        break;
//Security
    case GrubGroupFile:
        fileName = GRUB_CONFIGDIR + args.value("groupFile").toString();
        if (args.value("groupFile").toString() == GRUB_SECURITY) {
            if (!QFile::exists(fileName)) 
                executeCommand(QStringList() << "touch" << fileName, empty_hash);
            bool security = QFile::exists(fileName);
            reply.addData("security", security);
            reply.addData("securityOn", (bool)(QFile::permissions(fileName) & (QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther)));
            if (!security)
                qDebug() << "Unable to create" << fileName << ", please check file permissions.";
        }
        break;
    case GrubMemtestFile:
        bool memtest = QFile::exists(GRUB_MEMTEST);
        reply.addData("memtest", memtest);
        if (memtest) {
            reply.addData("memtestOn", (bool)(QFile::permissions(GRUB_MEMTEST) & (QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther)));
        }
        return reply;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        reply = ActionReply::HelperErrorReply();
        reply.addData("errorDescription", file.errorString());
        return reply;
    }
    reply.addData("rawFileContents", file.readAll());
    return reply;
}
ActionReply Helper::probe(QVariantMap args)
{
    ActionReply reply;
    QHash<QString, QString> empty_hash;
    QStringList mountPoints = args.value("mountPoints").toStringList();

    QStringList grubPartitions;
    HelperSupport::progressStep(0);
    for (int i = 0; i < mountPoints.size(); i++) {
        ActionReply grub_probeReply = executeCommand(QStringList() << GRUB_PROBE_EXE << "-t" << "drive" << mountPoints.at(i), empty_hash);
        if (grub_probeReply.failed()) {
            return grub_probeReply;
        }
        grubPartitions.append(grub_probeReply.data().value("output").toString().trimmed());
        HelperSupport::progressStep((i + 1) * 100. / mountPoints.size());
    }

    reply.addData("grubPartitions", grubPartitions);
    return reply;
}
ActionReply Helper::probevbe(QVariantMap args)
{
    Q_UNUSED(args)
    ActionReply reply;

#if HAVE_HD
    QStringList gfxmodes;
    hd_data_t hd_data;
    memset(&hd_data, 0, sizeof(hd_data));
    hd_t *hd = hd_list(&hd_data, hw_framebuffer, 1, NULL);
    for (hd_res_t *res = hd->res; res; res = res->next) {
        if (res->any.type == res_framebuffer) {
            gfxmodes += QString("%1x%2x%3").arg(QString::number(res->framebuffer.width), QString::number(res->framebuffer.height), QString::number(res->framebuffer.colorbits));
        }
    }
    hd_free_hd_list(hd);
    hd_free_hd_data(&hd_data);
    reply.addData("gfxmodes", gfxmodes);
#else
    reply = ActionReply::HelperErrorReply();
#endif

    return reply;
}
ActionReply Helper::save(QVariantMap args)
{
    ActionReply reply;
    QString configFileName = GRUB_CONFIG;
    QByteArray rawConfigFileContents = args.value("rawConfigFileContents").toByteArray();
    QByteArray rawDefaultEntry = args.value("rawDefaultEntry").toByteArray();
    QString resultLanguage = args.value("resultLanguage").toString();
    bool memtest = args.value("memtest").toBool();
    bool security = args.value("security").toBool();

    QFile::copy(configFileName, configFileName + ".original");

    QFile file(configFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        reply = ActionReply::HelperErrorReply();
        reply.addData("errorDescription", file.errorString());
        return reply;
    }
    file.write(rawConfigFileContents);
    file.close();

    if (args.contains("memtest")) {
        QFile::Permissions permissions = QFile::permissions(GRUB_MEMTEST);
        if (memtest) {
            permissions |= (QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
        } else {
            permissions &= ~(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
        }
        QFile::setPermissions(GRUB_MEMTEST, permissions);
    }
    
    if (args.contains("security")) {
        QString filePath(QString(GRUB_CONFIGDIR)+QString(GRUB_SECURITY));
        QFile::Permissions permissions = QFile::permissions(filePath);
        if (security) {
            permissions |= (QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
        } else {
            permissions &= ~(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
        }
        QFile::setPermissions(filePath, permissions);
    }
    
    if (args.contains("securityUsers")) {
        QByteArray rawUsersFileContents = args.value("securityUsers").toByteArray();
        //qDebug() << rawUsersFileContents;
        QFile usersFile(QString(GRUB_CONFIGDIR)+QString(GRUB_SECURITY));
        usersFile.open(QIODevice::WriteOnly | QIODevice::Text);
        usersFile.write(rawUsersFileContents);
        usersFile.close();
        
    }
    
    if (args.contains("securityGroupsList")) {
        QStringList groupFilesList = args.value("securityGroupsList").toString().split("/");
        for ( int i=0; i<groupFilesList.count(); ++i ) {
            QByteArray rawGroupFileContent = args.value(QString("GroupContent_")+groupFilesList[i]).toByteArray();
            //qDebug() << groupFilesList[i] << rawGroupFileContent;
            QFile groupFile(QString(GRUB_CONFIGDIR)+groupFilesList[i]);
            groupFile.open(QIODevice::WriteOnly | QIODevice::Text);
            groupFile.write(rawGroupFileContent);
            groupFile.close();
        }
        //qDebug() << "Groups modified :" << groupFilesList;
    }
   
    QHash<QString, QString> environment;


    if(resultLanguage == "")
    {
       QFile file("/etc/locale.conf");
       if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
     

          QString configStr = file.readAll();


          QString line;
          QTextStream stream(&configStr, QIODevice::ReadOnly | QIODevice::Text);
          while (!stream.atEnd()) {
             line = stream.readLine().trimmed();
             environment[line.section('=', 0, 0)] = line.section('=', 1);
          }
       
       }

    }
    else
    {
      environment["LANG"] = resultLanguage;
      environment["LANGUAGE"] = resultLanguage;
    }
 
    ActionReply grub_mkconfigReply = executeLongCommand(QStringList() << GRUB_MKCONFIG_EXE << "-o" << GRUB_MENU, environment);
    if (grub_mkconfigReply.failed()) {
        return grub_mkconfigReply;
    }

    ActionReply grub_set_defaultReply = executeCommand(QStringList() << GRUB_SET_DEFAULT_EXE << rawDefaultEntry, environment);
    if (grub_set_defaultReply.failed()) {
        return grub_set_defaultReply;
    }

    return grub_mkconfigReply;
}

KAUTH_HELPER_MAIN("org.kde.kcontrol.kcmgrub2", Helper)
