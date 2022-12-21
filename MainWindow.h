#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
public:
    MainWindow();

private slots:
    void on_open_action_triggered();
    void on_viewer_frameChanged(int f);
    void on_viewer_animLengthSet(int f);

private:
    void updateText();
};

#endif
