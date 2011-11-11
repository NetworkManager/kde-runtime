/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>

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

#include "indexer.h"
#include "nepomukindexwriter.h"

#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <strigi/strigiconfig.h>
#include <strigi/indexwriter.h>
#include <strigi/analysisresult.h>
#include <strigi/fileinputstream.h>
#include <strigi/analyzerconfiguration.h>

#include <iostream>


namespace {
    class StoppableConfiguration : public Strigi::AnalyzerConfiguration
    {
    public:
        StoppableConfiguration()
            : m_stop(false) {
#if defined(STRIGI_IS_VERSION)
#if STRIGI_IS_VERSION( 0, 6, 1 )
            setIndexArchiveContents( false );
#endif
#endif
        }

        bool indexMore() const {
            return !m_stop;
        }

        bool addMoreText() const {
            return !m_stop;
        }

        void setStop( bool s ) {
            m_stop = s;
        }

    private:
        bool m_stop;
    };
}


class Nepomuk::Indexer::Private
{
public:
    StoppableConfiguration m_analyzerConfig;
    StrigiIndexWriter* m_indexWriter;
    Strigi::StreamAnalyzer* m_streamAnalyzer;
};


Nepomuk::Indexer::Indexer( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->m_indexWriter = new StrigiIndexWriter();
    d->m_streamAnalyzer = new Strigi::StreamAnalyzer( d->m_analyzerConfig );
    d->m_streamAnalyzer->setIndexWriter( *d->m_indexWriter );
}


Nepomuk::Indexer::~Indexer()
{
    delete d->m_streamAnalyzer;
    delete d->m_indexWriter;
    delete d;
}


void Nepomuk::Indexer::indexFile( const KUrl& url, const KUrl resUri, uint mtime )
{
    indexFile( QFileInfo( url.toLocalFile() ), resUri, mtime );
}


void Nepomuk::Indexer::indexFile( const QFileInfo& info, const KUrl resUri, uint mtime )
{
    if( !info.exists() ) {
        kDebug() << info.filePath() << " does not exist";
        return;
    }
    
    d->m_analyzerConfig.setStop( false );
    d->m_indexWriter->forceUri( resUri );
    
    // strigi asserts if the file path has a trailing slash
    const KUrl url( info.filePath() );
    const QString filePath = url.toLocalFile( KUrl::RemoveTrailingSlash );
    const QString dir = url.directory( KUrl::IgnoreTrailingSlash );

    kDebug() << "Starting to analyze" << info.filePath();

    Strigi::AnalysisResult analysisresult( QFile::encodeName( filePath ).data(),
                                           mtime ? mtime : info.lastModified().toTime_t(),
                                           *d->m_indexWriter,
                                           *d->m_streamAnalyzer,
                                           QFile::encodeName( dir ).data() );
    if ( info.isFile() && !info.isSymLink() ) {
#ifdef STRIGI_HAS_FILEINPUTSTREAM_OPEN
        Strigi::InputStream* stream = Strigi::FileInputStream::open( QFile::encodeName( info.filePath() ) );
        analysisresult.index( stream );
        delete stream;
#else
        Strigi::FileInputStream stream( QFile::encodeName( info.filePath() ) );
        analysisresult.index( &stream );
#endif
    }
    else {
        analysisresult.index(0);
    }
}

void Nepomuk::Indexer::indexStdin(const KUrl resUri, uint mtime)
{
    d->m_analyzerConfig.setStop( false );
    d->m_indexWriter->forceUri( resUri );

    Strigi::AnalysisResult analysisresult( QFile::encodeName(resUri.fileName()).data(),
                                           mtime ? mtime : QDateTime::currentDateTime().toTime_t(),
                                           *d->m_indexWriter,
                                           *d->m_streamAnalyzer );
    Strigi::FileInputStream stream( stdin, QFile::encodeName(resUri.toLocalFile()).data() );
    analysisresult.index( &stream );
}

#include "indexer.moc"