#include "mainwindow.h"

SavedItems saved;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("rkTimer");
    resize(350,500);
    mainWidget = new QWidget();
    mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);

    timer = new QLabel();
    timer->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    timerFont = timer->font();
    timerFont.setPointSize(70);
    timer->setFont(timerFont);
    timer->setText("00:00");
    timer->setHidden(true);
    running = false;

    startStopBtn = new QPushButton();
    startStopFont = startStopBtn->font();
    startStopFont.setPointSize(18);
    startStopBtn->setFont(startStopFont);
    startStopBtn->setText("Start");
    startStopBtn->setFixedSize(160, 60);
    connect(startStopBtn, SIGNAL(clicked()), this, SLOT(startStop()));

    inputTask = new QLineEdit();
    inputTask->setFixedSize(200, 40);
    taskFont = inputTask->font();
    taskFont.setPointSize(13);
    inputTask->setFont(taskFont);
    inputTask->setAlignment(Qt::AlignCenter);

    buildMenuBar();
    createLayout();
    startTimer(0);
    setCentralWidget(mainWidget);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createLayout() {
    mainLayout->addWidget(timer);
    mainLayout->addWidget(inputTask, 0, Qt::AlignCenter | Qt::AlignTop);
    mainLayout->addWidget(startStopBtn, 0, Qt::AlignCenter | Qt::AlignTop);
}

void MainWindow::buildMenuBar() {
    fileMenu = menuBar()->addMenu("&File");
    quitAction = new QAction("Quit", this);
    quitAction->setShortcuts(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(quitAction);
    editMenu = menuBar()->addMenu("Edit");
    clearAction = new QAction("Clear All", this);
    // TODO connect clearAction Slot
    editMenu->addAction(clearAction);
}

void MainWindow::startStop() {
    if (running) {
        running = false;
        timer->setHidden(true);
        currentTimer = timer->text().toStdString();
        saved.thisTask = inputTask->text().toStdString();
        saved.stopTime = QDateTime::currentSecsSinceEpoch();
        savedTimes.push_back(saved);
        timer->setText("00:00");
        inputTask->setText("");
        inputTask->setReadOnly(false);
        startStopBtn->setText("Start");
    } else {
        timer->setHidden(false);
        saved.startTime = QDateTime::currentSecsSinceEpoch();
        beginTime = QTime::currentTime();
        running = true;
        inputTask->setReadOnly(true);
        startStopBtn->setText("Stop");
    }
}

void MainWindow::timerEvent(QTimerEvent *) {
    if (running) {
        const QChar zero = '0';
        qint64 ms = beginTime.msecsTo(QTime::currentTime());
        int h = ms / 1000 / 60 / 60;
        int m = (ms / 1000 / 60) - (h * 60);
        int s = (ms / 1000) - (m * 60);
        ms = ms - (s * 1000) - (m * 60000) - (h * 3600000);
        if (h > 0) {
            diff = QString("%1:%2:%3").
                                arg(h, 2, 10, zero).
                                arg(m, 2, 10, zero).
                                arg(s, 2, 10, zero);
        } else {
            diff = QString("%1:%2").
                                arg(m, 2, 10, zero).
                                arg(s, 2, 10, zero);
        }
        timer->setText(diff);
    }
}
