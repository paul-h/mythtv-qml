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

#include "designmodewidget.h"
#include "styledoutputpaneplaceholder.h"
#include "qmldesignerplugin.h"

#include <rewriterview.h>
#include <itemlibrarywidget.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/fileutils.h>

#include <QSettings>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QTabWidget>

using Core::MiniSplitter;
using Core::IEditor;
using Core::EditorManager;

using namespace QmlDesigner;

enum {
    debug = false
};

const char SB_PROJECTS[] = "Projects";
const char SB_FILESYSTEM[] = "FileSystem";
const char SB_OPENDOCUMENTS[] = "OpenDocuments";

namespace QmlDesigner {
namespace Internal {

DocumentWarningWidget::DocumentWarningWidget(DesignModeWidget *parent) :
        Utils::FakeToolTip(parent),
        m_errorMessage(new QLabel(tr("Placeholder"), this)),
        m_goToError(new QLabel(this)),
        m_designModeWidget(parent)
{
    setWindowFlags(Qt::Widget); //We only want the visual style from a ToolTip
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setAutoFillBackground(true);

    m_errorMessage->setForegroundRole(QPalette::ToolTipText);
    m_goToError->setText(tr("<a href=\"goToError\">Go to error</a>"));
    m_goToError->setForegroundRole(QPalette::Link);
    connect(m_goToError, SIGNAL(linkActivated(QString)), this, SLOT(goToError()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(20);
    layout->setSpacing(5);
    layout->addWidget(m_errorMessage);
    layout->addWidget(m_goToError, 1, Qt::AlignRight);
}

void DocumentWarningWidget::setError(const RewriterView::Error &error)
{
    m_error = error;
    QString str;
    if (error.type() == RewriterView::Error::ParseError) {
        str = tr("%3 (%1:%2)").arg(QString::number(error.line()), QString::number(error.column()), error.description());
        m_goToError->show();
    }  else if (error.type() == RewriterView::Error::InternalError) {
        str = tr("Internal error (%1)").arg(error.description());
        m_goToError->hide();
    }

    m_errorMessage->setText(str);
    resize(layout()->totalSizeHint());
}

class ItemLibrarySideBarItem : public Core::SideBarItem
{
public:
    explicit ItemLibrarySideBarItem(QWidget *widget, const QString &id);
    virtual ~ItemLibrarySideBarItem();

    virtual QList<QToolButton *> createToolBarWidgets();
};

ItemLibrarySideBarItem::ItemLibrarySideBarItem(QWidget *widget, const QString &id) : Core::SideBarItem(widget, id) {}

ItemLibrarySideBarItem::~ItemLibrarySideBarItem()
{

}

QList<QToolButton *> ItemLibrarySideBarItem::createToolBarWidgets()
{
    return qobject_cast<ItemLibraryWidget*>(widget())->createToolBarWidgets();
}

class DesignerSideBarItem : public Core::SideBarItem
{
public:
    explicit DesignerSideBarItem(QWidget *widget, WidgetInfo::ToolBarWidgetFactoryInterface *createToolBarWidgets, const QString &id);
    virtual ~DesignerSideBarItem();

    virtual QList<QToolButton *> createToolBarWidgets();

private:
    WidgetInfo::ToolBarWidgetFactoryInterface *m_toolBarWidgetFactory;

};

DesignerSideBarItem::DesignerSideBarItem(QWidget *widget, WidgetInfo::ToolBarWidgetFactoryInterface *toolBarWidgetFactory, const QString &id)
    : Core::SideBarItem(widget, id) , m_toolBarWidgetFactory(toolBarWidgetFactory)
{
}

DesignerSideBarItem::~DesignerSideBarItem()
{
    delete m_toolBarWidgetFactory;
}

QList<QToolButton *> DesignerSideBarItem::createToolBarWidgets()
{
    if (m_toolBarWidgetFactory)
        return m_toolBarWidgetFactory->createToolBarWidgets();

    return QList<QToolButton *>();
}

void DocumentWarningWidget::goToError()
{
    m_designModeWidget->textEditor()->gotoLine(m_error.line(), m_error.column() - 1);
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget(QWidget *parent) :
    QWidget(parent),
    m_mainSplitter(0),
    m_isDisabled(false),
    m_showSidebars(true),
    m_initStatus(NotInitialized),
    m_warningWidget(0),
    m_navigatorHistoryCounter(-1),
    m_keepNavigatorHistory(false)
{
    m_outputPlaceholderSplitter = new Core::MiniSplitter;
    m_outputPanePlaceholder = new StyledOutputpanePlaceHolder(Core::DesignMode::instance(), m_outputPlaceholderSplitter);
}

DesignModeWidget::~DesignModeWidget()
{
}

void DesignModeWidget::restoreDefaultView()
{
    QSettings *settings = Core::ICore::settings();
    m_leftSideBar->closeAllWidgets();
    m_rightSideBar->closeAllWidgets();
    m_leftSideBar->readSettings(settings,  "none.LeftSideBar");
    m_rightSideBar->readSettings(settings, "none.RightSideBar");
    m_leftSideBar->show();
    m_rightSideBar->show();
}

void DesignModeWidget::toggleLeftSidebar()
{
    if (m_leftSideBar)
        m_leftSideBar->setVisible(!m_leftSideBar->isVisible());
}

void DesignModeWidget::toggleRightSidebar()
{
    if (m_rightSideBar)
        m_rightSideBar->setVisible(!m_rightSideBar->isVisible());
}

void DesignModeWidget::toggleSidebars()
{
    if (m_initStatus == Initializing)
        return;

    m_showSidebars = !m_showSidebars;

    if (m_leftSideBar)
        m_leftSideBar->setVisible(m_showSidebars);
    if (m_rightSideBar)
        m_rightSideBar->setVisible(m_showSidebars);
}

void DesignModeWidget::readSettings()
{
    QSettings *settings = Core::ICore::settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->readSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->readSettings(settings, QLatin1String("RightSideBar"));
    if (settings->contains("MainSplitter")) {
        const QByteArray splitterState = settings->value("MainSplitter").toByteArray();
        m_mainSplitter->restoreState(splitterState);
        m_mainSplitter->setOpaqueResize(); // force opaque resize since it used to be off
    }
    settings->endGroup();
}

void DesignModeWidget::saveSettings()
{
    QSettings *settings = Core::ICore::settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->saveSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->saveSettings(settings, QLatin1String("RightSideBar"));
    settings->setValue("MainSplitter", m_mainSplitter->saveState());
    settings->endGroup();
}

void DesignModeWidget::enableWidgets()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    m_warningWidget->setVisible(false);
    viewManager().enableWidgets();
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);
    m_isDisabled = false;
}

void DesignModeWidget::disableWidgets()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    viewManager().disableWidgets();
    m_leftSideBar->setEnabled(false);
    m_rightSideBar->setEnabled(false);
    m_isDisabled = true;
}

void DesignModeWidget::updateErrorStatus(const QList<RewriterView::Error> &errors)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << errors.count();

