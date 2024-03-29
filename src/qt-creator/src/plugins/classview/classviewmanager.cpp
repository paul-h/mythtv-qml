/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "classviewmanager.h"
#include "classviewsymbollocation.h"
#include "classviewnavigationwidgetfactory.h"
#include "classviewparser.h"
#include "classviewutils.h"

#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

namespace ClassView {
namespace Internal {

///////////////////////////////// ManagerPrivate //////////////////////////////////

// static variable initialization
static Manager *managerInstance = 0;

/*!
    \class ClassView::Internal::Manager

    \brief The Manager class implements a class view manager that interacts with
    other \QC plugins and acts as a proxy between them and the parser.

    The parser is moved to a separate thread and is connected to the manager by
    using signals and slots. Manager's signals starting with 'request' are for
    the parser.
*/

/*!
    \fn explicit ClassView::Internal::Manager(QObject *parent = 0)

    Creates a shared instance of a \a parent object.
*/

/*!
    \fn void ClassView::Internal::Manager::stateChanged(bool state)

    Changes the internal manager state. \a state returns true if manager is
    enabled, otherwise false.

    \sa setState, state
*/

/*!
    \fn void ClassView::Internal::Manager::treeDataUpdate(QSharedPointer<QStandardItem> result)

    Emits a signal about a tree data update (to tree view). \a result holds the
    item with the current tree.
*/

/*!
    \fn void ClassView::Internal::Manager::requestTreeDataUpdate()

    Emits a signal that a request for sending the tree view has to be handled by
    listeners (the parser).

    \sa onRequestTreeDataUpdate
*/

/*!
    \fn void ClassView::Internal::Manager::requestDocumentUpdated(CPlusPlus::Document::Ptr doc)

    Emits a signal that \a doc was updated and has to be reparsed.

    \sa onDocumentUpdated
*/

/*!
    \fn void ClassView::Internal::Manager::requestResetCurrentState()

    Emits a signal that the parser has to reset its internal state to the
    current state from the code manager.
*/

/*!
    \fn void ClassView::Internal::Manager::requestClearCache()

    Requests the parser to clear a cache.
*/

/*!
    \fn void ClassView::Internal::Manager::requestClearCacheAll()

    Requests the parser to clear a full cache.
*/

/*!
    \fn void ClassView::Internal::Manager::requestSetFlatMode(bool flat)

    Requests the manager to set the flat mode without subprojects. Set \a flat
    to \c true to enable flat mode and to false to disable it.
*/

/*!
   \class ManagerPrivate
   \internal
   \brief The ManagerPrivate class contains private class data for the Manager
    class.
   \sa Manager
*/

class ManagerPrivate
{
public:
    ManagerPrivate() : state(false), disableCodeParser(false) {}

    //! State mutex
    QMutex mutexState;

    //! code state/changes parser
    Parser parser;

    //! separate thread for the parser
    QThread parserThread;

    //! Internal manager state. \sa Manager::state
    bool state;

