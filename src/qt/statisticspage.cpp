#include "statisticspage.h"
#include "ui_statisticspage.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcoinrpc.h"
#include <sstream>
#include <string>

using namespace json_spirit;

StatisticsPage::StatisticsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatisticsPage)
{
    ui->setupUi(this);
    
    setFixedSize(400, 420);
    
    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(updateStatistics()));
}

void StatisticsPage::updateStatistics()
{    
    double pDifficulty = this->model->GetDifficulty();
    int pPawrate = this->model->GetNetworkHashPS(-1);
    double pPawrate2 = 0.000;
    int nHeight = pindexBest->nHeight;
    int lPawrate = this->model->getHashrate();
    double lPawrate2 = 0.000;
    double nSubsidy = 0;
    int volume = getTotalVolume();
    int peers = this->model->getNumConnections();
    if(nHeight < 1960000)
    {
        nSubsidy = 250.000000 - ((double)nHeight * 0.000125);
    }
    else
    {
        nSubsidy = 5;
    }
    lPawrate2 = ((double)lPawrate / 1000);
    pPawrate2 = ((double)pPawrate / 1000);  
    QString height = QString::number(nHeight);
    QString subsidy = QString::number(nSubsidy, 'f', 6);
    QString difficulty = QString::number(pDifficulty, 'f', 6);
    QString pawrate = QString::number(pPawrate2, 'f', 3);
    QString Qlpawrate = QString::number(lPawrate2, 'f', 3);
    QString QPeers = QString::number(peers);
    QString qVolume = QString::number(volume);
    ui->heightBox->setText(height);
    ui->rewardBox->setText(subsidy);
    ui->diffBox->setText(difficulty);
    ui->pawrateBox->setText(pawrate + " KH/s");
    ui->localBox->setText(Qlpawrate + " KH/s");
    ui->connectionBox->setText(QPeers);
    ui->volumeBox->setText(qVolume + " FOX");
}

void StatisticsPage::setModel(ClientModel *model)
{
    this->model = model;
}

StatisticsPage::~StatisticsPage()
{
    delete ui;
}
