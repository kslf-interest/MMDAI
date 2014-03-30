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

#include <vpvl2/ICameraKeyframe.h>

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{
namespace extensions
{
namespace vpdb
{

class Motion;

class VPVL2_API CameraKeyframe VPVL2_DECL_FINAL : public ICameraKeyframe {
public:
    CameraKeyframe(Motion *parent);
    ~CameraKeyframe();

    void read(const uint8 *data);
    void write(uint8 *data) const;
    vsize estimateSize() const;
    const IString *name() const;
    TimeIndex timeIndex() const;
    LayerIndex layerIndex() const;
    void setName(const IString *value);
    void setTimeIndex(const TimeIndex &value);
    void setLayerIndex(const LayerIndex &value);
    Type type() const;

    ICameraKeyframe *clone() const;
    void setDefaultInterpolationParameter();
    void setInterpolationParameter(InterpolationType type, const QuadWord &value);
    void getInterpolationParameter(InterpolationType type, QuadWord &value) const;
    Vector3 lookAt() const;
    Vector3 angle() const;
    Scalar distance() const;
    Scalar fov() const;
    bool isPerspective() const;
    void setLookAt(const Vector3 &value);
    void setAngle(const Vector3 &value);
    void setDistance(const Scalar &value);
    void setFov(const Scalar &value);
    void setPerspective(bool value);

private:
    struct PrivateContext;
    PrivateContext *m_context;
};

} /* namespace vpdb */
} /* namespace extensions */
} /* namespace VPVL2_VERSION_NS */
using namespace VPVL2_VERSION_NS;

} /* namespace vpvl2 */