    //! there is some massive operation ongoing so temporary we should wait
    bool disableCodeParser;
};

///////////////////////////////// Manager //////////////////////////////////

Manager::Manager(QObject *parent)
    : QObject(parent),
    d(new ManagerPrivate())
{
    managerInstance = this;

    // register - to be able send between signal/slots
    qRegisterMetaType<QSharedPointer<QStandardItem> >("QSharedPointer<QStandardItem>");

    initialize();

    // start a separate thread for the parser
    d->parser.moveToThread(&d->parserThread);
    d->parserThread.start();

    // initial setup
    onProjectListChanged();
}

Manager::~Manager()
{
    d->parserThread.quit();
    d->parserThread.wait();
    delete d;
    managerInstance = 0;
}

Manager *Manager::instance()
{
    return managerInstance;
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
*/

bool Manager::canFetchMore(QStandardItem *item) const
{
    return d->parser.canFetchMore(item);
}

/*!
    Checks \a item for lazy data population of a QStandardItemModel.
    \a skipRoot item is needed for the manual update, call not from model.
*/

void Manager::fetchMore(QStandardItem *item, bool skipRoot)
{
    d->parser.fetchMore(item, skipRoot);
}

void Manager::initialize()
{
    // use Qt::QueuedConnection everywhere

    // widget factory signals
    connect(NavigationWidgetFactory::instance(), SIGNAL(widgetIsCreated()),
            SLOT(onWidgetIsCreated()), Qt::QueuedConnection);

    // internal manager state is changed
    connect(this, SIGNAL(stateChanged(bool)), SLOT(onStateChanged(bool)), Qt::QueuedConnection);

    // connections to enable/disable navi widget factory
    ProjectExplorer::SessionManager *sessionManager =
        ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            SLOT(onProjectListChanged()), Qt::QueuedConnection);
    connect(sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            SLOT(onProjectListChanged()), Qt::QueuedConnection);

    // connect to the progress manager for signals about Parsing tasks
    connect(Core::ICore::progressManager(), SIGNAL(taskStarted(QString)),
            SLOT(onTaskStarted(QString)), Qt::QueuedConnection);
    connect(Core::ICore::progressManager(), SIGNAL(allTasksFinished(QString)),
            SLOT(onAllTasksFinished(QString)), Qt::QueuedConnection);

    // when we signals that really document is updated - sent it to the parser
    connect(this, SIGNAL(requestDocumentUpdated(CPlusPlus::Document::Ptr)),
            &d->parser, SLOT(parseDocument(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);

    // translate data update from the parser to listeners
    connect(&d->parser, SIGNAL(treeDataUpdate(QSharedPointer<QStandardItem>)),
            this, SLOT(onTreeDataUpdate(QSharedPointer<QStandardItem>)), Qt::QueuedConnection);

    // requet current state - immediately after a notification
    connect(this, SIGNAL(requestTreeDataUpdate()),
            &d->parser, SLOT(requestCurrentState()), Qt::QueuedConnection);

    // full reset request to parser
    connect(this, SIGNAL(requestResetCurrentState()),
            &d->parser, SLOT(resetDataToCurrentState()), Qt::QueuedConnection);

    // clear cache request
    connect(this, SIGNAL(requestClearCache()),
            &d->parser, SLOT(clearCache()), Qt::QueuedConnection);

    // clear full cache request
    connect(this, SIGNAL(requestClearCacheAll()),
            &d->parser, SLOT(clearCacheAll()), Qt::QueuedConnection);

    // flat mode request
    connect(this, SIGNAL(requestSetFlatMode(bool)),
            &d->parser, SLOT(setFlatMode(bool)), Qt::QueuedConnection);

    // connect to the cpp model manager for signals about document updates
    CppTools::CppModelManagerInterface *codeModelManager
        = CppTools::CppModelManagerInterface::instance();

    // when code manager signals that document is updated - handle it by ourselves
    connect(codeModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)), Qt::QueuedConnection);
    //
    connect(codeModelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            &d->parser, SLOT(removeFiles(QStringList)), Qt::QueuedConnection);
}

/*!
    Gets the internal manager state. If it is disabled, does not emit signals
    about parsing requests. If enabled, does parsing in the background even if
    the navigation pane is not visible. Returns true if the manager is enabled,
    false otherwise.

   \sa setState, stateChanged
*/

bool Manager::state() const
{
    return d->state;
}

/*!
    Sets the internal manager \a state to \c true if the manager has to be
    enabled, otherwise sets it to \c false.

   \sa state, stateChanged
*/

void Manager::setState(bool state)
{
    QMutexLocker locker(&d->mutexState);

    // boolean comparsion - should be done correctly by any compiler
    if (state == d->state)
        return;

    d->state = state;

    emit stateChanged(d->state);
}

/*!
    Reacts to the widget factory creating a widget.

    \sa setState, state
*/

void Manager::onWidgetIsCreated()
{
    // do nothing - continue to sleep
}

/*!
    Reacts to the \a visibility of one navigation pane widget being changed
    (there might be a lot of them).

   \sa setState, state
*/

void Manager::onWidgetVisibilityIsChanged(bool visibility)
{
    // activate data handling - when 1st time 'Class View' will be open
    if (visibility)
        setState(true);
}

/*!
    Reacts to the state changed signal for the current manager \a state.
    For example, requests the currect code snapshot if needed.

    \sa setState, state, stateChanged
*/

void Manager::onStateChanged(bool state)
{
    if (state) {
        // enabled - request a current snapshots etc?..
        emit requestResetCurrentState();
    } else {
        // disabled - clean parsers internal cache
        emit requestClearCacheAll();
    }
}

/*!
    Reacts to the project list being changed by updating the navigation pane
    visibility if necessary.
*/

void Manager::onProjectListChanged()
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    // update to the latest state
    requestTreeDataUpdate();
}

/*!
    Handles parse tasks started by the progress manager. \a type holds the
    task index, which should be \c CppTools::Constants::TASK_INDEX.

   \sa CppTools::Constants::TASK_INDEX
*/

void Manager::onTaskStarted(const QString &type)
{
    if (type != QLatin1String(CppTools::Constants::TASK_INDEX))
        return;

    // disable tree updates to speed up
    d->disableCodeParser = true;
}

/*!
    Handles parse tasks finished by the progress manager.\a type holds the
    task index, which should be \c CppTools::Constants::TASK_INDEX.

   \sa CppTools::Constants::TASK_INDEX
*/

void Manager::onAllTasksFinished(const QString &type)
{
    if (type != QLatin1String(CppTools::Constants::TASK_INDEX))
        return;

    // parsing is finished, enable tree updates
    d->disableCodeParser = false;

    // do nothing if Manager is disabled
    if (!state())
        return;

    // any document might be changed, emit signal to clear cache
    emit requestClearCache();

    // request to update a tree to the current state
    emit requestResetCurrentState();
}

/*!
    Emits the signal \c documentUpdated when the code model manager state is
    changed for \a doc.

    \sa documentUpdated
*/

void Manager::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    // do nothing if updates are disabled
    if (d->disableCodeParser)
        return;

