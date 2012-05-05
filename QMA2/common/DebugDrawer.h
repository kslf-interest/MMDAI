/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2012  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#ifndef DEBUGDRAWER_H
#define DEBUGDRAWER_H

#include "SceneLoader.h"
#include "SceneWidget.h"
#include "util.h"

#include <QtCore/QObject>
#include <btBulletDynamicsCommon.h>
#include <vpvl2/Common.h>

namespace internal {

using namespace vpvl2;

class DebugDrawer : public btIDebugDraw
{
public:
    typedef QSet<IBone *> BoneSet;

    DebugDrawer()
        : m_flags(0),
          m_visible(true)
    {}
    virtual ~DebugDrawer() {}

    static void setLine(const btVector3 &from, const btVector3 &to, QVector3D *lines) {
        lines[0].setX(from.x());
        lines[0].setY(from.y());
        lines[0].setZ(from.z());
        lines[1].setX(to.x());
        lines[1].setY(to.y());
        lines[1].setZ(to.z());
    }
    void draw3dText(const btVector3 & /* location */, const char *textString) {
        fprintf(stderr, "[INFO]: %s\n", textString);
    }
    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int /* lifeTime */, const btVector3 &color) {
        QVector3D lines[2];
        setLine(PointOnB, PointOnB + normalOnB * distance, lines);
        m_program.setUniformValue("color", QColor::fromRgb(color.x() * 255, color.y() * 255, color.z() * 255));
        m_program.setAttributeArray("inPosition", lines);
        glDrawArrays(GL_LINES, 0, 2);
    }
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        QVector3D lines[2];
        setLine(from, to, lines);
        m_program.setUniformValue("color", QColor::fromRgb(color.x() * 255, color.y() * 255, color.z() * 255));
        m_program.setAttributeArray("inPosition", lines);
        glDrawArrays(GL_LINES, 0, 2);
    }
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 & /* toColor */) {
        QVector3D lines[2];
        setLine(from, to, lines);
        m_program.setUniformValue("color", QColor::fromRgb(fromColor.x() * 255, fromColor.y() * 255, fromColor.z() * 255));
        m_program.setAttributeArray("inPosition", lines);
        glDrawArrays(GL_LINES, 0, 2);
    }
    void reportErrorWarning(const char *warningString) {
        fprintf(stderr, "[ERROR]: %s\n", warningString);
    }
    int getDebugMode() const {
        return m_flags;
    }
    void setDebugMode(int debugMode) {
        m_flags = debugMode;
    }

    void load() {
        m_program.addShaderFromSourceFile(QGLShader::Vertex, ":shaders/handle.vsh");
        m_program.addShaderFromSourceFile(QGLShader::Fragment, ":shaders/handle.fsh");
        m_program.link();
    }
    void setVisible(bool value) {
        m_visible = value;
    }

    void drawModelBones(IModel *model, Scene *scene, const BoneSet &selectedBones) {
        if (!model || !m_visible || !m_program.isLinked())
            return;
        Array<IBone *> bones;
        model->getBones(bones);
        const int nbones = bones.count();
        float matrix[16];
        QGLFunctions func(QGLContext::currentContext());
        glDisable(GL_DEPTH_TEST);
        /* シェーダのパラメータ設定 */
        m_program.bind();
        scene->matrices()->getModelViewProjection(matrix);
        int modelViewProjectionMatrix = m_program.uniformLocation("modelViewProjectionMatrix");
        func.glUniformMatrix4fv(modelViewProjectionMatrix, 1, GL_FALSE, matrix);
        m_program.setUniformValue("boneMatrix", QMatrix4x4());
        m_program.enableAttributeArray("inPosition");
        /* IK ボーンの収集 */
        BoneSet bonesForIK;
        Array<IBone *> linkedBones;
        for (int i = 0; i < nbones; i++) {
            IBone *bone = bones[i];
            if (bone->hasInverseKinematics()) {
                linkedBones.clear();
                bonesForIK.insert(bone);
                bonesForIK.insert(bone->targetBone());
                bone->getLinkedBones(linkedBones);
                const int nlinks = linkedBones.count();
                for (int j = 0; j < nlinks; j++) {
                    IBone *linkedBone = linkedBones[j];
                    bonesForIK.insert(linkedBone);
                }
            }
        }
        /* ボーンの表示(レンダリング) */
        for (int i = 0; i < nbones; i++) {
            IBone *bone = bones[i];
            /* 操作不可能の場合はスキップ */
            if (!bone->isInteractive())
                continue;
            drawBone(bone, selectedBones, bonesForIK);
        }
        m_program.release();
        glEnable(GL_DEPTH_TEST);
    }
    void drawBoneTransform(IBone *bone, Scene *scene, int mode) {
        /* 固定軸がある場合は軸表示なし */
        if (!m_visible || !bone || !m_program.isLinked() || bone->hasFixedAxes())
            return;
        float matrix[16];
        QGLFunctions func(QGLContext::currentContext());
        glDisable(GL_DEPTH_TEST);
        /* シェーダのパラメータ設定 */
        m_program.bind();
        scene->matrices()->getModelViewProjection(matrix);
        int modelViewProjectionMatrix = m_program.uniformLocation("modelViewProjectionMatrix");
        func.glUniformMatrix4fv(modelViewProjectionMatrix, 1, GL_FALSE, matrix);
        m_program.setUniformValue("boneMatrix", QMatrix4x4());
        m_program.enableAttributeArray("inPosition");
        /* ボーン表示 */
        BoneSet selectedBones;
        selectedBones.insert(bone);
        drawBone(bone, selectedBones, BoneSet());
        /* 軸表示 */
        static const Scalar kLength = 2.0;
        static const Vector3 kRed   = Vector3(1, 0, 0);
        static const Vector3 kGreen = Vector3(0, 1, 0);
        static const Vector3 kBlue  = Vector3(0, 0, 1);
        if (mode == 'V') {
            /* モデルビュー行列を元に軸表示 */
            const Transform &transform = bone->localTransform();
            const btMatrix3x3 &modelView = scene->camera()->modelViewTransform().getBasis();
            const Vector3 &origin = bone->localTransform().getOrigin();
            drawLine(origin, transform * (modelView.getRow(0) * kLength), kRed);
            drawLine(origin, transform * (modelView.getRow(1) * kLength), kGreen);
            drawLine(origin, transform * (modelView.getRow(2) * kLength), kBlue);
        }
        else if (mode == 'L') {
            if (bone->hasLocalAxes()) {
                /* 子ボーンの方向をX軸、手前の方向をZ軸として設定する */
                const Transform &transform = bone->localTransform();
                const Vector3 &origin = transform.getOrigin();
                Matrix3x3 axes = Matrix3x3::getIdentity();
                bone->getLocalAxes(axes);
                drawLine(origin, transform * (axes[0] * kLength), kRed);
                drawLine(origin, transform * (axes[1] * kLength), kGreen);
                drawLine(origin, transform * (axes[2] * kLength), kBlue);
            }
            else {
                /* 現在のボーン位置と回転量を乗算した軸を表示 */
                const Transform &transform = bone->localTransform();
                const Vector3 &origin = transform.getOrigin();
                drawLine(origin, transform * Vector3(kLength, 0, 0), kRed);
                drawLine(origin, transform * Vector3(0, kLength, 0), kGreen);
                drawLine(origin, transform * Vector3(0, 0, kLength), kBlue);
            }
        }
        else {
            /* 現在のボーン位置に対する固定軸を表示 */
            const Vector3 &origin = bone->localTransform().getOrigin();
            drawLine(origin, origin + Vector3(kLength, 0, 0), kRed);
            drawLine(origin, origin + Vector3(0, kLength, 0), kGreen);
            drawLine(origin, origin + Vector3(0, 0, kLength), kBlue);
        }
        glEnable(GL_DEPTH_TEST);
        m_program.release();
    }

