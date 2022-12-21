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

    void setModel(const ModelData& model, const Skeleton& skeleton, const AnimationData& anim);

public slots:
    void displaySkeleton(bool display);
    void displayPose(bool display);

protected:
    void init() override;
    void draw() override;

private:
    std::vector<Eigen::Matrix4f> buildBoneMats(Seconds at) const;
    void drawBones(std::optional<Seconds> at = std::nullopt);

    SafeGl::VertexArray _vao;
    SafeGl::Buffer _vertex_buffer;
    SafeGl::Buffer _index_buffer;
    SafeGl::Program _program;
    Skeleton _skeleton;
    AnimationData _anim;
    std::size_t _size = 0;

    bool _display_skeleton = false;
    bool _display_pose = false;
};

#endif
