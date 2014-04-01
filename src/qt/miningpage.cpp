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
    initThreads = 0;

    connect(readTimer, SIGNAL(timeout()), this, SLOT(readProcessOutput()));

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(startPressed()));
    //connect(ui->pondMining, SIGNAL(pressed()), this, SLOT(pondPressed()));
    //connect(ui->NOPE, SIGNAL(pressed()), this, SLOT(pondUnPressed()));
    connect(ui->typeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(ui->minerBox, SIGNAL(currentIndexChanged(int)), this, SLOT(minerChanged(int)));
    connect(ui->debugCheckBox, SIGNAL(toggled(bool)), this, SLOT(debugToggled(bool)));
    connect(minerProcess, SIGNAL(started()), this, SLOT(minerStarted()));
    connect(minerProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(minerError(QProcess::ProcessError)));
    connect(minerProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(minerFinished()));
    connect(minerProcess, SIGNAL(readyRead()), this, SLOT(readProcessOutput()));

    typeChanged(-1);
}

MiningPage::~MiningPage()
{
    minerProcess->kill();

    delete ui;
}

void MiningPage::setModel(ClientModel *model)
{
    this->model = model;
    //loadSettings();
    setDefaults();

    //bool pool = model->getMiningType() == ClientModel::PoolMining;
    //ui->threadsBox->setValue(model->getMiningThreads());
    //ui->typeBox->setCurrentIndex(pool ? 1 : 0);
    //if (model->getMiningStarted()) startPressed();
}

void MiningPage::startPressed()
{
    initThreads = ui->threadsBox->text().toInt();

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
    QString program;
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

    // Minerd [CPU]
    if(getMinerType() == ClientModel::Minerd)
    {
        program = QDir::current().filePath("minerd/minerd");
        //if (!QFile::exists(program)) program = "minerd";

        args << "--algo" << "scrypt";
        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();

        if(!ui->scantimeBox->text().isEmpty()) args << "--scantime" << ui->scantimeBox->text().toAscii();
        if(!ui->threadsBox->text().isEmpty()) 
        {
            args << "--threads" << ui->threadsBox->text().toAscii();
        }
        else
        {
            args << "--threads" << "2";
            initThreads = 2;
        }
        args << "--retries" << "-1"; // Retry forever.
        args << "-P"; // Extra protocol dump helps flush the buffer quicker (for Windows).
    }

    // CGMiner [AMD/Intel GPU]
    else if (getMinerType() == ClientModel::CGMiner)
    {
        program = QDir::current().filePath("cgminer/cgminer");
        //if (!QFile::exists(program)) program = "cgminer";

        args << "--scrypt";
        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();

        if(!ui->threadsBox->text().isEmpty())
        {
            args << "--gpu-threads" << ui->threadsBox->text().toAscii();
        }
        else
        {
            initThreads = 1;
        }
        if(!ui->intensity->text().isEmpty()) args << "--intensity" << ui->intensity->text().toAscii();
        if(!ui->concurrency->text().isEmpty()) args << "--thread-concurrency" << ui->concurrency->text().toAscii();
        if(!ui->workload->text().isEmpty()) args << "--worksize" << ui->workload->text().toAscii();
        args << "--text-only"; // Output to STDOUT instead of ncurses.
    }

    // CUDAMiner [nVidia GPU]
    else if (getMinerType() == ClientModel::CUDAMiner)
    {
#if defined(Q_WS_WIN)
        program = QDir::current().filePath("cudaminer/cudaminer");
#elif defined(Q_WS_MAC)
        if(QSysInfo::MacintoshVersion==QSysInfo::MV_10_7)
            program = QDir::current().filePath("cudaminer/cudaminer.10.7");
        else if(QSysInfo::MacintoshVersion==QSysInfo::MV_10_8)
            program = QDir::current().filePath("cudaminer/cudaminer.10.8");
        else //if(QSysInfo::MacintoshVersion()==QSysInfo::MV_10_9)
            program = QDir::current().filePath("cudaminer/cudaminer.10.9");
#endif
        //if (!QFile::exists(program)) program = "cudaminer";

        args << "--url" << urlLine.toAscii();
        args << "--userpass" << userpassLine.toAscii();

        if(!ui->scantimeBox->text().isEmpty()) args << "--scantime" << ui->scantimeBox->text().toAscii();
        if(ui->autotune->currentIndex() == 1) args << "--no-autotune";
        if(!ui->launchConfig->text().isEmpty()) args << "--launch-config" << ui->launchConfig->text().toAscii();
        if(!ui->textureCache->text().isEmpty()) args << "--texture-cache" << ui->textureCache->text().toAscii();
        if(!ui->offLoadSHA->text().isEmpty()) args << "--hash-parallel" << ui->offLoadSHA->text().toAscii();
        if(!ui->memoryBlock->text().isEmpty()) args << "--single-memory" << ui->memoryBlock->text().toAscii();
        args << "--retries" << "-1"; // Retry forever.
    }

    threadSpeed.clear();
    acceptedShares = 0;
    rejectedShares = 0;

    if (ui->debugCheckBox->isChecked())
        ui->list->addItem(args.join(" ").prepend(" ").prepend(program));
    ui->mineSpeedLabel->setText("Speed: N/A");
    ui->shareCount->setText("Accepted: 0 - Rejected: 0");
    minerProcess->start(program,args);
    minerProcess->waitForStarted(-1);

    readTimer->start(500);
}

