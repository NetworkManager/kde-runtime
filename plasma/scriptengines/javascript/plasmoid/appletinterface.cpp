/*
 *   Copyright 2008 Chani Armitage <chani@kde.org>
 *   Copyright 2008, 2009 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2010 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "appletinterface.h"

#include <QAction>
#include <QFile>
#include <QScriptEngine>
#include <QSignalMapper>
#include <QTimer>

#include <KDebug>
#include <KIcon>
#include <KService>
#include <KServiceTypeTrader>

#include <Plasma/Plasma>
#include <Plasma/Applet>
#include <Plasma/Corona>
#include <Plasma/Context>
#include <Plasma/Package>

Q_DECLARE_METATYPE(AppletInterface*)

AppletInterface::AppletInterface(AbstractJsAppletScript *parent)
    : QObject(parent),
      m_appletScriptEngine(parent),
      m_actionSignals(0)
{
    connect(this, SIGNAL(releaseVisualFocus()), applet(), SIGNAL(releaseVisualFocus()));
    connect(this, SIGNAL(configNeedsSaving()), applet(), SIGNAL(configNeedsSaving()));
    connect(applet(), SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)), this, SIGNAL(immutableChanged()));
    connect(applet(), SIGNAL(newStatus(Plasma::ItemStatus)), this, SIGNAL(statusChanged()));
}

AppletInterface::~AppletInterface()
{
}

AppletInterface *AppletInterface::extract(QScriptEngine *engine)
{
    QScriptValue appletValue = engine->globalObject().property("plasmoid");
    return qobject_cast<AppletInterface*>(appletValue.toQObject());
}

Plasma::DataEngine* AppletInterface::dataEngine(const QString &name)
{
    return applet()->dataEngine(name);
}

AppletInterface::FormFactor AppletInterface::formFactor() const
{
    return static_cast<FormFactor>(applet()->formFactor());
}

AppletInterface::Location AppletInterface::location() const
{
    return static_cast<Location>(applet()->location());
}

QString AppletInterface::currentActivity() const
{
    return applet()->context()->currentActivity();
}

AppletInterface::AspectRatioMode AppletInterface::aspectRatioMode() const
{
    return static_cast<AspectRatioMode>(applet()->aspectRatioMode());
}

void AppletInterface::setAspectRatioMode(AppletInterface::AspectRatioMode mode)
{
    applet()->setAspectRatioMode(static_cast<Plasma::AspectRatioMode>(mode));
}

bool AppletInterface::shouldConserveResources() const
{
    return applet()->shouldConserveResources();
}

void AppletInterface::setFailedToLaunch(bool failed, const QString &reason)
{
    m_appletScriptEngine->setFailedToLaunch(failed, reason);
}

bool AppletInterface::isBusy() const
{
    return applet()->isBusy();
}

void AppletInterface::setBusy(bool busy)
{
    applet()->setBusy(busy);
}

AppletInterface::BackgroundHints AppletInterface::backgroundHints() const
{
    return static_cast<BackgroundHints>(static_cast<int>(applet()->backgroundHints()));
}

void AppletInterface::setBackgroundHints(BackgroundHints hint)
{
    applet()->setBackgroundHints(Plasma::Applet::BackgroundHints(hint));
}

void AppletInterface::setConfigurationRequired(bool needsConfiguring, const QString &reason)
{
    m_appletScriptEngine->setConfigurationRequired(needsConfiguring, reason);
}

void JsAppletInterface::update(const QRectF &rect)
{
    applet()->update(rect);
}

QGraphicsLayout *JsAppletInterface::layout() const
{
    return applet()->layout();
}

void JsAppletInterface::setLayout(QGraphicsLayout *layout)
{
    applet()->setLayout(layout);
}

QString AppletInterface::activeConfig() const
{
    return m_currentConfig.isEmpty() ? "main" : m_currentConfig;
}

void AppletInterface::setActiveConfig(const QString &name)
{
    if (name == "main") {
        m_currentConfig.clear();
        return;
    }

    Plasma::ConfigLoader *loader = m_configs.value(name, 0);

    if (!loader) {
        QString path = m_appletScriptEngine->filePath("config", name + ".xml");
        if (path.isEmpty()) {
            return;
        }

        QFile f(path);
        KConfigGroup cg = applet()->config();
        loader = new Plasma::ConfigLoader(&cg, &f, this);
        m_configs.insert(name, loader);
    }

    m_currentConfig = name;
}

void AppletInterface::writeConfig(const QString &entry, const QVariant &value)
{
    Plasma::ConfigLoader *config = 0;
    if (m_currentConfig.isEmpty()) {
        config = applet()->configScheme();
    } else {
        config = m_configs.value(m_currentConfig, 0);
    }

    if (config) {
        KConfigSkeletonItem *item = config->findItemByName(entry);
        if (item) {
            item->setProperty(value);
            config->blockSignals(true);
            config->writeConfig();
            config->blockSignals(false);
            m_appletScriptEngine->configNeedsSaving();
        }
    }
}

QScriptValue AppletInterface::readConfig(const QString &entry) const
{
    Plasma::ConfigLoader *config = 0;
    QVariant result;

    if (m_currentConfig.isEmpty()) {
        config = applet()->configScheme();
    } else {
        config = m_configs.value(m_currentConfig, 0);
    }

    if (config) {
        result = config->property(entry);
    }

    return m_appletScriptEngine->variantToScriptValue(result);
}

QString AppletInterface::file(const QString &fileType)
{
    return m_appletScriptEngine->filePath(fileType, QString());
}

QString AppletInterface::file(const QString &fileType, const QString &filePath)
{
    return m_appletScriptEngine->filePath(fileType, filePath);
}

QList<QAction*> AppletInterface::contextualActions() const
{
    QList<QAction*> actions;
    Plasma::Applet *a = applet();
    if (a->hasFailedToLaunch()) {
        return actions;
    }

    foreach (const QString &name, m_actions) {
        QAction *action = a->action(name);

        if (action) {
            actions << action;
        }
    }

    return actions;
}

QSizeF AppletInterface::size() const
{
    return applet()->size();
}

QRectF AppletInterface::rect() const
{
    return applet()->contentsRect();
}

void AppletInterface::setAction(const QString &name, const QString &text, const QString &icon, const QString &shortcut)
{
    Plasma::Applet *a = applet();
    QAction *action = a->action(name);

    if (action) {
        action->setText(text);
    } else {
        action = new QAction(text, this);
        a->addAction(name, action);

        Q_ASSERT(!m_actions.contains(name));
        m_actions.append(name);

        if (!m_actionSignals) {
            m_actionSignals = new QSignalMapper(this);
            connect(m_actionSignals, SIGNAL(mapped(QString)),
                    m_appletScriptEngine, SLOT(executeAction(QString)));
        }

        connect(action, SIGNAL(triggered()), m_actionSignals, SLOT(map()));
        m_actionSignals->setMapping(action, name);
    }

    if (!icon.isEmpty()) {
        action->setIcon(KIcon(icon));
    }

    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }

    action->setObjectName(name);
}

void AppletInterface::removeAction(const QString &name)
{
    Plasma::Applet *a = applet();
    QAction *action = a->action(name);

    if (action) {
        if (m_actionSignals) {
            m_actionSignals->removeMappings(action);
        }

        delete action;
    }

    m_actions.removeAll(name);
}

QAction *AppletInterface::action(QString name) const
{
    return applet()->action(name);
}

void AppletInterface::resize(qreal w, qreal h)
{
    applet()->resize(w,h);
}

void AppletInterface::setMinimumSize(qreal w, qreal h)
{
    applet()->setMinimumSize(w,h);
}

void AppletInterface::setPreferredSize(qreal w, qreal h)
{
    applet()->setPreferredSize(w,h);
}

bool AppletInterface::immutable() const
{
    return applet()->immutability() != Plasma::Mutable;
}

bool AppletInterface::userConfiguring() const
{
    return applet()->isUserConfiguring();
}

int AppletInterface::apiVersion() const
{
    const QString constraint("[X-Plasma-API] == 'javascript' and 'Applet' in [X-Plasma-ComponentTypes]");
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/ScriptEngine", constraint);
    if (offers.isEmpty()) {
        return -1;
    }

    return offers.first()->property("X-KDE-PluginInfo-Version", QVariant::Int).toInt();
}

bool AppletInterface::include(const QString &script)
{
    const QString path = m_appletScriptEngine->filePath("scripts", script);

    if (path.isEmpty()) {
        return false;
    }

    return m_appletScriptEngine->include(path);
}

void AppletInterface::debug(const QString &msg)
{
    kDebug() << msg;
}

QObject *AppletInterface::findChild(const QString &name) const
{
    if (name.isEmpty()) {
        return 0;
    }

    foreach (QGraphicsItem *item, applet()->childItems()) {
        QGraphicsWidget *widget = dynamic_cast<QGraphicsWidget *>(item);
        if (widget && widget->objectName() == name) {
            return widget;
        }
    }

    return 0;
}

Plasma::Extender *AppletInterface::extender() const
{
    return m_appletScriptEngine->extender();
}

void AppletInterface::setAssociatedApplication(const QString &string)
{
    applet()->setAssociatedApplication(string);
}

QString AppletInterface::associatedApplication() const
{
    return applet()->associatedApplication();
}

void AppletInterface::setStatus(const AppletInterface::ItemStatus &status)
{
    applet()->setStatus((Plasma::ItemStatus)status);
}

AppletInterface::ItemStatus AppletInterface::status() const
{
    return (AppletInterface::ItemStatus)((int)(applet()->status()));
}

void AppletInterface::gc()
{
    QTimer::singleShot(0, m_appletScriptEngine, SLOT(collectGarbage()));
}


PopupAppletInterface::PopupAppletInterface(AbstractJsAppletScript *parent)
    : APPLETSUPERCLASS(parent)
{
}

void PopupAppletInterface::setPopupIcon(const QIcon &icon)
{
    popupApplet()->setPopupIcon(icon);
}

QIcon PopupAppletInterface::popupIcon()
{
    return popupApplet()->popupIcon();
}

void PopupAppletInterface::setPopupIconByName(const QString &name)
{
    return popupApplet()->setPopupIcon(name);
}

void PopupAppletInterface::setPassivePopup(bool passive)
{
    popupApplet()->setPassivePopup(passive);
}

bool PopupAppletInterface::isPassivePopup() const
{
    return popupApplet()->isPassivePopup();
}

void PopupAppletInterface::togglePopup()
{
    popupApplet()->togglePopup();
}

void PopupAppletInterface::hidePopup()
{
    popupApplet()->hidePopup();
}

void PopupAppletInterface::showPopup()
{
    popupApplet()->showPopup();
}

void PopupAppletInterface::setPopupWidget(QGraphicsWidget *widget)
{
    popupApplet()->setGraphicsWidget(widget);
}

QGraphicsWidget *PopupAppletInterface::popupWidget()
{
    return popupApplet()->graphicsWidget();
}

ContainmentInterface::ContainmentInterface(AbstractJsAppletScript *parent)
    : APPLETSUPERCLASS(parent),
      m_movableApplets(true)
{
    connect(containment(), SIGNAL(appletRemoved(Plasma::Applet *)), this, SLOT(appletRemovedForward(Plasma::Applet *)));

    connect(containment(), SIGNAL(appletAdded(Plasma::Applet *, const QPointF &)), this, SLOT(appletAddedForward(Plasma::Applet *, const QPointF &)));

    connect(containment(), SIGNAL(screenChanged(int, int, Plasma::Containment)), this, SLOT(screenChanged()));

    connect(containment()->context(), SIGNAL(activityChanged(Plasma::Context *)), this, SIGNAL(activityNameChanged()));
    connect(containment()->context(), SIGNAL(changed(Plasma::Context *)), this, SIGNAL(activityIdChanged()));

     if (containment()->corona()) {
         connect(containment()->corona(), SIGNAL(availableScreenRegionChanged()),
                 this, SIGNAL(availableScreenRegionChanged()));
     }
}

QScriptValue ContainmentInterface::applets()
{
    QScriptValue list = m_appletScriptEngine->engine()->newArray(containment()->applets().size());
    int i = 0;
    foreach (Plasma::Applet *applet, containment()->applets()) {
        list.setProperty(i, m_appletScriptEngine->engine()->newQObject(applet));
        ++i;
    }
    return list;
}

void ContainmentInterface::setDrawWallpaper(bool drawWallpaper)
{
   m_appletScriptEngine->setDrawWallpaper(drawWallpaper);
}

bool ContainmentInterface::drawWallpaper()
{
    return m_appletScriptEngine->drawWallpaper();
}

ContainmentInterface::Type ContainmentInterface::containmentType() const
{
    return (ContainmentInterface::Type)m_appletScriptEngine->containmentType();
}

void ContainmentInterface::setContainmentType(ContainmentInterface::Type type)
{
    m_appletScriptEngine->setContainmentType((Plasma::Containment::Type)type);
}

int ContainmentInterface::screen() const
{
    return containment()->screen();
}

QScriptValue ContainmentInterface::screenGeometry(int id) const
{
    QRectF rect;
    if (containment()->corona()) {
        rect = QRectF(containment()->corona()->screenGeometry(id));
    }

    QScriptValue val = m_appletScriptEngine->engine()->newObject();
    val.setProperty("x", rect.x());
    val.setProperty("y", rect.y());
    val.setProperty("width", rect.width());
    val.setProperty("height", rect.height());
    return val;
}

QScriptValue ContainmentInterface::availableScreenRegion(int id) const
{
    QRegion reg;
    if (containment()->corona()) {
        reg = containment()->corona()->availableScreenRegion(id);
    }

    QScriptValue regVal = m_appletScriptEngine->engine()->newArray(reg.rects().size());
    int i = 0;
    foreach (QRect rect, reg.rects()) {
        QScriptValue val = m_appletScriptEngine->engine()->newObject();
        val.setProperty("x", rect.x());
        val.setProperty("y", rect.y());
        val.setProperty("width", rect.width());
        val.setProperty("height", rect.height());
        regVal.setProperty(i++, val);
    }
    return regVal;
}

void ContainmentInterface::appletAddedForward(Plasma::Applet *applet, const QPointF &pos)
{
    applet->setFlag(QGraphicsItem::ItemIsMovable, m_movableApplets);
    emit appletAdded(applet, pos);
}

void ContainmentInterface::appletRemovedForward(Plasma::Applet *applet)
{
    applet->setFlag(QGraphicsItem::ItemIsMovable, true);
    emit appletRemoved(applet);
}

void ContainmentInterface::setMovableApplets(bool movable)
{
    if (m_movableApplets == movable) {
        return;
    }

    m_movableApplets = movable;

    foreach (Plasma::Applet *applet, containment()->applets()) {
        applet->setFlag(QGraphicsItem::ItemIsMovable, movable);
    }
}

bool ContainmentInterface::hasMovableApplets() const
{
    return m_movableApplets;
}

QString ContainmentInterface::activityName() const
{
    return containment()->context()->currentActivity();
}

QString ContainmentInterface::activityId() const
{
    return containment()->context()->currentActivityId();
}

#ifndef USE_JS_SCRIPTENGINE
#include "appletinterface.moc"
#endif
