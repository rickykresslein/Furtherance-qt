#include "mainwindow.h"

SavedItems saved;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("rkTimer");
    resize(425,700);
    mainWidget = new QWidget();
    mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);
    listOfTasksLayout = new QVBoxLayout();

    databaseConnect();
    databaseInit();
    databaseRead();

    taskListTree = new QTreeWidget();
    taskListTree->setUniformRowHeights(true);
    taskListTree->setHeaderLabels(QStringList() << tr("Task") << tr("Time") << tr("Start") << tr("End"));
    taskListTree->header()->resizeSection(0, 170);
    taskListTree->header()->resizeSection(1, 70);
    taskListTree->header()->resizeSection(2, 85);
    taskListTree->header()->resizeSection(3, 85);

    timer = new QLabel();
    timer->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    timerFont = timer->font();
    timerFont.setPointSize(50);
    timer->setFont(timerFont);
    timer->setText("00:00");
    timer->setHidden(true);
    running = false;

    startStopBtn = new QPushButton();
    startStopFont = startStopBtn->font();
    startStopFont.setPointSize(14);
    startStopBtn->setFont(startStopFont);
    startStopBtn->setText("Start");
    startStopBtn->setFixedSize(160, 40);
    connect(startStopBtn, SIGNAL(clicked()), this, SLOT(startStop()));
    startStopBtn->setEnabled(false);

    inputTask = new QLineEdit();
    inputTask->setFixedSize(200, 40);
    taskFont = inputTask->font();
    taskFont.setPointSize(13);
    inputTask->setFont(taskFont);
    inputTask->setAlignment(Qt::AlignCenter);
    inputTask->setPlaceholderText("Task Name");
    connect(inputTask, SIGNAL(textChanged(QString)), this, SLOT(inputTaskChanged()));

    // Test Yesterday
