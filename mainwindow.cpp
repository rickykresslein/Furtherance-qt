#include "mainwindow.h"

SavedItems saved;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("rkTimer");
    resize(350,1000);
    mainWidget = new QWidget();
    mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);
    listOfTasksLayout = new QVBoxLayout();

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
    mainLayout->addLayout(listOfTasksLayout);
    mainLayout->addWidget(timer);
    mainLayout->addWidget(inputTask, 0, Qt::AlignCenter | Qt::AlignTop);
    mainLayout->addWidget(startStopBtn, 0, Qt::AlignCenter | Qt::AlignTop);
}

void MainWindow::clearLayout(QLayout *layout) {
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
           delete item->widget();
        }
        delete item;
    }
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
        saved.stopTime = QDateTime::currentDateTime();
        savedTimes.push_back(saved);
        clearLayout(listOfTasksLayout);
        sortSavedByDay();
        timer->setText("00:00");
        inputTask->setText("");
        inputTask->setReadOnly(false);
        startStopBtn->setText("Start");
    } else {
        timer->setHidden(false);
        saved.startTime = QDateTime::currentDateTime();
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

void MainWindow::sortSavedByDay() {
    tasksByDay.clear();
    for (SavedItems stored : savedTimes) {
        dayAndMonth = stored.startTime.toString("MMM d").toStdString();
        bool daysMatch = false;
        for (size_t i = 0; i < tasksByDay.size(); i++) {
            if (tasksByDay[i][0].startTime.toString("MMM d").toStdString() == dayAndMonth) {
                tasksByDay[i].push_back(stored);
                daysMatch = true;
                break;
            }
        } if (!daysMatch) {
            std::vector<SavedItems> taskByDay;
            taskByDay.push_back(stored);
            tasksByDay.push_back(taskByDay);
        }
    }
    createDayOverview();
}

void MainWindow::createDayOverview() {
    std::vector<Section*> allDays;
    std::vector<QVBoxLayout*> dailyTaskLayouts;
    for (std::vector<SavedItems> tasksThisDay : tasksByDay) {
        // Create Day List
        dayAndMonth = tasksThisDay[0].startTime.toString("MMM d").toStdString();
        if (QDateTime::currentDateTime().toString("MMM d").toStdString() == dayAndMonth) {
            dayAndMonth = "Today";
        } // TODO add Yesterday

        // Day Label shows "date - total time recorded that day".
        int differenceInSecs;
        int totalDailyTime = 0;
        for (SavedItems currTask : tasksThisDay) {
            differenceInSecs = currTask.stopTime.toSecsSinceEpoch() - currTask.startTime.toSecsSinceEpoch();
            totalDailyTime += differenceInSecs;
        }
        std::string dayLabel = dayAndMonth + " - " +
                QDateTime::fromSecsSinceEpoch( totalDailyTime, Qt::UTC ).toString( "hh:mm:ss" ).toStdString();
        Section *section = new Section(QString::fromStdString(dayLabel));
        allDays.push_back(section);

        // Create Task List sorted by day
        getSimilarTasks(tasksThisDay);
        QVBoxLayout *dayTaskLayout = new QVBoxLayout;
        for (std::vector<SavedItems> thisTaskByTask : tasksByTask) {
            std::string taskName = thisTaskByTask[0].thisTask;
            // Task label shows total time recorded for that task that day
            int totalTaskTime = 0;
            for (SavedItems currTask : thisTaskByTask) {
                differenceInSecs = currTask.stopTime.toSecsSinceEpoch() - currTask.startTime.toSecsSinceEpoch();
                totalTaskTime += differenceInSecs;
            }
            std::string taskLabel = taskName + " - " +
                    QDateTime::fromSecsSinceEpoch( totalTaskTime, Qt::UTC ).toString( "hh:mm:ss" ).toStdString();
            Section *section = new Section(QString::fromStdString(taskLabel));
            // TODO add TableWidget of task times to task list
            // setContentLayout of dayTaskLayout to TableWidget
            taskTable = new QTableWidget();
            taskTableLayout = new QVBoxLayout();
            taskTable->setColumnCount(3);
            QStringList horzHeaders;
            horzHeaders << "Time" << "Start" << "End";
            taskTable->setHorizontalHeaderLabels(horzHeaders);
            taskTable->verticalHeader()->setVisible(false);
            taskTable->setRowCount(0);
            for (SavedItems currTask : thisTaskByTask) {
                int row = taskTable->rowCount();
                taskTable->setRowCount(row+1);
                differenceInSecs = currTask.stopTime.toSecsSinceEpoch() - currTask.startTime.toSecsSinceEpoch();
                std::string totalTime = QDateTime::fromSecsSinceEpoch( differenceInSecs, Qt::UTC ).toString( "hh:mm:ss" ).toStdString();
                cell = new QTableWidgetItem(QString::fromStdString(totalTime));
                taskTable->setItem(row, 0, cell);
                std::string fmtStartTime = currTask.startTime.toString("hh:mm::ss").toStdString();
                cell = new QTableWidgetItem(QString::fromStdString(fmtStartTime));
                taskTable->setItem(row, 1, cell);
                std::string fmtStopTime = currTask.stopTime.toString("hh:mm::ss").toStdString();
                cell = new QTableWidgetItem(QString::fromStdString(fmtStopTime));
                taskTable->setItem(row, 2, cell);
            }
            taskTableLayout->addWidget(taskTable);
            section->setContentLayout(*taskTableLayout);
            dayTaskLayout->addWidget(section);
        }
        dailyTaskLayouts.push_back(dayTaskLayout);
    }
    for (size_t i = 0; i < allDays.size(); i++) {
        allDays[i]->setContentLayout(*dailyTaskLayouts[i]);
        listOfTasksLayout->addWidget(allDays[i]);
        if (allDays[i]->getTitle().toStdString().substr(0,5) == "Today") {
            allDays[i]->toggle(true);
        }
    }
}

void MainWindow::getSimilarTasks(std::vector<SavedItems> tasksThisDay) {
    tasksByTask.clear();
    for (SavedItems stored : tasksThisDay) {
        std::string taskName = stored.thisTask;
        bool daysMatch = false;
        for (size_t i = 0; i < tasksByTask.size(); i++) {
            if (tasksByTask[i][0].thisTask == taskName) {
                tasksByTask[i].push_back(stored);
                daysMatch = true;
                break;
            }
        } // TODO Check if this works because I likely have to modify it like sortSavedByDay
        if (!daysMatch) {
            std::vector<SavedItems> thisTaskByTask;
            thisTaskByTask.push_back(stored);
            tasksByTask.push_back(thisTaskByTask);
        }
    }
}
