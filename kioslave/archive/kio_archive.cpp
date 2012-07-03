/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kio_archive.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <QFile>

#include <kglobal.h>
#include <kurl.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <ktar.h>
#include <kzip.h>
#include <k7z.h>
#include <kar.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kde_file.h>
#include <kio/global.h>

#include <kuser.h>

using namespace KIO;

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KComponentData componentData( "kio_archive" );

  kDebug(7109) << "Starting" << getpid();

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_archive protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  ArchiveProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kDebug(7109) << "Done";
  return 0;
}

ArchiveProtocol::ArchiveProtocol( const QByteArray &pool, const QByteArray &app ) : SlaveBase( "tar", pool, app )
{
  kDebug( 7109 ) << "ArchiveProtocol::ArchiveProtocol";
  m_archiveFile = 0L;
}

ArchiveProtocol::~ArchiveProtocol()
{
    delete m_archiveFile;
}

bool ArchiveProtocol::checkNewFile( const KUrl & url, QString & path, KIO::Error& errorNum )
{
#ifndef Q_WS_WIN
    QString fullPath = url.path();
#else
    QString fullPath = url.path().remove(0, 1);
#endif
    kDebug(7109) << "ArchiveProtocol::checkNewFile" << fullPath;


    // Are we already looking at that file ?
    if ( m_archiveFile && m_archiveName == fullPath.left(m_archiveName.length()) )
    {
        // Has it changed ?
        KDE_struct_stat statbuf;
        if ( KDE_stat( QFile::encodeName( m_archiveName ), &statbuf ) == 0 )
        {
            if ( m_mtime == statbuf.st_mtime )
            {
                path = fullPath.mid( m_archiveName.length() );
                kDebug(7109) << "ArchiveProtocol::checkNewFile returning" << path;
                return true;
            }
        }
    }
    kDebug(7109) << "Need to open a new file";

    // Close previous file
    if ( m_archiveFile )
    {
        m_archiveFile->close();
        delete m_archiveFile;
        m_archiveFile = 0L;
    }

    // Find where the tar file is in the full path
    int pos = 0;
    QString archiveFile;
    path.clear();

    int len = fullPath.length();
    if ( len != 0 && fullPath[ len - 1 ] != '/' )
        fullPath += '/';

    kDebug(7109) << "the full path is" << fullPath;
    KDE_struct_stat statbuf;
    statbuf.st_mode = 0; // be sure to clear the directory bit
    while ( (pos=fullPath.indexOf( '/', pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        kDebug(7109) << fullPath << "trying" << tryPath;
        if ( KDE_stat( QFile::encodeName(tryPath), &statbuf ) == -1 )
        {
            // We are not in the file system anymore, either we have already enough data or we will never get any useful data anymore
            break;
        }
        if ( !S_ISDIR(statbuf.st_mode) )
        {
            archiveFile = tryPath;
            m_mtime = statbuf.st_mtime;
#ifdef Q_WS_WIN // st_uid and st_gid provides no information
            m_user.clear();
            m_group.clear();
#else
            KUser user(statbuf.st_uid);
            m_user = user.loginName();
            KUserGroup group(statbuf.st_gid);
            m_group = group.name();
#endif
            path = fullPath.mid( pos + 1 );
            kDebug(7109).nospace() << "fullPath=" << fullPath << " path=" << path;
            len = path.length();
            if ( len > 1 )
            {
                if ( path[ len - 1 ] == '/' )
                    path.truncate( len - 1 );
            }
            else
                path = QString::fromLatin1("/");
            kDebug(7109).nospace() << "Found. archiveFile=" << archiveFile << " path=" << path;
            break;
        }
    }
    if ( archiveFile.isEmpty() )
    {
        kDebug(7109) << "ArchiveProtocol::checkNewFile: not found";
        if ( S_ISDIR(statbuf.st_mode) ) // Was the last stat about a directory?
        {
            // Too bad, it is a directory, not an archive.
            kDebug(7109) << "Path is a directory, not an archive.";
            errorNum = KIO::ERR_IS_DIRECTORY;
        }
        else
            errorNum = KIO::ERR_DOES_NOT_EXIST;
        return false;
    }

    // Open new file
    if ( url.protocol() == "tar" ) {
        kDebug(7109) << "Opening KTar on" << archiveFile;
        m_archiveFile = new KTar( archiveFile );
    } else if ( url.protocol() == "ar" ) {
        kDebug(7109) << "Opening KAr on " << archiveFile;
        m_archiveFile = new KAr( archiveFile );
    } else if ( url.protocol() == "zip" ) {
        kDebug(7109) << "Opening KZip on " << archiveFile;
        m_archiveFile = new KZip( archiveFile );
    } else if ( url.protocol() == "p7zip" ) {
        kDebug(7109) << "Opening K7z on " << archiveFile;
        m_archiveFile = new K7z( archiveFile );
    } else {
        kWarning(7109) << "Protocol" << url.protocol() << "not supported by this IOSlave" ;
        errorNum = KIO::ERR_UNSUPPORTED_PROTOCOL;
        return false;
    }

    if ( !m_archiveFile->open( QIODevice::ReadOnly ) )
    {
        kDebug(7109) << "Opening" << archiveFile << "failed.";
        delete m_archiveFile;
        m_archiveFile = 0L;
        errorNum = KIO::ERR_CANNOT_OPEN_FOR_READING;
        return false;
    }

    m_archiveName = archiveFile;
    return true;
}


void ArchiveProtocol::createRootUDSEntry( KIO::UDSEntry & entry )
{
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, "." );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, m_mtime );
    //entry.insert( KIO::UDSEntry::UDS_ACCESS, 07777 ); // fake 'x' permissions, this is a pseudo-directory
    entry.insert( KIO::UDSEntry::UDS_USER, m_user);
    entry.insert( KIO::UDSEntry::UDS_GROUP, m_group);
}

