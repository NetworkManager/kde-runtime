/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcmkded.h"

#include <Qt3Support/Q3Header>

#include <QByteArray>
#include <QtDBus/QtDBus>
#include <QGroupBox>
#include <QHeaderView>
#include <QLayout>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdialogbuttonbox.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdialog.h>
#include <k3listview.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kstandarddirs.h>

#include <KPluginFactory>
#include <KPluginLoader>
#include "kcmkded.moc"

K_PLUGIN_FACTORY(KDEDFactory,
        registerPlugin<KDEDConfig>();
        )
K_EXPORT_PLUGIN(KDEDFactory("kcmkded"))

static const int LibraryRole = Qt::UserRole + 1;

KDEDConfig::KDEDConfig(QWidget* parent, const QVariantList &) :
	KCModule( KDEDFactory::componentData(), parent )
{
	KAboutData *about =
		new KAboutData( I18N_NOOP( "kcmkded" ), 0, ki18n( "KDE Service Manager" ),
				0, KLocalizedString(), KAboutData::License_GPL,
				ki18n( "(c) 2002 Daniel Molkentin" ) );
	about->addAuthor(ki18n("Daniel Molkentin"),KLocalizedString(),"molkentin@kde.org");
	setAboutData( about );

	setQuickHelp( i18n("<h1>Service Manager</h1><p>This module allows you to have an overview of all plugins of the "
			"KDE Daemon, also referred to as KDE Services. Generally, there are two types of service:</p>"
			"<ul><li>Services invoked at startup</li><li>Services called on demand</li></ul>"
			"<p>The latter are only listed for convenience. The startup services can be started and stopped. "
			"In Administrator mode, you can also define whether services should be loaded at startup.</p>"
			"<p><b> Use this with care: some services are vital for KDE; do not deactivate services if you"
			" do not know what you are doing.</b></p>"));

	RUNNING = i18n("Running")+' ';
	NOT_RUNNING = i18n("Not running")+' ';

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( KDialog::spacingHint() );

	QGroupBox *gb = new QGroupBox( i18n("Load-on-Demand Services"), this );
	gb->setWhatsThis( i18n("This is a list of available KDE services which will "
			"be started on demand. They are only listed for convenience, as you "
			"cannot manipulate these services."));
	lay->addWidget( gb );

	QVBoxLayout *gblay = new QVBoxLayout( gb );

	_lvLoD = new QTreeWidget( gb );
	QStringList cols;
	cols.append( i18n("Service") );
	cols.append( i18n("Description") );
	cols.append( i18n("Status") );
	_lvLoD->setHeaderLabels( cols );
	_lvLoD->setAllColumnsShowFocus(true);
	_lvLoD->setRootIsDecorated( false );
	//_lvLoD->header()->setStretchEnabled(true, 1);
	gblay->addWidget( _lvLoD );

 	gb = new QGroupBox( i18n("Startup Services"), this );
	gb->setWhatsThis( i18n("This shows all KDE services that can be loaded "
				"on KDE startup. Checked services will be invoked on next startup. "
				"Be careful with deactivation of unknown services."));
	lay->addWidget( gb );

	gblay = new QVBoxLayout( gb );

	_lvStartup = new K3ListView( gb );
	_lvStartup->addColumn(i18n("Use"));
	_lvStartup->addColumn(i18n("Service"));
	_lvStartup->addColumn(i18n("Description"));
	_lvStartup->addColumn(i18n("Status"));
	_lvStartup->setAllColumnsShowFocus(true);
	_lvStartup->header()->setStretchEnabled(true, 2);
	gblay->addWidget( _lvStartup );

	KDialogButtonBox *buttonBox = new KDialogButtonBox( gb, Qt::Horizontal);
	_pbStart = buttonBox->addButton( i18n("Start") , QDialogButtonBox::ActionRole  );
	_pbStop = buttonBox->addButton( i18n("Stop") , QDialogButtonBox::ActionRole );
	gblay->addWidget( buttonBox );

	_pbStart->setEnabled( false );
	_pbStop->setEnabled( false );

	connect(_pbStart, SIGNAL(clicked()), SLOT(slotStartService()));
	connect(_pbStop, SIGNAL(clicked()), SLOT(slotStopService()));
	connect(_lvStartup, SIGNAL(selectionChanged(Q3ListViewItem*)), SLOT(slotEvalItem(Q3ListViewItem*)) );

	load();
}

