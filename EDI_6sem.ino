#include <AltSoftSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <SoftwareSerial.h>

const String EMERGENCY_PHONE = "+918983939720";
const String EMERGENCY_PHONE1 = "+9199219868197";
#define rxPin 3 // GSM connection
#define txPin 2
#define Sober 200   // Define max value that we consider sober
#define Drunk 400   // Define min value that we consider drunk
#define MQ3 A0
#define ledPin 6
#define v1 10
#define v2 11
#define v3 12
#define xPin A1
#define yPin A2
#define zPin A3

float sensorValue;

SoftwareSerial sim800(rxPin, txPin);
AltSoftSerial neogps;
TinyGPSPlus gps;

byte updateflag;

int xaxis = 0, yaxis = 0, zaxis = 0;
int deltx = 0, delty = 0, deltz = 0;
int vibration = 2;
int devibrate = 75;
int magnitude = 0;
int sensitivity = 20;

double angle;
boolean impact_detected = false;
unsigned long time1;
unsigned long impact_time;
unsigned long alert_delay = 30000;
int maxSpeed = 60; // Maximum speed limit in kmph

void setup()
{
  Serial.begin(9600);
  sim800.begin(9600);
  neogps.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("MQ3 Heating Up!");

  delay(20000); // allow the MQ3 to warm up

  pinMode(v1, OUTPUT);
  pinMode(v2, OUTPUT);
  pinMode(v3, OUTPUT);

  String sms_status = "";
  String sender_number = "";
  String received_date = "";
  String msg = "";
  sim800.println("AT");
  delay(1000);
  sim800.println("ATE1");
  delay(1000);
  sim800.println("AT+CPIN?");
  delay(1000);
  sim800.println("AT+CMGF=1");
  delay(1000);
  sim800.println("AT+CNMI=1,1,0,0,0");
  delay(1000);
  time1 = micros();
  xaxis = analogRead(xPin);
  yaxis = analogRead(yPin);
  zaxis = analogRead(zPin);
}

void loop()
{
  if (micros() - time1 > 1999) Impact();
  if (updateflag > 0)
  {
    updateflag = 0;
    Serial.println("Impact detected!!");
    Serial.print("Magnitude:");
    Serial.println(magnitude);

    getGps();
    digitalWrite(ledPin, HIGH);
    impact_detected = true;
    impact_time = millis();
    sendAlert(); // Call SendAlert() when an accident is detected
  }
  if (impact_detected == true)
  {
    if (millis() - impact_time >= alert_delay)
    {
      digitalWrite(ledPin, LOW);
      makeCall();
      delay(1000);
      sendAlert();
      impact_detected = false;
      impact_time = 0;
    }
  }

  if (digitalRead(ledPin) == LOW)
  {
    delay(200);
    digitalWrite(ledPin, LOW);
    impact_detected = false;
    impact_time = 0;
  }

  parseGpsData();

  checkAlcoholLevel();
  checkOverspeed();
  
  delay(1000); // delay for stability
}

void Impact()
{
  time1 = micros();
  int oldx = xaxis;
  int oldy = yaxis;
  int oldz = zaxis;

  xaxis = analogRead(xPin);
  yaxis = analogRead(yPin);
  zaxis = analogRead(zPin);

  vibration--;
  Serial.print("Vibration = ");
  Serial.println(vibration);
  if (vibration < 0) vibration = 0;
  if (vibration > 0) return;
  deltx = xaxis - oldx;
  delty = yaxis - oldy;
  deltz = zaxis - oldz;

  magnitude = sqrt(sq(deltx) + sq(delty) + sq(deltz));
  if (magnitude >= sensitivity) //impact detected
  {
    updateflag = 1;
    vibration = devibrate;
  }
  else
  {
    magnitude = 0;
  }
}

void parseGpsData()
{
  while (neogps.available())
  {
    if (gps.encode(neogps.read()))
    {
      if (gps.location.isValid())
      {
        Serial.print("Latitude= ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Longitude= ");
        Serial.println(gps.location.lng(), 6);
      }
    }
  }
}

void getGps()
{
  while (neogps.available())
  {
    gps.encode(neogps.read());
  }
}

void checkAlcoholLevel()
{
  sensorValue = analogRead(MQ3); // read analog input pin 0

  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue < Sober)
  {
    Serial.println("  |  Status: Sober");
  }
  else if (sensorValue >= Sober && sensorValue < Drunk)
  {
    Serial.println

("  |  Status: Drinking but within legal limits");
    sendAlert1();
  }
  else if (sensorValue >= Drunk && sensorValue <= 700)
  {
    Serial.println("  |  Status: Moderately Drunk");
    sendAlert2();
  }
  else if (sensorValue > 700)
  {
    Serial.println("  |  Status: Fully Drunk");
    sendAlert();
    makeCall();
  }
}

void checkOverspeed()
{
  if (gps.speed.isValid() && gps.speed.kmph() > maxSpeed)
  {
    Serial.println("Overspeed detected!");
    sendAlert3(); // Call SendAlert() when overspeed is detected
    makeCall();
  }
}

void makeCall()
{
  Serial.println("calling....");
  sim800.println("ATD" + EMERGENCY_PHONE1 + ";");
  delay(20000);
  sim800.println("ATD" + EMERGENCY_PHONE + ";");
  delay(20000); // 20 sec delay
  sim800.println("ATH");
  delay(1000); // 1 sec delay
}

void sendAlert()
{
  String sms_data;
  sms_data = "Accident condition found (Immediately reach to the person)\r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(gps.location.lat(), 6);
  sms_data += ",";
  sms_data += String(gps.location.lng(), 6);
  sendSms(sms_data);
}

void sendAlert1()
{
  String sms_data;
  sms_data = "Drunk & Drive case Detected ->> Drunk moderately \r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(gps.location.lat(), 6);
  sms_data += ",";
  sms_data += String(gps.location.lng(), 6);
  sendSms(sms_data);
}

void sendAlert2()
{
  String sms_data;
  sms_data = "Drunk & Drive case Detected ->> Fully Drunk \r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(gps.location.lat(), 6);
  sms_data += ",";
  sms_data += String(gps.location.lng(), 6);
  sendSms(sms_data);
}

void sendAlert3()
{
  String sms_data;
  sms_data = "Overspeed detection \r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(gps.location.lat(), 6);
  sms_data += ",";
  sms_data += String(gps.location.lng(), 6);
  sendSms(sms_data);
}

void sendSms(String text)
{
  sim800.print("AT+CMGF=1\r");
  delay(1000);
  sim800.print("AT+CMGS=\"" + EMERGENCY_PHONE + "\"\r");
  delay(20000);
  sim800.print("AT+CMGS=\"" + EMERGENCY_PHONE1 + "\"\r");
  delay(1000);
  sim800.print(text);
  delay(100);
  sim800.write(0x1A);
  delay(1000);
  Serial.println("SMS Sent Successfully.");
}