void MiningPage::stopExtMining()
{
    ui->mineSpeedLabel->setText("");
    minerProcess->kill();
    readTimer->stop();
}

void MiningPage::saveSettings()
{
    //reportToList("SAVE settings", LOGSIMPLE, NULL);
    //model->setMiningScanTime(ui->scantimeBox->text().toInt());
    model->setMiningServer(ui->serverLine->text());
    model->setMiningPort(ui->portLine->text());
    model->setMiningUsername(ui->usernameLine->text());
    model->setMiningPassword(ui->passwordLine->text());
    model->setMiningDebug(ui->debugCheckBox->isChecked());
}

void MiningPage::loadSettings()
{
    //reportToList("LOAD settings", LOGSIMPLE, NULL);
    //ui->scantimeBox->setText(QString::number(model->getMiningScanTime()));
    ui->serverLine->setText(model->getMiningServer());
    ui->portLine->setText(model->getMiningPort());
    ui->usernameLine->setText(model->getMiningUsername());
    ui->passwordLine->setText(model->getMiningPassword());
    ui->debugCheckBox->setChecked(model->getMiningDebug());
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
            // !line.startsWith("[") || 
            if (line.contains("JSON protocol") || line.contains("HTTP hdr"))
                continue;
            if (!line.startsWith("[") && !line.startsWith(" [") && !line.startsWith("("))
                continue;


            if (ui->debugCheckBox->isChecked())
            {
                ui->list->addItem(line.trimmed());
                ui->list->scrollToBottom();
            }

            if (line.contains("(yay!!!)") || line.contains("Accepted"))
                reportToList("Success! PlusEVCoin block mined. +50 PEV :)", SHARE_SUCCESS, getTime(line));
            else if (line.contains("(booooo)") || line.contains("Rejected"))
                reportToList("Block rejected, keep mining.", SHARE_FAIL, getTime(line));
            else if (line.contains("LONGPOLL detected new block"))
                reportToList("LONGPOLL detected a new block.", LONGPOLL, getTime(line));
            else if (line.contains("Supported options:"))
                reportToList("Miner didn't start properly. Try checking your settings.", LOGERROR, NULL);
            else if (line.contains("The requested URL returned error: 403"))
                reportToList("Couldn't connect. Please check your username and password.", LOGERROR, NULL);
            else if (line.contains("HTTP request failed"))
                reportToList("Couldn't connect. Please check pool server and port.", LOGERROR, NULL);
            else if (line.contains("JSON-RPC call failed"))
                reportToList("Couldn't communicate with server. Retrying in 30 seconds.", LOGERROR, NULL);
            else if (line.contains("thread ") && line.contains("khash/s")) // Minerd speed
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
            else if (line.contains("(avg):") && line.contains("Kh/s")) // CGMiner speed
            {
                int speedStart = line.indexOf("(avg):")+6;
                int speedEnd = line.indexOf("Kh/s");
                QString threadSpeedstr = line.mid(speedStart, speedEnd-speedStart);
                double speed=0;
                speed = threadSpeedstr.toDouble();
                threadSpeed[0] = speed;
                updateSpeed();
            }
            else if (line.contains("GPU") && line.contains("khash/s")) // CUDAMiner speed
            {
                int speedStart = line.indexOf("hashes, ")+8;
                int speedEnd = line.indexOf(" khash/s");
                QString threadSpeedstr = line.mid(speedStart, speedEnd-speedStart);
                double speed=0;
                speed = threadSpeedstr.toDouble();
                threadSpeed[0] = speed;
                updateSpeed();
            }
        }
    }
}

