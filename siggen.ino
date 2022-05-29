
// Include

// ads
#include "AD9833.h"
// lcd
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header

void lcd_setup(void);
void lcd_loop();

byte ads_get_wave(void);
void ads_toggle_wave(void);
byte ads_get_scale();
void ads_toggle_scale(void);
void ads_set_delta(long d);
void ads_clear_delta();
long ads_get_frequency();
void ads_setup(void);
void ads_loop(void);

void encoder_setup(void);
void encoder_loop(void);

void buttons_setup(void);
void buttons_loop(void);

const int F_HZ = 0;
const int F_KHZ = 1;
const int F_MHZ = 2;

const int W_SIN = 0;
const int W_SQR = 1;
const int W_TRI = 2;

void setup()
{

    lcd_setup();
    encoder_setup();
    ads_setup();
    buttons_setup();

    Serial.begin(9600); // output
    Serial.println("Signal Gen 1.00");
}

void loop()
{
    lcd_loop();
    encoder_loop();
    ads_loop();
    buttons_loop();
}

// TODO: Move these to modules but lets get these working first

///////////////////////////////////////////////////////////////////////////////
// LCD
///////////////////////////////////////////////////////////////////////////////

const int rs = 8, en = 10, db4 = 4, db5 = 5, db6 = 6, db7 = 7;
const int LCD_COLS = 8;
const int LCD_ROWS = 2;
hd44780_pinIO lcd(rs, en, db4, db5, db6, db7);

char last_reported_f[14];

void lcd_setup(void)
{
    memset(last_reported_f, '\0', 14);
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.lineWrap();
    lcd.print("Signal Gen 1.00");
}

void lcd_loop()
{
    long f = ads_get_frequency();

    // nothing
    char frequencyText[14];
    memset(frequencyText, '\0', 14);
    dtostrf(f, 9, 0, frequencyText);

    byte scale = ads_get_scale();
    char scale_letter;

    switch (scale)
    {
    case F_HZ:
        scale_letter = ' ';
        break;

    case F_KHZ:
        scale_letter = 'K';
        break;

    case F_MHZ:
        scale_letter = 'M';
        break;

    default:
        scale_letter = '?'; // should never happen, just in case...
        break;
    }

    char wave_letter;
    byte wave = ads_get_wave();
    switch (wave)
    {
    case W_SIN:
        wave_letter = 'S';
        break;

    case W_SQR:
        wave_letter = 'Q';
        break;

    case W_TRI:
        wave_letter = 'T';
        break;

    default:
        wave_letter = '?'; // should never happen, just in case...
        break;
    }

    char lcdText[20];
    sprintf(lcdText, "%s %c%c", frequencyText, scale_letter, wave_letter);

    if (strcmp(last_reported_f, lcdText) == 0)
    {
        return;
    }

    char buffer[40];
    sprintf(buffer, "New frequency %s", lcdText);
    Serial.println(buffer);

    lcd.clear();
    lcd.home();
    lcd.print(lcdText);

    strcpy(last_reported_f, lcdText);
}

///////////////////////////////////////////////////////////////////////////////
// ADS Chip
///////////////////////////////////////////////////////////////////////////////

volatile long currentFrequency = 50000;
volatile byte currentScale = F_HZ;
volatile byte currentWave = W_SIN;
volatile long delta = 0;
bool delta_lock = 0;

#define FNC_PIN 9
AD9833 gen(FNC_PIN);

void ads_toggle_wave(void)
{
    currentWave++;

    if (currentWave > W_TRI)
    {
        currentWave = W_SIN;
    }
}

byte ads_get_scale(void)
{
    return currentScale;
}

byte ads_get_wave(void)
{
    return currentWave;
}

void ads_toggle_scale(void)
{
    currentScale++;
    if (currentScale > F_MHZ)
    {
        currentScale = F_HZ;
    }
}

void ads_set_delta(long d)
{
    delta = d;
    delta_lock = 1;
}

long ads_get_frequency(void)
{
    return currentFrequency;
}

void ads_clear_delta(void)
{
    delta = 0;
    delta_lock = 0;
}

