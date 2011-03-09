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

#include "resourcegraphstoringjob_p.h"
#include "simpleresourcegraph.h"
#include "datamanagementinterface.h"
#include "dbustypes.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusPendingCallWatcher>

#include <QtCore/QVariant>
#include <QtCore/QHash>

#include <KComponentData>


Nepomuk::ResourceGraphStoringJob::ResourceGraphStoringJob(const Nepomuk::SimpleResourceGraph& graph,
                                                          const QHash<QUrl, QVariant>& additionalMetadata,
                                                          const KComponentData& component)
    : KJob(0)
{
    org::kde::nepomuk::DataManagement dms(QLatin1String("org.kde.nepomuk.services.DataManagement"),
                                          QLatin1String("/datamanagementmodel"),
                                          QDBusConnection::sessionBus());
    QDBusPendingCallWatcher* dbusCallWatcher
            = new QDBusPendingCallWatcher(dms.storeResources(graph,
                                                             Nepomuk::DBus::convertMetadataHash(additionalMetadata),
                                                             component.componentName()));
    connect(dbusCallWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(slotDBusCallFinished(QDBusPendingCallWatcher*)));
}

Nepomuk::ResourceGraphStoringJob::~ResourceGraphStoringJob()
{
}

void Nepomuk::ResourceGraphStoringJob::start()
{
    // do nothing. we all do it in the constructor to avoid storing any members
}

void Nepomuk::ResourceGraphStoringJob::slotDBusCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    if (reply.isError()) {
        QDBusError error = reply.error();
        setErrorText(error.message());
    }
    watcher->deleteLater();
    emitResult();
}

#include "resourcegraphstoringjob_p.moc"
