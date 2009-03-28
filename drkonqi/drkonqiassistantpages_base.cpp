/*******************************************************************
* drkonqiassistantpages_base.cpp
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

#include "drkonqiassistantpages_base.h"
#include "drkonqi.h"
#include "krashconf.h"
#include "reportinfo.h"
#include "backtraceparser.h"
#include "backtracegenerator.h"
#include "drkonqi_globals.h"

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>

#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <ktoolinvocation.h>
#include <kicon.h>
#include <kmessagebox.h>
#include <klocale.h>

//BEGIN IntroductionPage

IntroductionPage::IntroductionPage( DrKonqiBugReport * parent ) : 
    DrKonqiAssistantPage(parent)
{
    QLabel * mainLabel = new QLabel(
        i18n("<para>This assistant will analyze the crash information and guide you through the bug reporting process</para><para>You can get help about this bug reporting assistant clicking the \"Help\" button</para><para>To start gathering the crash information press the \"Next\" button</para>")
    );
    mainLabel->setWordWrap( true );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( mainLabel );
    layout->addStretch();
    setLayout( layout );
}

//END IntroductionPage

//BEGIN CrashInformationPage

CrashInformationPage::CrashInformationPage( DrKonqiBugReport * parent ) 
    : DrKonqiAssistantPage(parent)
{
    m_backtraceWidget = new GetBacktraceWidget(DrKonqi::instance()->backtraceGenerator());
    
    //connect backtraceWidget signals
    connect( m_backtraceWidget, SIGNAL(stateChanged()) , this, SLOT(emitCompleteChanged()) );
    
    QLabel * subTitle = new QLabel(
        i18n( "This page will fetch some useful information about the crash to generate a better bug report" )
    );
    subTitle->setWordWrap( true ); 
    subTitle->setAlignment( Qt::AlignHCenter );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( subTitle );
    layout->addWidget( m_backtraceWidget );
    setLayout( layout );    
}

void CrashInformationPage::aboutToShow()
{ 
    m_backtraceWidget->generateBacktrace(); 
    emitCompleteChanged();
}

bool CrashInformationPage::isComplete()
{
    BacktraceGenerator *generator = DrKonqi::instance()->backtraceGenerator();
    return ( generator->state() != BacktraceGenerator::NotLoaded &&
             generator->state() != BacktraceGenerator::Loading );
}

bool CrashInformationPage::showNextPage()
{
    BacktraceParser::Usefulness use = DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();
    
    if( use == BacktraceParser::InvalidUsefulness || use == BacktraceParser::ProbablyUseless
        || use == BacktraceParser::Useless )
    {
        if( KMessageBox::questionYesNo( this, i18n( "The crash information is not useful enought, do you want to try to improve it?" ), i18n("Crash Information is not useful enought") ) == KMessageBox::Yes )
        {
            return false; //Cancel show next, to allow the user to write more
        } else {
            return true; //Allow to continue
        }
    } else {
        return true;
    }
}

//END CrashInformationPage

//BEGIN BugAwarenessPage

BugAwarenessPage::BugAwarenessPage( DrKonqiBugReport * parent ) 
    : DrKonqiAssistantPage(parent)
{
    //User can detail layout
    QVBoxLayout * canDetailLayout = new QVBoxLayout( );
    
    QLabel * detailQuestionLabel = new QLabel( QString("<strong>%1</strong>").arg( i18n( "Can you give detailed information about what were you doing when the application crashed ?" ) ) );
    detailQuestionLabel->setWordWrap( true );
    canDetailLayout->addWidget( detailQuestionLabel );
    
    QLabel * detailLabel = 
        new QLabel(
        i18n( "Including in your bug report what you were doing when the application crashed will help the developers "
        "reproduce and fix the bug. <i>( Important details are: what you were doing in the application and documents you were using )</i>. If you have found a common pattern which causes this crash, you can try to write the steps to help the developers reproduce it and fix it." ) );
        
    detailLabel->setWordWrap( true );
    canDetailLayout->addWidget( detailLabel );
    
    m_canDetailCheckBox = new QCheckBox( i18n( "Yes, I can give information about the crash" ) );
    canDetailLayout->addWidget( m_canDetailCheckBox );
    
    //User is willing to help layout
    QVBoxLayout * willingToHelpLayout = new QVBoxLayout();
    
    QLabel * willingToHelpQuestionLabel = new QLabel( i18n( "<strong>Are you willing to help the developers?</strong>" ) );
    willingToHelpLayout->addWidget( willingToHelpQuestionLabel );
    
    QLabel * willingToHelpLabel = 
        new QLabel( i18n( "Sometimes, while the report is being investigated, the developers need more information from the reporter in order to fix the bug." ) );
    willingToHelpLabel->setWordWrap( true );
    willingToHelpLayout->addWidget( willingToHelpLabel );
    
    m_willingToHelpCheckBox = new QCheckBox( i18n( "Yes, I am willing to help the developers" ) );
    willingToHelpLayout->addWidget( m_willingToHelpCheckBox );
    
    //Main layout
    QVBoxLayout * layout = new QVBoxLayout();
    layout->setSpacing( 8 );
    layout->addLayout( canDetailLayout );
    layout->addLayout( willingToHelpLayout );
    layout->addStretch();
    setLayout( layout );
}

void BugAwarenessPage::aboutToHide()
{
    //Save data
    reportInfo()->setUserCanDetail( m_canDetailCheckBox->checkState() == Qt::Checked );
    reportInfo()->setUserIsWillingToHelp( m_willingToHelpCheckBox->checkState() == Qt::Checked );
}

//END BugAwarenessPage

//BEGIN ConclusionPage

ConclusionPage::ConclusionPage( DrKonqiBugReport * parent ) 
    : DrKonqiAssistantPage(parent),
    needToReport(false)
{
    isBKO = DrKonqi::instance()->krashConfig()->isKDEBugzilla();
    
    m_reportEdit = new KTextBrowser();
    m_reportEdit->setReadOnly( true );

    m_saveReportButton = new KPushButton( KGuiItem2( i18nc( "Button action", "&Save to File" ), KIcon("document-save"), i18nc("button explanation", "Use this button to save the generated report and conclusions about this crash to a file. You can use this option to report the bug later.") ) );
    connect(m_saveReportButton, SIGNAL(clicked()), this, SLOT(saveReport()) );
    
    m_reportButton = new KPushButton( KGuiItem2( i18nc( "button action", "&Report bug to the application maintainer" ), KIcon("document-new"), i18nc( "button explanation", "Use this button to report this bug to the application maintainer. This will launch the browser or the mail client using the assigned bug report address" ) ) );
    m_reportButton->setVisible( false );
    connect( m_reportButton, SIGNAL(clicked()), this , SLOT(reportButtonClicked()) );

    m_explanationLabel = new QLabel();
    m_explanationLabel->setWordWrap( true );
    
    QHBoxLayout *bLayout = new QHBoxLayout();
    bLayout->addStretch();
    bLayout->addWidget( m_saveReportButton );
    bLayout->addWidget( m_reportButton );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 10 );
    layout->addWidget( m_reportEdit );
    layout->addWidget( m_explanationLabel );
    layout->addLayout( bLayout );
    setLayout( layout );
}

void ConclusionPage::saveReport()
{
    DrKonqi::saveReport(m_reportEdit->toPlainText(), this);
}

void ConclusionPage::reportButtonClicked()
{
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();

    if( krashConfig->isReportMail() )
    {
        QString subject = QString( "Automatic crash report generated by DrKonqi for " + krashConfig->productName() );
        QString body = reportInfo()->generateReportTemplate();
        KToolInvocation::invokeMailer( krashConfig->getReportLink(), "","" ,subject, body);
    }
    else
    {
        KToolInvocation::invokeBrowser( krashConfig->getReportLink() );
    }
    
    emit finished(false);
}

void ConclusionPage::aboutToShow()
{
    needToReport = false;
    emitCompleteChanged();

    QString report;
    
    const KrashConfig * krashConfig = DrKonqi::instance()->krashConfig();
    BacktraceParser::Usefulness use = DrKonqi::instance()->backtraceGenerator()->parser()->backtraceUsefulness();
    bool canDetails = reportInfo()->getUserCanDetail();
    bool willingToHelp = reportInfo()->getUserIsWillingToHelp();
    
    // Estimate needToReport
    switch( use )
    {
        case BacktraceParser::ReallyUseful:
        {
            needToReport = ( canDetails || willingToHelp );
            report = i18n( "* The crash information is really useful and worth reporting" );
            break;
        }
        case BacktraceParser::MayBeUseful:
        {
            needToReport = ( canDetails || willingToHelp );
            report = i18n( "* The crash information lacks some details but may be useful" ) ;
            break;
        }
        case BacktraceParser::ProbablyUseless:
        {
            needToReport = ( canDetails && willingToHelp );
            report = i18n( "* The crash information lacks a lot of important details and it is probably not really useful" );
            break;
        }           
        case BacktraceParser::Useless:
        case BacktraceParser::InvalidUsefulness:
        {
            needToReport =  false;
            report = i18n( "* The crash information is not really useful" ) ;
     break;
        }
    }
    
    //User can provide details
    report.append( QLatin1String( "<br />" ) );
    if( canDetails )
        report += i18n( "* You can explain in detail what were you doing when the application crashed" );
    else
        report += i18n( "* You are not sure what were you doing when the application crashed" ) ;
        
    //User is willing to help
    report.append( QLatin1String( "<br />" ) );
    if( willingToHelp )
        report += i18n( "* You are willing to help the developers" );
    else
        report += i18n( "* You are not willing to help the developers" );
        
    if ( needToReport )
    {
        QString reportMethod;
        QString reportLink;
        
        m_reportButton->setVisible( !isBKO );
        if ( isBKO )
        {
            emitCompleteChanged();
            m_explanationLabel->setText( i18n( "This application is supported in the KDE bug tracking system, press Next to start the report assistant" ) );
            
            reportMethod = i18n( "You need to file a new bug report, press Next to start the report assistant" );
            reportLink = i18nc( "address to report the bug", "You can manually report at %1", QLatin1String(KDE_BUGZILLA_URL) );
        }
        else
        {
            m_explanationLabel->setText( i18n( "<strong>Notice:</strong> This application is not supported in the KDE Bugtracker, you need to report the bug to the maintainer" ) );
            
            reportMethod = i18n( "You need to file a new bug report with the following information:" );
            reportLink =  i18nc( "address(mail or web) to report the bug", "You can manually report at %1", krashConfig->getReportLink() );
            
            if( krashConfig->isReportMail() )
            {
                m_reportButton->setGuiItem( KGuiItem2( i18n("Send an e-mail to the application maintainer" ), KIcon("mail-message-new"), i18n( "Launches the mail client to send an e-mail to the application maintainer with the crash information" ) ) ) ;
            }
            else
            {
                m_reportButton->setGuiItem( KGuiItem2( i18n("Open the application maintainer's web site" ), KIcon("internet-web-browser"), i18n( "Launches the web browser showing the application's web site in order to contact the maintainer" ) ) ) ;
            }
        }
        
        report += QString("<br /><strong>%1</strong><br />%2<br /><br />--------<br /><br />%3").arg( i18n( "This crash is worth reporting" ), reportMethod, reportInfo()->generateReportTemplate() );
        
        report += QString("<br /><br />") + reportLink;
    }
    else
    {
        m_reportButton->setVisible( false );
        
        report += QString("<br /><strong>%1</strong><br />%2<br />--------<br /><br />%3").arg( i18n( "This crash is not worth reporting" ), i18n( "However, you can report it on your own if you want, using the following information:" ), reportInfo()->generateReportTemplate() );
        
        report.append( QString( "<br /><br />" ) );
        if ( krashConfig->isKDEBugzilla() )
        {
            report += i18nc( "address to report the bug", "Report at %1", QLatin1String(KDE_BUGZILLA_URL) );
            m_explanationLabel->setText( i18n( "This application is supported in the KDE bug tracking system. You can report this bug at %1",  QLatin1String(KDE_BUGZILLA_URL) ) );
        }
        else
        {
            report += i18nc( "address(mail or web) to report the bug", "Report at %1", krashConfig->getReportLink() );
            m_explanationLabel->setText( i18n( "This application is not supported in the KDE bug tracking system. You can report this bug to its maintainer : <i>%1</i>", krashConfig->getReportLink() ) );
        }
        
        emit finished(true);
    }
    
    m_reportEdit->setHtml( report );
}

bool ConclusionPage::isComplete()
{
    return ( isBKO && needToReport );
}

//END ConclusionPage
