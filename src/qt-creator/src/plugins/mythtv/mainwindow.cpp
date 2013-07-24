#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sshdialog.h"
#include "zconf.h"

#include <QDesktopServices>
#include <QDebug>
#include <QUrl>
#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QtCore>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    //    m_HostName = "http://192.168.1.21";
//    m_DbName = "mythconverg";
//    m_User = "mythtv";
//    m_Password = "kweQI1U9";

    QSqlDatabase  db = QSqlDatabase::addDatabase(QLatin1String("QMYSQL") );
    db.setHostName(QLatin1String("http://192.168.1.21"));
    db.setDatabaseName(QLatin1String("mythconverg"));
    db.setUserName(QLatin1String("mythtv"));
    db.setPassword(QLatin1String(""));
    db.setPort(3306);
    if (!db.open())
    {
        qDebug() << db.lastError().text();
    }
    model = new QSqlTableModel(this);
    model->setTable(QLatin1String("recorded"));
    model->select();
    ui->tableView->setModel (model);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_openwiki_clicked()
{

    QDesktopServices wiki;
    wiki.openUrl(QUrl(QLatin1String("http://www.mythtv.org/wiki/")));
}

void MainWindow::on_sshToBackend_clicked()
{
SShDialog mDialog;
mDialog.setModal(true);
mDialog.exec();
}

void MainWindow::on_mythfill_clicked()
{
    ZConf zconf;
            zc.append(QLatin1String(zconf.getString().toLatin1()));
    qDebug() << zc;
}
