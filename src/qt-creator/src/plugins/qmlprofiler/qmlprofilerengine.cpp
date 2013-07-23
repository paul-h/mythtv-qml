/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprofilerengine.h"

#include "localqmlprofilerrunner.h"

#include <analyzerbase/analyzermanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <utils/qtcassert.h>
#include <coreplugin/helpmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <qmlprojectmanager/qmlprojectplugin.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <qmldebug/qmloutputparser.h>

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QTcpServer>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

//
// QmlProfilerEnginePrivate
//

class QmlProfilerEngine::QmlProfilerEnginePrivate
{
public:
    QmlProfilerEnginePrivate(QmlProfilerEngine *qq, const AnalyzerStartParameters &sp) : q(qq), m_runner(0), sp(sp) {}
    ~QmlProfilerEnginePrivate() { delete m_runner; }

    bool attach(const QString &address, uint port);
    AbstractQmlProfilerRunner *createRunner(ProjectExplorer::RunConfiguration *runConfiguration,
                                            QObject *parent);

    QmlProfilerEngine *q;

    QmlProfilerStateManager *m_profilerState;

    AbstractQmlProfilerRunner *m_runner;
    QTimer m_noDebugOutputTimer;
    QmlDebug::QmlOutputParser m_outputParser;
    const AnalyzerStartParameters sp;
};

AbstractQmlProfilerRunner *
QmlProfilerEngine::QmlProfilerEnginePrivate::createRunner(ProjectExplorer::RunConfiguration *runConfiguration,
                                                          QObject *parent)
{
    AbstractQmlProfilerRunner *runner = 0;
    if (!runConfiguration) // attaching
        return 0;

    QmlProjectManager::QmlProjectRunConfiguration *rc1 =
                qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration);
    LocalApplicationRunConfiguration *rc2 =
                   qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    // Supports only local run configurations
    if (!rc1 && !rc2)
        return 0;

    ProjectExplorer::EnvironmentAspect *environment
            = runConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(environment, return 0);
    LocalQmlProfilerRunner::Configuration conf;
    if (rc1) {
        // This is a "plain" .qmlproject.
        conf.executable = rc1->observerPath();
        conf.executableArguments = rc1->viewerArguments();
        conf.workingDirectory = rc1->workingDirectory();
        conf.environment = environment->environment();
    } else {
        // FIXME: Check.
        conf.executable = rc2->executable();
        conf.executableArguments = rc2->commandLineArguments();
        conf.workingDirectory = rc2->workingDirectory();
        conf.environment = environment->environment();
    }
    const ProjectExplorer::IDevice::ConstPtr device =
            ProjectExplorer::DeviceKitInformation::device(runConfiguration->target()->kit());
    QTC_ASSERT(device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE, return 0);
    conf.port = sp.analyzerPort;
    runner = new LocalQmlProfilerRunner(conf, parent);
    return runner;
}

//
// QmlProfilerEngine
//

QmlProfilerEngine::QmlProfilerEngine(IAnalyzerTool *tool,
                                     const Analyzer::AnalyzerStartParameters &sp,
                                     ProjectExplorer::RunConfiguration *runConfiguration)
    : IAnalyzerEngine(tool, sp, runConfiguration)
    , d(new QmlProfilerEnginePrivate(this, sp))
{
    d->m_profilerState = 0;

    // Only wait 4 seconds for the 'Waiting for connection' on application ouput, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(4000);
    connect(&d->m_noDebugOutputTimer, SIGNAL(timeout()), this, SLOT(processIsRunning()));

    d->m_outputParser.setNoOutputText(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput());
    connect(&d->m_outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            this, SLOT(processIsRunning(quint16)));
    connect(&d->m_outputParser, SIGNAL(noOutputMessage()),
            this, SLOT(processIsRunning()));
    connect(&d->m_outputParser, SIGNAL(errorMessage(QString)),
            this, SLOT(wrongSetupMessageBox(QString)));
}

QmlProfilerEngine::~QmlProfilerEngine()
{
    if (d->m_profilerState && d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning)
        stop();
    delete d;
}

bool QmlProfilerEngine::start()
{
    QTC_ASSERT(d->m_profilerState, return false);

    if (d->m_runner) {
        delete d->m_runner;
        d->m_runner = 0;
    }

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStarting);

    if (QmlProjectManager::QmlProjectRunConfiguration *rc =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration())) {
        if (rc->observerPath().isEmpty()) {
            QmlProjectManager::QmlProjectPlugin::showQmlObserverToolWarning();
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
            AnalyzerManager::stopTool();
            return false;
        }
    }

    d->m_runner = d->createRunner(runConfiguration(), this);

    if (LocalQmlProfilerRunner *qmlRunner = qobject_cast<LocalQmlProfilerRunner *>(d->m_runner)) {
        if (!qmlRunner->hasExecutable()) {
            showNonmodalWarning(tr("No executable file to launch."));
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
            AnalyzerManager::stopTool();
            return false;
        }
    }

    if (d->m_runner) {
        connect(d->m_runner, SIGNAL(stopped()), this, SLOT(notifyRemoteFinished()));
        connect(d->m_runner, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
                this, SLOT(logApplicationMessage(QString,Utils::OutputFormat)));
        d->m_runner->start();
        d->m_noDebugOutputTimer.start();
    } else if (d->sp.startMode == StartQmlRemote) {
        d->m_noDebugOutputTimer.start();
    } else {
        emit processRunning(startParameters().analyzerPort);
    }

    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppRunning);
    emit starting(this);
    return true;
}

