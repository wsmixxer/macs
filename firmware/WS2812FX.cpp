/*
  WS2812FX.cpp - Library for WS2812 LED effects.

  Harm Aldick - 2016
  www.aldick.org

  Modified by Winston Salem Mixxer (2019) for use with Particle Photon, stripped
  down to bare essentials
  wsmixxer.org

  FEATURES
    * A lot of blinken modes and counting
    * WS2812FX can be used as drop-in replacement for Adafruit NeoPixel Library

  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "WS2812FX.h"

void WS2812FX::init() {
  resetSegmentRuntimes();
  Adafruit_NeoPixel::begin();
}

// void WS2812FX::timer() {
//     for (int j=0; j < 1000; j++) {
//       uint16_t delay = (this->*_mode[SEGMENT.mode])();
//     }
// }

void WS2812FX::service() {
  if(_running || _triggered) {
    unsigned long now = millis(); // Be aware, millis() rolls over every 49 days
    bool doShow = false;
    for(uint8_t i=0; i < _num_segments; i++) {
      _segment_index = i;
      CLR_FRAME;
      if(now > SEGMENT_RUNTIME.next_time || _triggered) {
        SET_FRAME;
        doShow = true;
        uint16_t delay = (this->*_mode[SEGMENT.mode])();
        SEGMENT_RUNTIME.next_time = now + max(delay, SPEED_MIN);
        SEGMENT_RUNTIME.counter_mode_call++;
      }
    }
    if(doShow) {
      delay(1); // for ESP32 (see https://forums.adafruit.com/viewtopic.php?f=47&t=117327)
      show();
    }
    _triggered = false;
  }
}

// overload setPixelColor() functions so we can use gamma correction
// (see https://learn.adafruit.com/led-tricks-gamma-correction/the-issue)
void WS2812FX::setPixelColor(uint16_t n, uint32_t c) {
  if(IS_GAMMA) {
    uint8_t w = (c >> 24) & 0xFF;
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >>  8) & 0xFF;
    uint8_t b =  c        & 0xFF;
    Adafruit_NeoPixel::setPixelColor(n, gamma8(r), gamma8(g), gamma8(b), gamma8(w));
  } else {
    Adafruit_NeoPixel::setPixelColor(n, c);
  }
}

void WS2812FX::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(IS_GAMMA) {
    Adafruit_NeoPixel::setPixelColor(n, gamma8(r), gamma8(g), gamma8(b));
  } else {
    Adafruit_NeoPixel::setPixelColor(n, r, g, b);
  }
}

void WS2812FX::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if(IS_GAMMA) {
    Adafruit_NeoPixel::setPixelColor(n, gamma8(r), gamma8(g), gamma8(b), gamma8(w));
  } else {
    Adafruit_NeoPixel::setPixelColor(n, r, g, b, w);
  }
}

void WS2812FX::copyPixels(uint16_t dest, uint16_t src, uint16_t count) {
  uint8_t *pixels = getPixels();
  uint8_t bytesPerPixel = getNumBytesPerPixel(); // 3=RGB, 4=RGBW

  memmove(pixels + (dest * bytesPerPixel), pixels + (src * bytesPerPixel), count * bytesPerPixel);
}

// overload show() functions so we can use custom show()
void WS2812FX::show(void) {
  if(customShow == NULL) {
    Adafruit_NeoPixel::show();
  } else {
    customShow();
  }
}

void WS2812FX::start() {
  resetSegmentRuntimes();
  _running = true;
}

void WS2812FX::stop() {
  _running = false;
  strip_off();
}

void WS2812FX::pause() {
  _running = false;
}

void WS2812FX::resume() {
  _running = true;
}

void WS2812FX::trigger() {
  _triggered = true;
}

void WS2812FX::setMode(uint8_t m) {
  setMode(0, m);
}

void WS2812FX::setMode(uint8_t seg, uint8_t m) {
  resetSegmentRuntime(seg);
  _segments[seg].mode = constrain(m, 0, MODE_COUNT - 1);
}

void WS2812FX::setOptions(uint8_t seg, uint8_t o) {
  _segments[seg].options = o;
}

void WS2812FX::setSpeed(uint16_t s) {
  setSpeed(0, s);
}

void WS2812FX::setSpeed(uint8_t seg, uint16_t s) {
//  resetSegmentRuntime(seg);
  _segments[seg].speed = constrain(s, SPEED_MIN, SPEED_MAX);
}

void WS2812FX::increaseSpeed(uint8_t s) {
  uint16_t newSpeed = constrain(SEGMENT.speed + s, SPEED_MIN, SPEED_MAX);
  setSpeed(newSpeed);
}

void WS2812FX::decreaseSpeed(uint8_t s) {
  uint16_t newSpeed = constrain(SEGMENT.speed - s, SPEED_MIN, SPEED_MAX);
  setSpeed(newSpeed);
}

void WS2812FX::setColor(uint8_t r, uint8_t g, uint8_t b) {
  setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void WS2812FX::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  setColor((((uint32_t)w << 24)| ((uint32_t)r << 16) | ((uint32_t)g << 8)| ((uint32_t)b)));
}

void WS2812FX::setColor(uint32_t c) {
  setColor(0, c);
}

void WS2812FX::setColor(uint8_t seg, uint32_t c) {
//  resetSegmentRuntime(seg);
  _segments[seg].colors[0] = c;
}

void WS2812FX::setColors(uint8_t seg, uint32_t* c) {
//  resetSegmentRuntime(seg);
  for(uint8_t i=0; i<NUM_COLORS; i++) {
    _segments[seg].colors[i] = c[i];
  }
}

void WS2812FX::setBrightness(uint8_t b) {
  b = constrain(b, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  Adafruit_NeoPixel::setBrightness(b);
  show();
}

void WS2812FX::increaseBrightness(uint8_t s) {
  s = constrain(getBrightness() + s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  setBrightness(s);
}

void WS2812FX::decreaseBrightness(uint8_t s) {
  s = constrain(getBrightness() - s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  setBrightness(s);
}

void WS2812FX::setLength(uint16_t b) {
  resetSegmentRuntimes();
  if (b < 1) b = 1;

  // Decrease numLEDs to maximum available memory
  do {
      Adafruit_NeoPixel::updateLength(b);
      b--;
  } while(!Adafruit_NeoPixel::numLEDs && b > 1);

  _segments[0].start = 0;
  _segments[0].stop = Adafruit_NeoPixel::numLEDs - 1;
}

void WS2812FX::increaseLength(uint16_t s) {
  s = _segments[0].stop - _segments[0].start + 1 + s;
  setLength(s);
}

void WS2812FX::decreaseLength(uint16_t s) {
  if (s > _segments[0].stop - _segments[0].start + 1) s = 1;
  s = _segments[0].stop - _segments[0].start + 1 - s;

  for(uint16_t i=_segments[0].start + s; i <= (_segments[0].stop - _segments[0].start + 1); i++) {
    setPixelColor(i, 0);
  }
  show();

  setLength(s);
}

boolean WS2812FX::isRunning() {
  return _running;
}

boolean WS2812FX::isTriggered() {
  return _triggered;
}

boolean WS2812FX::isFrame() {
  return isFrame(0);
}

boolean WS2812FX::isFrame(uint8_t segIndex) {
  return (_segment_runtimes[segIndex].aux_param2 & FRAME);
}

boolean WS2812FX::isCycle() {
  return isCycle(0);
}

boolean WS2812FX::isCycle(uint8_t segIndex) {
  return (_segment_runtimes[segIndex].aux_param2 & CYCLE);
}

uint8_t WS2812FX::getMode(void) {
  return getMode(0);
}

uint8_t WS2812FX::getMode(uint8_t seg) {
  return _segments[seg].mode;
}

uint16_t WS2812FX::getSpeed(void) {
  return getSpeed(0);
}

uint16_t WS2812FX::getSpeed(uint8_t seg) {
  return _segments[seg].speed;
}


uint8_t WS2812FX::getOptions(uint8_t seg) {
  return _segments[seg].options;
}

uint16_t WS2812FX::getLength(void) {
  return numPixels();
}

uint16_t WS2812FX::getNumBytes(void) {
  return numBytes;
}

uint8_t WS2812FX::getNumBytesPerPixel(void) {
  return (wOffset == rOffset) ? 3 : 4; // 3=RGB, 4=RGBW
}

uint8_t WS2812FX::getModeCount(void) {
  return MODE_COUNT;
}

uint8_t WS2812FX::getNumSegments(void) {
  return _num_segments;
}

void WS2812FX::setNumSegments(uint8_t n) {
  _num_segments = n;
}

uint32_t WS2812FX::getColor(void) {
  return getColor(0);
}

uint32_t WS2812FX::getColor(uint8_t seg) {
  return _segments[seg].colors[0];
}

uint32_t* WS2812FX::getColors(uint8_t seg) {
  return _segments[seg].colors;
}

WS2812FX::Segment* WS2812FX::getSegment(void) {
  return &_segments[_segment_index];
}

WS2812FX::Segment* WS2812FX::getSegment(uint8_t seg) {
  return &_segments[seg];
}

WS2812FX::Segment* WS2812FX::getSegments(void) {
  return _segments;
}

WS2812FX::Segment_runtime* WS2812FX::getSegmentRuntime(void) {
  return &_segment_runtimes[_segment_index];
}

WS2812FX::Segment_runtime* WS2812FX::getSegmentRuntime(uint8_t seg) {
  return &_segment_runtimes[seg];
}

WS2812FX::Segment_runtime* WS2812FX::getSegmentRuntimes(void) {
  return _segment_runtimes;
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, uint32_t color, uint16_t speed, bool reverse) {
  uint32_t colors[] = {color, 0, 0};
  setSegment(n, start, stop, mode, colors, speed, reverse);
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, uint32_t color, uint16_t speed, uint8_t options) {
  uint32_t colors[] = {color, 0, 0};
  setSegment(n, start, stop, mode, colors, speed, options);
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, const uint32_t colors[], uint16_t speed, bool reverse) {
  setSegment(n, start, stop, mode, colors, speed, (uint8_t)(reverse ? REVERSE : NO_OPTIONS));
}

void WS2812FX::setSegment(uint8_t n, uint16_t start, uint16_t stop, uint8_t mode, const uint32_t colors[], uint16_t speed, uint8_t options) {
  if(n < (sizeof(_segments) / sizeof(_segments[0]))) {
    if(n + 1 > _num_segments) _num_segments = n + 1;
    _segments[n].start = start;
    _segments[n].stop = stop;
    _segments[n].mode = mode;
    _segments[n].speed = speed;
    _segments[n].options = options;

    for(uint8_t i=0; i<NUM_COLORS; i++) {
      _segments[n].colors[i] = colors[i];
    }
  }
}

void WS2812FX::resetSegments() {
  resetSegmentRuntimes();
  memset(_segments, 0, sizeof(_segments));
  _segment_index = 0;
  _num_segments = 1;
  setSegment(0, 0, 7, FX_MODE_STATIC, (const uint32_t[]){DEFAULT_COLOR, 0, 0}, DEFAULT_SPEED, NO_OPTIONS);
}

void WS2812FX::resetSegmentRuntimes() {
  memset(_segment_runtimes, 0, sizeof(_segment_runtimes));
}

void WS2812FX::resetSegmentRuntime(uint8_t seg) {
  memset(&_segment_runtimes[seg], 0, sizeof(_segment_runtimes[0]));
}

/* #####################################################
#
#  Color and Blinken Functions
#
##################################################### */

