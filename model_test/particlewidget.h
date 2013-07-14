#ifndef _particlewidget_h_
#define _particlewidget_h_

#if defined(HPX_BUILD_TYPE)
#include <hpx/hpx_fwd.hpp>
#endif

#include <QtGui>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>
#include <GL/glu.h>
#include <iostream>

#include <grid_storage.hpp>

#include <boost/array.hpp>

#include "particle.h"

#define LOG(x,...) std::cout << x;

class ParticleWidget
    : public QGLWidget, protected QGLFunctions
{
    Q_OBJECT

public:
    typedef Particles grid_type;
    static const std::size_t N = 140;

    ParticleWidget(grid_storage& gui, size_t x_size, size_t y_size,
                   QColor backgroundColor = Qt::black, QWidget *parent = 0) :
        QGLWidget(parent),
        x_size(x_size), y_size(y_size),
        backgroundColor(backgroundColor), 
        gui(gui),
        globalOffset(0, 0, 0),
        frameCounter(0),
        colors(N*4)
    {}

    ~ParticleWidget()
    {}

    QSize minimumSizeHint() const
    {
        return QSize(50, 50);
    }

    QSize sizeHint() const
    {
        return QSize(400, 200);
    }

Q_SIGNALS:
    void forceRecorded(QVector2D, QVector2D);
    void newFrame();
    void reset();

public:

    GLuint loadShader(GLenum type, const char * src)
    {
        GLuint shader = glCreateShader(type);
        if (shader != 0) {
            int len = strlen(src);
            glShaderSource(shader, 1, &src, &len);
            glCompileShader(shader);
            int compiled;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            LOG("Shader compile log:\n");
            const int bufLen = 2048;
            char buf[bufLen] = {0};
            glGetShaderInfoLog(shader, 2048, &len, buf);
            std::string str(buf, buf + len);
            LOG(str);

            if (!compiled) {
                LOG("Could not compile shader" << type << "!\n");
                glDeleteShader(shader);
                shader = 0;
            }

        }
        return shader;
    }

protected:

    GLuint createProgram()
    {
        static const char * vertexShaderSrc =
            "attribute vec3 vertex;\n"
            "attribute vec2 texCoord;\n"
            "attribute float index;\n"
            "varying vec2 tex;\n"
            "varying vec4 c;\n"
            "uniform vec4 data[140];\n"
            "uniform vec4 color[140];\n"
            "uniform mat4 matrix;\n"
            "const float fac = 0.01/180.0 * 3.14;\n"
            "void main(void)\n"
            "{\n"
            "    int i = int(index);\n"
            "    float rad_angle = data[i].w * fac;\n"
            "    float cos_angle = cos(rad_angle);\n"
            "    float sin_angle = sin(rad_angle);\n"
            "    mat4 model = mat4(\n"
            "           vec4( cos_angle,  sin_angle, 0.0, 0.0)\n"
            "         , vec4(-sin_angle,  cos_angle, 0.0, 0.0)\n"
            "         , vec4(       0.0,        0.0, 1.0, 0.0)\n"
            "         , vec4(data[i].xyz * 0.01, 1.0));\n"
            "    tex = texCoord;\n"
            "    c = color[i];\n"
            "    gl_Position = (matrix * model) * vec4(vertex, 1.0);\n"
            "}\n"
            ;

        static const char * fragmentShaderSrc =
            "varying vec4 c;\n"
            "varying vec2 tex;\n"
            "uniform sampler2D myTexture;\n"
            "const float fac = 1.0/255.0;\n"
            "void main(void)\n"
            "{\n"
            "    vec4 realColor = c;\n"
            "    vec2 delta = vec2(1.0/300.0, 1.0/4000.0);\n"
            "    vec2 realTex = vec2(tex.x, tex.y * (1.0 / 40.0)) + delta * vec2(0, c.w * 100.0);\n"
            "    realColor.w = 255.0;\n"
            "    vec4 baseColor = (realColor * fac) * texture2D(myTexture, realTex);\n"
            "    if(baseColor.a < 0.9) discard;\n"
            "    gl_FragColor = baseColor;\n"
            "}\n"
            ;

        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSrc);
        if(vertexShader == 0) return 0;
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
        if(fragmentShader == 0) return 0;

        GLuint program = glCreateProgram();

        if(program != 0)
        {
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);
            int linkStatus;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if(linkStatus != GL_TRUE)
            {
                LOG("Could not link program\n");
                glDeleteProgram(program);
                program = 0;
            }
        }

        return program;
    }

    void initializeGL()
    {
        initializeGLFunctions();

        glEnable(GL_DEPTH_TEST);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);

        QPixmap map("./brushstrokes.png");
        texture = bindTexture(map, GL_TEXTURE_2D);

        program = createProgram();
        if(program == 0) return;

        const double scale = 0.4f;

        static const double
            triangle[] = {
                scale * +3, scale * -1, scale * -1,
                scale * -3, scale * -1, scale * -1,
                scale * -3, scale * +1, scale * -1,

                scale * +3, scale * -1, scale * -1,
                scale * -3, scale * +1, scale * -1,
                scale * +3, scale * +1, scale * -1,
            };

        static const float
            triangleTexCoords[] = {
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,

                1.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 0.0f,
            };

        glGenBuffers(3, buffers);

        coords.reserve(N * 6 * 3);
        texCoords.reserve(N * 6 * 2);
        indices.reserve(N * 6);

        float index = 0.0;
        for(std::size_t i = 0; i < N; ++i)
        {
            for(std::size_t j = 0; j < sizeof(triangle)/sizeof(float); ++j)
            {
                coords.push_back(triangle[j]);
            }

            for(std::size_t j = 0; j < sizeof(triangleTexCoords)/sizeof(float); ++j)
            {
                texCoords.push_back(triangleTexCoords[j]);
            }
            for(std::size_t j = 0; j < 6; ++j)
            {
                indices.push_back(index);
            }
            index += 1.0f;
        }

        glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
        glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(float), &coords[0], GL_STREAM_DRAW);

        GLuint vertexLocation = glGetAttribLocation(program, "vertex");
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vertexLocation);

        glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
        glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), &texCoords[0], GL_STREAM_DRAW);

        GLuint texCoordLocation = glGetAttribLocation(program, "texCoord");
        glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(texCoordLocation);

        glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
        glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STREAM_DRAW);

        GLuint indexLocation = glGetAttribLocation(program, "index");
        glVertexAttribPointer(indexLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(indexLocation);

        dataLocation = glGetUniformLocation(program, "data");
        matrixLocation = glGetUniformLocation(program, "matrix");
        colorLocation  = glGetUniformLocation(program, "color");
        textureLocation  = glGetUniformLocation(program, "texture");

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1iv(textureLocation, 1, &texture);
    }

    size_t x_size, y_size;

    std::vector<float> coords;
    std::vector<float> texCoords;
    std::vector<float> indices;
    GLuint dataLocation;
    GLuint matrixLocation;
    GLuint colorLocation;
    GLuint textureLocation;
    GLuint buffers[3];

    void paintGL()
    {
        boost::shared_ptr<grid_type> next_particles = gui.next_grid();

        if (next_particles) {
            particles = next_particles;
        }

        if (!particles) return;

        qglClearColor(Qt::black);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, &matrix[0]);

        float * data_ptr = &particles->posAngle[0];
        uint32_t * color_ptr = &particles->colors[0];
        std::size_t particles_size = particles->size();
        particles_size = particles_size - (particles_size % N);

        for (std::size_t i = 0; i < particles_size; i+=N) {
            std::size_t n_data = N * 4;

            float * color_dst_ptr = &colors[0];
            for(std::size_t j = 0; j < N; ++j) {
                uint32_t color = *(color_ptr + j);
                *(color_dst_ptr++) = qRed  (color);
                *(color_dst_ptr++) = qGreen(color);
                *(color_dst_ptr++) = qBlue (color);
                *(color_dst_ptr++) = qAlpha(color);
            }

            glUniform4fv(dataLocation,  n_data, data_ptr);
            glUniform4fv(colorLocation, n_data, &colors[0]);
            glDrawArrays(GL_TRIANGLES, 0, 6 * N);
            data_ptr += n_data;
            color_ptr += N;
        }
    }

    void resizeGL(int width, int height)
    {
        float screenRatio = float(width)/height;
        float worldRatio = float(x_size)/y_size;
        if(worldRatio > screenRatio)
        {
            int viewportHeight = width/worldRatio;
            viewport[0] = 0;
            viewport[1] = (height - viewportHeight)/2;
            viewport[2] = width;
            viewport[3] = viewportHeight;
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }
        else
        {
            int viewportWidth = height*worldRatio;
            viewport[0] = (width - viewportWidth)/2;
            viewport[1] = 0;
            viewport[2] = viewportWidth;
            viewport[3] = height;
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }


        w = viewport[2] - viewport[0];
        h = viewport[3] - viewport[1];

        resetProjection();
    }

    void keyPressEvent(QKeyEvent * event)
    {
        switch(event->key())
        {
            case Qt::Key_F5:
                gui.reset();
                break;
            case Qt::Key_F:
                if(isFullScreen())
                {
                    showNormal();
                }
                else
                {
                    showFullScreen();
                }
                break;
            default:
                break;
        }
    }

    void mousePressEvent(QMouseEvent *event)
    {
        if (event->buttons() & Qt::LeftButton) {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
            gluUnProject(
                event->pos().x()
              , event->pos().y()
              , 0.0
              , modelview
              , mapMatrix.constData()
              , viewport
              , &x
              , &y
              , &z
            );
            lastSweepPos = QVector2D(x, y);
        }
        if (event->buttons() & Qt::RightButton) {
            lastPanPos = event->pos();
        }
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        if (event->buttons() & Qt::LeftButton) {
            gluUnProject(
                event->pos().x()
              , event->pos().y()
              , 0.0
              , modelview
              , mapMatrix.constData()
              , viewport
              , &x
              , &y
              , &z
            );
            QVector2D modelPos(x, y);

            QVector2D modelDelta(modelPos.x() - lastSweepPos.x(), modelPos.y() - lastSweepPos.y());

            gui.forceRecorded(modelPos, modelDelta);
            lastSweepPos = modelPos;
        }

        if (event->buttons() & Qt::RightButton) {
            double zoomFactor = -globalOffset.z() * 0.01 + 1;
            double factor = 0.0403 * zoomFactor;
            double factorX = factor * 1000.0 / width();
            double factorY = factor *  500.0 / height();

            QPoint delta = event->pos() - lastPanPos;

            globalOffset.setX(globalOffset.x() + factorX * delta.x());
            globalOffset.setY(globalOffset.y() + factorY * delta.y());
            lastPanPos = event->pos();
            resetProjection();
        }
    }

    void wheelEvent(QWheelEvent *event)
    {
        double newZ = globalOffset.z() - event->delta() * 0.03;
        if ((newZ < 90) && (newZ > -1000)) {
            globalOffset.setZ(newZ);
            resetProjection();
        }
    }

