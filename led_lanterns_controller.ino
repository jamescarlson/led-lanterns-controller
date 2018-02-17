#include <Adafruit_NeoPixel.h>
#include <Encoder.h>

/* To get a perceptually linear (actually a power law or near exponential
 *  in reality) brightness for one channel, index into this array. May
 *  want to skip the zeros at the start.
 */
const uint8_t PROGMEM gamma[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

/* Notable list of colors (input to Wheel function):
 *
 * Lantern glow yellow: 32-48
 */

// Output to NeoPixels
#define PIN 5

// Each group is any set of LEDs you want to control as a single unit
// In my case, I have 5 each of 16-NeoPixel rings
#define NUM_GROUPS 5
#define GROUP_SIZE 16
#define TOTAL_PIXELS (NUM_GROUPS * GROUP_SIZE)

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

/* Inclusive lower bound for the input to the Wheel function,
 *  this function can take numbers lower than this, or higher
 *  than the upper bound, but it will convert those via the
 *  modulo operator to be in this range.
 *
 *  This represents red, on the orange side of the spectrum.
 */
#define WHEEL_MIN 0
// Exclusive upper bound (red on the purple side of spectrum)
#define WHEEL_MAX 768

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

Encoder encoder(ENCODER_PIN_1, ENCODER_PIN_2);

typedef uint32_t color_t;
color_t last_color = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(TOTAL_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
  pinMode(ENCODER_LED_R_PIN, OUTPUT);
  pinMode(ENCODER_LED_G_PIN, OUTPUT);
  pinMode(ENCODER_LED_B_PIN, OUTPUT);
  pinMode(ENCODER_SWITCH_PIN, INPUT);
  analogWrite(ENCODER_LED_R_PIN, 20);
  analogWrite(ENCODER_LED_G_PIN, 100);
  analogWrite(ENCODER_LED_B_PIN, 160);
  //Serial.begin(9600);
  /*
  String vdivDist = String(VDIV_DIST, DEC);
  String halfVdivDist = String(HALF_VDIV_DIST, DEC);
  String startStr = String("Starting..., [VDIV_DIST, HALF_VDIV_DIST] == [" + vdivDist + ", " + halfVdivDist + "]");
  //Serial.println(startStr);
  */
}

/*
struct lanternProgram {
  // Does the program take scale, lower, and upper bounds?
  // Some programs need to keep state over multiple runs
  // this is the case for the fade from color to color program
  // #4.

  // All programs should be able to take in a scale, colorLB, and colorUB


  bool a;

};
*/
typedef struct lanternProgram lanternProgram;

void loop()
{

  int colorLB = DEFAULT_COLOR_LB;
  int colorUB = DEFAULT_COLOR_UB;
  float scale = 1.0;

  /* This could be refactored as a 3d array or an array of structs,
   *  taking the value from the read function and bounds checking it,
   *  then setting any valid values to the appropriate variables.
   *
   *  Doing it that way would be more extensible if you decide to add
   *  more voltage divider segments.
   */
  int paletteSelection = readPaletteSelection();
  switch (paletteSelection)
  {
  case 0:
    // Warm tones, 0.3 scale
    colorUB = 45;
    scale = 0.3;
    break;
  case 1:
    // Deep purples and reds, full scale
    colorLB = -224;
    colorUB = 16;
    break;
  case 2:
    // Warm tones, full scale
    colorUB = 45;
    break;
  case 3:
    // Anything at full scale, already set.

    break;
  default:
    // Anything at full scale, if we can't get a valid value as input
    break;
  }

  color_t new_color;
  int progSelection = readProgSelection();
  int delayAmount = readDelayValue();
  int switchPressed = digitalRead(ENCODER_SWITCH_PIN);
  switchPressed = digitalRead(ENCODER_SWITCH_PIN);
  int wheelValue = (readWheelValue() * 3) / 4; //0..1023 -> 0..767
  int scaleValue = readScaleValue() / 4;
  /*
  String swPressedStr = switchPressed ? "YES" : "NO";
  String progSelStr = String(progSelection, DEC);
  String paletteSelStr = String(paletteSelection, DEC);
  String delayAmtStr = String(delayAmount, DEC);
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
    setEncoderColor(strip.Color(0, 255, 0));
    setAllFast(strip.Color(32, 32, 32));
    delay(1000);
    switchPressed = digitalRead(ENCODER_SWITCH_PIN);
    switchPressed = digitalRead(ENCODER_SWITCH_PIN);
    long currentGroup = 0;
    if (switchPressed)
    {
      setAllFast(strip.Color(32, 64, 32));
      delay(1000);
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      while (!switchPressed)
      {
        delay(delayAmount);
        encValue = encoder.read();
        currentGroup = (encValue / 4L) % NUM_GROUPS;
        wheelValue = (readWheelValue() * 3) / 4;
        scaleValue = readScaleValue() / 4;
        delayAmount = readDelayValue();
        new_color = colorScaleFast(Wheel(wheelValue), scaleValue);
        setGroup(currentGroup, new_color);
        setEncoderColor(new_color);
        strip.show();
        switchPressed = digitalRead(ENCODER_SWITCH_PIN);
        switchPressed = digitalRead(ENCODER_SWITCH_PIN);
      }
    }
  }
  else
  {
    switch (progSelection)
    {
    case 0:
      // Random color from palette, short-ish fade in/out
      randomFadeInOutLinearInBoundsAtScale(SHORTISH_FADE_TIME, SHORTISH_FADE_TIME, colorLB, colorUB, scale);
      break;
    case 1:
      // Random color from palette, custom fade time
      randomFadeInOutLinearInBoundsFast(delayAmount, delayAmount, colorLB, colorUB);
      break;
    case 2:
      // Phased Rainbow 60FPS
      phasedRainbowWithSpanInBoundsAtScale(1.0, -1, colorLB, colorUB, scale);
      break;
      /*
      case 3:
        // Synchronized Rainbow 60FPS
        phasedRainbowWithSpanInBoundsAtScale(0.0, -1, colorLB, colorUB, scale);
        break;
        */
    case 3:
      // Smooth transition between random color in palette bounds for custom time
      new_color = randomWheelInBounds(colorLB, colorUB);
      smoothTransitionFromLastColorToColor(new_color, delayAmount, 60, &smootherstep);
      break;
    default:
      // White at 0.7 scale and wait for 0.5 second
      setAllFast(strip.Color(255, 255, 255));
      strip.show();
      delay(delayAmount);
      break;
    }
  }
}

