/*
   This file is part of the Nepomuk KDE project.
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

#include "timelinetools.h"

#include <Soprano/Node>
#include <Soprano/Vocabulary/XMLSchema>

#include <Nepomuk2/Vocabulary/NFO>
#include <Nepomuk2/Vocabulary/NIE>
#include <Nepomuk2/Vocabulary/NUAO>

#include <Nepomuk2/Query/FileQuery>
#include <Nepomuk2/Query/AndTerm>
#include <Nepomuk2/Query/OrTerm>
#include <Nepomuk2/Query/ComparisonTerm>
#include <Nepomuk2/Query/LiteralTerm>
#include <Nepomuk2/Query/StandardQuery>

#include <KUrl>
#include <KCalendarSystem>
#include <KGlobal>
#include <KLocale>
#include <KDebug>

#include <QtCore/QDate>
#include <QtCore/QRegExp>


namespace {
    QDate applyRelativeDateModificators( const QDate& date, const QMap<QString, QString>& modificators )
    {
        QDate newDate( date );
        const QString dayKey = QLatin1String("relDays");
        const QString weekKey = QLatin1String("relWeeks");
        const QString monthKey = QLatin1String("relMonths");
        const QString yearKey = QLatin1String("relYears");
        bool ok = false;

        if (modificators.contains(yearKey)) {
            int relYears = modificators[yearKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addYears(relYears);
            }
        }
        if (modificators.contains(monthKey)) {
            int relMonths = modificators[monthKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addMonths(relMonths);
            }
        }
        if (modificators.contains(weekKey)) {
            int relWeeks = modificators[weekKey].toInt(&ok);
            if (ok) {
                const KCalendarSystem* calSystem = KGlobal::locale()->calendar();
                newDate = newDate.addDays(relWeeks * calSystem->daysInWeek(date));
            }
        }
        if (modificators.contains(dayKey)) {
            int relDays = modificators[dayKey].toInt(&ok);
            if (ok) {
                newDate = newDate.addDays(relDays);
            }
        }
        return newDate;
    }
}


Nepomuk2::TimelineFolderType Nepomuk2::parseTimelineUrl( const KUrl& url, QDate* date, QString* filename )
{
    kDebug() << url;

    static QRegExp s_dateRegexp( QLatin1String("\\d{4}-\\d{2}(?:-(\\d{2}))?") );

    // reset
    *date = QDate();

    const QString path = url.path(KUrl::RemoveTrailingSlash);

    if( path.isEmpty() || path == QLatin1String("/") ) {
        kDebug() << url << "is root folder";
        return RootFolder;
    }
    else if( path.startsWith( QLatin1String( "/today" ) ) ) {
        *date = QDate::currentDate();
        if ( filename )
            *filename = path.mid( 7 );
        kDebug() << url << "is today folder:" << *date;
        return DayFolder;
    }
    else if( path == QLatin1String( "/calendar" ) ) {
        kDebug() << url << "is calendar folder";
        return CalendarFolder;
    }
    else {
        QStringList sections = path.split( QLatin1String( "/" ), QString::SkipEmptyParts );
        QString dateString;
        if ( s_dateRegexp.exactMatch( sections.last() ) ) {
            dateString = sections.last();
        }
        else if ( sections.count() > 1 && s_dateRegexp.exactMatch( sections[sections.count()-2] ) ) {
            dateString = sections[sections.count()-2];
            if ( filename )
                *filename = sections.last();
        }
        else {
            kDebug() << url << "COULD NOT PARSE";
            return NoFolder;
        }

        if ( s_dateRegexp.cap( 1 ).isEmpty() ) {
            // no day -> month listing
            kDebug() << "parsing " << dateString;
            *date = QDate::fromString( dateString, QLatin1String("yyyy-MM") );
            kDebug() << url << "is month folder:" << date->month() << date->year();
            if ( date->month() > 0 && date->year() > 0 )
                return MonthFolder;
        }
        else {
            kDebug() << "parsing " << dateString;
            *date = applyRelativeDateModificators( QDate::fromString( dateString, "yyyy-MM-dd" ), url.queryItems() );
            // only in day folders we can have filenames
            kDebug() << url << "is day folder:" << *date;
            if ( date->isValid() )
                return DayFolder;
        }
    }

    return NoFolder;
}


Nepomuk2::Query::Query Nepomuk2::buildTimelineQuery( const QDate& from, const QDate& to )
{
    Nepomuk2::Query::FileQuery query = Nepomuk2::Query::dateRangeQuery( from, to.isValid() ? to : from, Query::ModificationDate );
    query.setFileMode( Query::FileQuery::QueryFiles );
    return query;
}
