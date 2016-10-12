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
const byte LK_MOTHER_PORTAMENTO_PAD = 0x71;
const byte LK_TIMELINE_RT_PAD = 0x72;
const byte LK_SHIFT_CC_PAD = 0x73;
const byte LK_PLAY_PAUSE_PAD = 0x76;
const byte LK_STOP_PAD = 0x77;

const byte LK_LEFT_ARROW = 0x6A;
const byte LK_RIGHT_ARROW = 0x6B;
const byte LK_UP_ARROW = 0x68;
const byte LK_DOWN_ARROW = 0x69;

const byte LK_RED = 0x05;
const byte LK_GREEN = 0x30;
const byte LK_YELLOW = 0x13;
const byte LK_OFF = 0x00;

bool fwdTransportEnabled = false;
bool motherPortamentoEnabled = true;
bool timeLineRTEnabled = true;
bool shiftCC = false;

int resetLKShortcutCount = 0;

enum TransportState {
  stopped,
  paused,
  playing
} transportState = stopped;

enum TransportButton {
  playPause,
  stop
};

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
  if(channel == LK_INCONTROL_CH) {
    switch(note) {
    case LK_TIMELINE_RT_PAD:
      toggleTimeLineRT();
      break;
    case LK_SHIFT_CC_PAD:
      toggleShiftCC();
      break;
    case LK_FWD_TRANSPORT_PAD:
      toggleFwdTransport();
      break;
    case LK_MOTHER_PORTAMENTO_PAD:
      toggleMotherPortamento();
      break;
    case LK_PLAY_PAUSE_PAD:
      changeTransportState(playPause);
      break;
    case LK_STOP_PAD:
      changeTransportState(stop);
      break;
    case 0x68:
      usbMIDI.sendControlChange(108, 127, 11);
      break;
    case 0x78:
      usbMIDI.sendControlChange(109, 127, 11);
      break;
    }
  } else if(channel != OCTATRACK_AUTO_CH || ( note < 33 || note > 35 )) {
    usbMIDI.sendNoteOn(note, velocity, channel);
  }
}

void onNoteOff(byte channel, byte note, byte velocity) {
  if((channel != LK_INCONTROL_CH) && (channel != OCTATRACK_AUTO_CH || ( note < 33 || note > 35 ))) {
    usbMIDI.sendNoteOff(note, velocity, channel);
  }
}

void onControlChange(byte channel, byte control, byte value) {
  if(control == LK_UP_ARROW || control == LK_DOWN_ARROW || control == LK_LEFT_ARROW || control == LK_RIGHT_ARROW) {
    if(value == 0) {
      resetLKShortcut(-1);
    } else {
      resetLKShortcut(+1);
    }
  }

  if(channel != LK_INCONTROL_CH) {
    usbMIDI.sendControlChange(control, value, channel);
  }

  if(control >= 21 && control <= 28) {
    if(shiftCC) {
      control += 10;
    }
    usbMIDI.sendControlChange(control, value, 11);
  }

  if((control == 108 || control == 109) && value == 127) {
    usbMIDI.sendControlChange(control, value, 11);
  }

}

void resetLKShortcut(int countChange) {
  resetLKShortcutCount += countChange;
  if(resetLKShortcutCount >= 4) {
    resetLK();
  }
}

void resetLK() {
  enableLKInControl();
  setTimeLineRT(true);
  setShiftCC(false);
  setFwdTransport(true);
  setMotherPortamento(false);
  changeTransportState(stop);
}

void toggleTimeLineRT() {
  if(timeLineRTEnabled) {
    setTimeLineRT(false);
  } else {
    setTimeLineRT(true);
  }
}

void setTimeLineRT(bool value) {
  static int sysexLength = 83;
  static uint8_t onSysex[] = {
    0xF0, 0x00, 0x01, 0x73, 0x7E, // header
    0x00, 0x06, 0x00, 0x00, 0x00, 0x0F, 0x14, // product (body start)
    0x00, 0x01, // tx id
    0x40, 0x25, // command SetMIDIPortFilter
    0x00, 0x3F, // data len = 63, before checksum
    0x01, // command version number
    0x00, 0x39, // port ID = H9
    0x02, // filter ID = output
    0x08, // 8 CC filters
    0x00, 0x40, // (bit 5: set if port should filter MIDI realtime events (0xF8-0xFD)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // midi chan filter bitmap
    0x00, 0x00, 0x00, 0x00, 0x00, // noop CC filters
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x2E, // checksum = (0x80 - sum_all_body_bytes % 0x80)
    0xF7 // footer
  };
  static uint8_t offSysex[] = {
    0xF0, 0x00, 0x01, 0x73, 0x7E, // header
    0x00, 0x06, 0x00, 0x00, 0x00, 0x0F, 0x14, // product (body start)
    0x00, 0x01, // tx id
    0x40, 0x25, // command SetMidiPortFilter
    0x00, 0x3F, // data len = 63, before cheksum
    0x01, // command version number
    0x00, 0x39, // port ID = H9
    0x02, // filter ID = output
    0x08, // 8 CC filters
    0x00, 0x60, // (bit 5: set if port should filter MIDI realtime events (0xF8-0xFD)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // midi chan filter bitmap
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, // checksum = (0x80 - sum_all_body_bytes % 0x80)
    0xF7 // footer
  };

  if(value) {
    usbMIDI.sendSysEx(sysexLength, onSysex);
    setLKInControlLed(LK_TIMELINE_RT_PAD, LK_GREEN);
  } else {
    usbMIDI.sendSysEx(sysexLength, offSysex);
    setLKInControlLed(LK_TIMELINE_RT_PAD, LK_RED);
  }

  timeLineRTEnabled = value;
}

