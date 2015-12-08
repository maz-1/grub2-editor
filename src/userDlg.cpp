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
#include "userDlg.h"

//KDE
#include <KMessageBox>
//Qt
#include <QPushButton>
//Ui
#include "ui_userDlg.h"

UserDialog::UserDialog(QWidget *parent, QString userName, bool isSuperUser, bool isEncrypted, Qt::WindowFlags flags) : QDialog(parent, flags)
{
    resize(parent->size().width() / 2, parent->size().height() / 4);
    this->setWindowTitle(i18nc("@title:window", "Edit User"));
    QWidget *widget = new QWidget(this);
    ui = new Ui::UserDialog;
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
    
    QRegExp regExp("[A-Za-z0-9]+");
    regExp.setPatternSyntax(QRegExp::RegExp);
    QRegExpValidator *validator;
    validator = new QRegExpValidator(regExp);
    ui->lineEdit_username->setValidator(validator);
    if (userName != QString()) {
        ui->lineEdit_username->setText(userName);
        ui->lineEdit_username->setEnabled(false);
        ui->checkBox_superuser->setCheckState(isSuperUser ? Qt::Checked : Qt::Unchecked);
        ui->checkBox_cryptpass->setCheckState(isEncrypted ? Qt::Checked : Qt::Unchecked);
    }
    
}
UserDialog::~UserDialog()
{
    delete ui;
}


void UserDialog::slotOkButtonClicked()
{
    if (ui->lineEdit_password->text() != ui->lineEdit_passwordconfirm->text()) {
        KMessageBox::sorry(this, i18nc("@info", "Passwords don't match!"));
            return;
    } else if (ui->lineEdit_password->text().isEmpty()) {
        KMessageBox::sorry(this, i18nc("@info", "Please fill in passwords!"));
            return;
    } else if (ui->lineEdit_username->text().isEmpty()) {
        KMessageBox::sorry(this, i18nc("@info", "Please fill in username!"));
            return;
    } else {
        this->accept();
    }
}

QString UserDialog::getPassword()
{
    return ui->lineEdit_password->text();
}
QString UserDialog::getUserName()
{
    return ui->lineEdit_username->text();
}

bool UserDialog::isSuperUser()
{
    if (ui->checkBox_superuser->checkState() == Qt::Checked)
        return true;
    else
        return false;
}

bool UserDialog::requireEncryption()
{
    if (ui->checkBox_cryptpass->checkState() == Qt::Checked)
        return true;
    else
        return false;
}