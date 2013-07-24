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
    
protected:
    void changeEvent(QEvent *e);
    
private:
    Ui::SShDialog *ui;
};

#endif // SSHDIALOG_H
