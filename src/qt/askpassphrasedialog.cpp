#include "askpassphrasedialog.h"
#include "ui_askpassphrasedialog.h"

#include "guiconstants.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QPushButton>
#include <QKeyEvent>

AskPassphraseDialog::AskPassphraseDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AskPassphraseDialog),
    mode(mode),
    model(0),
    fCapsLock(false)
{
    ui->setupUi(this);
    ui->passEdit1->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit2->setMaxLength(MAX_PASSPHRASE_SIZE);
    ui->passEdit3->setMaxLength(MAX_PASSPHRASE_SIZE);
    
    // Setup Caps Lock detection.
    ui->passEdit1->installEventFilter(this);
    ui->passEdit2->installEventFilter(this);
    ui->passEdit3->installEventFilter(this);

    switch(mode)
    {
        case Encrypt: // Ask passphrase x2
            ui->passLabel1->hide();
            ui->passEdit1->hide();
            ui->warningLabel->setText(tr("Enter the new passphrase to the FoxHole.<br/>Please use a passphrase of <b>10 or more random characters</b>, or <b>eight or more words</b>."));
            setWindowTitle(tr("Barricade FoxHole"));
            break;
        case Unlock: // Ask passphrase
            ui->warningLabel->setText(tr("This operation needs your FoxHole passphrase to unlock the FoxHole."));
            ui->passLabel2->hide();
            ui->passEdit2->hide();
            ui->passLabel3->hide();
            ui->passEdit3->hide();
            setWindowTitle(tr("Unlock FoxHole"));
            break;
        case Decrypt:   // Ask passphrase
            ui->warningLabel->setText(tr("This operation needs your FoxHole passphrase to de-barricade the FoxHole."));
            ui->passLabel2->hide();
            ui->passEdit2->hide();
            ui->passLabel3->hide();
            ui->passEdit3->hide();
            setWindowTitle(tr("De-barricade FoxHole"));
            break;
        case ChangePass: // Ask old passphrase + new passphrase x2
            setWindowTitle(tr("Change passphrase"));
            ui->warningLabel->setText(tr("Enter the old and new passphrase to the FoxHole."));
            break;
    }

    textChanged();
    connect(ui->passEdit1, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit2, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
    connect(ui->passEdit3, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));
}

AskPassphraseDialog::~AskPassphraseDialog()
{
    // Attempt to overwrite text so that they do not linger around in memory
    ui->passEdit1->setText(QString(" ").repeated(ui->passEdit1->text().size()));
    ui->passEdit2->setText(QString(" ").repeated(ui->passEdit2->text().size()));
    ui->passEdit3->setText(QString(" ").repeated(ui->passEdit3->text().size()));
    delete ui;
}

void AskPassphraseDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void AskPassphraseDialog::accept()
{
    SecureString oldpass, newpass1, newpass2;
    if(!model)
        return;
    oldpass.reserve(MAX_PASSPHRASE_SIZE);
    newpass1.reserve(MAX_PASSPHRASE_SIZE);
    newpass2.reserve(MAX_PASSPHRASE_SIZE);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make this input mlock()'d to begin with.
    oldpass.assign(ui->passEdit1->text().toStdString().c_str());
    newpass1.assign(ui->passEdit2->text().toStdString().c_str());
    newpass2.assign(ui->passEdit3->text().toStdString().c_str());

    switch(mode)
    {
    case Encrypt: {
        if(newpass1.empty() || newpass2.empty())
        {
            // Cannot encrypt with empty passphrase
            break;
        }
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm FoxHole barricade"),
                 tr("WARNING: If you barricade your FoxHole and lose your passphrase, you will <b>LOSE ALL OF YOUR FoxCoins!!!!</b>!\nAre you sure you wish to barricade your FoxHole?"),
                 QMessageBox::Yes|QMessageBox::Cancel,
                 QMessageBox::Cancel);
        if(retval == QMessageBox::Yes)
        {
            if(newpass1 == newpass2)
            {
                if(model->setWalletEncrypted(true, newpass1))
                {
                    QMessageBox::warning(this, tr("FoxHole barricaded"),
                                         tr("FoxCoin will close now to finish the barricade process. Remember that barricading your FoxHole cannot fully protect your FoxCoin from being stolen by malware infecting your computer."));
                    QApplication::quit();
                }
                else
                {
                    QMessageBox::critical(this, tr("FoxHole barricade fell apart"),
                                         tr("FoxHole barricade failed due to an internal error. Your FoxHole was not barricaded."));
                }
                QDialog::accept(); // Success
            }
            else
            {
                QMessageBox::critical(this, tr("FoxHole barricade failed"),
                                     tr("The supplied passphrases do not match."));
            }
        }
        else
        {
            QDialog::reject(); // Cancelled
        }
        } break;
    case Unlock:
        if(!model->setWalletLocked(false, oldpass))
        {
            QMessageBox::critical(this, tr("FoxHole de-barricade failed"),
                                  tr("The battering ram entered for the FoxHole de-barricading wasn't good enough."));
        }
        else
        {
            QDialog::accept(); // Success
        }
        break;
    case Decrypt:
        if(!model->setWalletEncrypted(false, oldpass))
        {
            QMessageBox::critical(this, tr("FoxHole de-barricade failed"),
                                  tr("The battering ram entered for the FoxHole de-barricading wasn't good enough."));
        }
        else
        {
            QDialog::accept(); // Success
        }
        break;
    case ChangePass:
        if(newpass1 == newpass2)
        {
            if(model->changePassphrase(oldpass, newpass1))
            {
                QMessageBox::information(this, tr("FoxHole barricaded"),
                                     tr("FoxHole barricade was successfully reinforced."));
                QDialog::accept(); // Success
            }
            else
            {
                QMessageBox::critical(this, tr("FoxHole barricade failed"),
                                     tr("The battering ram entered for the FoxHole de-barricading wasn't good enough."));
            }
        }
        else
        {
            QMessageBox::critical(this, tr("FoxHole barricade failed"),
                                 tr("The supplied passphrases do not match."));
        }
        break;
    }
}

void AskPassphraseDialog::textChanged()
{
    // Validate input, set Ok button to enabled when accepable
    bool acceptable = false;
    switch(mode)
    {
    case Encrypt: // New passphrase x2
        acceptable = !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    case Unlock: // Old passphrase x1
    case Decrypt:
        acceptable = !ui->passEdit1->text().isEmpty();
        break;
    case ChangePass: // Old passphrase x1, new passphrase x2
        acceptable = !ui->passEdit1->text().isEmpty() && !ui->passEdit2->text().isEmpty() && !ui->passEdit3->text().isEmpty();
        break;
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}

bool AskPassphraseDialog::event(QEvent *event)
{
    // Detect Caps Lock key press.
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            fCapsLock = !fCapsLock;
        }
        if (fCapsLock) {
            ui->capsLabel->setText(tr("Warning: The Caps Lock key is on."));
        } else {
            ui->capsLabel->clear();
        }
    }
    return QWidget::event(event);
}

bool AskPassphraseDialog::eventFilter(QObject *, QEvent *event)
{
    /* Detect Caps Lock.
     * There is no good OS-independent way to check a key state in Qt, but we
     * can detect Caps Lock by checking for the following condition:
     * Shift key is down and the result is a lower case character, or
     * Shift key is not down and the result is an upper case character.
     */
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QString str = ke->text();
        if (str.length() != 0) {
            const QChar *psz = str.unicode();
            bool fShift = (ke->modifiers() & Qt::ShiftModifier) != 0;
            if ((fShift && psz->isLower()) || (!fShift && psz->isUpper())) {
                fCapsLock = true;
                ui->capsLabel->setText(tr("Warning: The Caps Lock key is on."));
            } else if (psz->isLetter()) {
                fCapsLock = false;
                ui->capsLabel->clear();
            }
        }
    }
    return false;
}
