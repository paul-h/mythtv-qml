#include "moresdkdialog.h"
#include "ui_moresdkdialog.h"

MoreSDKDialog::MoreSDKDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MoreSDKDialog)
{
    ui->setupUi(this);
}

MoreSDKDialog::~MoreSDKDialog()
{
    delete ui;
}

void MoreSDKDialog::changeEvent(QEvent *e)
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