void ArchiveProtocol::createUDSEntry( const KArchiveEntry * archiveEntry, UDSEntry & entry )
{
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, archiveEntry->name() );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, archiveEntry->permissions() & S_IFMT ); // keep file type only
    entry.insert( KIO::UDSEntry::UDS_SIZE, archiveEntry->isFile() ? ((KArchiveFile *)archiveEntry)->size() : 0L );
    entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, archiveEntry->date());
    entry.insert( KIO::UDSEntry::UDS_ACCESS, archiveEntry->permissions() & 07777 ); // keep permissions only
    entry.insert( KIO::UDSEntry::UDS_USER, archiveEntry->user());
    entry.insert( KIO::UDSEntry::UDS_GROUP, archiveEntry->group());
    entry.insert( KIO::UDSEntry::UDS_LINK_DEST, archiveEntry->symLinkTarget());
}

QString ArchiveProtocol::relativePath( const QString & fullPath )
{
    return QString(fullPath).remove(m_archiveFile->fileName() + QLatin1Char('/'));
}

void ArchiveProtocol::listDir( const KUrl & url )
{
    kDebug( 7109 ) << "ArchiveProtocol::listDir" << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.prettyUrl() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.prettyUrl() );
            return;
        }
        // It's a real dir -> redirect
        KUrl redir;
        redir.setPath( url.path() );
        kDebug( 7109 ) << "Ok, redirection to" << redir.url();
        redirection( redir );
        finished();
        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = 0L;
        return;
    }

    if ( path.isEmpty() )
    {
        KUrl redir( url.protocol() + QString::fromLatin1( ":/") );
        kDebug( 7109 ) << "url.path()=" << url.path();
        redir.setPath( url.path() + QString::fromLatin1("/") );
        kDebug( 7109 ) << "ArchiveProtocol::listDir: redirection" << redir.url();
        redirection( redir );
        finished();
        return;
    }

    kDebug( 7109 ) << "checkNewFile done";
    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveDirectory* dir;
    if (!path.isEmpty() && path != "/")
    {
        kDebug(7109) << "Looking for entry" << path;
        const KArchiveEntry* e = root->entry( path );
        if ( !e )
        {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
            return;
        }
        if ( ! e->isDirectory() )
        {
            error( KIO::ERR_IS_FILE, url.prettyUrl() );
            return;
        }
        dir = (KArchiveDirectory*)e;
    } else {
        dir = root;
    }

    const QStringList l = dir->entries();
    totalSize( l.count() );

    UDSEntry entry;
    if (!l.contains(".")) {
        createRootUDSEntry(entry);
        listEntry(entry, false);
    }

    QStringList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
        kDebug(7109) << (*it);
        const KArchiveEntry* archiveEntry = dir->entry( (*it) );

        createUDSEntry( archiveEntry, entry );

        listEntry( entry, false );
    }

    listEntry( entry, true ); // ready

    finished();

    kDebug( 7109 ) << "ArchiveProtocol::listDir done";
}

