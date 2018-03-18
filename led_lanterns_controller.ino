#include "FastLED.h"
#include <Encoder.h>
/* Notable list of colors (input to Wheel function):
 *
 * Lantern glow yellow: 11 - 16
 */

// Output to NeoPixels
#define LEDS_PIN 5

// Each group is any set of LEDs you want to control as a single unit
// In my case, I have 5 each of 16-NeoPixel rings
#define NUM_GROUPS 5
#define GROUP_SIZE 16
#define TOTAL_LEDS (NUM_GROUPS * GROUP_SIZE)

/* Analog inputs for potentiometers:
 *  A0-A4 - 5 total potentiometers
 *
 */

/* Number of resistors in voltage divider used to select options
 *  for program and color palette/brightness. You a number of
 *  "options" one greater than this.
 */

#define ADC_RES 1024

#define NUM_CHOICES 4
#define VDIV_DIST (ADC_RES / NUM_CHOICES)
#define HALF_VDIV_DIST (VDIV_DIST / 2)
#define ADC2BYTE 4

/* Inclusive lower bound for the input to the Wheel function,
 *  this function can take numbers lower than this, or higher
 *  than the upper bound, but it will convert those via the
 *  modulo operator to be in this range.
 *
 *  This represents red, on the orange side of the spectrum.
 */
#define WHEEL_MIN 0
// Exclusive upper bound (red on the purple side of spectrum)
#define WHEEL_MAX 256
#define VALUE_MAX 256

#define GROUP_SPACING (VALUE_MAX / NUM_GROUPS)
/* If we don't specify a range in the palette for colors, the
 * full spectrum is used.
 */
#define DEFAULT_COLOR_LB (WHEEL_MIN)
// Exclusive
#define DEFAULT_COLOR_UB (WHEEL_MAX)

// In milliseconds, one direction (in, or out)
#define SHORTISH_FADE_TIME 500
#define LONG_FADE_TIME 1000

#define ENCODER_PIN_1 2
#define ENCODER_PIN_2 3
#define ENCODER_SWITCH_PIN 4
#define ENCODER_LED_R_PIN 10
#define ENCODER_LED_G_PIN 11
#define ENCODER_LED_B_PIN 9

CRGB leds[TOTAL_LEDS];
Encoder encoder(ENCODER_PIN_1, ENCODER_PIN_2);

CRGB lastColor = CRGB(0, 0, 0);
CRGB currentColor = CRGB(0, 0, 0);

DEFINE_GRADIENT_PALETTE(deepPurplesAndReds_gp){
    0, 255, 40, 0,
    40, 255, 0, 0,
    255, 40, 0, 255};

DEFINE_GRADIENT_PALETTE(reds_gp){
    0, 255, 0, 0,
    255, 255, 128, 255};

CRGBPalette16 whitePalette = CRGBPalette16(CRGB::White);

CRGBPalette16 currentPalette = CRGBPalette16(CRGB::Black);

CRGBPalette16 palettes[NUM_CHOICES] = {
    RainbowColors_p,
    deepPurplesAndReds_gp,
    reds_gp,
    whitePalette,
};

// RainbowColors_p, RainbowStripeColors_p, OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p

void setup()
{
  FastLED.addLeds<NEOPIXEL, LEDS_PIN>(leds, TOTAL_LEDS);
  pinMode(ENCODER_LED_R_PIN, OUTPUT);
  pinMode(ENCODER_LED_G_PIN, OUTPUT);
  pinMode(ENCODER_LED_B_PIN, OUTPUT);
  pinMode(ENCODER_SWITCH_PIN, INPUT);
  analogWrite(ENCODER_LED_R_PIN, 20);
  analogWrite(ENCODER_LED_G_PIN, 100);
  analogWrite(ENCODER_LED_B_PIN, 160);
}

