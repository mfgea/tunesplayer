// A fun sketch to demonstrate the use of the tone() function written by Brett Hagman.

// This plays RTTTL (RingTone Text Transfer Language) songs using the
// now built-in tone() command in Wiring and Arduino.
// Written by Brett Hagman
// http://www.roguerobotics.com/

// To play the output on a small speaker (i.e. 8 Ohms or higher), simply use
// a 1K Ohm resistor from the output pin to the speaker, and connect the other
// side of the speaker to ground.

// You can get more RTTTL songs from
// http://code.google.com/p/rogue-code/wiki/ToneLibraryDocumentation

#include "tones.h"
#include <LiquidCrystal_I2C.h>

const int tonePin = 9;  // for rEDI board
const int selectPin = 8;
const int playPin = 7;

#define isdigit(n) (n >= '0' && n <= '9')

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte btn1State;
byte btn2State;

String songTitle;
boolean playing;
boolean refreshLCD;
int selectedSong;
int MAX_SONGS = 25;

char song[512];

void setup(void) {
  Serial.begin(9600);

  pciSetup(selectPin);
  pciSetup(playPin);

  lcd.init();                      // initialize the lcd 
  lcd.init();
  lcd.backlight();
  btn1State = LOW;
  btn1State = LOW;

  songTitle = " Themes player! ";
  refreshLCD = true;
}

void draw() {
  if (refreshLCD) {
    String buff = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    buff += String(selectedSong) + ">" + songTitle + "                ";
    lcd.println(buff.substring(0,16));
    lcd.setCursor(0, 1);
    if(playing) {
      lcd.println("<-Sel     Stop->");
    } else {
      lcd.println("<-Sel     Play->");
    }
    refreshLCD = false;
  }
}

void loop(void) {
  if(btn1State == HIGH) {
    selectedSong++;
    if(selectedSong > MAX_SONGS) {
      selectedSong = 0;
    }
    selectSong();
    Serial.println(song);
    songTitle = getSongTitle(song);
    refreshLCD = true;
    btn1State = LOW;
    Serial.print("selectedSong: ");
    Serial.println(selectedSong);
    Serial.print("songTitle: ");
    Serial.println(songTitle);
  }
  if(btn2State == HIGH) {
    if(playing) {
      playing = false;
    } else {
      playing = true;
      play_rtttl(song);
    }
    btn2State = LOW;
    refreshLCD = true;
  }
  draw();
}

void selectSong() {
  strcpy_P(song, (char*)pgm_read_word(&(songs[selectedSong]))); // Necessary casts and dereferencing, just copy.
}

String getSongTitle(char const *p) {
  String sname = String(p);
  sname = sname.substring(0, sname.indexOf(":"));
  return sname.substring(0,14);
}

void pciSetup(byte pin) {
  pinMode(pin, INPUT_PULLUP);

  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR(PCINT0_vect){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 100) {
    btn1State = digitalRead(selectPin);
  }
  last_interrupt_time = interrupt_time;
}
ISR(PCINT2_vect){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 100) {
    btn2State = digitalRead(playPin);
  }
  last_interrupt_time = interrupt_time;
}

void play_rtttl(char const *p) {
  // Absolutely no error checking in here

  byte default_dur = 4;
  byte default_oct = 6;
  int bpm = 63;
  int num;
  long wholenote;
  long duration;
  byte note;
  byte scale;

  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)

  while(*p != ':') p++;    // ignore name
  p++;                     // skip ':'

  // get default duration
  if(*p == 'd') {
    p++; p++;              // skip "d="
    num = 0;
    while(isdigit(*p)) {
      num = (num * 10) + (*p++ - '0');
    }
    if(num > 0) default_dur = num;
    p++;                   // skip comma
  }

  Serial.print("ddur: ");
  Serial.println(default_dur, 10);

  // get default octave
  if(*p == 'o')
  {
    p++; p++;              // skip "o="
    num = *p++ - '0';
    if(num >= 3 && num <=7) default_oct = num;
    p++;                   // skip comma
  }

  Serial.print("doct: "); Serial.println(default_oct, 10);

  // get BPM
  if(*p == 'b') {
    p++; p++;              // skip "b="
    num = 0;
    while(isdigit(*p)) {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++;                   // skip colon
  }

  Serial.print("bpm: "); Serial.println(bpm, 10);

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)

  Serial.print("wn: "); Serial.println(wholenote, 10);

  // now begin note loop
  while(*p) {
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p)) {
      num = (num * 10) + (*p++ - '0');
    }
    
    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

    // now get the note
    note = 0;

    switch(*p) {
      case 'c':
        note = 1;
        break;
      case 'd':
        note = 3;
        break;
      case 'e':
        note = 5;
        break;
      case 'f':
        note = 6;
        break;
      case 'g':
        note = 8;
        break;
      case 'a':
        note = 10;
        break;
      case 'b':
        note = 12;
        break;
      case 'p':
      default:
        note = 0;
    }
    p++;

    // now, get optional '#' sharp
    if(*p == '#') {
      note++;
      p++;
    }

    // now, get optional '.' dotted note
    if(*p == '.') {
      duration += duration/2;
      p++;
    }
  
    // now, get scale
    if(isdigit(*p)) {
      scale = *p - '0';
      p++;
    } else {
      scale = default_oct;
    }

    scale += OCTAVE_OFFSET;

    if(*p == ',')
      p++;       // skip comma for next note (or we may be at the end)

    // now play the note

    if(note) {
      Serial.print("Playing: ");
      Serial.print(scale, 10); Serial.print(' ');
      Serial.print(note, 10); Serial.print(" (");
      Serial.print(notes[(scale - 4) * 12 + note], 10);
      Serial.print(") ");
      Serial.println(duration, 10);
      tone(tonePin, notes[(scale - 4) * 12 + note]);
      delay(duration);
      noTone(tonePin);
    } else {
      Serial.print("Pausing: ");
      Serial.println(duration, 10);
      delay(duration);
    }
  }
}