void setEncoderColor(color_t color)
{
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  getColors(color, &r, &g, &b);
  analogWrite(ENCODER_LED_R_PIN, 255 - r);
  analogWrite(ENCODER_LED_G_PIN, 255 - g);
  analogWrite(ENCODER_LED_B_PIN, 255 - b);
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
  return rawValue;
}

int readScaleValue()
{
  int rawValue = analogRead(A4);
  rawValue = analogRead(A4);
  return rawValue;
}

/* Return a color within the two bounds, lower inclusive, upper exclusive.
 *
 *  0   : red
 *  128 : green + red = yellow
 *  256 : green
 *  384 : green + blue = cyan
 *  512 : blue
 *  640 : blue + red = purple
 *  768 : red
 *
 *  Accepts negative values, but lower must be less than upper.
 */
color_t randomWheelInBounds(int lower, int upper)
{
  int random_color_index = random(lower, upper);
  return Wheel(random_color_index);
}

/* scale is out of 255 */
color_t colorScaleFast(color_t color, uint16_t scale)
{
  uint16_t r = color >> 16;
  uint16_t g = (color >> 8) & 255;
  uint16_t b = color & 255;

  return strip.Color(
      (uint8_t)((r * scale) / 255),
      (uint8_t)((g * scale) / 255),
      (uint8_t)((b * scale) / 255));
}

color_t colorScale(color_t color, float scale)
{
  uint8_t r = color >> 16;
  uint8_t g = (color >> 8) & 255;
  uint8_t b = color & 255;
  r = r * scale;
  g = g * scale;
  b = b * scale;
  return strip.Color(r, g, b);
}

/* Populates values in r, g, b, with 0-255 integer values
 *  using a color_t as an input.
 */
void getColors(color_t color, uint8_t *r, uint8_t *g, uint8_t *b)
{
  *r = color >> 16;
  *g = (color >> 8) & 255;
  *b = color & 255;
}

