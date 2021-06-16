#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================ machine states ================

#define stateA0 300
#define stateA1 301
#define stateA2 302
#define stateB0 200
#define stateB1 201
#define stateB2 202
#define stateC0 100


// ================ some unit properties ================

// Pressure
const float bar = 1/101.325; //Pa/bar
const float Pa = 1000; //Pa/kPa
const float MPa = 1/1000; // MPa/Pa
const float kPa = 1; //kPa/kPa

// Voltage
const float mV = 1;
const float V = 1/1000; // mV/V

// Time
const float s = 1000; // ms/s
const float ms = 1;

//  ================ device properties (datasheet) ================

// pressure sensor
const float sensor_pressure_range [2] = {20*kPa, 400*kPa};
const float sensor_pressure_responseTime = 1*ms;
const float sensor_pressure_warmUpTime = 20*ms;
const float sensor_pressure_sensitivity = 12.1*mV/kPa; //mv/kPa
const float sensor_pressure_errorMax = 5.5*kPa; // +/- 5.5

// display
const int display_nCols = 20;
const int display_nRows = 4;

// ================ devices setup ================

// pressure sensor
int sensor_pressure_pin = A0;

// temperature sensor
#define SENSOR_BUS_PIN 12
OneWire oneWire(SENSOR_BUS_PIN);
DallasTemperature sensors(&oneWire);

// IO buttons
int button_push_pin = 5;
int button_switch_pin = 4;

const int debounce_timer = 100; // 1/10 of second
int debounce_time = 0; // 1/10 of second
bool debounce_set = 0;
bool button_push_toggle = 0;



// the keypad
I2CKeyPad keyPad(0x20);
uint32_t start, stop;
uint32_t lastKeyPressed = 0;

// LCD
LiquidCrystal_I2C lcd(0x27,display_nCols,display_nRows);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// ================ Script variables ================

// readings and corrections
float sensor_pressure_reading;
float sensor_pressure_voltage;
float sensor_supply_voltage = 4960;
float sensor_nominal_voltage = 5100;
float sensor_ampfactor;
float loop_pressure = millis();
float loop_display = millis();
float loop_temperature = millis();
float loop_serial = millis();
float dt_serial = 5*s;
float sensor_temperature_reading;

// display variables
float display_values_average_len = 5;
float display_values_average = 0;
float display_units_pressure = kPa; // so that you can express the readings in any other way
float display_values_pressure = 0;

// machine state
bool change_state = 0;
int state = 0;
int keyPadState = 300;
uint8_t idx = 17;

// ================ ================ ================
// ================       SETUP      ================
// ================ ================ ================

void setup() {


  pinMode(sensor_pressure_pin, INPUT);
  pinMode(button_push_pin, INPUT);
  pinMode(button_switch_pin, INPUT);

  // initialize the LCD
  lcd.init();



  // Turn on the blacklight and print a message..
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("WatAJet:WJmonitor A0");

  lcd.setCursor(0, 1);
  lcd.print("====================");

  Wire.begin();
    Wire.setClock(400000);
  keyPad.begin();

  Serial.begin(115200);
   Serial.println("DS18B20 test");
   sensors.begin();


}

// ================ ================ ================
// ================       LOOP       ================
// ================ ================ ================

void loop(){

  uint32_t now = millis();

    //char keys[] = "123A456B789C*0#DNF"; // IF keypad pins are connected 1-1 with I2C pins
    char keys[] = "DCBA#9630852*741NF";  // IF keypad pins are connected 1-8 with I2C pins
    // N = Nokey, F = Fail

    if (now - lastKeyPressed >= 100)
    {
      lastKeyPressed = now;
      start = micros();
      uint8_t idx2 = keyPad.getKey();
      stop = micros();

      if(idx2<16)
      {
        idx = idx2;
        lcd.setCursor(18, 0);
        lcd.print(keys[idx]);
        keyPadState = 100*idx + state; // 0 = D, 100 = C, 200 = B, 300 = A

      }


    }


  if(digitalRead(button_push_pin))
  {
    if(!debounce_set)
    {
      debounce_set = 1;
      debounce_time = millis();

    }else
    {
      if(millis()-debounce_time > debounce_timer)
      {
        change_state = 1;


      }
    }
  }else
  {
    if(debounce_set)
    {
      if(millis()-debounce_time > debounce_timer)
      {
        change_state = 0;
        debounce_set = 0;
        button_push_toggle = 0;
      }
    }
  }

  if(change_state && !button_push_toggle)
  {
    button_push_toggle = 1;
    state = state + 1;
    if(state > 2)
    {
      state = 0;
    }
    keyPadState = 100*idx + state;
    lcd.setCursor(19, 0);
    lcd.print(state);

    change_state = 0;
  }

  // ======= Pressure reading ==============

  if(millis() > loop_pressure + 10*ms) // every 10 ms, 100 Hz
  {
    loop_pressure = millis();
    sensor_pressure_voltage = map(analogRead(sensor_pressure_pin), 0, 1023, 0, 5000);
    sensor_ampfactor = sensor_nominal_voltage/sensor_supply_voltage;
    sensor_pressure_reading = sensor_pressure_voltage*sensor_ampfactor/sensor_pressure_sensitivity;
  }

  // ============ Temperature reading ================

  if(millis() > loop_temperature + 1*s) // 1 Hz signal
  {
      loop_temperature = millis();
      sensors.requestTemperatures();
      sensor_temperature_reading = sensors.getTempCByIndex(0);
  }

  // ============ Serial transmission =============

  if(millis() > loop_serial + dt_serial) // 1 Hz signal
  {

      loop_serial = millis();


    if(digitalRead(button_switch_pin))
    {
      Serial.println("<<;t:ms;T:°C;P:kPa");
      Serial.print(">>;");
      Serial.print(millis());
      Serial.print(";");
      Serial.print(sensor_temperature_reading);
      Serial.print(";");
      Serial.println(sensor_pressure_reading);

    }
  }

  // ============ Display refresh =============

  if(millis() > loop_display + 500*ms)
  {
    loop_display = millis();



    switch(keyPadState)
    {
      case stateA0:
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("mV:");
        lcd.print(sensor_pressure_voltage);

        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("P(kPa):");
        lcd.print(sensor_pressure_reading);
      break;

      case stateB0:
      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,2);
      lcd.print("t(ms):");
      lcd.print(millis());

      lcd.setCursor(0,3);
      lcd.print("                    ");
      lcd.setCursor(0,3);
      lcd.print("T(*C):");
      lcd.print(sensor_temperature_reading);

      break;

      case stateC0:
      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,2);
      lcd.print("T(*C):");
      lcd.print(sensor_temperature_reading);

      lcd.setCursor(0,3);
      lcd.print("                    ");
      lcd.setCursor(0,3);
      lcd.print("P(kPa):");
      lcd.print(sensor_pressure_reading);

      break;

      default:
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
      break;
    }

  }

}
