/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "simpleresourcegraph.h"
#include "simpleresource.h"
#include "datamanagement.h"

#include <QtCore/QSharedData>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include <KRandom>

class Nepomuk::SimpleResourceGraph::Private : public QSharedData
{
public:
    QHash<QUrl, SimpleResource> resources;
};


Nepomuk::SimpleResourceGraph::SimpleResourceGraph()
    : d(new Private)
{
}

Nepomuk::SimpleResourceGraph::SimpleResourceGraph(const SimpleResource& resource)
    : d(new Private)
{
    insert(resource);
}

Nepomuk::SimpleResourceGraph::SimpleResourceGraph(const QList<SimpleResource>& resources)
    : d(new Private)
{
    Q_FOREACH(const SimpleResource& res, resources) {
        insert(res);
    }
}

Nepomuk::SimpleResourceGraph::SimpleResourceGraph(const QSet<SimpleResource>& resources)
    : d(new Private)
{
    Q_FOREACH(const SimpleResource& res, resources) {
        insert(res);
    }
}

Nepomuk::SimpleResourceGraph::SimpleResourceGraph(const SimpleResourceGraph& other)
    : d(other.d)
{
}

Nepomuk::SimpleResourceGraph::~SimpleResourceGraph()
{
}

Nepomuk::SimpleResourceGraph & Nepomuk::SimpleResourceGraph::operator=(const Nepomuk::SimpleResourceGraph &other)
{
    d = other.d;
    return *this;
}

void Nepomuk::SimpleResourceGraph::insert(const SimpleResource &res)
{
    d->resources.insert(res.uri(), res);
}

Nepomuk::SimpleResourceGraph& Nepomuk::SimpleResourceGraph::operator<<(const SimpleResource &res)
{
    insert(res);
    return *this;
}

void Nepomuk::SimpleResourceGraph::remove(const QUrl &uri)
{
    d->resources.remove(uri);
}

void Nepomuk::SimpleResourceGraph::remove(const SimpleResource &res)
{
    if( contains( res ) )
        remove( res.uri() );
}

bool Nepomuk::SimpleResourceGraph::contains(const QUrl &uri) const
{
    return d->resources.contains(uri);
}

bool Nepomuk::SimpleResourceGraph::contains(const SimpleResource &res) const
{
    QHash< QUrl, SimpleResource >::const_iterator it = d->resources.find( res.uri() );
    if( it == d->resources.constEnd() )
        return false;

    return res == it.value();
}

Nepomuk::SimpleResource Nepomuk::SimpleResourceGraph::operator[](const QUrl &uri) const
{
    return d->resources[uri];
}

QSet<Nepomuk::SimpleResource> Nepomuk::SimpleResourceGraph::toSet() const
{
    return QSet<SimpleResource>::fromList(toList());
}

QList<Nepomuk::SimpleResource> Nepomuk::SimpleResourceGraph::toList() const
{
    return d->resources.values();
}

void Nepomuk::SimpleResourceGraph::clear()
{
    d->resources.clear();
}

bool Nepomuk::SimpleResourceGraph::isEmpty() const
{
    return d->resources.isEmpty();
}

int Nepomuk::SimpleResourceGraph::count() const
{
    return d->resources.count();
}

namespace {
QVariant nodeToVariant(const Soprano::Node& node) {
    if(node.isResource())
        return node.uri();
    else if(node.isBlank())
        return QUrl(QLatin1String("_:") + node.identifier());
    else
        return node.literal().variant();
}
}

void Nepomuk::SimpleResourceGraph::addStatement(const Soprano::Statement &s)
{
    const QUrl uri = nodeToVariant(s.subject()).toUrl();
    const QVariant value = nodeToVariant(s.object());
    d->resources[uri].setUri(uri);
    d->resources[uri].addProperty(s.predicate().uri(), value);
}

KJob* Nepomuk::SimpleResourceGraph::save(const KComponentData& component) const
{
    return Nepomuk::storeResources(*this, QHash<QUrl, QVariant>(), component);
}

QDebug Nepomuk::operator<<(QDebug dbg, const Nepomuk::SimpleResourceGraph& graph)
{
    return dbg << graph.toList();
}