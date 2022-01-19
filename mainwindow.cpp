#include "mainwindow.h"

SavedItems saved;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Furtherance");
    resize(430,700);
    mainWidget = new QWidget();
    mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);
    listOfTasksLayout = new QVBoxLayout();

    databaseConnect();
    databaseInit();
    databaseRead();

    taskListTree = new QTreeWidget();
    taskListTree->setUniformRowHeights(true);
    taskListTree->setHeaderLabels(QStringList() << tr("Task") << tr("Time") << tr("Start") << tr("End") << "ID");
    taskListTree->header()->resizeSection(0, 170);
    taskListTree->header()->resizeSection(1, 70);
    taskListTree->header()->resizeSection(2, 85);
    taskListTree->header()->resizeSection(3, 85);
    taskListTree->hideColumn(4);
    connect(taskListTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemEdited(QTreeWidgetItem*, int)));
    // Add context menu to taskListTree
    taskListMenu = new QMenu("Task menu", this);
    deleteItemAction = new QAction("Delete");
    connect(deleteItemAction, SIGNAL(triggered()), this, SLOT(deleteTask()));
    taskListMenu->addAction(deleteItemAction);
    taskListTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(taskListTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(taskListContextMenu(const QPoint &)));

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
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearDatabase()));
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
        saved.id = databaseGetLastID();
        savedTimes.push_back(saved);
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
    taskListTree->clear();
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
    taskListTree->blockSignals(true);
    std::vector<QVBoxLayout*> dailyTaskLayouts;
    for (size_t i = tasksByDay.size(); i-- > 0; ) {
        // Create Day List
        dayAndMonth = tasksByDay[i][0].startTime.toString("MMM d").toStdString();
        if (QDateTime::currentDateTime().toString("MMM d").toStdString() == dayAndMonth) {
            dayAndMonth = "Today";
        } else if (QDateTime::currentDateTime().addDays(-1).toString("MMM d").toStdString() == dayAndMonth) {
            dayAndMonth = "Yesterday";
        }

        // Day Label shows "date - total time recorded that day".
        int differenceInSecs;
        int totalDailyTime = 0;
        for (SavedItems currTask : tasksByDay[i]) {
            differenceInSecs = getTimeDifference(currTask.stopTime, currTask.startTime);
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
                differenceInSecs = getTimeDifference(currTask.stopTime, currTask.startTime);
                totalTaskTime += differenceInSecs;
                QTreeWidgetItem *sameTaskItem = new QTreeWidgetItem(taskItem);
                sameTaskItem->setFlags(sameTaskItem->flags() | Qt::ItemIsEditable);
                sameTaskItem->setText(1, QDateTime::fromSecsSinceEpoch(differenceInSecs, Qt::UTC).toString("h:mm:ss"));
                sameTaskItem->setText(2, currTask.startTime.toString("hh:mm:ss"));
                sameTaskItem->setText(3, currTask.stopTime.toString("hh:mm:ss"));
                sameTaskItem->setText(4, QString::number(currTask.id));
                sameTaskItemsVec.append(sameTaskItem);
            }
            taskItem->setText(0, QString::fromStdString(taskName));
            taskItem->setText(1, QDateTime::fromSecsSinceEpoch( totalTaskTime, Qt::UTC ).toString("h:mm:ss"));
            taskListTree->addTopLevelItems(sameTaskItemsVec);
        }
        if (dayAndMonth == "Today") {
            taskListTree->expandItem(dayItem);
        }
    }
    // Move expansions to here so they are not repeated for every day
    // User findItems to find "Today" to expand it.
    // Actually this is not necessary. They are not really repeated, just the check on the if's
    taskListTree->blockSignals(false);
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
        }
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

        #ifdef _WIN32
            QString furtheranceDir = QDir::homePath() + QDir::separator() + "Furtherance";
        #elif __APPLE__
            QString furtheranceDir = QDir::homePath() + QDir::separator() + ".Furtherance";
        #else
            QString furtheranceDir = QDir::homePath() + QDir::separator()+ ".config" + QDir::separator() + "Furtherance";
        #endif
        QDir dir(furtheranceDir);
        if (!dir.exists()){
          dir.mkpath(furtheranceDir);
        }
        QString timerDbPath = furtheranceDir + QDir::separator() + "furtherance.db";
        db.setDatabaseName(timerDbPath);

        if(!db.open())
            qWarning() << "MainWindow::databaseConnect - ERROR: " << db.lastError().text();
    }
    else
        qWarning() << "MainWindow::databaseConnect - ERROR: no driver " << DRIVER << " available";
}

void MainWindow::databaseInit()
{
    QSqlQuery query("CREATE TABLE tasks (id INTEGER PRIMARY KEY, taskName TEXT, startTime TIMESTAMP, stopTime TIMESTAMP)");

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
        qWarning() << "MainWindow::databasePopulate - ERROR: " << query.lastError().text();
}

void MainWindow::databaseRead()
{
    QSqlQuery query("SELECT * FROM tasks ORDER BY startTime");
    while (query.next())
    {
       saved.thisTask = query.value("taskName").toString().toStdString();
       saved.startTime = query.value("startTime").toDateTime();
       saved.stopTime = query.value("stopTime").toDateTime();
       saved.id = query.value("id").toInt();;
       savedTimes.push_back(saved);
    }
}

int MainWindow::databaseGetLastID()
{
    QSqlQuery query;
    query.prepare("SELECT MAX(id) FROM tasks");
    if(!query.exec())
        qWarning() << "MainWindow::databaseGetLastID - ERROR: " << query.lastError().text();
    query.next();
    return query.value(0).toInt();
}

