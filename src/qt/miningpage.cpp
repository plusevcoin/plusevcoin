#include "miningpage.h"
#include "ui_miningpage.h"

MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage)
{
    ui->setupUi(this);

    setFixedSize(400, 420);

    minerActive = false;

    minerProcess = new QProcess(this);
    minerProcess->setProcessChannelMode(QProcess::MergedChannels);

    readTimer = new QTimer(this);

    acceptedShares = 0;
    rejectedShares = 0;

    roundAcceptedShares = 0;
    roundRejectedShares = 0;

    initThreads = 0;

    connect(readTimer, SIGNAL(timeout()), this, SLOT(readProcessOutput()));

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(startPressed()));
//    connect(ui->pondMining, SIGNAL(pressed()), this, SLOT(pondPressed()));
//    connect(ui->NOPE, SIGNAL(pressed()), this, SLOT(pondUnPressed()));
    connect(ui->typeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(ui->debugCheckBox, SIGNAL(toggled(bool)), this, SLOT(debugToggled(bool)));
    connect(minerProcess, SIGNAL(started()), this, SLOT(minerStarted()));
    connect(minerProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(minerError(QProcess::ProcessError)));
    connect(minerProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(minerFinished()));
    connect(minerProcess, SIGNAL(readyRead()), this, SLOT(readProcessOutput()));
}

MiningPage::~MiningPage()
{
    minerProcess->kill();

    delete ui;
}

void MiningPage::setModel(ClientModel *model)
{
    this->model = model;

    loadSettings();

    bool pool = model->getMiningType() == ClientModel::PoolMining;
    ui->threadsBox->setValue(model->getMiningThreads());
    ui->typeBox->setCurrentIndex(pool ? 1 : 0);
//    if (model->getMiningStarted())
//        startPressed();
}

void MiningPage::startPressed()
{
    initThreads = ui->threadsBox->value();

    if (minerActive == false)
    {
        saveSettings();

        if (getMiningType() == ClientModel::InternalMining)
            minerStarted();
        else if (getMiningType() == ClientModel::Solo2Mining)
            startExtMining();
        else if (getMiningType() == ClientModel::P2PMining)
            startExtMining();
        else if (getMiningType() == ClientModel::PoolMining)
            startExtMining();
    }
    else
    {
        if (getMiningType() == ClientModel::InternalMining)
            minerFinished();
        else if (getMiningType() == ClientModel::Solo2Mining)
            stopExtMining();
        else if (getMiningType() == ClientModel::P2PMining)
            stopExtMining();
        else if (getMiningType() == ClientModel::PoolMining)
            stopExtMining();
    }
}