    if (m_isDisabled && errors.isEmpty()) {
        enableWidgets();
     } else if (!errors.isEmpty()) {
        disableWidgets();
        showErrorMessage(errors);
    }
}

TextEditor::ITextEditor *DesignModeWidget::textEditor() const
{
    return currentDesignDocument()->textEditor();
}

void DesignModeWidget::setCurrentDesignDocument(DesignDocument *newDesignDocument)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << newDesignDocument;

    //viewManager().setDesignDocument(newDesignDocument);


}

void DesignModeWidget::setup()
{
    QList<Core::INavigationWidgetFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<Core::INavigationWidgetFactory>();

    QWidget *openDocumentsWidget = 0;
    QWidget *projectsExplorer = 0;
    QWidget *fileSystemExplorer = 0;

    foreach (Core::INavigationWidgetFactory *factory, factories) {
        Core::NavigationView navigationView;
        navigationView.widget = 0;
        if (factory->id() == "Projects") {
            navigationView = factory->createWidget();
            projectsExplorer = navigationView.widget;
            projectsExplorer->setWindowTitle(tr("Projects"));
        } else if (factory->id() == "File System") {
            navigationView = factory->createWidget();
            fileSystemExplorer = navigationView.widget;
            fileSystemExplorer->setWindowTitle(tr("File System"));
        } else if (factory->id() == "Open Documents") {
            navigationView = factory->createWidget();
            openDocumentsWidget = navigationView.widget;
            openDocumentsWidget->setWindowTitle(tr("Open Documents"));
        }

        if (navigationView.widget) {
            QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
            sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
            sheet += "QLabel { background-color: #4f4f4f; }";
            navigationView.widget->setStyleSheet(QString::fromUtf8(sheet));
        }
    }


    m_toolBar = Core::EditorManager::createToolBar(this);

    m_mainSplitter = new MiniSplitter(this);
    m_mainSplitter->setObjectName("mainSplitter");

    // warning frame should be not in layout, but still child of the widget
    m_warningWidget = new DocumentWarningWidget(this);
    m_warningWidget->setVisible(false);

    QList<Core::SideBarItem*> sideBarItems;
    QList<Core::SideBarItem*> leftSideBarItems;
    QList<Core::SideBarItem*> rightSideBarItems;

    foreach (const WidgetInfo &widgetInfo, viewManager().widgetInfos()) {
        if (widgetInfo.placementHint == widgetInfo.LeftPane) {
            Core::SideBarItem *sideBarItem = new DesignerSideBarItem(widgetInfo.widget, widgetInfo.toolBarWidgetFactory, widgetInfo.uniqueId);
            sideBarItems.append(sideBarItem);
            leftSideBarItems.append(sideBarItem);
        }

        if (widgetInfo.placementHint == widgetInfo.RightPane) {
            Core::SideBarItem *sideBarItem = new DesignerSideBarItem(widgetInfo.widget, widgetInfo.toolBarWidgetFactory, widgetInfo.uniqueId);
            sideBarItems.append(sideBarItem);
            rightSideBarItems.append(sideBarItem);
        }
    }

    if (projectsExplorer) {
        Core::SideBarItem *projectExplorerItem = new Core::SideBarItem(projectsExplorer, QLatin1String(SB_PROJECTS));
        sideBarItems.append(projectExplorerItem);
    }

    if (fileSystemExplorer) {
        Core::SideBarItem *fileSystemExplorerItem = new Core::SideBarItem(fileSystemExplorer, QLatin1String(SB_FILESYSTEM));
        sideBarItems.append(fileSystemExplorerItem);
    }

    if (openDocumentsWidget) {
        Core::SideBarItem *openDocumentsItem = new Core::SideBarItem(openDocumentsWidget, QLatin1String(SB_OPENDOCUMENTS));
        sideBarItems.append(openDocumentsItem);
    }

    m_leftSideBar.reset(new Core::SideBar(sideBarItems, leftSideBarItems));
    m_rightSideBar.reset(new Core::SideBar(sideBarItems, rightSideBarItems));

    connect(m_leftSideBar.data(), SIGNAL(availableItemsChanged()), SLOT(updateAvailableSidebarItemsRight()));
    connect(m_rightSideBar.data(), SIGNAL(availableItemsChanged()), SLOT(updateAvailableSidebarItemsLeft()));

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(deleteSidebarWidgets()));

    m_toolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(true);

    connect(m_toolBar, SIGNAL(goForwardClicked()), this, SLOT(toolBarOnGoForwardClicked()));
    connect(m_toolBar, SIGNAL(goBackClicked()), this, SLOT(toolBarOnGoBackClicked()));

    if (currentDesignDocument())
        setupNavigatorHistory(currentDesignDocument()->textEditor());

    // m_mainSplitter area:
    m_mainSplitter->addWidget(m_leftSideBar.data());
    m_mainSplitter->addWidget(createCenterWidget());
    m_mainSplitter->addWidget(m_rightSideBar.data());

    // Finishing touches:
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes(QList<int>() << 150 << 300 << 150);

    QLayout *mainLayout = new QBoxLayout(QBoxLayout::RightToLeft, this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_mainSplitter);

    m_warningWidget->setVisible(false);
    viewManager().enableWidgets();
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);
    m_leftSideBar->setCloseWhenEmpty(true);
    m_rightSideBar->setCloseWhenEmpty(true);

    readSettings();

    show();
}

