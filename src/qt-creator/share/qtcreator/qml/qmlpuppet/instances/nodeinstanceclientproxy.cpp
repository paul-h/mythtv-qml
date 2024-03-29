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

#include "nodeinstanceclientproxy.h"

#include <QLocalSocket>
#include <QVariant>
#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>

#include "nodeinstanceserverinterface.h"

#include "propertyabstractcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeauxiliarycommand.h"
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "completecomponentcommand.h"
#include "synchronizecommand.h"
#include "removesharedmemorycommand.h"
#include "tokencommand.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"
#include "changenodesourcecommand.h"
#include "endpuppetcommand.h"
#include "debugoutputcommand.h"

namespace QmlDesigner {

NodeInstanceClientProxy::NodeInstanceClientProxy(QObject *parent)
    : QObject(parent),
      m_inputIoDevice(0),
      m_outputIoDevice(0),
      m_nodeInstanceServer(0),
      m_writeCommandCounter(0),
      m_synchronizeId(-1)
{
}

void NodeInstanceClientProxy::initializeSocket()
{
    QLocalSocket *localSocket = new QLocalSocket(this);
    connect(localSocket, SIGNAL(readyRead()), this, SLOT(readDataStream()));
    connect(localSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), QCoreApplication::instance(), SLOT(quit()));
    connect(localSocket, SIGNAL(disconnected()), QCoreApplication::instance(), SLOT(quit()));
    localSocket->connectToServer(QCoreApplication::arguments().at(1), QIODevice::ReadWrite | QIODevice::Unbuffered);
    localSocket->waitForConnected(-1);

    m_inputIoDevice = localSocket;
    m_outputIoDevice = localSocket;
}

void NodeInstanceClientProxy::initializeCapturedStream(const QString &fileName)
{
    m_inputIoDevice = new QFile(fileName, this);
    bool inputStreamCanBeOpened = m_inputIoDevice->open(QIODevice::ReadOnly);
    if (!inputStreamCanBeOpened) {
        qDebug() << "Input stream file cannot be opened: " << fileName;
        exit(-1);
    }

    if (QCoreApplication::arguments().count() == 3) {
        QFileInfo inputFileInfo(fileName);
        m_outputIoDevice = new QFile(inputFileInfo.path()+ "/" + inputFileInfo.baseName() + ".commandcontrolstream", this);
        bool outputStreamCanBeOpened = m_outputIoDevice->open(QIODevice::WriteOnly);
        if (!outputStreamCanBeOpened) {
            qDebug() << "Output stream file cannot be opened";
            exit(-1);
        }
    } else if (QCoreApplication::arguments().count() == 4) {
        m_controlStream.setFileName(QCoreApplication::arguments().at(3));
        bool controlStreamCanBeOpened = m_controlStream.open(QIODevice::ReadOnly);
        if (!controlStreamCanBeOpened) {
            qDebug() << "Control stream file cannot be opened";
            exit(-1);
        }
    }

}

