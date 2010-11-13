/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>

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


#include "resourcemerger.h"

#define USING_SOPRANO_NRLMODEL_UNSTABLE_API

#include <Soprano/Model>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/NRLModel>
#include <Soprano/Graph>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <KUrl>
#include <KDebug>

class Nepomuk::Sync::ResourceMerger::Private {
public:
    Private( ResourceMerger * resMerger );
    
    Soprano::Model * m_model;
    ResourceManager * m_resourceManager;

    Soprano::NRLModel * m_nrlModel;
    KUrl m_graphType;

    ResourceMerger * q;
    
    QHash<KUrl, Resource> m_oldMappings;
    QHash<KUrl, Resource> m_newMappings;

    void push( const Soprano::Statement & st, const KUrl & graphUri );
    KUrl resolve( const KUrl& oldUri );
};

Nepomuk::Sync::ResourceMerger::Private::Private(Nepomuk::Sync::ResourceMerger* resMerger)
    : q( resMerger )
{
}


Nepomuk::Sync::ResourceMerger::ResourceMerger(Nepomuk::ResourceManager* rm)
    : d( new Nepomuk::Sync::ResourceMerger::Private( this ) )
{
    d->m_nrlModel = 0;
    setResourceManager( rm );
    d->m_graphType = Soprano::Vocabulary::NRL::InstanceBase();
}

Nepomuk::Sync::ResourceMerger::~ResourceMerger()
{
    delete d;
}

void Nepomuk::Sync::ResourceMerger::setResourceManager(Nepomuk::ResourceManager* rm)
{
    if( !rm )
        d->m_resourceManager = ResourceManager::instance();
    d->m_model = d->m_resourceManager->mainModel();
    
    delete d->m_nrlModel;
    d->m_nrlModel = new Soprano::NRLModel( d->m_model );
}

Nepomuk::ResourceManager* Nepomuk::Sync::ResourceMerger::resourceManager() const
{
    return d->m_resourceManager;
}


Soprano::Model* Nepomuk::Sync::ResourceMerger::model() const
{
    return d->m_resourceManager->mainModel();
}


Nepomuk::Types::Class Nepomuk::Sync::ResourceMerger::graphType() const
{
    return d->m_graphType;
}

bool Nepomuk::Sync::ResourceMerger::setGraphType(const Nepomuk::Types::Class& type)
{
    if( type.isSubClassOf( Soprano::Vocabulary::NRL::Graph() ) ){
        d->m_graphType = type.uri();
        return true;
    }
    return false;
}


Nepomuk::Resource Nepomuk::Sync::ResourceMerger::resolveUnidentifiedResource(const KUrl& uri)
{
    // The default implementation is to create it.
    QHash< KUrl, Resource >::const_iterator it = d->m_newMappings.constFind( uri );
    if( it != d->m_newMappings.constEnd() )
        return it.value();
    
    KUrl newUri = d->m_resourceManager->generateUniqueUri( QString("res") );
    d->m_newMappings.insert( uri, newUri );
    return Nepomuk::Resource( newUri );
}


void Nepomuk::Sync::ResourceMerger::merge(const Soprano::Graph& graph, const QHash< KUrl, Nepomuk::Resource >& mappings)
{
    d->m_oldMappings = mappings;

    KUrl graphUri = createGraph();
    
    QList<Soprano::Statement> statements = graph.toList();
    foreach( Soprano::Statement st, statements ) {
        if( !st.isValid() )
            continue;

        st.setSubject( d->resolve( st.subject().uri() ) );
        if( st.object().isResource() ) {
            KUrl resolvedObject = d->resolve( st.object().uri() );
            if( resolvedObject.isEmpty() ) {
                kDebug() << st.object().uri() << " resolution failed!";
                continue;
            }
            st.setObject( resolvedObject );
        }

        d->push( st, graphUri );
    }
}


KUrl Nepomuk::Sync::ResourceMerger::createGraph()
{
    return d->m_nrlModel->createGraph( d->m_graphType );
}


void Nepomuk::Sync::ResourceMerger::Private::push(const Soprano::Statement& st, const KUrl& graphUri)
{
    if( m_model->containsAnyStatement( st.subject(), st.predicate(), st.object() ) ) {
        // Already exists. Ignore
        return;
    }

    Soprano::Statement statement( st );
    if( statement.context().isEmpty() )
        statement.setContext( graphUri );
    
    m_model->addStatement( statement );
}


KUrl Nepomuk::Sync::ResourceMerger::Private::resolve(const KUrl& oldUri)
{
    // Find in mappings
    QHash< KUrl, Resource >::const_iterator it = m_oldMappings.constFind( oldUri );
    if( it != m_oldMappings.constEnd() ) {
        return it.value().resourceUri();
    } else {
        Nepomuk::Resource res = q->resolveUnidentifiedResource( oldUri );
        return res.resourceUri();
    }
}

void Nepomuk::Sync::ResourceMerger::push(const Soprano::Statement& st)
{
    if( !st.context().isEmpty() )
        d->push( st, QUrl() );
}