void MiningPage::minerError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        reportToList("Miner failed to start. Make sure you have the executable and libraries in the same directory as PlusEVCoin-qt.\n", LOGERROR, NULL);
    }
}

void MiningPage::minerFinished()
{
    if (getMiningType() == ClientModel::InternalMining)
        reportToList("Solo (internal) mining stopped.", LOGERROR, NULL);
    else
        reportToList("Miner exited.", LOGERROR, NULL);
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
            reportToList("Internal (solo) mining started. With internal mining you will see no output in this box.", LOGERROR, NULL);
        }
        else
        {
            reportToList("Miner started. You might not see any output for a few moments.", STARTED, NULL);
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

    QString speedString = QString("%1").arg(totalSpeed);
    QString threadsString = QString("%1").arg(initThreads);
    QString acceptedString = QString("%1").arg(acceptedShares);
    QString rejectedString = QString("%1").arg(rejectedShares);

    if (totalThreads == initThreads)
        ui->mineSpeedLabel->setText(QString("Speed: %1 khash/sec - %2 thread(s)").arg(speedString, threadsString));
    else
        ui->mineSpeedLabel->setText(QString("Speed: ~%1 khash/sec - %2 thread(s)").arg(speedString, threadsString));

    ui->shareCount->setText(QString("Accepted: %1 - Rejected: %2").arg(acceptedString, rejectedString));

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
            updateSpeed();
            break;

        case SHARE_FAIL:
            rejectedShares++;
            updateSpeed();
            break;

        case LONGPOLL:
            //roundAcceptedShares = 0;
            //roundRejectedShares = 0;
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

void MiningPage::enableBaseControls(bool enable)
{
    //ui->typeBox->setEnabled(enable);
    //ui->threadsBox->setEnabled(enable);
    //ui->scantimeBox->setEnabled(enable);
    ui->serverLine->setEnabled(enable);
    ui->portLine->setEnabled(enable);
    ui->usernameLine->setEnabled(enable);
    ui->passwordLine->setEnabled(enable);
}

void MiningPage::enableMinerdControls(bool enable)
{
    ui->threadsBox->setEnabled(enable);
    ui->scantimeBox->setEnabled(enable);
}

void MiningPage::enableCUDAMinerControls(bool enable)
{
    ui->scantimeBox->setEnabled(enable);
    ui->launchConfig->setEnabled(enable);
    ui->textureCache->setEnabled(enable);
    ui->offLoadSHA->setEnabled(enable);
    ui->memoryBlock->setEnabled(enable);
    ui->autotune->setEnabled(enable);
}

void MiningPage::enableCGMinerControls(bool enable)
{
    ui->threadsBox->setEnabled(enable);
    ui->intensity->setEnabled(enable);
    ui->concurrency->setEnabled(enable);
    ui->workload->setEnabled(enable);
}

void MiningPage::setDefaults()
{
    //ui->threadsBox->setValue(2);
    //ui->scantimeBox->setValue(8);
    ui->serverLine->setText("127.0.0.1");
    ui->portLine->setText("5252");
    ui->usernameLine->setText("plusevcoin");
    ui->passwordLine->setText("pev");
    ui->debugCheckBox->setChecked(true);
    //ui->intensityBox->setText("12");
}


ClientModel::MiningType MiningPage::getMiningType()
{
    if (ui->typeBox->currentIndex() == 0) // Solo2 Mining
    {
        return ClientModel::Solo2Mining;
    }
    else if (ui->typeBox->currentIndex() == 1) // Poool Mining
    {
        return ClientModel::PoolMining;
    }
    else if (ui->typeBox->currentIndex() == 2)  // P2P Mining
    {
        return ClientModel::P2PMining;
    }
    if (ui->typeBox->currentIndex() == 3)  // Internal Mining
    {
        return ClientModel::InternalMining;
    }
    return ClientModel::InternalMining;
}

ClientModel::MinerType MiningPage::getMinerType()
{
    if(ui->minerBox->currentIndex() == 0) // Minerd [CPU]
    {
        return ClientModel::Minerd;
    } 
    else if(ui->minerBox->currentIndex() == 1) // CGMiner [AMD]
    {
        return ClientModel::CGMiner;
    }
    else if(ui->minerBox->currentIndex() == 2) // CUDA Miner [nVidia]
    {
        return ClientModel::CUDAMiner;
    }
    return ClientModel::Minerd;
}

void MiningPage::typeChanged(int index)
{
    if (getMiningType() == ClientModel::Solo2Mining) // Solo2 Mining
    {
        ui->minerBox->setEnabled(true);
        enableBaseControls(true);
        minerChanged(-1);
    }
    else if (getMiningType() == ClientModel::PoolMining) // Poool Mining
    {
        ui->minerBox->setEnabled(true);
        enableBaseControls(true);
        minerChanged(-1);
    }
    else if (getMiningType() == ClientModel::P2PMining) // P2P Mining
    {
        ui->minerBox->setEnabled(true);
        enableBaseControls(true);
        minerChanged(-1);
    }
    else if (getMiningType() == ClientModel::InternalMining) // Internal Mining
    {
        ui->minerBox->setEnabled(false);
        enableBaseControls(false);
        enableMinerdControls(false);
        enableCUDAMinerControls(false);
        enableCGMinerControls(false);
    }
}

void MiningPage::minerChanged(int index)
{
    if(getMinerType() == ClientModel::Minerd) // Minerd [CPU]
    {
        enableCUDAMinerControls(false);
        enableCGMinerControls(false);
        enableMinerdControls(true);
    } 
    else if(getMinerType() == ClientModel::CGMiner) // CGMiner [AMD]
    {
        enableMinerdControls(false);
        enableCUDAMinerControls(false);
        enableCGMinerControls(true);
    }
    else if(getMinerType() == ClientModel::CUDAMiner) // CUDA Miner [nVidia]
    {
        enableMinerdControls(false);
        enableCGMinerControls(false);
        enableCUDAMinerControls(true);
    }
}

void MiningPage::debugToggled(bool checked)
{
    model->setMiningDebug(checked);
}

void MiningPage::resetMiningButton()
{
    ui->startButton->setText(minerActive ? "Stop Mining" : "Start Mining");
    if(minerActive)
    {
        ui->typeBox->setEnabled(false);
        ui->minerBox->setEnabled(false);
        enableBaseControls(false);
        enableMinerdControls(false);
        enableCUDAMinerControls(false);
        enableCGMinerControls(false);
    }
    else
    {
        ui->typeBox->setEnabled(true);
        typeChanged(-1);
    }
}

