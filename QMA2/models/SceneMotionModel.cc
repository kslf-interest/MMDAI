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

#include "common/SceneLoader.h"
#include "common/SceneWidget.h"
#include "models/SceneMotionModel.h"

#include <QtGui/QtGui>
#include <vpvl/vpvl.h>
#include <vpvl/gl2/Renderer.h>

using namespace vpvl;

namespace {

class RootTreeItem : public MotionBaseModel::ITreeItem
{
public:
    RootTreeItem()
    {
    }
    ~RootTreeItem() {
        qDeleteAll(m_children);
    }

    void addChild(ITreeItem *item) {
        m_children.append(item);
    }
    ITreeItem *parent() const {
        return 0;
    }
    ITreeItem *child(int row) const {
        return m_children.value(row);
    }
    const QString &name() const {
        static const QString empty = "";
        return empty;
    }
    bool isRoot() const {
        return true;
    }
    bool isCategory() const {
        return false;
    }
    int rowIndex() const {
        return 0;
    }
    int countChildren() const {
        return m_children.count();
    }

private:
    QList<ITreeItem *> m_children;
};

class CameraTreeItem : public MotionBaseModel::ITreeItem
{
public:
    CameraTreeItem(RootTreeItem *root)
        : m_parent(root)
    {
    }
    ~CameraTreeItem() {
    }

    void addChild(ITreeItem * /* item */) {
    }
    ITreeItem *parent() const {
        return m_parent;
    }
    ITreeItem *child(int /* row */) const {
        return 0;
    }
    const QString &name() const {
        static const QString name = "Camera";
        return name;
    }
    bool isRoot() const {
        return false;
    }
    bool isCategory() const {
        return true;
    }
    int rowIndex() const {
        return 0;
    }
    int countChildren() const {
        return 0;
    }

private:
    ITreeItem *m_parent;
};

/* 今のところカメラフレームしか対応していないため、暫定実装になってる */
class SetFramesCommand : public QUndoCommand
{
public:
    SetFramesCommand(SceneMotionModel *smm,
                     const SceneMotionModel::KeyFramePairList &frames,
                     SceneMotionModel::ITreeItem *cameraTreeItem)
        : QUndoCommand(),
          m_smm(smm),
          m_cameraTreeItem(cameraTreeItem),
          m_parameter(smm->cameraInterpolationParameter())
    {
        QList<int> indices;
        foreach (const SceneMotionModel::KeyFramePair &frame, frames) {
            int frameIndex = frame.first;
            const QModelIndex &index = m_smm->frameIndexToModelIndex(m_cameraTreeItem, frameIndex);
            if (index.data(SceneMotionModel::kBinaryDataRole).canConvert(QVariant::ByteArray))
                m_indices.append(index);
            indices.append(frameIndex);
        }
        m_frames = frames;
        m_frameIndices = indices;
    }
    virtual ~SetFramesCommand() {
    }

    virtual void undo() {
        CameraAnimation *animation = m_smm->currentMotion()->mutableCameraAnimation();
        foreach (int frameIndex, m_frameIndices) {
            animation->deleteKeyframes(frameIndex);
            const QModelIndex &index = m_smm->frameIndexToModelIndex(m_cameraTreeItem, frameIndex);
            m_smm->setData(index, QVariant());
        }
        foreach (const QModelIndex &index, m_indices) {
            const QByteArray &bytes = index.data(SceneMotionModel::kBinaryDataRole).toByteArray();
            m_smm->setData(index, bytes, Qt::EditRole);
            CameraKeyframe *frame = new CameraKeyframe();
            frame->read(reinterpret_cast<const uint8_t *>(bytes.constData()));
            animation->replaceKeyframe(frame);
        }
        animation->refresh();
        m_smm->refreshScene();
    }
    virtual void redo() {
        CameraAnimation *animation = m_smm->currentMotion()->mutableCameraAnimation();
        foreach (const SceneMotionModel::KeyFramePair &pair, m_frames) {
            int frameIndex = pair.first;
            SceneMotionModel::KeyFramePtr ptr = pair.second;
            CameraKeyframe *frame = static_cast<CameraKeyframe *>(ptr.data());
            const QModelIndex &modelIndex = m_smm->frameIndexToModelIndex(m_cameraTreeItem, frameIndex);
            QByteArray bytes(CameraKeyframe::strideSize(), '0');
            CameraKeyframe *newFrame = static_cast<CameraKeyframe *>(frame->clone());
            newFrame->setInterpolationParameter(CameraKeyframe::kX, m_parameter.x);
            newFrame->setInterpolationParameter(CameraKeyframe::kY, m_parameter.y);
            newFrame->setInterpolationParameter(CameraKeyframe::kZ, m_parameter.z);
            newFrame->setInterpolationParameter(CameraKeyframe::kRotation, m_parameter.rotation);
            newFrame->setInterpolationParameter(CameraKeyframe::kFovy, m_parameter.fovy);
            newFrame->setInterpolationParameter(CameraKeyframe::kDistance, m_parameter.distance);
            newFrame->setFrameIndex(frameIndex);
            newFrame->write(reinterpret_cast<uint8_t *>(bytes.data()));
            animation->replaceKeyframe(newFrame);
            m_smm->setData(modelIndex, bytes);
        }
        animation->refresh();
        m_smm->refreshScene();
    }

private:
    QList<int> m_frameIndices;
    QModelIndexList m_indices;
    SceneMotionModel::KeyFramePairList m_frames;
    SceneMotionModel *m_smm;
    SceneMotionModel::ITreeItem *m_cameraTreeItem;
    CameraKeyframe::InterpolationParameter m_parameter;
};

}

