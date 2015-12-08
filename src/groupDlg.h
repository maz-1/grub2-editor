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

#ifndef GROUPDLG_H
#define GROUPDLG_H

//Qt
#include <QDialog>

//Ui
namespace Ui
{
    class GroupDialog;
}

class GroupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GroupDialog(QWidget *parent = 0, QString userName = QString(), QStringList allUsers = QStringList(), QStringList usersInGroup = QStringList(), bool Locked = true, Qt::WindowFlags flags = 0);
    virtual ~GroupDialog();
    QStringList allowedUsers();
    bool isLocked();
protected Q_SLOTS:
    virtual void slotOkButtonClicked();
    virtual void slotTriggerLocked(bool lockStatus);
private:
    Ui::GroupDialog *ui;
    QStringList m_selectedUsers;
    QStringList m_unselectedUsers;
};

#endif
