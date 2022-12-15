#include "Viewer.h"


void Viewer::setModel(const ModelData& model)
{
    constexpr float inf = std::numeric_limits<float>::infinity();
    qglviewer::Vec min{inf,inf,inf}, max{-inf,-inf,-inf};
    std::vector<float> vertices(model.vertices.size() * 6);
    auto v_it = vertices.begin();
    for(const Vertex& v : model.vertices)
    {
        for(int i = 0; i < 3; ++i)
        {
            *v_it++ = v.pos[i];
            max[i] = std::max<float>(max[i], v.pos[i]);
            min[i] = std::min<float>(min[i], v.pos[i]);
        }
        for(float f : v.normal)
        {
            *v_it++ = f;
        }
    }
    qglviewer::Vec diff = max - min;
    camera()->setSceneRadius(std::max(std::max(diff[0], diff[1]), diff[2]));
    camera()->setSceneCenter((min + max) / 2);
    camera()->showEntireScene();
    std::size_t stride = 6 * sizeof(float);
    auto& gl = *QOpenGLContext::currentContext()->extraFunctions();
    gl.glBindVertexArray(_vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
    gl.glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    gl.glEnableVertexAttribArray(0);
    gl.glEnableVertexAttribArray(1);
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
}

void Viewer::draw()
{
    auto& gl = QGl();
    gl.glUseProgram(_program);
    float pMatrix[16];
    float mvMatrix[16];
    camera()->getProjectionMatrix(pMatrix);
    camera()->getModelViewMatrix(mvMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "proj_matrix"),
                                       1, GL_FALSE, pMatrix);
    gl.glUniformMatrix4fv(gl.glGetUniformLocation(_program, "mv_matrix"),
                                       1, GL_FALSE, mvMatrix);

    gl.glBindVertexArray(_vao);
    if(_size) gl.glDrawElements(GL_TRIANGLES, _size, GL_UNSIGNED_INT, nullptr);
    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}
