/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

   The basis for this file was generated by qdbusxml2cpp version 0.7
   Command line was: qdbusxml2cpp -a datamanagementadaptor -c DataManagementAdaptor -m org.kde.nepomuk.DataManagement.xml

   qdbusxml2cpp is Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 */

#include "datamanagementadaptor.h"
#include "datamanagementmodel.h"
#include "datamanagementcommand.h"
#include "simpleresourcegraph.h"
#include "dbustypes.h"

#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QThreadPool>

#define USING_SOPRANO_NRLMODEL_UNSTABLE_API
#include <Soprano/NRLModel>


Nepomuk::DataManagementAdaptor::DataManagementAdaptor(Nepomuk::DataManagementModel *parent)
    : QObject(parent),
      m_model(parent),
      m_namespacePrefixRx(QLatin1String("(\\w+)\\:(\\w+)"))
{
    DBus::registerDBusTypes();

    m_threadPool = new QThreadPool(this);

    // never let go of our threads - that is just pointless cpu cycles wasted
    m_threadPool->setExpiryTimeout(-1);

    // N threads means N connections to Virtuoso
    m_threadPool->setMaxThreadCount(10);
}

Nepomuk::DataManagementAdaptor::~DataManagementAdaptor()
{
}

void Nepomuk::DataManagementAdaptor::addProperty(const QStringList &resources, const QString &property, const QVariantList &values, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new AddPropertyCommand(decodeUris(resources), decodeUri(property), values, app, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::addProperty(const QString &resource, const QString &property, const QDBusVariant &value, const QString &app)
{
    addProperty(QStringList() << resource, property, QVariantList() << value.variant(), app);
}

QString Nepomuk::DataManagementAdaptor::createResource(const QStringList &types, const QString &label, const QString &description, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new CreateResourceCommand(decodeUris(types), label, description, app, m_model, message()));
    // QtDBus will ignore this return value
    return QString();
}

QString Nepomuk::DataManagementAdaptor::createResource(const QString &type, const QString &label, const QString &description, const QString &app)
{
    return createResource(QStringList() << type, label, description, app);
}

QList<Nepomuk::SimpleResource> Nepomuk::DataManagementAdaptor::describeResources(const QStringList &resources, bool includeSubResources)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new DescribeResourcesCommand(decodeUris(resources), includeSubResources, m_model, message()));
    // QtDBus will ignore this return value
    return QList<SimpleResource>();
}

void Nepomuk::DataManagementAdaptor::storeResources(const QList<Nepomuk::SimpleResource>& resources, int identificationMode, int flags, const Nepomuk::PropertyHash &additionalMetadata, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new StoreResourcesCommand(resources, app, identificationMode, flags, additionalMetadata, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::mergeResources(const QString &resource1, const QString &resource2, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new MergeResourcesCommand(decodeUri(resource1), decodeUri(resource2), app, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeDataByApplication(int flags, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new RemoveDataByApplicationCommand(app, flags, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeDataByApplication(const QStringList &resources, int flags, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new RemoveResourcesByApplicationCommand(decodeUris(resources), app, flags, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeProperties(const QStringList &resources, const QStringList &properties, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new RemovePropertiesCommand(decodeUris(resources), decodeUris(properties), app, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeProperties(const QString &resource, const QString &property, const QString &app)
{
    removeProperties(QStringList() << resource, QStringList() << property, app);
}

void Nepomuk::DataManagementAdaptor::removeProperty(const QStringList &resources, const QString &property, const QVariantList &values, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new RemovePropertyCommand(decodeUris(resources), decodeUri(property), values, app, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeProperty(const QString &resource, const QString &property, const QDBusVariant &value, const QString &app)
{
    removeProperty(QStringList() << resource, property, QVariantList() << value.variant(), app);
}

void Nepomuk::DataManagementAdaptor::removeResources(const QStringList &resources, int flags, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new RemoveResourcesCommand(decodeUris(resources), app, flags, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::removeResources(const QString &resource, int flags, const QString &app)
{
    removeResources(QStringList() << resource, flags, app);
}

void Nepomuk::DataManagementAdaptor::setProperty(const QStringList &resources, const QString &property, const QVariantList &values, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new SetPropertyCommand(decodeUris(resources), decodeUri(property), values, app, m_model, message()));
}

void Nepomuk::DataManagementAdaptor::setProperty(const QString &resource, const QString &property, const QDBusVariant &value, const QString &app)
{
    setProperty(QStringList() << resource, property, QVariantList() << value.variant(), app);
}

void Nepomuk::DataManagementAdaptor::enqueueCommand(DataManagementCommand *cmd)
{
    m_threadPool->start(cmd);
}

QUrl Nepomuk::DataManagementAdaptor::decodeUri(const QString &s, bool namespaceAbbrExpansion) const
{
    if(namespaceAbbrExpansion) {
        if(m_namespacePrefixRx.exactMatch(s)) {
            const QString ns = m_namespacePrefixRx.cap(1);
            const QString name = m_namespacePrefixRx.cap(2);
            QHash<QString, QString>::const_iterator it = m_namespaces.constFind(ns);
            if(it != m_namespaces.constEnd()) {
                return QUrl::fromEncoded(QString(it.value() + name).toAscii());
            }
        }
    }

    // fallback
    return Nepomuk::decodeUrl(s);
}

QList<QUrl> Nepomuk::DataManagementAdaptor::decodeUris(const QStringList &urlStrings, bool namespaceAbbrExpansions) const
{
    QList<QUrl> urls;
    Q_FOREACH(const QString& urlString, urlStrings) {
        urls << decodeUri(urlString, namespaceAbbrExpansions);
    }
    return urls;
}

void Nepomuk::DataManagementAdaptor::setPrefixes(const QHash<QString, QString>& prefixes)
{
    m_namespaces = prefixes;
}

void Nepomuk::DataManagementAdaptor::importResources(const QString &url, const QString &serialization, int identificationMode, int flags, const QString &app)
{
    importResources(url, serialization, identificationMode, flags, PropertyHash(), app);
}

void Nepomuk::DataManagementAdaptor::importResources(const QString &url, const QString &serialization, int identificationMode, int flags, const Nepomuk::PropertyHash &additionalMetadata, const QString &app)
{
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);
    enqueueCommand(new ImportResourcesCommand(decodeUri(url), Soprano::mimeTypeToSerialization(serialization), serialization, identificationMode, flags, additionalMetadata, app, m_model, message()));
}

#include "datamanagementadaptor.moc"