private:
    void drawBone(IBone *bone, const BoneSet &selected, const BoneSet &IK) {
        const Vector3 &dest = bone->destinationOrigin();
        if (!bone || !bone->isVisible() || dest.isZero())
            return;
        Transform tr = Transform::getIdentity();
        Array<Vector3> vertices;
        static const int indices[] = {
            0, 1, 2, 3
        };
        static const QColor kColorRed = QColor::fromRgbF(1.0, 0.0, 0.0);
        static const QColor kColorOrange = QColor::fromRgbF(1.0, 0.75, 0.0);
        static const QColor kColorBlue = QColor::fromRgbF(0.0, 0.0, 1.0);
        const Vector3 &origin = bone->localTransform().getOrigin();
        const Scalar &coneRadius = 0.05f;//btMin(0.1, childOrigin.distance(origin) * 0.1);
        const Scalar &sphereRadius = 0.2f;
        /* ボーン接続を表示するための頂点設定 */
        tr.setOrigin(Vector3(coneRadius, 0.0f, 0.0f));
        vertices.add(tr * origin);
        vertices.add(dest);
        tr.setOrigin(Vector3(-coneRadius, 0.0f, 0.0f));
        vertices.add(tr * origin);
        vertices.add(dest);
        /* 選択中の場合は赤色で表示 */
        if (selected.contains(bone)) {
            drawSphere(origin, sphereRadius, Vector3(1.0f, 0.0f, 0.0f));
            m_program.setUniformValue("color", kColorRed);
        }
        /* 固定軸(例:捻り)ありのボーンの場合は球体のみ紫色、接続部分を青で表示 */
        else if (bone->hasFixedAxes()) {
            drawSphere(origin, sphereRadius, Vector3(1.0, 0.0, 1.0f));
            m_program.setUniformValue("color", kColorBlue);
        }
        /* IK ボーンの場合は橙色で表示 */
        else if (IK.contains(bone)) {
            drawSphere(origin, sphereRadius, Vector3(1.0f, 0.75f, 0.0f));
            m_program.setUniformValue("color", kColorOrange);
        }
        /* 上記以外の場合は青色で表示 */
        else {
            drawSphere(origin, sphereRadius, Vector3(0.0f, 0.0f, 1.0f));
            m_program.setUniformValue("color", kColorBlue);
        }
        /* 描写 */
        m_program.setAttributeArray("inPosition",
                                    reinterpret_cast<const GLfloat *>(&vertices[0]),
                                    3,
                                    sizeof(Vector3));
        glDrawElements(GL_LINE_LOOP, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, indices);
    }

    QGLShaderProgram m_program;
    int m_flags;
    bool m_visible;

    Q_DISABLE_COPY(DebugDrawer)
};

}

#endif // DEBUGDRAWER_H
