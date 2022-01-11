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

class SavedItems {

public:
    std::string thisTask;
    qint64 startTime;
    qint64 stopTime;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void startStop();

private:
    // Variables
    bool running;
    std::vector<SavedItems> savedTimes;
    QString diff;
    std::string currentTimer;

    // Widgets
    QWidget *mainWidget;
    QLabel *timer;
    QFont timerFont;
    QPushButton *startStopBtn;
    QFont startStopFont;
    QLineEdit *inputTask;
    QFont taskFont;

    // Menu
    QMenu *fileMenu;
    QMenu *editMenu;

    // Actions
    QTime beginTime;
    QAction *quitAction;
    QAction *clearAction;

    // Layouts
    QVBoxLayout *mainLayout;

    // Functions
    void createLayout();
    void timerEvent(QTimerEvent *);
    void buildMenuBar();
    std::string getCurrentTime();
    std::string convertToString(char* a, int size);

    // Classes

};

#endif // MAINWINDOW_H