/* Populates values in r, g, b, with 0.0-255.0 floating point values
 *  using a color_t as an input.
 */
void getFloatColors(color_t color, float *r, float *g, float *b)
{
  getNormalizedToColors(color, r, g, b, 255.0);
}

/* Populates values in r, g, b, with floats in range of 0.0-NORM_MAX
 *  using a color_t as an input.
 */
void getNormalizedToColors(color_t color,
                           float *r,
                           float *g,
                           float *b,
                           float norm_max)
{
  float denom = 255.0 / norm_max;
  uint8_t int_r = color >> 16;
  *r = (float)int_r / denom;

  uint8_t int_g = (color >> 8) & 255;
  *g = (float)int_g / denom;

  uint8_t int_b = color & 255;
  *b = (float)int_b / denom;
}

/* 0..1 -> 0..1 smooth interpolation function, with zero first and
 *  second derivatives.
 *  Thanks for the slick implemenation, Wikipedia!:
 *    https://en.wikipedia.org/wiki/Smoothstep
 */
float smootherstep(float x)
{
  if (x <= 0)
  {
    return 0;
  }
  else if (x >= 1)
  {
    return 1;
  }
  else
  {
    return x * x * x * (x * (x * 6 - 15) + 10);
  }
}

/* Transitions smoothly from LAST_COLOR global to COLOR, taking TOTAL_TIME
 *  milliseconds, using the supplied interpolation function (takes a float
 *  from 0..1 and returns a float from 0..1) at FPS frames per second
 *  (or slightly faster, to make frame interval at the millisecond boundary).
 */
void smoothTransitionFromLastColorToColor(color_t color,
                                          int total_time,
                                          int fps,
                                          float (*interp_function)(float))
{
  float last_r;
  float last_g;
  float last_b;
  float r;
  float g;
  float b;
  getFloatColors(last_color, &last_r, &last_g, &last_b);
  getFloatColors(color, &r, &g, &b);

  long num_frames = ((float)total_time / 1000.0) * (long)fps;
  int wait_time_ms = 1000 / fps;
  for (long i = 0; i < num_frames; i += 1)
  {
    float interp_val = (*interp_function)((float)i / (float)num_frames);
    float interp_r = (1.0 - interp_val) * last_r + (interp_val * r);
    float interp_g = (1.0 - interp_val) * last_g + (interp_val * g);
    float interp_b = (1.0 - interp_val) * last_b + (interp_val * b);
    color_t frame_color = strip.Color((int)interp_r,
                                      (int)interp_g,
                                      (int)interp_b);
    setAll(frame_color);
    strip.show();
    delay(wait_time_ms);
  }
  last_color = color;
}

/* Fade in and out of any random color at full brightness, taking
 *  INTIME milliseconds to fade in, and OUTTIME to fade out.
 */
void randomFadeInOutLinear(int inTime, int outTime)
{
  randomFadeInOutLinearInBoundsAtScale(inTime, outTime, DEFAULT_COLOR_LB, DEFAULT_COLOR_UB, 1.0);
}

/* Fade an and out of a random color within palette bounds COLORLB
 *  and COLORUB at brightness SCALE (0.0...1.0), taking INTIME
 *  milliseconds to fade in, and OUTTIME milliseconds to fade out.
 */
void randomFadeInOutLinearInBoundsAtScale(int inTime, int outTime, int colorLB, int colorUB, float scale)
{
  color_t color = randomWheelInBounds(colorLB, colorUB);
  last_color = color;

  //fade in
  int iterations = (int)(255 * scale);
  int inStep = inTime / iterations;
  int outStep = outTime / iterations;
  int minStart = 0;
  int minEnd = 0;
  for (int i = minStart; i < iterations; i += 1)
  {
    setAll(colorScale(color, i / 255.0));
    strip.show();
    delay(inStep);
  }
  for (int i = iterations; i >= minEnd; i -= 1)
  {
    setAll(colorScale(color, i / 255.0));
    strip.show();
    delay(outStep);
  }
}

void randomFadeInOutLinearInBoundsFast(int inTime, int outTime, int colorLB, int colorUB)
{
  color_t color = randomWheelInBounds(colorLB, colorUB);
  setAllFast(color);
  delay(inTime);
}

