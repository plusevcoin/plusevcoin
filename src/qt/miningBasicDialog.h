#ifndef MININGBASICDIALOG_H
#define MININGBASICDIALOG_H

#include <QDialog>

namespace Ui {
    class miningBasicDialog;
}
class ClientModel;

/** "miningBasic" dialog box */
class miningBasicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit miningBasicDialog(QWidget *parent = 0);
    ~miningBasicDialog();

    void setModel(ClientModel *model);
private:
    Ui::miningBasicDialog *ui;

private slots:
    void on_buttonBox_accepted();
};

#endif // miningBasicDIALOGH
