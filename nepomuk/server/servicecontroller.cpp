/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "servicecontroller.h"
#include "processcontrol.h"
#include "servicecontrolinterface.h"
#include "nepomukserver.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include <KStandardDirs>
#include <KConfigGroup>
#include <KDebug>


namespace {
    QString dbusServiceName( const QString& serviceName ) {
        return QString("org.kde.nepomuk.services.%1").arg(serviceName);
    }
}


class Nepomuk::ServiceController::Private
{
public:
    Private()
        : processControl( 0 ),
          serviceControlInterface( 0 ),
          attached(false),
          initialized( false ) {
    }

    KService::Ptr service;
    bool autostart;
    bool startOnDemand;
    bool runOnce;

    ProcessControl* processControl;
    OrgKdeNepomukServiceControlInterface* serviceControlInterface;

    // true if we attached to an already running instance instead of
    // starting our own (in that case processControl will be 0)
    bool attached;

    bool initialized;

    // list of loops waiting for the service to become initialized
    QList<QEventLoop*> loops;

    void init( KService::Ptr service );
};


void Nepomuk::ServiceController::Private::init( KService::Ptr s )
{
    service = s;
    autostart = service->property( "X-KDE-Nepomuk-autostart", QVariant::Bool ).toBool();
    KConfigGroup cg( Server::self()->config(), QString("Service-%1").arg(service->desktopEntryName()) );
    autostart = cg.readEntry( "autostart", autostart );

    QVariant p = service->property( "X-KDE-Nepomuk-start-on-demand", QVariant::Bool );
    startOnDemand = ( p.isValid() ? p.toBool() : false );

    p = service->property( "X-KDE-Nepomuk-run-once", QVariant::Bool );
    runOnce = ( p.isValid() ? p.toBool() : false );

    initialized = false;
}


Nepomuk::ServiceController::ServiceController( KService::Ptr service, QObject* parent )
    : QObject( parent ),
      d(new Private())
{
    d->init( service );
}


Nepomuk::ServiceController::~ServiceController()
{
    stop();
    delete d;
}


KService::Ptr Nepomuk::ServiceController::service() const
{
    return d->service;
}


QString Nepomuk::ServiceController::name() const
{
    return d->service->desktopEntryName();
}


QStringList Nepomuk::ServiceController::dependencies() const
{
    QStringList deps = d->service->property( "X-KDE-Nepomuk-dependencies", QVariant::StringList ).toStringList();
    if ( deps.isEmpty() ) {
        deps.append( "nepomukstorage" );
    }
    deps.removeAll( name() );
    return deps;
}


void Nepomuk::ServiceController::setAutostart( bool enable )
{
    KConfigGroup cg( Server::self()->config(), QString("Service-%1").arg(name()) );
    cg.writeEntry( "autostart", enable );
}


bool Nepomuk::ServiceController::autostart() const
{
    return d->autostart;
}


bool Nepomuk::ServiceController::startOnDemand() const
{
    return d->startOnDemand;
}


bool Nepomuk::ServiceController::runOnce() const
{
    return d->runOnce;
}


bool Nepomuk::ServiceController::start()
{
    if( isRunning() ) {
        return true;
    }

    d->initialized = false;

    // check if the service is already running, ie. has been started by someone else or by a crashed instance of the server
    // we cannot rely on the auto-restart feature of ProcessControl here. So we handle that completely in slotServiceOwnerChanged
    if( QDBusConnection::sessionBus().interface()->isServiceRegistered( dbusServiceName( name() ) ) ) {
        kDebug() << "Attaching to already running service" << name();
        d->attached = true;
        createServiceControlInterface();
        return true;
    }
    else {
        kDebug(300002) << "Starting" << name();

        if( !d->processControl ) {
            d->processControl = new ProcessControl( this );
            connect( d->processControl, SIGNAL( finished( bool ) ),
                     this, SLOT( slotProcessFinished( bool ) ) );
        }

        connect( QDBusConnection::sessionBus().interface(),
                 SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
                 this,
                 SLOT( slotServiceOwnerChanged( const QString&, const QString&, const QString& ) ) );

        return d->processControl->start( KGlobal::dirs()->locate( "exe", "nepomukservicestub" ),
                                         QStringList() << name() );
    }
}


void Nepomuk::ServiceController::stop()
{
    if( isRunning() ) {
        kDebug(300002) << "Stopping" << name();

        d->attached = false;

        if( d->processControl ) {
            d->processControl->setCrashPolicy( ProcessControl::StopOnCrash );
        }
        d->serviceControlInterface->shutdown();

        if( d->processControl ) {
            d->processControl->stop();
            d->processControl->setCrashPolicy( ProcessControl::RestartOnCrash );
        }
    }
}


bool Nepomuk::ServiceController::isRunning() const
{
    return( d->attached || ( d->processControl ? d->processControl->isRunning() : false ) );
}


bool Nepomuk::ServiceController::isInitialized() const
{
    return d->initialized;
}


bool Nepomuk::ServiceController::waitForInitialized( int timeout )
{
    if( !isRunning() ) {
        return false;
    }

    if( !d->initialized ) {
        QEventLoop loop;
        d->loops.append( &loop );
        if ( timeout > 0 ) {
            QTimer::singleShot( timeout, &loop, SLOT(quit()) );
        }
        loop.exec();
        d->loops.removeAll( &loop );
    }

    return d->initialized;
}


void Nepomuk::ServiceController::slotProcessFinished( bool /*clean*/ )
{
    kDebug() << "Service" << name() << "went down";
    d->initialized = false;
    d->attached = false;
    disconnect( QDBusConnection::sessionBus().interface() );
    delete d->serviceControlInterface;
    d->serviceControlInterface = 0;
    foreach( QEventLoop* loop, d->loops ) {
        loop->exit();
    }
}


void Nepomuk::ServiceController::slotServiceOwnerChanged( const QString& serviceName,
                                                          const QString& oldOwner,
                                                          const QString& newOwner )
{
    if( !newOwner.isEmpty() && serviceName == dbusServiceName( name() ) ) {
        createServiceControlInterface();
    }

    // an attached service was not started through ProcessControl. Thus, we cannot rely
    // on its restart-on-crash feature and have to do it manually. Afterwards it is back
    // to normals
    else if( d->attached &&
             oldOwner == dbusServiceName( name() ) ) {
        kDebug() << "Attached service" << name() << "went down. Restarting ourselves.";
        d->attached = false;
        start();
    }
}


void Nepomuk::ServiceController::createServiceControlInterface()
{
    delete d->serviceControlInterface;

    d->serviceControlInterface = new OrgKdeNepomukServiceControlInterface( dbusServiceName( name() ),
                                                                           "/servicecontrol",
                                                                           QDBusConnection::sessionBus(),
                                                                           this );
    connect( d->serviceControlInterface, SIGNAL( serviceInitialized( bool ) ),
             this, SLOT( slotServiceInitialized( bool ) ) );

    if ( d->serviceControlInterface->isInitialized() ) {
        slotServiceInitialized( true );
    }
}


void Nepomuk::ServiceController::slotServiceInitialized( bool success )
{
    if ( !d->initialized ) {
        kDebug() << "Service" << name() << "initialized:" << success;
        d->initialized = true;
        emit serviceInitialized( this );

        if ( runOnce() ) {
            // we have been run once. Do not autostart next time
            KConfigGroup cg( Server::self()->config(), QString("Service-%1").arg(name()) );
            cg.writeEntry( "autostart", false );
        }
    }

    foreach( QEventLoop* loop, d->loops ) {
        loop->exit();
    }
}