/* Pick a random color, set a random group to it, wait WAIT
 *  milliseconds, and repeat for HOWMANY times.
 */
void randomUpdate(int wait, int howMany)
{
  for (int i = 0; i < howMany; i += 1)
  {
    last_color = Wheel((int)random(768));
    setGroup((int)random(NUM_GROUPS), last_color);
    strip.show();
    delay(wait);
  }
}

/* Gamma corrected color wheel function, might make color transitions
 *  look more uniform through the entire wheel range of 0..767.
 */
color_t WheelLog(int WheelPos)
{
  while (WheelPos < 0)
  {
    WheelPos += 768;
  }
  WheelPos = 767 - (WheelPos % 768);
  if (WheelPos < 256)
  {
    return logColor(255 - WheelPos, 0, WheelPos);
  }
  else if (WheelPos < 512)
  {
    WheelPos -= 256;
    return logColor(0, WheelPos, 255 - WheelPos);
  }
  else
  {
    WheelPos -= 512;
    return logColor(WheelPos, 255 - WheelPos, 0);
  }
}

/* Gamma correct an input COLOR. */
color_t logColorPacked(color_t color)
{
  uint8_t r = color >> 16;
  uint8_t g = (color >> 8) & 255;
  uint8_t b = color & 255;
  return strip.Color(
      pgm_read_byte(&gamma[r]),
      pgm_read_byte(&gamma[g]),
      pgm_read_byte(&gamma[b]));
}

/* Gamma correct a given R, G, B value and output as a color_t. */
color_t logColor(int r, int g, int b)
{
  return strip.Color(
      pgm_read_byte(&gamma[r]),
      pgm_read_byte(&gamma[g]),
      pgm_read_byte(&gamma[b]));
}

/* Select a color of the rainbow using the color wheel, with input
 *  WHEELPOS from 0 to 767. 0 is Red, 256 Green, 512 Blue.
 */
color_t Wheel(int WheelPos)
{
  while (WheelPos < 0)
  {
    WheelPos += 768;
  }
  WheelPos = 767 - (WheelPos % 768);
  if (WheelPos < 256)
  {
    return strip.Color(255 - WheelPos, 0, WheelPos);
  }
  else if (WheelPos < 512)
  {
    WheelPos -= 256;
    return strip.Color(0, WheelPos, 255 - WheelPos);
  }
  else
  {
    WheelPos -= 512;
    return strip.Color(WheelPos, 255 - WheelPos, 0);
  }
}

/* "Hopscotch" the FOREGROUND color, represented as a wheel position,
 *  along groups, using BACKGROUND color (wheel position), and wait
 *  WAIT milliseconds between transitions.
 */
void hopscotch(int background, int foreground, int wait)
{
  color_t last_color = Wheel(background);
  for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
  {
    setGroup(lantern, Wheel(background));
  }
  strip.show();
  for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
  {
    setGroup(lantern, Wheel(foreground));
    strip.show();
    delay(wait);
    setGroup(lantern, Wheel(background));
  }
}

/* Starting with all lanterns set to BACKGROUND wheel position color (if
 *  doClear is set), turn each, starting from the first, to FOREGROUND
 *  wheel position color, waiting WAIT milliseconds each time. Then "roll
 *  back" the foreground coloring to the background color.
 */
void colorBounce(int background, int foreground, int wait, boolean doClear)
{
  last_color = Wheel(background);
  if (doClear)
  {
    for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
    {
      setGroup(lantern, Wheel(background));
    }
  }
  strip.show();
  for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
  {
    setGroup(lantern, Wheel(foreground));
    strip.show();
    delay(wait);
  }
  for (int lantern = NUM_GROUPS - 1; lantern >= 0; lantern -= 1)
  {
    setGroup(lantern, Wheel(background));
    strip.show();
    delay(wait);
  }
}

/* Starting with the first, set each group to red, waiting WAIT milliseconds
 *  between each, then repeat for yellow, green, turquoise, blue, and purple.
 */
void colorWipe6(int wait)
{
  for (int of6 = 0; of6 < 768; of6 += 128)
  {
    for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
    {
      setGroup(lantern, Wheel(of6));
      strip.show();
      delay(wait);
    }
  }
}

