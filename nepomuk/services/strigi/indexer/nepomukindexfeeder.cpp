/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

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


#include "nepomukindexfeeder.h"
#include "../util.h"

#include <QtCore/QDateTime>

#include <Soprano/Model>
#include <Soprano/Statement>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/NRL>

#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>

#include <KDebug>


Nepomuk::IndexFeeder::IndexFeeder( QObject* parent )
    : QObject( parent )
{
}


Nepomuk::IndexFeeder::~IndexFeeder()
{
}


void Nepomuk::IndexFeeder::begin( const QUrl & url )
{
    //kDebug() << "BEGINNING";
    Request req;
    req.uri = url;

    m_stack.push( req );
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Statement& st)
{
    Q_ASSERT( !m_stack.isEmpty() );
    Request & req = m_stack.top();

    const Soprano::Node & n = st.subject();

    QUrl uriOrId;
    if( n.isResource() )
        uriOrId = n.uri();
    else if( n.isBlank() )
        uriOrId = n.identifier();

    if( !req.hash.contains( uriOrId ) ) {
        ResourceStruct rs;
        if( n.isResource() )
            rs.uri = n.uri();

        req.hash.insert( uriOrId, rs );
    }

    ResourceStruct & rs = req.hash[ uriOrId ];
    rs.propHash.insert( st.predicate().uri(), st.object() );
}


void Nepomuk::IndexFeeder::addStatement(const Soprano::Node& subject, const Soprano::Node& predicate, const Soprano::Node& object)
{
    addStatement( Soprano::Statement( subject, predicate, object, Soprano::Node() ) );
}


void Nepomuk::IndexFeeder::end()
{
    if( m_stack.isEmpty() )
        return;
    //kDebug() << "ENDING";

    Request req = m_stack.pop();
    handleRequest( req );
}


QString Nepomuk::IndexFeeder::buildResourceQuery(const Nepomuk::IndexFeeder::ResourceStruct& rs) const
{
    QString query = QString::fromLatin1("select distinct ?r where { ");

    // TODO: trueg: use an iterator instead of creating temp lists. That seems like wasted resources.
    QList<QUrl> keys = rs.propHash.uniqueKeys();
    foreach( const QUrl & prop, keys ) {
        const QList<Soprano::Node>& values = rs.propHash.values( prop );

        foreach( const Soprano::Node & n, values ) {
            query += " ?r " + Soprano::Node::resourceToN3( prop ) + " " + n.toN3() + " . ";
        }
    }
    query += " } LIMIT 1";
    return query;
}


void Nepomuk::IndexFeeder::addToModel(const Nepomuk::IndexFeeder::ResourceStruct& rs) const
{
    QUrl context = generateGraph( rs.uri );
    QHashIterator<QUrl, Soprano::Node> iter( rs.propHash );
    while( iter.hasNext() ) {
        iter.next();

        Soprano::Statement st( rs.uri, iter.key(), iter.value(), context );
        //kDebug() << "ADDING : " << st;
        ResourceManager::instance()->mainModel()->addStatement( st );
    }
}

void Nepomuk::IndexFeeder::handleRequest( Request& request )
{
    // Search for the resources or create them
    //kDebug() << " Searching for duplicates or creating them ... ";
    QMutableHashIterator<QUrl, ResourceStruct> it( request.hash );
    while( it.hasNext() ) {
        it.next();

        // If it already exists
        ResourceStruct & rs = it.value();
        if( !rs.uri.isEmpty() )
            continue;

        QString query = buildResourceQuery( rs );
        //kDebug() << query;
        Soprano::QueryResultIterator it = ResourceManager::instance()->mainModel()->executeQuery( query, Soprano::Query::QueryLanguageSparql );

        if( it.next() ) {
            //kDebug() << "Found exact match " << rs.uri << " " << it[0].uri();
            rs.uri = it[0].uri();
        }
        else {
            //kDebug() << "Creating ..";
            rs.uri = ResourceManager::instance()->generateUniqueUri( QString() );

            // Add to the repository
            addToModel( rs );
        }
    }

    // Fix links for main
    ResourceStruct & rs = request.hash[ request.uri ];
    QMutableHashIterator<QUrl, Soprano::Node> iter( rs.propHash );
    while( iter.hasNext() ) {
        iter.next();
        Soprano::Node & n = iter.value();

        if( n.isEmpty() )
            continue;

        if( n.isBlank() ) {
            const QString & id = n.identifier();
            if( !request.hash.contains( id ) )
                continue;
            QUrl newUri = request.hash.value( id ).uri;
            //kDebug() << id << " ---> " << newUri;
            iter.value() = Soprano::Node( newUri );
        }
    }

    // Add main file to the repository
    addToModel( rs );

    m_lastRequestUri = request.uri;
}


QUrl Nepomuk::IndexFeeder::generateGraph( const QUrl& resourceUri ) const
{
    Soprano::Model* model = ResourceManager::instance()->mainModel();
    QUrl context = Nepomuk::ResourceManager::instance()->generateUniqueUri( "ctx" );

    // create the provedance data for the data graph
    // TODO: add more data at some point when it becomes of interest
    QUrl metaDataContext = Nepomuk::ResourceManager::instance()->generateUniqueUri( "ctx" );
    model->addStatement( context,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::DiscardableInstanceBase(),
                         metaDataContext );
    model->addStatement( context,
                         Soprano::Vocabulary::NAO::created(),
                         Soprano::LiteralValue( QDateTime::currentDateTime() ),
                         metaDataContext );
    model->addStatement( context,
                         Strigi::Ontology::indexGraphFor(),
                         resourceUri,
                         metaDataContext );
    model->addStatement( metaDataContext,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::GraphMetadata(),
                         metaDataContext );
    model->addStatement( metaDataContext,
                         Soprano::Vocabulary::NRL::coreGraphMetadataFor(),
                         context,
                         metaDataContext );

    return context;
}

QUrl Nepomuk::IndexFeeder::lastRequestUri() const
{
    return m_lastRequestUri;
}