void enableLKInControl() {
  usbMIDI.sendNoteOn(0x0C, 0x7F, LK_INCONTROL_CH);
}

void setLKInControlLed(byte pad, byte color) {
  usbMIDI.sendNoteOn(pad, color, LK_INCONTROL_CH);
}

void changeTransportState(TransportButton button) {
  if(button == playPause) {
    switch(transportState) {
    case stopped:
      sendRealTime(MIDI_START);
      setLKInControlLed(LK_PLAY_PAUSE_PAD, LK_GREEN);
      setLKInControlLed(LK_STOP_PAD, LK_OFF);
      transportState = playing;
      break;
    case playing:
      sendRealTime(MIDI_STOP);
      setLKInControlLed(LK_PLAY_PAUSE_PAD, LK_YELLOW);
      setLKInControlLed(LK_STOP_PAD, LK_OFF);
      transportState = paused;
      break;
    case paused:
      sendRealTime(MIDI_CONTINUE);
      setLKInControlLed(LK_PLAY_PAUSE_PAD, LK_GREEN);
      setLKInControlLed(LK_STOP_PAD, LK_OFF);
      transportState = playing;
      break;
    }
  } else if(button == stop) {
    sendRealTime(MIDI_STOP);
    setLKInControlLed(LK_PLAY_PAUSE_PAD, LK_OFF);
    setLKInControlLed(LK_STOP_PAD, LK_RED);
    transportState = stopped;
  }
}

void toggleMotherPortamento() {
  if(motherPortamentoEnabled) {
    setMotherPortamento(false);
  } else {
    setMotherPortamento(true);
  }
}

void setMotherPortamento(bool value) {
  static int sysexLength = 83;
  static uint8_t onSysex[] = {
    0xF0, 0x00, 0x01, 0x73, 0x7E,
    0x00, 0x06, 0x00, 0x00, 0x00, 0x0F, 0x14,
    0x00, 0x01,
    0x40, 0x25,
    0x00, 0x3F,
    0x01,
    0x00, 0x03,
    0x02,
    0x08,
    0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x5F,
    0xF7
  };
  static uint8_t offSysex[] = {
    0xF0, 0x00, 0x01, 0x73, 0x7E,
    0x00, 0x06, 0x00, 0x00, 0x00, 0x0F, 0x14,
    0x00, 0x01,
    0x40, 0x25,
    0x00, 0x3F,
    0x01,
    0x00, 0x03,
    0x02,
    0x08,
    0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x05,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x5D,
    0xF7
  };

  if(value) {
    usbMIDI.sendSysEx(sysexLength, onSysex);
    setLKInControlLed(LK_MOTHER_PORTAMENTO_PAD, LK_GREEN);
  } else {
    usbMIDI.sendSysEx(sysexLength, offSysex);
    setLKInControlLed(LK_MOTHER_PORTAMENTO_PAD, LK_RED);
  }

  motherPortamentoEnabled = value;
}

void toggleFwdTransport() {
  if(fwdTransportEnabled) {
    setFwdTransport(false);
  } else {
    setFwdTransport(true);
  }
}

void setFwdTransport(bool value) {
  if(value) {
    setLKInControlLed(LK_FWD_TRANSPORT_PAD, LK_GREEN);
  } else {
    setLKInControlLed(LK_FWD_TRANSPORT_PAD, LK_RED);
  }

  fwdTransportEnabled = value;
}

void toggleShiftCC() {
  if(shiftCC) {
    setShiftCC(false);
  } else {
    setShiftCC(true);
  }
}

void setShiftCC(bool value) {
  if(value) {
    setLKInControlLed(LK_SHIFT_CC_PAD, LK_GREEN);
  } else {
    setLKInControlLed(LK_SHIFT_CC_PAD, LK_RED);
  }

  shiftCC = value;
}

void onRealTimeSystem(byte msg) {
  if(( fwdTransportEnabled && ( msg == MIDI_START || msg == MIDI_STOP || msg == MIDI_CONTINUE )) || msg == MIDI_CLOCK) {
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
