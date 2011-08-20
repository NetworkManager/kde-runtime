/*
Copyright (C) 2011  Smit Shah <who828@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "writebackservice.h"
#include "writebackplugin.h"
#include "writebackjob.h"

#include <kmimetypetrader.h>
#include <KDebug>
#include <KUrl>
#include <KService>
#include <KServiceTypeTrader>

#include <QtCore/QString>
#include <QList>
#include <QDebug>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Variant>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Types/Property>

using namespace Nepomuk::Vocabulary;

Nepomuk::WriteBackService::WriteBackService( QObject* parent, const QList< QVariant >& )
    : Service(parent),
      m_currentjob(0)
{
    ResourceWatcher* resourcewatcher = new ResourceWatcher(this);

    foreach(const KService::Ptr& service, KServiceTypeTrader::self()->query(QLatin1String("Nepomuk/WritebackPlugin"))) {
        foreach(const QString& type, service->property("X-Nepomuk-ResourceTypes", QVariant::StringList).toStringList()) {
            resourcewatcher->addType(Nepomuk::Types::Class(KUrl(type)));
        }
    }

    connect( resourcewatcher, SIGNAL( propertyAdded(Nepomuk::Resource, Nepomuk::Types::Property, QVariant) ),
             this, SLOT ( slotQueue(Nepomuk::Resource) ) );
    connect( resourcewatcher, SIGNAL( propertyRemoved(Nepomuk::Resource, Nepomuk::Types::Property, QVariant) ),
             this, SLOT ( slotQueue(Nepomuk::Resource) ) );

    resourcewatcher->start();
}

Nepomuk::WriteBackService::~WriteBackService()
{
}

void Nepomuk::WriteBackService::slotQueue(const Nepomuk::Resource &resource)
{
    if (!m_queue.contains(resource))
        m_queue.enqueue(resource);

    startWriteback();
}

void Nepomuk::WriteBackService::startWriteback()
{
    if (m_currentjob == 0 && !m_queue.isEmpty())
        writebackResource(m_queue.dequeue());
}

void Nepomuk::WriteBackService::slotFinished()
{
    m_currentjob = 0;
    startWriteback();
}

void Nepomuk::WriteBackService::writebackResource(const Nepomuk::Resource & resource)
{
    if (resource.hasType(NFO::FileDataObject())) {
        const QStringList mimetypes = resource.property(NIE::mimeType()).toStringList();
        if (!mimetypes.isEmpty()) {
            QString  mimetype = mimetypes.first();

            KService::List services
            = KMimeTypeTrader::self()->query(mimetype,QString::fromLatin1 ("Nepomuk/WritebackPlugin"));

            performWriteback(services,resource);
        }
    }
    else {
        QStringList subQueries( "'*' in [X-Nepomuk-ResourceTypes]" );
        for ( int i = 0; i < (resource.types()).count(); ++i ) {
            subQueries << QString::fromLatin1( "'%1' in [X-Nepomuk-ResourceTypes]" ).arg( resource.types().at(i).toString() );
        }

        KService::List services
                = KServiceTypeTrader::self()->query( QString::fromLatin1 ("Nepomuk/WritebackPlugin"), subQueries.join( " or " ) );

        performWriteback(services,resource);
    }
}

void Nepomuk::WriteBackService::performWriteback(const KService::List services,const Nepomuk::Resource & resource)
{
    QList<WritebackPlugin*> plugins;
    foreach(const KSharedPtr<KService>& service, services) {
        if (WritebackPlugin* plugin = service->createInstance<WritebackPlugin>()) {
            plugins.append(plugin);
        }
    }

    if (!plugins.isEmpty()) {
        m_currentjob = new WritebackJob(this);
        m_currentjob->setPlugins(plugins);
        m_currentjob->setResource(resource);
        connect(m_currentjob,SIGNAL(result(KJob *)),this,SLOT(slotFinished()));
        m_currentjob->start();
    }
}

#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::WriteBackService, "nepomukwritebackservice" )

#include "writebackservice.moc"