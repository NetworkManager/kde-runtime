/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include <QtCore/QString>
#include <QtCore/QMap>

#include <Soprano/Util/SignalCacheModel>


namespace Soprano {
    class Model;
    class Backend;
}

class KJob;

namespace Nepomuk {

    class ModelCopyJob;

    class Repository : public Soprano::Util::SignalCacheModel
    {
        Q_OBJECT

    public:
        Repository( const QString& name );
        ~Repository();

        QString name() const { return m_name; }

        enum State {
            CLOSED,
            OPENING,
            OPEN
        };

        State state() const;

        static const Soprano::Backend* activeSopranoBackend();

    public Q_SLOTS:
        /**
         * Will emit the opened signal
         */
        void open();

        void close();

    Q_SIGNALS:
        void opened( Repository*, bool success );

    private Q_SLOTS:
        void copyFinished( KJob* job );

    private:
        QString m_name;
        State m_state;

        // the base path for the data. Will contain subfolder:
        // "data" for the data
        QString m_basePath;

        const Soprano::Backend* m_oldStorageBackend;
        QString m_oldStoragePath;

        Soprano::Model* m_model;

        // can only be non-null in the opening state
        ModelCopyJob* m_modelCopyJob;
    };

    typedef QMap<QString, Repository*> RepositoryMap;
}

#endif
