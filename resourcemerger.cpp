/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "resourcemerger.h"
#include "datamanagementmodel.h"
#include "classandpropertytree.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NAO>

#include <Soprano/StatementIterator>
#include <Soprano/FilterModel>
#include <Soprano/NodeIterator>

#include <Nepomuk/Variant>
#include <KDebug>
#include <Soprano/Graph>

using namespace Soprano::Vocabulary;

Nepomuk::ResourceMerger::ResourceMerger(Nepomuk::DataManagementModel* model, const QString& app,
                                        const QHash< QUrl, QVariant >& additionalMetadata)
{
    m_app = app;
    m_additionalMetadata = additionalMetadata;
    m_model = model;
    
    setModel( m_model );
}

Nepomuk::ResourceMerger::~ResourceMerger()
{
}

KUrl Nepomuk::ResourceMerger::createGraph()
{
    return m_model->createGraph( m_app, m_additionalMetadata );
}

void Nepomuk::ResourceMerger::resolveDuplicate(const Soprano::Statement& newSt)
{
    kDebug() << newSt;
    // Merge rules
    // 1. If old graph is of type discardable and new is non-discardable
    //    -> Then update the graph
    // 2. Otherwsie
    //    -> Keep the old graph
    
    const QUrl oldGraph = model()->listStatements( newSt.subject(), newSt.predicate(), newSt.object() ).iterateContexts().allNodes().first().uri();

    kDebug() << m_model->statementCount();
    QUrl newGraph = mergeGraphs( oldGraph );
    if( newGraph.isValid() ) {
        kDebug() << "Is valid!";
        model()->removeAllStatements( newSt.subject(), newSt.predicate(), newSt.object() );
        
        Soprano::Statement st( newSt );
        st.setContext( newGraph );
        addStatement( st );
    }
    kDebug() << m_model->statementCount();
}

QUrl Nepomuk::ResourceMerger::mergeGraphs(const QUrl& oldGraph)
{
    using namespace Soprano::Vocabulary;
    
    QList<Soprano::Statement> oldGraphStatements = model()->listStatements( oldGraph, Soprano::Node(), Soprano::Node() ).allStatements();

    kDebug() << oldGraphStatements;
    
    //Convert to prop hash
    QMultiHash<QUrl, Soprano::Node> oldPropHash;
    foreach( const Soprano::Statement & st, oldGraphStatements )
        oldPropHash.insert( st.predicate().uri(), st.object() );

    QHash< QUrl, Soprano::Node >::iterator iter = oldPropHash.find( NAO::maintainedBy() );
    QUrl oldAppUri;
    if( iter != oldPropHash.end() )
        oldAppUri = iter.value().uri();
    
    QHash< QUrl, QUrl >::const_iterator fit = m_graphHash.constFind( oldAppUri );
    if( fit != m_graphHash.constEnd() )
        return fit.value();
    
    if( m_appUri.isEmpty() )
        m_appUri = m_model->findApplicationResource( m_app );

    //
    // If both the graphs have been made by the same application - do nothing
    // FIXME: There might be an additional statement in the new graph
    if( oldAppUri == m_appUri ) {
        return QUrl();
    }
        
    QMultiHash<QUrl, Soprano::Node> newPropHash;
    QHash< QUrl, QVariant >::const_iterator it = m_additionalMetadata.begin();
    for( ; it != m_additionalMetadata.end(); it++ )
        newPropHash.insert( it.key(), Nepomuk::Variant( it.value() ).toNode() );

    QMultiHash<QUrl, Soprano::Node> finalPropHash;

    finalPropHash.insert( RDF::type(), NRL::InstanceBase() );
    if( oldPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) &&
        newPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) )
        finalPropHash.insert( RDF::type(), NRL::DiscardableInstanceBase() );

    oldPropHash.remove( RDF::type() );
    newPropHash.remove( RDF::type() );

    //TODO: Should check for cardinality in the properties
    finalPropHash.unite( oldPropHash );
    finalPropHash.unite( newPropHash );

    finalPropHash.insert( NAO::maintainedBy(), m_appUri );

    const QUrl graph = createGraphUri();
    const QUrl metadatagraph = createGraphUri();
    
    // add metadata graph itself
    m_model->addStatement( metadatagraph, NRL::coreGraphMetadataFor(), graph, metadatagraph );
    m_model->addStatement( metadatagraph, RDF::type(), NRL::GraphMetadata(), metadatagraph );
    
    for(QHash<QUrl, Soprano::Node>::const_iterator it = finalPropHash.constBegin();
        it != finalPropHash.constEnd(); ++it) {

        Soprano::Statement st( graph, it.key(), it.value(), metadatagraph );
        addStatement( st );
    }

    m_graphHash.insert( oldAppUri, graph );
    return graph;
}


KUrl Nepomuk::ResourceMerger::resolveUnidentifiedResource(const KUrl& uri)
{
    // If it is a nepomuk:/ uri, just add it as it is.
    if( uri.scheme() == QLatin1String("nepomuk") )
        return uri;
    else if( uri.url().startsWith("_:") ) // Blank node
        return Nepomuk::Sync::ResourceMerger::resolveUnidentifiedResource( uri );
    return KUrl();
}

QUrl Nepomuk::ResourceMerger::createResourceUri()
{
    return m_model->createUri( DataManagementModel::ResourceUri );
}

QUrl Nepomuk::ResourceMerger::createGraphUri()
{
    return m_model->createUri( DataManagementModel::GraphUri );
}