/* Generate an integer triangle wave with slope 1, -1, with lower bound LB
 * (inclusive) and upper bound UB (exclusive). LB must be less than UB.
 *
 * Cycle length = 2 * (UB - LB) - 2
 *
 * For example for LB = 0, UB = 256, the cycle length is 510.
 *
 */
int triangleInBounds(unsigned long x, int lb, int ub)
{
  int halfCycleLength = ub - lb - 1;

  int fullCycleLength = 2 * halfCycleLength;
  int whereInCycle = (int)(x % (unsigned long)fullCycleLength);
  // Ends up being 1 for "in second half" due to rounding
  int inSecondHalf = whereInCycle / halfCycleLength;
  if (inSecondHalf == 0)
  {
    return whereInCycle + lb;
  }
  else
  {
    int howFarDown = whereInCycle - halfCycleLength;
    return ub - howFarDown - 1;
  }
}

/* Generate an integer sawtooth wave with a slope of 1, with
 *  lower bound LB (inclusive) and upper bound UB (exclusive).
 *  LB must be less than UB.
 *
 *  Cycle Length = UB - LB
 */
int sawInBounds(unsigned long x, int lb, int ub)
{
  int fullCycleLength = ub - lb;
  int whereInCycle = x % fullCycleLength;
  return whereInCycle + lb;
}

/* Create a rainbow within color bounds that "moves" colors along each lantern, with total
 *  light output scaled by SCALE.
 *
 *  The SPAN parameter determines how closely spaced the lanterns are in the moving rainbow.
 *  1.0 corresponds to even spacing. (exclusive of the end, so with 5 lanterns and a rainbow
 *  that spans, for example, 500, each lantern would start at 0, 100, 200, 300, and 400.)
 *
 *  Values close to zero make all the lanterns close to the same color as they travel
 *  through the specified color range. Values greater than 1 cause the rainbow to repeat.
 *  Aliasing will occur for values greater than the number of lanterns divided by 2, for
 *  partial rainbows, and values greater than the number of lanterns for full color rainbows.
 */
void phasedRainbowWithSpanInBoundsAtScale(float span, int delayMs, int colorLB, int colorUB, float scale)
{
  boolean pollDelay = false;
  if (delayMs == -1)
  {
    pollDelay = true;
  }
  int (*transF)(unsigned long, int, int);
  if (span <= 0.2)
  {
    last_color = Wheel(colorLB + ((colorUB - colorLB) * span / 2.0));
  }

  int cycle_length;
  if (colorLB != DEFAULT_COLOR_LB || colorUB != DEFAULT_COLOR_UB)
  {
    // Use "triangle" wave of color wheel instead of "sawtooth" that is acutally continuous
    transF = &triangleInBounds;
    cycle_length = (colorUB - colorLB - 1) * 2;
  }
  else
  {
    transF = &sawInBounds;
    cycle_length = (colorUB - colorLB);
  }

  float diff = span * ((float)cycle_length / (float)NUM_GROUPS);

  for (unsigned long x = 0; x < cycle_length; x += 1)
  {
    for (int lantern = 0; lantern < NUM_GROUPS; lantern += 1)
    {
      color_t color = Wheel((*transF)((x + (unsigned long)((float)lantern * diff)), colorLB, colorUB));
      color = colorScale(color, scale);
      setGroup(lantern, color);
    }
    strip.show();
    if (pollDelay)
    {
      delayMs = readDelayValue() / 64;
    }
    delay(delayMs);
  }
}

/* Convenience version of above function */
void phasedRainbow(float span, int fps)
{
  phasedRainbowWithSpanInBoundsAtScale(span, fps, DEFAULT_COLOR_LB, DEFAULT_COLOR_UB, 1.0);
}

/* Set all by iterating over the entire thing unrolled. */
void setAllFast(color_t color)
{
  for (int i = 0; i < TOTAL_PIXELS; i += 1)
  {
    strip.setPixelColor(i, color);
  }
}

/* Set everything to a color. */
void setAll(color_t color)
{
  for (int i = 0; i < 5; i += 1)
  {
    setGroup(i, color);
  }
}

void setGroup(int which, color_t color)
{
  for (int i = GROUP_SIZE * which; i < (GROUP_SIZE * (which + 1)); i += 1)
  {
    strip.setPixelColor(i, color);
  }
}
