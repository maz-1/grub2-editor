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
#include "kcm_grub2.h"
#include "widgets/regexpinputdialog.h"
//Qt
#include <QDesktopWidget>
#include <QStandardItemModel>
#include <QTreeView>
#include <QMenu>
#include <QProgressBar>
#include <QTextStream>
#include <QProcess>
//Encryption
//#define random(x) (rand()%x)
//KDE
#include <KAboutData>
//#include <KDebug>
#include <KMessageBox>
#include <kmountpoint.h>
#include <KPluginFactory>
#include <KAuth>
using namespace KAuth;

#define TRANSLATION_DOMAIN "kcm-grub2"
#include <klocalizedstring.h>

//Project
#include "common.h"
#if HAVE_IMAGEMAGICK
#include "convertDlg.h"
#endif
#include "entry.h"
#include "installDlg.h"
//Security
#include "userDlg.h"
#include "groupDlg.h"
#define MENUPLACEHOLDER "__MENUPLACEHOLDER__"
#define SUBPLACEHOLDER "__SUBPLACEHOLDER__"
//
#if HAVE_QAPT || HAVE_QPACKAGEKIT
#include "removeDlg.h"
#endif

//Ui
#include "ui_kcm_grub2.h"

K_PLUGIN_FACTORY(GRUB2Factory, registerPlugin<KCMGRUB2>();)
K_EXPORT_PLUGIN(GRUB2Factory("kcmgrub2"))

#include <kcm_grub2.moc>

KCMGRUB2::KCMGRUB2(QWidget *parent, const QVariantList &list) : KCModule(parent, list)
{
    //Isn't KAboutData's second argument supposed to do this?

    KAboutData* about = new KAboutData("kcmgrub2", i18n("KDE GRUB2 Bootloader Control Module"), KCM_GRUB2_VERSION);
    about->setShortDescription(i18n("A KDE Control Module for configuring the GRUB2 bootloader."));
    about->setLicense(KAboutLicense::GPL_V3);
    about->setHomepage("http://ksmanis.wordpress.com/projects/grub2-editor/");

    about->addAuthor("Κonstantinos Smanis", i18n("Main Developer"), "konstantinos.smanis@gmail.com");
    about->addAuthor("Lin Ziyun", i18n("Developer"), "ohmygod19993@gmail.com");
    
    setAboutData(about);

    ui = new Ui::KCMGRUB2;
    ui->setupUi(this);
    setupObjects();
    setupConnections();
}
KCMGRUB2::~KCMGRUB2()
{
    delete ui;
}