bool Nepomuk::ResourceMerger::isOfType(const Soprano::Node & node, const QUrl& type, const QList<QUrl> & newTypes) const
{
    kDebug() << "Checking " << node << " for type " << type;
    
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();
    QList<QUrl> types;

    if( !node.isBlank() ) {
        QList< Soprano::Node > oldTypes = m_model->listStatements( node, RDF::type(), Soprano::Node() ).iterateObjects().allNodes();
        foreach( const Soprano::Node & n, oldTypes ) {
            types << n.uri(); 
        }
    }
    types += newTypes;
    
    if( types.isEmpty() ) {
        kDebug() << node << " does not have a type!!";
        return false;
    }

    foreach( const QUrl & uri, types ) {
        if( uri == type || tree->isChildOf( uri, type ) ) {
            return true;
        }
    }
    
    return false;
}

namespace {
    QUrl getBlankOrResourceUri( const Soprano::Node & n ) {
        if( n.isResource() ) {
            return n.uri();
        }
        else if( n.isBlank() ) {
            return QLatin1String("_:") + n.identifier();
        }
        return QUrl();
    }
}

void Nepomuk::ResourceMerger::merge(const Soprano::Graph& graph )
{
    QMultiHash<QUrl, QUrl> types;
    QHash<QPair<QUrl,QUrl>, int> cardinality;

    //
    // First separate all the statements predicate rdf:type.
    // and collect info required to check the types and cardinality

    QList<Soprano::Statement> remainingStatements;
    QList<Soprano::Statement> typeStatements;

    foreach( const Soprano::Statement & st, graph.toList() ) {
        const QUrl subUri = getBlankOrResourceUri( st.subject() );
        const QUrl objUri = getBlankOrResourceUri( st.object() );
        
        if( st.predicate() == RDF::type() ) {
            typeStatements << st;
            types.insert( subUri, objUri );
        }
        else {
            remainingStatements << st;
        }

        // Get the cardinality
        QPair<QUrl,QUrl> subPredPair( subUri, st.predicate().uri() );

        QHash< QPair< QUrl, QUrl >, int >::iterator it = cardinality.find( subPredPair );
        if( it != cardinality.end() ) {
            it.value()++;
        }
        else
            cardinality.insert( subPredPair, 1 );
    }
    
    //
    // Check the cardinality + domain/range of remaining statements
    //
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();
    
    foreach( const Soprano::Statement & st, remainingStatements ) {
        const QUrl subUri = getBlankOrResourceUri( st.subject() );
        const QUrl & propUri = st.predicate().uri();
        //
        // Check for Cardinality
        //
        QPair<QUrl,QUrl> subPredPair( subUri, propUri );

        int maxCardinality = tree->maxCardinality( propUri );
        int existingCardinality = m_model->listStatements( st.subject(), st.predicate(), Soprano::Node() ).allStatements().size();
        int stCardinality = cardinality.value( subPredPair );

        if( maxCardinality > 0 ) {
            if( stCardinality + existingCardinality > maxCardinality) {
                kDebug() << "Max Cardinality : " << maxCardinality;
                kDebug() << "Existing Cardinality: " << existingCardinality;
                kDebug() << "St Cardinality: " << stCardinality;
                m_model->setError( QString::fromLatin1("%1 has a max cardinality of %2")
                                .arg( st.predicate().toString(), maxCardinality ),
                                Soprano::Error::ErrorInvalidStatement);
                return;
            }
        }

        //
        // Check for rdfs:domain and rdfs:range
        //
        
        QUrl domain = tree->propertyDomain( propUri );
        QUrl range = tree->propertyRange( propUri );
        
        kDebug() << "Domain : " << domain;
        kDebug() << "Range : " << range;

        QList<QUrl> subjectNewTypes = types.values( subUri );

        // domain
        if( !domain.isEmpty() && !isOfType( subUri, domain, subjectNewTypes ) ) {
            kDebug() << "invalid domain range";
            m_model->setError( QString::fromLatin1("%1 has a rdfs:domain of %2")
                               .arg( propUri.toString(), domain.toString() ),
                               Soprano::Error::ErrorInvalidArgument);
            return;
        }
        
        // range
        if( !range.isEmpty() ) {
            if( st.object().isResource() || st.object().isBlank() ) {
                
                const QUrl objUri = getBlankOrResourceUri( st.object() );
                QList<QUrl> objectNewTypes= types.values( objUri );
                
                if( !isOfType( objUri, range, objectNewTypes ) ) {

                    kDebug() << "Invalid resource range";
                    m_model->setError( QString::fromLatin1("%1 has a rdfs:range of %2")
                                       .arg( propUri.toString(), range.toString() ),
                                       Soprano::Error::ErrorInvalidArgument);
                    return;
                }
            }
            else if( st.object().isLiteral() ) {
                const Soprano::LiteralValue lv = st.object().literal();
                if( lv.dataTypeUri() != range ) {
                    kDebug() << "Invalid literal range";
                    m_model->setError( QString::fromLatin1("%1 has a rdfs:range of %2")
                                       .arg( propUri.toString(), range.toString() ),
                                       Soprano::Error::ErrorInvalidArgument);
                    return;
                }
            }
        } // range
        
    }


    foreach( const Soprano::Statement & st, graph.toList() ) {
        mergeStatement( st );
        if( m_model->lastError() != Soprano::Error::ErrorNone )
            return;
    }
}


