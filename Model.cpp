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
    float pMatrix[16];
    float mvMatrix[16];
    camera.getProjectionMatrix(pMatrix);
    camera.getModelViewMatrix(mvMatrix);
    std::vector<Eigen::Matrix4f> bone_mats(_data->skeleton->boneCount(), Eigen::Matrix4f::Identity());
    if(anim) bone_mats = anim->buildBoneMats(at);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "proj_matrix"),
                                       1, GL_FALSE, pMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "mv_matrix"),
                                       1, GL_FALSE, mvMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "bone_mats"), bone_mats.size(), GL_FALSE, reinterpret_cast<float*>(bone_mats.data()));

    gl.glBindVertexArray(_vao);
    gl.glDrawElements(GL_TRIANGLES, _size, GL_UNSIGNED_INT, nullptr);
    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}

const ModelData& Model::data() const
{
    return *_data;
}
