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
#include "removeDlg.h"

//Qt
#include <QFile>
#include <QTextStream>
#include <QTimer>

//KDE
#include <KMessageBox>
#include <KProgressDialog>

//Project
#include "entry.h"

//Ui
#include "ui_removeDlg.h"

RemoveDialog::RemoveDialog(const QList<Entry> &entries, QWidget *parent, Qt::WFlags flags) : KDialog(parent, flags)
{
    QWidget *widget = new QWidget(this);
    ui = new Ui::RemoveDialog;
    ui->setupUi(widget);
    setMainWidget(widget);
    enableButtonOk(false);
    setWindowTitle(i18nc("@title:window", "Remove Old Entries"));
    setWindowIcon(KIcon("list-remove"));

    m_progressDlg = 0;

#if HAVE_QAPT && QAPT_VERSION_MAJOR == 1
    m_backend = new QAptBackend;
#elif HAVE_QAPT && QAPT_VERSION_MAJOR == 2
    m_backend = new QApt2Backend;
#elif HAVE_QPACKAGEKIT
    m_backend = new QPkBackend;
#endif

    detectCurrentKernelImage();
    KProgressDialog progressDlg(this, i18nc("@title:window", "Finding Old Entries"), i18nc("@info:progress", "Finding Old Entries..."));
    progressDlg.setAllowCancel(false);
    progressDlg.setModal(true);
    progressDlg.show();
    bool found = false;
    for (int i = 0; i < entries.size(); i++) {
        progressDlg.progressBar()->setValue(100. / entries.size() * (i + 1));
        QString file = entries.at(i).kernel();
        if (file.isEmpty() || file == m_currentKernelImage) {
            continue;
        }
        QStringList package = m_backend->ownerPackage(file);
        if (package.size() < 2) {
            continue;
        }
        found = true;
        QString packageName = package.takeFirst();
        QString packageVersion = package.takeFirst();
        QTreeWidgetItem *item = 0;
        for (int j = 0; j < ui->treeWidget->topLevelItemCount(); j++) {
            if (ui->treeWidget->topLevelItem(j)->data(0, Qt::UserRole).toString() == packageName && ui->treeWidget->topLevelItem(j)->data(0, Qt::UserRole + 1).toString() == packageVersion) {
                item = ui->treeWidget->topLevelItem(j);
                break;
            }
        }
        if (!item) {
            item = new QTreeWidgetItem(ui->treeWidget, QStringList(i18nc("@item:inlistbox", "Kernel %1", packageVersion)));
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setData(0, Qt::UserRole, packageName);
            item->setData(0, Qt::UserRole + 1, packageVersion);
            item->setCheckState(0, Qt::Checked);
            ui->treeWidget->addTopLevelItem(item);
        }
        item->addChild(new QTreeWidgetItem(QStringList(entries.at(i).prettyTitle())));
    }
    if (found) {
        ui->treeWidget->expandAll();
        ui->treeWidget->resizeColumnToContents(0);
        ui->treeWidget->setMinimumWidth(ui->treeWidget->columnWidth(0) + ui->treeWidget->sizeHintForRow(0));
        connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(slotItemChanged()));
        enableButtonOk(true);
    } else {
        KMessageBox::sorry(this, i18nc("@info", "No removable entries were found."));
        QTimer::singleShot(0, this, SLOT(reject()));
    }
}
RemoveDialog::~RemoveDialog()
{
    delete m_backend;
    delete ui;
}
void RemoveDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
            if (ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked) {
                QString packageName = ui->treeWidget->topLevelItem(i)->data(0, Qt::UserRole).toString();
                m_backend->markForRemoval(packageName);
                if (ui->checkBox_headers->isChecked()) {
                    packageName.replace("image", "headers");
                    m_backend->markForRemoval(packageName);
                }
            }
        }
        if (KMessageBox::questionYesNoList(this, i18nc("@info", "Are you sure you want to remove the following packages?"), m_backend->markedForRemoval()) == KMessageBox::Yes) {
            connect(m_backend, SIGNAL(progress(QString,int)), this, SLOT(slotProgress(QString,int)));
            connect(m_backend, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
            m_backend->removePackages();
        } else {
            m_backend->undoChanges();
        }
        return;
    }
    KDialog::slotButtonClicked(button);
}
void RemoveDialog::slotItemChanged()
{
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
        if (ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked) {
            enableButtonOk(true);
            return;
        }
    }
    enableButtonOk(false);
}
void RemoveDialog::slotProgress(const QString &status, int percentage)
{
    if (!m_progressDlg) {
        m_progressDlg = new KProgressDialog(this, i18nc("@title:window", "Removing Old Entries"));
        m_progressDlg->setAllowCancel(false);
        m_progressDlg->setModal(true);
        m_progressDlg->show();
    }
    m_progressDlg->setLabelText(status);
    m_progressDlg->progressBar()->setValue(percentage);
}
void RemoveDialog::slotFinished(bool success)
{
    if (success) {
        accept();
    } else {
        KMessageBox::error(this, i18nc("@info", "Package removal failed."));
        reject();
    }
}
void RemoveDialog::detectCurrentKernelImage()
{
    QFile file("/proc/cmdline");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    Q_FOREACH(const QString &argument, stream.readAll().split(QRegExp("\\s+"))) {
        if (argument.startsWith(QLatin1String("BOOT_IMAGE"))) {
            m_currentKernelImage = argument.section('=', 1);
            return;
        }
    }
}