/*
 * Turns everything off. Doh.
 */
void WS2812FX::strip_off() {
  Adafruit_NeoPixel::clear();
  show();
}


/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 */
uint32_t WS2812FX::color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if(pos < 170) {
    pos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
}

// Return the sum of all LED intensities (can be used for
// rudimentary power calculations)
uint32_t WS2812FX::intensitySum() {
  uint8_t *pixels = getPixels();
  uint32_t sum = 0;
  for(uint16_t i=0; i <numBytes; i++) {
    sum+= pixels[i];
  }
  return sum;
}

// Return the sum of each color's intensity. Note, the order of
// intensities in the returned array depends on the type of WS2812
// LEDs you have. NEO_GRB LEDs will return an array with entries
// in a different order then NEO_RGB LEDs.
uint32_t* WS2812FX::intensitySums() {
  static uint32_t intensities[] = { 0, 0, 0, 0 };
  memset(intensities, 0, sizeof(intensities));

  uint8_t *pixels = getPixels();
  uint8_t bytesPerPixel = getNumBytesPerPixel(); // 3=RGB, 4=RGBW
  for(uint16_t i=0; i <numBytes; i += bytesPerPixel) {
    intensities[0] += pixels[i];
    intensities[1] += pixels[i + 1];
    intensities[2] += pixels[i + 2];
    if(bytesPerPixel == 4) intensities[3] += pixels[i + 3]; // for RGBW LEDs
  }
  return intensities;
}