bool compareCommands(const QVariant &command, const QVariant &controlCommand)
{
    static const int informationChangedCommandType = QMetaType::type("InformationChangedCommand");
    static const int valuesChangedCommandType = QMetaType::type("ValuesChangedCommand");
    static const int pixmapChangedCommandType = QMetaType::type("PixmapChangedCommand");
    static const int childrenChangedCommandType = QMetaType::type("ChildrenChangedCommand");
    static const int statePreviewImageChangedCommandType = QMetaType::type("StatePreviewImageChangedCommand");
    static const int componentCompletedCommandType = QMetaType::type("ComponentCompletedCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int debugOutputCommandType = QMetaType::type("DebugOutputCommand");

    if (command.userType() == controlCommand.userType()) {
        if (command.userType() == informationChangedCommandType)
            return command.value<InformationChangedCommand>() == controlCommand.value<InformationChangedCommand>();
        else if (command.userType() == valuesChangedCommandType)
            return command.value<ValuesChangedCommand>() == controlCommand.value<ValuesChangedCommand>();
         else if (command.userType() == pixmapChangedCommandType)
            return command.value<PixmapChangedCommand>() == controlCommand.value<PixmapChangedCommand>();
        else if (command.userType() == childrenChangedCommandType)
            return command.value<ChildrenChangedCommand>() == controlCommand.value<ChildrenChangedCommand>();
        else if (command.userType() == statePreviewImageChangedCommandType)
            return command.value<StatePreviewImageChangedCommand>() == controlCommand.value<StatePreviewImageChangedCommand>();
        else if (command.userType() == componentCompletedCommandType)
            return command.value<ComponentCompletedCommand>() == controlCommand.value<ComponentCompletedCommand>();
        else if (command.userType() == synchronizeCommandType)
            return command.value<SynchronizeCommand>() == controlCommand.value<SynchronizeCommand>();
        else if (command.userType() == tokenCommandType)
            return command.value<TokenCommand>() == controlCommand.value<TokenCommand>();
        else if (command.userType() == debugOutputCommandType)
            return command.value<DebugOutputCommand>() == controlCommand.value<DebugOutputCommand>();
    }

    return false;
}

void NodeInstanceClientProxy::writeCommand(const QVariant &command)
{
    if (m_controlStream.isReadable()) {
        static quint32 readCommandCounter = 0;
        static quint32 blockSize = 0;

        QVariant controlCommand = readCommandFromIOStream(&m_controlStream, &readCommandCounter, &blockSize);

        if (!compareCommands(command, controlCommand)) {
            qDebug() << "Commands differ!";
            exit(-1);
        }
    } else if (m_outputIoDevice) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_8);
        out << quint32(0);
        out << quint32(m_writeCommandCounter);
        m_writeCommandCounter++;
        out << command;
        out.device()->seek(0);
        out << quint32(block.size() - sizeof(quint32));

        m_outputIoDevice->write(block);
    }
}

