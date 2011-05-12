/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

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


#include "resourcewatchermanager.h"
#include "resourcewatcherconnection.h"

#include <Soprano/Statement>
#include <Soprano/Vocabulary/RDF>

#include <QtDBus/QDBusConnection>
#include <KDebug>

#include <QtCore/QStringList>
#include <QtCore/QMutableSetIterator>

using namespace Soprano::Vocabulary;

Nepomuk::ResourceWatcherManager::ResourceWatcherManager(QObject* parent)
    : QObject(parent)
{
    // Register the service
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    sessionBus.registerService("org.kde.nepomuk.ResourceWatcher");

    sessionBus.registerObject("/watcher", this, QDBusConnection::ExportScriptableSlots);
}

void Nepomuk::ResourceWatcherManager::addProperty(QList<QUrl> resources, QUrl property, QVariantList values)
{
    typedef ResourceWatcherConnection RWC;

    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QHash<RWC*, QUrl> resConections;
    foreach( const QUrl resUri, resources ) {
        QList<RWC*> connections = m_resHash.values( resUri );
        foreach( RWC* con, connections ) {
            if( !con->hasProperties() ) {
                emit con->propertyAdded( resUri.toString(),
                                         property.toString(),
                                         values.at( resources.indexOf(resUri) ).toString() );
            }
            else {
                resConections.insert( con, resUri );
            }
        }
    }
    
    //
    // Emit signals for the connections that are watching specific resources and properties
    //
    QList<RWC*> propConnections = m_propHash.values( property );
    foreach( RWC* con, propConnections ) {
        QHash< RWC*, QUrl >::const_iterator it = resConections.constFind( con );
        if( it != resConections.constEnd() ) {
            const QUrl resUri = it.value();
            emit con->propertyAdded( resUri.toString(),
                                     property.toString(),
                                     values.at( resources.indexOf(resUri) ).toString() );
        }
    }

    //
    // Emit type + property signals
    //
    //TODO: Implement me! ( How? )
}

void Nepomuk::ResourceWatcherManager::removeProperty(QList< QUrl > resources, QUrl property, QVariantList values)
{
    typedef ResourceWatcherConnection RWC;
    
    //
    // Emit signals for all the connections that are only watching specific resources
    //
    QHash<RWC*, QUrl> resConections;
    foreach( const QUrl resUri, resources ) {
        QList<RWC*> connections = m_resHash.values( resUri );
        foreach( RWC* con, connections ) {
            if( !con->hasProperties() ) {
                emit con->propertyRemoved( resUri.toString(),
                                           property.toString(),
                                           values.at( resources.indexOf(resUri) ).toString() );
            }
            else {
                resConections.insert( con, resUri );
            }
        }
    }
    
    //
    // Emit signals for the connections that are watching specific resources and properties
    //
    QList<RWC*> propConnections = m_propHash.values( property );
    foreach( RWC* con, propConnections ) {
        QHash< RWC*, QUrl >::const_iterator it = resConections.constFind( con );
        if( it != resConections.constEnd() ) {
            const QUrl resUri = it.value();
            emit con->propertyRemoved( resUri.toString(),
                                       property.toString(),
                                       values.at( resources.indexOf(resUri) ).toString() );
        }
    }
}

QDBusObjectPath Nepomuk::ResourceWatcherManager::watch(const QList< QString >& resources, const QList< QString >& properties, const QList< QString >& types)
{
    kDebug();

    if( resources.isEmpty() && properties.isEmpty() && types.isEmpty() )
        return QDBusObjectPath();

    ResourceWatcherConnection * con = new ResourceWatcherConnection( this, !properties.isEmpty() );
    foreach( const QString & res, resources ) {
        m_resHash.insert( QUrl(res), con );
    }

    foreach( const QString & prop, properties ) {
        m_propHash.insert( QUrl(prop), con );
    }

    foreach( const QString & type, properties ) {
        m_typeHash.insert( QUrl(type), con );
    }
    
    return con->dBusPath();
}

namespace {
    void remove( QMultiHash<QUrl, Nepomuk::ResourceWatcherConnection*> & hash,
                 const Nepomuk::ResourceWatcherConnection * con )
    {
        QMutableHashIterator<QUrl, Nepomuk::ResourceWatcherConnection*> it( hash );
        while( it.hasNext() ) {
            it.next();

            if( it.value() == con )
                it.remove();
        }
    }
}

void Nepomuk::ResourceWatcherManager::stopWatcher(const QString& objectName)
{
    kDebug();

    ResourceWatcherConnection * con = 0;
    QObjectList cl = children();
    foreach( QObject * obj, cl ) {
        ResourceWatcherConnection * c = static_cast<ResourceWatcherConnection*>( obj );
        if( c->objectName() == objectName ) {
            con = c;
            break;
        }
    }

    remove( m_resHash, con );
    remove( m_propHash, con );
    remove( m_typeHash, con );

    con->deleteLater();
}


