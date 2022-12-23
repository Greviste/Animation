#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include "Model.h"
#include "Animation.h"
#include "SafeGl.h"


class Viewer : public QGLViewer
{
    Q_OBJECT
public:
    using QGLViewer::QGLViewer;

    void setModel(std::unique_ptr<Model> model, std::unique_ptr<Animation> animation);

signals:
    void frameChanged(int);
    void animLengthSet(int);

public slots:
    void displaySkeleton(bool display);
    void displayPose(bool display);
    void setFrame(int f);
    void useLinearSkinning();
    void useDualQuatSkinning();

protected:
    void init() override;
    void draw() override;
    void startAnimation() override;
    void stopAnimation() override;
    void animate() override;

private:
    Seconds consumeTick();
    void drawBones(const Eigen::Matrix4f* bone_mats = nullptr);
    void handleModelChanged();

    std::unique_ptr<Model> _model;
    std::unique_ptr<Animation> _anim;

    bool _display_skeleton = false;
    bool _display_pose = false;

    Seconds _time;
    std::chrono::high_resolution_clock::time_point _last_tick;
};

#endif
