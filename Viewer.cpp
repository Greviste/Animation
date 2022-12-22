#include "Viewer.h"


namespace
{
//Needed for type inferrence because Eigen's function returns an intermediary type
Eigen::Matrix4f identity()
{
    return Eigen::Matrix4f::Identity();
}
}

void Viewer::drawBones(const Eigen::Matrix4f* bone_mats)
{
    glBegin(GL_POINTS);
    _model->data().skeleton->exploreTree(0, [&](BoneIndex index, Eigen::Matrix4f& parent_mat) {
        Transform transform = _model->data().skeleton->boneTransform(index);
        parent_mat.applyOnTheRight(transform.matrix());
        Eigen::Vector4f result, zero{0,0,0,1};
        result = parent_mat * zero;
        if(bone_mats) result = bone_mats[index] * result;
        glVertex3f(result.x(), result.y(), result.z());
    }, identity());
    glEnd();
}

void Viewer::setModel(std::unique_ptr<Model> model, std::unique_ptr<Animation> anim)
{
    _model = std::move(model);
    _anim = std::move(anim);
    handleModelChanged();
}

void Viewer::handleModelChanged()
{
    emit animLengthSet(_anim->duration().count());
    _frame = Frames{0};
    emit frameChanged(0);

    constexpr float inf = std::numeric_limits<float>::infinity();
    qglviewer::Vec min{inf,inf,inf}, max{-inf,-inf,-inf};
    for(const Vertex& v : _model->data().vertices)
    {
        for(int i = 0; i < 3; ++i)
        {
            max[i] = std::max<float>(max[i], v.pos[i]);
            min[i] = std::min<float>(min[i], v.pos[i]);
        }
    }
    qglviewer::Vec diff = max - min;
    camera()->setSceneRadius(std::max(std::max(diff[0], diff[1]), diff[2]));
    camera()->setSceneCenter((min + max) / 2);
    camera()->showEntireScene();

    updateGL();
}

void Viewer::displaySkeleton(bool display)
{
    _display_skeleton = display;
    updateGL();
}

void Viewer::displayPose(bool display)
{
    _display_pose = display;
    updateGL();
}

void Viewer::setFrame(int f)
{
    _frame = Frames{f};
    emit frameChanged(f);
    updateGL();
}

void Viewer::init()
{
    auto& gl = QGl();

    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthFunc(GL_LESS);
    gl.glEnable(GL_CULL_FACE);
    gl.glCullFace(GL_BACK);
    glPointSize(5);
}

void Viewer::draw()
{
    if(!_model) return;

    _model->draw(*camera(), _anim.get(), _frame);

    if(_display_skeleton || _display_pose)
    {
        auto& gl = QGl();
        gl.glDisable(GL_DEPTH_TEST);
        glColor3f(0,0,1);
        glBegin(GL_POINTS);
        glVertex3d(0,0,0);
        glEnd();
        if(_display_skeleton)
        {
            glColor3f(0,1,0);
            drawBones();
        }
        if(_anim && _display_pose)
        {
            glColor3f(1,0,0);
            auto bone_mats = _anim->buildBoneMats(_frame);
            drawBones(bone_mats.data());
        }
        gl.glEnable(GL_DEPTH_TEST);
    }
}

void Viewer::startAnimation()
{
    QGLViewer::startAnimation();
    _last_frame = std::chrono::high_resolution_clock::now();
}

void Viewer::stopAnimation()
{
    if(animationIsStarted()) //QGLViewer seems to try to delete its timer multiple times
        QGLViewer::stopAnimation();
}

void Viewer::animate()
{
    if(!_anim) return;

    auto now = std::chrono::high_resolution_clock::now();
    while(now - _last_frame > Frames{1})
    {
        if(++_frame >= _anim->duration()) _frame = Frames{0};
        _last_frame += duration_cast<decltype(_last_frame)::duration>(Frames{1});
        emit frameChanged(_frame.count());
    }
}