    emit requestDocumentUpdated(doc);
}

/*!
    Opens the text editor for the file \a fileName on \a line (1-based) and
    \a column (1-based).
*/

void Manager::gotoLocation(const QString &fileName, int line, int column)
{
    Core::EditorManager::openEditorAt(fileName, line, column);
}

/*!
    Opens the text editor for any of the symbol locations in the \a list.

   \sa Manager::gotoLocations
*/

void Manager::gotoLocations(const QList<QVariant> &list)
{
    QSet<SymbolLocation> locations = Utils::roleToLocations(list);

    if (locations.count() == 0)
        return;

    QString fileName;
    int line = 0;
    int column = 0;
    bool currentPositionAvailable = false;

    // what is open now?
    if (Core::IEditor *editor = Core::EditorManager::currentEditor()) {
        // get current file name
        if (Core::IDocument *document = editor->document())
            fileName = document->filePath();

        // if text file - what is current position?
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        if (textEditor) {
            // there is open currently text editor
            int position = textEditor->position();
            textEditor->convertPosition(position, &line, &column);
            currentPositionAvailable = true;
        }
    }

    // if there is something open - try to check, is it currently activated symbol?
    if (currentPositionAvailable) {
        SymbolLocation current(fileName, line, column);
        QSet<SymbolLocation>::const_iterator it = locations.find(current);
        QSet<SymbolLocation>::const_iterator end = locations.constEnd();
        // is it known location?
        if (it != end) {
            // found - do one additional step
            ++it;
            if (it == end)
                it = locations.begin();
            const SymbolLocation &found = *it;
            gotoLocation(found.fileName(), found.line(), found.column());
            return;
        }
    }

    // no success - open first item in the list
    const SymbolLocation loc = *locations.constBegin();

    gotoLocation(loc.fileName(), loc.line(), loc.column());
}

/*!
    Emits the signal \c requestTreeDataUpdate if the latest tree info is
    requested and if parsing is enabled.

   \sa requestTreeDataUpdate, NavigationWidget::requestDataUpdate
*/

void Manager::onRequestTreeDataUpdate()
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    emit requestTreeDataUpdate();
}

/*!
    Switches to flat mode (without subprojects) if \a flat is set to \c true.
*/

void Manager::setFlatMode(bool flat)
{
    emit requestSetFlatMode(flat);
}

/*!
    Sends a new tree data update to a tree view. \a result holds the item with
    the current tree.
*/

void Manager::onTreeDataUpdate(QSharedPointer<QStandardItem> result)
{
    // do nothing if Manager is disabled
    if (!state())
        return;

    emit treeDataUpdate(result);
}

} // namespace Internal
} // namespace ClassView
