#include <MIDI.h>
#include <SPI.h>

#define NP_SEL1 A0  // Note priority is set by pins A0 and A2
#define NP_SEL2 A2  

#define GATE  2
#define CLOCK 4
#define DAC1  8 
#define RESET 10 // Pin for PW Reset

#define NUM_NOTES 88
#define ORDER_SIZE 20

#define NOTE_SF 47.069f // This value can be tuned if CV output isn't exactly 1V/octave

MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  pinMode(NP_SEL1, INPUT_PULLUP);
  pinMode(NP_SEL2, INPUT_PULLUP);
  
  pinMode(GATE, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DAC1, OUTPUT);
  pinMode(RESET, OUTPUT);
  digitalWrite(GATE, LOW);
  digitalWrite(CLOCK, LOW);
  digitalWrite(DAC1, HIGH);
  digitalWrite(RESET, LOW);

  SPI.begin();
  MIDI.begin(1);  // Recive MIDI msg only on Channel 1
}

bool notes[NUM_NOTES] = {false}; 
int noteOrder[ORDER_SIZE] = {0}, orderIndx = 0;
unsigned long trigTimer = 0;

void loop()
{
  int type, noteMsg, channel, vel, d2;
  static unsigned long clock_timer = 0, clock_timeout = 0;
  static unsigned int clock_count = 0;
  bool S1, S2;

  if ((clock_timer > 0) && (millis() - clock_timer > 20)) { 
    digitalWrite(CLOCK, LOW); // Set clock pulse low after 20 msec 
    clock_timer = 0;  
  }

  delay(5);
  
  if (MIDI.read()) {              
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn: 
   noteMsg = MIDI.getData1();
        digitalWrite(GATE, HIGH);

      if ((noteMsg < 0) || (noteMsg >= NUM_NOTES)) {
          break; // Only 88 notes of keyboard are supported
        }

        case midi::NoteOff:

        noteMsg = MIDI.getData1();

        if (type == midi::NoteOn) {
          notes[noteMsg] = true;
        } else {
          notes[noteMsg] = false;
           digitalWrite(GATE, LOW);
          
        }

        // Pins NP_SEL1 and NP_SEL2 indicate note priority
        S1 = digitalRead(NP_SEL1);
        S2 = digitalRead(NP_SEL2);

        if (S1 && S2) { // Highest note priority (delete due to only last note use)
          break;
        }
        else if (!S1 && S2) { // Lowest note priority (delete due to only last note use)
          break;
        }
        else { // Last note priority
           if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
            orderIndx = (orderIndx + 1) % ORDER_SIZE;
            noteOrder[orderIndx] = noteMsg;                 
          }
          commandLastNote();         
        }
        break;

      case midi::ControlChange: 

        d2 = MIDI.getData2(); // From 0 to 127

        setVoltage(DAC1, 1, 1, d2<<5);  // DAC2, channel 1, gain = 2X
        break;
        
      case midi::Clock:
        if (millis() > clock_timeout + 300) clock_count = 0; // Prevents Clock from starting in between quarter notes after clock is restarted!
        clock_timeout = millis();
        
        if (clock_count == 0) {
          digitalWrite(CLOCK, HIGH); // Start clock pulse
          clock_timer = millis();    
        }
        clock_count++;
        if (clock_count == 24) {  // MIDI timing clock sends 24 pulses per quarter note.  Sent pulse only once every 24 pulses
          clock_count = 0;  
        }
        break;
        
      case midi::ActiveSensing: 
        break;

      case midi::Stop:
        // When a Stop message is received, generate RESET
        generateRESET();
        break;
        
      default:
        // Handle other MIDI message types if needed
        break;
      delay(1);    }

  }
}

void commandLastNote()
{
  int noteIndx;
  
  for (int i = 0; i < ORDER_SIZE; i++) {
    noteIndx = noteOrder[mod(orderIndx - i, ORDER_SIZE)];
    if (noteIndx >= 0 && noteIndx < NUM_NOTES && notes[noteIndx]) {
      commandNote(noteIndx);
      return;
    }
  }
  digitalWrite(GATE, LOW);  // All notes are off
}

void setVoltage(int dacpin, bool channel, bool gain, unsigned int mV)
{
  unsigned int command = channel ? 0x9000 : 0x1000;

  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin, LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(dacpin, HIGH);
  SPI.endTransaction();
}

void generateRESET() {
  digitalWrite(RESET, HIGH); // Set RESET pin to HIGH
  delay(30); // PWM pulse duration (30 milliseconds)
  digitalWrite(RESET, LOW); // Set RESET pin back to LOW after desired pulse duration
}

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

void commandNote(int noteMsg) {
  digitalWrite(GATE, HIGH);
  
  unsigned int mV = (unsigned int) ((float) noteMsg * NOTE_SF + 0.5); 
  setVoltage(DAC1, 0, 1, mV);  // DAC1, channel 0, gain = 2X

}
