/*
 *   Copyright (C) 2011 Ivan Cukic ivan.cukic(at)kde.org
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rankings.h"
#include "rankingsadaptor.h"

#include <QDBusConnection>
#include <QVariantList>
#include <KDebug>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>

#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/NegationTerm>

#include <Soprano/QueryResultIterator>
#include <Soprano/Node>
#include <Soprano/Model>

#include <Nepomuk/Vocabulary/NUAO>
#include <Soprano/Vocabulary/NAO>
#include "kext.h"

#include "NepomukCommon.h"

#define RESULT_COUNT_LIMIT 10
#define COALESCE_ACTIVITY(Activity) ((Activity.isEmpty()) ? \
        (NepomukPlugin::self()->sharedInfo()->currentActivity()) : (Activity))

using namespace Soprano::Vocabulary;
using namespace Nepomuk::Vocabulary;
using namespace Nepomuk::Query;

Rankings * Rankings::s_instance = NULL;

void Rankings::init(QObject * parent)
{
    if (s_instance) return;

    s_instance = new Rankings(parent);
}

Rankings * Rankings::self()
{
    return s_instance;
}

Rankings::Rankings(QObject * parent)
    : QObject(parent)
{
    kDebug() << "We are in the Rankings";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    new RankingsAdaptor(this);
    dbus.registerObject("/Rankings", this);
}

Rankings::~Rankings()
{
}

void Rankings::resourceScoreUpdated(const QString & activity,
        const QString & application, const QUrl & uri, qreal score)
{
    kDebug() << activity << application << uri << score;

    QList < ResultItem > & list = m_results[activity];

    for (int i = 0; i < list.size(); i++) {
        if (list[i].uri == uri) {
            list.removeAt(i);
            break;
        }
    }

    int i = list.size() - 1;

    kDebug() << "List size is:" << i;

    if (i < 0) {
        list << ResultItem(uri, score);

        return;
    }

    while (list[i].score < score) {
        --i;
    }

    if (i + 1 < RESULT_COUNT_LIMIT) {
        list.insert(i + 1, ResultItem(uri, score));
    }

    while (list.size() > RESULT_COUNT_LIMIT) {
        list.removeLast();
    }

    notifyResultsUpdated(activity);
}

void Rankings::registerClient(const QString & client,
        const QString & activity, const QString & type)
{
    kDebug() << client << "wants to get resources for" << activity;

    if (!m_clients.contains(activity)) {
        kDebug() << "Initialising the resources for" << activity;
        initResults(COALESCE_ACTIVITY(activity));
    }

    if (!m_clients[activity].contains(client)) {
        kDebug() << "Adding client";
        m_clients[activity] << client;

        notifyResultsUpdated(activity, QStringList() << client);
    }
}

void Rankings::deregisterClient(const QString & client)
{
    QMutableHashIterator < Activity, QStringList > i(m_clients);

    while (i.hasNext()) {
        i.next();

        i.value().removeAll(client);
        if (i.value().isEmpty()) {
            i.remove();
        }
    }
}

void Rankings::setCurrentActivity(const QString & activity)
{
    // We need to update scores for items that have no
    // activity specified

    initResults(activity);
}

void Rankings::initResults(const QString & activity)
{
    m_results[activity].clear();

    const QString query = QString::fromLatin1(
        "select distinct ?resource, "
        "( "
            "( "
                "SUM ( "
                    "?lastScore * bif:exp( "
                        "- bif:datediff('day', ?lastUpdate, %1) "
                    ") "
                ") "
            ") "
            "as ?score "
        ") where { "
            "?cache kext:targettedResource ?resource . "
            "?cache a kext:ResourceScoreCache . "
            "?cache nao:lastModified ?lastUpdate . "
            "?cache kext:cachedScore ?lastScore . "
            "?resource nao:prefLabel ?label . "
            "?resource nie:url ?icon . "
            "?resource nie:url ?description . "
            "?cache kext:usedActivity %2 . "
        "} "
        "GROUP BY (?resource) ORDER BY DESC (?score) LIMIT 10"
    ).arg(
        litN3(QDateTime::currentDateTime()),
        resN3(activityResource(COALESCE_ACTIVITY(activity)))
    );

    kDebug() << query;

    Soprano::QueryResultIterator it
        = Nepomuk::ResourceManager::instance()->mainModel()
                ->executeQuery(query, Soprano::Query::QueryLanguageSparql);

    while (it.next()) {
        Nepomuk::Resource result(it[0].uri());

        kDebug() << "This is one result" << it[0].uri() << result
            << it[1].literal().toDouble();

        // kDebug() << "###" << result.resourceUri()
        //     << result.label() << result.genericIcon();

        m_results[activity] << ResultItem(it[0].uri(), it[1].literal().toDouble());
    }

    it.close();

    notifyResultsUpdated(activity);
}

void Rankings::notifyResultsUpdated(const QString & activity, QStringList clients)
{
    kDebug() << "####### These are the results for: " << activity << "current activity is" <<
            NepomukPlugin::self()->sharedInfo()->currentActivity();

    QVariantList data;
    foreach (const ResultItem & item, m_results[COALESCE_ACTIVITY(activity)]) {
        kDebug() << item.uri << item.score;
        data << item.uri.toString();
    }

    kDebug() << m_clients << clients;

    if (clients.isEmpty()) {
        clients = m_clients[COALESCE_ACTIVITY(activity)];

        if (activity == NepomukPlugin::self()->sharedInfo()->currentActivity()) {
            kDebug() << "This is the current activity, notifying all";
            clients.append(m_clients[QString()]);
        }
    }

    kDebug() << "Notify clients" << clients << data;

    foreach (const QString & client, clients) {
        QDBusInterface rankingsservice(client, "/RankingsClient", "org.kde.ActivityManager.RankingsClient");
        rankingsservice.call("updated", data);
    }
}