/*
 * No blinking. Just plain old static light.
 */
uint16_t WS2812FX::mode_static(void) {
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    setPixelColor(i, SEGMENT.colors[0]);
  }
  return 500;
}


/*
 * Blink/strobe function
 * Alternate between color1 and color2
 * if(strobe == true) then create a strobe effect
 */
uint16_t WS2812FX::blink(uint32_t color1, uint32_t color2, bool strobe) {
  uint32_t color = ((SEGMENT_RUNTIME.counter_mode_call & 1) == 0) ? color1 : color2;
  if(IS_REVERSE) color = (color == color1) ? color2 : color1;
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    setPixelColor(i, color);
  }

  if((SEGMENT_RUNTIME.counter_mode_call & 1) == 0) {
    return strobe ? 20 : (SEGMENT.speed / 2);
  } else {
    return strobe ? SEGMENT.speed - 20 : (SEGMENT.speed / 2);
  }
}


/*
 * Normal blinking. 50% on/off time.
 */
uint16_t WS2812FX::mode_blink(void) {
  return blink(SEGMENT.colors[0], SEGMENT.colors[1], false);
}


/*
 * Classic Strobe effect.
 */
uint16_t WS2812FX::mode_strobe(void) {
  return blink(SEGMENT.colors[0], SEGMENT.colors[1], true);
}


