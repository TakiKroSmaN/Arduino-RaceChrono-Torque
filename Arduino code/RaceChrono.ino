#include <String.h>

// #define DEBUG  // zakomentować w gotowym projekcie
// #define RandomDebug  // randomuje wartości w funkcji get sensor
#define BT Serial1


// Various constants.
const String ATE = "ATE"; // Echo off/on
const String ATI = "ATI"; // Version id
const String ATZ = "ATZ"; // Reset
const String ATS = "ATS"; // Set protocol X
const String ATH = "ATH"; // Headers off / on
const String ATL = "ATL"; // Linefeeds off/on
const String ATM = "ATM"; // Memory off/on
const String GETDEFINITIONS = "GETDEFINITIONS"; // Get sensor definitions
const String GETCONFIGURATION = "GETCONFIGURATION"; // Get config of app (hide car sensors, devices sensors, etc)
const String GETSENSORS = "G"; // Get sensor values, one shot.
const String SETSENSOR = "S"; // Set a sensor value
const String PROMPT = ">";
const String CANBUS = "6"; // canbus 500k 11 bit protocol id for elm.
const String ATDPN = "ATDPN";
const String ATDESC = "AT@1";
const String ATAT = "ATAT";
const String LF = "\n";
const String VERSION = "Torque Protocol Interface v0.0.1"; // Don't change this - it's used by Torque so it knows what interface it is connected to
const String VERSION_DESC = "Tomasz Kroczak Racing :)";
const String OK = "OK";
const String ANALOG = "a";
const String DIGITAL = "d";
const String IS_INPUT = "i";
const String IS_OUTPUT = "o";

String fromTorque = "";

const String INI0 = "0100";
const String INI2 = "0120";
const String INI4 = "0140";
const String INI6 = "0160";
const String INI8 = "0180";
const String RPM = "010C";
const String PREDKOSC = "010D";
const String THROTTLE = "0111";



// *********************************************************************
//                definicje pinów
// *********************************************************************
#define SpeedSensor            2
#define ShaftSensor            3
#define clutch                 7
#define brakePin               8
#define VTECPin               13
#define ThrottlePin           A7

// *********************************************************************
//                aliasowanie stałych wartości
// *********************************************************************
#define iloscPomiarow 10

// współczynniki po podzieleniu obrotów przez prędkość
#define tolgear    5
#define wspgear1 163
#define wspgear2 100
#define wspgear3  71
#define wspgear4  53
#define wspgear5  43


// *********************************************************************
//                        globalne zmienne
// *********************************************************************
boolean brake;
volatile unsigned long speed = 0, rpm = 0;
byte gear = 0, throttlePos = 0, minThrottlePos = 255;

float gearRatio[6] = {
  5.5,        // FD
  3.23,       // 1 bieg
  2.105,      // 2 bieg
  1.458,      // 3 bieg
  1.034,      // 4 bieg
  0.848,      // 5 bieg
};





// *********************************************************************
//                definicje czujników
// *********************************************************************
const int SENSORSSIZE = 9 * 6; // each line is 9 attributes, and we have 3 lines.
const String sensors[SENSORSSIZE] = {
  "0", ANALOG,   IS_INPUT,    "0",   "VSS",      "Speed",       "km/h",    "0", "200",
  "1", ANALOG,   IS_INPUT,    "0",   "Revs",     "Revs",        "rpm",     "0", "9000",
  "2", ANALOG,   IS_INPUT,    "0",   "Throttle", "TPS",         "%",       "0", "100",
  "3", ANALOG,   IS_INPUT,    "0",   "Gear",     "GearPos",     " ",       "0", "6",
  "8", DIGITAL,  IS_INPUT,    "0",   "Brake",    "Brake",       "bit",     "0", "1",
  "13", DIGITAL, IS_INPUT,    "0",   "VTEC",     "VTEC",        "bit",     "0", "1"
};
/**
  Configuration directives for the app to hide various things. Comma separated. Remove to enable visibility in Torque
  - handy if your project isn't car related or you want to make sensor selections relatively easy.

  Supported types:
  NO_CAR_SENSORS  - hide any car related sensors
  NO_DEVICE_SENSORS - hide any device (phone) sensors

*/
const String CONFIGURATION = "";


