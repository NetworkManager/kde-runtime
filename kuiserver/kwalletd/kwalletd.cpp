/*
   This file is part of the KDE libraries

   Copyright (c) 2002 George Staikos <staikos@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

*/

#include "kwalletd.h"

#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kwalletentry.h>
#include <kpassdlg.h>

#include <qdir.h>

#include <assert.h>


extern "C" {
   KDEDModule *create_kwalletd(const QCString &name) {
	   return new KWalletD(name);
   }

   void *__kde_do_unload;
}


KWalletD::KWalletD(const QCString &name)
: KDEDModule(name) {
	srand(time(0));
	KGlobal::dirs()->addResourceType("kwallet", "share/apps/kwallet");
	KApplication::dcopClient()->setNotifications(true);
	connect(KApplication::dcopClient(),
		SIGNAL(applicationRemoved(const QCString&)),
		this,
		SLOT(slotAppUnregistered(const QCString&)));
}
  

KWalletD::~KWalletD() {
	// Open wallets get closed without being saved of course.
	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		emitDCOPSignal("walletClosed(int)", it.currentKey());
		delete it.current();
		// FIXME: removeme later
		_wallets.replace(it.currentKey(), 0L);
	}
	_wallets.clear();
}


int KWalletD::generateHandle() {
int rc;

	do {
		rc = 9999999 * rand()/(RAND_MAX+1);
	} while(_wallets.find(rc));

return rc;
}


int KWalletD::open(const QString& wallet) {
DCOPClient *dc = callingDcopClient();
int rc = -1;

	if (dc) {
		for (QIntDictIterator<KWallet::Backend> i(_wallets); i.current(); ++i) {
			if (i.current()->walletName() == wallet) {
				rc = i.currentKey();
				break;
			}
		}

		if (rc == -1) {
			if (_wallets.count() > 20) {
				kdDebug() << "Too many wallets open." << endl;
				return -1;
			}

			KWallet::Backend *b;
			KPasswordDialog *kpd;
			if (KWallet::Backend::exists(wallet)) {
				b = new KWallet::Backend(wallet);
				kpd = new KPasswordDialog(KPasswordDialog::Password, i18n("The application '%1' has requested to open the wallet '%2'.  Please enter the password for this wallet below if you wish to open it, or cancel to deny access.").arg(dc->senderId()).arg(wallet), false);
			} else {
				b = new KWallet::Backend(wallet);
				kpd = new KPasswordDialog(KPasswordDialog::NewPassword, i18n("The application '%1' has requested to create a new wallet named '%2'.  Please choose a password for this wallet, or cancel to deny the application's request.").arg(dc->senderId()).arg(wallet), false);
			}
			kpd->setCaption(i18n("KDE Wallet Service"));
			if (kpd->exec() == KDialog::Accepted) {
				const char *p = kpd->password();
				int rc = b->open(QByteArray().duplicate(p, strlen(p)+1));
			}
			delete kpd;
			if (!b->isOpen()) {
				delete b;
				return -1;
			}
			_wallets.insert(rc = generateHandle(), b);
			_handles[dc->senderId()].append(rc);
			b->ref();
		} else if (!_handles[dc->senderId()].contains(rc)) {
			_handles[dc->senderId()].append(rc);
		 	_wallets.find(rc)->ref();
		}
	}

return rc;
}


int KWalletD::close(const QString& wallet, bool force) {
int handle = -1;
KWallet::Backend *w = 0L;

	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			handle = it.currentKey();
			w = it.current();
			break;
		}
	}

	if (w) {
		// can refCount() ever be 0?
		if (w->refCount() == 0 || force) {
			invalidateHandle(handle);
			_wallets.remove(handle);
			delete w;
			return 0;
		}
		return 1;
	}

return -1;
}


int KWalletD::close(int handle, bool force) {
DCOPClient *dc = callingDcopClient();
KWallet::Backend *w = _wallets.find(handle);

	if (dc && w) { // the handle is valid and we have a client
		if (_handles.contains(dc->senderId())) { // we know this app
			if (_handles[dc->senderId()].contains(handle)) {
				// the app owns this handle
				_handles[dc->senderId()].remove(handle);
			}
		}

		// watch the side effect of the deref()
		if (w->deref() == 0 || force) {
			_wallets.remove(handle);
			if (force) {
				invalidateHandle(handle);
			}
			delete w;
			return 0;
		}
		return 1; // not closed
	}

return -1; // not open to begin with, or other error
}