void ArchiveProtocol::stat( const KUrl & url )
{
    QString path;
    UDSEntry entry;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        // We may be looking at a real directory - this happens
        // when pressing up after being in the root of an archive
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.prettyUrl() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.prettyUrl() );
            return;
        }
        // Real directory. Return just enough information for KRun to work
        entry.insert( KIO::UDSEntry::UDS_NAME, url.fileName());
        kDebug( 7109 ).nospace() << "ArchiveProtocol::stat returning name=" << url.fileName();

        KDE_struct_stat buff;
#ifdef Q_WS_WIN
        QString fullPath = url.path().remove(0, 1);
#else
        QString fullPath = url.path();
#endif

        if ( KDE_stat( QFile::encodeName( fullPath ), &buff ) == -1 )
        {
            // Should not happen, as the file was already stated by checkNewFile
            error( KIO::ERR_COULD_NOT_STAT, url.prettyUrl() );
            return;
        }

        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, buff.st_mode & S_IFMT);

        statEntry( entry );

        finished();

        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = 0L;
        return;
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry;
    if ( path.isEmpty() )
    {
        path = QString::fromLatin1( "/" );
        archiveEntry = root;
    } else {
        archiveEntry = root->entry( path );
    }
    if ( !archiveEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        return;
    }

    createUDSEntry( archiveEntry, entry );
    statEntry( entry );

    finished();
}

void ArchiveProtocol::get( const KUrl & url )
{
    kDebug( 7109 ) << "ArchiveProtocol::get" << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.prettyUrl() ) );
            return;
        }
        else
        {
            // We have any other error
            error( errorNum, url.prettyUrl() );
            return;
        }
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry = root->entry( path );

    if ( !archiveEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
        return;
    }
    if ( archiveEntry->isDirectory() )
    {
        error( KIO::ERR_IS_DIRECTORY, url.prettyUrl() );
        return;
    }
    const KArchiveFile* archiveFileEntry = static_cast<const KArchiveFile *>(archiveEntry);
    if ( !archiveEntry->symLinkTarget().isEmpty() )
    {
      kDebug(7109) << "Redirection to" << archiveEntry->symLinkTarget();
      KUrl realURL( url, archiveEntry->symLinkTarget() );
      kDebug(7109).nospace() << "realURL=" << realURL.url();
      redirection( realURL );
      finished();
      return;
    }

    //kDebug(7109) << "Preparing to get the archive data";

    /*
     * The easy way would be to get the data by calling archiveFileEntry->data()
     * However this has drawbacks:
     * - the complete file must be read into the memory
     * - errors are skipped, resulting in an empty file
     */

    QIODevice* io = archiveFileEntry->createDevice();

    if (!io)
    {
        error( KIO::ERR_SLAVE_DEFINED,
            i18n( "The archive file could not be opened, perhaps because the format is unsupported.\n%1" ,
                      url.prettyUrl() ) );
        return;
    }

    if ( !io->open( QIODevice::ReadOnly ) )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl() );
        delete io;
        return;
    }

    totalSize( archiveFileEntry->size() );

    // Size of a QIODevice read. It must be large enough so that the mime type check will not fail
    const qint64 maxSize = 0x100000; // 1MB

    qint64 bufferSize = qMin( maxSize, archiveFileEntry->size() );
    QByteArray buffer;
    buffer.resize( bufferSize );
    if ( buffer.isEmpty() && bufferSize > 0 )
    {
        // Something went wrong
        error( KIO::ERR_OUT_OF_MEMORY, url.prettyUrl() );
        delete io;
        return;
    }

    bool firstRead = true;

    // How much file do we still have to process?
    qint64 fileSize = archiveFileEntry->size();
    KIO::filesize_t processed = 0;

    while ( !io->atEnd() && fileSize > 0 )
    {
        if ( !firstRead )
        {
            bufferSize = qMin( maxSize, fileSize );
            buffer.resize( bufferSize );
        }
        const qint64 read = io->read( buffer.data(), buffer.size() ); // Avoid to use bufferSize here, in case something went wrong.
        if ( read != bufferSize )
        {
            kWarning(7109) << "Read" << read << "bytes but expected" << bufferSize ;
            error( KIO::ERR_COULD_NOT_READ, url.prettyUrl() );
            delete io;
            return;
        }
        if ( firstRead )
        {
            // We use the magic one the first data read
            // (As magic detection is about fixed positions, we can be sure that it is enough data.)
            KMimeType::Ptr mime = KMimeType::findByNameAndContent( path, buffer );
            kDebug(7109) << "Emitting mimetype" << mime->name();
            mimeType( mime->name() );
            firstRead = false;
        }
        data( buffer );
        processed += read;
        processedSize( processed );
        fileSize -= bufferSize;
    }
    io->close();
    delete io;

    data( QByteArray() );

    finished();
}

