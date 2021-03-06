/**

 Copyright (c) 2010-2014  hkrn

 All rights reserved.

 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:

 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
 - Neither the name of the MMDAI project team nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once
#ifndef VPVL2_INTERNAL_MODELHELPER_H_
#define VPVL2_INTERNAL_MODELHELPER_H_

#include "vpvl2/Common.h"
#include "vpvl2/IModel.h"
#include "vpvl2/internal/util.h"

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{
namespace internal
{

enum {
    kPMDBoneNameSize = 20,
    kPMDBoneCategoryNameSize = 50,
    kPMDJointNameSize = 20,
    kPMDMaterialNameSize = 20,
    kPMDModelNameSize = 20,
    kPMDModelCommentSize = 256,
    kPMDModelCustomToonTextureSize = 100,
    kPMDMorphNameSize = 20,
    kPMDRigidBodyNameSize = 20,
    kPMDVertexMaxBoneSize = 2
};

class ModelHelper VPVL2_DECL_FINAL {
public:
    static inline void transformVertex(const Transform &transform,
                                       const Vector3 &inPosition,
                                       const Vector3 &inNormal,
                                       Vector3 &outPosition,
                                       Vector3 &outNormal) VPVL2_DECL_NOEXCEPT
    {
        outPosition = transform * inPosition;
        outNormal = transform.getBasis() * inNormal;
    }
    static inline void transformVertex(const Transform &transformA,
                                       const Transform &transformB,
                                       const Vector3 &inPosition,
                                       const Vector3 &inNormal,
                                       Vector3 &outPosition,
                                       Vector3 &outNormal,
                                       const IVertex::WeightPrecision &weight) VPVL2_DECL_NOEXCEPT
    {
        const Vector3 &v1 = transformA * inPosition;
        const Vector3 &n1 = transformA.getBasis() * inNormal;
        const Vector3 &v2 = transformB * inPosition;
        const Vector3 &n2 = transformB.getBasis() * inNormal;
        const Scalar &w = Scalar(weight);
        outPosition.setInterpolate3(v2, v1, w);
        outNormal.setInterpolate3(n2, n1, w);
    }
    static inline uint8 adjustSharedToonTextureIndex(uint8 value) VPVL2_DECL_NOEXCEPT {
        return (value == 0xff) ? 0 : value + 1;
    }
    static inline bool hasBoneLoopChain(const IBone * /* parentBoneRef */, const IModel * /* baseModelRef */) VPVL2_DECL_NOEXCEPT {
        /* FIXME: implement this to stop loop chain */
        return false;
    }
    static inline bool hasModelLoopChain(const IModel *baseModelRef, const IModel *targetModelRef) VPVL2_DECL_NOEXCEPT {
        if (baseModelRef) {
            IModel *modelRef = baseModelRef->parentModelRef();
            while (modelRef) {
                if (modelRef == targetModelRef) {
                    return true;
                }
                modelRef = modelRef->parentModelRef();
            }
        }
        return false;
    }
    static inline void dumpName(const IString *value) {
        (void) value;
        VPVL2_VLOG(1, internal::cstr(value, "(null)"));
    }
    static inline void dumpPosition(const char *name, const Vector3 &position) {
        (void) name;
        (void) position;
        VPVL2_VLOG(1, name << ":" << position.x() << "," << position.y() << "," << position.z());
    }
    static inline void getTextureRefs(const Hash<HashString, IString *> &textures, Array<const IString *> &value) {
        const int nobjects = textures.count();
        value.clear();
        value.reserve(nobjects);
        for (int i = 0; i < nobjects; i++) {
            const IString *const *v = textures.value(i);
            value.append(*v);
        }
    }
    template<typename IndexType>
    static inline void swapIndices(IndexType *indicesPtr, const int nindices) VPVL2_DECL_NOEXCEPT {
        for (int i = 0; i < nindices; i += 3) {
            btSwap(indicesPtr[i], indicesPtr[i + 1]);
        }
    }
    template<typename T, typename I>
    static inline void getObjectRefs(const Array<T *> &objects, Array<I *> &value) {
        const int nobjects = objects.count();
        value.clear();
        value.reserve(nobjects);
        for (int i = 0; i < nobjects; i++) {
            T *object = objects[i];
            value.append(object);
        }
    }
    template<typename T, typename I>
    static inline I *findObjectAt(const Array<T *> &objects, int index) VPVL2_DECL_NOEXCEPT {
        return internal::checkBound(index, 0, objects.count()) ? objects[index] : 0;
    }
    template<typename T, typename I>
    static inline void addObject(IModel *modelRef, I *value, Array<T *> &objects) {
        if (value && value->index() == -1 && value->parentModelRef() == modelRef) {
            T *object = static_cast<T *>(value);
            object->setIndex(objects.count());
            objects.append(object);
        }
    }
    template<typename T, typename I>
    static inline void addObject2(IModel *modelRef, I *value, Array<I *> &objects) {
        if (value && value->index() == -1 && value->parentModelRef() == modelRef) {
            T *object = static_cast<T *>(value);
            object->setIndex(objects.count());
            objects.append(object);
        }
    }
    template<typename T, typename I>
    static inline void removeObject(IModel *modelRef, I *value, Array<T *> &objects) {
        if (value && value->parentModelRef() == modelRef) {
            T *object = static_cast<T *>(value);
            object->setIndex(-1);
            objects.remove(object);
        }
    }
    template<typename T, typename I>
    static inline void removeObject2(IModel *modelRef, I *value, Array<I *> &objects) {
        if (value && value->parentModelRef() == modelRef) {
            T *object = static_cast<T *>(value);
            object->setIndex(-1);
            objects.remove(object);
        }
    }
    template<typename IBone, typename TBone>
    static void removeBoneReferenceInBones(IBone *value, Array<TBone *> &bones) {
        const int nbones = bones.count();
        for (int i = 0; i < nbones; i++) {
            TBone *bone = bones[i];
            if (bone->parentBoneRef() == value) {
                bone->setParentBoneRef(0);
            }
            if (bone->parentInherentBoneRef() == value) {
                bone->setParentInherentBoneRef(0);
            }
            if (bone->destinationOriginBoneRef() == value) {
                bone->setDestinationOriginBoneRef(0);
            }
        }
    }
    template<typename IBone, typename TRigidBody>
    static void removeBoneReferenceInRigidBodies(IBone *value, Array<TRigidBody *> &rigidBodies) {
        const int nbodies = rigidBodies.count();
        for (int i = 0; i < nbodies; i++) {
            TRigidBody *body = rigidBodies[i];
            if (body->boneRef() == value) {
                body->setBoneRef(0);
            }
        }
    }
    template<typename IBone, typename TVertex>
    static void removeBoneReferenceInVertices(IBone *value, Array<TVertex *> &vertices) {
        const int nvertices = vertices.count();
        for (int i = 0; i < nvertices; i++) {
            TVertex *vertex = vertices[i];
            for (int j = 0; j < 4; j++) {
                if (vertex->boneRef(j) == value) {
                    vertex->setBoneRef(j, 0);
                }
            }
        }
    }
    template<typename IMaterial, typename TVertex>
    static void removeMaterialReferenceInVertices(IMaterial *value, Array<TVertex *> &vertices) {
        const int nvertices = vertices.count();
        for (int i = 0; i < nvertices; i++) {
            TVertex *vertex = vertices[i];
            if (vertex->materialRef() == value) {
                vertex->setMaterialRef(0);
            }
        }
    }
    template<typename IRigidBody, typename TJoint>
    static void removeRigidBodyReferenceInJoints(IRigidBody *value, Array<TJoint *> &joints) {
        const int njoints = joints.count();
        for (int i = 0; i < njoints; i++) {
            TJoint *joint = joints[i];
            if (joint->rigidBody1Ref() == value) {
                joint->setRigidBody1Ref(0);
            }
            if (joint->rigidBody2Ref() == value) {
                joint->setRigidBody2Ref(0);
            }
        }
    }
    static void setName(const IString *value, IString *&namePtr, IString *&englishNamePtr, IEncoding::LanguageType language) {
        switch (language) {
        case IEncoding::kDefaultLanguage:
        case IEncoding::kJapanese:
            if (value && !value->equals(namePtr)) {
                internal::setString(value, namePtr);
            }
            else if (!value && value != namePtr) {
                internal::deleteObject(namePtr);
            }
            break;
        case IEncoding::kEnglish:
            if (value && !value->equals(englishNamePtr)) {
                internal::setString(value, englishNamePtr);
            }
            else if (!value && value != englishNamePtr) {
                internal::deleteObject(englishNamePtr);
            }
            break;
        default:
            break;
        }
    }

private:
    VPVL2_MAKE_STATIC_CLASS(ModelHelper)
};

} /* namespace internal */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */

#endif
