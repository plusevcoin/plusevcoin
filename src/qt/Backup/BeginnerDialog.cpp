#include "beginnerdialog.h"
#include "ui_beginnerdialog.h"
#include "clientmodel.h"

BeginnerDialog::BeginnerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BeginnerDialog)
{
    ui->setupUi(this);
}

BeginnerDialog::~BeginnerDialog()
{
    delete ui;
}

void BeginnerDialog::on_buttonBox_accepted()
{
    close();
}
