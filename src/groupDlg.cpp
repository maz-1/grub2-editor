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
#include "groupDlg.h"

//KDE
#include <KMessageBox>
//Qt
#include <QPushButton>
#include <QDebug>
//Ui
#include "ui_groupDlg.h"
//Strange, not included by KActionSelector
#include <QListWidget>

GroupDialog::GroupDialog(QWidget *parent, QString groupName, QStringList allUsers, QStringList usersInGroup, bool Locked, Qt::WindowFlags flags) : QDialog(parent, flags)
{
    resize(parent->size().width(), parent->size().height() / 2);
    this->setWindowTitle(i18nc("@title:window", "Edit Group"));
    QWidget *widget = new QWidget(this);
    ui = new Ui::GroupDialog;
    ui->setupUi(widget);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(widget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
    buttonBox->button(QDialogButtonBox::Cancel)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotOkButtonClicked()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
//initialize users list
    ui->userSelector->setEnabled(Locked);
    ui->checkbox_locked->setCheckState(Locked ? Qt::Checked : Qt::Unchecked);
    m_selectedUsers.clear();
    m_unselectedUsers.clear();
    for (int i=0; i<allUsers.count(); ++i) {
        if (usersInGroup.contains(allUsers[i]))
            m_selectedUsers.append(allUsers[i]);
        else
            m_unselectedUsers.append(allUsers[i]);
    }
    ui->userSelector->availableListWidget()->clear();
    ui->userSelector->availableListWidget()->addItems(m_unselectedUsers);
    ui->userSelector->selectedListWidget()->clear();
    ui->userSelector->selectedListWidget()->addItems(m_selectedUsers);
    
    connect(ui->checkbox_locked, SIGNAL(toggled(bool)), this, SLOT(slotTriggerLocked(bool)));
    
}
GroupDialog::~GroupDialog()
{
    delete ui;
}


void GroupDialog::slotOkButtonClicked()
{
    //if (ui->userSelector->availableListWidget() && ui->userSelector->selectedListWidget()) {
        //KMessageBox::sorry(this, i18nc("@info", "Passwords don't match!"));
            //return;
    //} else {
        this->accept();
    //}
}

void GroupDialog::slotTriggerLocked(bool lockStatus)
{
    ui->userSelector->setEnabled(lockStatus);
}

QStringList GroupDialog::allowedUsers()
{
    m_selectedUsers.clear();
    QListWidget *selectedUsersWidget = ui->userSelector->selectedListWidget();
    for (int i=0; i<selectedUsersWidget->count(); ++i) {
        m_selectedUsers.append(selectedUsersWidget->item(i)->text());
    }
    return m_selectedUsers;
}
bool GroupDialog::isLocked() {
    if (ui->checkbox_locked->checkState() == Qt::Checked)
        return true;
    else
        return false;
}