void ArchiveProtocol::put( const KUrl & url, int permissions, KIO::JobFlags flags  )
{
    kDebug(7109);

#if 0
    if (url.protocol() != QLatin1String("p7zip")) {
        kDebug(7109) << "put operation not supported by protocol" << url.protocol();
        finished();
        return;
    }
#endif

    QString destName = relativePath(url.path());
    Q_ASSERT(!destName.isEmpty());
    kDebug(7109) << "Putting" << url << "as" << destName;

    const KArchiveEntry * ent = m_archiveFile->directory()->entry( destName );

    if (ent && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
        if (ent->isDirectory()) {
            kDebug(7109) << url <<" already isdir !!";
            error( KIO::ERR_DIR_ALREADY_EXIST, url.prettyUrl());
        } else {
            kDebug(7109) << url << " already exist !!";
            error( KIO::ERR_FILE_ALREADY_EXIST, url.prettyUrl());
        }
        return;
    }

    if (ent && !(flags & KIO::Resume) && (flags & KIO::Overwrite)) {
        kDebug(7109) << "exists try to remove " << url;
        // TODO: implement
    }

    if (flags & KIO::Resume) {
        // TODO: implement
        kDebug(7109) << "resume not supported " << url;
        error( KIO::ERR_UNSUPPORTED_ACTION, url.prettyUrl());
        return;
    } else {
        if (permissions == -1) {
            permissions = 600;//0666;
        } else {
            permissions = permissions | S_IWUSR | S_IRUSR;
        }
    }

    if (!m_archiveFile->open(QIODevice::WriteOnly)) {
        kWarning() << " open" << m_archiveFile->fileName() << "failed";
        error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyUrl());
    }

    const QString mtimeStr = metaData( "modified" );
    time_t mtime = 0;
    if ( !mtimeStr.isEmpty() ) {
        QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
        if ( dt.isValid() ) {
            mtime = dt.toTime_t();
            kDebug(7109) << "setting modified time to" << dt;
        }
    }

    // TODO: set the missing metadata.
    if (!m_archiveFile->prepareWriting(destName, m_archiveFile->directory()->user(), m_archiveFile->directory()->group(), 0 /*size*/,
                                       permissions, 0 /*atime*/, mtime, 0 /*ctime*/))
    {
        kWarning() << " prepareWriting" << destName << "failed";
        error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyUrl());
        return;
    }

    // Read and write data in chunks to minimize memory usage
    QByteArray buffer;
    qint64 total = 0;
    while (1) {
        dataReq(); // Request for data

        if (readData(buffer) <= 0) {
            kDebug(7109) << "readData <= 0";
            break;
        }

        if ( !m_archiveFile->writeData( buffer.data(), buffer.size() ) ) {
            kWarning() << "writeData failed";
            error(ERR_COULD_NOT_WRITE, url.prettyUrl());
            return;
        }
	kDebug(7109) << "Wrote" << buffer.size() << "bytes";
        total += buffer.size();
    }

    if ( !m_archiveFile->finishWriting( total /* to set file size */ ) ) {
        kWarning() << "finishWriting failed";
        error(ERR_COULD_NOT_WRITE, url.prettyUrl());
        return;
    }

    kDebug(7109) << "closing archive";
    m_archiveFile->close();
    finished();
    kDebug(7109) << "finished";
}

void ArchiveProtocol::close()
{
    kDebug(7109);
    m_archiveFile->close();
    finished();
}