// *********************************************************************
//                                SETUP
// *********************************************************************
void setup() {
  // Init the pins
  initSensors();

#ifdef DEBUG
  Serial.begin(115200);
  delay(1500);
  Serial.println("Setup Start");
#endif

  BT.begin(115200);  // BT.connenct

  pinMode(SpeedSensor, INPUT);
  pinMode(ShaftSensor, INPUT);
  pinMode(clutch, INPUT);
  pinMode(brakePin, INPUT);
  pinMode(VTECPin, INPUT);
  pinMode(ThrottlePin, INPUT);

  attachInterrupt(digitalPinToInterrupt(SpeedSensor), speedRead, RISING);             //call interrupt #2 when the tach input goes high
  attachInterrupt(digitalPinToInterrupt(ShaftSensor), rpmRead, RISING);             //call interrupt #3 when the tach input goes high
}




// *********************************************************************
//                                LOOP
// *********************************************************************
void loop() {

  if (BT.available()) {
    char c = BT.read();
    if ((c == '\n' || c == '\r') && fromTorque.length() > 0) {
      fromTorque.toUpperCase();
      processCommand(fromTorque);
      fromTorque = "";
    } else if (c != ' ' && c != '\n' && c != '\r') {
      // Ignore spaces.
      fromTorque += c;
    }
  }



  //  wrzucNaLcd();
}

/**
  Parse the commands sent from Torque
*/
void processCommand(String command) {
#ifdef DEBUG
  // Debug - see what torque is sending on your serial monitor
  Serial.println(command);
#endif

  // Simple command processing from the app to the arduino..
  if (command.equals(ATZ)) {
    initSensors(); // reset the pins
    BT.print(VERSION);
    BT.print(LF);
    BT.print(OK);
  } else if (command.startsWith(ATE)) {
    BT.print(OK);
  } else if (command.startsWith(ATI)) {
    BT.print(VERSION);
    BT.print(LF);
    BT.print(OK);
  } else if (command.startsWith(ATDESC)) {
    BT.print(VERSION_DESC);
    BT.print(LF);
    BT.print(OK);
  } else if (command.startsWith(ATL)) {
    BT.print(OK);
  } else if (command.startsWith(ATAT)) {
    BT.print(OK);
  } else if (command.startsWith(ATH)) {
    BT.print(OK);
  } else if (command.startsWith(ATM)) {
    BT.print(OK);
  } else if (command.startsWith(ATS)) {
    // Set protocol
    BT.print(OK);
  } else if (command.startsWith(ATDPN)) {
    BT.print(CANBUS);
  } else if (command.startsWith(GETDEFINITIONS)) {
    showSensorDefinitions();
  } else if (command.startsWith(GETSENSORS)) {
    getSensorValues();
  } else if (command.startsWith(GETCONFIGURATION)) {
    getConfiguration();
  } else if (command.startsWith(SETSENSOR)) {
    setSensorValue(command);
  }
  else if (command.startsWith(INI0)) {
    BT.print("41 00 98 3B A0 13");
  }
  else if (command.startsWith(INI2)) {
    BT.print("41 20 B8 01 A0 01");
  }
  else if (command.startsWith(INI4)) {
    BT.print("41 40 CC D2 00 01");
  }
  else if (command.startsWith(INI6)) {
    BT.print("41 60 02 80 03 01");
  }
  else if (command.startsWith(INI8)) {
    BT.print("41 80 00 20 00 00");
  }
  else if (command.startsWith(RPM)) {
#ifdef DEBUG
    //  rpm = random(0, 9000);
    Serial.print("RPM: ");
    Serial.print(String(rpm, HEX) + " : ");
    Serial.println(rpm);
#endif
    BT.print(RPM);
    BT.print("41 0C ");
    BT.print(IntNaChar(rpm * 4, 'H'), HEX);
    BT.print(" ");
    BT.print(IntNaChar(rpm * 4, 'L'), HEX);
  }
  else if (command.startsWith(PREDKOSC)) {
#ifdef DEBUG
    //   speed = random(0, 200);
    Serial.print("VSS: ");
    Serial.println(speed);
#endif
    BT.print(PREDKOSC);
    BT.print("41 0D " + String(speed, HEX));
  }
  else if (command.startsWith(THROTTLE)) {
    readThrottleBrake();
#ifdef DEBUG
    // throttlePos = random(0, 100);
    Serial.print("TPS: ");
    Serial.println(throttlePos);
#endif
    BT.print(THROTTLE);
    BT.print("41 11 " + String((throttlePos * 255) / 100, HEX));
  }

  BT.print(LF);
  BT.print(PROMPT);


}