/*
 * Does the "standby-breathing" of well known i-Devices. Fixed Speed.
 * Use mode "fade" if you like to have something similar with a different speed.
 */
uint16_t WS2812FX::mode_breath(void) {
  int lum = SEGMENT_RUNTIME.counter_mode_step;
  if(lum > 255) lum = 511 - lum; // lum = 15 -> 255 -> 15

  uint16_t delay;
  if(lum == 15) delay = 970; // 970 pause before each breath
  else if(lum <=  25) delay = 38; // 19
  else if(lum <=  50) delay = 36; // 18
  else if(lum <=  75) delay = 28; // 14
  else if(lum <= 100) delay = 20; // 10
  else if(lum <= 125) delay = 14; // 7
  else if(lum <= 150) delay = 11; // 5
  else delay = 10; // 4

  uint32_t color = SEGMENT.colors[0];
  uint8_t w = (color >> 24 & 0xFF) * lum / 256;
  uint8_t r = (color >> 16 & 0xFF) * lum / 256;
  uint8_t g = (color >>  8 & 0xFF) * lum / 256;
  uint8_t b = (color       & 0xFF) * lum / 256;
  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    setPixelColor(i, r, g, b, w);
  }

  SEGMENT_RUNTIME.counter_mode_step += 2;
  if(SEGMENT_RUNTIME.counter_mode_step > (512-15)) SEGMENT_RUNTIME.counter_mode_step = 15;
  return delay;
}


/*
 * fade out functions
 */
void WS2812FX::fade_out() {
  return fade_out(SEGMENT.colors[1]);
}

void WS2812FX::fade_out(uint32_t targetColor) {
  static const uint8_t rateMapH[] = {0, 1, 1, 1, 2, 3, 4, 6};
  static const uint8_t rateMapL[] = {0, 2, 3, 8, 8, 8, 8, 8};

  uint8_t rate  = FADE_RATE;
  uint8_t rateH = rateMapH[rate];
  uint8_t rateL = rateMapL[rate];

  uint32_t color = targetColor;
  int w2 = (color >> 24) & 0xff;
  int r2 = (color >> 16) & 0xff;
  int g2 = (color >>  8) & 0xff;
  int b2 =  color        & 0xff;

  for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
    color = getPixelColor(i); // current color
    if(rate == 0) { // old fade-to-black algorithm
      setPixelColor(i, (color >> 1) & 0x7F7F7F7F);
    } else { // new fade-to-color algorithm
      int w1 = (color >> 24) & 0xff;
      int r1 = (color >> 16) & 0xff;
      int g1 = (color >>  8) & 0xff;
      int b1 =  color        & 0xff;

      // calculate the color differences between the current and target colors
      int wdelta = w2 - w1;
      int rdelta = r2 - r1;
      int gdelta = g2 - g1;
      int bdelta = b2 - b1;

      // if the current and target colors are almost the same, jump right to the target
      // color, otherwise calculate an intermediate color. (fixes rounding issues)
      wdelta = abs(wdelta) < 3 ? wdelta : (wdelta >> rateH) + (wdelta >> rateL);
      rdelta = abs(rdelta) < 3 ? rdelta : (rdelta >> rateH) + (rdelta >> rateL);
      gdelta = abs(gdelta) < 3 ? gdelta : (gdelta >> rateH) + (gdelta >> rateL);
      bdelta = abs(bdelta) < 3 ? bdelta : (bdelta >> rateH) + (bdelta >> rateL);

      setPixelColor(i, r1 + rdelta, g1 + gdelta, b1 + bdelta, w1 + wdelta);
    }
  }
}
