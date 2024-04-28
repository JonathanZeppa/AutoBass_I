#include <MIDI.h>
//#include <MIDIcontroller.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <Wire.h>
#include <Bounce.h>
#include <Encoder.h>

//extern const uint16_t bass[];
//extern const uint16_t treble[];

// pinout for the quadrature encoders
Encoder knob04(11, 12);
Encoder knob03(20, 21);
Encoder knob02(2, 3);
Encoder knob01(22, 23);

int bTime = 10; // Bass note sustain release time, MIDI CC 72
int boing = 124; // Daves was 30, time in milliseconds?
//int bassCC = 72; // 72 - release, 80- Decay On/Off, 69-hold(63Off/64On), Legato- 68 On/Off, 

#define PADS 3 // How many drum pads?
#define DEBOUNCE boing // Debounce time (in milli-seconds)

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_LED LED_BUILTIN

const int BOUNCE_TIME = 25; 
// constant bounce time makes it easier to fine tune if the buttons need more/less time

const int TOGGLE01 = 6; // SUSTAIN (WORKING) // Future Leslie control
const int TOGGLE02 = 5; // Chord on/off mono/poly
const int TOGGLE03 = 4; // HOLD on/off mono/poly // Also PANIC "All notes off"
const int TOGGLE04 = 17;// Treble low octave switch
const int TOGGLE05 = 7; // Channel Switch Bass Treble, PAN Intitialize
const int TOGGLE06 = 30; // Drum Menu
const int TOGGLE07 = 29; // Bass Legato, will this work?


// Initial toggle variable for each button. 1 = OFF ,  0 = ON
int b01 = 1; // SUSTAIN - Long release for drums
int b02 = 1; // Chord on/off
int b03 = 1; // HOLD on/off
int b04 = 1; // Treble Low Octave
int b05 = 1; // Channel Switch Bass Treble, PAN Start
int b06 = 1; // Drum Menu
int b07 = 1; // Panic Button, All notes off 120-123

Bounce b01Toggle= Bounce(TOGGLE01, BOUNCE_TIME);
Bounce b02Toggle= Bounce(TOGGLE02, BOUNCE_TIME);
Bounce b03Toggle= Bounce(TOGGLE03, BOUNCE_TIME);
Bounce b04Toggle= Bounce(TOGGLE04, BOUNCE_TIME);
Bounce b05Toggle= Bounce(TOGGLE05, BOUNCE_TIME);
Bounce b06Toggle= Bounce(TOGGLE06, BOUNCE_TIME);
Bounce b07Toggle= Bounce(TOGGLE07, BOUNCE_TIME);

int sensitivity = 91; // Maximum input range, Dave's = 100
int threshold = 91; // Minimum input to drive the note

unsigned long timer[PADS];
bool playing[PADS];
int highScore[PADS];

//Where does the default note value go? 

// Waveshare RGB OLED 128x128px
#define TFT_CS   10  // CS
#define TFT_RST   8  // Reset
#define TFT_DC    9  // DC/RS
#define TFT_MOSI  18 // MOSI/SDA
#define TFT_SCLK  19 // SCLK/CLK

// Screen dimensions
#define TFT_W 128
#define TFT_H 128

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

//Adafruit_SSD1351 tft = Adafruit_SSD1351(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SSD1351 tft = Adafruit_SSD1351(TFT_W, TFT_H, TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Used to refer to the width and height of the display
uint16_t tft_w;
uint16_t tft_h;

// Character positions for text writing
uint8_t tft_rows;
uint8_t tft_cols;
uint8_t col, row;
uint8_t testnote = 21;

