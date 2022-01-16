#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <vector>
#include <string>
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
#include <QDebug>
#include <sqlite3.h>

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

private:
    // Variables
    bool running;
    std::vector<SavedItems> savedTimes;
    QString diff;
    std::string currentTimer;
    std::string dayAndMonth;
    std::vector<std::vector<SavedItems>> tasksByDay;
    std::vector<std::vector<SavedItems>> tasksByTask;


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

    // Actions
    QTime beginTime;
    QAction *quitAction;
    QAction *clearAction;

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

    void databaseConnect();
    void databaseInit();
    void databasePopulate();
    void databaseRead();

    // Classes

};

#endif // MAINWINDOW_H
