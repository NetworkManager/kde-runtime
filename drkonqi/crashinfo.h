/*******************************************************************
* crashinfo.h
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

#ifndef CRASHINFO__H
#define CRASHINFO__H

#include "backtraceinfo.h"
#include "krashconf.h"
#include "backtrace.h"
#include <kdeversion.h>

class CrashInfo : public QObject
{
    Q_OBJECT
    
  public:
    
    enum BacktraceGenState { DebuggerFailed, NonLoaded, Loading, Loaded, Failed };
    
    CrashInfo( KrashConfig* );
    ~CrashInfo();
    
    const KrashConfig * getCrashConfig() { return m_crashConfig; }
    const QString getCrashTitle();
    
    void generateBacktrace();
    
    BacktraceInfo * getBacktraceInfo() { return m_backtraceInfo; }
    const QString getBacktraceOutput() { return m_backtraceOutput; } 
    BacktraceGenState getBacktraceState() { return m_backtraceState; }
    
    void setUserCanDetail( bool canDetail ) { m_userCanDetail = canDetail; }
    void setUserCanReproduce ( bool canReproduce ) { m_userCanReproduce = canReproduce; }
    void setUserGetCompromise ( bool getCompromise ) { m_userGetCompromise = getCompromise; }
    
    bool getUserCanDetail() { return m_userCanDetail; }
    bool getUserCanReproduce() { return m_userCanReproduce; }
    bool getUserGetCompromise() { return m_userGetCompromise; }
    
    bool isKDEBugzilla();
    bool isReportMail();
    QString getReportLink();
    
    QString getKDEVersion() { return KDE_VERSION_STRING; } 
    QString getQtVersion() { return QT_VERSION_STR; }
    QString getOS();
    QString getProductName();
    QString getProductVersion();
    
    QString getDebugger();
    
    QString generateReportTemplate();
    
  Q_SIGNALS:
    void backtraceGenerated();
    void backtraceNewData( QString );
  
  private Q_SLOTS:
    void backtraceGeneratorFinished( const QString & );
    void backtraceGeneratorAppend( const QString & );
    void backtraceGeneratorFailed();
    
  private:
  
    KrashConfig *       m_crashConfig;
    
    BackTrace *         m_backtraceGenerator;
    BacktraceInfo *     m_backtraceInfo;
    QString             m_backtraceOutput;
    BacktraceGenState   m_backtraceState;
    
    bool                m_userCanDetail;
    bool                m_userCanReproduce;
    bool                m_userGetCompromise;
  
    QString             m_OS;
};

#endif
