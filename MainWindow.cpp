#include "MainWindow.h"
#include <QFileDialog>
#include "Loader.h"


MainWindow::MainWindow()
{
    setupUi(this);
}

void MainWindow::on_open_action_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, "Open FBX file", "", "FBX files (*.fbx)");
    auto result = decodeFbx(file.toLocal8Bit().data());
    viewer->setModel(std::make_unique<Model>(get<0>(result)), std::make_unique<Animation>(get<1>(result)[0]));
}

void MainWindow::on_viewer_frameChanged(int f)
{
    frame_slider->setValue(f);
    updateText();
}

void MainWindow::on_viewer_animLengthSet(int f)
{
    frame_slider->setRange(0, f-1);
    updateText();
}

void MainWindow::updateText()
{
    frame_label->setText(tr("Frame %1/%2").arg(frame_slider->value()).arg(frame_slider->maximum()));
}