void loop()
{
  static uint8_t paletteSelection = readPaletteSelection();
  static uint8_t progSelection = readProgSelection();
  static int delayValue = readDelayValue();
  static int switchPressed = digitalRead(ENCODER_SWITCH_PIN);
  static uint8_t wheelValue = readWheelValue();
  static uint8_t scaleValue = readScaleValue();
  static uint16_t progress = 0;
  /*
  String swPressedStr = switchPressed ? "YES" : "NO";
  String progSelStr = String(progSelection, DEC);
  String paletteSelStr = String(paletteSelection, DEC);
  String delayAmtStr = String(delayValue, DEC);
  String wheelValStr = String(wheelValue, DEC);
  String scaleValStr = String(scaleValue, DEC);
  String info = String("[PROG: " + progSelStr +
      ", PALETTE: " + paletteSelStr +
      ", DELAY: " + delayAmtStr +
      ", SW: " + swPressedStr +
      ", WHEEL: " + wheelValStr +
      ", SCALE: " + scaleValStr +
      "]");
      */
  //Serial.println(info);
  if (switchPressed)
  {
    encoder.write(1073741824L);
    long encValue = encoder.read();

    /* The lights turn green, keep holding on until they turn
       turquoise to enter selection mode */
    setEncoderColor(CRGB(0, 255, 0));
    setAll(CRGB(0, 32, 0));
    FastLED.delay(1000);
    switchPressed = digitalRead(ENCODER_SWITCH_PIN);
    switchPressed = digitalRead(ENCODER_SWITCH_PIN);

    long currentGroup = 0;
    if (switchPressed)
    {
      /* Entered Selection Mode */
      setEncoderColor(CRGB(0, 255, 255));
      setAll(CRGB(0, 64, 64));
      FastLED.delay(1000);
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);

      int amountSwitchPressed = 0;

      while (amountSwitchPressed < 4)
      {
        encValue = encoder.read();
        currentGroup = (encValue / 8L) % NUM_GROUPS;
        wheelValue = readWheelValue();
        scaleValue = readScaleValue();
        delayValue = readDelayValue();
        CHSV newColor = CHSV(wheelValue, 255, scaleValue);

        setGroup(currentGroup, newColor);
        setEncoderColor(newColor);

        FastLED.show();

        switchPressed = digitalRead(ENCODER_SWITCH_PIN);
        switchPressed = digitalRead(ENCODER_SWITCH_PIN);
        if (switchPressed)
        {
          amountSwitchPressed += 1;
        }
        FastLED.delay(delayValue);
      }
      setEncoderColor(CRGB(32, 32, 32));
      FastLED.delay(500);
    }
  }
  else
  {
    EVERY_N_MILLISECONDS(200)
    {
      currentPalette = palettes[readPaletteSelection()];
      progSelection = readProgSelection();
      delayValue = readDelayValue();
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      wheelValue = readWheelValue();
      scaleValue = readScaleValue();
      FastLED.setBrightness(scaleValue);
    }
    EVERY_N_MILLISECONDS(10)
    {
      progress += delayValue;
      uint8_t progress8 = progress >> 8;
      switch (progSelection)
      {
      case 0:
        // Random color from palette, custom fade time
        fadeInOut(progress8);
        break;
      case 1:
        // Phased Rainbow
        phasedRainbow(progress8);
        break;
      case 2:
        // Phased Rainbow w/ same color on each
        discretePhasedRainbow(progress8);
        break;
        /*
      case 3:
        // Synchronized Rainbow 60FPS
        phasedRainbowWithSpanInBoundsAtScale(0.0, -1, colorLB, colorUB, scale);
        break;
        */
      case 3:
        // Smooth transition between random color in palette bounds for custom time
        setAll(CRGB(255, 192, 128));
        FastLED.show();
        break;
      default:
        // White at 0.7 scale and wait for 0.5 second
        setAll(CRGB(255, 192, 128));
        FastLED.show();
        break;
      }
    }
  }
}

void fadeInOut(uint8_t progress)
{
  static uint8_t lastProgress = 255;
  if (lastProgress > progress)
  {
    lastColor = currentColor;
    currentColor = randomColorFromPalette();
  }

  uint8_t scaling = quadwave8(progress);
  CRGB currentCopy = CRGB(currentColor);
  nscale8x3(currentCopy.r, currentCopy.g, currentCopy.b, scaling);
  setAll(currentCopy);
  lastProgress = progress;
  FastLED.show();
}

