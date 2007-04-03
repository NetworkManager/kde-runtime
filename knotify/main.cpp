/*
   Copyright (C) 2005-2006 by Olivier Goffart <ogoffart at kde.org>


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


#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessage.h>
#include <kpassivepopupmessagehandler.h>

#include "knotify.h"


extern "C"{

KDE_EXPORT int kdemain(int argc, char **argv)
{
    KAboutData aboutdata("knotify", I18N_NOOP("KNotify"),
                         "4.0", I18N_NOOP("KDE Notification Daemon"),
                         KAboutData::License_GPL, "(C) 1997-2006, KDE Developers");
    aboutdata.addAuthor("Olivier Goffart",I18N_NOOP("Current Maintainer"),"pfeiffer@kde.org");
    aboutdata.addAuthor("Carsten Pfeiffer",I18N_NOOP("Previous Maintainer"),"pfeiffer@kde.org");
    aboutdata.addAuthor("Christian Esken",0,"esken@kde.org");
    aboutdata.addAuthor("Stefan Westerfeld",I18N_NOOP("Sound support"),"stefan@space.twc.de");
    aboutdata.addAuthor("Charles Samuels",I18N_NOOP("Previous Maintainer"),"charles@kde.org");
    aboutdata.addAuthor("Allan Sandfeld Jensen",I18N_NOOP("Porting to KDE 4"),"kde@carewolf.com");

    KCmdLineArgs::init( argc, argv, &aboutdata );
    KUniqueApplication::addCmdLineOptions();

    // initialize application
    if ( !KUniqueApplication::start() ) {
        kDebug() << "Running knotify found" << endl;
        return 0;
    }

    KUniqueApplication app;
    app.disableSessionManagement();
    
    /*
     * the default KMessageBoxMessageHandler will do messagesbox that notify
     * so we have a deadlock if one debug message is shown as messagebox.
     * that's why we're forced to change the default handler
     */
    KMessage::setMessageHandler( new KPassivePopupMessageHandler(0) );

    // start notify service
    KNotify notify;

    return app.exec();
}
}// end extern "C"
