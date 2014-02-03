#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Ui {
    class StatisticsPage;
}
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

/** Statistics ("home") page widget */
class StatisticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsPage(QWidget *parent = 0);
    ~StatisticsPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);
    void setNumTransactions(int count);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::StatisticsPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
};

#endif // STATISTICSPAGE_H
