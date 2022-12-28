#include "MainWindow.h"
#include <QFileDialog>
#include "Loader.h"


MainWindow::MainWindow()
{
    setupUi(this);
    anim_list_view->setModel(&_animations);
}

void MainWindow::on_open_action_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, "Open FBX file", "", "FBX files (*.fbx)");
    if(file.isEmpty())
        return;
    
    _animations.clear();
    auto result = decodeFbx(file.toLocal8Bit().data());

    for(auto& data_ptr : get<1>(result))
    {
        _animations.addAnimation(std::make_unique<SimpleAnimation>(data_ptr));
    }

    _tree = std::make_unique<AnimationTreeModel>(get<0>(result)->skeleton);
    anim_tree_view->setModel(_tree.get());
    viewer->setModel(std::make_unique<Model>(get<0>(result)));
    viewer->setAnimation(_animations[0]);
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

void MainWindow::on_set_simple_anim_button_clicked()
{
    for(const QModelIndex& index : anim_list_view->selectionModel()->selectedIndexes())
    {
        viewer->setAnimation(_animations[index.row()]);
        _use_tree = false;
        return;
    }
}

void MainWindow::on_set_anim_button_clicked()
{
    if(!_tree)
        return;
    _use_tree = true;
    viewer->setAnimation(_tree->getAnimation());
}

void MainWindow::on_anim_tree_view_clicked(const QModelIndex& index)
{
    Animation* animation = &_tree->getAnimation(index);
    BlendedAnimation* blended = dynamic_cast<BlendedAnimation*>(animation);
    if(!blended)
    {
        blending_slider->setEnabled(false);
        return;
    }
    blending_slider->setEnabled(true);
    blending_slider->blockSignals(true);
    blending_slider->setMaximum((blended->count() - 1) * 100);
    blending_slider->setValue(blended->blendFactor() * 100);
    blending_slider->blockSignals(false);
}

void MainWindow::on_anim_tree_view_customContextMenuRequested(QPoint pos)
{
    if(!_tree)
        return;

    QModelIndex index=anim_tree_view->indexAt(pos);
    QMenu* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction(tr("Replace with Null"), [this, index](){ _tree->replaceItem<NullAnimation>(index); if(_use_tree) viewer->setAnimation(_tree->getAnimation()); });
    menu->addAction(tr("Replace with Blended"), [this, index](){ _tree->replaceItem<BlendedAnimation>(index); if(_use_tree) viewer->setAnimation(_tree->getAnimation()); });
    menu->addAction(tr("Replace with Additive"), [this, index](){ _tree->replaceItem<AdditiveAnimation>(index); if(_use_tree) viewer->setAnimation(_tree->getAnimation()); });
    menu->addAction(tr("Replace with Selected"), [this, index]()
    {
        for(const QModelIndex& i : anim_list_view->selectionModel()->selectedIndexes())
        {
            _tree->replaceItem(index, std::make_unique<SimpleAnimation>(_animations[i.row()].data()));
            viewer->setAnimation(_tree->getAnimation());
            return;
        }
    });
    menu->addSeparator();
    menu->addAction(tr("Add child"), [this, index](){ _tree->addChild(index); });
    menu->addAction(tr("Remove from parent"), [this, index](){ _tree->removeItem(index); });
    menu->popup(anim_tree_view->viewport()->mapToGlobal(pos));
}

void MainWindow::on_blending_slider_valueChanged(int v)
{
    for(const QModelIndex& index : anim_tree_view->selectionModel()->selectedIndexes())
    {
        static_cast<BlendedAnimation&>(_tree->getAnimation(index)).setBlendFactor(v / 100.f);
        if(_use_tree) viewer->setAnimation(_tree->getAnimation(), false);
        return;
    }
}

void MainWindow::updateText()
{
    frame_label->setText(tr("Frame %1/%2").arg(frame_slider->value()).arg(frame_slider->maximum()));
}
