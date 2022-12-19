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
    viewer->setModel(get<ModelData>(result), get<Skeleton>(result), get<AnimationData>(result));
}
