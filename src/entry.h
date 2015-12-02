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

#ifndef ENTRY_H
#define ENTRY_H

//Qt
#include <QList>
#include <QString>

class Entry
{
public:
    struct Title {
        QString str;
        int num;
    };
    enum Type {
        Invalid,
        Menuentry,
        Submenu
    };

    Entry(const QString &strTitle = QString(), int numTitle = -1, Entry::Type type = Entry::Invalid, int level = -1);

    Entry::Title title() const;
    QString prettyTitle() const;
    QString fullTitle() const;
    QString fullNumTitle() const;
    Entry::Type type() const;
    int level() const;
    QList<Entry::Title> ancestors() const;
    QString kernel() const;

    void setTitle(const Entry::Title &title);
    void setTitle(const QString &strTitle, int numTitle);
    void setType(Entry::Type type);
    void setLevel(int level);
    void setAncestors(const QList<Entry::Title> &ancestors);
    void setKernel(const QString &kernel);

    void clear();
private:
    Entry::Title m_title;
    Entry::Type m_type;
    int m_level;
    QList<Entry::Title> m_ancestors;
    QString m_kernel;
};

#endif
