#include "Viewer.h"


namespace
{
//Needed for type inferrence because Eigen's function returns an intermediary type
Eigen::Matrix4f identity()
{
    return Eigen::Matrix4f::Identity();
}
}

std::vector<Eigen::Matrix4f> Viewer::buildBoneMats(Seconds at) const
{
    std::vector<Eigen::Matrix4f> bone_mats(_skeleton.boneCount());
    
    _skeleton.exploreTree(0, [&](BoneIndex index, Eigen::Matrix4f& parent_mat_skinned, Eigen::Matrix4f& inv_parent_mat_unskinned) {
        Transform transform = _skeleton.boneTransform(index);
        inv_parent_mat_unskinned.applyOnTheLeft(transform.inverseMatrix());
        transform.rotation *= _anim.curves[index].sample(at);
        parent_mat_skinned.applyOnTheRight(transform.matrix());
        bone_mats[index] = parent_mat_skinned * inv_parent_mat_unskinned;
    }, identity(), identity());

    return bone_mats;
}

void Viewer::drawBones(std::optional<Seconds> at)
{
    glBegin(GL_POINTS);
    _skeleton.exploreTree(0, [&](BoneIndex index, Eigen::Matrix4f& parent_mat) {
        Transform transform = _skeleton.boneTransform(index);
        if(at) transform.rotation *= _anim.curves[index].sample(*at);
        parent_mat.applyOnTheRight(transform.matrix());
        Eigen::Vector4f result, zero{0,0,0,1};
        result = parent_mat * zero;
        glVertex3f(result.x(), result.y(), result.z());
    }, identity());
    glEnd();
}

void Viewer::setModel(const ModelData& model, const Skeleton& skeleton, const AnimationData& anim)
{
    _skeleton = skeleton;
    _anim = anim;
    constexpr float inf = std::numeric_limits<float>::infinity();
    qglviewer::Vec min{inf,inf,inf}, max{-inf,-inf,-inf};
    for(const Vertex& v : model.vertices)
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
    auto& gl = *QOpenGLContext::currentContext()->extraFunctions();
    gl.glBindVertexArray(_vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);

    std::size_t stride = sizeof(Vertex);
    gl.glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * stride, model.vertices.data(), GL_STATIC_DRAW);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    gl.glVertexAttribIPointer(2, 4, GL_UNSIGNED_SHORT, stride, (void*)(6 * sizeof(float)));
    gl.glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float) + 4 * sizeof(BoneIndex)));
    gl.glEnableVertexAttribArray(0);
    gl.glEnableVertexAttribArray(1);
    gl.glEnableVertexAttribArray(2);
    gl.glEnableVertexAttribArray(3);

    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.faces.size() * sizeof(Triangle), model.faces.data(), GL_STATIC_DRAW);
    gl.glBindVertexArray(0);
    _size = model.faces.size() * 3;
}

void Viewer::init()
{
    auto& gl = QGl();
    gl.glGenVertexArrays(1, handleInit(_vao));
    gl.glGenBuffers(1, handleInit(_vertex_buffer));
    gl.glGenBuffers(1, handleInit(_index_buffer));
    _program = SafeGl::loadAndCompileProgram("shaders/vert.glsl", "shaders/frag.glsl");

    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthFunc(GL_LESS);
    gl.glEnable(GL_CULL_FACE);
    gl.glCullFace(GL_BACK);
    glPointSize(5);
}

void Viewer::draw()
{
    if(!_size) return;
    auto& gl = QGl();
    gl.glUseProgram(_program);
    float pMatrix[16];
    float mvMatrix[16];
    camera()->getProjectionMatrix(pMatrix);
    camera()->getModelViewMatrix(mvMatrix);
    std::vector<Eigen::Matrix4f> bone_mats = buildBoneMats(Seconds{0});
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "proj_matrix"),
                                       1, GL_FALSE, pMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "mv_matrix"),
                                       1, GL_FALSE, mvMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "bone_mats"), bone_mats.size(), GL_FALSE, reinterpret_cast<float*>(bone_mats.data()));

    gl.glBindVertexArray(_vao);
    gl.glDrawElements(GL_TRIANGLES, _size, GL_UNSIGNED_INT, nullptr);
    gl.glBindVertexArray(0);
    gl.glUseProgram(0);

    gl.glDisable(GL_DEPTH_TEST);
    glColor3f(0,0,1);
    glBegin(GL_POINTS);
    glVertex3d(0,0,0);
    glEnd();
    glColor3f(0,1,0);
    drawBones();
    glColor3f(1,0,0);
    drawBones(Seconds{0});
    gl.glEnable(GL_DEPTH_TEST);
}
