/*
Copyright (C) 2011  Smit Shah <Who828@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include<QStringList>
#include <QFile>
#include <QUrl>

#include<taglib/fileref.h>
#include<taglib/tag.h>
#include<taglib/tstring.h>

#include<Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Variant>

#include<mywritebackplugin.h>
using namespace Nepomuk::Vocabulary;
Nepomuk::MyWritebackPlugin::MyWritebackPlugin(QObject* parent): WritebackPlugin(parent)

{

}

Nepomuk::MyWritebackPlugin::~MyWritebackPlugin()
{

}


void Nepomuk::MyWritebackPlugin::doWriteback(const QUrl& url)
{
    Nepomuk::Resource resource(url);
if(resource.exists())
   {
    // creatin  Nepomuk::Resource resource(KUrl(url));
    TagLib::FileRef f(QFile::encodeName( url.toLocalFile()).data());
    // just an example
    QString m_album = (resource.property(NIE::title())).toString();
    if(Q4StringToTString(m_album) == f.tag()->album())
{
    f.tag()->setAlbum(Q4StringToTString(m_album));
   // f.tag()->setTitle("Joy");
    //f.tag()->setArtist("Who");
    //f.tag()->setComment("this is the best song,ever !");
    f.save();

    emitFinished();
}
}
}

