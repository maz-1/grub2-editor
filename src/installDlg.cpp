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
#include "installDlg.h"

//Qt
#include <QFile>
#include <QRadioButton>

//KDE
#include <kdeversion.h>
#include <KMessageBox>
#include <KProgressDialog>
#include <KAuth/ActionWatcher>
using namespace KAuth;
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

//Ui
#include "ui_installDlg.h"

InstallDialog::InstallDialog(QWidget *parent, Qt::WFlags flags) : KDialog(parent, flags)
{
    QWidget *widget = new QWidget(this);
    ui = new Ui::InstallDialog;
    ui->setupUi(widget);
    setMainWidget(widget);
    enableButtonOk(false);
    setWindowTitle(i18nc("@title:window", "Install/Recover Bootloader"));
    setWindowIcon(KIcon("system-software-update"));
    if (parent) {
        setInitialSize(parent->size());
    }

    ui->treeWidget_recover->headerItem()->setText(0, QString());
    ui->treeWidget_recover->header()->setResizeMode(QHeaderView::Stretch);
    ui->treeWidget_recover->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    Q_FOREACH(const Solid::Device &device, Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess)) {
        if (!device.is<Solid::StorageAccess>() || !device.is<Solid::StorageVolume>()) {
            continue;
        }
        const Solid::StorageAccess *partition = device.as<Solid::StorageAccess>();
        if (!partition) {
            continue;
        }
        const Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
        if (!volume || volume->usage() != Solid::StorageVolume::FileSystem) {
            continue;
        }

        QString uuidDir = "/dev/disk/by-uuid/", uuid = volume->uuid(), name;
        name = (QFile::exists((name = uuidDir + uuid)) || QFile::exists((name = uuidDir + uuid.toLower())) || QFile::exists((name = uuidDir + uuid.toUpper())) ? QFile::symLinkTarget(name) : QString());
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_recover, QStringList() << QString() << name << partition->filePath() << volume->label() << volume->fsType() << KGlobal::locale()->formatByteSize(volume->size()));
        item->setIcon(1, KIcon(device.icon()));
        item->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
        ui->treeWidget_recover->addTopLevelItem(item);
        QRadioButton *radio = new QRadioButton(ui->treeWidget_recover);
        connect(radio, SIGNAL(clicked(bool)), this, SLOT(enableButtonOk(bool)));
        ui->treeWidget_recover->setItemWidget(item, 0, radio);
    }
}
InstallDialog::~InstallDialog()
{
    delete ui;
}

void InstallDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        Action installAction("org.kde.kcontrol.kcmgrub2.install");
        installAction.setHelperID("org.kde.kcontrol.kcmgrub2");
        for (int i = 0; i < ui->treeWidget_recover->topLevelItemCount(); i++) {
            QRadioButton *radio = qobject_cast<QRadioButton *>(ui->treeWidget_recover->itemWidget(ui->treeWidget_recover->topLevelItem(i), 0));
            if (radio && radio->isChecked()) {
                installAction.addArgument("partition", ui->treeWidget_recover->topLevelItem(i)->text(1));
                installAction.addArgument("mountPoint", ui->treeWidget_recover->topLevelItem(i)->text(2));
                installAction.addArgument("mbrInstall", !ui->checkBox_partition->isChecked());
                break;
            }
        }
        if (installAction.arguments().value("partition").toString().isEmpty()) {
            KMessageBox::sorry(this, i18nc("@info", "Sorry, you have to select a partition with a proper name!"));
            return;
        }
#if KDE_IS_VERSION(4,6,0)
        installAction.setParentWidget(this);
#endif

        if (installAction.authorize() != Action::Authorized) {
            return;
        }

        KProgressDialog progressDlg(this, i18nc("@title:window", "Installing"), i18nc("@info:progress", "Installing GRUB..."));
        progressDlg.setAllowCancel(false);
        progressDlg.setModal(true);
        progressDlg.progressBar()->setMinimum(0);
        progressDlg.progressBar()->setMaximum(0);
        progressDlg.show();
        connect(installAction.watcher(), SIGNAL(actionPerformed(ActionReply)), &progressDlg, SLOT(hide()));

        ActionReply reply = installAction.execute();
        if (reply.succeeded()) {
            KDialog *dialog = new KDialog(this, Qt::Dialog);
            dialog->setCaption(i18nc("@title:window", "Information"));
            dialog->setButtons(KDialog::Ok | KDialog::Details);
            dialog->setModal(true);
            dialog->setDefaultButton(KDialog::Ok);
            dialog->setEscapeButton(KDialog::Ok);
            KMessageBox::createKMessageBox(dialog, QMessageBox::Information, i18nc("@info", "Successfully installed GRUB."), QStringList(), QString(), 0, KMessageBox::Notify, reply.data().value("output").toString()); // krazy:exclude=qclasses
        } else {
            KMessageBox::detailedError(this, i18nc("@info", "Failed to install GRUB."), KDE_IS_VERSION(4,7,0) ? reply.errorDescription() : reply.data().value("errorDescription").toString());
        }
    }
    KDialog::slotButtonClicked(button);
}