void DesignModeWidget::updateAvailableSidebarItemsRight()
{
    // event comes from m_leftSidebar, so update right side.
    m_rightSideBar->setUnavailableItemIds(m_leftSideBar->unavailableItemIds());
}

void DesignModeWidget::updateAvailableSidebarItemsLeft()
{
    // event comes from m_rightSidebar, so update left side.
    m_leftSideBar->setUnavailableItemIds(m_rightSideBar->unavailableItemIds());
}

void DesignModeWidget::deleteSidebarWidgets()
{
    m_leftSideBar.reset();
    m_rightSideBar.reset();
}

void DesignModeWidget::qmlPuppetCrashed()
{
    QList<RewriterView::Error> errorList;
    RewriterView::Error error(tr("Qt Quick emulation layer crashed"));
    errorList.append(error);

    disableWidgets();
    showErrorMessage(errorList);
}

void DesignModeWidget::toolBarOnGoBackClicked()
{
    if (m_navigatorHistoryCounter > 0) {
        --m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter),
                                        Core::Id(), Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
}

void DesignModeWidget::toolBarOnGoForwardClicked()
{
    if (m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1)) {
        ++m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter),
                                        Core::Id(), Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
}

DesignDocument *DesignModeWidget::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

ViewManager &DesignModeWidget::viewManager()
{
    return QmlDesignerPlugin::instance()->viewManager();
}

