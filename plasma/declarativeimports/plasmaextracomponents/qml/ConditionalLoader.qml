/*
    Copyright (C) 2012 Marco Martin <mart@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
import QtQuick 1.1

/**Documented API
Inherits:
        Item

Imports:
        QtQuick 1.1

Description:
    This item can load any qml component, just like a Loader. Important difference, the component will be only loaded when the "when" property is satisfied (ie evaluates to true) in this way it's easy to have big (and memory expensive) parts of the user interface load only when a certain condition is satisfied.
    For instance the contents of the tabs of a TabBar can be loaded only when they become the current page

Properties:
    bool when:
    Boolean condition that tells when to load the declarative component

    variant source:
    It can be a string with a path name of a qml file or a Component. It's the component that will be loaded when "when" is true. If the component changes the old instantiated component will be deleted and the new will be loaded instead.

    Item item: the item instantiated from component, if any.
**/
Item {
    id: root
    property alias when: loader.when
    property alias source: loader.conditionalSource
    property alias item: loader.item

    Loader {
        id: loader
        anchors.fill: parent

        property bool when: false
        property variant conditionalSource

        //internal
        property variant oldConditionalSource

        onWhenChanged: loadTimer.restart()
        onConditionalSourceChanged: loadTimer.restart()

        Timer {
            id: loadTimer
            interval: 0

            onTriggered: {
                if (loader.when &&
                    (loader.item === null ||
                     loader.conditionalSource !== loader.oldConditionalSource)) {
                    if (typeof(loader.conditionalSource) === "string") {
                        loader.source = loader.conditionalSource
                    } else {
                        loader.sourceComponent = loader.conditionalSource
                    }
                    loader.oldConditionalSource = loader.conditionalSource
                    loader.item.visible = true
                    loader.item.anchors.fill = loader.item.parent
                }
            }
        }
    }
}