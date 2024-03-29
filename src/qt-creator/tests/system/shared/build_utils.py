#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

import re;

# dictionary to hold a list of all installed handler functions for all object-signalSignature pairs
installedSignalHandlers = {}
# flag to indicate whether overrideInstallLazySignalHandler() has been called already
overridenInstallLazySignalHandlers = False
# flag to indicate whether a tasks file should be created when building ends with errors
createTasksFileOnError = True
# currently used directory for tasks files
tasksFileDir = None
# counter for written tasks files
tasksFileCount = 0

# call this function to override installLazySignalHandler()
def overrideInstallLazySignalHandler():
    global overridenInstallLazySignalHandlers
    if overridenInstallLazySignalHandlers:
        return
    overridenInstallLazySignalHandlers = True
    global installLazySignalHandler
    installLazySignalHandler = __addSignalHandlerDict__(installLazySignalHandler)

# avoids adding a handler to a signal twice or more often
# do not call this function directly - use overrideInstallLazySignalHandler() instead
def __addSignalHandlerDict__(lazySignalHandlerFunction):
    global installedSignalHandlers
    def wrappedFunction(name, signalSignature, handlerFunctionName):
        handlers = installedSignalHandlers.get("%s____%s" % (name,signalSignature))
        if handlers == None:
            lazySignalHandlerFunction(name, signalSignature, handlerFunctionName)
            installedSignalHandlers.setdefault("%s____%s" % (name,signalSignature), [handlerFunctionName])
        else:
            if not handlerFunctionName in handlers:
                lazySignalHandlerFunction(name, signalSignature, handlerFunctionName)
                handlers.append(handlerFunctionName)
                installedSignalHandlers.setdefault("%s____%s" % (name,signalSignature), handlers)
    return wrappedFunction

# returns the currently assigned handler functions for a given object and signal
def getInstalledSignalHandlers(name, signalSignature):
    return installedSignalHandlers.get("%s____%s" % (name,signalSignature))

# this method checks the last build (if there's one) and logs the number of errors, warnings and
# lines within the Issues output
# optional parameter can be used to tell this function if the build was expected to fail or not
def checkLastBuild(expectedToFail=False):
    try:
        # can't use waitForObject() 'cause visible is always 0
        buildProg = findObject("{type='ProjectExplorer::Internal::BuildProgress' unnamed='1' }")
    except LookupError:
        test.log("checkLastBuild called without a build")
        return
    ensureChecked(":Qt Creator_Issues_Core::Internal::OutputPaneToggleButton")
    model = waitForObject(":Qt Creator.Issues_QListView").model()
    buildIssues = dumpBuildIssues(model)
    errors = len(filter(lambda i: i[5] == "1", buildIssues))
    warnings = len(filter(lambda i: i[5] == "2", buildIssues))
    gotErrors = errors != 0
    if not (gotErrors ^ expectedToFail):
        test.passes("Errors: %s | Warnings: %s" % (errors, warnings))
    else:
        test.fail("Errors: %s | Warnings: %s" % (errors, warnings))
    # additional stuff - could be removed... or improved :)
    test.log("Rows inside issues: %d" % model.rowCount())
    if gotErrors and createTasksFileOnError:
        createTasksFile(buildIssues)
    return not gotErrors

# helper function to check the compilation when build wasn't successful
def checkCompile():
    ensureChecked(":Qt Creator_CompileOutput_Core::Internal::OutputPaneToggleButton")
    output = waitForObject(":Qt Creator.Compile Output_Core::OutputWindow")
    waitFor("len(str(output.plainText))>0",5000)
    if compileSucceeded(output.plainText):
        if os.getenv("SYSTEST_DEBUG") == "1":
            test.log("Compile Output:\n%s" % output.plainText)
        test.passes("Compile successful")
        return True
    else:
        test.fail("Compile Output:\n%s" % output.plainText)
        return False

def compileSucceeded(compileOutput):
    return None != re.match(".*exited normally\.\n\d\d:\d\d:\d\d: Elapsed time: "
                            "(\d:)?\d{2}:\d\d\.$", str(compileOutput), re.S)

def dumpBuildIssues(listModel):
    issueDump = []
    for row in range(listModel.rowCount()):
        index = listModel.index(row, 0)
        issueDump.extend([map(lambda role: index.data(role).toString(),
                              range(Qt.UserRole, Qt.UserRole + 6))])
    return issueDump

