#pragma once

#include "fl/stdint.h"

#include "fl/namespace.h"
#include "fl/ptr.h"
#include "fl/xymap.h"
#include "fx/fx.h"
#include "fl/int.h"

namespace fl {

FASTLED_SMART_PTR(Fx2d);

// Abstract base class for 2D effects that use a grid, which is defined
// by an XYMap.
class Fx2d : public Fx {
  public:
    // XYMap holds either a function or a look up table to map x, y coordinates
    // to a 1D index.
    Fx2d(const XYMap &xyMap) : Fx(xyMap.getTotal()), mXyMap(xyMap) {}
    fl::u16 xyMap(fl::u16 x, fl::u16 y) const {
        return mXyMap.mapToIndex(x, y);
    }
    fl::u16 getHeight() const { return mXyMap.getHeight(); }
    fl::u16 getWidth() const { return mXyMap.getWidth(); }
    void setXYMap(const XYMap &xyMap) { mXyMap = xyMap; }
    XYMap &getXYMap() { return mXyMap; }
    const XYMap &getXYMap() const { return mXyMap; }

  protected:
    XYMap mXyMap;
};

} // namespace fl
