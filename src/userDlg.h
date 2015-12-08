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

#ifndef USERDLG_H
#define USERDLG_H

//Qt
#include <QDialog>

//Ui
namespace Ui
{
    class UserDialog;
}

class UserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UserDialog(QWidget *parent = 0, QString userName = QString(), bool isSuperUser = false, bool isEncrypted = false, Qt::WindowFlags flags = 0);
    virtual ~UserDialog();
    QString getPassword();
    QString getUserName();
    bool isSuperUser();
    bool requireEncryption();
protected Q_SLOTS:
    virtual void slotOkButtonClicked();
private:
    Ui::UserDialog *ui;
};

#endif
