#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned int xLocator [] = {10, 17, 24, 31, 38, 45, 52, 59, 66, 73, 80, 87, 94, 101, 108, 115};
const unsigned int xButtons [] = {90, 70, 50, 30};

// 'Arrow', 17x10px
const unsigned char Arrow [] PROGMEM = {
	0x00, 0x08, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0e, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x80, 0xff, 
	0xff, 0x80, 0xff, 0xff, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x08, 0x00
};

// 'OLEDCursor', 6x8px
const unsigned char OLEDCursor [] PROGMEM = {
	0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x78, 0x30
};

// 'pauseIcon', 10x16px
const unsigned char pauseIcon [] PROGMEM = {
	0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 
	0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0, 0xf3, 0xc0
};

// 'playIcon', 10x16px
const unsigned char playIcon [] PROGMEM = {
	0xe0, 0x00, 0xf0, 0x00, 0xf8, 0x00, 0xfc, 0x00, 0xfe, 0x00, 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 
	0xff, 0xc0, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 0xfc, 0x00, 0xf8, 0x00, 0xf0, 0x00, 0xe0, 0x00
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 1104)
const int epd_bitmap_allArray_LEN = 5;
const unsigned char* epd_bitmap_allArray[4] = {
	Arrow,
	OLEDCursor,
  playIcon,
  pauseIcon
};

//BUTTON LOGIC CODE
#include <Bounce.h>

struct theBinaries {
  
  static const int numOfBs = 4;
  const int binaryButtons[numOfBs] = {12, 11, 10, 6};
  bool btState[numOfBs] = {false, false, false, false};
  bool changes = false;
  int values[numOfBs] = {1, 2, 4, 8};

  theBinaries() {
    for (int i = 0; i < numOfBs; i++) {
      pinMode(binaryButtons[i], INPUT_PULLUP);
    }
  }

  Bounce binaries[numOfBs] = {
    Bounce(binaryButtons[0], 30),
    Bounce(binaryButtons[1], 30),
    Bounce(binaryButtons[2], 30),
    Bounce(binaryButtons[3], 30)
  };

  bool update() {
    if (changes) {
      changes = false;
      return true;
    }
    return false;
  }

  int value(int a) {
    if (btState[a]) {
      return 1;
    } else{
      return 0;
    }
  }

  int tally() {
    int total = 0;
    for (int i=0;i<numOfBs;i++) {
      if (btState[i]) {
        total |= (1 << i);
      }
    }
    if (total == 0){
      total = 16;
    }
    return total;
  }
};
theBinaries bGang = theBinaries();

// ROTARY ENCODER BPM COUNTER
struct Counter{
  int bpm = 120;
  int bpmPoly = 120;
  int min = 60;
  int max = 240;
  int step = 0;

  void increment(int by){
    if(by>0 && bpm < max){
      bpm += by;
      if(bpm > max) bpm = max;
    }
    else if(by<0 && bpm > min){
      bpm += by;
      if(bpm < min) bpm = min;
    }
  }
};
Counter ctr;

// ROTARY ENCODER CODE
#include <Encoder.h>
#include <Button2.h>
const int sw = 20, dt = 21, clk = 22;
Encoder encoder(dt, clk);
Button2 encoderButton(sw);

// OLED CONTROL STRUCT
bool divideState = true;

struct OLEDControl {
  enum ClockState {    
    STOP_MODE,  // Timer is counting down
    PLAY_MODE   // User sets the timer duration
  };
  ClockState currentState =  STOP_MODE; // Initial state
  