char * Key1[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
int keyNUM[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
int transpose = 0;

int modeColor[] = {
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0,
0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
0x07FF, 0x07FF, 0x07FF
};

char * Mode2[] = {
"Chroma 5", "Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian", 

"HarmonicMIN", "Locrian #6", "Ionian #5", "Dorian #4", "Phrygian DM", "Lydian #2", "UltraLocrian",

"D. HarmonicMAJ", "Lydian#2#6", "UltraPhrygian", "HungarianMIN", "Asian", "Ionian#2#5", "Loc bb3 bb7", 

"HarmonicMAJ", "HungarianMAJ", "Lydian #9"
  }; //

int modeVar = 0;// filled in from the encoder

char modeSelect2[25][12][2] = {
  {{0,4}, {1,4}, {2, 4}, {3, 4}, {4,4}, {5,4}, {6,4}, {7,4}, {8,4}, {9,4}, {10,4}, {11,4}}, // 0. Chromatic with 5th/Oct
  {{0,0}, {0,0}, {2, 1}, {2, 1}, {4,1}, {5,0}, {5,0}, {7,0}, {7,0}, {9,1}, {9,1}, {11,2}}, // 1. Ionian
  {{0,1}, {1,0}, {2, 1}, {2, 1}, {3,0}, {5,0}, {5,0}, {7,1}, {8,0}, {9,2}, {10,5}, {10,0}}, // 2. Dorian
  {{0,1}, {0,1}, {1, 0}, {1, 0}, {3,0}, {5,1}, {5,1}, {7,2}, {7,2}, {8,0}, {8,0}, {10,1}}, // 3. Phrygian
  {{0,0}, {0,0}, {2, 0}, {2, 0}, {4,1}, {6,2}, {6,2}, {7,0}, {7,0}, {9,1}, {9,1}, {11,1}}, // 4. Lydian
//    0      1      2      3      4      5      6      7     8       9      10      11     12
// {"MAJ", "MIN", "DIM", "AUG", "5th", "7#9", "b5", "sus4", "S4b5", "Loc", "Phr", "Lyd", "DMb3"};

  
  {{0,0}, {0,0}, {2, 1}, {2, 1}, {4,2}, {5,0}, {7,0}, {7,1}, {8,0}, {9,1}, {10,1}, {10,0}}, // 5. Mixolydian

//           #               #                    #             #             #
//   1              2              3      4             5              6             7
//   0      1       2      3       4      5      6      7      8       9      10     11 
  {{0,1}, {0,0}, {2, 2}, {2, 0}, {3,0}, {5,1}, {5,0}, {7,1}, {7,0}, {8,0}, {8,0}, {10,0}}, // 6. Aeolian

  
  {{0,2}, {0,2}, {1, 0}, {1, 0}, {3,1}, {5,1}, {5,1}, {6,0}, {6,0}, {8,0}, {8,0}, {10,1}},  // 7. Locrian 

  {{0,1}, {0,1}, {2, 2}, {2, 2}, {3,3}, {5,1}, {5,1}, {7,0}, {7,0}, {8,0}, {8,0}, {11,2}}, // 8. Harmonic Minor!, Aeolian Natural 7
  {{0,2}, {0,2}, {1, 3}, {1, 0}, {3,1}, {5,0}, {5,1}, {6,0}, {6,0}, {9,2}, {8,0}, {10,1}}, // 9. Locrian #6.
  {{0,3}, {0,0}, {2, 1}, {2, 0}, {4,0}, {5,0}, {5,0}, {7,2}, {7,0}, {9,1}, {9,1}, {11,2}}, // 10. Ionian #5.
  {{0,1}, {1,0}, {2, 0}, {2, 1}, {3,0}, {6,2}, {6,0}, {7,1}, {8,0}, {9,2}, {10,5}, {10,3}},// 11. Dorian #4.
  {{0,0}, {1,1}, {1, 2}, {1, 0}, {4,2}, {5,1}, {5,2}, {7,2}, {7,3}, {8,3}, {8,2}, {10,1}}, // 12. Phrygian Dominant.
  {{0,0}, {0,0}, {3, 2}, {3, 0}, {4,1}, {6,2}, {6,2}, {7,3}, {7,0}, {9,1}, {9,1}, {11,0}}, // 13. Lydian #2.
  {{0,2}, {0,2}, {1, 1}, {1, 0}, {3,2}, {5,3}, {5,1}, {6,1}, {6,0}, {8,0}, {8,0}, {9,0}},  // 14. UltraLocrian, Superlocrian bb7.

  {{0,0}, {1,7}, {1, 0}, {4, 11}, {4,1}, {5,1}, {5,2}, {7,6}, {7,9}, {8,3}, {11,9}, {11,12}}, // 15. Double harmonic major 1 ♭2  3 4 5 ♭6  7 8
  {{0,0}, {1,0}, {3, 1}, {3, 1}, {4,1}, {6,6}, {6,0}, {7,3}, {8,0}, {10,12}, {10,5}, {11,0}}, // 16. Lydian♯2♯6
  {{0,1}, {0,1}, {1, 1}, {1, 0}, {3,6}, {4,3}, {5,1}, {7,12}, {7,2}, {8,0}, {8,0}, {9,0}},    // 17. Ultraphrygian  1 ♭2  ♭3  ♭4  5 ♭6  double flat7  8
  {{0,1}, {0,0}, {2, 6}, {2, 0}, {3,3}, {6,12}, {6,2}, {7,0}, {7,0}, {8,0}, {9,1}, {11,1}},   // 18. Hungarian minor, Double Harmonic Minor
  {{0,6}, {0,0}, {1, 3}, {1, 1}, {4,12}, {5,0}, {5,0}, {6,0}, {7,1}, {9,1}, {9,1}, {10,1}},   // 19. Asian  1 ♭2  3 4 ♭5  6 ♭7  8
  {{0,3}, {0,1}, {3, 12}, {3, 2}, {4,0}, {5,0}, {5,1}, {7,1}, {7,1}, {9,1}, {11,0}, {11,6}},  // 20. Ionian ♯2 ♯5  1 ♯2  3 4 ♯5  6 7 8
  {{0,12}, {0,1}, {1, 0}, {2, 2}, {2,0}, {5,1}, {5,1}, {6,1}, {6,0}, {8,6}, {8,0}, {9,3}},    // 21. Locrian bb3 bb7  1 ♭2  double flat3  4 ♭5  ♭6

  {{0,0}, {0,1}, {2, 2}, {2, 0}, {4,1}, {5,1}, {5,1}, {7,0}, {8,0}, {8,3}, {10,0}, {11,2}},  //?? 22. Harmonic Major 1 2 3 4 5 ♭6 7
  {{0,0}, {0,0}, {3, 2}, {3, 2}, {4,2}, {6,2}, {6,4}, {7,3}, {8,4}, {9,1}, {10,4}, {10,3}}, // 23. Blues Exotic - Hungarian Major


  {{0,1}, {0,0}, {3, 2}, {3, 3}, {4,1}, {6,3}, {6,8}, {7,3}, {9,11}, {9,1}, {11,7}, {11,0}}// 24. Lydian #9, 


};

int s1 = 0; 
int s2 = 0;
int chord = 0;


int ModeMin[] = {0,0,0,0};
int ModeMax[] = {11, 24, 127, 127};

long lastrotval[] = {-999,-999,-999,-999};
long currrotval[] = {0,0,0,0};

int positions[] = {0,0,0,0};

// Button array for octave control for bass and treble? [FUTURE]
int octaves[] = {12,24,36,48,60,72,84,96};
int oct = octaves[4];
int dOct = octaves[3];
int root = 0;
int padNote = 36;
int rootPlay = 0;
//int troot = 0;
//int dOctaves[] = {12,24,36,48,60,72,84,96};

char* chordName[13] = {"MAJ ", "MIN ", "DIM ", "AUG ", "5th ", "7#9 ", "b5  ", "sus4", "S4b5", "Loc ", "Phr ", "Lyd ", "DMb3"};

int addOct = 0;

void myNoteOn(byte channel, byte note, byte velocity) {
if (channel == 1) {
if (b03 == 0) // Hold ON
{
    
    MIDI.sendControlChange(123, 0, 1);
    //usbMIDI.sendControlChange(120, 0, 1);
    delay(5);
    if  (note == 36) { root = ((modeSelect2[modeVar][0][0])+oct); chord = modeSelect2[modeVar][0][1];}
    if  (note == 37) { root = ((modeSelect2[modeVar][1][0])+oct); chord = modeSelect2[modeVar][1][1];}
    if  (note == 38) { root = ((modeSelect2[modeVar][2][0])+oct); chord = modeSelect2[modeVar][2][1];} 
    if  (note == 39) { root = ((modeSelect2[modeVar][3][0])+oct); chord = modeSelect2[modeVar][3][1];}
    if  (note == 40) { root = ((modeSelect2[modeVar][4][0])+oct); chord = modeSelect2[modeVar][4][1];}
    if  (note == 41) { root = ((modeSelect2[modeVar][5][0])+oct); chord = modeSelect2[modeVar][5][1];}
    if  (note == 42) { root = ((modeSelect2[modeVar][6][0])+oct); chord = modeSelect2[modeVar][6][1];}
    if  (note == 43) { root = ((modeSelect2[modeVar][7][0])+oct); chord = modeSelect2[modeVar][7][1];}
    if  (note == 44) { root = ((modeSelect2[modeVar][8][0])+oct); chord = modeSelect2[modeVar][8][1];}
    if  (note == 45) { root = ((modeSelect2[modeVar][9][0])+oct); chord = modeSelect2[modeVar][9][1];}
    if  (note == 46) { root = ((modeSelect2[modeVar][10][0])+oct); chord = modeSelect2[modeVar][10][1];}
    if  (note == 47) { root = ((modeSelect2[modeVar][11][0])+oct); chord = modeSelect2[modeVar][11][1];}
    if  (note == 48) { root = ((modeSelect2[modeVar][0][0])+oct+12); chord = modeSelect2[modeVar][0][1]; //Octave, no need for change
    }
    MIDI.sendNoteOn(root+transpose, 100, channel); // root
    Serial.print ("Root:");
    Serial.println (root+transpose);
    rootPlay = root+transpose;
    if (b02 == 0) // CHORD ON
    { 
 
      switch (chord) {
          case 0: //maj
              s1 = 4;
              s2 = 7;
              break;
          case 1: //minor
              s1 = 3;
              s2 = 7;
              break;
          case 2: //dim
              s1 = 3;
              s2 = 6;
              break;   
          case 3: //aug
              s1 = 4;
              s2 = 8;
              break;     
          case 4: //5th-OCT
              s1 = 7;
              s2 = 12;
              break;
          case 5: //7#9
              s1 = 4;
              s2 = 10;
              break;
          case 6: //b5
              s1 = 4;
              s2 = 6;
              break;                  
          case 7: //sus4
              s1 = 5;
              s2 = 7;
              break;
          case 8: //S4b5
              s1 = 5;
              s2 = 7;
              break;             
          case 9: //LOC
              s1 = 1;
              s2 = 6;
              break;
          case 10: //PHR
              s1 = 1;
              s2 = 7;
              break;
          case 11: //LYD
              s1 = 6;
              s2 = 7;
              break;          
          case 12: //DMb3
              s1 = 2;
              s2 = 6;
              break;                                     
      }

    

      MIDI.sendNoteOn(root+s1+transpose, 100, channel); // dominant
      MIDI.sendNoteOn(root+s2+transpose, 100, channel); // tonic
    }
   }

else {
    MIDI.sendControlChange(123, 0, 1);
    delay (5);
    if  (note == 36) { root = ((modeSelect2[modeVar][0][0])+oct); chord = modeSelect2[modeVar][0][1];}
    if  (note == 37) { root = ((modeSelect2[modeVar][1][0])+oct); chord = modeSelect2[modeVar][1][1];}
    if  (note == 38) { root = ((modeSelect2[modeVar][2][0])+oct); chord = modeSelect2[modeVar][2][1];} 
    if  (note == 39) { root = ((modeSelect2[modeVar][3][0])+oct); chord = modeSelect2[modeVar][3][1];}
    if  (note == 40) { root = ((modeSelect2[modeVar][4][0])+oct); chord = modeSelect2[modeVar][4][1];}
    if  (note == 41) { root = ((modeSelect2[modeVar][5][0])+oct); chord = modeSelect2[modeVar][5][1];}
    if  (note == 42) { root = ((modeSelect2[modeVar][6][0])+oct); chord = modeSelect2[modeVar][6][1];}
    if  (note == 43) { root = ((modeSelect2[modeVar][7][0])+oct); chord = modeSelect2[modeVar][7][1];}
    if  (note == 44) { root = ((modeSelect2[modeVar][8][0])+oct); chord = modeSelect2[modeVar][8][1];}
    if  (note == 45) { root = ((modeSelect2[modeVar][9][0])+oct); chord = modeSelect2[modeVar][9][1];}
    if  (note == 46) { root = ((modeSelect2[modeVar][10][0])+oct); chord = modeSelect2[modeVar][10][1];}
    if  (note == 47) { root = ((modeSelect2[modeVar][11][0])+oct); chord = modeSelect2[modeVar][11][1];}
    if  (note == 48) { root = ((modeSelect2[modeVar][0][0])+oct+12); chord = modeSelect2[modeVar][0][1]; //Octave, no need for change
    }
    MIDI.sendNoteOn(root+transpose, 100, channel);
    Serial.print ("Root:");
    Serial.println (root+transpose);
    rootPlay = root+transpose;
    // root
    if (b02 == 0) // CHORD ON
    { 
      switch (chord) {
          case 0: //maj
              s1 = 4;
              s2 = 7;
              break;
          case 1: //minor
              s1 = 3;
              s2 = 7;
              break;
          case 2: //dim
              s1 = 3;
              s2 = 6;
              break;   
          case 3: //aug
              s1 = 4;
              s2 = 8;
              break;     
          case 4: //5th-OCT
              s1 = 7;
              s2 = 12;
              break;
          case 5: //7#9
              s1 = 4;
              s2 = 10;
              break;
          case 6: //b5
              s1 = 4;
              s2 = 6;
              break;                  
          case 7: //sus4
              s1 = 5;
              s2 = 7;
              break;
          case 8: //S4b5
              s1 = 5;
              s2 = 7;
              break;             
          case 9: //LOC
              s1 = 1;
              s2 = 6;
              break;
          case 10: //PHR
              s1 = 1;
              s2 = 7;
              break;
          case 11: //LYD
              s1 = 6;
              s2 = 7;
              break;          
          case 12: //DMb3
              s1 = 2;
              s2 = 6;
              break;        
      }

    

      MIDI.sendNoteOn(root+s1+transpose, 100, channel); // dominant
      MIDI.sendNoteOn(root+s2+transpose, 100, channel); // tonic
    }
   }
} //channel 1

if (channel == 10) { //usbMIDI.sendControlChange(123, 0, 2);
    int seqNote;
    int bass;
    seqNote = MIDI.getData1();
    MIDI.sendNoteOn(seqNote, 100, 10);
    //usbMIDI.sendControlChange(123, 0, 2);  // Kill this line? // This method isolates the bass to only the kick bveing played, but now the note is short
    if  (seqNote == 36) { 
      MIDI.sendControlChange(123, 0, 2);
      bass = rootPlay-dOct+addOct; Serial.print ("seqNote:"); Serial.print (seqNote); Serial.print (" Kick:"); Serial.println (bass); MIDI.sendNoteOn(bass, 100, 2);}
 }   
} 
  
void myNoteOff(byte channel, byte note, byte velocity){
  if (channel == 1) {
  // if (b03 == 0), sendNoteOff to channel 3 (or 15?). Will this work?  In theory, this way channel 1 never gets a note off message.
    if  (note == 36) { root = ((modeSelect2[modeVar][0][0])+oct); chord = modeSelect2[modeVar][0][1];}
    if  (note == 37) { root = ((modeSelect2[modeVar][1][0])+oct); chord = modeSelect2[modeVar][1][1];}
    if  (note == 38) { root = ((modeSelect2[modeVar][2][0])+oct); chord = modeSelect2[modeVar][2][1];} 
    if  (note == 39) { root = ((modeSelect2[modeVar][3][0])+oct); chord = modeSelect2[modeVar][3][1];}
    if  (note == 40) { root = ((modeSelect2[modeVar][4][0])+oct); chord = modeSelect2[modeVar][4][1];}
    if  (note == 41) { root = ((modeSelect2[modeVar][5][0])+oct); chord = modeSelect2[modeVar][5][1];}
    if  (note == 42) { root = ((modeSelect2[modeVar][6][0])+oct); chord = modeSelect2[modeVar][6][1];}
    if  (note == 43) { root = ((modeSelect2[modeVar][7][0])+oct); chord = modeSelect2[modeVar][7][1];}
    if  (note == 44) { root = ((modeSelect2[modeVar][8][0])+oct); chord = modeSelect2[modeVar][8][1];}
    if  (note == 45) { root = ((modeSelect2[modeVar][9][0])+oct); chord = modeSelect2[modeVar][9][1];}
    if  (note == 46) { root = ((modeSelect2[modeVar][10][0])+oct); chord = modeSelect2[modeVar][10][1];}
    if  (note == 47) { root = ((modeSelect2[modeVar][11][0])+oct); chord = modeSelect2[modeVar][11][1];}
    if  (note == 48) { root = ((modeSelect2[modeVar][0][0])+oct+12); chord = modeSelect2[modeVar][0][1];
    
    } //Octave, no need for change
    
    if (b03 == 0){ // HOLD ON
    MIDI.sendNoteOff(root+transpose, velocity, 3); // Experiment to see if this line is necessary 
    if (b02 == 0){ // CHORD OFF
    MIDI.sendNoteOff(root+s1+transpose, velocity, 3);
    MIDI.sendNoteOff(root+s2+transpose, velocity, 3);}
    }

    if (b03 == 1){ // HOLD ON
    MIDI.sendNoteOff(root+transpose, velocity, channel); // Experiment to see if this line is necessary 
    if (b02 == 0){ // CHORD OFF
    MIDI.sendNoteOff(root+s1+transpose, velocity, channel);
    MIDI.sendNoteOff(root+s2+transpose, velocity, channel);}
    }
}//Channel 1
 //if (channel == 10)  {usbMIDI.sendNoteOff(bass, velocity, 2);}
}
void myControlChange(byte channel, byte number, byte value) {
  MIDI.sendControlChange(number, value, channel);
}

void setup() {
int ver = 1;
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  MIDI.setHandleControlChange(myControlChange);

  // Buttons
  pinMode(TOGGLE01, INPUT_PULLUP);
  pinMode(TOGGLE02, INPUT_PULLUP);
  pinMode(TOGGLE03, INPUT_PULLUP);
  pinMode(TOGGLE04, INPUT_PULLUP);
  pinMode(TOGGLE05, INPUT_PULLUP);
  pinMode(TOGGLE06, INPUT_PULLUP);
  pinMode(TOGGLE07, INPUT_PULLUP);
  
  tft.begin();
  tft.setRotation(0);
  tft_w = tft.width();
  tft_h = tft.height();
  tft.fillScreen(BLACK);

  tft.setTextSize(5);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(4, 10);
  tft.println("Auto");
  tft.setTextColor(BLACK, RED);
  tft.setCursor(4, 50);
  tft.println("BASS");
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(15, 100);
  tft.print("VER. 3.0");
  //tft.println(ver);

  delay(200);
  tft.fillScreen(BLACK);

  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(78, 0);
  tft.println("Auto");
  tft.setTextColor(BLACK, RED);
  tft.setCursor(78, 15);
  tft.println("BASS");
  tft.setTextSize(1);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(78, 32);
  tft.print("VER. 3.0");
  //tft.println(ver);
 
// Drum Sensitivity / Threshold
  tft.setTextSize(1);
  tft.setTextColor(BLUE, BLACK);
  tft.setCursor(23, 87);
  tft.print("S:");
  tft.setCursor(36, 87);
  tft.print(sensitivity);
  tft.setCursor(59, 87);
  tft.print("T:");
  tft.setCursor(72, 87);
  tft.print(threshold);
  tft.setCursor(95, 87);
  tft.print("B:");
  tft.setCursor(108, 87);
  tft.print(DEBOUNCE);
  
// KEY
  tft.setCursor(8, 50);
  tft.setTextSize(2);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("C");
  tft.println("");
// Mode
  tft.setCursor(50, 53);
  tft.setTextSize(1);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Ionian");
  tft.println("");




// Bank    patch
 tft.setTextSize(1);
  tft.setTextColor(RED, BLACK);
  tft.setCursor(23, 75);
  tft.println("BANK:");
   tft.setTextSize(1);
  tft.setTextColor(RED, BLACK);
  tft.setCursor(75, 75);
  tft.println("PATCH:");

//Bass clef graphic, or G Clef
  //tft.drawRGBBitmap(0,75,treble,20,20);
  
  tft.setTextSize(1);
  tft.setTextColor(GREEN, BLACK);
  tft.setCursor(0, 98);
  tft.println("Mod:");
  tft.setCursor(0, 110);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.print("Off");
  tft.println(" ");

  tft.setTextColor(GREEN, BLACK);
  tft.setCursor(52, 98);
  tft.setTextSize(1);
  tft.println("Chord:");
  tft.setCursor(52, 110);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.print("Off");
  tft.println(" ");

  tft.setTextColor(BLACK, GREEN);
  tft.setCursor(92, 98);
  tft.setTextSize(1);
  tft.println("HOLD:");
  tft.setCursor(92, 110);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.print("Off");
  tft.println(" ");

  printInit();

  pinMode (MIDI_LED, OUTPUT);
  digitalWrite (MIDI_LED, LOW);
// I don't think this works. Does seem to load up a default 
//int boots = 36;
  
  //Midi bank/patch start is not working
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  for (int x = 0; x < PADS; x++) {
    playing[x] = false;
    highScore[x] = 0;
    timer[x] = 0;

  } 
}


// channel switching byte for bass and treble bank/patches
byte ch05 = 1;

void loop() {

  //MIDI.read();

  //read each encoder
  currrotval[0]  = (knob01.read()/4);
  currrotval[1]  = (knob02.read()/4);
  currrotval[2]  = (knob03.read()/4);
  currrotval[3]  = (knob04.read()/4);

 for (int x = 0; x <4; x++)
  {
    if (currrotval[x] > lastrotval[x])
    {
      positions[x]++;
      if (positions[x] > ModeMax[x]) positions[x] = ModeMin[x];

// KEY    A 
if (x == 0)  
      {
        if (b06 == 1){
       tft.setCursor(8, 50);
       tft.setTextSize(2);
       tft.setTextColor(WHITE, BLACK);
       tft.print(Key1[positions[0]]);
       tft.println(" ");
       transpose = (keyNUM[positions[0]]);}

//Drum sensitivity A

       if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(36, 87);
       tft.println("   ");
       tft.setCursor(36, 87);
       tft.print(positions[0]+85);
       sensitivity = (positions[0]+85);
       }
      }

// MODE   A
      if (x == 1)
      {
        
  if (b06 == 1) {
        tft.setCursor(45, 53);
        tft.setTextSize(1);
        tft.setTextColor((modeColor[positions[1]]), BLACK);
        //if (modeVar <= 7) {tft.setTextColor(WHITE, BLACK);}
        //if (modeVar >= 8) {tft.setTextColor(modeColor, BLACK);}
        //if (modeVar >= 15) {tft.setTextColor(CYAN, BLACK);}
        tft.println("              ");
        tft.setCursor(45, 53);
        tft.print(Mode2[positions[1]]);
        modeVar = (positions[1]);}

// Threshold A  1-25
        if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(72, 87);
       tft.println("   ");
       tft.setCursor(72, 87);
       tft.print(positions[1]+75);
       threshold = (positions[1]+75);
       }       
      }

// BANK   A   
      if (x == 2)
      {
        if (b06 == 1){
        tft.setCursor(53, 75);
        tft.setTextSize(1);
        tft.setTextColor(WHITE, BLACK);
        tft.print("   ");
        tft.setCursor(53, 75);
        tft.print(positions[2]); 
        //ch05
        MIDI.sendControlChange(32,positions[2],16);
        //usbMIDI.sendControlChange(14,positions[2],16);
        }
 if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(108, 87);
       tft.println("   ");
       tft.setCursor(108, 87);
       tft.print(positions[2]*2);
       boing = (positions[2]*2);
       }
}

// Patch  A    
      if (x == 3)
      {
        if (b06 == 1){
        tft.setCursor(110, 75);
        tft.setTextSize(1);
        tft.setTextColor(WHITE, BLACK);
        tft.print(positions[3]);  
        tft.println("  ");
        //ch05
        MIDI.sendProgramChange(positions[3],16);
        //usbMIDI.sendControlChange(16,positions[3],16);
        }
// if (b06 == 0){
//       tft.setTextSize(1);
//       tft.setTextColor(WHITE, BLACK);
//       tft.setCursor(100, 42);
//       tft.println("    ");
//       tft.setCursor(100, 42);
//       tft.print(positions[3]);
//       bTime = (positions[3]*10);
//       //MIDI.sendControlChange(bassCC, bTime, 2); //80 Decay
//       }   
      }
      
    }
    if (currrotval[x] < lastrotval[x])
    {
      positions[x]--;
      if (positions[x] < ModeMin[x]) positions[x] = ModeMax[x];

// KEY    B
      if (x == 0)  
{
        if (b06 == 1){
       tft.setCursor(8, 50);
       tft.setTextSize(2);
       tft.setTextColor(WHITE, BLACK);
       tft.print(Key1[positions[0]]);
       tft.println(" ");
       transpose = (keyNUM[positions[0]]);}

//Drum sensitivity B

       if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(36, 87);
       tft.println("   ");
       tft.setCursor(36, 87);
       tft.print(positions[0]+85);
       sensitivity = (positions[0]+85);
       }
       
      }
// 02 MODE B
      if (x == 1)
      {
    if (b06 == 1) {
        tft.setCursor(45, 53);
        tft.setTextSize(1);
        tft.setTextColor((modeColor[positions[1]]), BLACK);
        tft.println("              ");
        tft.setCursor(45, 53);
        tft.print(Mode2[positions[1]]); // tft.print(Mode2[position02]);
        modeVar = (positions[1]);}

// Threshold B  1-25
    if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(72, 87);
       tft.println("   ");
       tft.setCursor(72, 87);
       tft.print(positions[1]+75);
       threshold = (positions[1]+75);
       }
 }
            
