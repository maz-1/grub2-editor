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
#include "common.h"

//Qt
#include <QTextStream>

//KDE
#include <KProcess>
#include <KShell>

QString quoteWord(const QString &word)
{
    return !word.startsWith('`') || !word.endsWith('`') ? KShell::quoteArg(word.simplified()) : word.simplified();
}
QString unquoteWord(const QString &word)
{
    KProcess echo;
    echo.setShellCommand(QString("echo -n %1").arg(word));
    echo.setOutputChannelMode(KProcess::OnlyStdoutChannel);
    if (echo.execute() == 0) {
        return QString::fromLocal8Bit(echo.readAllStandardOutput());
    }

    QChar ch;
    QString quotedWord = word, unquotedWord;
    QTextStream stream(&quotedWord, QIODevice::ReadOnly | QIODevice::Text);
    while (!stream.atEnd()) {
        stream >> ch;
        if (ch == '\'') {
            while (true) {
                if (stream.atEnd()) {
                    return QString();
                }
                stream >> ch;
                if (ch == '\'') {
                    return unquotedWord;
                } else {
                    unquotedWord.append(ch);
                }
            }
        } else if (ch == '"') {
            while (true) {
                if (stream.atEnd()) {
                    return QString();
                }
                stream >> ch;
                if (ch == '\\') {
                    if (stream.atEnd()) {
                        return QString();
                    }
                    stream >> ch;
                    switch (ch.toLatin1()) {
                        case '$':
                        case '"':
                        case '\\':
                            unquotedWord.append(ch);
                            break;
                        case '\n':
                            unquotedWord.append(' ');
                            break;
                        default:
                            unquotedWord.append('\\').append(ch);
                            break;
                    }
                } else if (ch == '"') {
                    return unquotedWord;
                } else {
                    unquotedWord.append(ch);
                }
            }
        } else {
            while (true) {
                if (ch == '\\') {
                    if (stream.atEnd()) {
                        return unquotedWord;
                    }
                    stream >> ch;
                    switch (ch.toLatin1()) {
                        case '\n':
                            break;
                        default:
                            unquotedWord.append(ch);
                            break;
                    }
                } else if (ch.isSpace()) {
                    return unquotedWord;
                } else {
                    unquotedWord.append(ch);
                }
                if (stream.atEnd()) {
                    return unquotedWord;
                }
                stream >> ch;
            }
        }
    }
    return QString();
}
