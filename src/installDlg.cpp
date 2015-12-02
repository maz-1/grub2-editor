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
#include <QPushButton>
//KDE
#include <KMessageBox>
//#include <KProgressDialog>

#include <KFormat>
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>

#include <KAuth>
using namespace KAuth;

//Ui
#include "ui_installDlg.h"

InstallDialog::InstallDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags)
{
    QWidget *widget = new QWidget(this);
    KFormat format;
    ui = new Ui::InstallDialog;
    ui->setupUi(widget);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(widget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
    buttonBox->button(QDialogButtonBox::Cancel)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(SlotOkButtonClicked()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    setWindowTitle(i18nc("@title:window", "Install/Recover Bootloader"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));
    if (parent) {
        resize(parent->size());
    }

    ui->treeWidget_recover->headerItem()->setText(0, QString());
    ui->treeWidget_recover->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->treeWidget_recover->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
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
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_recover, QStringList() << QString() << name << partition->filePath() << volume->label() << volume->fsType() << format.formatByteSize(volume->size())); //KGlobal::locale()->
        item->setIcon(1, QIcon::fromTheme(device.icon()));
        item->setTextAlignment(5, Qt::AlignRight | Qt::AlignVCenter);
        ui->treeWidget_recover->addTopLevelItem(item);
        QRadioButton *radio = new QRadioButton(ui->treeWidget_recover);
        connect(radio, SIGNAL(clicked(bool)), buttonBox->button(QDialogButtonBox::Ok), SLOT(setEnabled(bool)));
        ui->treeWidget_recover->setItemWidget(item, 0, radio);
    }
}
InstallDialog::~InstallDialog()
{
    delete ui;
}

void InstallDialog::SlotOkButtonClicked()
{
        Action installAction("org.kde.kcontrol.kcmgrub2.install");
        installAction.setHelperId("org.kde.kcontrol.kcmgrub2");
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


        QProgressDialog progressDlg(this, Qt::Dialog);
        progressDlg.setWindowTitle(i18nc("@title:window", "Installing"));
        progressDlg.setLabelText(i18nc("@info:progress", "Installing GRUB..."));
        progressDlg.setCancelButton(0);
        progressDlg.setModal(true);
        progressDlg.setRange(0,0);
        progressDlg.show();

        ExecuteJob* reply = installAction.execute();
        reply->exec();
        
        if (installAction.status() != Action::AuthorizedStatus ) {
          progressDlg.hide();
          return;
        }
        
        //connect(reply, SIGNAL(result()), &progressDlg, SLOT(hide()));
        progressDlg.hide();
        if (reply->error()) {
            KMessageBox::detailedError(this, i18nc("@info", "Failed to install GRUB."), reply->data().value("errorDescription").toString());
        } else {
            progressDlg.hide();
            QDialog *dialog = new QDialog(this, Qt::Dialog);
            dialog->setWindowTitle(i18nc("@title:window", "Information"));
            dialog->setModal(true);
            QDialogButtonBox *btnbox = new QDialogButtonBox(QDialogButtonBox::Ok);
            KMessageBox::createKMessageBox(dialog, btnbox, QMessageBox::Information, i18nc("@info", "Successfully installed GRUB."), QStringList(), QString(), 0, KMessageBox::Notify, reply->data().value("output").toString()); // krazy:exclude=qclasses
        }
    this->accept();
}
