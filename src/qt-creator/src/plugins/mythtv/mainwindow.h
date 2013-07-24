#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QSqlTableModel>
#include   <QtCore>
namespace Ui {
class MainWindow;

}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void mythWiki();
//    void tableView();

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_openwiki_clicked();

    void on_sshToBackend_clicked();

    void on_mythfill_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *model;
    QString zc;
};

#endif // MAINWINDOW_H
