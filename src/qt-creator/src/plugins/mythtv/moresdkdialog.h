#ifndef MORESDKDIALOG_H
#define MORESDKDIALOG_H

#include <QDialog>

namespace Ui {
class MoreSDKDialog;
}

class MoreSDKDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit MoreSDKDialog(QWidget *parent = 0);
    ~MoreSDKDialog();
    
protected:
    void changeEvent(QEvent *e);
    
private:
    Ui::MoreSDKDialog *ui;
};

#endif // MORESDKDIALOG_H