void NodeInstanceClientProxy::informationChanged(const InformationChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::valuesChanged(const ValuesChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::pixmapChanged(const PixmapChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::childrenChanged(const ChildrenChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::statePreviewImagesChanged(const StatePreviewImageChangedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::componentCompleted(const ComponentCompletedCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::token(const TokenCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::debugOutput(const DebugOutputCommand &command)
{
    writeCommand(QVariant::fromValue(command));
}

void NodeInstanceClientProxy::flush()
{
}

void NodeInstanceClientProxy::synchronizeWithClientProcess()
{
    if (m_synchronizeId >= 0) {
        SynchronizeCommand synchronizeCommand(m_synchronizeId);
        writeCommand(QVariant::fromValue(synchronizeCommand));
    }
}

qint64 NodeInstanceClientProxy::bytesToWrite() const
{
    return m_inputIoDevice->bytesToWrite();
}

QVariant NodeInstanceClientProxy::readCommandFromIOStream(QIODevice *ioDevice, quint32 *readCommandCounter, quint32 *blockSize)
{



    QDataStream in(ioDevice);
    in.setVersion(QDataStream::Qt_4_8);

    if (*blockSize == 0) {
        in >> *blockSize;
    }

    if (ioDevice->bytesAvailable() < *blockSize)
        return QVariant();

    quint32 commandCounter;
    in >> commandCounter;
    bool commandLost = !((commandCounter == 0 && *readCommandCounter == 0) || (*readCommandCounter + 1 == commandCounter));
    if (commandLost)
        qDebug() << "client command lost: " << *readCommandCounter <<  commandCounter;
    *readCommandCounter = commandCounter;

    QVariant command;
    in >> command;
    *blockSize = 0;

    if (in.status() != QDataStream::Ok) {
        qWarning() << "Stream is no ok!!!";
        exit(1);
    }

    return command;
}

void NodeInstanceClientProxy::readDataStream()
{
    QList<QVariant> commandList;

    while (!m_inputIoDevice->atEnd()) {
        if (m_inputIoDevice->bytesAvailable() < int(sizeof(quint32)))
            break;

        static quint32 readCommandCounter = 0;
        static quint32 blockSize = 0;

        QVariant command = readCommandFromIOStream(m_inputIoDevice, &readCommandCounter, &blockSize);

        if (command.isValid())
            commandList.append(command);
        else
            break;
    }

    foreach (const QVariant &command, commandList) {
        dispatchCommand(command);
    }
}

NodeInstanceServerInterface *NodeInstanceClientProxy::nodeInstanceServer() const
{
    return m_nodeInstanceServer;
}

void NodeInstanceClientProxy::setNodeInstanceServer(NodeInstanceServerInterface *nodeInstanceServer)
{
    m_nodeInstanceServer = nodeInstanceServer;
}

void NodeInstanceClientProxy::createInstances(const CreateInstancesCommand &command)
{
    nodeInstanceServer()->createInstances(command);
}

void NodeInstanceClientProxy::changeFileUrl(const ChangeFileUrlCommand &command)
{
    nodeInstanceServer()->changeFileUrl(command);
}

void NodeInstanceClientProxy::createScene(const CreateSceneCommand &command)
{
    nodeInstanceServer()->createScene(command);
}

void NodeInstanceClientProxy::clearScene(const ClearSceneCommand &command)
{
    nodeInstanceServer()->clearScene(command);
}

void NodeInstanceClientProxy::removeInstances(const RemoveInstancesCommand &command)
{
    nodeInstanceServer()->removeInstances(command);
}

void NodeInstanceClientProxy::removeProperties(const RemovePropertiesCommand &command)
{
    nodeInstanceServer()->removeProperties(command);
}

void NodeInstanceClientProxy::changePropertyBindings(const ChangeBindingsCommand &command)
{
    nodeInstanceServer()->changePropertyBindings(command);
}

void NodeInstanceClientProxy::changePropertyValues(const ChangeValuesCommand &command)
{
    nodeInstanceServer()->changePropertyValues(command);
}

void NodeInstanceClientProxy::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    nodeInstanceServer()->changeAuxiliaryValues(command);
}

void NodeInstanceClientProxy::reparentInstances(const ReparentInstancesCommand &command)
{
    nodeInstanceServer()->reparentInstances(command);
}

void NodeInstanceClientProxy::changeIds(const ChangeIdsCommand &command)
{
    nodeInstanceServer()->changeIds(command);
}

void NodeInstanceClientProxy::changeState(const ChangeStateCommand &command)
{
    nodeInstanceServer()->changeState(command);
}

void NodeInstanceClientProxy::completeComponent(const CompleteComponentCommand &command)
{
    nodeInstanceServer()->completeComponent(command);
}

void NodeInstanceClientProxy::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    nodeInstanceServer()->changeNodeSource(command);
}

void NodeInstanceClientProxy::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    nodeInstanceServer()->removeSharedMemory(command);
}
void NodeInstanceClientProxy::redirectToken(const TokenCommand &command)
{
    nodeInstanceServer()->token(command);
}

void NodeInstanceClientProxy::redirectToken(const EndPuppetCommand & /*command*/)
{
    if (m_outputIoDevice && m_outputIoDevice->isOpen())
        m_outputIoDevice->close();

    if (m_inputIoDevice && m_inputIoDevice->isOpen())
        m_inputIoDevice->close();

    if (m_controlStream.isOpen())
        m_controlStream.close();

    qDebug() << "End Process: " << QCoreApplication::applicationPid();
    QCoreApplication::exit();
}

void NodeInstanceClientProxy::dispatchCommand(const QVariant &command)
{
    static const int createInstancesCommandType = QMetaType::type("CreateInstancesCommand");
    static const int changeFileUrlCommandType = QMetaType::type("ChangeFileUrlCommand");
    static const int createSceneCommandType = QMetaType::type("CreateSceneCommand");
    static const int clearSceneCommandType = QMetaType::type("ClearSceneCommand");
    static const int removeInstancesCommandType = QMetaType::type("RemoveInstancesCommand");
    static const int removePropertiesCommandType = QMetaType::type("RemovePropertiesCommand");
    static const int changeBindingsCommandType = QMetaType::type("ChangeBindingsCommand");
    static const int changeValuesCommandType = QMetaType::type("ChangeValuesCommand");
    static const int changeAuxiliaryCommandType = QMetaType::type("ChangeAuxiliaryCommand");
    static const int reparentInstancesCommandType = QMetaType::type("ReparentInstancesCommand");
    static const int changeIdsCommandType = QMetaType::type("ChangeIdsCommand");
    static const int changeStateCommandType = QMetaType::type("ChangeStateCommand");
    static const int completeComponentCommandType = QMetaType::type("CompleteComponentCommand");
    static const int synchronizeCommandType = QMetaType::type("SynchronizeCommand");
    static const int changeNodeSourceCommandType = QMetaType::type("ChangeNodeSourceCommand");
    static const int removeSharedMemoryCommandType = QMetaType::type("RemoveSharedMemoryCommand");
    static const int tokenCommandType = QMetaType::type("TokenCommand");
    static const int endPuppetCommandType = QMetaType::type("EndPuppetCommand");

    if (command.userType() ==  createInstancesCommandType)
        createInstances(command.value<CreateInstancesCommand>());
    else if (command.userType() ==  changeFileUrlCommandType)
        changeFileUrl(command.value<ChangeFileUrlCommand>());
    else if (command.userType() ==  createSceneCommandType)
        createScene(command.value<CreateSceneCommand>());
    else if (command.userType() ==  clearSceneCommandType)
        clearScene(command.value<ClearSceneCommand>());
    else if (command.userType() ==  removeInstancesCommandType)
        removeInstances(command.value<RemoveInstancesCommand>());
    else if (command.userType() ==  removePropertiesCommandType)
        removeProperties(command.value<RemovePropertiesCommand>());
    else if (command.userType() ==  changeBindingsCommandType)
        changePropertyBindings(command.value<ChangeBindingsCommand>());
    else if (command.userType() ==  changeValuesCommandType)
        changePropertyValues(command.value<ChangeValuesCommand>());
    else if (command.userType() ==  changeAuxiliaryCommandType)
        changeAuxiliaryValues(command.value<ChangeAuxiliaryCommand>());
    else if (command.userType() ==  reparentInstancesCommandType)
        reparentInstances(command.value<ReparentInstancesCommand>());
    else if (command.userType() ==  changeIdsCommandType)
        changeIds(command.value<ChangeIdsCommand>());
    else if (command.userType() ==  changeStateCommandType)
        changeState(command.value<ChangeStateCommand>());
    else if (command.userType() ==  completeComponentCommandType)
        completeComponent(command.value<CompleteComponentCommand>());
    else if (command.userType() ==  changeNodeSourceCommandType)
        changeNodeSource(command.value<ChangeNodeSourceCommand>());
    else if (command.userType() == removeSharedMemoryCommandType)
        removeSharedMemory(command.value<RemoveSharedMemoryCommand>());
    else if (command.userType() ==  tokenCommandType)
        redirectToken(command.value<TokenCommand>());
    else if (command.userType() ==  endPuppetCommandType)
        redirectToken(command.value<EndPuppetCommand>());
    else if (command.userType() == synchronizeCommandType) {
        SynchronizeCommand synchronizeCommand = command.value<SynchronizeCommand>();
        m_synchronizeId = synchronizeCommand.synchronizeId();
    } else {
        Q_ASSERT(false);
    }
}
} // namespace QmlDesigner