void ArchiveProtocol::copy( const KUrl& src, const KUrl &dest, int permissions, KIO::JobFlags flags )
{
    kDebug(7109) << src << dest;
    QString srcRelativePath = relativePath(src.path());
    kDebug(7109) << "srcRelativePath == " << srcRelativePath;
    Q_ASSERT(!srcRelativePath.isEmpty());

    const KArchiveEntry * ent = m_archiveFile->directory()->entry( srcRelativePath );

    if (!ent) {
        error( ERR_DOES_NOT_EXIST, src.prettyUrl() );
        return;
    }

    if (ent->isDirectory()) {
        error( KIO::ERR_IS_DIRECTORY, src.prettyUrl() );
        return;
    }

    QString destRelativePath = relativePath(dest.path());
    kDebug(7109) << "destRelativePath == " << destRelativePath;
    Q_ASSERT(!destRelativePath.isEmpty());

    const KArchiveEntry * ent2 = m_archiveFile->directory()->entry( destRelativePath );

    if (ent2) {
        if (ent2->isDirectory()) {
            error( KIO::ERR_DIR_ALREADY_EXIST, dest.prettyUrl() );
            return;
        } else if (!(flags & KIO::Overwrite))  {
            error( KIO::ERR_FILE_ALREADY_EXIST, dest.prettyUrl() );
            return;
        }
    }

    const KArchiveFile * srcArchiveFile = dynamic_cast<const KArchiveFile *>(ent);
    if (!srcArchiveFile) {
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, src.prettyUrl());
        return;
    }
    QIODevice * ioDevice = srcArchiveFile->createDevice();
    ioDevice->open(QIODevice::ReadOnly);
    QByteArray buffer = ioDevice->readAll();
    ioDevice->close();
    ioDevice->deleteLater();

    if (!m_archiveFile->open(QIODevice::WriteOnly)) {
        kWarning() << " open" << m_archiveFile->fileName() << "failed";
        error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, dest.prettyUrl());
    }

    const QString mtimeStr = metaData( "modified" );
    time_t mtime = 0;
    if ( !mtimeStr.isEmpty() ) {
        QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
        if ( dt.isValid() ) {
            mtime = dt.toTime_t();
            kDebug(7109) << "setting modified time to" << dt;
        }
    }

    // TODO: set the missing metadata.
    if (!m_archiveFile->prepareWriting(destRelativePath, m_archiveFile->directory()->user(), m_archiveFile->directory()->group(), 0 /*size*/,
                                       permissions, 0 /*atime*/, mtime, 0 /*ctime*/))
    {
        kWarning() << " prepareWriting" << destRelativePath << "failed";
        error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, dest.prettyUrl());
        return;
    }

    // Read and write data in chunks to minimize memory usage.
    /*QByteArray buffer;
    QIODevice * ioDevice = srcArchiveFile->createDevice();
    qint64 total = 0;
    while (1) {
        buffer = ioDevice->read(1024 * 1024);
        if (buffer.isEmpty()) {
            break;
        }*/
    
        if ( !m_archiveFile->writeData( buffer.data(), buffer.size() ) ) {
            kWarning() << "writeData failed";
            error(ERR_COULD_NOT_WRITE, dest.prettyUrl());
            return;
        }
	kDebug(7109) << "Wrote" << buffer.size() << "bytes";
        /*total += buffer.size();
    }
    ioDevice->deleteLater();*/

    if ( !m_archiveFile->finishWriting( /*total*/ buffer.size() /* to set file size */ ) ) {
        kWarning() << "finishWriting failed";
        error(ERR_COULD_NOT_WRITE, dest.prettyUrl());
        return;
    }

    kDebug(7109) << "closing archive";
    m_archiveFile->close();

    finished();
}

