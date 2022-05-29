// Include

// ads
#include "AD9833.h"
// lcd
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header

#define APP_NAME "Signal Gen"
#define APP_VERSION "1.00"

#define VERBOSE 0

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

const int F_x1HZ = 0;
const int F_x10HZ = 1;
const int F_x100HZ = 2;
const int F_x1KHZ = 3;
const int F_x10KHZ = 4;
const int F_x100KHZ = 5;
const int F_x1MHZ = 6;

const int W_SIN = 0;
const int W_SQR = 1;
const int W_TRI = 2;

void setup()
{

    lcd_setup();
    encoder_setup();
    ads_setup();
    buttons_setup();

    Serial.begin(115200); // output
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
    memset(last_reported_f, '\0', sizeof(last_reported_f));
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.lineWrap();
    lcd.print("Signal Gen 1.00");
}

void lcd_loop()
{
    long f = ads_get_frequency();

    // nothing
    char frequencyText[20];
    memset(frequencyText, '\0', sizeof(frequencyText));
    dtostrf(f, 7, 0, frequencyText);

    byte scale = ads_get_scale();

    char scale_text[10];
    memset(scale_text, '\0', sizeof(scale_text));

    switch (scale)
    {
    case F_x1HZ:
        strncpy(scale_text, "      ", sizeof(scale_text));
        break;

    case F_x10HZ:
        strncpy(scale_text, "   x10", sizeof(scale_text));
        break;

    case F_x100HZ:
        strncpy(scale_text, "  x100", sizeof(scale_text));
        break;

    case F_x1KHZ:
        strncpy(scale_text, "   Khz", sizeof(scale_text));
        break;

    case F_x10KHZ:
        strncpy(scale_text, "  x10K", sizeof(scale_text));
        break;

    case F_x100KHZ:
        strncpy(scale_text, " x100K", sizeof(scale_text));
        break;

    case F_x1MHZ:
        strncpy(scale_text, "   Mhz", sizeof(scale_text));
        break;

    default:
        strncpy(scale_text, "     ?", sizeof(scale_text)); // should never happen, just in case...
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

    char lcdText[40];
    memset(lcdText, '\0', sizeof(lcdText));
    sprintf(lcdText, "%s %s", frequencyText, scale_text);

    if (strncmp(last_reported_f, lcdText, sizeof(last_reported_f)) == 0)
    {
        return;
    }    
    
    lcd.clear();
    lcd.home();
    lcd.print(lcdText);

    memset(last_reported_f, '\0', sizeof(last_reported_f));
    strncpy(last_reported_f, lcdText, sizeof(last_reported_f));    
}

///////////////////////////////////////////////////////////////////////////////
// ADS Chip
///////////////////////////////////////////////////////////////////////////////

volatile long currentFrequency = 50000;
volatile byte currentScale = F_x1HZ;
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
    if (currentScale > F_x1MHZ)
    {
        currentScale = F_x1HZ;
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
        case F_x1HZ:
            scale = 1;
            break;

        case F_x10HZ:
            scale = 10;
            break;

        case F_x100HZ:
            scale = 100;
            break;

        case F_x1KHZ:
            scale = 1000;
            break;

        case F_x10KHZ:
            scale = 10000;
            break;

        case F_x100KHZ:
            scale = 100000;
            break;

        case F_x1MHZ:
            scale = 1000000;
            break;

        default:
            break;
        }

        if(VERBOSE)
        {
            Serial.println("Adjusting frequency");
            Serial.print("Previous frequency: ");
            Serial.println(currentFrequency);
            Serial.print("Delta: ");
            Serial.println(delta);
        }

        currentFrequency += delta * scale;

        if(VERBOSE)
        {            
            Serial.print("New frequency: ");
            Serial.println(currentFrequency);
        }

        ads_clear_delta();
        float converted_freq = (float)currentFrequency;

        if(VERBOSE)
        {            
            Serial.print("New frequency (float): ");
            Serial.println(converted_freq);
        }

        gen.SetFrequency(REG0, converted_freq);
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

volatile long encoderPos = 1;
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
            if(VERBOSE)
            {
                Serial.print("Detected encoder change: ");
                Serial.println(dif);
            }
            
            lastReportedPos = encoderPos;
        }
        else if(VERBOSE)
        {
            Serial.print("Waiting to set delta");                
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
        {
            encoderPos += 1;
        }

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
        {
            encoderPos -= 1;
        }

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