void MiningPage::startExtMining()
{
    if(getMinerType() == ClientModel::Minerd)
    {
        QStringList args;
        QString url = ui->serverLine->text();
        if (!url.contains("stratum+tcp://") && !url.contains("http://"))
	{
	    if(getMiningType() == ClientModel::P2PMining)
            	url.prepend("http://");
	    else if (getMiningType() == ClientModel::PoolMining)
            	url.prepend("stratum+tcp://");
	}
        QString urlLine = QString("%1:%2").arg(url, ui->portLine->text());
        QString userpassLine = QString("%1:%2").arg(ui->usernameLine->text(), ui->passwordLine->text());
        args << "--algo" << "scrypt";
        args << "--scantime" << ui->scantimeBox->text().toAscii();
        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();
        args << "--threads" << ui->threadsBox->text().toAscii();
        args << "--retries" << "-1"; // Retry forever.
        args << "-P"; // This is needed for this to work correctly on Windows. Extra protocol dump helps flush the buffer quicker.

        threadSpeed.clear();

        acceptedShares = 0;
        rejectedShares = 0;

        roundAcceptedShares = 0;
        roundRejectedShares = 0;

        // If minerd is in current path, then use that. Otherwise, assume minerd is in the path somewhere.
        QString program = QDir::current().filePath("minerd");
        if (!QFile::exists(program))
            program = "minerd";

        if (ui->debugCheckBox->isChecked())
            ui->list->addItem(args.join(" ").prepend(" ").prepend(program));

        ui->mineSpeedLabel->setText("Speed: N/A");
        ui->shareCount->setText("Accepted: 0 - Rejected: 0");
        minerProcess->start(program,args);
        minerProcess->waitForStarted(-1);

        readTimer->start(500);
    }

    else if (getMinerType() == ClientModel::CUDAMiner)
    {
        QStringList args;
        QString url = ui->serverLine->text();
        if (!url.contains("stratum+tcp://") && !url.contains("http://"))
	{
	    if(getMiningType() == ClientModel::P2PMining)
            	url.prepend("http://");
	    else if (getMiningType() == ClientModel::PoolMining)
            	url.prepend("stratum+tcp://");
	}
        QString urlLine = QString("%1:%2").arg(url, ui->portLine->text());
        QString kernel = ui->kernel->text();
        QString userpassLine = QString("%1:%2").arg(ui->usernameLine->text(), ui->passwordLine->text());
        QString textureCache = getTextureCache();
        QString offloadSHA = getOffloadSHA();
        QString memoryBlock = getMemoryBlock();
        args << "--launch-config" << kernel.toAscii();
        if(ui->autotune->currentIndex() == 0)
        {
            args << "--no-autotune";
        }
        args << "--texture-cache" << textureCache.toAscii();
        args << "--hash-parallel" << offloadSHA.toAscii();
        args << "--single-memory" << memoryBlock.toAscii();
        args << "--scantime" << ui->scantimeBox->text().toAscii();
        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();
        args << "--retries" << "-1"; // Retry forever.
    
        threadSpeed.clear();

        acceptedShares = 0;
        rejectedShares = 0;

        roundAcceptedShares = 0;
        roundRejectedShares = 0;

        // If minerd is in current path, then use that. Otherwise, assume minerd is in the path somewhere.
        QString program = QDir::current().filePath("cudaminer");
        if (!QFile::exists(program))
            program = "cudaminer";

        if (ui->debugCheckBox->isChecked())
            ui->list->addItem(args.join(" ").prepend(" ").prepend(program));

        ui->mineSpeedLabel->setText("Speed: N/A");
        ui->shareCount->setText("Accepted: 0 - Rejected: 0");
        minerProcess->start(program,args);
        minerProcess->waitForStarted(-1);

        readTimer->start(500);        
    }

    else //if (getMinerType() == ClientModel::CUDAMiner)
    {
        QStringList args;
        QString url = ui->serverLine->text();
        if (!url.contains("stratum+tcp://") && !url.contains("http://"))
	{
	    if(getMiningType() == ClientModel::P2PMining)
            	url.prepend("http://");
	    else if (getMiningType() == ClientModel::PoolMining)
            	url.prepend("stratum+tcp://");
	}
        QString urlLine = QString("%1:%2").arg(url, ui->portLine->text());
        QString userpassLine = QString("%1:%2").arg(ui->usernameLine->text(), ui->passwordLine->text());
        QString intensity = getIntensity();
        QString concurrency = ui->concurrency->text();
        QString workload = ui->workload->text();
        args << "--scrypt";
        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();
        args << "--intensity" << intensity.toAscii();
        args << "--thread-concurrency" << concurrency.toAscii();
        args << "--worksize" << workload.toAscii();
        args << "--gpu-threads" << ui->threadsBox->text().toAscii();

        threadSpeed.clear();

        acceptedShares = 0;
        rejectedShares = 0;

        roundAcceptedShares = 0;
        roundRejectedShares = 0;

        // If minerd is in current path, then use that. Otherwise, assume minerd is in the path somewhere.
        QString program = QDir::current().filePath("cgminer");
        if (!QFile::exists(program))
            program = "cgminer";

        if (ui->debugCheckBox->isChecked())
            ui->list->addItem(args.join(" ").prepend(" ").prepend(program));

        ui->mineSpeedLabel->setText("Speed: N/A");
        ui->shareCount->setText("Accepted: 0 - Rejected: 0");
        minerProcess->start(program,args);
        minerProcess->waitForStarted(-1);

        readTimer->start(500);
    }
}

