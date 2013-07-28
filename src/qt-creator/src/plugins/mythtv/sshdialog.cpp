#include "sshdialog.h"
#include "ui_sshdialog.h"
#include <QProcess>
#include <QObject>

SShDialog::SShDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SShDialog)
{
    ui->setupUi(this);
}

SShDialog::~SShDialog()
{
    delete ui;
}

void SShDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
