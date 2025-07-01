#ifndef __INC_OCTOWS2811_CONTROLLER_H
#define __INC_OCTOWS2811_CONTROLLER_H

#ifdef USE_OCTOWS2811

#include "OctoWS2811.h"
#include "fl/int.h"

FASTLED_NAMESPACE_BEGIN

template<EOrder RGB_ORDER = GRB, fl::u8 CHIP = WS2811_800kHz>
class COctoWS2811Controller : public CPixelLEDController<RGB_ORDER, 8, 0xFF> {
  OctoWS2811  *pocto;
  fl::u8 *drawbuffer,*framebuffer;

  void _init(int nLeds) {
    if(pocto == NULL) {
      drawbuffer = (fl::u8*)malloc(nLeds * 8 * 3);
      framebuffer = (fl::u8*)malloc(nLeds * 8 * 3);

      // byte ordering is handled in show by the pixel controller
      int config = WS2811_RGB;
      config |= CHIP;

      pocto = new OctoWS2811(nLeds, framebuffer, drawbuffer, config);

      pocto->begin();
    }
  }
public:
  COctoWS2811Controller() { pocto = NULL; }
  virtual int size() { return CLEDController::size() * 8; }

  virtual void init() { /* do nothing yet */ }

  virtual void showPixels(PixelController<RGB_ORDER, 8, 0xFF> &pixels) {
    uint32_t size = pixels.size();
    uint32_t sizeTimes8 = 8U * size;
    _init(size);

    uint32_t index = 0;
    while (pixels.has(1)) {
      for (int lane = 0; lane < 8; lane++) {
        fl::u8 r = pixels.loadAndScale0(lane);
        fl::u8 g = pixels.loadAndScale1(lane);
        fl::u8 b = pixels.loadAndScale2(lane);
        pocto->setPixel(index, r, g, b);
        index += size;
      }
      index -= sizeTimes8;
      index++;
      pixels.stepDithering();
      pixels.advanceData();
    }

    pocto->show();
  }

};

FASTLED_NAMESPACE_END

#endif

#endif