void DesignModeWidget::resizeEvent(QResizeEvent *event)
{
    if (m_warningWidget)
        m_warningWidget->move(QPoint(event->size().width() / 2, event->size().height() / 2));
    QWidget::resizeEvent(event);
}

void DesignModeWidget::setupNavigatorHistory(Core::IEditor *editor)
{
    if (!m_keepNavigatorHistory)
        addNavigatorHistoryEntry(editor->document()->filePath());

    const bool canGoBack = m_navigatorHistoryCounter > 0;
    const bool canGoForward = m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1);
    m_toolBar->setCanGoBack(canGoBack);
    m_toolBar->setCanGoForward(canGoForward);
    m_toolBar->setCurrentEditor(editor);
}

void DesignModeWidget::addNavigatorHistoryEntry(const QString &fileName)
{
    if (m_navigatorHistoryCounter > 0)
        m_navigatorHistory.insert(m_navigatorHistoryCounter + 1, fileName);
    else
        m_navigatorHistory.append(fileName);

    ++m_navigatorHistoryCounter;
}

static QWidget *createWidgetsInTabWidget(const QList<WidgetInfo> &widgetInfos)
{
    QTabWidget *tabWidget = new QTabWidget;

    foreach (const WidgetInfo &widgetInfo, widgetInfos)
        tabWidget->addTab(widgetInfo.widget, widgetInfo.tabName);

    return tabWidget;
}

QWidget *DesignModeWidget::createCenterWidget()
{
    QWidget *centerWidget = new QWidget;

    QVBoxLayout *rightLayout = new QVBoxLayout(centerWidget);
    rightLayout->setMargin(0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_toolBar);


    //### we now own these here
    QList<WidgetInfo> topWidgetInfos;
    foreach (const WidgetInfo &widgetInfo, viewManager().widgetInfos()) {
        if (widgetInfo.placementHint == widgetInfo.TopPane)
            topWidgetInfos.append(widgetInfo);
    }

    if (topWidgetInfos.count() == 1)
        rightLayout->addWidget(topWidgetInfos.first().widget);
    else
        rightLayout->addWidget(createWidgetsInTabWidget(topWidgetInfos));

    QList<WidgetInfo> centralWidgetInfos;
    foreach (const WidgetInfo &widgetInfo, viewManager().widgetInfos()) {
        if (widgetInfo.placementHint == widgetInfo.CentralPane)
            centralWidgetInfos.append(widgetInfo);
    }

    // editor and output panes
    if (centralWidgetInfos.count() == 1)
        m_outputPlaceholderSplitter->addWidget(centralWidgetInfos.first().widget);
    else
         m_outputPlaceholderSplitter->addWidget(createWidgetsInTabWidget(centralWidgetInfos));

    m_outputPlaceholderSplitter->addWidget(m_outputPanePlaceholder);
    m_outputPlaceholderSplitter->setStretchFactor(0, 10);
    m_outputPlaceholderSplitter->setStretchFactor(1, 0);
    m_outputPlaceholderSplitter->setOrientation(Qt::Vertical);

    rightLayout->addWidget(m_outputPlaceholderSplitter);

    return centerWidget;
}

void DesignModeWidget::showErrorMessage(const QList<RewriterView::Error> &errors)
{
    Q_ASSERT(!errors.isEmpty());
    m_warningWidget->setError(errors.first());
    m_warningWidget->setVisible(true);
    m_warningWidget->move(width() / 2, height() / 2);
}

QString DesignModeWidget::contextHelpId() const
{
    if (currentDesignDocument())
        return currentDesignDocument()->contextHelpId();
    return QString();
}

void DesignModeWidget::initialize()
{
    if (m_initStatus == NotInitialized) {
        m_initStatus = Initializing;
        setup();
    }

    m_initStatus = Initialized;
}

} // namespace Internal
} // namespace Designer