QString setModuleGroup(const QString &filename)
{
	QString module = filename;
	int i = module.lastIndexOf('/');
	if (i != -1)
	   module = module.mid(i+1);
	i = module.lastIndexOf('.');
	if (i != -1)
	   module = module.left(i);

	return QString("Module-%1").arg(module);
}

bool KDEDConfig::autoloadEnabled(KConfig *config, const QString &filename)
{
	KConfigGroup cg(config, setModuleGroup(filename));
	return cg.readEntry("autoload", true);
}

void KDEDConfig::setAutoloadEnabled(KConfig *config, const QString &filename, bool b)
{
	KConfigGroup cg(config, setModuleGroup(filename));
	return cg.writeEntry("autoload", b);
}

void KDEDConfig::load() {
	KConfig kdedrc( "kdedrc", KConfig::CascadeConfig );

	_lvStartup->clear();
	_lvLoD->clear();

	QStringList files;
	KGlobal::dirs()->findAllResources( "services",
			QLatin1String( "kded/*.desktop" ),
			KStandardDirs::Recursive | KStandardDirs::NoDuplicates,
			files );

	Q3ListViewItem* item = 0L;
	QTreeWidgetItem* treeitem = 0L;
	CheckListItem* clitem;
	for ( QStringList::ConstIterator it = files.begin(); it != files.end(); ++it ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {
			KDesktopFile file( "services", *it );

			if ( file.desktopGroup().readEntry("X-KDE-Kded-autoload", false) ) {
				clitem = new CheckListItem(_lvStartup, QString());
				connect(clitem, SIGNAL(changed(Q3CheckListItem*)), SLOT(slotItemChecked(Q3CheckListItem*)));
				clitem->setOn(autoloadEnabled(&kdedrc, *it));
				item = clitem;
				item->setText(1, file.readName());
				item->setText(2, file.readComment());
				item->setText(3, NOT_RUNNING);
				item->setText(4, file.desktopGroup().readEntry("X-KDE-Library"));
			}
			else if ( file.desktopGroup().readEntry("X-KDE-Kded-load-on-demand", false) ) {
				treeitem = new QTreeWidgetItem();
				treeitem->setText( 0, file.readName() );
				treeitem->setText( 1, file.readComment() );
				treeitem->setText( 2, NOT_RUNNING );
				treeitem->setData( 0, LibraryRole, file.desktopGroup().readEntry( "X-KDE-Library" ) );
				_lvLoD->addTopLevelItem( treeitem );
			}
		}
	}
	_lvLoD->resizeColumnToContents( 0 );

	getServiceStatus();
}

void KDEDConfig::save() {
	Q3CheckListItem* item = 0L;

	QStringList files;
	KGlobal::dirs()->findAllResources( "services",
			QLatin1String( "kded/*.desktop" ),
			KStandardDirs::Recursive | KStandardDirs::NoDuplicates,
			files );

	KConfig kdedrc("kdedrc", KConfig::CascadeConfig);

	for ( QStringList::ConstIterator it = files.begin(); it != files.end(); ++it ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {

                        KConfig _file( *it, KConfig::CascadeConfig, "services"  );
                        KConfigGroup file(&_file, "Desktop Entry");

			if (file.readEntry("X-KDE-Kded-autoload", false)){

				item = static_cast<Q3CheckListItem *>(_lvStartup->findItem(file.readEntry("X-KDE-Library"),4));
				if (item) {
					// we found a match, now compare and see what changed
					setAutoloadEnabled(&kdedrc, *it, item->isOn());
				}
			}
		}
	}
	kdedrc.sync();

	QDBusInterface kdedInterface( "org.kde.kded", "/kded", "org.kde.kded" );
	kdedInterface.call( "reconfigure" );
	QTimer::singleShot(0, this, SLOT(slotServiceRunningToggled()));
}


void KDEDConfig::defaults()
{
	Q3ListViewItemIterator it( _lvStartup);
	while ( it.current() != 0 ) {
		if (it.current()->rtti()==1) {
			Q3CheckListItem *item = static_cast<Q3CheckListItem *>(it.current());
			item->setOn(false);
		}
		++it;
	}

	getServiceStatus();
}


