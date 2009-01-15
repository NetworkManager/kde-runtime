/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 * Copyright (C) 2008 Lubos Lunak <l.lunak@kde.org>
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

#ifndef BACKTRACEGDB_H
#define BACKTRACEGDB_H

#include "backtrace.h"

class BackTraceGdb : public BackTrace
{
  Q_OBJECT

public:
  BackTraceGdb(const KrashConfig *krashconf, QObject *parent);
  ~BackTraceGdb();

protected:
  virtual QString processDebuggerOutput( QString bt );
  virtual bool usefulDebuggerOutput( QString bt );
private:
  void processBacktrace( int index );
  bool usefulBacktrace( int index );
  QStringList removeLines( QStringList list, const QRegExp& regexp );
  QStringList removeFirstLine( QStringList list, const QRegExp& regexp );
  bool prettyKcrashHandler( int index );
  QStringList common; // the first part gdb output, before any backtraces
  QList< QStringList > backtraces; // backtraces, more if there are more threads
  int backtraceWithCrash; // index of the thread in which the crash happened
};

#endif