void ArchiveProtocol::del( const KUrl & url, bool isFile )
{
    kDebug(7109) << url;

    QString relPath = relativePath(url.path());
    Q_ASSERT(!relPath.isEmpty());
    kDebug(7109) << "Deleting" << relPath << "from" << url;

    // when renaming files it can be closed at this point.
    if (!m_archiveFile->isOpen()) {
        m_archiveFile->open(QIODevice::ReadOnly);
    }

    const KArchiveEntry * ent = m_archiveFile->directory()->entry( relPath );

    if (!ent) {
        error( ERR_DOES_NOT_EXIST, url.prettyUrl() );
        return;
    }

    if (!m_archiveFile->open(QIODevice::WriteOnly)) {
        kWarning() << " deleting" << relPath << "failed";
        error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyUrl());
    }

    if ( url.protocol() == "p7zip" ) {
        QString relPath = relativePath(url.path());
        Q_ASSERT(!relPath.isEmpty());

        if (!(dynamic_cast<K7z *>(m_archiveFile)->del(relPath, isFile))) {
            if (isFile) {
                error( KIO::ERR_CANNOT_DELETE, url.prettyUrl() );
            } else {
                error( KIO::ERR_COULD_NOT_RMDIR, url.prettyUrl() );
            }
            m_archiveFile->close();
            return;
        }
    } else {
        error( KIO::ERR_UNSUPPORTED_ACTION, url.prettyUrl());
        return;
    }

    m_archiveFile->close();
    finished();
}

void ArchiveProtocol::mkdir( const KUrl & url, int permissions )
{
    kDebug(7109) << url;
    QString destName = relativePath(url.path());
    Q_ASSERT(!destName.isEmpty());

    const KArchiveEntry * ent = m_archiveFile->directory()->entry( destName );

    if (ent) {
        if (ent->isDirectory()) {
            kDebug(7109) << url <<" already isdir !!";
            error( KIO::ERR_DIR_ALREADY_EXIST, url.prettyUrl());
        } else {
            kDebug(7109) << url << " already exist !!";
            error( KIO::ERR_FILE_ALREADY_EXIST, url.prettyUrl());
        }
        return;
    }

    time_t time = QDateTime::currentDateTime().toTime_t();
    if (!m_archiveFile->writeDir( destName, m_archiveFile->directory()->user(), m_archiveFile->directory()->group(),
                             permissions, time /*atime*/, time /*mtime*/, time /*ctime*/ )) {
        error( KIO::ERR_COULD_NOT_MKDIR, url.prettyUrl() );
        return;
    }

    finished();
}

void ArchiveProtocol::rename( const KUrl& src, const KUrl& dest, KIO::JobFlags flags )
{
    kDebug(7109) << src << dest;

    // kio is smart enough to do copy(src, dest) + del(src) actions to simulate renaming.
    error( KIO::ERR_UNSUPPORTED_ACTION, src.prettyUrl() );
}

/*
  In case someone wonders how the old filter stuff looked like :    :)
void TARProtocol::slotData(void *_p, int _len)
{
  switch (m_cmd) {
    case CMD_PUT:
      assert(m_pFilter);
      m_pFilter->send(_p, _len);
      break;
    default:
      abort();
      break;
    }
}

void TARProtocol::slotDataEnd()
{
  switch (m_cmd) {
    case CMD_PUT:
      assert(m_pFilter && m_pJob);
      m_pFilter->finish();
      m_pJob->dataEnd();
      m_cmd = CMD_NONE;
      break;
    default:
      abort();
      break;
    }
}

void TARProtocol::jobData(void *_p, int _len)
{
  switch (m_cmd) {
  case CMD_GET:
    assert(m_pFilter);
    m_pFilter->send(_p, _len);
    break;
  case CMD_COPY:
    assert(m_pFilter);
    m_pFilter->send(_p, _len);
    break;
  default:
    abort();
  }
}

void TARProtocol::jobDataEnd()
{
  switch (m_cmd) {
  case CMD_GET:
    assert(m_pFilter);
    m_pFilter->finish();
    dataEnd();
    break;
  case CMD_COPY:
    assert(m_pFilter);
    m_pFilter->finish();
    m_pJob->dataEnd();
    break;
  default:
    abort();
  }
}

void TARProtocol::filterData(void *_p, int _len)
{
debug("void TARProtocol::filterData");
  switch (m_cmd) {
  case CMD_GET:
    data(_p, _len);
    break;
  case CMD_PUT:
    assert (m_pJob);
    m_pJob->data(_p, _len);
    break;
  case CMD_COPY:
    assert(m_pJob);
    m_pJob->data(_p, _len);
    break;
  default:
    abort();
  }
}
*/

// kate: space-indent on; indent-width 4; replace-tabs on;
