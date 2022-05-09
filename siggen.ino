#include "AD9833.h" // Include the library

#define FNC_PIN 4 // Can be any digital IO pin

//--------------- Create an AD9833 object ----------------
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
AD9833 gen(9); // Defaults to 25MHz internal reference frequency

// usually the rotary encoders three pins have the ground pin in the middle
enum PinAssignments
{
    encoderPinA = 2, // right
    encoderPinB = 3, // left
    clearButton = 8  // another two pins
};

volatile unsigned int encoderPos = 0; // a counter for the dial
unsigned int lastReportedPos = 1;     // change management
static boolean rotating = false;      // debounce management

// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;

void setup()
{
    pinMode(encoderPinA, INPUT);
    pinMode(encoderPinB, INPUT);
    pinMode(clearButton, INPUT);
    // turn on pullup resistors
    digitalWrite(encoderPinA, HIGH);
    digitalWrite(encoderPinB, HIGH);
    digitalWrite(clearButton, HIGH);

    // encoder pin on interrupt 0 (pin 2)
    attachInterrupt(0, doEncoderA, CHANGE);
    // encoder pin on interrupt 1 (pin 3)
    attachInterrupt(1, doEncoderB, CHANGE);

    // This MUST be the first command after declaring the AD9833 object
    gen.Begin();

    // Apply a 1 MHz square wave using REG0 (register set 0). There are two register sets,
    // REG0 and REG1.
    // Each one can be programmed for:
    //   Signal type - SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, and HALF_SQUARE_WAVE
    //   Frequency - 0 to 12.5 MHz
    //   Phase - 0 to 360 degress (this is only useful if it is 'relative' to some other signal
    //           such as the phase difference between REG0 and REG1).
    // In ApplySignal, if Phase is not given, it defaults to 0.
    gen.ApplySignal(SINE_WAVE, REG0, 12000000);

    gen.EnableOutput(true); // Turn ON the output - it defaults to OFF
    // There should be a 1 MHz square wave on the PGA output of the AD9833

    Serial.begin(9600); // output
}

void loop()
{
    // To change the signal, you can just call ApplySignal again with a new frequency and/or signal
    // type.

    rotating = true; // reset the debouncer

    if (lastReportedPos != encoderPos)
    {
        lastReportedPos = encoderPos;
    }
    if (digitalRead(clearButton) == LOW)
    {
        encoderPos = 0;
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