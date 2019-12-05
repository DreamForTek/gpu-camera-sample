/*
 Copyright 2011-2019 Fastvideo, LLC.
 All rights reserved.

 This file is a part of the GPUCameraSample project
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 3. Any third-party SDKs from that project (XIMEA SDK, Fastvideo SDK, etc.) are licensed on different terms. Please see their corresponding license terms.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
*/

#include "GLImageViewer.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPoint>
#include <QScreen>
#include <QTimer>

#include "fastvideo_sdk.h"
namespace
{
    const qreal zoomStep = 0.1;
    const qreal zoomMin = 0.1;
    const qreal zoomMax = 8.0;
}

GLImageViewer::GLImageViewer(GLRenderer *renderer) :
    QOpenGLWindow(),
    mRenderer(renderer)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFormat(renderer->format());

    mZoom = 1.;
    mPtDown = QPoint(-1, -1);
    mTexTopLeft = QPoint(0, 0);
    mViewMode = GLImageViewer::vmZoomFit;
    mShowImage = false;
}

GLImageViewer::~GLImageViewer(){}

void GLImageViewer::clear()
{
    if(mRenderer != nullptr)
        mRenderer->showImage(false);
    update();
}

void GLImageViewer::load(void* img, int width, int height)
{

    if(mRenderer == nullptr)
        return;
    //mImageSize = QSize(width, height);
    //mShowImage = true;


    mRenderer->loadImage(img, width, height);
    setViewMode(mViewMode);
    update();
}

void GLImageViewer::setViewMode(GLImageViewer::ViewMode mode)
{
    mViewMode = mode;
    if(mViewMode == vmZoomFit)
    {
        setFitZoom(size());
    }
    update();
}


GLImageViewer::ViewMode GLImageViewer::getViewMode() const
{
    return mViewMode;
}


void GLImageViewer::setZoom(qreal scale)
{
    setZoomInternal(scale);
}

void GLImageViewer::setZoomInternal(qreal newZoom, QPoint fixPoint)
{
    if(newZoom < ::zoomMin || newZoom > ::zoomMax || newZoom == getZoom())
        return;

    qreal zoom = getZoom();

    if(fixPoint.isNull())
        fixPoint = geometry().center();

    float x = fixPoint.x();
    float y = fixPoint.y();

    mTexTopLeft += QPointF(x * (1. / zoom - 1. / newZoom), -(height() - y) * (1. / zoom - 1. / newZoom));

    mZoom = newZoom;
    adjustTexTopLeft();
    update();
    emit zoomChanged(newZoom);
}

void GLImageViewer::adjustTexTopLeft()
{
    float w = width();
    float h = height();

    QSize imageSize(mRenderer->imageSize());
    float iw = imageSize.width();
    float ih = imageSize.height();

    if(mTexTopLeft.x() < 0)
        mTexTopLeft.setX(0);

    if(iw - w / mZoom > 0)
    {
        if(mTexTopLeft.x() > iw - w / mZoom)
            mTexTopLeft.setX(iw - w / mZoom);
    }

    if(mTexTopLeft.y() < h / mZoom)
        mTexTopLeft.setY(h / mZoom);

    if(mTexTopLeft.y() > ih)
        mTexTopLeft.setY(ih);

}

qreal GLImageViewer::getZoom() const
{
    return mZoom;
}

void GLImageViewer::resizeEvent(QResizeEvent * event)
{
    if(mViewMode == vmZoomFit)
    {
        QSize sz = event->size();

        setFitZoom(sz);
        emit sizeChanged(sz);
    }
    QOpenGLWindow::resizeEvent(event);
}

void GLImageViewer::mouseMoveEvent(QMouseEvent * event)
{
    if(event->buttons() != Qt::LeftButton)
        return;
    if(mPtDown.isNull())
        return;

    float dx = mPtDown.x() - event->pos().x();
    float dy = mPtDown.y() - event->pos().y();

    mTexTopLeft.rx() = mTexTopLeft.x() + dx / mZoom;
    mTexTopLeft.ry() = mTexTopLeft.y() + dy / mZoom;

    adjustTexTopLeft();
    update();

    mPtDown = event->pos();
}

