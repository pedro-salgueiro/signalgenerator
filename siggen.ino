
// Include

// ads
#include "AD9833.h" 
// lcd
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header

void lcd_setup(void);
void lcd_loop();

void ads_setup(void);
void ads_loop(void);

void encoder_setup(void);
void encoder_loop(void);

const int rs = 8, en = 10, db4 = 4, db5 = 5, db6 = 6, db7 = 7;
hd44780_pinIO lcd(rs, en, db4, db5, db6, db7);
const int LCD_COLS = 8;
const int LCD_ROWS = 2;

const int FNC_PIN = 9;
AD9833 gen(FNC_PIN);


const int encoderPinA = 2; // right
const int encoderPinB = 3; // left

volatile unsigned int encoderPos = 0; // a counter for the dial
volatile long currentFrequency = 50000;
unsigned int lastReportedPos = 1; // change management
static boolean rotating = false;  // debounce management

// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;

const int F_HZ = 0;
const int F_KHZ = 1;
const int F_MHZ = 2;

const int freqUnitPin = A0;
int freqUnitPinState = LOW;
int freqUnit = F_HZ;

const int waveShapePin = A1;
int waveShapeState = LOW;

const int atnPin = A2;
int atnPinState = LOW;

int buttonState = 0; // current state of the button

void setup()
{
    pinMode(encoderPinA, INPUT);
    pinMode(encoderPinB, INPUT);

    pinMode(freqUnitPin, INPUT);
    pinMode(waveShapePin, INPUT);
    pinMode(atnPin, INPUT);

    digitalWrite(encoderPinA, HIGH);
    digitalWrite(encoderPinB, HIGH);
    
    attachInterrupt(0, doEncoderA, CHANGE);
    attachInterrupt(1, doEncoderB, CHANGE);

    gen.Begin();
    gen.ApplySignal(SINE_WAVE, REG0, currentFrequency);
    gen.EnableOutput(true);

    Serial.begin(9600); // output

    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.lineWrap();
    lcd.print("Signal Gen 1.00");

    Serial.println("Signal Gen 1.00");
}

int wasLow = 0;

void loop()
{    

    rotating = true; // reset the debouncer

    if (lastReportedPos != encoderPos)
    {
        lastReportedPos = encoderPos;
        
            long scale;

            switch (freqUnit)
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

            currentFrequency += lastReportedPos * scale;
            lastReportedPos = 0;
            encoderPos=0;

            char buffer[40];
            char frequencyText[14];
            memset(frequencyText, '\0', 14);
            dtostrf(currentFrequency, 10, 1, frequencyText);
            sprintf(buffer, "New frequency %s", frequencyText);
            Serial.println(buffer);
            gen.SetFrequency(REG0, currentFrequency);
            lcd.clear();
            lcd.home();
            lcd.print(frequencyText);
        
    }

    buttonState = digitalRead(freqUnitPin);
    
    if(buttonState == LOW)
    {        
        if(wasLow == 0)
        {
            Serial.println("Low");
        }

        wasLow = 1;
    }
    else if(buttonState == HIGH)
    {
        wasLow = 0;
        Serial.println("High");
    }
    
    if (buttonState != freqUnitPinState)
    {        
        if (buttonState == HIGH)
        {            
            freqUnit++;
            if(freqUnit > F_MHZ)
            {
                freqUnit = F_HZ;
            }
        }

        delay(50);
    }

    freqUnitPinState = buttonState;
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

// TODO: Move these to modules but lets get these working first

///////////////////////////////////////////////////////////////////////////////
// LCD
///////////////////////////////////////////////////////////////////////////////

void lcd_setup(void)
{

}

void lcd_loop()
{

}

///////////////////////////////////////////////////////////////////////////////
// ADS Chip
///////////////////////////////////////////////////////////////////////////////
void ads_setup(void)
{

}

void ads_loop(void)
{

}

///////////////////////////////////////////////////////////////////////////////
// Rotary Encoder
///////////////////////////////////////////////////////////////////////////////
void encoder_setup(void)
{

}

void encoder_loop(void)
{

}