void KDEDConfig::getServiceStatus()
{
	QStringList modules;
	QDBusInterface kdedInterface( "org.kde.kded", "/kded", "org.kde.kded" );
	QDBusReply<QStringList> reply = kdedInterface.call( "loadedModules"  );

	if ( reply.isValid() ) {
		modules = reply.value();
	}
	else {
		_lvLoD->setEnabled( false );
		_lvStartup->setEnabled( false );
		KMessageBox::error(this, i18n("Unable to contact KDED."));
		return;
	}

	int count = _lvLoD->topLevelItemCount();
	for( int i = 0; i < count; ++i )
                _lvLoD->topLevelItem( i )->setText( 2, NOT_RUNNING );
	for( Q3ListViewItemIterator it( _lvStartup); it.current() != 0; ++it )
                it.current()->setText(3, NOT_RUNNING);
	foreach( const QString& module, modules )
	{
		count = _lvLoD->topLevelItemCount();
		for( int i = 0; i < count; ++i )
		{
			QTreeWidgetItem *treeitem = _lvLoD->topLevelItem( i );
                	if ( treeitem->data( 0, LibraryRole ).toString() == module )
			{
				treeitem->setText( 2, RUNNING );
				break;
			}
		}

		Q3ListViewItem *item = _lvStartup->findItem(module, 4);
		if ( item )
		{
			item->setText(3, RUNNING);
		}
	}
}

void KDEDConfig::slotReload()
{
	QString current = _lvStartup->currentItem()->text(4);
	load();
	Q3ListViewItem *item = _lvStartup->findItem(current, 4);
	if (item)
		_lvStartup->setCurrentItem(item);
}

void KDEDConfig::slotEvalItem(Q3ListViewItem * item)
{
	if (!item)
		return;

	if ( item->text(3) == RUNNING ) {
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( true );
	}
	else if ( item->text(3) == NOT_RUNNING ) {
		_pbStart->setEnabled( true );
		_pbStop->setEnabled( false );
	}
	else // Error handling, better do nothing
	{
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( false );
	}

	getServiceStatus();
}

void KDEDConfig::slotServiceRunningToggled()
{
	getServiceStatus();
	slotEvalItem(_lvStartup->currentItem());
}

void KDEDConfig::slotStartService()
{
	QString service = _lvStartup->currentItem()->text(4);

	QDBusInterface kdedInterface( "org.kde.kded", "/kded","org.kde.kded" );
	QDBusReply<bool> reply = kdedInterface.call( "loadModule", service  );

	if ( reply.isValid() ) {
		if ( reply.value() )
			slotServiceRunningToggled();
		else
			KMessageBox::error(this, "<qt>" + i18n("Unable to start server <em>service</em>.") + "</qt>");
	}
	else {
		KMessageBox::error(this, "<qt>" + i18n("Unable to start service <em>service</em>.<br /><br /><i>Error: %1</i>",
											reply.error().message()) + "</qt>" );
	}
}

void KDEDConfig::slotStopService()
{
	QString service = _lvStartup->currentItem()->text(4);
	kDebug() << "Stopping: " << service;

	QDBusInterface kdedInterface( "org.kde.kded", "/kded", "org.kde.kded" );
	QDBusReply<bool> reply = kdedInterface.call( "unloadModule", service  );

	if ( reply.isValid() ) {
		if ( reply.value() )
			slotServiceRunningToggled();
		else
			KMessageBox::error(this, "<qt>" + i18n("Unable to stop server <em>service</em>.") + "</qt>");
	}
	else {
		KMessageBox::error(this, "<qt>" + i18n("Unable to stop service <em>service</em>.<br /><br /><i>Error: %1</i>",
											reply.error().message()) + "</qt>" );
	}
}

void KDEDConfig::slotItemChecked(Q3CheckListItem*)
{
	emit changed(true);
}

CheckListItem::CheckListItem(Q3ListView *parent, const QString &text)
	: QObject(parent),
	  Q3CheckListItem(parent, text, CheckBox)
{ }

void CheckListItem::stateChange(bool on)
{
	Q3CheckListItem::stateChange(on);
	emit changed(this);
}
