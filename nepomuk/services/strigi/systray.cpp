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

#include "systray.h"
#include "indexscheduler.h"
#include "strigiservice.h"

#include <KMenu>
#include <KToggleAction>
#include <KLocale>
#include <KIcon>
#include <KToolInvocation>
#include <KActionCollection>
#include <KStandardAction>



Nepomuk::SystemTray::SystemTray( StrigiService* service, QWidget* parent )
    : KNotificationItem( parent ),
      m_service( service )
{
    setCategory( SystemServices );
    setStatus( Passive );
    setIcon( "nepomuk" );

    KMenu* menu = new KMenu;
    menu->addTitle( i18n( "Strigi File Indexing" ) );

    m_suspendResumeAction = new KToggleAction( i18n( "Suspend Strigi Indexing" ), menu );
    m_suspendResumeAction->setCheckedState( KGuiItem( i18n( "Suspend Strigi Indexing" ) ) );
    m_suspendResumeAction->setToolTip( i18n( "Suspend or resume the Strigi file indexer manually" ) );
    connect( m_suspendResumeAction, SIGNAL( toggled( bool ) ),
             m_service, SLOT( setSuspended( bool ) ) );

    KAction* configAction = new KAction( menu );
    configAction->setText( i18n( "Configure Strigi" ) );
    configAction->setIcon( KIcon( "configure" ) );
    connect( configAction, SIGNAL( triggered() ),
             this, SLOT( slotConfigure() ) );

    menu->addAction( m_suspendResumeAction );
    menu->addAction( configAction );

    connect( m_service, SIGNAL( statusStringChanged() ), this, SLOT( slotUpdateStrigiStatus() ) );

    setContextMenu( menu );

    // trueg: Hack until KNotificationItem allows to disable default actions
    connect( menu, SIGNAL(aboutToShow()), this, SLOT(slotContextMenuAboutToShow()));
}


Nepomuk::SystemTray::~SystemTray()
{
}


void Nepomuk::SystemTray::slotUpdateStrigiStatus()
{
    setToolTip("nepomuk", i18n("Nepomuk"),  m_service->userStatusString() );
    m_suspendResumeAction->setChecked( m_service->indexScheduler()->isSuspended() );
    if (m_service->indexScheduler()->isIndexing()) {
        setStatus(Active);
    } else {
        setStatus(Passive);
    }
}


void Nepomuk::SystemTray::slotConfigure()
{
    QStringList args;
    args << "kcm_nepomuk";
    KToolInvocation::kdeinitExec("kcmshell4", args);
}


void Nepomuk::SystemTray::slotContextMenuAboutToShow()
{
    if ( QAction* a = actionCollection()->action( KStandardAction::name(KStandardAction::Quit) ) )
        contextMenu()->removeAction( a );
}

#include "systray.moc"