# helper method that writes a tasks file
def createTasksFile(buildIssues):
    global tasksFileDir, tasksFileCount
    if tasksFileDir == None:
            tasksFileDir = os.getcwd() + "/tasks"
            tasksFileDir = os.path.abspath(tasksFileDir)
    if not os.path.exists(tasksFileDir):
        try:
            os.makedirs(tasksFileDir)
        except OSError:
            test.log("Could not create %s - falling back to a temporary directory" % tasksFileDir)
            tasksFileDir = tempDir()

    tasksFileCount += 1
    outfile = os.path.join(tasksFileDir, os.path.basename(squishinfo.testCase)+"_%d.tasks" % tasksFileCount)
    file = codecs.open(outfile, "w", "utf-8")
    test.log("Writing tasks file - can take some time (according to number of issues)")
    rows = len(buildIssues)
    if os.environ.get("SYSTEST_DEBUG") == "1":
        firstrow = 0
    else:
        firstrow = max(0, rows - 100)
    for issue in buildIssues[firstrow:rows]:
        # the following is currently a bad work-around
        fData = issue[0] # file
        lData = issue[1] # line -> linenumber or empty
        tData = issue[5] # type -> 1==error 2==warning
        dData = issue[3] # description
        if lData == "":
            lData = "-1"
        if tData == "1":
            tData = "error"
        elif tData == "2":
            tData = "warning"
        else:
            tData = "unknown"
        file.write("%s\t%s\t%s\t%s\n" % (fData, lData, tData, dData))
    file.close()
    test.log("Written tasks file %s" % outfile)

# returns a list of pairs each containing the zero based number of a kit
# and the name of the matching build configuration
# param kitCount specifies the number of kits currently defined (must be correct!)
# param filter is a regular expression to filter the configuration by their name
def iterateBuildConfigs(kitCount, filter = ""):
    switchViewTo(ViewConstants.PROJECTS)
    configs = []
    for currentKit in range(kitCount):
        switchToBuildOrRunSettingsFor(kitCount, currentKit, ProjectSettings.BUILD)
        model = waitForObject(":scrollArea.Edit build configuration:_QComboBox").model()
        prog = re.compile(filter)
        # for each row in the model, write its data to a list
        configNames = dumpItems(model)
        # pick only those configuration names which pass the filter
        configs += zip([currentKit] * len(configNames),
                       [config for config in configNames if prog.match(config)])
    switchViewTo(ViewConstants.EDIT)
    return configs

# selects a build configuration for building the current project
# param targetCount specifies the number of targets currently defined (must be correct!)
# param currentTarget specifies the target for which to switch into the specified settings (zero based index)
# param configName is the name of the configuration that should be selected
# returns information about the selected kit, see getQtInformationForBuildSettings
def selectBuildConfig(targetCount, currentTarget, configName):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    if selectFromCombo(":scrollArea.Edit build configuration:_QComboBox", configName):
        progressBarWait(30000)
    return getQtInformationForBuildSettings(targetCount, True, ViewConstants.EDIT)

# This will not trigger a rebuild. If needed, caller has to do this.
def verifyBuildConfig(targetCount, currentTarget, shouldBeDebug=False, enableShadowBuild=False, enableQmlDebug=False):
    switchViewTo(ViewConstants.PROJECTS)
    switchToBuildOrRunSettingsFor(targetCount, currentTarget, ProjectSettings.BUILD)
    ensureChecked(waitForObject(":scrollArea.Details_Utils::DetailsButton"))
    ensureChecked("{name='shadowBuildCheckBox' type='QCheckBox' visible='1'}", enableShadowBuild)
    buildCfCombo = waitForObject("{type='QComboBox' name='buildConfigurationComboBox' visible='1' "
                                 "window=':Qt Creator_Core::Internal::MainWindow'}")
    if shouldBeDebug:
        test.compare(buildCfCombo.currentText, 'Debug', "Verifying whether it's a debug build")
    else:
        test.compare(buildCfCombo.currentText, 'Release', "Verifying whether it's a release build")
    try:
        libLabel = waitForObject(":scrollArea.Library not available_QLabel", 2000)
        mouseClick(libLabel, libLabel.width - 10, libLabel.height / 2, 0, Qt.LeftButton)
    except:
        pass
    # Since waitForObject waits for the object to be enabled,
    # it will wait here until compilation of the debug libraries has finished.
    qmlDebugCheckbox = waitForObject(":scrollArea.qmlDebuggingLibraryCheckBox_QCheckBox", 150000)
    if qmlDebugCheckbox.checked != enableQmlDebug:
        clickButton(qmlDebugCheckbox)
        # Don't rebuild now
        clickButton(waitForObject(":QML Debugging.No_QPushButton", 5000))
    try:
        problemFound = waitForObject("{window=':Qt Creator_Core::Internal::MainWindow' "
                                     "type='QLabel' name='problemLabel' visible='1'}", 1000)
        if problemFound:
            test.warning('%s' % problemFound.text)
    except:
        pass
    clickButton(waitForObject(":scrollArea.Details_Utils::DetailsButton"))
    switchViewTo(ViewConstants.EDIT)
