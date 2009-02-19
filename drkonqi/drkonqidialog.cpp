/*******************************************************************
* drkonqidialog.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#include "drkonqidialog.h"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>

#include <kicon.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include "krashconf.h"
#include "crashinfo.h"

#include "getbacktracewidget.h"
#include "drkonqibugreport.h"
#include "aboutbugreportingdialog.h"

DrKonqiDialog::DrKonqiDialog(KrashConfig * conf, QWidget * parent) : 
    KDialog(parent), 
    aboutBugReportingDialog(0),
    backtraceWidget(0)
{
    crashInfo = new CrashInfo(conf);
    
    setCaption( i18n("An Error Ocurred") );
    setWindowIcon( KIcon("tools-report-bug") );

    //GUI
    QLabel * title = new QLabel( i18n("<strong>An Error Ocurred and the Application Closed</strong>") );
    title->setWordWrap( true ); 
    
    QLabel * pixmap = new QLabel("");
    pixmap->setPixmap( KIcon("application-exit").pixmap( QSize(40,40) ) );
    pixmap->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ) );
    
    QHBoxLayout * layoutTitle = new QHBoxLayout();
    layoutTitle->addWidget( title );
    //layoutTitle->addWidget( pixmap );
    
    QLabel * crashTitle = new QLabel( crashInfo->getCrashTitle() );
    crashTitle->setWordWrap( true ); 
    
    QLabel * info = new QLabel( i18nc("Small explanation of the crash cause",
    "<para>This probably happened because there is a bug in the application.</para><nl />"
    "<para>[Signal Explanation]</para><nl />"
    "<nl /><para>You can help us to improve the software you use reporting this bug.</para>"
    ) );
    info->setWordWrap( true ); 

    aboutBugReportingButton = new KPushButton( KGuiItem( i18nc("button action", "Learn more about bug reporting") , KIcon("help-hint"),  i18nc("help text", "Get help in order to know how to file an useful bug report") ) ); //TODO
    aboutBugReportingButton->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
    connect( aboutBugReportingButton, SIGNAL(clicked()), this, SLOT(aboutBugReporting()) );

    //Introduction widget layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 5 );
    layout->addLayout( layoutTitle );
    layout->addSpacing( 15 );
    layout->addWidget( crashTitle );
    layout->addSpacing( 15 );
    layout->addWidget( info );
    layout->addWidget( aboutBugReportingButton );
    layout->addStretch();
    
    //Introduction widget
    introWidget = new QWidget();
    introWidget->setLayout( layout );
    introWidget->setMinimumSize( QSize(550, 250) );
    
    QString styleSheet = QString(".QWidget {"
                       "background-image: url(%1);"
                       "background-repeat: no-repeat;"
                       "background-position: right;"
                       "}").arg(KStandardDirs::locate("appdata", QLatin1String("pics/konqi.png")));
    introWidget->setStyleSheet(styleSheet);
    
    //Backtrace Widget
    backtraceWidget = new GetBacktraceWidget( crashInfo );
    backtraceWidget->setMinimumSize( QSize(550, 250) );

    //Stacked main widget
    stackedWidget = new QStackedWidget();
    stackedWidget->addWidget( introWidget );
    stackedWidget->addWidget( backtraceWidget );
    setMainWidget( stackedWidget );
    
    //Set kdialog buttons
    setButtons( KDialog::User1 | KDialog::User2 | KDialog::User3 | KDialog::Close );
    
    setButtonGuiItem( KDialog::User3, KGuiItem( i18nc("Button action","Show Backtrace (Advanced)"), KIcon("document-new"), i18nc("help text","Generate the backtrace (crash information)") ) );
    connect( this, SIGNAL(user3Clicked()), this, SLOT(toggleBacktrace()) );
    setButtonGuiItem( KDialog::User2, KGuiItem( i18nc("Button action", "Start Debugger") , KIcon("document-edit"), i18nc("help text", "Starts the configured application to debug crashes" )) );
    setButtonGuiItem( KDialog::User1, KGuiItem( i18nc("Button action", "Report Bug"), KIcon("tools-report-bug"), i18nc("help text", "Starts the bug report assistant" )) );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(reportBugAssistant()) );
    
    setButtonToolTip( KDialog::Close, i18nc("help text", "Close this dialog (you will lose the crash information)") );
    
    enableButton( KDialog::User2, false );
    
    setDefaultButton( KDialog::Close );
    setButtonFocus( KDialog::Close );    
}

void DrKonqiDialog::reportBugAssistant()
{
    DrKonqiBugReport * assistant = new DrKonqiBugReport( crashInfo );
    assistant->show();
    close();
}

void DrKonqiDialog::aboutBugReporting()
{
    if ( !aboutBugReportingDialog )
    {
        aboutBugReportingDialog = new AboutBugReportingDialog( this );
    }
    
    aboutBugReportingDialog->show();
}

void DrKonqiDialog::toggleBacktrace()
{
    if( stackedWidget->currentWidget() == introWidget )
    {
        backtraceWidget->generateBacktrace();
        stackedWidget->setCurrentWidget( backtraceWidget );
        
        setButtonGuiItem( KDialog::User3, KGuiItem(i18nc("button action", "Show Introduction"), KIcon("help-contextual"), i18nc("help text", "Show the Crash Dialog introduction page" )) );
    }
    else
    {
        stackedWidget->setCurrentWidget( introWidget );
        
        setButtonGuiItem( KDialog::User3, KGuiItem(i18nc("button action", "Show Backtrace (Advanced)"), KIcon("document-new"), i18nc("help text", "Generate and show the backtrace (crash information)" )) );
    }
}

DrKonqiDialog::~DrKonqiDialog()
{
    delete crashInfo;
    delete aboutBugReportingDialog;
    delete backtraceWidget;
    delete stackedWidget;
}