void KCMGRUB2::defaults()
{
    Action defaultsAction("org.kde.kcontrol.kcmgrub2.defaults");
    defaultsAction.setHelperId("org.kde.kcontrol.kcmgrub2");
    
    ExecuteJob *reply = defaultsAction.execute();
    if (!reply->exec())
        KMessageBox::detailedError(this, i18nc("@info", "Failed to restore the default values."), processReply(reply));
    else
        load();
        save();
        KMessageBox::information(this, i18nc("@info", "Successfully restored the default values."));
}
void KCMGRUB2::load()
{
    readEntries();
    //stop load if not authorized
    if (initializeAuthorized == false)
        return;
    readSettings();
    readEnv();
    readMemtest();
//Security
    parseGroupDir();
//TEST
    //qDebug() << "Password : passwd_sample";
    //qDebug() << "Result :" << pbkdf2Encrypt("passwd_sample", 64, 10000);
#if HAVE_HD
    readResolutions();
#endif
    QString grubDefault = unquoteWord(m_settings.value("GRUB_DEFAULT"));
    if (grubDefault == QLatin1String("saved")) {
        grubDefault = (m_env.value("saved_entry").isEmpty() ? "0" : m_env.value("saved_entry"));
    }

    ui->kcombobox_default->clear();
    if (!m_entries.isEmpty()) {
        int maxLen = 0, maxLvl = 0;
        QStandardItemModel *model = new QStandardItemModel(ui->kcombobox_default);
        QTreeView *view = static_cast<QTreeView *>(ui->kcombobox_default->view());
        QList<QStandardItem *> ancestors;
        ancestors.append(model->invisibleRootItem());
        for (int i = 0; i < m_entries.size(); i++) {
            const Entry &entry = m_entries.at(i);
            const QString &prettyTitle = entry.prettyTitle();
            if (prettyTitle.length() > maxLen) {
                maxLen = prettyTitle.length();
            }
            if (entry.level() > maxLvl) {
                maxLvl = entry.level();
            }
            QStandardItem *item = new QStandardItem(prettyTitle);
            item->setData(entry.fullTitle());
            item->setSelectable(entry.type() == Entry::Menuentry);
            ancestors.last()->appendRow(item);
            if (i + 1 < m_entries.size()) {
                int n = m_entries.at(i + 1).level() - entry.level();
                if (n == 1) {
                    ancestors.append(item);
                } else if (n < 0) {
                    for (int j = 0; j > n; j--) {
                        ancestors.removeLast();
                    }
                }
            }
        }
        view->setModel(model);
        view->expandAll();
        ui->kcombobox_default->setModel(model);
        ui->kcombobox_default->setMinimumContentsLength(maxLen + maxLvl * 3);

        bool numericDefault = QRegExp("((\\d)+>)*(\\d)+").exactMatch(grubDefault);
        int entryIndex = -1;
        for (int i = 0; i < m_entries.size(); i++) {
            if ((numericDefault && m_entries.at(i).fullNumTitle() == grubDefault) || (!numericDefault && m_entries.at(i).fullTitle() == grubDefault)) {
                entryIndex = i;
                break;
            }
        }
        if (entryIndex != -1) {
            const Entry &entry = m_entries.at(entryIndex);
            if (entry.level() == 0) {
                ui->kcombobox_default->setCurrentIndex(entry.title().num);
            } else {
                QStandardItem *item = model->item(entry.ancestors().at(0).num);
                for (int i = 1; i < entry.level(); i++) {
                    item = item->child(entry.ancestors().at(i).num);
                }
                ui->kcombobox_default->setRootModelIndex(model->indexFromItem(item));
                ui->kcombobox_default->setCurrentIndex(entry.title().num);
                ui->kcombobox_default->setRootModelIndex(model->indexFromItem(model->invisibleRootItem()));
            }
        } else {
            qDebug() << "Invalid GRUB_DEFAULT value";
        }
    }
    ui->kpushbutton_remove->setEnabled(!m_entries.isEmpty());
    ui->checkBox_savedefault->setChecked(unquoteWord(m_settings.value("GRUB_SAVEDEFAULT")).compare("true") == 0);

    bool ok;
    if (!m_settings.value("GRUB_HIDDEN_TIMEOUT").isEmpty()) {
        int grubHiddenTimeout = unquoteWord(m_settings.value("GRUB_HIDDEN_TIMEOUT")).toInt(&ok);
        if (ok && grubHiddenTimeout >= 0) {
            ui->checkBox_hiddenTimeout->setChecked(grubHiddenTimeout > 0);
            ui->spinBox_hiddenTimeout->setValue(grubHiddenTimeout);
        } else {
            qDebug() << "Invalid GRUB_HIDDEN_TIMEOUT value";
        }
    }
    ui->checkBox_hiddenTimeoutShowTimer->setChecked(unquoteWord(m_settings.value("GRUB_HIDDEN_TIMEOUT_QUIET")).compare("true") != 0);
    int grubTimeout = (m_settings.value("GRUB_TIMEOUT").isEmpty() ? 5 : unquoteWord(m_settings.value("GRUB_TIMEOUT")).toInt(&ok));
    if (ok && grubTimeout >= -1) {
        ui->checkBox_timeout->setChecked(grubTimeout > -1);
        ui->radioButton_timeout0->setChecked(grubTimeout == 0);
        ui->radioButton_timeout->setChecked(grubTimeout > 0);
        ui->spinBox_timeout->setValue(grubTimeout);
    } else {
        qDebug() << "Invalid GRUB_TIMEOUT value";
    }

    ui->checkBox_recovery->setChecked(unquoteWord(m_settings.value("GRUB_DISABLE_RECOVERY")).compare("true") != 0);
    ui->checkBox_memtest->setVisible(m_memtest);
    ui->checkBox_memtest->setChecked(m_memtestOn);
    //Security
    ui->usersGroup->setEnabled(m_securityOn);
    ui->groupsGroup->setEnabled(m_securityOn);
    
    ui->checkBox_osProber->setChecked(unquoteWord(m_settings.value("GRUB_DISABLE_OS_PROBER")).compare("true") != 0);

    m_resolutions.append("640x480");
    QString grubGfxmode = (m_settings.value("GRUB_GFXMODE").isEmpty() ? "640x480" : unquoteWord(m_settings.value("GRUB_GFXMODE")));
    if (!grubGfxmode.isEmpty() && !m_resolutions.contains(grubGfxmode)) {
        m_resolutions.append(grubGfxmode);
    }
    QString grubGfxpayloadLinux = unquoteWord(m_settings.value("GRUB_GFXPAYLOAD_LINUX"));
    if (!grubGfxpayloadLinux.isEmpty() && grubGfxpayloadLinux.compare("text") != 0 && grubGfxpayloadLinux.compare("keep") != 0 && !m_resolutions.contains(grubGfxpayloadLinux)) {
        m_resolutions.append(grubGfxpayloadLinux);
    }
    m_resolutions.removeDuplicates();
    sortResolutions();
    showResolutions();
    ui->kcombobox_gfxmode->setCurrentIndex(ui->kcombobox_gfxmode->findData(grubGfxmode));
    ui->kcombobox_gfxpayload->setCurrentIndex(ui->kcombobox_gfxpayload->findData(grubGfxpayloadLinux));

    QString grubColorNormal = unquoteWord(m_settings.value("GRUB_COLOR_NORMAL"));
    if (!grubColorNormal.isEmpty()) {
        int normalForegroundIndex = ui->kcombobox_normalForeground->findData(grubColorNormal.section('/', 0, 0));
        int normalBackgroundIndex = ui->kcombobox_normalBackground->findData(grubColorNormal.section('/', 1));
        if (normalForegroundIndex == -1 || normalBackgroundIndex == -1) {
            qDebug() << "Invalid GRUB_COLOR_NORMAL value";
        }
        if (normalForegroundIndex != -1) {
            ui->kcombobox_normalForeground->setCurrentIndex(normalForegroundIndex);
        }
        if (normalBackgroundIndex != -1) {
            ui->kcombobox_normalBackground->setCurrentIndex(normalBackgroundIndex);
        }
    }
    QString grubColorHighlight = unquoteWord(m_settings.value("GRUB_COLOR_HIGHLIGHT"));
    if (!grubColorHighlight.isEmpty()) {
        int highlightForegroundIndex = ui->kcombobox_highlightForeground->findData(grubColorHighlight.section('/', 0, 0));
        int highlightBackgroundIndex = ui->kcombobox_highlightBackground->findData(grubColorHighlight.section('/', 1));
        if (highlightForegroundIndex == -1 || highlightBackgroundIndex == -1) {
            qDebug() << "Invalid GRUB_COLOR_HIGHLIGHT value";
        }
        if (highlightForegroundIndex != -1) {
            ui->kcombobox_highlightForeground->setCurrentIndex(highlightForegroundIndex);
        }
        if (highlightBackgroundIndex != -1) {
            ui->kcombobox_highlightBackground->setCurrentIndex(highlightBackgroundIndex);
        }
    }

    QString grubBackground = unquoteWord(m_settings.value("GRUB_BACKGROUND"));
    ui->kurlrequester_background->setText(grubBackground);
    ui->kpushbutton_preview->setEnabled(!grubBackground.isEmpty());
    ui->kurlrequester_theme->setText(unquoteWord(m_settings.value("GRUB_THEME")));

    ui->klineedit_cmdlineDefault->setText(unquoteWord(m_settings.value("GRUB_CMDLINE_LINUX_DEFAULT")));
    ui->klineedit_cmdline->setText(unquoteWord(m_settings.value("GRUB_CMDLINE_LINUX")));

    QString grubTerminal = unquoteWord(m_settings.value("GRUB_TERMINAL"));
    ui->klineedit_terminal->setText(grubTerminal);
    ui->klineedit_terminalInput->setReadOnly(!grubTerminal.isEmpty());
    ui->klineedit_terminalOutput->setReadOnly(!grubTerminal.isEmpty());
    ui->klineedit_terminalInput->setText(!grubTerminal.isEmpty() ? grubTerminal : unquoteWord(m_settings.value("GRUB_TERMINAL_INPUT")));
    ui->klineedit_terminalOutput->setText(!grubTerminal.isEmpty() ? grubTerminal : unquoteWord(m_settings.value("GRUB_TERMINAL_OUTPUT")));

    ui->klineedit_distributor->setText(unquoteWord(m_settings.value("GRUB_DISTRIBUTOR")));
    ui->klineedit_serial->setText(unquoteWord(m_settings.value("GRUB_SERIAL_COMMAND")));
    ui->klineedit_initTune->setText(unquoteWord(m_settings.value("GRUB_INIT_TUNE")));
    ui->checkBox_uuid->setChecked(unquoteWord(m_settings.value("GRUB_DISABLE_LINUX_UUID")).compare("true") != 0);

//Security
    ui->secEnabled->setVisible(m_security);
    ui->secEnabled->setChecked(m_securityOn);
    
    
    m_dirtyBits.fill(0);
    emit changed(false);
}
void KCMGRUB2::save()
{
    QString grubDefault;
    if (!m_entries.isEmpty()) {
        m_settings["GRUB_DEFAULT"] = "saved";
        QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->kcombobox_default->model());
        QTreeView *view = static_cast<QTreeView *>(ui->kcombobox_default->view());
        //Ugly, ugly hack. The view's current QModelIndex is invalidated
        //while the view is hidden and there is no access to the internal
        //QPersistentModelIndex (it is hidden in QComboBox's pimpl).
        //While the popup is shown, the QComboBox selects the corrent entry.
        //TODO: Maybe move away from the QComboBox+QTreeView implementation?
        ui->kcombobox_default->showPopup();
        grubDefault = model->itemFromIndex(view->currentIndex())->data().toString();
        ui->kcombobox_default->hidePopup();
    }
    if (m_dirtyBits.testBit(grubSavedefaultDirty)) {
        if (ui->checkBox_savedefault->isChecked()) {
            m_settings["GRUB_SAVEDEFAULT"] = "true";
        } else {
            m_settings.remove("GRUB_SAVEDEFAULT");
        }
    }
    if (m_dirtyBits.testBit(grubHiddenTimeoutDirty)) {
        if (ui->checkBox_hiddenTimeout->isChecked()) {
            m_settings["GRUB_HIDDEN_TIMEOUT"] = QString::number(ui->spinBox_hiddenTimeout->value());
        } else {
            m_settings.remove("GRUB_HIDDEN_TIMEOUT");
        }
    }
    if (m_dirtyBits.testBit(grubHiddenTimeoutQuietDirty)) {
        if (ui->checkBox_hiddenTimeoutShowTimer->isChecked()) {
            m_settings.remove("GRUB_HIDDEN_TIMEOUT_QUIET");
        } else {
            m_settings["GRUB_HIDDEN_TIMEOUT_QUIET"] = "true";
        }
    }
    if (m_dirtyBits.testBit(grubTimeoutDirty)) {
        if (ui->checkBox_timeout->isChecked()) {
            if (ui->radioButton_timeout0->isChecked()) {
                m_settings["GRUB_TIMEOUT"] = '0';
            } else {
                m_settings["GRUB_TIMEOUT"] = QString::number(ui->spinBox_timeout->value());
            }
        } else {
            m_settings["GRUB_TIMEOUT"] = "-1";
        }
    }
    if (m_dirtyBits.testBit(grubDisableRecoveryDirty)) {
        if (ui->checkBox_recovery->isChecked()) {
            m_settings.remove("GRUB_DISABLE_RECOVERY");
        } else {
            m_settings["GRUB_DISABLE_RECOVERY"] = "true";
        }
    }
    if (m_dirtyBits.testBit(grubDisableOsProberDirty)) {
        if (ui->checkBox_osProber->isChecked()) {
            m_settings.remove("GRUB_DISABLE_OS_PROBER");
        } else {
            m_settings["GRUB_DISABLE_OS_PROBER"] = "true";
        }
    }
    if (m_dirtyBits.testBit(grubGfxmodeDirty)) {
        if (ui->kcombobox_gfxmode->currentIndex() <= 0) {
            qDebug() << "Something went terribly wrong!";
        } else {
            m_settings["GRUB_GFXMODE"] = quoteWord(ui->kcombobox_gfxmode->itemData(ui->kcombobox_gfxmode->currentIndex()).toString());
        }
    }
    if (m_dirtyBits.testBit(grubGfxpayloadLinuxDirty)) {
        if (ui->kcombobox_gfxpayload->currentIndex() <= 0) {
            qDebug() << "Something went terribly wrong!";
        } else if (ui->kcombobox_gfxpayload->currentIndex() == 1) {
            m_settings.remove("GRUB_GFXPAYLOAD_LINUX");
        } else if (ui->kcombobox_gfxpayload->currentIndex() > 1) {
            m_settings["GRUB_GFXPAYLOAD_LINUX"] = quoteWord(ui->kcombobox_gfxpayload->itemData(ui->kcombobox_gfxpayload->currentIndex()).toString());
        }
    }
    if (m_dirtyBits.testBit(grubColorNormalDirty)) {
        QString normalForeground = ui->kcombobox_normalForeground->itemData(ui->kcombobox_normalForeground->currentIndex()).toString();
        QString normalBackground = ui->kcombobox_normalBackground->itemData(ui->kcombobox_normalBackground->currentIndex()).toString();
        if (normalForeground.compare("light-gray") != 0 || normalBackground.compare("black") != 0) {
            m_settings["GRUB_COLOR_NORMAL"] = normalForeground + '/' + normalBackground;
        } else {
            m_settings.remove("GRUB_COLOR_NORMAL");
        }
    }
    if (m_dirtyBits.testBit(grubColorHighlightDirty)) {
        QString highlightForeground = ui->kcombobox_highlightForeground->itemData(ui->kcombobox_highlightForeground->currentIndex()).toString();
        QString highlightBackground = ui->kcombobox_highlightBackground->itemData(ui->kcombobox_highlightBackground->currentIndex()).toString();
        if (highlightForeground.compare("black") != 0 || highlightBackground.compare("light-gray") != 0) {
            m_settings["GRUB_COLOR_HIGHLIGHT"] = highlightForeground + '/' + highlightBackground;
        } else {
            m_settings.remove("GRUB_COLOR_HIGHLIGHT");
        }
    }
    if (m_dirtyBits.testBit(grubBackgroundDirty)) {
        QString background = ui->kurlrequester_background->url().toLocalFile();
        if (!background.isEmpty()) {
            m_settings["GRUB_BACKGROUND"] = quoteWord(background);
        } else {
            m_settings.remove("GRUB_BACKGROUND");
        }
    }
    if (m_dirtyBits.testBit(grubThemeDirty)) {
        QString theme = ui->kurlrequester_theme->url().toLocalFile();
        if (!theme.isEmpty()) {
            m_settings["GRUB_THEME"] = quoteWord(theme);
        } else {
            m_settings.remove("GRUB_THEME");
        }
    }
    if (m_dirtyBits.testBit(grubCmdlineLinuxDefaultDirty)) {
        QString cmdlineLinuxDefault = ui->klineedit_cmdlineDefault->text();
        if (!cmdlineLinuxDefault.isEmpty()) {
            m_settings["GRUB_CMDLINE_LINUX_DEFAULT"] = quoteWord(cmdlineLinuxDefault);
        } else {
            m_settings.remove("GRUB_CMDLINE_LINUX_DEFAULT");
        }
    }
    if (m_dirtyBits.testBit(grubCmdlineLinuxDirty)) {
        QString cmdlineLinux = ui->klineedit_cmdline->text();
        if (!cmdlineLinux.isEmpty()) {
            m_settings["GRUB_CMDLINE_LINUX"] = quoteWord(cmdlineLinux);
        } else {
            m_settings.remove("GRUB_CMDLINE_LINUX");
        }
    }
    if (m_dirtyBits.testBit(grubTerminalDirty)) {
        QString terminal = ui->klineedit_terminal->text();
        if (!terminal.isEmpty()) {
            m_settings["GRUB_TERMINAL"] = quoteWord(terminal);
        } else {
            m_settings.remove("GRUB_TERMINAL");
        }
    }
    if (m_dirtyBits.testBit(grubTerminalInputDirty)) {
        QString terminalInput = ui->klineedit_terminalInput->text();
        if (!terminalInput.isEmpty()) {
            m_settings["GRUB_TERMINAL_INPUT"] = quoteWord(terminalInput);
        } else {
            m_settings.remove("GRUB_TERMINAL_INPUT");
        }
    }
    if (m_dirtyBits.testBit(grubTerminalOutputDirty)) {
        QString terminalOutput = ui->klineedit_terminalOutput->text();
        if (!terminalOutput.isEmpty()) {
            m_settings["GRUB_TERMINAL_OUTPUT"] = quoteWord(terminalOutput);
        } else {
            m_settings.remove("GRUB_TERMINAL_OUTPUT");
        }
    }
    if (m_dirtyBits.testBit(grubDistributorDirty)) {
        QString distributor = ui->klineedit_distributor->text();
        if (!distributor.isEmpty()) {
            m_settings["GRUB_DISTRIBUTOR"] = quoteWord(distributor);
        } else {
            m_settings.remove("GRUB_DISTRIBUTOR");
        }
    }
    if (m_dirtyBits.testBit(grubSerialCommandDirty)) {
        QString serialCommand = ui->klineedit_serial->text();
        if (!serialCommand.isEmpty()) {
            m_settings["GRUB_SERIAL_COMMAND"] = quoteWord(serialCommand);
        } else {
            m_settings.remove("GRUB_SERIAL_COMMAND");
        }
    }
    if (m_dirtyBits.testBit(grubInitTuneDirty)) {
        QString initTune = ui->klineedit_initTune->text();
        if (!initTune.isEmpty()) {
            m_settings["GRUB_INIT_TUNE"] = quoteWord(initTune);
        } else {
            m_settings.remove("GRUB_INIT_TUNE");
        }
    }
    if (m_dirtyBits.testBit(grubDisableLinuxUuidDirty)) {
        if (ui->checkBox_uuid->isChecked()) {
            m_settings.remove("GRUB_DISABLE_LINUX_UUID");
        } else {
            m_settings["GRUB_DISABLE_LINUX_UUID"] = "true";
        }
    }
    
    QString configFileContents;
    QTextStream stream(&configFileContents, QIODevice::WriteOnly | QIODevice::Text);
    QHash<QString, QString>::const_iterator it = m_settings.constBegin();
    QHash<QString, QString>::const_iterator end = m_settings.constEnd();
    for (; it != end; ++it) {
        stream << it.key() << '=' << it.value() << endl;
    }
    
    Action saveAction("org.kde.kcontrol.kcmgrub2.save");
    saveAction.setHelperId("org.kde.kcontrol.kcmgrub2");
    saveAction.addArgument("rawConfigFileContents", configFileContents.toLocal8Bit());
    saveAction.addArgument("rawDefaultEntry", !m_entries.isEmpty() ? grubDefault : m_settings.value("GRUB_DEFAULT").toLocal8Bit());
    if (m_dirtyBits.testBit(memtestDirty)) {
        saveAction.addArgument("memtest", ui->checkBox_memtest->isChecked());
    }
    //Security
    if (m_dirtyBits.testBit(securityDirty)) {
        saveAction.addArgument("security", ui->secEnabled->isChecked());
    }
    //Security : save users list
    if (m_dirtyBits.testBit(securityUsersDirty)) {
        QString userFileContents;
        userFileContents.append("#!/bin/sh\n# This file is generated by grub2-editor, don't try to edit, all changes made hrer will be overwritten!\necho '\n#insmod password\n#insmod password_pbkdf2\n#insmod pbkdf2\nset superusers=\"");
        QStringList superUsers;
        for( int i=0; i<m_users.count(); ++i ) {
            if (m_userIsSuper[m_users[i]]){
                superUsers.append(m_users[i]);
            }
        }
        userFileContents.append(superUsers.join(","));
        userFileContents.append("\"\n");
        
        for ( int i=0; i<m_users.count(); ++i ) {
            userFileContents.append(m_userPasswordEncrypted[m_users[i]]?"password_pbkdf2":"password");
            userFileContents.append(QChar(32) + m_users[i] + QChar(32));
            userFileContents.append(m_userPassword[m_users[i]]);
            userFileContents.append("\n");
        }
        userFileContents.append("'\n");
        //qDebug() << userFileContents;
        saveAction.addArgument("securityUsers", userFileContents);
    }
    //Security : save group files
    if (m_dirtyBits.testBit(securityGroupsDirty)) {
        saveAction.addArgument("securityGroupsList", m_groupFilesList.join("/"));
        QHash<QString, QString> groupFilesContent(m_groupFilesContent);
        for ( int i=0; i<m_groupFilesList.count(); ++i ) {
            if (m_groupFileLocked[m_groupFilesList[i]]) {
                groupFilesContent[m_groupFilesList[i]].replace(QString(MENUPLACEHOLDER), QString("menuentry --users ")+m_groupFileAllowedUsers[m_groupFilesList[i]]+QString(" "));
                groupFilesContent[m_groupFilesList[i]].replace(QString(SUBPLACEHOLDER), QString("submenu --users ")+m_groupFileAllowedUsers[m_groupFilesList[i]]+QString(" "));
            } else {
                groupFilesContent[m_groupFilesList[i]].replace(QString(MENUPLACEHOLDER), QString("menuentry --unrestricted "));
                groupFilesContent[m_groupFilesList[i]].replace(QString(SUBPLACEHOLDER), QString("submenu --unrestricted "));
            }
            //qDebug() << "groupFiles :" << groupFilesContent[m_groupFilesList[i]];
            saveAction.addArgument(QString("GroupContent_")+m_groupFilesList[i], groupFilesContent[m_groupFilesList[i]]);
        }
        
    }
    
    QProgressDialog progressDlg(this, Qt::Dialog);
        progressDlg.setWindowTitle(i18nc("@title:window Verb (gerund). Refers to current status.", "Saving"));
        progressDlg.setLabelText(i18nc("@info:progress", "Saving GRUB settings..."));
        progressDlg.setCancelButton(0);
        progressDlg.setModal(true);
        progressDlg.setRange(0,0);
        progressDlg.show();
    
    
    ExecuteJob *reply = saveAction.execute();
    //connect(reply, SIGNAL(result()), &progressDlg, SLOT(hide()));
    reply->exec();
    
    if (reply->action().status() != Action::AuthorizedStatus ) {
        progressDlg.hide();
        return;
    }

    progressDlg.hide();
    if (!reply->error()) {
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Information"));
        dialog->setModal(true);
        //TO BE FIXED. Have no idea how to show a kmessagebox with a "Details" button.
        QDialogButtonBox *btnbox = new QDialogButtonBox(QDialogButtonBox::Ok);
        KMessageBox::createKMessageBox(dialog, btnbox, QMessageBox::Information, i18nc("@info", "Successfully saved GRUB settings."), QStringList(), QString(), 0, KMessageBox::Notify, reply->data().value("output").toString()); // krazy:exclude=qclasses
        load();
    } else {
        KMessageBox::detailedError(this, i18nc("@info", "Failed to save GRUB settings."), processReply(reply));
    }
}