void MiningPage::stopExtMining()
{
    ui->mineSpeedLabel->setText("");
    minerProcess->kill();
    readTimer->stop();
}

void MiningPage::saveSettings()
{
    model->setMiningDebug(ui->debugCheckBox->isChecked());
    model->setMiningScanTime(ui->scantimeBox->value());
    model->setMiningServer(ui->serverLine->text());
    model->setMiningPort(ui->portLine->text());
    model->setMiningUsername(ui->usernameLine->text());
    model->setMiningPassword(ui->passwordLine->text());
}

void MiningPage::loadSettings()
{
    ui->debugCheckBox->setChecked(model->getMiningDebug());
    ui->scantimeBox->setValue(model->getMiningScanTime());
    ui->serverLine->setText(model->getMiningServer());
    ui->portLine->setText(model->getMiningPort());
    ui->usernameLine->setText(model->getMiningUsername());
    ui->passwordLine->setText(model->getMiningPassword());
}

void MiningPage::readProcessOutput()
{
    QByteArray output;

    minerProcess->reset();

    output = minerProcess->readAll();

    QString outputString(output);

    if (!outputString.isEmpty())
    {
        QStringList list = outputString.split("\n", QString::SkipEmptyParts);
        int i;
        for (i=0; i<list.size(); i++)
        {
            QString line = list.at(i);

            // Ignore protocol dump
            if (!line.startsWith("[") || line.contains("JSON protocol") || line.contains("HTTP hdr"))
                continue;

            if (ui->debugCheckBox->isChecked())
            {
                ui->list->addItem(line.trimmed());
                ui->list->scrollToBottom();
            }

            if (line.contains("(yay!!!)"))
                reportToList("Success! PlusEVCoin block mined :)", SHARE_SUCCESS, getTime(line));
            else if (line.contains("(booooo)"))
                reportToList("Block rejected, keep mining.", SHARE_FAIL, getTime(line));
            else if (line.contains("LONGPOLL detected new block"))
                reportToList("LONGPOLL detected a new block", LONGPOLL, getTime(line));
            else if (line.contains("Supported options:"))
                reportToList("Miner didn't start properly. Try checking your settings.", ERROR, NULL);
            else if (line.contains("The requested URL returned error: 403"))
                reportToList("Couldn't connect. Please check your username and password.", ERROR, NULL);
            else if (line.contains("HTTP request failed"))
                reportToList("Couldn't connect. Please check pool server and port.", ERROR, NULL);
            else if (line.contains("JSON-RPC call failed"))
                reportToList("Couldn't communicate with server. Retrying in 30 seconds.", ERROR, NULL);
            else if (line.contains("thread ") && line.contains("khash/s"))
            {
                QString threadIDstr = line.at(line.indexOf("thread ")+7);
                int threadID = threadIDstr.toInt();

                int threadSpeedindx = line.indexOf(",");
                QString threadSpeedstr = line.mid(threadSpeedindx);
                threadSpeedstr.chop(8);
                threadSpeedstr.remove(", ");
                threadSpeedstr.remove(" ");
                threadSpeedstr.remove('\n');
                double speed=0;
                speed = threadSpeedstr.toDouble();

                threadSpeed[threadID] = speed;

                updateSpeed();
            }
        }
    }
}

void MiningPage::minerError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        reportToList("Miner failed to start. Make sure you have the minerd executable and libraries in the same directory as PlusEVCoin-qt.\n", ERROR, NULL);
    }
}

void MiningPage::minerFinished()
{
    if (getMiningType() == ClientModel::InternalMining)
        reportToList("Solo (internal) mining stopped.", ERROR, NULL);
    else
        reportToList("Miner exited.", ERROR, NULL);
    ui->list->addItem("");
    minerActive = false;
    resetMiningButton();
    model->setMining(getMiningType(), getMinerType(), false, initThreads, 0);
}