  //OLED FUNCTION
  void updateBPM() {
    display.fillRect(0, 0, 48, 7, BLACK); //7
    display.fillRect(65, 0, 60, 7, BLACK); //7
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.write("BPM: ");
    display.print(int(ctr.bpm));
    display.setCursor(0, 0);
    display.setCursor(65, 0);
    display.write("BPM: ");
    display.print(ctr.bpmPoly);
    display.display(); // Partial update
  }
  // UPDATES VALUE
  void updateValue(int a) {
    display.fillRect(xButtons[a], 18, 10, 20, BLACK);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(xButtons[a], 18);
    display.print(bGang.value(a));
    display.display();
  }
  //ASSIGNS CURSOR POSITION BASED ON WHAT MODE WE'RE IN
  void configureCursor() {
    if (divideState) {
      if (bGang.tally() < 10) {
        display.setCursor(43, 50);
      } else {
        display.setCursor(31, 50);
      }
    } else {
      display.setCursor(75, 50);
    }
  }
  // CHANGE BETWEEN DIVIDE AND MULTIPLY MODES
  void updateMode() {
    if (divideState) {
      display.fillRect(110, 18, 10, 20, BLACK);
      display.setCursor(110, 18);
      display.setTextSize(2);
      display.write("%");
      display.fillRect(0, 50, 100, 20, BLACK);
      configureCursor();
      display.print(bGang.tally());
      display.setCursor(59, 50);
      display.write("/");
      display.setCursor(75,50);
      display.write("16");
    } else {
      display.fillRect(110, 18, 10, 20, BLACK);
      display.setCursor(110, 18);
      display.setTextSize(2);
      display.write("*");
      display.fillRect(0, 50, 100, 20, BLACK);
      display.setCursor(31, 50);
      display.write("16");
      display.setCursor(59, 50);
      display.write("/");
      display.setCursor(75,50);
      configureCursor();
      display.print(bGang.tally());
    }
    display.display();
  }
  // USED TO UPDATE THE FRACTION DISPLAY VALUE
  void updateCustomStep(int a) {
    a=a-1;
    if (divideState) {
      display.fillRect(31,50,25,25, BLACK);
      configureCursor();
      display.print(bGang.tally());
    } else {
      display.fillRect(75,50,25,25, BLACK);
      configureCursor();
      display.print(bGang.tally());
    }
    display.display();
  }
  //CHANGES LCD SCREEN BETWEEN MODES
  void setScreen() {
    switch (currentState) {
      case PLAY_MODE:
        display.fillRect(6, 18, 10, 16, BLACK);
        display.drawBitmap(6,18, playIcon, 10, 16, WHITE);
        updateBPM(); // redraw the bpm
        break;
      case STOP_MODE:
        display.fillRect(6, 18, 10, 16, BLACK);
        display.drawBitmap(6,18, pauseIcon, 10, 16, WHITE);
        updateBPM(); // redraw the bpm
        break;
    }
  }
  //FLIPS ENUM STATE, straightforward
  void switchState(){
    if (currentState == PLAY_MODE) {
      currentState = STOP_MODE;
    } else {
      currentState = PLAY_MODE;
    }
  }

  // RETURNS ENUM STATE, straightforward
  bool isPlaying() {
    return currentState;
  }
};
OLEDControl oledControl;

// INITIALISE THE SCREEN
void displayImage() {
  display.clearDisplay(); // Clear the buffer
  display.drawBitmap(110, 53, Arrow, 17, 10, WHITE);
  display.drawBitmap(6,18, pauseIcon, 10, 16, WHITE);

  display.setTextSize(1);  // Normal 1:1 pixel scale
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.write("BPM: ");
  display.print(ctr.bpm);
  display.setCursor(65, 0);
  display.write("BPM: ");
  display.print(ctr.bpmPoly);

  display.setTextSize(2);
  for (int i = 0; i < 4; i++) {
    display.setCursor(xButtons[i], 18);
    display.print(bGang.value(3-i));
  }
  oledControl.updateMode();
  display.display(); // Refresh the display to show the image
}

// MIDI CODE
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC
#define DEFAULT 16

//CLOCK 1 IS NORMAL
//CLOCK 2 IS POLYRHYTHMIC

IntervalTimer clock1;
IntervalTimer clock2;

uint32_t clockCounter = 0;
bool timerFlag = false;

void midiStart(HardwareSerial &serialPort) {
  serialPort.write(MIDI_START);
}

void midiClock(HardwareSerial &serialPort, int portNum) {
  serialPort.write(MIDI_CLOCK);
  if(oledControl.isPlaying()) {
    if (portNum==1) {
      clockCounter+=1;
      clockCounter=clockCounter%96;
      if (clockCounter==0&&timerFlag==true){
        clock2.end();
        midiStart(Serial2);
        midiStart(Serial1);
        clock2.begin(clock2Handler, tickSwitcher(ctr.bpm, bGang.tally())); 
        timerFlag=false;
        }
    }
  }
}