void GLImageViewer::mousePressEvent(QMouseEvent * event)
{
    if(event->buttons() == Qt::LeftButton)
        mPtDown = event->pos();
    else
        emit contextMenu(event->globalPos());
}

void GLImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    mPtDown = QPoint();
}

void GLImageViewer::setFitZoom(QSize szClient)
{
    if(!mRenderer)
        return;

    szClient -=(QSize(6,6));
    QSize imageSize(mRenderer->imageSize());

    if(imageSize.isEmpty())
        return;

    qreal zoom = qMin((qreal)(szClient.height()) / (qreal)(imageSize.height()), (qreal)(szClient.width()) / (qreal)(imageSize.width()));
    setZoom(zoom);
}

void GLImageViewer::wheelEvent(QWheelEvent * event)
{
    if(mViewMode == vmZoomFit)
        return;

    float numDegrees = event->delta() / 8.;
    float numSteps = numDegrees / 15.;

    Qt::KeyboardModifiers keyState = QApplication::queryKeyboardModifiers();
    if(keyState.testFlag(Qt::ControlModifier))
    {
        qreal newZoom = getZoom() * std::pow(1.125, numSteps);
        setZoomInternal(newZoom, event->pos());
        update();
    }
}

void GLImageViewer::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);
    update();
}

GLRenderer::GLRenderer(QObject *parent):
    QObject(parent)

{
    m_format.setDepthBufferSize(16);
    m_format.setSwapInterval(1);
    m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    m_format.setRenderableType(QSurfaceFormat::OpenGL);
    m_format.setProfile(QSurfaceFormat::CoreProfile);

    m_context = new QOpenGLContext(this);
    m_context->setFormat(m_format);
    m_context->create();

    mRenderThread.setObjectName(QStringLiteral("RenderThread"));
    moveToThread(&mRenderThread);
    mRenderThread.start();
}

GLRenderer::~GLRenderer()
{
    mRenderThread.quit();
    mRenderThread.wait(3000);
}

void GLRenderer::initialize()
{
    QSize sz = QGuiApplication::primaryScreen()->size() * 2;
    initializeOpenGLFunctions();

    GLint bsize;
    glGenTextures(1, &texture);
    glGenBuffers(1, &pbo_buffer);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, GLsizeiptr(3 * sizeof(unsigned char)) * sz.width() * sz.height(), nullptr, GL_STREAM_COPY);
    glGetBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &bsize);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glDisable(GL_TEXTURE_2D);
}

void GLRenderer::showImage(bool show)
{
    mShowImage = show;
}

void GLRenderer::update()
{
    QTimer::singleShot(0, this, [this](){render();});
}