void MiningPage::minerStarted()
{
    if (!minerActive)
    {
        if (getMiningType() == ClientModel::InternalMining)
        {
            reportToList("Solo (internal) mining started.", ERROR, NULL);
        }
        else
        {
            reportToList("Miner started. You might not see any output for a few minutes.", STARTED, NULL);
        }
    }
    minerActive = true;
    resetMiningButton();
    model->setMining(getMiningType(), getMinerType(), true, initThreads, 0);
}

void MiningPage::updateSpeed()
{
    double totalSpeed=0;
    int totalThreads=0;

    QMapIterator<int, double> iter(threadSpeed);
    while(iter.hasNext())
    {
        iter.next();
        totalSpeed += iter.value();
        totalThreads++;
    }

    // If all threads haven't reported the hash speed yet, make an assumption
    if (totalThreads != initThreads)
    {
        totalSpeed = (totalSpeed/totalThreads)*initThreads;
    }

    QString speedString = QString("%1").arg(totalSpeed);
    QString threadsString = QString("%1").arg(initThreads);

    QString acceptedString = QString("%1").arg(acceptedShares);
    QString rejectedString = QString("%1").arg(rejectedShares);

    QString roundAcceptedString = QString("%1").arg(roundAcceptedShares);
    QString roundRejectedString = QString("%1").arg(roundRejectedShares);

    if (totalThreads == initThreads)
        ui->mineSpeedLabel->setText(QString("Speed: %1 khash/sec - %2 thread(s)").arg(speedString, threadsString));
    else
        ui->mineSpeedLabel->setText(QString("Speed: ~%1 khash/sec - %2 thread(s)").arg(speedString, threadsString));

    ui->shareCount->setText(QString("Accepted: %1 (%3) - Rejected: %2 (%4)").arg(acceptedString, rejectedString, roundAcceptedString, roundRejectedString));

    model->setMining(getMiningType(), getMinerType(), true, initThreads, totalSpeed*1000);
}

void MiningPage::reportToList(QString msg, int type, QString time)
{
    QString message;
    if (time == NULL)
        message = QString("[%1] - %2").arg(QTime::currentTime().toString(), msg);
    else
        message = QString("[%1] - %2").arg(time, msg);

    switch(type)
    {
        case SHARE_SUCCESS:
            acceptedShares++;
            roundAcceptedShares++;
            updateSpeed();
            break;

        case SHARE_FAIL:
            rejectedShares++;
            roundRejectedShares++;
            updateSpeed();
            break;

        case LONGPOLL:
            roundAcceptedShares = 0;
            roundRejectedShares = 0;
            break;

        default:
            break;
    }

    ui->list->addItem(message);
    ui->list->scrollToBottom();
}

// Function for fetching the time
QString MiningPage::getTime(QString time)
{
    if (time.contains("["))
    {
        time.resize(21);
        time.remove("[");
        time.remove("]");
        time.remove(0,11);

        return time;
    }
    else
        return NULL;
}

void MiningPage::enableMiningControls(bool enable)
{
    ui->typeBox->setEnabled(enable);
    ui->threadsBox->setEnabled(enable);
    ui->scantimeBox->setEnabled(enable);
    ui->serverLine->setEnabled(enable);
    ui->portLine->setEnabled(enable);
    ui->usernameLine->setEnabled(enable);
    ui->passwordLine->setEnabled(enable);
}

void MiningPage::enablePoolMiningControls(bool enable)
{
    ui->scantimeBox->setEnabled(enable);
    ui->serverLine->setEnabled(enable);
    ui->portLine->setEnabled(enable);
    ui->usernameLine->setEnabled(enable);
    ui->passwordLine->setEnabled(enable);
}