// 03 BANK B    
      if (x == 2)
      { 
         if (b06 == 1){
        tft.setCursor(53, 75);
        tft.setTextSize(1);
        tft.setTextColor(WHITE, BLACK);
        tft.print("   ");
        tft.setCursor(53, 75);
        tft.print(positions[2]); 
        //ch05
        MIDI.sendControlChange(32,positions[2],16);
        //usbMIDI.sendControlChange(15,positions[2],16);
      }
       
 if (b06 == 0){
       tft.setTextSize(1);
       tft.setTextColor(WHITE, BLACK);
       tft.setCursor(108, 87);
       tft.println("   ");
       tft.setCursor(108, 87);
       tft.print(positions[2]*2);
       boing = (positions[2]*2);
       }
       
       }


       
// 04 PATCH B     
      if (x == 3) {
        
        if (b06 == 1){
        tft.setCursor(110, 75);
        tft.setTextSize(1);
        tft.setTextColor(WHITE, BLACK);
        //tft.print(position04);
        tft.print(positions[3]);
        tft.println("    ");
        //ch05
        MIDI.sendProgramChange(positions[3],16);
        //usbMIDI.sendControlChange(17,positions[3],16);
        }        
      }
    }
    lastrotval[x] = currrotval[x];
  }


int channel;
int noteNUM;
int note02;
  if (MIDI.read()) {
    switch (MIDI.getType()) {
      case midi::NoteOn:
      channel = MIDI.getChannel();
      noteNUM = MIDI.getData1();
      note02 = MIDI.getData2();
      if (channel == 1) { printNote (root+transpose); } 
      if (channel == 10) {  
        Serial.print("Note: "); //Data-01
        Serial.print(noteNUM);
        Serial.print(" Channel: "); //Data-01
        Serial.println(channel);
        break;
        }
        break;

      default: // Otherwise it is a message we aren't interested in
        break;
    }
  }