//    saved.thisTask = "Task 1";
//    saved.startTime = QDateTime::currentDateTime().addDays(-1);
//    saved.stopTime = QDateTime::currentDateTime().addDays(-1).addSecs(65);
//    savedTimes.push_back(saved);
//    saved.thisTask = "Ydays Task";
//    saved.startTime = QDateTime::currentDateTime().addDays(-1);
//    saved.stopTime = QDateTime::currentDateTime().addDays(-1).addSecs(65);
//    savedTimes.push_back(saved);

    buildMenuBar();
    createLayout();
    startTimer(0);
    setCentralWidget(mainWidget);

    if (savedTimes.size() > 0) {
        sortSavedByDay();
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::createLayout() {
    mainLayout->addWidget(timer);
    mainLayout->addWidget(inputTask, 0, Qt::AlignCenter | Qt::AlignTop);
    mainLayout->addWidget(startStopBtn, 0, Qt::AlignCenter | Qt::AlignTop);
    mainLayout->addWidget(taskListTree);

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
        databasePopulate();
        savedTimes.push_back(saved);
        taskListTree->clear();
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
    std::vector<QVBoxLayout*> dailyTaskLayouts;
    for (size_t i = tasksByDay.size(); i-- > 0; ) {
        // Create Day List
        dayAndMonth = tasksByDay[i][0].startTime.toString("MMM d").toStdString();
        if (QDateTime::currentDateTime().toString("MMM d").toStdString() == dayAndMonth) {
            dayAndMonth = "Today";
        } else if (QDateTime::currentDateTime().addDays(-1).toString("MMM d").toStdString() == dayAndMonth) {
            dayAndMonth = "Yesterday";
        }
        // TODO add Yesterday

        // Day Label shows "date - total time recorded that day".
        int differenceInSecs;
        int totalDailyTime = 0;
        for (SavedItems currTask : tasksByDay[i]) {
            differenceInSecs = currTask.stopTime.toSecsSinceEpoch() - currTask.startTime.toSecsSinceEpoch();
            totalDailyTime += differenceInSecs;
        }
        QTreeWidgetItem *dayItem = new QTreeWidgetItem(taskListTree);
        dayItem->setText(0, QString::fromStdString(dayAndMonth));
        dayItem->setText(1, QDateTime::fromSecsSinceEpoch(totalDailyTime, Qt::UTC).toString("h:mm:ss"));

        // Create Task List sorted by day
        getSimilarTasks(tasksByDay[i]);
        for (std::vector<SavedItems> thisTaskByTask : tasksByTask) {
            std::string taskName = thisTaskByTask[0].thisTask;
            // Task label shows total time recorded for that task that day
            QTreeWidgetItem *taskItem = new QTreeWidgetItem(dayItem);
            QList<QTreeWidgetItem *> sameTaskItemsVec;
            int totalTaskTime = 0;
            for (SavedItems currTask : thisTaskByTask) {
                differenceInSecs = currTask.stopTime.toSecsSinceEpoch() - currTask.startTime.toSecsSinceEpoch();
                totalTaskTime += differenceInSecs;
                // TODO Here - add each item in this task to the taskListTree under the taskItem task
                QTreeWidgetItem *sameTaskItem = new QTreeWidgetItem(taskItem);
                sameTaskItem->setText(1, QDateTime::fromSecsSinceEpoch(differenceInSecs, Qt::UTC).toString("h:mm:ss"));
                sameTaskItem->setText(2, currTask.startTime.toString("h:mm:ss ap"));
                sameTaskItem->setText(3, currTask.stopTime.toString("h:mm:ss ap"));
                sameTaskItemsVec.append(sameTaskItem);
            }
            taskItem->setText(0, QString::fromStdString(taskName));
            taskItem->setText(1, QDateTime::fromSecsSinceEpoch( totalTaskTime, Qt::UTC ).toString("h:mm:ss"));
            taskListTree->addTopLevelItems(sameTaskItemsVec);//TODO should add sameTaskItem
        }
        if (dayAndMonth == "Today") {
            taskListTree->expandItem(dayItem);
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

void MainWindow::inputTaskChanged() {
    if (inputTask->text() != "") {
        startStopBtn->setEnabled(true);
    } else {
        startStopBtn->setEnabled(false);
    }
}

void MainWindow::databaseConnect()
{
    const QString DRIVER("QSQLITE");

    if(QSqlDatabase::isDriverAvailable(DRIVER))
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(DRIVER);

        db.setDatabaseName("timer.db");

        if(!db.open())
            qWarning() << "MainWindow::DatabaseConnect - ERROR: " << db.lastError().text();
    }
    else
        qWarning() << "MainWindow::DatabaseConnect - ERROR: no driver " << DRIVER << " available";
}

void MainWindow::databaseInit()
{
    QSqlQuery query("CREATE TABLE tasks (id INTEGER PRIMARY KEY, taskName TEXT, startTime TIMESTAMP, stopTime TIMESTAMP)");

//    if(!query.isActive())
//        qWarning() << "MainWindow::DatabaseInit - ERROR: " << query.lastError().text();

}

void MainWindow::databasePopulate()
{
    QSqlQuery query;

    query.prepare("INSERT INTO tasks (taskName, startTime, stopTime) "
                  "VALUES(:taskName, :startTime, :stopTime)");
    query.bindValue(":taskName", QString::fromStdString(saved.thisTask));
    query.bindValue(":startTime", saved.startTime);
    query.bindValue(":stopTime", saved.stopTime);

    if(!query.exec())
        qWarning() << "MainWindow::DatabasePopulate - ERROR: " << query.lastError().text();
}

void MainWindow::databaseRead()
{
    QSqlQuery query("SELECT * FROM tasks");
    while (query.next())
    {
       saved.thisTask = query.value("taskName").toString().toStdString();
       saved.startTime = query.value("startTime").toDateTime();
       saved.stopTime = query.value("stopTime").toDateTime();
       savedTimes.push_back(saved);
    }
}