bool KWalletD::isOpen(const QString& wallet) const {
	for (QIntDictIterator<KWallet::Backend> it(_wallets);
						it.current();
							++it) {
		if (it.current()->walletName() == wallet) {
			return true;
		}
	}
return false;
}


bool KWalletD::isOpen(int handle) const {
return _wallets.find(handle) != 0;
}


QStringList KWalletD::wallets() const {
QString path = KGlobal::dirs()->saveLocation("kwallet");
QDir dir(path, "*.kwl");
QStringList rc;

	dir.setFilter(QDir::Files | QDir::NoSymLinks);

	const QFileInfoList *list = dir.entryInfoList();
	QFileInfoListIterator it(*list);
	QFileInfo *fi;
	while ((fi = it.current()) != 0L) {
		rc += fi->fileName();
		++it;
	}
return rc;
}


QStringList KWalletD::folderList(int handle) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		return b->folderList();
	}

return QStringList();
}


bool KWalletD::hasFolder(int handle, const QString& f) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		return b->hasFolder(f);
	}

return false;
}


bool KWalletD::removeFolder(int handle, const QString& f) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		return b->removeFolder(f);
	}

return false;
}


QByteArray KWalletD::readEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (b->hasFolder(folder)) {
			b->setFolder(folder);
			KWallet::Entry *e = b->readEntry(key);
			if (e && e->type() == KWallet::Entry::Stream) {
				return e->value();
			}
		}
	}

return QByteArray();
}


QString KWalletD::readPassword(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (b->hasFolder(folder)) {
			b->setFolder(folder);
			KWallet::Entry *e = b->readEntry(key);
			if (e && e->type() == KWallet::Entry::Password) {
				return e->password();
			}
		}
	}

return QString::null;
}


int KWalletD::writeEntry(int handle, const QString& folder, const QString& key, const QByteArray& value) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return -2;
		}
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Entry::Stream);
		b->writeEntry(&e);
		return 0;
	}

return -1;
}


int KWalletD::writePassword(int handle, const QString& folder, const QString& key, const QString& value) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return -2;
		}
		b->setFolder(folder);
		KWallet::Entry e;
		e.setKey(key);
		e.setValue(value);
		e.setType(KWallet::Entry::Password);
		b->writeEntry(&e);
		return 0;
	}

return -1;
}


bool KWalletD::hasEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return false;
		}
		b->setFolder(folder);
		return b->hasEntry(key);
	}

return false;
}


int KWalletD::removeEntry(int handle, const QString& folder, const QString& key) {
KWallet::Backend *b;

	if ((b = getWallet(handle))) {
		if (!b->hasFolder(folder)) {
			return -2;
		}
		b->setFolder(folder);
		return b->removeEntry(key) ? 0 : -3;
	}

return -1;
}


void KWalletD::slotAppUnregistered(const QCString& app) {
	if (_handles.contains(app)) {
		QValueList<int> *l = &_handles[app];
		for (QValueList<int>::Iterator i = l->begin(); i != l->end(); i++) {
			close(*i, false);
		}
		_handles.remove(app);
	}
}


void KWalletD::invalidateHandle(int handle) {
	for (QMap<QCString,QValueList<int> >::Iterator i = _handles.begin();
							i != _handles.end();
									++i) {
		i.data().remove(handle);
	}
}


KWallet::Backend *KWalletD::getWallet(int handle) {
DCOPClient *dc = callingDcopClient();
KWallet::Backend *w = _wallets.find(handle);

	if (dc && w) { // the handle is valid and we have a client
		if (_handles.contains(dc->senderId())) { // we know this app
			if (_handles[dc->senderId()].contains(handle)) {
				// the app owns this handle
				return w;
			}
		}
	}
return 0L;
}

#include "kwalletd.moc"