updateButtons();

// Analog Trigger Section
byte padNote[PADS] = {root+transpose-dOct+addOct, root+transpose+s1-dOct+addOct, root+transpose+s2-dOct+addOct}; // Set drum pad notes here
  for (int x = 0; x < PADS; x++) {
    int volp = analogRead(x);
    int volume = (volp/8);

    if (volume >= threshold && playing[x] == false) {
      if (millis() - timer[x] >= DEBOUNCE) {
        playing[x] = true;
        playNote(x, volume);
      }
    }
    else if (volume >= threshold && playing[x] == true) {
      playNote(x, volume);
    }
    else if (volume < threshold && playing[x] == true) {
      usbMIDI.sendControlChange(123, 0, 2);
      //usbMIDI.sendControlChange(120, 0, 2);
      //delay(5);
      //MIDI.sendControlChange(64, 127, 2);
      usbMIDI.sendNoteOn(padNote[x], highScore[x], 2);
      //delay(bTime); //default 10ms
      //MIDI.sendControlChange(64, 0, 2);
      //MIDI.sendNoteOff(padNote[x], 0, 2);

      highScore[x] = 0;
      playing[x] = false;
      timer[x] = millis();
    }
  }

}


//#define NEXTCOL 4 - I don't think this is used
char notes[12]  = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
char sharps[12] = {' ', '#', ' ', '#', ' ', ' ', '#', ' ', '#', ' ', '#', ' '};
//void printNote () {
void printNote (uint8_t note) {
  // MIDI notes go from A0 = 21 up to G9 = 127
  tft.setCursor(0, 0);
  tft.setTextSize(5);
  tft.setTextColor(WHITE, BLACK);
  tft.print(notes[note % 12]);
  tft.setTextSize(2);
  tft.print(sharps[note % 12]);
  tft.setTextColor(BLUE, BLACK);
  tft.print((note / 12) - 1, DEC);
  tft.println(" ");
  tft.setCursor(28, 22);
  tft.setTextSize(2);
  tft.setTextColor(BLUE, BLACK);
  if (b02 == 0){
  tft.println(chordName[chord]);}
  else {
    tft.println("    ");
    }
}

