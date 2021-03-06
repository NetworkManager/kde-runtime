/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef FAKEBACKTRACEGENERATOR_H
#define FAKEBACKTRACEGENERATOR_H

#include <QObject>

class FakeBacktraceGenerator : public QObject
{
    Q_OBJECT
public:
    FakeBacktraceGenerator(QObject *parent = 0) : QObject(parent) {}
    void sendData(const QString & filename);

signals:
    void starting();
    void newLine(const QString & line);
};

#endif // FAKEBACKTRACEGENERATOR_H
