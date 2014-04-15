#include "miningBasicDialog.h"
#include "ui_miningBasicDialog.h"
#include "clientmodel.h"

#include "version.h"

miningBasicDialog::miningBasicDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::miningBasicDialog)
{
    ui->setupUi(this);
}

void miningBasicDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion());
    }
}

miningBasicDialog::~miningBasicDialog()
{
    delete ui;
}

void miningBasicDialog::on_buttonBox_accepted()
{
    close();
}
