/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 * Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/
#include "backtracegenerator.h"

#include <KDebug>
#include <KTemporaryFile>
#include <KShell>

#include "parser/backtraceparser.h"

BacktraceGenerator::BacktraceGenerator(const Debugger & debugger, QObject *parent)
        : QObject(parent),
          m_debugger(debugger), m_proc(NULL),
          m_temp(NULL), m_state(NotLoaded)
{
    m_parser = BacktraceParser::newParser(m_debugger.codeName(), this);
    m_parser->connectToGenerator(this);

#ifdef BACKTRACE_PARSER_DEBUG
    m_debugParser = BacktraceParser::newParser(QString(), this);   //uses the null parser
    m_debugParser->connectToGenerator(this);
#endif
}

BacktraceGenerator::~BacktraceGenerator()
{
    if (m_proc && m_proc->state() == QProcess::Running) {
        kWarning() << "Killing running debugger instance";
        m_proc->disconnect(this);
        m_proc->terminate();
        if (!m_proc->waitForFinished(10000)) {
            m_proc->kill();
            m_proc->waitForFinished();
        }
        delete m_proc;
        delete m_temp;
    }
}

bool BacktraceGenerator::start()
{
    //they should always be null before entering this function.
    Q_ASSERT(m_proc == NULL && m_temp == NULL);

    m_parsedBacktrace.clear();
    m_state = Loading;

    emit starting();

    if (!m_debugger.isValid() || !m_debugger.isInstalled()) {
        m_state = FailedToStart;
        emit failedToStart();
        return false;
    }

    m_proc = new KProcess;
    m_proc->setEnv("LC_ALL", "C");   // force C locale

    m_temp = new KTemporaryFile;
    m_temp->open();
    m_temp->write(m_debugger.backtraceBatchCommands().toLatin1());
    m_temp->write("\n", 1);
    m_temp->flush();

    // start the debugger
    QString str = m_debugger.command();
    Debugger::expandString(str, Debugger::ExpansionUsageShell, m_temp->fileName());

    *m_proc << KShell::splitArgs(str);
    m_proc->setOutputChannelMode(KProcess::OnlyStdoutChannel);
    m_proc->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Text);
    connect(m_proc, SIGNAL(readyReadStandardOutput()),
            SLOT(slotReadInput()));
    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)),
            SLOT(slotProcessExited(int,QProcess::ExitStatus)));

    m_proc->start();
    if (!m_proc->waitForStarted()) {
        //we mustn't keep these around...
        m_proc->deleteLater();
        m_temp->deleteLater();
        m_proc = NULL;
        m_temp = NULL;

        m_state = FailedToStart;
        emit failedToStart();
        return false;
    }

    return true;
}

void BacktraceGenerator::slotReadInput()
{
    // we do not know if the output array ends in the middle of an utf-8 sequence
    m_output += m_proc->readAllStandardOutput();

    int pos;
    while ((pos = m_output.indexOf('\n')) != -1) {
        QString line = QString::fromLocal8Bit(m_output, pos + 1);
        m_output.remove(0, pos + 1);

        emit newLine(line);
    }
}

void BacktraceGenerator::slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    //these are useless now
    m_proc->deleteLater();
    m_temp->deleteLater();
    m_proc = NULL;
    m_temp = NULL;

    //mark the end of the backtrace for the parser
    emit newLine(QString());

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        m_state = Failed;
        emit someError();
        return;
    }

    //no translation, string appears in the report
    QString tmp("Application: %progname (%execname), signal: %signame\n");
    Debugger::expandString(tmp);

    m_parsedBacktrace = tmp + m_parser->parsedBacktrace();
    m_state = Loaded;

#ifdef BACKTRACE_PARSER_DEBUG
    //append the raw unparsed backtrace
    m_parsedBacktrace += "\n------------ Unparsed Backtrace ------------\n";
    m_parsedBacktrace += m_debugParser->parsedBacktrace(); //it's not really parsed, it's from the null parser.
#endif

    emit done();
}

#include "backtracegenerator.moc"
