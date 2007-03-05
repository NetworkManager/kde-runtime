/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -l UIServer -m -i uiserver.h -a uiserveradaptor_p.h:uiserveradaptor.cpp /d/kde/src/trunk/kdelibs/kio/kio/org.kde.KIO.UIServer.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech ASA. All rights reserved.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "uiserveradaptor_p.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class UIServerAdaptor
 */

UIServerAdaptor::UIServerAdaptor(UIServer *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

UIServerAdaptor::~UIServerAdaptor()
{
    // destructor
}

void UIServerAdaptor::infoMessage(int id, const QString &msg)
{
    // handle method call org.kde.KIO.UIServer.infoMessage
    parent()->infoMessage(id, msg);
}

void UIServerAdaptor::jobFinished(int id)
{
    // handle method call org.kde.KIO.UIServer.jobFinished
    parent()->jobFinished(id);
}

int UIServerAdaptor::newJob(const QString &appServiceName, int capabilities, bool showProgress, const QString &internalAppName, const QString &jobIcon, const QString &appName)
{
    // handle method call org.kde.KIO.UIServer.newJob
    return parent()->newJob(appServiceName, capabilities, showProgress, internalAppName, jobIcon, appName);
}

void UIServerAdaptor::percent(int id, uint ipercent)
{
    // handle method call org.kde.KIO.UIServer.percent
    parent()->percent(id, ipercent);
}

void UIServerAdaptor::processedDirs(int id, uint dirs)
{
    // handle method call org.kde.KIO.UIServer.processedDirs
    parent()->processedDirs(id, dirs);
}

void UIServerAdaptor::processedFiles(int id, uint files)
{
    // handle method call org.kde.KIO.UIServer.processedFiles
    parent()->processedFiles(id, files);
}

void UIServerAdaptor::processedSize(int id, qulonglong size)
{
    // handle method call org.kde.KIO.UIServer.processedSize
    parent()->processedSize(id, size);
}

void UIServerAdaptor::progressInfoMessage(int id, const QString &msg)
{
    // handle method call org.kde.KIO.UIServer.progressInfoMessage
    parent()->progressInfoMessage(id, msg);
}

bool UIServerAdaptor::setDescription(int id, const QString &description)
{
    // handle method call org.kde.KIO.UIServer.setDescription
    return parent()->setDescription(id, description);
}

bool UIServerAdaptor::setDescriptionFirstField(int id, const QString &name, const QString &value)
{
    // handle method call org.kde.KIO.UIServer.setDescriptionFirstField
    return parent()->setDescriptionFirstField(id, name, value);
}

bool UIServerAdaptor::setDescriptionSecondField(int id, const QString &name, const QString &value)
{
    // handle method call org.kde.KIO.UIServer.setDescriptionSecondField
    return parent()->setDescriptionSecondField(id, name, value);
}

void UIServerAdaptor::setJobVisible(int jobId, bool visible)
{
    // handle method call org.kde.KIO.UIServer.setJobVisible
    parent()->setJobVisible(jobId, visible);
}

void UIServerAdaptor::speed(int id, const QString &bytesPerSecond)
{
    // handle method call org.kde.KIO.UIServer.speed
    parent()->speed(id, bytesPerSecond);
}

void UIServerAdaptor::totalDirs(int id, uint dirs)
{
    // handle method call org.kde.KIO.UIServer.totalDirs
    parent()->totalDirs(id, dirs);
}

void UIServerAdaptor::totalFiles(int id, uint files)
{
    // handle method call org.kde.KIO.UIServer.totalFiles
    parent()->totalFiles(id, files);
}

void UIServerAdaptor::totalSize(int id, qulonglong size)
{
    // handle method call org.kde.KIO.UIServer.totalSize
    parent()->totalSize(id, size);
}


#include "uiserveradaptor_p.moc"
