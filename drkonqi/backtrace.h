/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
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

#ifndef BACKTRACE_H
#define BACKTRACE_H

class KrashConfig;
class KTemporaryFile;

#include <KProcess>

class BackTrace : public QObject
{
  Q_OBJECT

public:
  BackTrace(const KrashConfig *krashconf, QObject *parent);
  ~BackTrace();

  void start();

Q_SIGNALS:
  void append(const QString &str); // Just the new text

  void someError();
  void done(const QString &); // replaces whole text

protected Q_SLOTS:
  void slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus);
  void slotReadInput();

protected:
  virtual QString processDebuggerOutput( QString bt ) = 0;
  virtual bool usefulDebuggerOutput( QString bt ) = 0;
  const KrashConfig * const m_krashconf;

private:
  KProcess *m_proc;
  KTemporaryFile *m_temp;
  QString m_strBt;
  QByteArray m_output;
};
#endif
