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
#ifndef MADDEDEVICETESTER_H
#define MADDEDEVICETESTER_H

#include <remotelinux/linuxdevicetester.h>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Madde {
namespace Internal {

class MaddeDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit MaddeDeviceTester(QObject *parent = 0);
    ~MaddeDeviceTester();

    void testDevice(const ProjectExplorer::IDevice::ConstPtr &deviceConfiguration);
    void stopTest();

private slots:
    void handleGenericTestFinished(ProjectExplorer::DeviceTester::TestResult result);
    void handleConnectionError();
    void handleProcessFinished(int exitStatus);

private:
    enum State { Inactive, GenericTest, QtTest, MadDeveloperTest, QmlToolingTest };

    void handleQtTestFinished(int exitStatus);
    void handleMadDeveloperTestFinished(int exitStatus);
    void handleQmlToolingTestFinished(int exitStatus);

    QString processedQtLibsList();
    void setFinished();

    RemoteLinux::GenericLinuxDeviceTester * const m_genericTester;
    State m_state;
    ProjectExplorer::DeviceTester::TestResult m_result;
    QSsh::SshRemoteProcessRunner *m_processRunner;
    ProjectExplorer::IDevice::ConstPtr m_deviceConfiguration;
};

} // namespace Internal
} // namespace Madde

#endif // MADDEDEVICETESTER_H