void GLRenderer::render()
{
    if(mRenderWnd == nullptr)
        return;

    if(!mRenderWnd->isExposed())
        return;

    if(!m_context->makeCurrent(mRenderWnd))
        return;

    if(!m_initialized)
    {
        initialize();
        m_initialized = true;
    }

    if(mImageSize.isEmpty() || !mShowImage)
    {
        //Render empty background
        glViewport(0, 0, 1, 1);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, 0, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glClearColor(0.25, 0.25, 0.25, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_context->swapBuffers(mRenderWnd);
        m_context->doneCurrent();
        return;
    }

    int w = mRenderWnd->width();
    int h = mRenderWnd->height();

    GLfloat iw = mImageSize.width();
    GLfloat ih = mImageSize.height();

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.25, 0.25, 0.25, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_buffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iw, ih, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    float rectLeft, rectTop, rectRight, rectBottom;
    float texLeft, texTop, texRight, texBottom;

    //Calc left and right bounds
    auto zoom = GLfloat(mRenderWnd->getZoom());

    QPointF texTopLeft(mRenderWnd->texTopLeft());
    if(w / zoom <= iw)
    {
        rectLeft = 0;
        rectRight = 1;
        texLeft = texTopLeft.x();

        texRight = texLeft + w / zoom;
        if(texRight > iw)
            texRight = iw;
    }
    else
    {
        rectLeft = (w - iw * zoom) / (2 * w);
        rectRight = rectLeft + (iw * zoom) / w;
        texLeft = 0;
        texRight = iw;
    }

    //Calc top and bottom bounds
    if(h / zoom <= ih)
    {
        rectTop = 1;
        rectBottom = 0;
        texBottom =  texTopLeft.y() - h / zoom;
        if(texBottom < 0)
            texBottom = 0;
        texTop = texBottom + h / zoom;
        if(texTop > ih)
            texTop = ih;
    }
    else
    {
        rectBottom = (h - ih * zoom) / (2 * h);
        rectTop = rectBottom + (ih * zoom) / h;

        texBottom = 0;
        texTop = ih;
    }
    texLeft /= iw;
    texRight /= iw;

    texTop /= ih;
    texBottom /= ih;


    glBegin(GL_QUADS);
    {
        glTexCoord2f(texRight, texBottom);
        glVertex2f(rectRight, rectTop);

        glTexCoord2f(texRight, texTop);
        glVertex2f(rectRight, rectBottom);

        glTexCoord2f(texLeft, texTop);
        glVertex2f(rectLeft, rectBottom);

        glTexCoord2f(texLeft, texBottom);
        glVertex2f(rectLeft, rectTop);
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glFinish();
    m_context->swapBuffers(mRenderWnd);
    glFinish();
}

void GLRenderer::loadImage(void* img, int width, int height)
{
    QTimer::singleShot(0, this, [this, img,width,height](){loadImageInternal(img, width, height);});
}

void GLRenderer::loadImageInternal(void* img, int width, int height)
{
    unsigned char *data = NULL;
    size_t pboBufferSize = 0;

    cudaError_t error = cudaSuccess;

    mImageSize = QSize(width, height);



    if(!pbo_buffer)
        initialize();

    if(!m_context->makeCurrent(mRenderWnd))
        return;

    GLint bsize;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBufferData(GL_PIXEL_UNPACK_BUFFER, 3 * sizeof(unsigned char) * width * height, NULL, GL_STREAM_COPY);
    glGetBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &bsize);
    struct cudaGraphicsResource* cuda_pbo_resource = 0;

    if(img == nullptr)
        return;

    error = cudaGraphicsGLRegisterBuffer(&cuda_pbo_resource, pbo_buffer, cudaGraphicsMapFlagsWriteDiscard);
    if ( error != cudaSuccess )
    {
        qDebug("Cannot register CUDA Graphic Resource: %s\n", cudaGetErrorString(error));
        return;
    }

    if ( ( error = cudaGraphicsMapResources( 1, &cuda_pbo_resource, 0 ) ) != cudaSuccess)
    {
        qDebug("cudaGraphicsMapResources failed: %s\n", cudaGetErrorString(error) );
        return;
    }

    if(( error = cudaGraphicsResourceGetMappedPointer( (void **)&data, &pboBufferSize, cuda_pbo_resource ) ) != cudaSuccess )
    {
        qDebug("cudaGraphicsResourceGetMappedPointer failed: %s\n", cudaGetErrorString(error) );
        return;
    }

    if(pboBufferSize < ( width * height * 3 * sizeof(unsigned char) ))
    {
        qDebug("cudaGraphicsResourceGetMappedPointer failed: %s\n", cudaGetErrorString(error) );
        return;
    }

    if(( error = cudaMemcpy( data, img, width * height * 3 * sizeof(unsigned char), cudaMemcpyDeviceToDevice ) ) != cudaSuccess)
    {
        qDebug("cudaMemcpy failed: %s\n", cudaGetErrorString(error) );
        return;
    }

    if(( error = cudaGraphicsUnmapResources( 1, &cuda_pbo_resource, 0 ) ) != cudaSuccess )
    {
         qDebug("cudaGraphicsUnmapResources failed: %s\n", cudaGetErrorString(error) );
         return;
    }

    if(cuda_pbo_resource)
    {
        if((error  = cudaGraphicsUnregisterResource(cuda_pbo_resource))!= cudaSuccess)
        {
            qDebug("Cannot unregister CUDA Graphic Resource: %s\n", cudaGetErrorString(error));
            return;
        }
        cuda_pbo_resource = 0;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_buffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    m_context->doneCurrent();
}