private:
    boost::array<float, 16> matrix;
    GLuint program;
    GLint texture;
    QMatrix4x4 mapMatrix;
    double modelview[16] = { 1.0, 0.0, 0.0, 0.0
                           , 0.0, 1.0, 0.0, 0.0
                           , 0.0, 0.0, 1.0, 0.0
                           , 0.0, 0.0, 0.0, 1.0
                           };
    int viewport[4];
    QColor backgroundColor;
    QVector2D lastSweepPos;
    QPoint lastPanPos;
    boost::shared_ptr<grid_type> particles;
    grid_storage& gui;
    QVector3D globalOffset;
    int frameCounter;

    int h;
    int w;

    std::vector<float> colors;

    void resetProjection()
    {
      QMatrix4x4 pmvMatrix;

      const float ratio = float(h)/w;

      pmvMatrix.setToIdentity();
      pmvMatrix.frustum(-1.0, 1.0, ratio, -ratio, 5, 1000);
      pmvMatrix.translate(
          globalOffset.x(),
          globalOffset.y(),
          globalOffset.z());

      mapMatrix.setToIdentity();
      mapMatrix.ortho(0, x_size, 0, y_size, 0, 1);
      mapMatrix.translate(
          globalOffset.x(),
          globalOffset.y(),
          globalOffset.z());

      for(std::size_t i = 0; i < 16; ++i)
      {
        matrix[i] = pmvMatrix.constData()[i];
      }
    }

};

#include "particlewidget.h.moc"

#endif
