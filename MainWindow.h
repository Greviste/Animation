#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "AnimationListModel.h"
#include "AnimationTreeModel.h"
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
    void on_set_simple_anim_button_clicked();
    void on_set_anim_button_clicked();
    void on_anim_tree_view_clicked(const QModelIndex& index);
    void on_anim_tree_view_customContextMenuRequested(QPoint pos);
    void on_blending_slider_valueChanged(int v);

private:
    void updateText();

    AnimationListModel _animations;
    std::unique_ptr<AnimationTreeModel> _tree;
    bool _use_tree = false;
};

#endif