SceneMotionModel::SceneMotionModel(QUndoGroup *undo, const SceneWidget *sceneWidget, QObject *parent)
    : MotionBaseModel(undo, parent),
      m_sceneWidget(sceneWidget),
      m_rootTreeItem(0),
      m_cameraTreeItem(0)
{
    RootTreeItem *root = new RootTreeItem();
    CameraTreeItem *camera = new CameraTreeItem(root);
    root->addChild(camera);
    m_rootTreeItem = root;
    m_cameraTreeItem = camera;
}

SceneMotionModel::~SceneMotionModel()
{
    delete m_cameraTreeItem;
    delete m_rootTreeItem;
}

QVariant SceneMotionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ITreeItem *item = static_cast<ITreeItem *>(index.internalPointer());
    if (role == Qt::DisplayRole && index.column() == 0) {
        return item->name();
    }
    else if (role == kBinaryDataRole) {
        QVariant value;
        if (index.row() == m_cameraTreeItem->rowIndex() && item->isCategory())
            value = m_cameraData.value(index);
        return value;
    }
    else {
        return QVariant();
    }
}

bool SceneMotionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        ITreeItem *item = static_cast<ITreeItem *>(index.internalPointer());
        if (index.row() == m_cameraTreeItem->rowIndex() && item->isCategory()) {
            m_cameraData.insert(index, value);
            setModified(true);
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}

int SceneMotionModel::columnCount(const QModelIndex & /* parent */) const
{
    return maxFrameCount() + 2;
}

int SceneMotionModel::maxFrameCount() const
{
    return 54000;
}

int SceneMotionModel::maxFrameIndex() const
{
    return m_motion ? m_motion->maxFrameIndex() : 0;
}

const QModelIndex SceneMotionModel::frameIndexToModelIndex(ITreeItem *item, int frameIndex) const
{
    QModelIndex modelIndex;
    int rowIndex = item->rowIndex();
    if (rowIndex == m_cameraTreeItem->rowIndex() && item->isCategory()) {
        modelIndex = index(rowIndex, toModelIndex(frameIndex), QModelIndex());
        if (!modelIndex.isValid())
            createIndex(rowIndex, frameIndex, item);
    }
    else {
        const QModelIndex &parentIndex = index(item->parent()->rowIndex(), 0);
        modelIndex = index(rowIndex, toModelIndex(frameIndex), parentIndex);
        if (!modelIndex.isValid())
            createIndex(rowIndex, frameIndex, item);
    }
    return modelIndex;
}

void SceneMotionModel::saveMotion(VMDMotion *motion)
{
    CameraAnimation *animation = motion->mutableCameraAnimation();
    if (m_cameraData.size() > 1) {
        foreach (const QVariant &value, m_cameraData) {
            CameraKeyframe *newFrame = new CameraKeyframe();
            const QByteArray &bytes = value.toByteArray();
            newFrame->read(reinterpret_cast<const uint8_t *>(bytes.constData()));
            animation->addKeyframe(newFrame);
        }
    }
    setModified(false);
}

void SceneMotionModel::addKeyframesByModelIndices(const QModelIndexList &indices)
{
    KeyFramePairList sceneFrames;
    foreach (const QModelIndex &index, indices) {
        ITreeItem *item = static_cast<ITreeItem *>(index.internalPointer());
        int frameIndex = toFrameIndex(index);
        if (frameIndex >= 0) {
            if (index.row() == m_cameraTreeItem->rowIndex() && item->isCategory()) {
                CameraKeyframe *frame = new CameraKeyframe();
                const Scene *scene = m_sceneWidget->sceneLoader()->renderEngine()->scene();
                frame->setDefaultInterpolationParameter();
                frame->setPosition(scene->cameraPosition());
                frame->setAngle(scene->cameraAngle());
                frame->setFovy(scene->fovy());
                frame->setDistance(scene->cameraDistance());
                sceneFrames.append(KeyFramePair(frameIndex, KeyFramePtr(frame)));
            }
        }
    }
    setFrames(sceneFrames);
}