/**
  List all the sensors to the app
*/
void showSensorDefinitions() {
  int id = 0;
  for (int i = 0; i < SENSORSSIZE / 9; i++) {
    for (int j = 0; j < 9; j++) {
      id = (i * 9) + j;
      BT.print(sensors[id]);

      if (id + 1 < SENSORSSIZE) {
        BT.print(',');
      }
    }
    BT.print(LF);
  }
}

/**
  Dump sensor information for input sensors.

  Format to Torque is id:type:value
*/
void getSensorValues() {
  noInterrupts();
#ifdef RandomDebug
  speed = random(0, 200);
  rpm = random(0, 9000);
  gear = random(0, 5);
  throttlePos = random(0, 100);
#endif

  readThrottleBrake();
  gearRead();

  for (int i = 0; i < SENSORSSIZE / 9; i++) {
    int group = i * 9;
    int id = sensors[group].toInt();
    String type = sensors[group + 1];
    boolean isOutput = sensors[group + 2].equals(IS_OUTPUT);

#ifdef DEBUG
    Serial.println(SENSORSSIZE);
    Serial.println(group);
    Serial.println(id);
    Serial.println("**********************");
#endif
    // ****************************
    // Speed
    // ****************************
    if (!isOutput) {
      if (id == 0) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          //    Serial.print("speed = ");
          //    Serial.println(speed);
          BT.print(speed);
        } else if (type.equals(DIGITAL)) {
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

    // ****************************
    // RPM
    // ****************************
    if (!isOutput) {
      if (id == 1) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          //     Serial.print("rpm = ");
          //     Serial.println(rpm);
          BT.print(rpm);
        } else if (type.equals(DIGITAL)) {
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

    // ****************************
    // ThrottlePos
    // ****************************
    if (!isOutput) {
      if (id == 2) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          //     Serial.print("throttlePos = ");
          //     Serial.println(throttlePos);
          BT.print(throttlePos);
        } else if (type.equals(DIGITAL)) {
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

    // ****************************
    // Gear
    // ****************************
    if (!isOutput) {
      if (id == 3) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          //   Serial.print("gear = ");
          //   Serial.println(gear);
          BT.print(gear);
        } else if (type.equals(DIGITAL)) {
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

    // ****************************
    // Brake
    // ****************************
    if (!isOutput) {
      if (id == 8) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          BT.print(analogRead(id));
        } else if (type.equals(DIGITAL)) {
          // Serial.print("Brake = ");
          // Serial.println(digitalRead(id));
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

    // ****************************
    // VTEC
    // ****************************
    if (!isOutput) {
      if (id == 13) {
        BT.print(id);
        BT.print(":");
        BT.print(type);
        BT.print(":");
        if (type.equals(ANALOG)) {
          BT.print(analogRead(id));
        } else if (type.equals(DIGITAL)) {
          //   Serial.print("VTEC = ");
          //   Serial.println(digitalRead(id));
          BT.print(digitalRead(id));
        }
        BT.print('\n');
      }
    }

  }
  interrupts();
}

/**
  Sets a sensors value
*/
void setSensorValue(String command) {
  int index = command.indexOf(":");
  int id = command.substring(1, index).toInt();
  int value = command.substring(index + 1, command.length()).toInt();

  for (int i = 0; i < SENSORSSIZE / 9; i++) {
    int group = i * 9;
    int sid = sensors[group].toInt();
    boolean isOutput = sensors[group + 2].equals(IS_OUTPUT);
    if (isOutput) {

      if (sid == id) {

        String type = sensors[group + 1];
        if (type.equals(ANALOG)) {
          analogWrite(sid, constrain(value, 0, 255));
        } else if (type.equals(DIGITAL)) {
          digitalWrite(sid, value > 0 ? HIGH : LOW);
        }
        break;
      }
    }
  }
}

/**
  Init the sensor definitions (input/output, default output states, etc)
*/
void initSensors() {
  for (int i = 0; i < SENSORSSIZE / 9; i++) {
    int group = i * 9;
    int id = sensors[group].toInt();
    String type = sensors[group + 1];
    boolean isOutput = sensors[group + 2].equals(IS_OUTPUT);
    int defaultValue = sensors[group + 3].toInt();


    if (isOutput) {
      if (type.equals(ANALOG)) {
        pinMode(id, OUTPUT);
        analogWrite(id, constrain(defaultValue, 0, 255));
      } else if (type.equals(DIGITAL)) {
        pinMode(id, OUTPUT);
        digitalWrite(id, defaultValue > 0 ? HIGH : LOW);
      }
    }
  }
}

void getConfiguration() {
  BT.print(CONFIGURATION);
}












// *********************************************************************
//                       odczyt obrotów silnika
// *********************************************************************
void rpmRead() {
  static boolean impulsRpmCounter;
  static byte licznikDoUsrednienia;
  static unsigned int rpmSrednia[iloscPomiarow];
  static unsigned long rpmCalculation;

  noInterrupts();
  if (impulsRpmCounter == 0) {
    rpmCalculation = micros();
  }
  else if (impulsRpmCounter == 1) {
    rpmCalculation = micros() - rpmCalculation;
    rpmSrednia[licznikDoUsrednienia] = 30000000 / rpmCalculation;
    licznikDoUsrednienia++;
  }
  rpm = 0;
  for (byte i = 0; i < iloscPomiarow; i++) {
    rpm += rpmSrednia[i];
  }
  rpm /= iloscPomiarow;

  impulsRpmCounter = !impulsRpmCounter;
  if (licznikDoUsrednienia >= iloscPomiarow) licznikDoUsrednienia = 0;
  interrupts();
}



// *********************************************************************
//                          odczyt prędkości
// *********************************************************************
void speedRead() {
  static boolean impulsSpeedCounter;
  static byte licznikDoUsrednienia;
  static unsigned int speedSrednia[iloscPomiarow];
  static unsigned long speedCalculation;

  noInterrupts();
  if (impulsSpeedCounter == 0) {
    speedCalculation = micros();
  }
  else if (impulsSpeedCounter == 1) {
    speedCalculation = micros() - speedCalculation;
    speedSrednia[licznikDoUsrednienia] = 1420000 / speedCalculation;
    licznikDoUsrednienia++;
  }
  speed = 0;
  for (byte i = 0; i < iloscPomiarow; i++) {
    speed += speedSrednia[i];
  }

  speed /= iloscPomiarow;
  if (speed < 5) {
    speed = 0;
  }

  impulsSpeedCounter = !impulsSpeedCounter;
  if (licznikDoUsrednienia >= iloscPomiarow) licznikDoUsrednienia = 0;
  interrupts();
}


// *********************************************************************
//        odczyt aktualnego biegu na podstawie rpm i prędkości
// *********************************************************************
void gearRead() {
  // wynaczenie na którym biegu jedzie auto
  gear = rpm / speed;

#ifdef DEBUG
  Serial.print ("rpm / speed = ");
  Serial.println (gear);
#endif

  if      (gear >= wspgear1 - tolgear && gear <= wspgear1 + tolgear) gear = 1;
  else if (gear >= wspgear2 - tolgear && gear <= wspgear2 + tolgear) gear = 2;
  else if (gear >= wspgear3 - tolgear && gear <= wspgear3 + tolgear) gear = 3;
  else if (gear >= wspgear4 - tolgear && gear <= wspgear4 + tolgear) gear = 4;
  else if (gear >= wspgear5 - tolgear && gear <= wspgear5 + tolgear) gear = 5;
  else gear = 0;

#ifdef DEBUG
  Serial.print ("gear = ");
  Serial.println (gear);
#endif
}

// *********************************************************************
//                 Odczyt pozycji przepustnicy
// *********************************************************************
void readThrottleBrake() {
  if (analogRead(ThrottlePin) < minThrottlePos) {
    minThrottlePos = analogRead(ThrottlePin) - 3;
  }
  throttlePos = map(analogRead(ThrottlePin), minThrottlePos, 1023, 0, 100);

  brake = digitalRead(brakePin);
#ifdef DEBUG
  Serial.print ("AnalogRead Throttle = ");  Serial.println (analogRead(ThrottlePin));
  Serial.print ("throttlePos = ");  Serial.print (throttlePos);  Serial.println ("%");
  Serial.print ("brake = ");  Serial.println (brake);
#endif
}



// *********************************************************************
//                  Konversja zmiennej na High i Low bajt
// *********************************************************************

uint8_t IntNaChar(uint16_t liczba, uint8_t ktory_bajt) {
  if (ktory_bajt == 'L') {
    return (liczba & 0xFF); //Zwracamy młodszy bajt
  } else if (ktory_bajt == 'H') {
    return (liczba >> 8); //Zwracamy starszy bajt
  }
  return 1;
}
