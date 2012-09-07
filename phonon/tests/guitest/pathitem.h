/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PATHITEM_H
#define PATHITEM_H

#include <QtCore/QObject>
#include <QGraphicsPathItem>
#include "widgetrectitem.h"
#include <Phonon/Path>

using Phonon::Path;

class PathItem : public QObject, public QGraphicsPathItem
{
    Q_OBJECT
    public:
        PathItem(WidgetRectItem *start, WidgetRectItem *end, const Path &path);

        enum { Type = UserType + 20 };
        int type() const { return Type; }

        Path path() const { return m_path; }

        void endPointMoved(const WidgetRectItem *item);

    public slots:
        void updateChildrenPositions();

    private:
        void updatePainterPath();

        Path m_path;
        WidgetRectItem *m_startItem;
        WidgetRectItem *m_endItem;
        QPointF m_startPos;
        QPointF m_endPos;
};

#endif // PATHITEM_H