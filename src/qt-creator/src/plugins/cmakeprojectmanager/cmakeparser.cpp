/****************************************************************************
**
** Copyright (C) 2013 Axonian LLC.
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

#include "cmakeparser.h"

#include <utils/qtcassert.h>

#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace CMakeProjectManager;
using namespace Internal;
using namespace ProjectExplorer;

namespace {
const char * const COMMON_ERROR_PATTERN = "^CMake Error at (.*):([0-9]*) \\((.*)\\):";
const char * const NEXT_SUBERROR_PATTERN = "^CMake Error in (.*):";
}

CMakeParser::CMakeParser() :
    m_skippedFirstEmptyLine(false)
{
    m_commonError.setPattern(QLatin1String(COMMON_ERROR_PATTERN));
    m_commonError.setMinimal(true);
    QTC_CHECK(m_commonError.isValid());

    m_nextSubError.setPattern(QLatin1String(NEXT_SUBERROR_PATTERN));
    m_nextSubError.setMinimal(true);
    QTC_CHECK(m_nextSubError.isValid());
    appendOutputParser(new GnuMakeParser());
}

void CMakeParser::stdError(const QString &line)
{
    QString trimmedLine = rightTrimmed(line);
    if (trimmedLine.isEmpty() && !m_lastTask.isNull()) {
        if (m_skippedFirstEmptyLine) {
            doFlush();
        } else {
            m_skippedFirstEmptyLine = true;
        }
        return;
    }
    if (m_skippedFirstEmptyLine)
        m_skippedFirstEmptyLine= false;

    if (m_commonError.indexIn(trimmedLine) != -1) {
        m_lastTask = Task(Task::Error, QString(), Utils::FileName::fromUserInput(m_commonError.cap(1)),
                          m_commonError.cap(2).toInt(), Core::Id(Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (m_nextSubError.indexIn(trimmedLine) != -1) {
        m_lastTask = Task(Task::Error, QString(), Utils::FileName::fromUserInput(m_nextSubError.cap(1)), -1,
                          Core::Id(Constants::TASK_CATEGORY_COMPILE));
        return;
    } else if (trimmedLine.startsWith(QLatin1String("  ")) && !m_lastTask.isNull()) {
        if (!m_lastTask.description.isEmpty())
            m_lastTask.description.append(QLatin1Char(' '));
        m_lastTask.description.append(trimmedLine.trimmed());
        return;
    }

    IOutputParser::stdError(line);
}

void CMakeParser::doFlush()
{
    if (m_lastTask.isNull())
        return;
    Task t = m_lastTask;
    m_lastTask.clear();
    emit addTask(t);
}

#ifdef WITH_TESTS
#include "cmakeprojectplugin.h"

#include <projectexplorer/outputparser_test.h>

#include <QTest>

void CMakeProjectPlugin::testCMakeParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<ProjectExplorer::Task> >("tasks");
    QTest::addColumn<QString>("outputLines");

    const Core::Id categoryCompile = Core::Id(Constants::TASK_CATEGORY_COMPILE);

    // negative tests
    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<ProjectExplorer::Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<ProjectExplorer::Task>()
            << QString();

    // positive tests
    QTest::newRow("add custom target")
            << QString::fromLatin1("CMake Error at src/1/app/CMakeLists.txt:70 (add_custom_target):\n"
                                   "  Cannot find source file:\n\n"
                                   "    unknownFile.qml\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n\n"
                                   "CMake Error in src/1/app/CMakeLists.txt:\n"
                                   "  Cannot find source file:\n\n"
                                   "    CMakeLists.txt2\n\n"
                                   "  Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp\n"
                                   "  .hxx .in .txx\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Cannot find source file: unknownFile.qml Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx"),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/app/CMakeLists.txt")), 70,
                        categoryCompile)
            << Task(Task::Error,
                    QLatin1String("Cannot find source file: CMakeLists.txt2 Tried extensions .c .C .c++ .cc .cpp .cxx .m .M .mm .h .hh .h++ .hm .hpp .hxx .in .txx"),
                    Utils::FileName::fromUserInput(QLatin1String("src/1/app/CMakeLists.txt")), -1,
                    categoryCompile))
            << QString();

    QTest::newRow("add subdirectory")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (add_subdirectory):\n"
                                   "  add_subdirectory given source \"app1\" which is not an existing directory.\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("add_subdirectory given source \"app1\" which is not an existing directory."),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryCompile))
            << QString();

    QTest::newRow("unknown command")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (i_am_wrong_command):\n"
                                   "  Unknown CMake command \"i_am_wrong_command\".\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("Unknown CMake command \"i_am_wrong_command\"."),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryCompile))
            << QString();

    QTest::newRow("incorrect arguments")
            << QString::fromLatin1("CMake Error at src/1/CMakeLists.txt:8 (message):\n"
                                   "  message called with incorrect number of arguments\n\n")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<ProjectExplorer::Task>()
                << Task(Task::Error,
                        QLatin1String("message called with incorrect number of arguments"),
                        Utils::FileName::fromUserInput(QLatin1String("src/1/CMakeLists.txt")), 8,
                        categoryCompile))
            << QString();
}

void CMakeProjectPlugin::testCMakeParser()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new CMakeParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}

#endif