void phasedRainbow(uint8_t progress)
{
  fill_rainbow(leds, TOTAL_LEDS, progress, (VALUE_MAX / TOTAL_LEDS));
  FastLED.show();
}

void discretePhasedRainbow(uint8_t progress)
{
  for (uint8_t i = 0; i < NUM_GROUPS; i += 1)
  {
    setGroup(i, CHSV(progress + (i * GROUP_SPACING), 255, 255));
  }
  FastLED.show();
}

void setEncoderColor(CRGB color)
{
  analogWrite(ENCODER_LED_R_PIN, 255 - color.r);
  analogWrite(ENCODER_LED_G_PIN, 255 - color.g);
  analogWrite(ENCODER_LED_B_PIN, 255 - color.b);
}

int readAnalogSelection(int whichPin)
{
  int rawValue = analogRead(whichPin);
  // Do twice since capactior doesn't have enough time to fill if once (?)
  rawValue = analogRead(whichPin);
  int retVal = (rawValue + HALF_VDIV_DIST) / VDIV_DIST;
  return retVal;
}

int readProgSelection()
{
  return readAnalogSelection(A0);
}

int readPaletteSelection()
{
  return readAnalogSelection(A1);
}

int readDelayValue()
{
  int rawValue = analogRead(A2);
  rawValue = analogRead(A2);
  return rawValue;
}

int readWheelValue()
{
  int rawValue = analogRead(A3);
  rawValue = analogRead(A3);
  return rawValue / ADC2BYTE;
}

int readScaleValue()
{
  int rawValue = analogRead(A4);
  rawValue = analogRead(A4);
  return rawValue / ADC2BYTE;
}

CRGB randomColorFromPalette()
{
  uint8_t randomSelection = random8();
  return ColorFromPalette(currentPalette, randomSelection);
}

/* Return a color within the two bounds, lower inclusive, upper exclusive.
 *
 *  0   : red
 *  43?: green + red = yellow
 *  85? : green
 *  128? : green + blue = cyan
 *  170? : blue
 *  213? : blue + red = purple
 *  256 : red
 *
 *  Accepts negative values, but lower must be less than upper.
 */
CHSV randomWheelInBounds(int lower, int upper)
{
  int randomHue = random16(lower, upper);
  return CHSV(randomHue, 255, 255);
}

/* ease8InOutCubic, ease8InOutApprox could be useful */

/* 0..1 -> 0..1 smooth interpolation function, with zero first and
 *  second derivatives.
 *
 * My first foray into fixed point arithmetic, there are probably many things
 * wrong with this heh.
 *
 * So I compiled a test vs. a naive floating point implementation of this,
 * and it looks like it just doesn't even work for the first 40 or so indices.
 *
 * Gonna table this one for later despite how fun it seems.
 */
// uint8_t smootherstep(uint8_t x)
// {
//   uint16_t sixX2_4_12 = 6 * (((uint16_t)x >> 2) * ((uint16_t)x >> 2));
//   uint16_t ten4_12 = 10 << 12;
//   uint16_t positive_middle = sixX2_4_12 + ten4_12;
//   uint16_t negative_middle = (15 * (uint16_t)x) << 4;
//   uint16_t middle = positive_middle - negative_middle;
//   uint16_t x3_0_16 = ((uint16_t)x >> 3) * ((uint16_t)x >> 3) * ((uint16_t)x >> 2);
//   uint16_t result = (x3_0_16 >> 8) * (middle >> 8);
//   return result >> 4;
// }

/* Transitions smoothly from LAST_COLOR global to COLOR, taking TOTAL_TIME
 *  milliseconds, using a smoothstep interpolation function.
 */
/*
void smoothTransitionFromLastColorToColor(CRGB color, int totalTime);
{

  {
    CRGB currentColor = blend(lastColor, color, ease8InOutCubic()
  }
  lastColor = color;
}

*/
/* Set everything to a color. */
void setAll(CRGB color)
{
  fill_solid(leds, TOTAL_LEDS, color);
}

void setGroup(uint8_t which, CRGB color)
{
  uint8_t startPos = which * GROUP_SIZE;
  fill_solid(&(leds[startPos]), GROUP_SIZE, color);
}
