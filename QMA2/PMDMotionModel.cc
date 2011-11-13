/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2011  hkrn                                    */
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

#include "PMDMotionModel.h"
#include "util.h"
#include <QtGui/QtGui>
#include <vpvl/vpvl.h>

PMDMotionModel::PMDMotionModel(QUndoGroup *undo, QObject *parent) :
    MotionBaseModel(undo, parent),
    m_model(0),
    m_state(0)
{
    m_roots.insert(0, RootPtr(0));
    m_keys.insert(0, Keys());
    m_values.insert(0, Values());
}

PMDMotionModel::~PMDMotionModel()
{
    if (m_model)
        m_model->discardState(m_state);
    if (m_state)
        qWarning("It seems memory leak occured: m_state");
}

QVariant PMDMotionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == 0) {
        ITreeItem *item = static_cast<ITreeItem *>(index.internalPointer());
        return item->name();
    }
    else if (role == kBinaryDataRole && m_model) {
        QVariant value = m_values[m_model].value(index);
        return value;
    }
    else {
        return QVariant();
    }
}

bool PMDMotionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (m_model && index.isValid() && role == Qt::EditRole) {
        m_values[m_model].insert(index, value);
        setModified(true);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

const QModelIndex PMDMotionModel::frameIndexToModelIndex(ITreeItem *item, int frameIndex) const
{
    int rowIndex = item->rowIndex();
    const QModelIndex &parentIndex = index(item->parent()->rowIndex(), 0);
    const QModelIndex modelIndex = index(rowIndex, toModelIndex(frameIndex), parentIndex);
    if (!modelIndex.isValid())
        createIndex(rowIndex, frameIndex, item);
    return modelIndex;
}

void PMDMotionModel::saveState()
{
    if (m_model) {
        m_model->discardState(m_state);
        m_state = m_model->saveState();
    }
}

void PMDMotionModel::restoreState()
{
    if (m_model) {
        m_model->restoreState(m_state);
        m_model->updateImmediate();
        m_model->discardState(m_state);
    }
}

void PMDMotionModel::discardState()
{
    if (m_model)
        m_model->discardState(m_state);
}

int PMDMotionModel::columnCount(const QModelIndex & /* parent */) const
{
    return m_model ? maxFrameCount() + 2 : 1;
}

void PMDMotionModel::markAsNew(vpvl::PMDModel *model)
{
    if (model == m_model)
        setModified(false);
}

void PMDMotionModel::refreshModel()
{
    updateModel();
    reset();
    emit motionDidUpdate(m_model);
}

int PMDMotionModel::maxFrameCount() const
{
    return 54000;
}

void PMDMotionModel::updateModel()
{
    if (m_model) {
        m_model->seekMotion(m_frameIndex);
        m_model->updateImmediate();
    }
}

void PMDMotionModel::addPMDModel(vpvl::PMDModel *model, const RootPtr &root, const Keys &keys)
{
    if (!m_stacks.contains(model)) {
        QUndoStack *stack = new QUndoStack();
        m_stacks.insert(model, UndoStackPtr(stack));
        m_undo->addStack(stack);
        m_undo->setActiveStack(stack);
    }
    else {
        m_undo->setActiveStack(m_stacks[model].data());
    }
    if (!m_roots.contains(model))
        m_roots.insert(model, root);
    if (!m_keys.contains(model))
        m_keys.insert(model, keys);
    if (!m_values.contains(model))
        m_values.insert(model, Values());
}

void PMDMotionModel::removePMDModel(vpvl::PMDModel *model)
{
    m_model = 0;
    m_undo->setActiveStack(0);
    m_values.remove(model);
    m_keys.remove(model);
    m_stacks.remove(model);
    m_roots.remove(model);
}

void PMDMotionModel::removePMDMotion(vpvl::PMDModel *model)
{
    if (m_values.contains(model))
        m_values[model].clear();
    QUndoStack *stack = m_undo->activeStack();
    if (stack)
        stack->clear();
}
