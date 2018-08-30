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

#ifndef KCMGRUB2_H
#define KCMGRUB2_H

//Qt
#include <QBitArray>
#include <QProgressDialog>
#include <QDebug>
//KDE
#include <KCModule>

#include <KCoreAddons/KJob>
#include <KAuth/KAuthActionReply>
using namespace KAuth;

//Project
#include "config.h"
class Entry;

//Ui
namespace Ui
{
    class KCMGRUB2;
}

class KCMGRUB2 : public KCModule
{
    Q_OBJECT
public:
    explicit KCMGRUB2(QWidget *parent = 0, const QVariantList &list = QVariantList());
    virtual ~KCMGRUB2();

    virtual void defaults();
    virtual void load();
    virtual void save();
private Q_SLOTS:
    void saveComplete(KJob*);
    void slotRetry();
    void slotRemoveOldEntries();
    void slotGrubSavedefaultChanged();
    void slotGrubNoEchoChanged();
    void slotGrubHiddenTimeoutToggled(bool checked);
    void slotGrubHiddenTimeoutChanged();
    void slotGrubHiddenTimeoutQuietChanged();
    void slotGrubTimeoutToggled(bool checked);
    void slotGrubTimeoutChanged();
    void slotGrubDisableRecoveryChanged();
    void slotMemtestChanged();
    void slotGrubDisableOsProberChanged();
    void slotInstallBootloader();
    void slotGrubGfxmodeChanged();
    void slotGrubGfxpayloadLinuxChanged();
    void slotReadResolutions();
    void slotGrubColorNormalChanged();
    void slotGrubColorHighlightChanged();
    void slowGrubBackgroundChanged();
    void slotPreviewGrubBackground();
    void slotCreateGrubBackground();
    void slotGrubThemeChanged();
    void slotGrubCmdlineLinuxDefaultChanged();
    void slotGrubCmdlineLinuxChanged();
    void slotGrubTerminalChanged();
    void slotGrubTerminalInputChanged();
    void slotGrubTerminalOutputChanged();
    void slotGrubDistributorChanged();
    void slotGrubSerialCommandChanged();
    void slotGrubInitTuneChanged();
    void slotGrubDisableLinuxUuidChanged();
    void slotGrubLanguageChanged();
 
    //Security
    void slotSecurityChanged();
    //users
    void slotAddUser();
    void slotDeleteUser();
    void slotEditUser();
    //groups
    void slotEditGroup();
    
    //custom entries
    void slotCustomChanged();

    void slotUpdateSuggestions();
    void slotTriggeredSuggestion(QAction *action);
private:
    void setupObjects();
    void setupConnections();

    //TODO: Maybe remove?
    QString convertToGRUBFileName(const QString &fileName);
    QString convertToLocalFileName(const QString &grubFileName);

    ExecuteJob * loadFile(GrubFile grubFile);
    QString readFile(GrubFile grubFile);
    void readEntries();
    bool initializeAuthorized = false;
    bool saveAuthorized = false;
    void readSettings();
    void readEnv();
    void readMemtest();
    void readDevices();
    void readLanguages();
    void initResolutions();
    void readResolutions();
//Security
    void parseGroupDir();
    void getSuperUsers();
    void getUsers();
    void getGroups();
//Encryption
    QString pbkdf2Encrypt(QString passwd); //, int key_length, int iteration_count
    
    void sortResolutions();
    void showResolutions();

    void showLanguages();

    QString processReply(ExecuteJob *reply);
    QString parseTitle(const QString &line);
    void parseEntries(const QString &config);
    void parseSettings(const QString &config);
    void parseEnv(const QString &config);

    Ui::KCMGRUB2 *ui;

    enum {
        grubSavedefaultDirty,
        grubHiddenTimeoutDirty,
        grubHiddenTimeoutQuietDirty,
        grubTimeoutDirty,
        grubNoEchoDirty,
        grubDisableRecoveryDirty,
        memtestDirty,
        grubDisableOsProberDirty,
        grubGfxmodeDirty,
        grubGfxpayloadLinuxDirty,
        grubColorNormalDirty,
        grubColorHighlightDirty,
        grubBackgroundDirty,
        grubThemeDirty,
        grubCmdlineLinuxDefaultDirty,
        grubCmdlineLinuxDirty,
        grubTerminalDirty,
        grubTerminalInputDirty,
        grubTerminalOutputDirty,
        grubDistributorDirty,
        grubSerialCommandDirty,
        grubInitTuneDirty,
        grubDisableLinuxUuidDirty,
        grubLanguageDirty,
//Security
//-------------------------------------------------
        securityDirty,
        securityUsersDirty,
        securityGroupsDirty,
//-------------------------------------------------
//custom entries
        customEntriesDirty,
//--------------
        lastDirtyBit
    };
    QBitArray m_dirtyBits;
    QBitArray m_dirtyBitsBak;

    QString resultLanguage;

    QList<Entry> m_entries;
    QHash<QString, QString> m_settings;
    QHash<QString, QString> m_env;
    bool m_memtest;
    bool m_memtestOn;
    QHash<QString, QString> m_devices;
    QHash<QString, QString> m_languages;
    QStringList m_resolutions;
//Security
    bool m_security;
    bool m_securityOn;
//Group
//-----------------------------------------------------
    QStringList m_groupFilesList;
    QHash<QString, QString> m_groupFilesContent;
    QHash<QString, bool> m_groupFileLocked;
    QHash<QString, QString> m_groupFileAllowedUsers;
//-----------------------------------------------------
    QString m_headerFile;
//SuperUsers
    QStringList m_superUsers;
//Users
//-----------------------------------------------------
    QStringList m_users;
    QHash<QString, QString> m_userPassword;
    QHash<QString, bool> m_userPasswordEncrypted;
    QHash<QString, bool> m_userIsSuper;
//-----------------------------------------------------
};


#endif
