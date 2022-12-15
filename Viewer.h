#ifndef VIEWER_H
#define VIEWER_H

#include <QGLViewer/qglviewer.h>
#include "Model.h"
#include "SafeGl.h"


class Viewer : public QGLViewer
{
    Q_OBJECT
public:
    using QGLViewer::QGLViewer;

    void setModel(const ModelData& model);

protected:
    void init() override;
    void draw() override;

private:
    SafeGl::VertexArray _vao;
    SafeGl::Buffer _vertex_buffer;
    SafeGl::Buffer _index_buffer;
    SafeGl::Program _program;
    std::size_t _size = 0;
};

#endif
