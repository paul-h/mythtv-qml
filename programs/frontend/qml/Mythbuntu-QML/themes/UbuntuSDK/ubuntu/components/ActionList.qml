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

/*!
    \qmltype ActionList
    \inqmlmodule Ubuntu.Components 0.1
    \ingroup ubuntu
    \brief List of \l Action items

    Examples: See \l Page.
*/

QtObject {
    id: list
    // internal objects using nested elements,
    // which isn't allowed by QtObject; this fix makes this possible
    /*!
      \internal
      Default property to allow adding of children.
      */
    default property alias children: list.__actionList

    /*!
      \internal
      Property list to allow adding of children.
      */
    property list<Action> __actionList
}