// print out of the note on screen
void printInit (void) {
  tft.setTextColor(WHITE, BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(3);
  tft.setCursor(0, 0);
}

// The notes that the drums will play
void playNote (int pad, int volume) {
  float velocity = ((volume) / float(sensitivity - threshold)) * 127;
  if (velocity > 127) velocity = 127;
  if (velocity > highScore[pad]) highScore[pad] = velocity;

}

void updateButtons() {



// Drum Sustain On/Off


b01Toggle.update();
  if (b01Toggle.fallingEdge()) {
    b01 = !b01; 
    if (b01 == 0) {
    MIDI.sendControlChange(64, 127, 1); //release time
    //MIDI.sendControlChange(1, 64, 1); // MOD Wheel
    //(b01 = 1);
    tft.setCursor(0, 110);
    tft.setTextSize(2);
    tft.setTextColor(WHITE, BLACK);
    tft.print("ON");
    tft.println(" "); 
   } 
   if (b01 == 1) {
   MIDI.sendControlChange(64, 0, 1);
   //MIDI.sendControlChange(1, 0, 1); // MOD Wheel
   
   //(b01 = 0);
   tft.setCursor(0, 110);
   tft.setTextSize(2);
   tft.setTextColor(WHITE, BLACK);
   tft.print("Off");
   tft.println(" ");
          }
  }

    


// Chord OnOff
// 90 is an undefined midi CC number. Used here with teh chord to turn on the leslie
// 72 is used for leslie speed fast/slow
b02Toggle.update();
  if (b02Toggle.fallingEdge()) {
    b02 = !b02; 
    if (b02 == 0) {
    //usbMIDI.sendControlChange(90, 127, 1);
    tft.setCursor(52, 110);
    tft.setTextSize(2);
    tft.setTextColor(WHITE, BLACK);
    tft.print("ON");
    tft.println(" "); 
   } 
   if (b02 == 1) {
   tft.setCursor(52, 110);
   tft.setTextSize(2);
   tft.setTextColor(WHITE, BLACK);
   tft.println("Off");
   //usbMIDI.sendControlChange(90, 0, 1);
   
          }
  }


// HOLD On/Off
b03Toggle.update();
  if (b03Toggle.fallingEdge()) {
    b03 = !b03; 
    if (b03 == 0) {
    MIDI.sendControlChange(123, 0, 1);
    MIDI.sendControlChange(120, 0, 1);
    tft.setCursor(92, 110);
    tft.setTextSize(2);
    tft.setTextColor(WHITE, BLACK);
    tft.print("ON");
    tft.println(" ");
    delay(5);
    MIDI.sendControlChange(123, 0, 2);
    MIDI.sendControlChange(120, 0, 2);
    //MIDI.sendControlChange(64, 127, 1);
    //MIDI.sendControlChange(1, 30, 1); // MOD Wheel
   } 
   if (b03 == 1) {
   tft.setCursor(92, 110);
   tft.setTextSize(2);
   tft.setTextColor(WHITE, BLACK);
   tft.println("Off");
   MIDI.sendControlChange(123, 0, 1);
   MIDI.sendControlChange(120, 0, 1);
   delay(5);
   MIDI.sendControlChange(123, 0, 2);
   MIDI.sendControlChange(120, 0, 2);
   //MIDI.sendControlChange(64, 0, 1);
   //MIDI.sendControlChange(1, 0, 1);  // MOD Wheel
   
          }
  }


// Treble Low Octave select
b04Toggle.update();
  if (b04Toggle.fallingEdge()) {
    b04 = !b04; 
    if (b04 == 0) {
    oct = octaves[3];
    //dOct = octaves[1];
    //addOct = 12;
    tft.setTextSize(2);
    tft.setTextColor(BLACK, BLUE);
    tft.setCursor(78, 15);
    tft.println("BASS");
    } 
     if (b04 == 1) {
     oct = octaves[4];
     //dOct = octaves[2];
     //addOct = 0;
     tft.setTextSize(2);
     tft.setTextColor(BLACK, RED);
     tft.setCursor(78, 15);
     tft.println("BASS");    
    }
  }

// Treble-Bass channel switch BANK-PATCH
b05Toggle.update();
  if (b05Toggle.fallingEdge()) {
    b05 = !b05; 
    if (b05 == 0) {
    //usbMIDI.sendControlChange(10, 0, 1); // Channel 01, panned left
    //usbMIDI.sendControlChange(10, 127, 2); // Channel 02, panned right
    
    ch05 = 2;
    //tft.drawRGBBitmap(0,75,bass,20,20);
   } 
   if (b05 == 1) {
    ch05 = 1;
    //tft.drawRGBBitmap(0,75,treble,20,20);
    }
  }


  // Treble Low Octave select
b06Toggle.update();
  if (b06Toggle.fallingEdge()) {
    b06 = !b06; 
    if (b06 == 0) {
    tft.setTextSize(1);
  tft.setTextColor(WHITE, BLUE);
  tft.setCursor(23, 87);
  tft.print("S:");
  tft.setCursor(59, 87);
  tft.print("T:");
  tft.setCursor(95, 87);
  tft.print("B:");
  //tft.setCursor(75, 42);
  //tft.print("bMS:");
    } 
     if (b06 == 1) {
    tft.setTextSize(1);
  tft.setTextColor(BLUE, BLACK);
  tft.setCursor(23, 87);
  tft.print("S:");
  tft.setCursor(59, 87);
  tft.print("T:");
  tft.setCursor(95, 87);
  tft.print("B:");
  //tft.setCursor(75, 42);
  //tft.print("bMS:");
  }
 }

  // 2nd Rotary - Silence All
b07Toggle.update();
  if (b07Toggle.fallingEdge()) {
    b07 = !b07; 
    if (b07 == 0) {
    MIDI.sendControlChange(123, 0, 1);
    MIDI.sendControlChange(120, 0, 1);
    delay(50);
    MIDI.sendControlChange(123, 0, 2);
    MIDI.sendControlChange(120, 0, 2);
    } 
     if (b07 == 1) {
    MIDI.sendControlChange(123, 0, 1);
    MIDI.sendControlChange(120, 0, 1);
    delay(50);
    MIDI.sendControlChange(123, 0, 2);
    MIDI.sendControlChange(120, 0, 2);
  }
 }
 
}