void SceneMotionModel::copyKeyframes(int frameIndex)
{
    m_cameraIndex = frameIndexToModelIndex(m_cameraTreeItem, frameIndex);
}

void SceneMotionModel::pasteKeyframes(int frameIndex)
{
    if (m_cameraIndex.isValid()) {
        const QVariant &variant = m_cameraIndex.data(SceneMotionModel::kBinaryDataRole);
        if (variant.canConvert(QVariant::ByteArray)) {
            KeyFramePairList frames;
            CameraKeyframe *frame = new CameraKeyframe();
            frame->read(reinterpret_cast<const uint8_t *>(variant.toByteArray().constData()));
            frames.append(KeyFramePair(frameIndex, KeyFramePtr(frame)));
            addUndoCommand(new SetFramesCommand(this, frames, m_cameraTreeItem));
        }
    }
}

void SceneMotionModel::selectKeyframesByModelIndices(const QModelIndexList &indices)
{
    QList<KeyFramePtr> frames;
    foreach (const QModelIndex &index, indices) {
        if (index.isValid()) {
            const QVariant &data = index.data(kBinaryDataRole);
            if (data.canConvert(QVariant::ByteArray)) {
                CameraKeyframe *frame = new CameraKeyframe();
                frame->read(reinterpret_cast<const uint8_t *>(data.toByteArray().constData()));
                frames.append(KeyFramePtr(frame));
            }
        }
    }
    if (!frames.isEmpty())
        keyframesDidSelect(frames);
}

const QByteArray SceneMotionModel::nameFromModelIndex(const QModelIndex & /* index */) const
{
    return QByteArray();
}

void SceneMotionModel::loadMotion(VMDMotion *motion)
{
    m_cameraData.clear();
    if (motion) {
        const CameraAnimation &animation = motion->cameraAnimation();
        const int nCameraFrames = animation.countKeyframes();
        for (int i = 0; i < nCameraFrames; i++) {
            const CameraKeyframe *frame = animation.frameAt(i);
            int frameIndex = static_cast<int>(frame->frameIndex());
            QByteArray bytes(CameraKeyframe::strideSize(), '0');
            const QModelIndex &modelIndex = frameIndexToModelIndex(m_cameraTreeItem, frameIndex);
            // It seems to use CameraKeyFrame#clone()
            CameraKeyframe newFrame;
            newFrame.setPosition(frame->position());
            newFrame.setAngle(frame->angle());
            newFrame.setFovy(frame->fovy());
            newFrame.setDistance(frame->distance());
            newFrame.setFrameIndex(frameIndex);
            QuadWord v;
            for (int i = 0; i < CameraKeyframe::kMax; i++) {
                CameraKeyframe::InterpolationType type = static_cast<CameraKeyframe::InterpolationType>(i);
                frame->getInterpolationParameter(type, v);
                newFrame.setInterpolationParameter(type, v);
            }
            newFrame.write(reinterpret_cast<uint8_t *>(bytes.data()));
            setData(modelIndex, bytes);
        }
        m_motion = motion;
    }
    reset();
}

void SceneMotionModel::setFrames(const KeyFramePairList &frames)
{
    if (m_motion)
        addUndoCommand(new SetFramesCommand(this, frames, m_cameraTreeItem));
    else
        qWarning("No motion to register camera frames.");
}

void SceneMotionModel::refreshScene()
{
    reset();
}

void SceneMotionModel::removeMotion()
{
    m_cameraData.clear();
    setModified(false);
    reset();
}

void SceneMotionModel::deleteKeyframesByModelIndices(const QModelIndexList &indices)
{
    static const uint8_t *kNoName = reinterpret_cast<const uint8_t *>("");
    CameraAnimation *animation = m_motion->mutableCameraAnimation();
    foreach (const QModelIndex &index, indices) {
        if (index.isValid() && index.column() > 1) {
            ITreeItem *item = static_cast<ITreeItem *>(index.internalPointer());
            if (index.row() == m_cameraTreeItem->rowIndex() && item->isCategory())
                animation->deleteKeyframe(toFrameIndex(index), kNoName);
            setData(index, QVariant());
        }
    }
}

void SceneMotionModel::applyKeyframeWeightByModelIndices(const QModelIndexList & /* indices */, float /* value */)
{
    /* 現在この処理はまだ何も無い */
}
