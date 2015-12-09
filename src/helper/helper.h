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

#ifndef HELPER_H
#define HELPER_H
//Qt
#include <QDebug>
//KDE
#include <KAuth>
using namespace KAuth;

class Helper : public QObject
{
    Q_OBJECT
public:
    Helper();
private:
    ActionReply executeCommand(const QStringList &command);
    //merge
    ActionReply load(QVariantMap args);
    ActionReply probe(QVariantMap args);
    ActionReply probevbe(QVariantMap args);
public Q_SLOTS:
    ActionReply initialize(QVariantMap args);
    ActionReply defaults(QVariantMap args);
    ActionReply install(QVariantMap args);
    ActionReply save(QVariantMap args);
};

#endif