void MainWindow::itemEdited(QTreeWidgetItem * item, int column)
{
    QSqlQuery query;
    QSqlQuery queryWrite;
    int currID = item->text(4).toInt();
    query.prepare("SELECT * FROM tasks WHERE ID = :id");
    query.bindValue(":id", currID);
    if(!query.exec())
        qWarning() << "MainWindow::itemEdited - ERROR: " << query.lastError().text();
    query.next();
    if (column == 2 || column == 3) {
        // Get current column data.
        QDateTime currData = query.value(column).toDateTime();
        // Convert current data to yyyy-MM-dd
        QString currTimeString = currData.toString("yyyy-MM-dd");
        // create a string that assembles yyyy-MM-dd of existing with hh:mm:ss of new"
        QString newData = item->text(column);
        int newDataLength = newData.toStdString().length();
        QString concatString;
        if (newDataLength == 7) {
            concatString = currTimeString + " 0" + item->text(column);
        } else if (newDataLength == 4){
            concatString = currTimeString + " 0" + item->text(column) + ":00";
        } else if (newDataLength == 5){
            concatString = currTimeString + " " + item->text(column) + ":00";
        } else {
            concatString = currTimeString + " " + item->text(column);
        }
        QDateTime editedTime = QDateTime::fromString(concatString, "yyyy-MM-dd hh:mm:ss");

        // User cannot enter an invalid time, a time greater than the stop time, smaller than the stop time,
        // or a time later than the current time.
        int editedTimeInSecs = editedTime.toSecsSinceEpoch();
        int timeNow = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (editedTime.isValid() &&
                ((column == 2 && editedTimeInSecs < query.value(3).toDateTime().toSecsSinceEpoch()) ||
                  (column == 3 && editedTimeInSecs > query.value(2).toDateTime().toSecsSinceEpoch())) &&
                editedTimeInSecs < timeNow)
        {
                // Write to database
                if (column == 2) {
                    queryWrite.prepare("UPDATE tasks SET startTime = :value WHERE id = :id ");
                } else {
                    queryWrite.prepare("UPDATE tasks SET stopTime = :value WHERE id = :id ");
                }
                queryWrite.bindValue(":id", currID);
                queryWrite.bindValue(":value", editedTime);
                if(!queryWrite.exec())
                    qWarning() << "MainWindow::DatabasePopulate - ERROR: Write New" << queryWrite.lastError().text();
        }
    } else if (column == 0) {
        QString editedTaskName = item->text(column);
        if (!isWhitespace(editedTaskName.toStdString())) {
            queryWrite.prepare("UPDATE tasks SET taskName = :value WHERE id = :id ");
            queryWrite.bindValue(":id", currID);
            queryWrite.bindValue(":value", editedTaskName);
            if(!queryWrite.exec())
                qWarning() << "MainWindow::DatabasePopulate - ERROR: Write Task Name" << queryWrite.lastError().text();
        }
    }

    // Rebuild list
    refreshTaskList();
    QList<QTreeWidgetItem *> editedItemVec = taskListTree->findItems(QString::number(currID), Qt::MatchExactly|Qt::MatchRecursive, 4);
    QTreeWidgetItem *editedItemParent = editedItemVec[0]->parent();
    QTreeWidgetItem *editedItemParentOfParent = editedItemParent->parent();
    taskListTree->expandItem(editedItemParentOfParent);
    taskListTree->expandItem(editedItemParent);
}

int MainWindow::getTimeDifference(QDateTime stopTime, QDateTime startTime) {
    return stopTime.toSecsSinceEpoch() - startTime.toSecsSinceEpoch();
}

void MainWindow::taskListContextMenu(const QPoint &point)
{
    QTreeWidgetItem *item = taskListTree->itemAt(point);
    if (item) {
        if (item->childCount() == 0) {
            taskListMenu->exec(taskListTree->viewport()->mapToGlobal(point));
        }
    }
}

void MainWindow::deleteTask()
{
    QTreeWidgetItem *currentSelection = taskListTree->currentItem();
    int currentSelectionIndex = currentSelection->text(4).toInt();
    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE ID = :id");
    query.bindValue(":id", currentSelectionIndex);
    if(!query.exec())
        qWarning() << "MainWindow::deleteTask - ERROR: " << query.lastError().text();
    refreshTaskList();
}

// Thank you to Tyler Davis on Stack Overflow for this function.
bool MainWindow::isWhitespace(std::string s){
    for(size_t index = 0; index < s.length(); index++){
        if(!std::isspace(s[index]))
            return false;
    }
    return true;
}

void MainWindow::clearDatabase() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Delete Database"),
                                         tr("Are you sure you want to delete all saved data? If so, type DELETE."),
                                         QLineEdit::Normal,
                                         "", &ok);
    // If user entered DELETE (case insensitive) erase the database
    std::string textStr = text.toStdString();
    std::transform(textStr.begin(), textStr.end(),textStr.begin(), ::toupper);
    if (ok && textStr == "DELETE")
    {
        QSqlQuery query;
        query.prepare("DELETE FROM tasks");
        if(!query.exec())
            qWarning() << "MainWindow::clearDatabase - ERROR: " << query.lastError().text();
        refreshTaskList();
    }
    return;
}

void MainWindow::refreshTaskList() {
    savedTimes.clear();
    databaseRead();
    sortSavedByDay();
}
