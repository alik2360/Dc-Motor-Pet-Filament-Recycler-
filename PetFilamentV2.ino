#include "thermistor.h" //Thermistor Library by  Miguel califa
#include "LiquidCrystal.h"
#include "AutoPID.h"
#include "OneButton.h"
#include "EEPROM.h"

// For temperature control
double temperature = 0;
const int pwmHeater = 2;
double set_temperature = 180;
bool heater = false;
bool isFilament = false;
const int delta = 10;

// Motor control
const int filament = A5;
const int strip = A4;

// Top Buttons
const int filament_pin = 10;
const int strip_pin = 11;
const int home_pin = 12;
const int home_switch = 9;

OneButton filament_button(filament_pin, true);
OneButton strip_button(strip_pin, true);
OneButton home_button(home_pin, true);

// Lcd Buttons
const int up_pin = A0;
const int down_pin = A1;
const int heater_pin = A2;

OneButton up_button(up_pin, true);
OneButton down_button(down_pin, true);
OneButton heater_button(heater_pin, true);

// LCD Initialization
LiquidCrystal lcd(7, 13, 6, 5, 4, 3);

// ThermistorS
thermistor therm1(A3, 1);

// Home
bool isHoming = false;

long previousMillis = 0;
long interval = 500;

// pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255

double outputVal;

// Motor Control
bool motor = false;

// pid values from autotune
double Kp = 0.0035;
double Ki = 0.02;
double Kd = 0.25;

// Eeprom
const int eepromAddress = 0;
const int min_temp = 100;
const int max_temp = 280;

AutoPID myPID(&temperature, &set_temperature, &outputVal, OUTPUT_MIN, OUTPUT_MAX, Kp, Ki, Kd);

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);

  int storedValue;
  EEPROM.get(eepromAddress, storedValue);
  Serial.print(storedValue);
  if (storedValue >= min_temp && storedValue <= max_temp)
  {
    set_temperature = storedValue;
  }
  else
  {
    EEPROM.write(eepromAddress, (int)set_temperature);
  }

  up_button.attachClick(upButton);
  down_button.attachClick(downButton);
  heater_button.attachClick(heaterButton);
  heater_button.attachDuringLongPress(saveTemp);

  strip_button.attachClick(stripButton);
  home_button.attachClick(homeButton);
  filament_button.attachClick(start_filament);
  filament_button.attachDuringLongPress(start_pwm);
  filament_button.setLongPressIntervalMs(1000);

  pinMode(pwmHeater, outputVal);
  digitalWrite(pwmHeater, LOW);

  pinMode(filament, OUTPUT);
  pinMode(strip, OUTPUT);
  pinMode(home_switch, INPUT);

  myPID.setBangBang(2);
  myPID.setTimeStep(50);

  welcomeScreen();

  initialData();
}

void loop()
{

  unsigned long currentMillis = millis();

  temperature = (int)therm1.analog2temp();

  strip_button.tick();
  home_button.tick();
  filament_button.tick();

  up_button.tick();
  down_button.tick();
  heater_button.tick();

  heater_control(currentMillis);
  if (isFilament)
  {

    if (temperature + delta >= set_temperature)
    {
      runMotorPWM();
    }
    else
    {
      stopMotor();
    }
  }
  else if (isHoming)
  {
    if (digitalRead(home_switch) == 1)
    {
      homeSwitch();
    }
  }
  delay(30);
}

void heater_control(unsigned long currentMillis)
{
  if (heater)
  {
    myPID.run();
    analogWrite(pwmHeater, outputVal);
  }
  else
  {
    myPID.stop();
    digitalWrite(pwmHeater, LOW);
  }

  if (updateTime(currentMillis))
  {

    lcd.setCursor(12, 0);
    lcd.print("    ");
    lcd.setCursor(12, 0);
    lcd.print((int)temperature);
  }
}
bool updateTime(unsigned long currentMillis)
{
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  else
  {
    return false;
  }
}
void initialData()
{
  lcd.setCursor(0, 0);
  if (heater == true)
  {
    lcd.print("Tmax=");
    lcd.print((int)set_temperature);
    lcd.print(" ");
  }
  else
  {
    lcd.print("Heat=Off Tc=");
  }
  lcd.setCursor(0, 1);
  lcd.print("Ac=None  ");
  lcd.setCursor(9, 1);
  lcd.print("Mo=Off ");
}
void updateAction(String action, String mode)
{
  delay(150);
  lcd.setCursor(3, 1);
  lcd.print(action);
  lcd.setCursor(12, 1);
  lcd.print(mode);
}

void welcomeScreen()
{
  delay(1000);
  lcd.begin(16, 2);
  lcd.print(" PET RECYCLING ");
  lcd.setCursor(0, 1);
  lcd.print("Yt: @M.SKhan123");

  delay(2500);
  lcd.clear();
}

void runMotorPWM()
{
  digitalWrite(strip, LOW);
  digitalWrite(filament, HIGH);
  motor = true;
}
void runMotor()
{
  digitalWrite(filament, LOW);
  digitalWrite(strip, HIGH);
  motor = true;
}
void stopMotor()
{
  digitalWrite(strip, LOW);
  digitalWrite(filament, LOW);
  motor = false;
}

void upButton()
{
  if (set_temperature < max_temp)
  {
    setTemperature(true);
  }
}

void downButton()
{
  if (set_temperature > min_temp)
  {
    setTemperature(false);
  }
}

void setTemperature(bool up)
{
  if (up)
  {
    set_temperature = set_temperature + 1;
  }
  else
  {

    set_temperature = set_temperature - 1;
  }
  lcd.setCursor(0, 0);
  lcd.print("Temp=");
  lcd.print((int)set_temperature);
  lcd.print(" ");
}
void saveTemp()
{

  EEPROM.update(eepromAddress, set_temperature);

  lcd.setCursor(0, 0);
  if (heater == true)
  {
    lcd.print("Tmax=");
    lcd.print((int)set_temperature);
    lcd.print(" ");
  }
  else
  {
    lcd.print("Heat=Off Tc=");
  }
}

void homeButton()
{
  isHoming = !isHoming;
  isFilament = false;
  if (isHoming)
  {
    runMotor();
    updateAction("Home ", "Norm");
  }
  else
  {
    homeSwitch();
  }
}

void start_filament()
{
  isFilament = !isFilament;
  if (isFilament)
  {
    heater = false;
    heaterButton();
    updateAction("Film ", "Pwm ");
  }
  else
  {
    stopMotor();
    heaterButton();
    initialData();
  }
}

void start_pwm()
{
  isFilament = false;
  isHoming = false;
  runMotorPWM();
  updateAction("Manu ", "Pwm");
}
void stripButton()
{
  isFilament = false;
  if (motor)
  {
    stopMotor();
    initialData();
  }
  else
  {
    runMotor();
    updateAction("Strip ", "Norm");
  }
}

void homeSwitch()
{
  isHoming = false;
  stopMotor();
  initialData();
}

void heaterButton()
{

  heater = !(heater);
  lcd.setCursor(0, 0);
  if (heater == true)
  {

    lcd.print("Tmax=");
    lcd.print((int)set_temperature);
    lcd.print(" ");
  }
  else
  {
    lcd.print("Heat=Off ");
  }
}