ClientModel::MiningType MiningPage::getMiningType()
{
    if (ui->typeBox->currentIndex() == 0)  // Internal Mining
    {
        return ClientModel::InternalMining;
    }
    else if (ui->typeBox->currentIndex() == 1) // Solo2 Mining
    {
        return ClientModel::Solo2Mining;
    }
    else if (ui->typeBox->currentIndex() == 2)  // P2P Mining
    {
        return ClientModel::P2PMining;
    }
    else if (ui->typeBox->currentIndex() == 3) // Poool Mining
    {
        return ClientModel::PoolMining;
    }
    return ClientModel::InternalMining;
}

ClientModel::MinerType MiningPage::getMinerType()
{
    if(ui->minerBox->currentIndex() == 0) // Minerd [CPU]
    {
        return ClientModel::Minerd;
    } 
    else if(ui->minerBox->currentIndex() == 1) // CUDA Miner [nVidia]
    {
        return ClientModel::CUDAMiner;
    }
    else   if(ui->minerBox->currentIndex() == 2) // CGMiner [AMD]
    {
        return ClientModel::CGMiner;
    }
    return ClientModel::Minerd;
}

const char* MiningPage::getTextureCache()
{
    if(ui->textureCache->currentIndex() == 0)
    {
        return "0";
    }
    else if (ui->textureCache->currentIndex() == 1)
    {
        return "1";
    }
    else
    {
        return "2";
    }
}

const char* MiningPage::getOffloadSHA()
{
    if(ui->offloadSHA->currentIndex() == 0)
    {
        return "0";
    }
    else if (ui->offloadSHA->currentIndex() == 1)
    {
        return "1";
    }
    else
    {
        return "2";
    }
}

const char* MiningPage::getMemoryBlock()
{
    if(ui->memoryBlock->currentIndex() == 0)
    {
        return "0";
    }
    else
    {
        return "1";
    }
}

const char* MiningPage::getIntensity()
{
    if(ui->intensity->value() == 1)
    {
        return "1";
    }
    else if(ui->intensity->value() == 2)
    {
        return "2";
    }
    else if(ui->intensity->value() == 3)
    {
        return "3";
    }
    else if(ui->intensity->value() == 4)
    {
        return "4";
    }
    else if(ui->intensity->value() == 5)
    {
        return "5";
    }
    else if(ui->intensity->value() == 6)
    {
        return "6";
    }
    else if(ui->intensity->value() == 7)
    {
        return "7";
    }
    else if(ui->intensity->value() == 8)
    {
        return "8";
    }
    else if(ui->intensity->value() == 9)
    {
        return "9";
    }
    else if(ui->intensity->value() == 10)
    {
        return "10";
    }
    else if(ui->intensity->value() == 11)
    {
        return "11";
    }
    else if(ui->intensity->value() == 12)
    {
        return "12";
    }
    else if(ui->intensity->value() == 13)
    {
        return "13";
    }
    else if(ui->intensity->value() == 14)
    {
        return "14";
    }    
    else if(ui->intensity->value() == 15)
    {
        return "15";
    }    
    else if(ui->intensity->value() == 16)
    {
        return "16";
    }    
    else if(ui->intensity->value() == 17)
    {
        return "17";
    }   
    else if(ui->intensity->value() == 18)
    {
        return "18";
    }    
    else if(ui->intensity->value() == 19)
    {
        return "19";
    }    
    else if(ui->intensity->value() == 20)
    {
        return "20";
    }    
    else
    {
        return "1";
    }
}

void MiningPage::typeChanged(int index)
{
    if (index == 0)  // Solo Mining
    {
        enablePoolMiningControls(false);
    }
    else if (index == 1)  // Pool Mining
    {
        enablePoolMiningControls(true);
    }
}

void MiningPage::debugToggled(bool checked)
{
    model->setMiningDebug(checked);
}

void MiningPage::resetMiningButton()
{
    ui->startButton->setText(minerActive ? "Stop Mining" : "Start Mining");
    enableMiningControls(!minerActive);
}