bool can_set_delta(void)
{
    return !delta_lock;
}

void ads_setup(void)
{
    gen.Begin();
    gen.ApplySignal(SINE_WAVE, REG0, (float)currentFrequency);
    gen.EnableOutput(true);
}

void ads_loop(void)
{
    // detect if we have to change the frequency

    if (delta != 0)
    {
        long scale;

        switch (currentScale)
        {
        case F_HZ:
            scale = 1;
            break;
        case F_KHZ:
            scale = 1000;
            break;
        case F_MHZ:
            scale = 1000000;
            break;
        default:
            break;
        }

        currentFrequency += delta * scale;
        ads_clear_delta();
        gen.SetFrequency(REG0, (float)currentFrequency);
    }

    WaveformType w = gen.GetWaveForm(REG0);

    WaveformType ours;

    switch (currentWave)
    {
    case W_SIN:
        ours = WaveformType::SINE_WAVE;
        break;
    case W_SQR:
        ours = WaveformType::SQUARE_WAVE;
        break;
    case W_TRI:
        ours = WaveformType::TRIANGLE_WAVE;
        break;
    default:
        ours = WaveformType::SINE_WAVE; // sines are just more perfect :)
        break;
    }

    if (ours != w)
    {
        gen.SetWaveform(REG0, ours);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Rotary Encoder
///////////////////////////////////////////////////////////////////////////////

const int encoderPinA = 2; // right
const int encoderPinB = 3; // left

volatile long encoderPos = 0;
long lastReportedPos = 1;
static boolean rotating = false;

// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;

void encoder_setup(void)
{
    pinMode(encoderPinA, INPUT);
    pinMode(encoderPinB, INPUT);

    digitalWrite(encoderPinA, HIGH);
    digitalWrite(encoderPinB, HIGH);

    attachInterrupt(0, doEncoderA, CHANGE);
    attachInterrupt(1, doEncoderB, CHANGE);
}

void encoder_loop(void)
{
    rotating = true; // reset the debouncer

    if (lastReportedPos != encoderPos)
    {
        // push the delta into the frequency
        if (can_set_delta())
        {
            long dif = encoderPos - lastReportedPos;
            ads_set_delta(dif);
            Serial.println(dif);
            lastReportedPos = encoderPos;
        }
    }
}

// Interrupt on A changing state
void doEncoderA()
{
    // debounce
    if (rotating)
        delay(1); // wait a little until the bouncing is done

    // Test transition, did things really change?
    if (digitalRead(encoderPinA) != A_set)
    { // debounce once more
        A_set = !A_set;

        // adjust counter + if A leads B
        if (A_set && !B_set)
            encoderPos += 1;

        rotating = false; // no more debouncing until loop() hits again
    }
}

// Interrupt on B changing state, same as A above
void doEncoderB()
{
    if (rotating)
        delay(1);
    if (digitalRead(encoderPinB) != B_set)
    {
        B_set = !B_set;
        //  adjust counter - 1 if B leads A
        if (B_set && !A_set)
            encoderPos -= 1;

        rotating = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Buttons
///////////////////////////////////////////////////////////////////////////////

const int freqUnitPin = A0;
int freqUnitPinState = LOW;

const int waveShapePin = A1;
int waveShapeState = LOW;

const int atnPin = A2;
int atnPinState = LOW;

int buttonState = 0; // current state of the button

void buttons_setup(void)
{
    pinMode(freqUnitPin, INPUT);
    pinMode(waveShapePin, INPUT);
    pinMode(atnPin, INPUT);
}

void buttons_loop(void)
{
    buttonState = digitalRead(freqUnitPin);

    if (buttonState != freqUnitPinState)
    {
        if (buttonState == HIGH)
        {
            ads_toggle_scale();
        }

        freqUnitPinState = buttonState;
    }

    buttonState = digitalRead(waveShapePin);
    if (buttonState != waveShapeState)
    {
        if (buttonState == HIGH)
        {
            ads_toggle_wave();
        }

        waveShapeState = buttonState;
    }
}
