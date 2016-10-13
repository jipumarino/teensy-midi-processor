const int ledPin = 13;

// Realtime MIDI messages
const byte MIDI_CLOCK = 0xF8;
const byte MIDI_START = 0xFA;
const byte MIDI_CONTINUE = 0xFB;
const byte MIDI_STOP = 0xFC;

const byte OCTATRACK_AUTO_CH = 10;

// LaunchKey Mini InControl Pads (including round pads)
// Row 1: 0x60 ... 0x68
// Row 2: 0x70 ... 0x78

const byte LK_INCONTROL_CH = 1;

const byte LK_FWD_TRANSPORT_PAD = 0x70;

const byte LK_LEFT_ARROW = 0x6A;
const byte LK_RIGHT_ARROW = 0x6B;
const byte LK_UP_ARROW = 0x68;
const byte LK_DOWN_ARROW = 0x69;

const byte LK_RED = 0x05;
const byte LK_GREEN = 0x30;
const byte LK_YELLOW = 0x13;
const byte LK_OFF = 0x00;

bool fwdTransportEnabled = false;

int resetLKShortcutCount = 0;

void setup() {
  pinMode(ledPin, OUTPUT);
  usbMIDI.setHandleNoteOff(onNoteOff);
  usbMIDI.setHandleNoteOn(onNoteOn);
  usbMIDI.setHandleRealTimeSystem(onRealTimeSystem);
  usbMIDI.setHandleControlChange(onControlChange);
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void loop() {
  usbMIDI.read();
}

void onNoteOn(byte channel, byte note, byte velocity) {
  if ( channel == LK_INCONTROL_CH ) {
    switch(note) {
    case LK_FWD_TRANSPORT_PAD:
      toggleFwdTransport();
      break;
    }
  } else {
    usbMIDI.sendNoteOn(note, velocity, channel);
  }
}

void onNoteOff(byte channel, byte note, byte velocity) {
  if ( channel != LK_INCONTROL_CH ) {
    usbMIDI.sendNoteOff(note, velocity, channel);
  }
}

void onControlChange(byte channel, byte control, byte value) {
  if ( channel == LK_INCONTROL_CH ) {
    switch( control ) {
    case LK_LEFT_ARROW:
    case LK_RIGHT_ARROW:
      if ( value == 0 ) {
        resetLKShortcut(-1);
      } else {
        resetLKShortcut(+1);
      }
      break;        
    }
  } else {
    usbMIDI.sendControlChange(control, value, channel);
  }
}

void resetLKShortcut(int countChange) {
  resetLKShortcutCount += countChange;
  if ( resetLKShortcutCount >= 2) {
    resetLK();
  }
}

void resetLK() {
  enableLKInControl();
  setFwdTransport(true);
}

void enableLKInControl() {
  usbMIDI.sendNoteOn(0x0C, 0x7F, LK_INCONTROL_CH);
}

void setLKInControlLed(byte pad, byte color) {
  usbMIDI.sendNoteOn(pad, color, LK_INCONTROL_CH);
}

void toggleFwdTransport() {
  if ( fwdTransportEnabled ) {
    setFwdTransport(false);
  } else {
    setFwdTransport(true);
  }
}

void setFwdTransport(bool value) {
  if ( value ) {
    setLKInControlLed(LK_FWD_TRANSPORT_PAD, LK_GREEN);
  } else {
    setLKInControlLed(LK_FWD_TRANSPORT_PAD, LK_RED);
  }

  fwdTransportEnabled = value;
}

void onRealTimeSystem(byte msg) {
  if ( ( fwdTransportEnabled && ( msg == MIDI_START || msg == MIDI_STOP || msg == MIDI_CONTINUE ) ) || msg == MIDI_CLOCK ) {
    sendRealTime(msg);
  }
}

void sendRealTime(byte msg) {
  // USB MIDI message:
  // 4 bits Cable Number 0:                  0x0
  // 4 bits Code Index Number "Single Byte": 0xF
  // 8 bits Status, msg byte received
  // 16 bits zero padding
  // All together: 0x0F[msg]0000
  // Reverse for endianness: 0x0000[msg]0F
  // Discard leading zeroes: 0x[msg]0F
  usb_midi_write_packed(((msg & 0xFF) << 8) | 0x0F);
}