void midiStop(HardwareSerial &serialPort) {
  serialPort.write(MIDI_STOP);
}

uint32_t calcBPM(float bpm, float inputStep) 
{
  if (divideState) {
    return (bpm * inputStep / 16.0f);
  } else {
    return (bpm * 16.0f / inputStep);
  }
}

uint32_t tickSwitcher(float bpm, float inputStep) 
{
  if (divideState) {
    return bpmTickInterval(bpm, inputStep, 16.0);
  } 
  else {
    return bpmTickInterval(bpm, 16.0, inputStep);
  }
}

uint32_t bpmTickInterval(float bpm, float inputStep, float normally16) 
{ 
  return (60000000.0 / 24.0 / bpm) * (normally16 / inputStep);
}

void clock1Handler() { midiClock(Serial1, 1); }
void clock2Handler() { midiClock(Serial2, 2); }

void setup() {
  //PINS
  pinMode(clk, INPUT); // primary input from rotary encoder, standard pin setting NOT INTERRUPTS that was a lie that made me waste a whole day
  pinMode(dt, INPUT); // secondary input from rotary encoder, standard pin setting
  pinMode(sw, INPUT_PULLUP); // button input from rotary encoder
  encoderButton.setLongClickTime(1500);
  encoderButton.setClickHandler([](Button2 &b) {
    if (!oledControl.isPlaying()) {
        midiStart(Serial1);
        midiStart(Serial2);
        oledControl.switchState(); // switch the lcd state
        oledControl.setScreen(); // update the screen
    } else {
        midiStop(Serial1);
        midiStop(Serial2);
        oledControl.switchState(); // switch the lcd state
        oledControl.setScreen(); // update the screen
        }
  });
  encoderButton.setDoubleClickHandler([](Button2 &b) {
    divideState=!divideState;
    clock2.update(tickSwitcher(ctr.bpm, bGang.tally()));
    ctr.bpmPoly = calcBPM(ctr.bpm, bGang.tally()); 
    oledControl.updateBPM();
    oledControl.updateMode();
    Serial.println("divideState flipped");
  });
  encoder.write(ctr.bpm);

  //Serial
  Serial.begin(31250);
  Serial1.begin(31250);
  Serial2.begin(31250);

  clock1.begin(clock1Handler, tickSwitcher(ctr.bpm, DEFAULT));
  clock2.begin(clock2Handler, tickSwitcher(ctr.bpm, bGang.tally()));

  ctr.bpmPoly = calcBPM(ctr.bpm, bGang.tally());
  //OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
  displayImage();
}

void loop() {

  // CHECK ALL BUTTONS 
  for (int i = 0; i < bGang.numOfBs; i++) {
    if (bGang.binaries[i].update()) {
      if (bGang.binaries[i].fallingEdge()) {
        bGang.btState[i] =! bGang.btState[i];
        bGang.changes = true;
        oledControl.updateValue(i);
      }
    } 
  }

  // CHECK IF WE NEED TO CHANGE THE ANIMATION
  if (bGang.update()) {
    oledControl.updateCustomStep(bGang.tally());
    timerFlag = true;
    ctr.bpmPoly = calcBPM(ctr.bpm, bGang.tally()); 
    oledControl.updateBPM();
    if (!oledControl.isPlaying()) {
      clock2.begin(clock2Handler, tickSwitcher(ctr.bpm, bGang.tally()));
      timerFlag = false;
    }
  }

  // CHECK THE ENCODER VALUE
  int encoderVal = encoder.read(); 
  int delta = encoderVal - ctr.bpm;
  // IF THE VALUE HAS CHANGED, UPDATE THE SCREEN AND THE BPM
  if (delta!=0) {
    ctr.increment(delta);         
    clock1.update(tickSwitcher(ctr.bpm, DEFAULT));
    clock2.update(tickSwitcher(ctr.bpm, bGang.tally()));
    encoder.write(ctr.bpm);
    ctr.bpmPoly = calcBPM(ctr.bpm, bGang.tally());    
    oledControl.updateBPM();  
  }
  // CHECKING FOR ROTARY ENCODER PRESSES
  encoderButton.loop();
}
