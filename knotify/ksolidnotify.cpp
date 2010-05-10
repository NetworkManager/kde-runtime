/*
   Copyright (C) 2010 by Jacopo De Simoi <wilderkde@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#include "ksolidnotify.h"

//solid specific includes
#include <Solid/DeviceNotifier>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>
#include <Solid/OpticalDisc>
#include <Solid/Predicate>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#include <KLocale>

static const char dbusServiceName[] = "org.kde.DeviceNotifications";
static const char dbusInterfaceName[] = "org.kde.DeviceNotifications";
static const char dbusPath[] = "/org/kde/DeviceNotifications";


KSolidNotify::KSolidNotify(KNotify* parent):
	QObject(parent),
	m_kNotify(parent),
	m_dbusServiceExists(false)
{
	Solid::Predicate p(Solid::DeviceInterface::StorageAccess);
	p |= Solid::Predicate(Solid::DeviceInterface::OpticalDrive);
	QList<Solid::Device> devices = Solid::Device::listFromQuery(p);
	foreach (const Solid::Device &dev, devices)
	{
		m_devices.insert(dev.udi(), dev);
		connectSignals(&m_devices[dev.udi()]);
	}

	connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)),
			this, SLOT(onDeviceAdded(const QString &)));
	connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)),
			this, SLOT(onDeviceRemoved(const QString &)));

	// check if service already exists on plugin instantiation
	QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
	m_dbusServiceExists = interface && interface->isServiceRegistered(dbusServiceName);

	if( m_dbusServiceExists )
		slotServiceOwnerChanged(dbusServiceName, QString(), "_"); //connect signals

	// to catch register/unregister events from service in runtime
	QDBusServiceWatcher *watcher = new QDBusServiceWatcher(this);
	watcher->setConnection(QDBusConnection::sessionBus());
	watcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
	watcher->addWatchedService(dbusServiceName);
	connect(watcher, SIGNAL(serviceOwnerChanged(const QString&, const QString&, const QString&)),
			SLOT(slotServiceOwnerChanged(const QString&, const QString&, const QString&)));
}

void KSolidNotify::onDeviceAdded(const QString &udi)
{
	Solid::Device device(udi);
	m_devices.insert(udi, device);
	connectSignals(&m_devices[udi]);
}

void KSolidNotify::onDeviceRemoved(const QString &udi)
{
	if (m_devices[udi].is<Solid::StorageVolume>())
	{
		Solid::StorageAccess *access = m_devices[udi].as<Solid::StorageAccess>();
		if (access)
		{
			disconnect(access, 0, this, 0);
		}
	}
	m_devices.remove(udi);
}

void KSolidNotify::connectSignals(Solid::Device* device)
{
	if (device->is<Solid::StorageVolume>())
	{
		Solid::StorageAccess *access = device->as<Solid::StorageAccess>();
		if (access )
		{
			connect(access, SIGNAL(teardownDone(Solid::ErrorType, QVariant, const QString &)),
					this, SLOT(storageTeardownDone(Solid::ErrorType, QVariant , const QString &)));
			connect(access, SIGNAL(setupDone(Solid::ErrorType, QVariant, const QString &)),
					this, SLOT(storageSetupDone(Solid::ErrorType, QVariant , const QString &)));
		}
	}
	if (device->is<Solid::OpticalDisc>())
	{
		Solid::OpticalDrive *drive = device->as<Solid::OpticalDrive>();
		connect(drive, SIGNAL(ejectDone(Solid::ErrorType, QVariant, const QString &)),
				this, SLOT(storageEjectDone(Solid::ErrorType, QVariant , const QString &)));
	}
}

void KSolidNotify::storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
	if (error)
	{
		Solid::Device device(udi);
		QString errorMessage = i18n("Could not mount the following device: %1", device.description());
		if (!m_dbusServiceExists)
		{
			m_kNotify->event("mounterror", "hardwarenotifications", ContextList(), i18n("Device error"), errorMessage, KNotifyImage(), QStringList(), -1);
		} else
		{
			QDBusMessage m = QDBusMessage::createMethodCall( dbusServiceName, dbusPath, dbusInterfaceName, "notify" );
			m << error << errorMessage << errorData.toString().simplified() << udi;
			QDBusConnection::sessionBus().call(m);
		}
	}
}

void KSolidNotify::storageTeardownDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
	if (error)
	{
		Solid::Device device(udi);
		QString errorMessage = i18n("Could not unmount the following device: %1\nOne or more files on this device are open within an application ", device.description());
		if (!m_dbusServiceExists)
		{
			m_kNotify->event("mounterror", "hardwarenotifications", ContextList(), i18n("Device error"), errorMessage, KNotifyImage(), QStringList(), -1);
		} else
		{
			QDBusMessage m = QDBusMessage::createMethodCall( dbusServiceName, dbusPath, dbusInterfaceName, "notify" );
			m << error << errorMessage << errorData.toString().simplified() << udi;
			QDBusConnection::sessionBus().call(m);
		}
	}
}

void KSolidNotify::storageEjectDone(Solid::ErrorType error, QVariant errorData, const QString &udi)
{
	if (error)
	{
		Solid::Device device(udi);
		QString errorMessage = i18n("Could not eject the following device: %1\nOne or more files on this device are open within an application ", device.description());
		if (!m_dbusServiceExists)
		{
			m_kNotify->event("mounterror", "hardwarenotifications", ContextList(), i18n("Device error"), errorMessage, KNotifyImage(), QStringList(), -1);
		} else
		{
			QDBusMessage m = QDBusMessage::createMethodCall( dbusServiceName, dbusPath, dbusInterfaceName, "notify" );
			m << error << errorMessage << errorData.toString().simplified() << udi;
			QDBusConnection::sessionBus().call(m);
		}
	}
}

void KSolidNotify::slotServiceOwnerChanged( const QString & serviceName, const QString & oldOwner, const QString & newOwner )
{
	if (newOwner.isEmpty())
		m_dbusServiceExists = false;
	else if (oldOwner.isEmpty())
		m_dbusServiceExists = true;
}


#include "ksolidnotify.moc"
