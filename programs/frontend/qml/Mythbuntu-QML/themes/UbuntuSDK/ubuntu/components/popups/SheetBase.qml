/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import "../components"
// FIXME: When a module contains QML, C++ and JavaScript elements exported,
// we need to use named imports otherwise namespace collision is reported
// by the QML engine. As workaround, we use Theming named import.
// Bug to watch: https://bugreports.qt-project.org/browse/QTBUG-27645
import "../components" as Theming

/*!
    \qmltype SheetBase
    \inqmlmodule Ubuntu.Components.Popups 0.1
    \ingroup ubuntu-popups
    \brief Parent class of different types of sheets. Not to be used directly.

    Examples: See subclasses.
    \b{This component is under heavy development.}
*/
PopupBase {
    id: sheet

    /*!
      \preliminary
      Content will be put inside the foreround of the sheet.
    */
    default property alias container: containerItem.data

    /*!
      Override the default width of the contents of the sheet.
      Total sheet width will be clamped between 50 grid units and the screen width.
      \qmlproperty real contentsWidth
     */
    property alias contentsWidth: foreground.contentsWidth

    /*!
      \preliminary
      Override the default height of the contents of the sheet.
      Total sheet height will be clamped between 40 grid units and the screen height.
      \qmlproperty real contentsHeight
     */
    property alias contentsHeight: foreground.contentsHeight

    /*!
      \preliminary
      The text shown in the header of the sheet.
      \qmlproperty string title
     */
    property alias title: foreground.title

    /*! \internal */
    property alias __leftButton: foreground.leftButton

    /*! \internal */
    property alias __rightButton: foreground.rightButton

    Background {
        dim: false
        dismissOnTap: false
    }

    Item {
        id: foreground

        // FIXME: see above
        Theming.ItemStyle.class: "sheet-foreground"

        property string title
        property real contentsWidth: units.gu(64)
        property real contentsHeight: units.gu(40)
        property Button leftButton
        property Button rightButton

        y: Math.min(units.gu(15), (sheet.height - height)/2)
        anchors.horizontalCenter: parent.horizontalCenter

        property real minWidth: Math.min(units.gu(50), sheet.width)
        property real maxWidth: sheet.width
        property real minHeight: Math.min(units.gu(40), sheet.height)
        property real maxHeight: sheet.height

        width: childrenRect.width
        height: childrenRect.height

        Item {
            id: containerItem
            anchors {
                fill: parent
                margins: units.gu(1)
            }
        }
    }
}