void QmlProfilerEngine::stop()
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopRequested);
        break;
    }
    case QmlProfilerStateManager::AppReadyToStop : {
        cancelProcess();
        break;
    }
    case QmlProfilerStateManager::AppDying :
        // valid, but no further action is needed
        break;
    default: {
        const QString message = QString::fromLatin1("Unexpected engine stop from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
    }
        break;
    }
}

void QmlProfilerEngine::notifyRemoteFinished(bool success)
{
    QTC_ASSERT(d->m_profilerState, return);

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppRunning : {
        if (success)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        else
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppKilled);
        AnalyzerManager::stopTool();

        emit finished();
        break;
    }
    case QmlProfilerStateManager::AppStopped :
    case QmlProfilerStateManager::AppKilled :
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        break;
    default: {
        const QString message = QString::fromLatin1("Process died unexpectedly from state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
}
        break;
    }
}

void QmlProfilerEngine::cancelProcess()
{
    QTC_ASSERT(d->m_profilerState, return);

    // no process to be canceled? (there might be multiple engines, but only one runs a process)
    if (!d->m_runner)
        return;

    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppReadyToStop : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppStopped);
        break;
    }
    case QmlProfilerStateManager::AppRunning : {
        d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
        break;
    }
    default: {
        const QString message = QString::fromLatin1("Unexpected process termination requested with state %1 in %2:%3")
            .arg(d->m_profilerState->currentStateAsString(), QString::fromLatin1(__FILE__), QString::number(__LINE__));
        qWarning("%s", qPrintable(message));
        return;
    }
    }

    if (d->m_runner)
        d->m_runner->stop();
    emit finished();
}

void QmlProfilerEngine::logApplicationMessage(const QString &msg, Utils::OutputFormat format)
{
    emit outputReceived(msg, format);
    d->m_outputParser.processOutput(msg);
}

void QmlProfilerEngine::wrongSetupMessageBox(const QString &errorMessage)
{
    QMessageBox *infoBox = new QMessageBox(Core::ICore::mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    //: %1 is detailed error message
    infoBox->setText(tr("Could not connect to the in-process QML debugger:\n%1")
                     .arg(errorMessage));
    infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Ok);
    infoBox->setModal(true);

    connect(infoBox, SIGNAL(finished(int)),
            this, SLOT(wrongSetupMessageBoxFinished(int)));

    infoBox->show();

    // KILL
    d->m_profilerState->setCurrentState(QmlProfilerStateManager::AppDying);
    AnalyzerManager::stopTool();
    emit finished();
}

void QmlProfilerEngine::wrongSetupMessageBoxFinished(int button)
{
    if (button == QMessageBox::Help) {
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                               "#setting-up-qml-debugging"));
    }
}

void QmlProfilerEngine::showNonmodalWarning(const QString &warningMsg)
{
    QMessageBox *noExecWarning = new QMessageBox(Core::ICore::mainWindow());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

void QmlProfilerEngine::notifyRemoteSetupDone(quint16 port)
{
    d->m_noDebugOutputTimer.stop();
    emit processRunning(port);
}

void QmlProfilerEngine::processIsRunning(quint16 port)
{
    d->m_noDebugOutputTimer.stop();

    if (port > 0)
        emit processRunning(port);
    else if (d->m_runner)
        emit processRunning(d->m_runner->debugPort());
}

////////////////////////////////////////////////////////////////
// Profiler State
void QmlProfilerEngine::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    // disconnect old
    if (d->m_profilerState)
        disconnect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));

    d->m_profilerState = profilerState;

    // connect
    if (d->m_profilerState)
        connect(d->m_profilerState, SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));
}

void QmlProfilerEngine::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppReadyToStop : {
        cancelProcess();
        break;
    }
    case QmlProfilerStateManager::Idle : {
        // When all the profiling is done, delete the profiler runner
        // (a new one will be created at start)
        d->m_noDebugOutputTimer.stop();
        if (d->m_runner) {
            delete d->m_runner;
            d->m_runner = 0;
        }
        break;
    }
    default:
        break;
    }
}

} // namespace Internal
} // namespace QmlProfiler
