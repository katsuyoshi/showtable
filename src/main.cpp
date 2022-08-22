
// NeoPixelFunFadeInOut
// This example will randomly pick a color and fade all pixels to that color, then
// it will fade them to black and restart over
// 
// This example demonstrates the use of a single animation channel to animate all
// the pixels at once.
//
#include <M5Unified.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "ServoEasing.hpp"


#define PIXEL_PIN           2
#define SERVO_PIN           5

ServoEasing servo;

bool running = false;
bool use_led = true;
int speed = 1;

float deg = 83;



const uint16_t PixelCount = 24; // make sure to set this to the number of pixels in your strip
//const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
const uint8_t AnimationChannels = 1; // we only need one as all the pixels are animated at once

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PIXEL_PIN);
// For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.  
// There are other Esp8266 alternative methods that provide more pin options, but also have
// other side effects.
// for details see wiki linked here https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods 

NeoPixelAnimator animations(AnimationChannels); // NeoPixel animation management object

boolean fadeToColor = true;  // general purpose variable used to store effect state


// what is stored for state is specific to the need, in this case, the colors.
// basically what ever you need inside the animation update function
struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

// one entry per pixel to match the animation timing manager
MyAnimationState animationState[AnimationChannels];

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

// simple blend function
void BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        if (running && use_led) {
            strip.SetPixelColor(pixel, updatedColor);
        } else {
            strip.SetPixelColor(pixel, BLACK);
        }
    }
}

void FadeInFadeOutRinseRepeat(float luminance)
{
    if (fadeToColor)
    {
        // Fade upto a random color
        // we use HslColor object as it allows us to easily pick a hue
        // with the same saturation and luminance so the colors picked
        // will have similiar overall brightness
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
        uint16_t time = random(800, 2000);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }
    else 
    {
        // fade to black
        uint16_t time = random(600, 700);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = RgbColor(0);

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }

    // toggle to the next effect state
    fadeToColor = !fadeToColor;
}

void task_table(void* arg) {
  int n = 0;
  while(true) {
    if (running) {
      servo.easeTo(deg);
      delay(30);
    }
    servo.easeTo(90.0f);
    delay((speed + 1) * 100);
  }
}

void displayInfo()
{
  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%s\n", running ? "RUN" : "STOP");
  M5.Lcd.printf("     LED: %s\n", use_led ? "ON" : "OFF");
  M5.Lcd.printf("INTERVAL: %d\n", speed + 1);
}

void setup()
{
  M5.begin();
  M5.Lcd.setTextFont(4);

  servo.attach(SERVO_PIN, 0, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE);
  servo.setEasingType(EASE_LINEAR);
  setSpeedForAllServos(60);

    strip.Begin();
    strip.Show();

    SetRandomSeed();

  xTaskCreatePinnedToCore(task_table, "table", 1024, NULL, 1, NULL, 1);

  displayInfo();
}

void loop()
{
    if (animations.IsAnimating())
    {
        // the normal loop just needs these two to run the active animations
        animations.UpdateAnimations();
        strip.Show();
    }
    else
    {
        // no animation runnning, start some 
        //
        FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
    }


    M5.update();
    if (M5.BtnA.wasPressed()) {
      running = !running;
    }
    if (M5.BtnB.wasPressed()) {
      use_led = !use_led;
    }
    if (M5.BtnC.wasPressed()) {
      speed = (speed + 1) % 30;
    }

    bool changed = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed();
    if (changed) {
      displayInfo();
    }
}
