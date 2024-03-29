#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QPushButton>
#include <QTime>
#include <QDateTime>
#include <QMenuBar>
#include <QLineEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDebug>
#include <QInputDialog>
#include <QDir>
#include <QMessageBox>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QVariant>

class SavedItems {

public:
    std::string thisTask;
    QDateTime startTime;
    QDateTime stopTime;
    int id;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void startStop();
    void inputTaskChanged();
    void itemEdited(QTreeWidgetItem * item, int column);
    void taskListContextMenu(const QPoint &point);
    void deleteTask();
    void clearDatabase();
    void idleTimeReached();
    void resumeFromIdle();

private:
    // Variables
    bool running;
    std::vector<SavedItems> savedTimes;
    QString diff;
    std::string currentTimer;
    std::string dayAndMonth;
    std::vector<std::vector<SavedItems>> tasksByDay;
    std::vector<std::vector<SavedItems>> tasksByTask;
    bool subtractIdle = false;
    int idleTime;
    int idleStartTime;
    int notifyOfIdle;
    int storedIdleTime = 0;
    bool idleTimeReachedBool = false;
    bool idleNotified = false;

    bool kdePlasma = false;
    bool gnome = false;

    // Widgets
    QWidget *mainWidget;
    QLabel *timer;
    QFont timerFont;
    QPushButton *startStopBtn;
    QFont startStopFont;
    QLineEdit *inputTask;
    QFont taskFont;
    QTableWidget *taskTable;
    QTableWidgetItem *cell;
    QTreeWidget *taskListTree;
    // Menu
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *taskListMenu;

    // Actions
    QTime beginTime;
    QAction *quitAction;
    QAction *clearAction;
    QAction *deleteItemAction;

    // Layouts
    QVBoxLayout *mainLayout;
    QVBoxLayout *listOfTasksLayout;
    QVBoxLayout *taskTableLayout;

    // Functions
    void createLayout();
    void clearLayout(QLayout *layout);
    void timerEvent(QTimerEvent *);
    void buildMenuBar();
    std::string getCurrentTime();
    std::string convertToString(char* a, int size);
    void sortSavedByDay();
    void createDayOverview();
    void getSimilarTasks(std::vector<SavedItems> tasksThisDay);
    int getTimeDifference(QDateTime stopTime, QDateTime startTime);
    bool isWhitespace(std::string s);
    void refreshTaskList();
    void getDesktopEnv();
    int getGnomeIdleTime();

    void databaseConnect();
    void databaseInit();
    void databasePopulate();
    void databaseRead();
    int databaseGetLastID();

    // Classes

};

#endif // MAINWINDOW_H
