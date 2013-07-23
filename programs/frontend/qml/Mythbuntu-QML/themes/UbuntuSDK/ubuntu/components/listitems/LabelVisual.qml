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

// internal helper class for text inside the list items.
Label {
    id: label
    property bool selected: false

    fontSize: "medium"
    elide: Text.ElideRight
    color: selected ? "#f37505" : Qt.rgba(0.4, 0.4, 0.4, 1.0)
    opacity: label.enabled ? 1.0 : 0.5
}
