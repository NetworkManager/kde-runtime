/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/                                                                            

#include <qlistbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qregexp.h>

#include <kglobal.h>
#include <klocale.h>

#include "searchwidget.h"
#include "searchwidget.moc"
#include "global.h"

KeywordListEntry::KeywordListEntry(const QString& name, ConfigModule* module)
  : _name(name)
{
  if(module)
    _modules.append(module);
}
  
void KeywordListEntry::addModule(ConfigModule* module)
{
  if(module)
    _modules.append(module);
}

SearchWidget::SearchWidget(QWidget *parent , const char *name)
  : QWidget(parent, name)
{
  _keywords.setAutoDelete(true);

  QVBoxLayout * l = new QVBoxLayout(this, 2, 2);

  // input
  _input = new QLineEdit(this);
  QLabel *inputl = new QLabel(_input, i18n("Se&arch:"), this);

  l->addWidget(inputl);
  l->addWidget(_input);

  // keyword list
  _keyList = new QListBox(this);
  QLabel *keyl = new QLabel(_keyList, i18n("&Keywords:"), this);  

  l->addWidget(keyl);
  l->addWidget(_keyList);

  // result list
  _resultList = new QListBox(this);
  QLabel *resultl = new QLabel(_keyList, i18n("&Results:"), this);  

  l->addWidget(resultl);
  l->addWidget(_resultList);


  connect(_input, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotSearchTextChanged(const QString&)));

  connect(_keyList, SIGNAL(highlighted(const QString&)),
	  this, SLOT(slotKeywordSelected(const QString&)));

  connect(_resultList, SIGNAL(highlighted(const QString&)),
	  this, SLOT(slotModuleSelected(const QString&)));
}

void SearchWidget::populateKeywordList(ConfigModuleList *list)
{
  ConfigModule *module;

  // loop through all control modules
  for (module=list->first(); module != 0; module=list->next())
    {
      if (module->library().isEmpty())
	continue;
      
      if (KCGlobal::system()) {
	  if (!module->onlyRoot())
	    continue;
	}
      else {
	  if (module->onlyRoot() && !KCGlobal::root())
	    continue;
	}
      
      // get the modules keyword lsit
      QStringList kw = module->keywords();

      // loop through the keyword list to populate _keywords
      for(QStringList::ConstIterator it = kw.begin(); it != kw.end(); it++)
	{
	  QString name = *it;
	  bool found = false;

	  // look if _keywords already has an entry for this keyword
	  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
	    {
	      // if there is an entry for this keyword, add the module to the entries modul list
	      if (k->name() == name)
		{
		  k->addModule(module);
		  found = true;
		  break;
		}
	    }

	  // if there is entry for this keyword, create a new one
	  if (!found)
	    {
	      KeywordListEntry *k = new KeywordListEntry(name, module);
	      _keywords.append(k);
	    }
	}
    }
  populateKeyListBox("*");
}

void SearchWidget::populateKeyListBox(const QString& s)
{
  _keyList->clear();

  QStringList matches;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if ( QRegExp(s, false, true).match(k->name()) >= 0)   
	  matches.append(k->name());
    }

  matches.sort();

  for(QStringList::ConstIterator it = matches.begin(); it != matches.end(); it++)
      _keyList->insertItem(*it);
}

void SearchWidget::populateResultListBox(const QString& s)
{
  _resultList->clear();

  QStringList results;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if (k->name() == s)
	{
	  QList<ConfigModule> modules = k->modules();

	  for(ConfigModule *m = modules.first(); m != 0; m = modules.next())
	    results.append(m->name());
	}
    }

  results.sort();

  for(QStringList::ConstIterator it = results.begin(); it != results.end(); it++)
    _resultList->insertItem(*it);
}

void SearchWidget::slotSearchTextChanged(const QString & s)
{
  QString regexp = s;
  regexp += "*";
  populateKeyListBox(regexp);
}

void SearchWidget::slotKeywordSelected(const QString & s)
{
  populateResultListBox(s);
}

void SearchWidget::slotModuleSelected(const QString & s)
{
  emit moduleSelected(s);
}