void KCMGRUB2::slotRemoveOldEntries()
{
#if HAVE_QAPT || HAVE_QPACKAGEKIT
    QPointer<RemoveDialog> removeDlg = new RemoveDialog(m_entries, this);
    if (removeDlg->exec()) {
        load();
    }
    delete removeDlg;
#endif
}
void KCMGRUB2::slotGrubSavedefaultChanged()
{
    m_dirtyBits.setBit(grubSavedefaultDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubHiddenTimeoutToggled(bool checked)
{
    ui->spinBox_hiddenTimeout->setEnabled(checked);
    ui->checkBox_hiddenTimeoutShowTimer->setEnabled(checked);
}
void KCMGRUB2::slotGrubHiddenTimeoutChanged()
{
    m_dirtyBits.setBit(grubHiddenTimeoutDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubHiddenTimeoutQuietChanged()
{
    m_dirtyBits.setBit(grubHiddenTimeoutQuietDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubTimeoutToggled(bool checked)
{
    ui->radioButton_timeout0->setEnabled(checked);
    ui->radioButton_timeout->setEnabled(checked);
    ui->spinBox_timeout->setEnabled(checked && ui->radioButton_timeout->isChecked());
}
void KCMGRUB2::slotGrubTimeoutChanged()
{
    m_dirtyBits.setBit(grubTimeoutDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubDisableRecoveryChanged()
{
    m_dirtyBits.setBit(grubDisableRecoveryDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotMemtestChanged()
{
    m_dirtyBits.setBit(memtestDirty);
    if (initializeAuthorized) emit changed(true);
}
//Security
void KCMGRUB2::slotSecurityChanged()
{
    m_dirtyBits.setBit(securityDirty);
    if (initializeAuthorized) emit changed(true);
}

void KCMGRUB2::slotDeleteUser()
{
    QList<QTableWidgetItem *> selectedUser = ui->users->selectedItems();
    if (selectedUser.count() == 0)
        return;
    int selectedUserNum = selectedUser[0]->row();
    QString selectedUserName = ui->users->item(selectedUserNum, 0)->text();
    m_users.removeOne(selectedUserName);
    m_userPassword.remove(selectedUserName);
    m_userPasswordEncrypted.remove(selectedUserName);
    m_userIsSuper.remove(selectedUserName);
    ui->users->removeRow(selectedUserNum);
    m_dirtyBits.setBit(securityUsersDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotEditUser()
{
    QList<QTableWidgetItem *> selectedUser = ui->users->selectedItems();
    if (selectedUser.count() == 0)
        return;
    int selectedUserNum = selectedUser[0]->row();
    QString selectedUserName = ui->users->item(selectedUserNum, 0)->text();
    QPointer<UserDialog> userDlg = new UserDialog(this, selectedUserName, m_userIsSuper[selectedUserName], m_userPasswordEncrypted[selectedUserName]);
    if(userDlg->exec()) {
        m_userIsSuper[selectedUserName] = userDlg->isSuperUser();
        if (userDlg->requireEncryption()) {
            m_userPasswordEncrypted[selectedUserName] = true;
            m_userPassword[selectedUserName] = pbkdf2Encrypt(userDlg->getPassword());
        } else {
            m_userPasswordEncrypted[selectedUserName] = false;
            m_userPassword[selectedUserName] = userDlg->getPassword();
        }
        ui->users->setItem(selectedUserNum,1,new QTableWidgetItem(userDlg->isSuperUser() ? i18nc("@property", "Yes") : i18nc("@property", "No")));
        ui->users->setItem(selectedUserNum,2,new QTableWidgetItem(userDlg->requireEncryption() ? i18nc("@property", "Encrypted") : i18nc("@property", "Plain")));
        m_dirtyBits.setBit(securityUsersDirty);
        if (initializeAuthorized) emit changed(true);
    }
    delete userDlg;
}
void KCMGRUB2::slotEditGroup(){
    if (m_users.count() == 0){
        KMessageBox::sorry(this, i18nc("@info", "No users found!"));
        return;
    }
    
    QList<QTableWidgetItem *> selectedGroup = ui->groups->selectedItems();
    if (selectedGroup.count() == 0)
        return;
    int selectedGroupNum = selectedGroup[0]->row();
    QString selectedGroupName = ui->groups->item(selectedGroupNum, 0)->text();
    QPointer<GroupDialog> groupDlg = new GroupDialog(this, selectedGroupName, m_users, m_groupFileAllowedUsers[selectedGroupName].split(","), m_groupFileLocked[selectedGroupName]);
    
    if(groupDlg->exec()) {
        m_dirtyBits.setBit(securityGroupsDirty);
        if (initializeAuthorized) emit changed(true);
        m_groupFileAllowedUsers[selectedGroupName] = groupDlg->allowedUsers().join(",");
        if (groupDlg->isLocked()) {
            if (groupDlg->allowedUsers().count() == 0)
                ui->groups->setItem(selectedGroupNum, 2, new QTableWidgetItem(i18nc("@property", "Superusers only")));
            else
                ui->groups->setItem(selectedGroupNum, 2, new QTableWidgetItem(m_groupFileAllowedUsers[selectedGroupName]));
        } else {
            ui->groups->setItem(selectedGroupNum, 2, new QTableWidgetItem(i18nc("@property", "Everyone")));
        }
        m_groupFileLocked[selectedGroupName] = groupDlg->isLocked();
        ui->groups->setItem(selectedGroupNum,1,new QTableWidgetItem(groupDlg->isLocked() ? i18nc("@property", "Yes") : i18nc("@property", "No")));
    }
    delete groupDlg;
}
void KCMGRUB2::slotAddUser(){
    QPointer<UserDialog> userDlg = new UserDialog(this);
    if(userDlg->exec()) {
        int j = ui->users->rowCount();
        ui->users->setRowCount(j+1);
        m_users.append(userDlg->getUserName());
        m_userIsSuper[userDlg->getUserName()] = userDlg->isSuperUser();
        if (userDlg->requireEncryption()) {
            m_userPasswordEncrypted[userDlg->getUserName()] = true;
            m_userPassword[userDlg->getUserName()] = pbkdf2Encrypt(userDlg->getPassword());
        } else {
            m_userPasswordEncrypted[userDlg->getUserName()] = false;
            m_userPassword[userDlg->getUserName()] = userDlg->getPassword();
        }
        ui->users->setItem(j,0,new QTableWidgetItem(userDlg->getUserName()));
        
        ui->users->setItem(j,1,new QTableWidgetItem(userDlg->isSuperUser() ? i18nc("@property", "Yes") : i18nc("@property", "No")));
        
        ui->users->setItem(j,2,new QTableWidgetItem(userDlg->requireEncryption() ? i18nc("@property", "Encrypted") : i18nc("@property", "Plain")));
        m_dirtyBits.setBit(securityUsersDirty);
        if (initializeAuthorized) emit changed(true);
    }
    delete userDlg;
}


void KCMGRUB2::slotGrubDisableOsProberChanged()
{
    m_dirtyBits.setBit(grubDisableOsProberDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotInstallBootloader()
{
    QPointer<InstallDialog> installDlg = new InstallDialog(this);
    installDlg->exec();
    delete installDlg;
}
void KCMGRUB2::slotGrubGfxmodeChanged()
{
    if (ui->kcombobox_gfxmode->currentIndex() == 0) {
        bool ok;
        QRegExp regExp("\\d{3,4}x\\d{3,4}(x\\d{1,2})?");
        QString resolution = RegExpInputDialog::getText(this, i18nc("@title:window", "Enter screen resolution"), i18nc("@label:textbox", "Please enter a GRUB resolution:"), QString(), regExp, &ok);
        if (ok) {
            if (!m_resolutions.contains(resolution)) {
                QString gfxpayload = ui->kcombobox_gfxpayload->itemData(ui->kcombobox_gfxpayload->currentIndex()).toString();
                m_resolutions.append(resolution);
                sortResolutions();
                showResolutions();
                ui->kcombobox_gfxpayload->setCurrentIndex(ui->kcombobox_gfxpayload->findData(gfxpayload));
            }
            ui->kcombobox_gfxmode->setCurrentIndex(ui->kcombobox_gfxmode->findData(resolution));
        } else {
            ui->kcombobox_gfxmode->setCurrentIndex(ui->kcombobox_gfxmode->findData("640x480"));
        }
    }
    m_dirtyBits.setBit(grubGfxmodeDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubGfxpayloadLinuxChanged()
{
    if (ui->kcombobox_gfxpayload->currentIndex() == 0) {
        bool ok;
        QRegExp regExp("\\d{3,4}x\\d{3,4}(x\\d{1,2})?");
        QString resolution = RegExpInputDialog::getText(this, i18nc("@title:window", "Enter screen resolution"), i18nc("@label:textbox", "Please enter a GRUB resolution:"), QString(), regExp, &ok);
        if (ok) {
            if (!m_resolutions.contains(resolution)) {
                QString gfxmode = ui->kcombobox_gfxmode->itemData(ui->kcombobox_gfxmode->currentIndex()).toString();
                m_resolutions.append(resolution);
                sortResolutions();
                showResolutions();
                ui->kcombobox_gfxmode->setCurrentIndex(ui->kcombobox_gfxmode->findData(gfxmode));
            }
            ui->kcombobox_gfxpayload->setCurrentIndex(ui->kcombobox_gfxpayload->findData(resolution));
        } else {
            ui->kcombobox_gfxpayload->setCurrentIndex(ui->kcombobox_gfxpayload->findData(QString()));
        }
    }
    m_dirtyBits.setBit(grubGfxpayloadLinuxDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubColorNormalChanged()
{
    m_dirtyBits.setBit(grubColorNormalDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubColorHighlightChanged()
{
    m_dirtyBits.setBit(grubColorHighlightDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slowGrubBackgroundChanged()
{
    ui->kpushbutton_preview->setEnabled(!ui->kurlrequester_background->text().isEmpty());
    m_dirtyBits.setBit(grubBackgroundDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotPreviewGrubBackground()
{
    QFile file(ui->kurlrequester_background->url().toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    //TODO: Need something more elegant.
    QDialog *dialog = new QDialog(this);
    QLabel *label = new QLabel(dialog);
    label->setPixmap(QPixmap::fromImage(QImage::fromData(file.readAll())).scaled(QDesktopWidget().screenGeometry(this).size()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    KMessageBox::information(dialog, i18nc("@info", "Press <shortcut>Escape</shortcut> to exit fullscreen mode."), QString(), "GRUBFullscreenPreview");
}
void KCMGRUB2::slotCreateGrubBackground()
{
#if HAVE_IMAGEMAGICK
    QPointer<ConvertDialog> convertDlg = new ConvertDialog(this);
    QString resolution = ui->kcombobox_gfxmode->itemData(ui->kcombobox_gfxmode->currentIndex()).toString();
    convertDlg->setResolution(resolution.section('x', 0, 0).toInt(), resolution.section('x', 1, 1).toInt());
    connect(convertDlg, SIGNAL(splashImageCreated(QString)), ui->kurlrequester_background, SLOT(setText(QString)));
    convertDlg->exec();
    delete convertDlg;
#endif
}
void KCMGRUB2::slotGrubThemeChanged()
{
    m_dirtyBits.setBit(grubThemeDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubCmdlineLinuxDefaultChanged()
{
    m_dirtyBits.setBit(grubCmdlineLinuxDefaultDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubCmdlineLinuxChanged()
{
    m_dirtyBits.setBit(grubCmdlineLinuxDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubTerminalChanged()
{
    QString grubTerminal = ui->klineedit_terminal->text();
    ui->klineedit_terminalInput->setReadOnly(!grubTerminal.isEmpty());
    ui->klineedit_terminalOutput->setReadOnly(!grubTerminal.isEmpty());
    ui->klineedit_terminalInput->setText(!grubTerminal.isEmpty() ? grubTerminal : unquoteWord(m_settings.value("GRUB_TERMINAL_INPUT")));
    ui->klineedit_terminalOutput->setText(!grubTerminal.isEmpty() ? grubTerminal : unquoteWord(m_settings.value("GRUB_TERMINAL_OUTPUT")));
    m_dirtyBits.setBit(grubTerminalDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubTerminalInputChanged()
{
    m_dirtyBits.setBit(grubTerminalInputDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubTerminalOutputChanged()
{
    m_dirtyBits.setBit(grubTerminalOutputDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubDistributorChanged()
{
    m_dirtyBits.setBit(grubDistributorDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubSerialCommandChanged()
{
    m_dirtyBits.setBit(grubSerialCommandDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubInitTuneChanged()
{
    m_dirtyBits.setBit(grubInitTuneDirty);
    if (initializeAuthorized) emit changed(true);
}
void KCMGRUB2::slotGrubDisableLinuxUuidChanged()
{
    m_dirtyBits.setBit(grubDisableLinuxUuidDirty);
    if (initializeAuthorized) emit changed(true);
}

void KCMGRUB2::slotUpdateSuggestions()
{
    if (!sender()->isWidgetType()) {
        return;
    }

    KLineEdit *lineEdit = 0;
    if (ui->kpushbutton_cmdlineDefaultSuggestions->isDown()) {
        lineEdit = ui->klineedit_cmdlineDefault;
    } else if (ui->kpushbutton_cmdlineSuggestions->isDown()) {
        lineEdit = ui->klineedit_cmdline;
    } else if (ui->kpushbutton_terminalSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminal;
    } else if (ui->kpushbutton_terminalInputSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminalInput;
    } else if (ui->kpushbutton_terminalOutputSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminalOutput;
    } else {
        return;
    }

    Q_FOREACH(QAction *action, qobject_cast<const QWidget*>(sender())->actions()) {
        if (!action->isCheckable()) {
            action->setCheckable(true);
        }
        action->setChecked(lineEdit->text().contains(QRegExp(QString("\\b%1\\b").arg(action->data().toString()))));
    }
}
void KCMGRUB2::slotTriggeredSuggestion(QAction *action)
{
    KLineEdit *lineEdit = 0;
    void (KCMGRUB2::*updateFunction)() = 0;
    if (ui->kpushbutton_cmdlineDefaultSuggestions->isDown()) {
        lineEdit = ui->klineedit_cmdlineDefault;
        updateFunction = &KCMGRUB2::slotGrubCmdlineLinuxDefaultChanged;
    } else if (ui->kpushbutton_cmdlineSuggestions->isDown()) {
        lineEdit = ui->klineedit_cmdline;
        updateFunction = &KCMGRUB2::slotGrubCmdlineLinuxChanged;
    } else if (ui->kpushbutton_terminalSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminal;
        updateFunction = &KCMGRUB2::slotGrubTerminalChanged;
    } else if (ui->kpushbutton_terminalInputSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminalInput;
        updateFunction = &KCMGRUB2::slotGrubTerminalInputChanged;
    } else if (ui->kpushbutton_terminalOutputSuggestions->isDown()) {
        lineEdit = ui->klineedit_terminalOutput;
        updateFunction = &KCMGRUB2::slotGrubTerminalOutputChanged;
    } else {
        return;
    }

    QString lineEditText = lineEdit->text();
    if (!action->isChecked()) {
        lineEdit->setText(lineEditText.remove(QRegExp(QString("\\b%1\\b").arg(action->data().toString()))).simplified());
    } else {
        lineEdit->setText(lineEditText.isEmpty() ? action->data().toString() : lineEditText + ' ' + action->data().toString());
    }
    (this->*updateFunction)();
}

void KCMGRUB2::setupObjects()
{
    setButtons(Default | Apply);
    setNeedsAuthorization(true);

    m_dirtyBits.resize(lastDirtyBit);

    QTreeView *view = new QTreeView(ui->kcombobox_default);
    view->setHeaderHidden(true);
    view->setItemsExpandable(false);
    view->setRootIsDecorated(false);
    ui->kcombobox_default->setView(view);

    ui->kpushbutton_install->setIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    ui->kpushbutton_remove->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    ui->kpushbutton_remove->setVisible(HAVE_QAPT || HAVE_QPACKAGEKIT);

    QPixmap black(16, 16), transparent(16, 16);
    black.fill(Qt::black);
    transparent.fill(Qt::transparent);
    ui->kcombobox_normalForeground->addItem(QIcon(black), i18nc("@item:inlistbox Refers to color.", "Black"), "black");
    ui->kcombobox_highlightForeground->addItem(QIcon(black), i18nc("@item:inlistbox Refers to color.", "Black"), "black");
    ui->kcombobox_normalBackground->addItem(QIcon(transparent), i18nc("@item:inlistbox Refers to color.", "Transparent"), "black");
    ui->kcombobox_highlightBackground->addItem(QIcon(transparent), i18nc("@item:inlistbox Refers to color.", "Transparent"), "black");
    QHash<QString, QString> colors;
    colors.insertMulti("blue", i18nc("@item:inlistbox Refers to color.", "Blue"));
    colors.insertMulti("blue", "blue");
    colors.insertMulti("cyan", i18nc("@item:inlistbox Refers to color.", "Cyan"));
    colors.insertMulti("cyan", "cyan");
    colors.insertMulti("dark-gray", i18nc("@item:inlistbox Refers to color.", "Dark Gray"));
    colors.insertMulti("dark-gray", "darkgray");
    colors.insertMulti("green", i18nc("@item:inlistbox Refers to color.", "Green"));
    colors.insertMulti("green", "green");
    colors.insertMulti("light-cyan", i18nc("@item:inlistbox Refers to color.", "Light Cyan"));
    colors.insertMulti("light-cyan", "lightcyan");
    colors.insertMulti("light-blue", i18nc("@item:inlistbox Refers to color.", "Light Blue"));
    colors.insertMulti("light-blue", "lightblue");
    colors.insertMulti("light-green", i18nc("@item:inlistbox Refers to color.", "Light Green"));
    colors.insertMulti("light-green", "lightgreen");
    colors.insertMulti("light-gray", i18nc("@item:inlistbox Refers to color.", "Light Gray"));
    colors.insertMulti("light-gray", "lightgray");
    colors.insertMulti("light-magenta", i18nc("@item:inlistbox Refers to color.", "Light Magenta"));
    colors.insertMulti("light-magenta", "magenta");
    colors.insertMulti("light-red", i18nc("@item:inlistbox Refers to color.", "Light Red"));
    colors.insertMulti("light-red", "orangered");
    colors.insertMulti("magenta", i18nc("@item:inlistbox Refers to color.", "Magenta"));
    colors.insertMulti("magenta", "darkmagenta");
    colors.insertMulti("red", i18nc("@item:inlistbox Refers to color.", "Red"));
    colors.insertMulti("red", "red");
    colors.insertMulti("white", i18nc("@item:inlistbox Refers to color.", "White"));
    colors.insertMulti("white", "white");
    colors.insertMulti("yellow", i18nc("@item:inlistbox Refers to color.", "Yellow"));
    colors.insertMulti("yellow", "yellow");
    QHash<QString, QString>::const_iterator it = colors.constBegin();
    QHash<QString, QString>::const_iterator end = colors.constEnd();
    for (; it != end; it += 2) {
        QPixmap color(16, 16);
        color.fill(QColor(colors.values(it.key()).at(0)));
        ui->kcombobox_normalForeground->addItem(QIcon(color), colors.values(it.key()).at(1), it.key());
        ui->kcombobox_highlightForeground->addItem(QIcon(color), colors.values(it.key()).at(1), it.key());
        ui->kcombobox_normalBackground->addItem(QIcon(color), colors.values(it.key()).at(1), it.key());
        ui->kcombobox_highlightBackground->addItem(QIcon(color), colors.values(it.key()).at(1), it.key());
    }
    ui->kcombobox_normalForeground->setCurrentIndex(ui->kcombobox_normalForeground->findData("light-gray"));
    ui->kcombobox_normalBackground->setCurrentIndex(ui->kcombobox_normalBackground->findData("black"));
    ui->kcombobox_highlightForeground->setCurrentIndex(ui->kcombobox_highlightForeground->findData("black"));
    ui->kcombobox_highlightBackground->setCurrentIndex(ui->kcombobox_highlightBackground->findData("light-gray"));

    ui->kpushbutton_preview->setIcon(QIcon::fromTheme(QStringLiteral("image-png")));
    ui->kpushbutton_create->setIcon(QIcon::fromTheme(QStringLiteral("insert-image")));
    ui->kpushbutton_create->setVisible(HAVE_IMAGEMAGICK);

    ui->kpushbutton_cmdlineDefaultSuggestions->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    ui->kpushbutton_cmdlineDefaultSuggestions->setMenu(new QMenu(ui->kpushbutton_cmdlineDefaultSuggestions));
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Quiet Boot"))->setData("quiet");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Show Splash Screen"))->setData("splash");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Disable Plymouth"))->setData("noplymouth");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off ACPI"))->setData("acpi=off");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off APIC"))->setData("noapic");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off Local APIC"))->setData("nolapic");
    ui->kpushbutton_cmdlineDefaultSuggestions->menu()->addAction(i18nc("@action:inmenu", "Single User Mode"))->setData("single");
    ui->kpushbutton_cmdlineSuggestions->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    ui->kpushbutton_cmdlineSuggestions->setMenu(new QMenu(ui->kpushbutton_cmdlineSuggestions));
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Quiet Boot"))->setData("quiet");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Show Splash Screen"))->setData("splash");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Disable Plymouth"))->setData("noplymouth");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off ACPI"))->setData("acpi=off");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off APIC"))->setData("noapic");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Turn Off Local APIC"))->setData("nolapic");
    ui->kpushbutton_cmdlineSuggestions->menu()->addAction(i18nc("@action:inmenu", "Single User Mode"))->setData("single");
    ui->kpushbutton_terminalSuggestions->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    ui->kpushbutton_terminalSuggestions->setMenu(new QMenu(ui->kpushbutton_terminalSuggestions));
    ui->kpushbutton_terminalSuggestions->menu()->addAction(i18nc("@action:inmenu", "PC BIOS && EFI Console"))->setData("console");
    ui->kpushbutton_terminalSuggestions->menu()->addAction(i18nc("@action:inmenu", "Serial Terminal"))->setData("serial");
    ui->kpushbutton_terminalSuggestions->menu()->addAction(i18nc("@action:inmenu 'Open' is an adjective here, not a verb. 'Open Firmware' is a former IEEE standard.", "Open Firmware Console"))->setData("ofconsole");
    ui->kpushbutton_terminalInputSuggestions->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    ui->kpushbutton_terminalInputSuggestions->setMenu(new QMenu(ui->kpushbutton_terminalInputSuggestions));
    ui->kpushbutton_terminalInputSuggestions->menu()->addAction(i18nc("@action:inmenu", "PC BIOS && EFI Console"))->setData("console");
    ui->kpushbutton_terminalInputSuggestions->menu()->addAction(i18nc("@action:inmenu", "Serial Terminal"))->setData("serial");
    ui->kpushbutton_terminalInputSuggestions->menu()->addAction(i18nc("@action:inmenu 'Open' is an adjective here, not a verb. 'Open Firmware' is a former IEEE standard.", "Open Firmware Console"))->setData("ofconsole");
    ui->kpushbutton_terminalInputSuggestions->menu()->addAction(i18nc("@action:inmenu", "PC AT Keyboard (Coreboot)"))->setData("at_keyboard");
    ui->kpushbutton_terminalInputSuggestions->menu()->addAction(i18nc("@action:inmenu", "USB Keyboard (HID Boot Protocol)"))->setData("usb_keyboard");
    ui->kpushbutton_terminalOutputSuggestions->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    ui->kpushbutton_terminalOutputSuggestions->setMenu(new QMenu(ui->kpushbutton_terminalOutputSuggestions));
    ui->kpushbutton_terminalOutputSuggestions->menu()->addAction(i18nc("@action:inmenu", "PC BIOS && EFI Console"))->setData("console");
    ui->kpushbutton_terminalOutputSuggestions->menu()->addAction(i18nc("@action:inmenu", "Serial Terminal"))->setData("serial");
    ui->kpushbutton_terminalOutputSuggestions->menu()->addAction(i18nc("@action:inmenu 'Open' is an adjective here, not a verb. 'Open Firmware' is a former IEEE standard.", "Open Firmware Console"))->setData("ofconsole");
    ui->kpushbutton_terminalOutputSuggestions->menu()->addAction(i18nc("@action:inmenu", "Graphics Mode Output"))->setData("gfxterm");
    ui->kpushbutton_terminalOutputSuggestions->menu()->addAction(i18nc("@action:inmenu", "VGA Text Output (Coreboot)"))->setData("vga_text");
}
void KCMGRUB2::setupConnections()
{
    connect(ui->kcombobox_default, SIGNAL(activated(int)), this, SLOT(changed()));
    connect(ui->kpushbutton_remove, SIGNAL(clicked(bool)), this, SLOT(slotRemoveOldEntries()));
    connect(ui->checkBox_savedefault, SIGNAL(clicked(bool)), this, SLOT(slotGrubSavedefaultChanged()));

    connect(ui->checkBox_hiddenTimeout, SIGNAL(toggled(bool)), this, SLOT(slotGrubHiddenTimeoutToggled(bool)));
    connect(ui->checkBox_hiddenTimeout, SIGNAL(clicked(bool)), this, SLOT(slotGrubHiddenTimeoutChanged()));
    connect(ui->spinBox_hiddenTimeout, SIGNAL(valueChanged(int)), this, SLOT(slotGrubHiddenTimeoutChanged()));
    connect(ui->checkBox_hiddenTimeoutShowTimer, SIGNAL(clicked(bool)), this, SLOT(slotGrubHiddenTimeoutQuietChanged()));

    connect(ui->checkBox_timeout, SIGNAL(toggled(bool)), this, SLOT(slotGrubTimeoutToggled(bool)));
    connect(ui->checkBox_timeout, SIGNAL(clicked(bool)), this, SLOT(slotGrubTimeoutChanged()));
    connect(ui->radioButton_timeout0, SIGNAL(clicked(bool)), this, SLOT(slotGrubTimeoutChanged()));
    connect(ui->radioButton_timeout, SIGNAL(toggled(bool)), ui->spinBox_timeout, SLOT(setEnabled(bool)));
    connect(ui->radioButton_timeout, SIGNAL(clicked(bool)), this, SLOT(slotGrubTimeoutChanged()));
    connect(ui->spinBox_timeout, SIGNAL(valueChanged(int)), this, SLOT(slotGrubTimeoutChanged()));

    connect(ui->checkBox_recovery, SIGNAL(clicked(bool)), this, SLOT(slotGrubDisableRecoveryChanged()));
    connect(ui->checkBox_memtest, SIGNAL(clicked(bool)), this, SLOT(slotMemtestChanged()));
    //Security
    connect(ui->secEnabled, SIGNAL(clicked(bool)), this, SLOT(slotSecurityChanged()));
    connect(ui->secEnabled, SIGNAL(clicked(bool)), ui->usersGroup, SLOT(setEnabled(bool)));
    connect(ui->secEnabled, SIGNAL(clicked(bool)), ui->groupsGroup, SLOT(setEnabled(bool)));
    //user
    connect(ui->userDel, SIGNAL(clicked(bool)), this, SLOT(slotDeleteUser()));
    connect(ui->userMod, SIGNAL(clicked(bool)), this, SLOT(slotEditUser()));
    connect(ui->userAdd, SIGNAL(clicked(bool)), this, SLOT(slotAddUser()));
    //Group
    connect(ui->groupMod, SIGNAL(clicked(bool)), this, SLOT(slotEditGroup()));
    
    connect(ui->checkBox_osProber, SIGNAL(clicked(bool)), this, SLOT(slotGrubDisableOsProberChanged()));

    connect(ui->kcombobox_gfxmode, SIGNAL(activated(int)), this, SLOT(slotGrubGfxmodeChanged()));
    connect(ui->kcombobox_gfxpayload, SIGNAL(activated(int)), this, SLOT(slotGrubGfxpayloadLinuxChanged()));

    connect(ui->kcombobox_normalForeground, SIGNAL(activated(int)), this, SLOT(slotGrubColorNormalChanged()));
    connect(ui->kcombobox_normalBackground, SIGNAL(activated(int)), this, SLOT(slotGrubColorNormalChanged()));
    connect(ui->kcombobox_highlightForeground, SIGNAL(activated(int)), this, SLOT(slotGrubColorHighlightChanged()));
    connect(ui->kcombobox_highlightBackground, SIGNAL(activated(int)), this, SLOT(slotGrubColorHighlightChanged()));

    connect(ui->kurlrequester_background, SIGNAL(textChanged(QString)), this, SLOT(slowGrubBackgroundChanged()));
    connect(ui->kpushbutton_preview, SIGNAL(clicked(bool)), this, SLOT(slotPreviewGrubBackground()));
    connect(ui->kpushbutton_create, SIGNAL(clicked(bool)), this, SLOT(slotCreateGrubBackground()));
    connect(ui->kurlrequester_theme, SIGNAL(textChanged(QString)), this, SLOT(slotGrubThemeChanged()));

    connect(ui->klineedit_cmdlineDefault, SIGNAL(textEdited(QString)), this, SLOT(slotGrubCmdlineLinuxDefaultChanged()));
    connect(ui->kpushbutton_cmdlineDefaultSuggestions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpdateSuggestions()));
    connect(ui->kpushbutton_cmdlineDefaultSuggestions->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggeredSuggestion(QAction*)));
    connect(ui->klineedit_cmdline, SIGNAL(textEdited(QString)), this, SLOT(slotGrubCmdlineLinuxChanged()));
    connect(ui->kpushbutton_cmdlineSuggestions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpdateSuggestions()));
    connect(ui->kpushbutton_cmdlineSuggestions->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggeredSuggestion(QAction*)));

    connect(ui->klineedit_terminal, SIGNAL(textEdited(QString)), this, SLOT(slotGrubTerminalChanged()));
    connect(ui->kpushbutton_terminalSuggestions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpdateSuggestions()));
    connect(ui->kpushbutton_terminalSuggestions->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggeredSuggestion(QAction*)));
    connect(ui->klineedit_terminalInput, SIGNAL(textEdited(QString)), this, SLOT(slotGrubTerminalInputChanged()));
    connect(ui->kpushbutton_terminalInputSuggestions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpdateSuggestions()));
    connect(ui->kpushbutton_terminalInputSuggestions->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggeredSuggestion(QAction*)));
    connect(ui->klineedit_terminalOutput, SIGNAL(textEdited(QString)), this, SLOT(slotGrubTerminalOutputChanged()));
    connect(ui->kpushbutton_terminalOutputSuggestions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpdateSuggestions()));
    connect(ui->kpushbutton_terminalOutputSuggestions->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggeredSuggestion(QAction*)));

    connect(ui->klineedit_distributor, SIGNAL(textEdited(QString)), this, SLOT(slotGrubDistributorChanged()));
    connect(ui->klineedit_serial, SIGNAL(textEdited(QString)), this, SLOT(slotGrubSerialCommandChanged()));
    connect(ui->klineedit_initTune, SIGNAL(textEdited(QString)), this, SLOT(slotGrubInitTuneChanged()));
    connect(ui->checkBox_uuid, SIGNAL(clicked(bool)), this, SLOT(slotGrubDisableLinuxUuidChanged()));

    connect(ui->kpushbutton_install, SIGNAL(clicked(bool)), this, SLOT(slotInstallBootloader()));
}

QString KCMGRUB2::convertToGRUBFileName(const QString &fileName)
{
    QString grubFileName = fileName;
    QString mountpoint = KMountPoint::currentMountPoints().findByPath(grubFileName)->mountPoint();
    if (m_devices.contains(mountpoint)) {
        if (mountpoint.compare("/") != 0) {
            grubFileName.remove(0, mountpoint.length());
        }
        grubFileName.prepend(m_devices.value(mountpoint));
    }
    return grubFileName;
}
QString KCMGRUB2::convertToLocalFileName(const QString &grubFileName)
{
    QString fileName = grubFileName;
    QHash<QString, QString>::const_iterator it = m_devices.constBegin();
    QHash<QString, QString>::const_iterator end = m_devices.constEnd();
    for (; it != end; ++it) {
        if (fileName.startsWith(it.value())) {
            fileName.remove(0, it.value().length());
            if (it.key().compare("/") != 0) {
                fileName.prepend(it.key());
            }
            break;
        }
    }
    return fileName;
}

ExecuteJob * KCMGRUB2::loadFile(GrubFile grubFile)
{
    Action loadAction("org.kde.kcontrol.kcmgrub2.initialize");
    loadAction.setHelperId("org.kde.kcontrol.kcmgrub2");
    loadAction.addArgument("actionType", actionLoad);
    loadAction.addArgument("grubFile", grubFile);

    return loadAction.execute();
}
QString KCMGRUB2::readFile(GrubFile grubFile)
{
    QString fileName;
    switch (grubFile) {
    case GrubMenuFile:
        fileName = GRUB_MENU;
        break;
    case GrubConfigurationFile:
        fileName = GRUB_CONFIG;
        break;
    case GrubEnvironmentFile:
        fileName = GRUB_ENV;
        break;
    case GrubMemtestFile:
        return QString();
    case GrubGroupFile:
        return QString();
    }

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }

    ExecuteJob *reply = loadFile(grubFile);
    reply->exec();
    
    if (reply->action().status() == Action::AuthorizedStatus ) {
        initializeAuthorized = true;
    }
    
    if (reply->error()) {
        qDebug() << "Error loading:" << fileName;
        qDebug() << "Error description:" << processReply(reply);
        return QString();
    }
    return QString::fromLocal8Bit(reply->data().value("rawFileContents").toByteArray());
}
void KCMGRUB2::readEntries()
{
    QString fileContents = readFile(GrubMenuFile);

    m_entries.clear();
    parseEntries(fileContents);
}
void KCMGRUB2::readSettings()
{
    QString fileContents = readFile(GrubConfigurationFile);

    m_settings.clear();
    parseSettings(fileContents);
}
void KCMGRUB2::readEnv()
{
    QString fileContents = readFile(GrubEnvironmentFile);

    m_env.clear();
    parseEnv(fileContents);
}
void KCMGRUB2::readMemtest()
{
    bool memtest = QFile::exists(GRUB_MEMTEST);
    if (memtest) {
        m_memtest = true;
        m_memtestOn = (bool)(QFile::permissions(GRUB_MEMTEST) & (QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther));
        return;
    }

    ExecuteJob *reply = loadFile(GrubMemtestFile);
    if (!reply->exec()) {
        qDebug() << "Error loading:" << GRUB_MEMTEST;
        qDebug() << "Error description:" << processReply(reply);
        return;
    }
    m_memtest = reply->data().value("memtest").toBool();
    if (m_memtest) {
        m_memtestOn = reply->data().value("memtestOn").toBool();
    }
}
void KCMGRUB2::readDevices()
{
    QStringList mountPoints;
    Q_FOREACH(const KMountPoint::Ptr mp, KMountPoint::currentMountPoints()) {
        if (mp->mountedFrom().startsWith(QLatin1String("/dev"))) {
            mountPoints.append(mp->mountPoint());
        }
    }

    Action probeAction("org.kde.kcontrol.kcmgrub2.initialize");
    probeAction.setHelperId("org.kde.kcontrol.kcmgrub2");
    probeAction.addArgument("actionType", actionProbe);
    probeAction.addArgument("mountPoints", mountPoints);
    
    QProgressDialog progressDlg(this, Qt::Dialog);
        progressDlg.setWindowTitle(i18nc("@title:window", "Probing devices"));
        progressDlg.setLabelText(i18nc("@info:progress", "Probing devices for their GRUB names..."));
        progressDlg.setCancelButton(0);
        progressDlg.setModal(true);
        QProgressBar * mProgressBar = new QProgressBar(this);
        progressDlg.setBar(mProgressBar);
        progressDlg.show();
        
    ExecuteJob *reply = probeAction.execute();
    //connect(reply, SIGNAL(progressStep(int)), mProgressBar, SLOT(setValue(int)));
    reply->exec();
    
    if (probeAction.status() != Action::AuthorizedStatus ) {
        progressDlg.hide();
        return;
    }
    
    progressDlg.hide();
    
    if (reply->error()) {
        KMessageBox::detailedError(this, i18nc("@info", "Failed to get GRUB device names."), processReply(reply));
        return;
    }// else {
     // progressDlg.hide();
//}
    QStringList grubPartitions = reply->data().value("grubPartitions").toStringList();
    if (mountPoints.size() != grubPartitions.size()) {
        KMessageBox::error(this, i18nc("@info", "Helper returned malformed device list."));
        return;
    }

    m_devices.clear();
    for (int i = 0; i < mountPoints.size(); i++) {
        m_devices[mountPoints.at(i)] = grubPartitions.at(i);
    }
}
void KCMGRUB2::readResolutions()
{
    Action probeVbeAction("org.kde.kcontrol.kcmgrub2.initialize");
    probeVbeAction.setHelperId("org.kde.kcontrol.kcmgrub2");
    probeVbeAction.addArgument("actionType", actionProbevbe);

    ExecuteJob *reply = probeVbeAction.execute();
    if (!reply->exec()) {
        return;
    }

    m_resolutions.clear();
    m_resolutions = reply->data().value("gfxmodes").toStringList();
}

//Security
void KCMGRUB2::parseGroupDir()
{
    m_groupFilesList.clear();
    m_groupFilesContent.clear();
    QDir GroupDir(GRUB_CONFIGDIR);
    //exclude GRUB_SECURITY
    QRegExp filters(QString("[1-9]\\d{1,}.*"));
    m_groupFilesList = GroupDir.entryList(QDir::Files).filter(filters);
    m_groupFilesList.removeDuplicates();
    for( int i=0; i<m_groupFilesList.count(); ++i ) {
        Action readGroupAction("org.kde.kcontrol.kcmgrub2.initialize");
        readGroupAction.setHelperId("org.kde.kcontrol.kcmgrub2");
        readGroupAction.addArgument("actionType", actionLoad);
        readGroupAction.addArgument("grubFile", GrubGroupFile);
        readGroupAction.addArgument("groupFile", m_groupFilesList[i]);
        ExecuteJob * readGroupJob = readGroupAction.execute();
        if (!readGroupJob->exec()) {
           qDebug() << "Error loading:" << m_groupFilesList[i];
           qDebug() << "Error description:" << readGroupJob->errorString();
           m_groupFilesContent[m_groupFilesList[i]] = QString();
        } else {
           m_groupFilesContent[m_groupFilesList[i]] = QString::fromLocal8Bit(readGroupJob->data().value("rawFileContents").toByteArray());
        }
    }
    //parse GRUB_SECURITY
    Action readHeaderAction("org.kde.kcontrol.kcmgrub2.initialize");
     readHeaderAction.setHelperId("org.kde.kcontrol.kcmgrub2");
     readHeaderAction.addArgument("actionType", actionLoad);
     readHeaderAction.addArgument("grubFile", GrubGroupFile);
     readHeaderAction.addArgument("groupFile", GRUB_SECURITY);
     
     ExecuteJob * readHeaderJob = readHeaderAction.execute();
     if (!readHeaderJob->exec()) {
           qDebug() << "Error loading:" << GRUB_SECURITY;
           qDebug() << "Error description:" << readHeaderJob->errorString();
           m_headerFile = QString();
     } else {
           m_headerFile = QString::fromLocal8Bit(readHeaderJob->data().value("rawFileContents").toByteArray()).replace(";", "\n");
     }
     
    m_security = readHeaderJob->data().value("security").toBool();
    if (m_security) {
        m_securityOn = readHeaderJob->data().value("securityOn").toBool();
    }
     
    getSuperUsers();
    getUsers();
    getGroups();
}
void KCMGRUB2::getSuperUsers()
{
    m_superUsers.clear();
    QRegExp regex("set( +)superusers *= *\"?([a-zA-Z0-9,]{1,})\"?");
     regex.setMinimal(false);
    int superusersmatch = regex.indexIn(m_headerFile);
    QString superusersstring = regex.cap(2);
    m_superUsers = superusersstring.split(",", QString::SkipEmptyParts);
}
void KCMGRUB2::getUsers()
{
    m_users.clear();
    m_userPasswordEncrypted.clear();
    m_userPassword.clear();
    ui->users->setRowCount(0);
    ui->users->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->users->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList tableHeader;  
    tableHeader << i18nc("@header", "Name") << i18nc("@header", "Superuser") << i18nc("@header", "Password type");  
    ui->users->setHorizontalHeaderLabels(tableHeader); 
    
    QRegExp cryptoreg(" *password_pbkdf2 +[a-zA-Z0-9]{1,} +[^ \n#]+ *");
     cryptoreg.setMinimal(false);
    QRegExp plainreg(" *password +[a-zA-Z0-9]{1,} +[^ \n#]+ *");
     plainreg.setMinimal(false);
    
    QStringList headerFileLines = m_headerFile.split("\n", QString::SkipEmptyParts);
    
    int j = 0;
    for( int i=0; i<headerFileLines.count(); ++i ) {
        QStringList currentUser = headerFileLines[i].split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (cryptoreg.exactMatch(headerFileLines[i]) || plainreg.exactMatch(headerFileLines[i])) {
            m_users.append(currentUser[1]);
            m_userPassword[currentUser[1]] = currentUser[2];
            ui->users->setRowCount(j + 1);
            ui->users->setItem(j,0,new QTableWidgetItem(m_users[j]));
            
            if (m_superUsers.contains(m_users[j])) {
                m_userIsSuper[m_users[j]] = true;
                ui->users->setItem(j,1,new QTableWidgetItem(i18nc("@property", "Yes")));
            } else {
                m_userIsSuper[m_users[j]] = false;
                ui->users->setItem(j,1,new QTableWidgetItem(i18nc("@property", "No")));
            }
            
            if (cryptoreg.exactMatch(headerFileLines[i])) {
                m_userPasswordEncrypted[currentUser[1]] = true;
                ui->users->setItem(j,2,new QTableWidgetItem(i18nc("@property", "Encrypted")));
            } else {
                m_userPasswordEncrypted[currentUser[1]] = false;
                ui->users->setItem(j,2,new QTableWidgetItem(i18nc("@property", "Plain")));
            }
            
            
            ++j;
        }
    }
    
}
void KCMGRUB2::getGroups()
{
    m_groupFileLocked.clear();
    m_groupFileAllowedUsers.clear();
    ui->groups->setRowCount(0);
    ui->groups->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->groups->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList tableHeader;  
    tableHeader << i18nc("@header", "Name") << i18nc("@header", "Locked") << i18nc("@header", "Allowed users");  
    ui->groups->setHorizontalHeaderLabels(tableHeader); 
    QRegExp hasentryregex("menuentry .+\\{");
     hasentryregex.setMinimal(true);
    QRegExp lockedregex("menuentry +--users \"?[a-zA-Z0-9,]{0,}\"? .*\\{");
     lockedregex.setMinimal(true);
    QRegExp unlockedregex("menuentry +--unrestricted .*\\{");
     unlockedregex.setMinimal(true);
    QRegExp usersregex("--users \"?([a-zA-Z0-9,]{0,})\"? .*\\{");
     usersregex.setMinimal(true);
    //strip
    QRegExp menustripregex("menuentry (--users \"?[a-zA-Z0-9,]{0,}\"? |--unrestricted )? *");
     menustripregex.setMinimal(false);
    QRegExp substripregex("submenu (--users \"?[a-zA-Z0-9,]{0,}\"? |--unrestricted )? *");
     substripregex.setMinimal(false);
    
    int j = 0;
    for( int i=0; i<m_groupFilesContent.count(); ++i ) {
        int hasentrymatch = hasentryregex.indexIn(m_groupFilesContent[m_groupFilesList[i]]);
        int lockedmatch = lockedregex.indexIn(m_groupFilesContent[m_groupFilesList[i]]);
        int unlockedmatch = unlockedregex.indexIn(m_groupFilesContent[m_groupFilesList[i]]);
        int usersmatch = usersregex.indexIn(m_groupFilesContent[m_groupFilesList[i]]);
        m_groupFilesContent[m_groupFilesList[i]].replace(menustripregex, MENUPLACEHOLDER);
        m_groupFilesContent[m_groupFilesList[i]].replace(substripregex, SUBPLACEHOLDER);
        //qDebug() << m_groupFilesContent[m_groupFilesList[i]];
        QString usersmatchstring = usersregex.cap(1);
        if (hasentrymatch != -1) {
            ui->groups->setRowCount(j + 1);
            ui->groups->setItem(j,0,new QTableWidgetItem(m_groupFilesList[i]));
            if (lockedmatch != -1 || unlockedmatch == -1) {
                m_groupFileLocked[m_groupFilesList[i]] = true;
                m_groupFileAllowedUsers[m_groupFilesList[i]] = usersmatchstring;
                ui->groups->setItem(j,1,new QTableWidgetItem(i18nc("@property", "Yes")));
                if (usersmatch!= -1 && usersmatchstring != "") {
                    ui->groups->setItem(j,2,new QTableWidgetItem(usersmatchstring));
                } else {
                    ui->groups->setItem(j,2,new QTableWidgetItem(i18nc("@property", "Superusers only")));
                }
            } else {
                m_groupFileLocked[m_groupFilesList[i]] = false;
                ui->groups->setItem(j,1,new QTableWidgetItem(i18nc("@property", "No")));
                ui->groups->setItem(j,2,new QTableWidgetItem(i18nc("@property", "Everyone")));
            }
            //qDebug() << "File" << GRUB_CONFIGDIR + m_groupFilesList[i] << "processed.";
            ++j;
        } else {
            //qDebug() << "File" << GRUB_CONFIGDIR + m_groupFilesList[i] << "ignored.";
        }
    }
}
QString KCMGRUB2::pbkdf2Encrypt(QString passwd){ //, int key_length = 64, int iteration_count = 10000
    /*
    QByteArray salt;
    for( int i=0; i<key_length; ++i ) {
        qsrand(time(NULL));
        char salt_char = random(256);
        salt.append(salt_char);
    }
    QByteArray salt_hex = salt.toHex().toUpper();
    qDebug() << "Salt :" << salt_hex;
    return passwd;
    */
    QByteArray passwdstr("");
    for (unsigned i = 0; i < 2; ++i){
        passwdstr.append(passwd.toLatin1().append(QChar(10)));
    }
    QProcess process;
    process.start(GRUB_MAKE_PASSWD_EXE);
    process.write(passwdstr);
    process.waitForFinished(-1);
    QRegExp passwdregex("(grub[A-Za-z0-9.]{20,})");
    int pos = passwdregex.indexIn(process.readAllStandardOutput());
    if (pos > -1)
        return passwdregex.cap(1);
    else
        return QString();
}


void KCMGRUB2::sortResolutions()
{
    for (int i = 0; i < m_resolutions.size(); i++) {
        if (m_resolutions.at(i).contains(QRegExp("^\\d{3,4}x\\d{3,4}$"))) {
            m_resolutions[i] = QString("%1x%2x0").arg(m_resolutions.at(i).section('x', 0, 0).rightJustified(4, '0'), m_resolutions.at(i).section('x', 1).rightJustified(4, '0'));
        } else if (m_resolutions.at(i).contains(QRegExp("^\\d{3,4}x\\d{3,4}x\\d{1,2}$"))) {
            m_resolutions[i] = QString("%1x%2x%3").arg(m_resolutions.at(i).section('x', 0, 0).rightJustified(4, '0'), m_resolutions.at(i).section('x', 1, 1).rightJustified(4, '0'), m_resolutions.at(i).section('x', 2).rightJustified(2, '0'));
        }
    }
    m_resolutions.sort();
    for (int i = 0; i < m_resolutions.size(); i++) {
        if (!m_resolutions.at(i).contains(QRegExp("^\\d{3,4}x\\d{3,4}x\\d{1,2}$"))) {
            continue;
        }
        if (m_resolutions.at(i).startsWith('0')) {
            m_resolutions[i].remove(0, 1);
        }
        m_resolutions[i].replace("x0", "x");
        if (m_resolutions.at(i).endsWith('x')) {
            m_resolutions[i].remove(m_resolutions.at(i).length() - 1, 1);
        }
    }
}
void KCMGRUB2::showResolutions()
{
    ui->kcombobox_gfxmode->clear();
    ui->kcombobox_gfxmode->addItem(i18nc("@item:inlistbox Refers to screen resolution.", "Custom..."), "custom");

    ui->kcombobox_gfxpayload->clear();
    ui->kcombobox_gfxpayload->addItem(i18nc("@item:inlistbox Refers to screen resolution.", "Custom..."), "custom");
    ui->kcombobox_gfxpayload->addItem(i18nc("@item:inlistbox", "Unspecified"), QString());
    ui->kcombobox_gfxpayload->addItem(i18nc("@item:inlistbox", "Boot in Text Mode"), "text");
    ui->kcombobox_gfxpayload->addItem(i18nc("@item:inlistbox", "Keep GRUB's Resolution"), "keep");

    Q_FOREACH(const QString &resolution, m_resolutions) {
        ui->kcombobox_gfxmode->addItem(resolution, resolution);
        ui->kcombobox_gfxpayload->addItem(resolution, resolution);
    }
}

QString KCMGRUB2::processReply(ExecuteJob * reply)
{
    QString errorMessage;
    if (reply->action().status() != Action::AuthorizedStatus) {
        errorMessage = i18nc("@info", "Not Authorized.");
    } else {
        switch (reply->error()) {
        case -2:
            errorMessage = i18nc("@info", "The process could not be started.");
            break;
        case -1:
            errorMessage = i18nc("@info", "The process crashed.");
            break;
        default:
            errorMessage = QString::fromLocal8Bit(reply->data().value(QLatin1String("output")).toByteArray());
            break;
        }
    
    }
    if (reply->data().value("command").toString() != QString())
        return i18nc("@info", "Command: %1 Error code: %2 Error message: %3", reply->data().value("command").toString(), reply->data().value("output").toString(), errorMessage);
    else
        return i18nc("@info", "Error message: %1", errorMessage);
    
}

QString KCMGRUB2::parseTitle(const QString &line)
{
    QString entry, lineStr = line;
    QStringList lineStrList = lineStr.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    QRegExp entryRegex("\\s*(menuentry|submenu)\\s+[^\\n]+\\{\\s*");
    //qDebug() << lineStrList;
    if (lineStrList[1] == "--unrestricted")
        entry = QChar(39) + lineStr.split(QRegExp("['\"]"))[1] + QChar(39);
    else if (lineStrList[1] == "--users")
        if (lineStrList[2].contains(QRegExp("['\"]")))
            entry = QChar(39) + lineStr.split(QRegExp("['\"]"))[3] + QChar(39);
        else
            entry = QChar(39) + lineStr.split(QRegExp("['\"]"))[1] + QChar(39);
    else
        entry = QChar(39) + lineStr.split(QRegExp("['\"]"))[1] + QChar(39);
    return entry;
}
void KCMGRUB2::parseEntries(const QString &config)
{
    bool inEntry = false;
    int menuLvl = 0;
    QList<int> levelCount;
    levelCount.append(0);
    QList<Entry::Title> submenus;
    QString word, configStr = config;
    QTextStream stream(&configStr, QIODevice::ReadOnly | QIODevice::Text);
    while (!stream.atEnd()) {
        //Read the first word of the line
        stream >> word;
        if (stream.atEnd()) {
            return;
        }
        //If the first word is known, process the rest of the line
        if (word == QLatin1String("menuentry")) {
            if (inEntry) {
                qDebug() << "Malformed configuration file! Aborting entries' parsing.";
                qDebug() << "A 'menuentry' directive was detected inside the scope of a menuentry.";
                m_entries.clear();
                return;
            }
            Entry entry(parseTitle(stream.readLine()), levelCount.at(menuLvl), Entry::Menuentry, menuLvl);
            if (menuLvl > 0) {
                entry.setAncestors(submenus);
            }
            m_entries.append(entry);
            levelCount[menuLvl]++;
            inEntry = true;
            continue;
        } else if (word == QLatin1String("submenu")) {
            if (inEntry) {
                qDebug() << "Malformed configuration file! Aborting entries' parsing.";
                qDebug() << "A 'submenu' directive was detected inside the scope of a menuentry.";
                m_entries.clear();
                return;
            }
            Entry entry(parseTitle(stream.readLine()), levelCount.at(menuLvl), Entry::Submenu, menuLvl);
            if (menuLvl > 0) {
                entry.setAncestors(submenus);
            }
            m_entries.append(entry);
            submenus.append(entry.title());
            levelCount[menuLvl]++;
            levelCount.append(0);
            menuLvl++;
            continue;
        } else if (word == QLatin1String("linux")) {
            if (!inEntry) {
                qDebug() << "Malformed configuration file! Aborting entries' parsing.";
                qDebug() << "A 'linux' directive was detected outside the scope of a menuentry.";
                m_entries.clear();
                return;
            }
            stream >> word;
            m_entries.last().setKernel(word);
        } else if (word == QLatin1String("}")) {
            if (inEntry) {
                inEntry = false;
            } else if (menuLvl > 0) {
                submenus.removeLast();
                levelCount[menuLvl] = 0;
                menuLvl--;
            }
        }
        //Drop the rest of the line
        stream.readLine();
    }
}
void KCMGRUB2::parseSettings(const QString &config)
{
    QString line, configStr = config;
    QTextStream stream(&configStr, QIODevice::ReadOnly | QIODevice::Text);
    while (!stream.atEnd()) {
        line = stream.readLine().trimmed();
        if (line.startsWith(QLatin1String("GRUB_"))) {
            m_settings[line.section('=', 0, 0)] = line.section('=', 1);
        }
    }
}
void KCMGRUB2::parseEnv(const QString &config)
{
    QString line, configStr = config;
    QTextStream stream(&configStr, QIODevice::ReadOnly | QIODevice::Text);
    while (!stream.atEnd()) {
        line = stream.readLine().trimmed();
        if (line.startsWith('#')) {
            continue;
        }
        m_env[line.section('=', 0, 0)] = line.section('=', 1);
    }
}
