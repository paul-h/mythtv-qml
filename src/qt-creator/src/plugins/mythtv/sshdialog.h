#ifndef SSHDIALOG_H
#define SSHDIALOG_H

#include <QDialog>

namespace Ui {
class SShDialog;
}

class SShDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SShDialog(QWidget *parent = 0);
    ~SShDialog();
    void runProgram(const QString &mUser,const QString &mHost);

protected:
    void changeEvent(QEvent *e);
    
private slots:
    void on_pushButton_clicked();

    void on_okButton_clicked();

private:
    Ui::SShDialog *ui;
};

#endif // SSHDIALOG_H
