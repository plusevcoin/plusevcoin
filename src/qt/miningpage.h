#ifndef MININGPAGE_H
#define MININGPAGE_H

#include "clientmodel.h"

#include <QWidget>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include <QSlider>

// Log types
#define STARTED 0
#define SHARE_SUCCESS 1
#define SHARE_FAIL 2
#define ERROR 3
#define LONGPOLL 4

namespace Ui {
    class MiningPage;
}
class ClientModel;

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(QWidget *parent = 0);
    ~MiningPage();

    bool minerActive;

    QProcess *minerProcess;

    QMap<int, double> threadSpeed;

    QTimer *readTimer;

    int acceptedShares;
    int rejectedShares;

    int roundAcceptedShares;
    int roundRejectedShares;

    int initThreads;
    
    int getMiner();
    const char* getTextureCache();
    const char* getOffloadSHA();
    const char* getMemoryBlock();
    const char* getIntensity();
    void setModel(ClientModel *model);

public slots:
    void startPressed();

    void minerStarted();
    void minerError(QProcess::ProcessError);
    void minerFinished();

    void startExtMining();
    void stopExtMining();

    void updateSpeed();

    void loadSettings();
    void saveSettings();

    void reportToList(QString, int, QString);


    void readProcessOutput();

    QString getTime(QString);
    void enableMiningControls(bool enable);
    void enablePoolMiningControls(bool enable);
    ClientModel::MiningType getMiningType();
    ClientModel::MinerType getMinerType();
    void typeChanged(int index);
    void minerChanged(int index);
    void debugToggled(bool checked);

private slots:

private:
    Ui::MiningPage *ui;
    ClientModel *model;

    void resetMiningButton();
};

#endif // MININGPAGE_H
