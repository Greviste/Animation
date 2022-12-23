#include "Model.h"


Model::Model(std::shared_ptr<const ModelData> data)
    :_data(std::move(data))
{
    if(!_data) throw std::invalid_argument("Model built without data");
    auto& gl = QGl();
    gl.glGenVertexArrays(1, handleInit(_vao));
    gl.glGenBuffers(1, handleInit(_vertex_buffer));
    gl.glGenBuffers(1, handleInit(_index_buffer));
    _program = SafeGl::loadAndCompileProgram("shaders/vert.glsl", "shaders/frag.glsl");
    buildModel();
}

void Model::buildModel()
{
    auto& gl = QGl();
    gl.glBindVertexArray(_vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);

    std::size_t stride = sizeof(Vertex);
    gl.glBufferData(GL_ARRAY_BUFFER, _data->vertices.size() * stride, _data->vertices.data(), GL_STATIC_DRAW);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    gl.glVertexAttribIPointer(2, 4, GL_UNSIGNED_SHORT, stride, (void*)(6 * sizeof(float)));
    gl.glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float) + 4 * sizeof(BoneIndex)));
    gl.glEnableVertexAttribArray(0);
    gl.glEnableVertexAttribArray(1);
    gl.glEnableVertexAttribArray(2);
    gl.glEnableVertexAttribArray(3);

    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, _data->faces.size() * sizeof(Triangle), _data->faces.data(), GL_STATIC_DRAW);
    gl.glBindVertexArray(0);
    _size = _data->faces.size() * 3;
}

void Model::draw(const qglviewer::Camera& camera, const Animation* anim, Seconds at)
{
    auto& gl = QGl();
    gl.glUseProgram(_program);
    Eigen::Matrix4f proj_matrix;
    Eigen::Matrix4f mv_matrix;
    camera.getProjectionMatrix(proj_matrix.data());
    camera.getModelViewMatrix(mv_matrix.data());
    Eigen::Matrix4f norm_matrix = mv_matrix.inverse();
    std::vector<Eigen::Matrix4f> bone_mats(_data->skeleton->boneCount(), Eigen::Matrix4f::Identity());
    std::vector<Eigen::Matrix4f> norm_bone_mats(_data->skeleton->boneCount(), Eigen::Matrix4f::Identity());
    std::vector dual_quats(_data->skeleton->boneCount(), Eigen::Matrix<float, 4, 2>{{0,0},{0,0},{0,0},{1,0}});
    if(anim) std::tie(bone_mats, norm_bone_mats, dual_quats) = anim->buildBoneMats(at);

    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "proj_matrix"),
                                       1, GL_FALSE, proj_matrix.data());
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "mv_matrix"),
                                       1, GL_FALSE, mv_matrix.data());
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "norm_matrix"),
                                       1, GL_TRUE, norm_matrix.data());
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "bone_mats"), bone_mats.size(), GL_FALSE, reinterpret_cast<float*>(bone_mats.data()));
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "norm_bone_mats"), norm_bone_mats.size(), GL_TRUE, reinterpret_cast<float*>(norm_bone_mats.data()));
    gl.glUniformMatrix2x4fv(gl.glGetUniformLocation(_program, "dual_quats"), dual_quats.size(), GL_FALSE, reinterpret_cast<float*>(dual_quats.data()));

    gl.glUniform1f(gl.glGetUniformLocation(_program, "dual_quat_part"), _dual_quat_part);

    gl.glBindVertexArray(_vao);
    gl.glDrawElements(GL_TRIANGLES, _size, GL_UNSIGNED_INT, nullptr);
    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}

void Model::setDualQuatPart(float part)
{
    _dual_quat_part = part;
}

const ModelData& Model::data() const
{
    return *_data;
}
