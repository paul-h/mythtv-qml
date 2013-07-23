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
import Ubuntu.Components.ListItems 0.1

/*!
    \qmltype SingleControl
    \inqmlmodule Ubuntu.Components.ListItems 0.1
    \ingroup ubuntu-listitems
    \brief A list item containing a single control

    Examples:
    \qml
        import "../components"
        import Ubuntu.Components.ListItems 0.1 as ListItem
        Column {
            ListItem.SingleControl {
                control: Button {
                    anchors {
                        margins: units.gu(1)
                        fill: parent
                    }
                    text: "Large button"
                }
            }
        }
    \endqml

    \b{This component is under heavy development.}
*/
// TODO: Add more examples when more types of controls become available.
Empty {
    id: singleControlListItem

    height: control ? control.height + control.anchors.topMargin + control.anchors.bottomMargin : undefined

    /*!
      \preliminary
      The control of this SingleControl list item.
      The control will automatically be re-parented to, and centered in, this list item.
     */
    property AbstractButton control

    /*! \internal */
    onClicked: control.clicked(mouse)
    pressed: __mouseArea.pressed || control.__mouseArea.pressed
    /*! \internal */
    onPressedChanged: control.pressed = singleControlListItem.pressed

    /*!
      \internal
     */
    function __updateControl() {
        if (control) {
            control.parent = singleControlListItem;
            control.anchors.centerIn = singleControlListItem;
        }
    }

    /*!
      \internal
      This handler is an implementation detail. Mark as internal to prevent QDoc publishing it
     */
    onControlChanged: __updateControl()
}
