/ Based on the hpsdr arduino controller code by John Melton, G0ORX/N6LYT
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

/*
*  HPSDR radio controller using Arduino
*  using step endcoders and push buttons.
*  
*  sends CAT comands over USB serial port
*
*  Uses the encoder library from https://github.com/mprograms/SimpleRotary
*  Uses the bounce2 library from https://github.com/thomasfredericks/Bounce2/wiki
*  
*  Tested using Keyes KY-040 rotary encoders
*/

#include <SimpleRotary.h>
#include <Bounce2.h>


SimpleRotary tuningEnc(4, 5, 6);
SimpleRotary gainEnc(7, 8, 9);

#define pttPin 1
Bounce pttSwitch = Bounce();

#define bandPin 2
Bounce bandSwitch = Bounce();

int encoder_a = 0;
int encoder_b = 0;
int agcGain = -200;
int rfGain = -1;
int ptt = 1;
char message[8];
int messageIndex = 0;
byte cw = 1;
byte ccw = 2;

void setup() {
  // setup band pin
  pinMode(bandPin, INPUT);
  bandSwitch.attach(bandPin);
  bandSwitch.interval(20);
  digitalWrite(bandPin, HIGH);

  // setup ptt pin
  pinMode(pttPin, INPUT);
  pttSwitch.attach(pttPin);
  pttSwitch.interval(20);
  digitalWrite(pttPin, HIGH);

  Serial.begin(57600);
}

void loop() {
  checkSerialData();
  int tEncBtnPush = tuningEnc.pushTime();
  int gEncBtnPush = gainEnc.pushTime();
  encoder_a = tEncBtnPush > 1;
  encoder_b = gEncBtnPush > 1;

  byte tEnc = tuningEnc.rotate();
  if (tEnc > 0) {
    if (encoder_a) {
      if (tEnc == cw) {
        Serial.print("ZZSU;");
      } else {
        Serial.print("ZZSD;");
      }
    } else {
      if (tEnc == cw) {
        Serial.print("ZZSB;");
      } else {
        Serial.print("ZZSA;");
      }
    }
  }

  byte gEnc = gainEnc.rotate();
  if (gEnc > 0) {
    if (encoder_b) {
      if (rfGain == -1) {
        Serial.print("ZZPC;");
      } else {
        if (gEnc == cw) {
          rfGain--;
          if (rfGain < 0) {
            rfGain = 0;
          }
        } else {
          rfGain++;
          if (rfGain > 100) {
            rfGain = 100;
          }
        }
        Serial.print("ZZPC");
        Serial.print(rfGain / 100);
        Serial.print((rfGain % 100) / 10);
        Serial.print(rfGain % 10);
        Serial.print(";");
      }
    } else {
      if (agcGain == -200) {
        Serial.print("ZZAR;");
      } else {
        if (gEnc == cw) {
          agcGain--;
          if (agcGain < 0) {
            agcGain = 0;
          }
        } else {
          agcGain++;
          if (agcGain > 100) {
            agcGain = 100;
          }
        }
        Serial.print("ZZAR");
        int gain=agcGain;
          if(gain<0) {
            Serial.print("-");
            gain=-gain;
          } else {
            Serial.print("+");
          }
        Serial.print(agcGain / 100);
        Serial.print((agcGain % 100) / 10);
        Serial.print(agcGain % 10);
        Serial.print(";");
      }
    }
  }

  if (bandSwitch.update()) {
    // note this switch has no pullup resistor
    if (bandSwitch.read() == 0) {
      Serial.print("ZZBU;");
    }
  }

  if (pttSwitch.update()) {
    if (pttSwitch.read() == 0) {
      if (ptt) {
        ptt = 0;
        Serial.print("ZZTX1;");
      }
    } else {
      if (!ptt) {
        ptt = 1;
        Serial.print("ZZTX0;");
      }
    }
  }
}

void XcheckSerialData() {
  while (Serial.available() > 0) {
    char c = Serial.read();
  }
}

void checkSerialData() {
  while (Serial.available() > 0) {
    // read the incoming byte:
    char c = Serial.read();
    if (c == ';') {
      if (strncmp(message, "ZZAR", 4) == 0 && messageIndex == 8) {
        int gain = ((message[5] - '0') * 100) +
                   ((message[6] - '0') * 10) +
                   (message[7] - '0');
        if(message[4]=='-') {
          gain =- gain;
        }
        agcGain=gain;
      } else if (strncmp(message, "ZZPC", 4) == 0 && messageIndex == 7) {
        int gain = ((message[4] - '0') * 100) +
                   ((message[5] - '0') * 10) +
                   (message[6] - '0');
        rfGain = gain;
      } else {
        // unhandled message;
      }
      messageIndex = 0;
    } else {
      message[messageIndex++] = c;
    }
  }
}
