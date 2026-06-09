#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <HTTPClient.h>

WiFiClient wifiClient;
#define ONE_WIRE_BUS 16
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;

const char *ssid = "DVD-ROM";
const char *password = "trss87112";

const char *host = "pemantauankesehatan.000webhostapp.com";

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0;

/// Inisialisasi nilai untuk menunjukkan pin sensor MPX5050GP
int SensorPressureOutput = A3;

/// Inisialisasi nilai untuk menunjukkan pin modul AD8232
int ECGBoardOutput = A0;

/// Inisialisasi variabel untuk output sensor MAX30102
float beatsPerMinute;
int beatAvg;
float temp;
float perCent;
int count1;

/// Inisialisasi variabel untuk output sensor Gula Darah
int adcsensor;
int adcsensor1;
float fix;
int datafix;
int cacah;

/// Inisialisasi variabel untuk output sensor MPX5050GP
float ADCOutput;
float VOut;
float kPa;
float mmHgx;
float mmHg;

/// Inisialisasi variabel untuk nilai tekanan sistolik dan diastolik
float SV;
float DV;
float SistoleBP;
float DiastoleBP;

/// Inisialisasi variabel untuk salah satu proses pengukuran tekanan sistolik dan diastolik
int mark = 0;

unsigned long previousMillis = 0; 
unsigned long previousMillis1 = 0; 
unsigned long previousMillis2 = 0; 

int ledState = HIGH;

void setup()  {
  // Koneksi ke database
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /// Inisialisasi pin LO- dan LO+ pada modul AD8232.
  pinMode(25, INPUT);
  pinMode(26, INPUT);

  /// Inisialisasi pin pada LED.
  pinMode(4, OUTPUT);
  
  particleSensor.begin(Wire, I2C_SPEED_FAST);
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.setPulseAmplitudeGreen(0); 
  sensors.begin();

  byte ledBrightness = 25;
  byte sampleAverage = 4;
  byte ledMode = 2;
  int sampleRate = 400;
  int pulseWidth = 411; 
  int adcRange = 2048; 
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void pressure() {
  /// Inisialisasi untuk memperoleh output ADC dari sensor MPX5050GP
  ADCOutput = analogRead(SensorPressureOutput);

  /// Inisialisasi untuk memperoleh output tegangan dari sensor MPX5050GP (Vout = ADC * (Vs/Bit ADC Arduino)
  VOut      = ADCOutput * 0.0012210012210012;

  /// Inisialisasi untuk memperoleh output berupa tegangan dalam kPa [kPa = Vout/Vs]
  kPa       = (((VOut / 5) - 0.04) / 0.018);

  mmHg      = (kPa * 7.50062);

  if ((mmHg >= mmHgx + 10) && (mmHg > 100) && (mark == 0)) {
    Serial.println("Mulai pengukuran Tekanan Sistolik...");
    SV = mmHg;
    mark = 2;
  }

  if ((mmHg > 50) && (mmHg < 90) && (mark == 2)) {
    Serial.println("Mulai pengukuran Tekanan Diastolik...");
    DV = mmHg;
    mark = 3;
  }

  mmHgx = mmHg;

  if ((mark == 3) && (mmHg < 50)) {
    mark = 0;
    SistoleBP = SV;
    DiastoleBP = DV;
    return;
  }
  delay(1);
  pressure();
}

void proses() {
  digitalWrite(4, HIGH);
  
  adcsensor = analogRead(34);
  adcsensor1 = adcsensor / 4;

  cacah++;
  datafix = adcsensor1 + datafix;

  if (cacah > 20) {
    fix = datafix / 20.0;
    return;
  }
  proses();
}

void loop() {
  digitalWrite(4, LOW);

  unsigned long currentMillis = millis();
  unsigned long currentMillis1 = millis();
  unsigned long currentMillis2 = millis();
  float irOffset = particleSensor.getRed();
  float irValue = particleSensor.getIR();
  
  /// Inisialisasi untuk memperoleh output ADC dari sensor MPX5050GP
  ADCOutput = analogRead(SensorPressureOutput);

  /// Inisialisasi untuk memperoleh output tegangan dari sensor MPX5050GP (Vout = ADC * (Vs/Bit ADC Arduino)
  VOut      = ADCOutput * 0.0012210012210012;
  /// Inisialisasi untuk memperoleh output berupa tegangan dalam kPa [kPa = Vout/Vs]  
  kPa       = (((VOut / 5) - 0.04) / 0.018);

  mmHgx     = (kPa * 7.50062);

  if (mmHgx > 0) {
    mark = 0;
    pressure ();
    Serial.print("\n");
    Serial.print(F("Sistolik= "));
    Serial.print(SistoleBP);
    Serial.print(F("mmHg"));    
    Serial.print("\n");
    Serial.print(F("Diastolik= "));
    Serial.print(DiastoleBP);
    Serial.print(F("mmHg"));

    Serial.print("connecting to ");
    Serial.println(host);
    WiFiClient client;
    const int httpPort = 80;

    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    
    String url5 = "/write-data4.php?data=";
    String dat1 = String(SistoleBP) + "&data2=" + String(DiastoleBP);

    url5 += dat1;
    Serial.print("Requesting URL: ");
    Serial.println(url5);
    String Link5;
    HTTPClient http;
    Link5 = "http://" + String(host) + String(url5);
    http.begin(wifiClient, Link5);
    http.GET();
    http.end();

    client.print(String("GET ") + url5 + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
    unsigned long timeout = millis();

    while (client.available() == 0) {
      if (millis() - timeout > 1000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    Serial.println("closing connection");
  }
  else  {
  /// Mengecek detak jantung dan Sp02
    if (irValue < 50000)  {
      beatAvg = 0;
      perCent = 0;
      Serial.print(F("Please Place Finger "));
      Serial.print(F("\n"));
      delay(1000);
    }
    else  {
      if (checkForBeat(irValue) == true && ledState == HIGH)  {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20)  {
        rates[rateSpot++] = (byte)beatsPerMinute; 
        rateSpot %= RATE_SIZE; 

        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
          beatAvg /= RATE_SIZE;
      }
     
      /// Tampilkan nilai detak jantung
      Serial.print(F("HR="));
      Serial.print(beatAvg);
      Serial.print(F("bpm"));
      Serial.print(F(", IR val="));
      Serial.print(irValue);
      Serial.print(F(", IR off="));
      Serial.print(irOffset);

      perCent = irValue / irOffset;
      perCent = perCent * 100;
      if (perCent >= 100) {
        perCent = 100;
      }

      /// Tampilkan nilai Sp02
      Serial.print(F(", Blood Oxygen"));
      Serial.print(perCent);
      Serial.print(F("%SpO2"));
      Serial.print(F("\n"));

      /// Pengiriman data ke website
      count1 = count1 + 1;
      if (count1 == 8) {
        Serial.print("connecting to ");
        Serial.println(host);
        WiFiClient client;
        const int httpPort = 80;

        if (!client.connect(host, httpPort)) {
          Serial.println("connection failed");
          return;
        }
        
        String url = "/write-data1.php?data=";
        String dat = String(beatAvg) + "&data2=" + String(perCent);

        url += dat;
        Serial.print("Requesting URL: ");
        Serial.println(url);
        String Link1;
        HTTPClient http;
        Link1 = "http://" + String(host) + String(url);
        http.begin(wifiClient, Link1);
        http.GET();
        http.end();

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n\r\n");
        unsigned long timeout = millis();

        while (client.available() == 0) {
          if (millis() - timeout > 1000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
          }
        }

        Serial.println("closing connection");
        count1 = 0;
      }
      }
    }
 
  /// Tampilkan nilai suhu
  if (currentMillis - previousMillis >= 20000)  {
    previousMillis = currentMillis;

    temp = sensors.getTempCByIndex(0); 
    sensors.setWaitForConversion(false);  
    sensors.requestTemperatures(); 

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.print((char)176);
    Serial.print("C  |  ");
  }
 
  /// Tampilkan nilai gula darah
  if (currentMillis1 - previousMillis1 >= 20000)  {
    previousMillis1 = currentMillis1;
    proses();
    Serial.print("\n");
    Serial.print(F("mg/dl= "));
    Serial.print(fix);

    /// Koneksi ke database
    Serial.print("connecting to ");
    Serial.println(host);
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    
    String url2 = "/write-data.php?data=";
    url2 += temp;
    Serial.print("Requesting URL: ");
    Serial.println(url2);
    String Link3;
    HTTPClient http;
    Link3 = "http://" + String(host) + String(url2);
    http.begin(wifiClient, Link3);
    http.GET();
    http.end();

    client.print(String("GET ") + url2 + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout3 = millis();
    while (client.available() == 0) {
      if (millis() - timeout3 > 1000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    String url3 = "/write-data3.php?data=";
    url3 += fix;
    Serial.print("Requesting URL: ");
    Serial.println(url3);
    String Link4;
    Link4 = "http://" + String(host) + String(url3);
    http.begin(wifiClient, Link4);
    http.GET();
    http.end();
    client.print(String("GET ") + url3 + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout4 = millis();
    while (client.available() == 0) {
      if (millis() - timeout4 > 1000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
      Serial.println("closing connection");
      cacah = 0;
      datafix = 0;
      Serial.print("\n");
  }
  
  /// Potongan Program untuk Monitoring ECG 
  if (currentMillis2 - previousMillis2 >= 25000)
  {
    previousMillis2 = currentMillis2;
    String banyak = "/write-data5.php?data=";
    if((digitalRead(25) == 1)||(digitalRead(26) == 1)){
      Serial.println('!');
    }
    else{
    for (int i = 0; i < 50; i++) {
      // Mengirim nilai analog pada modul AD8232 melalui pin Vn
      Serial.println(analogRead(ECGBoardOutput));
      if (i == 0) {
        String EKG = String(analogRead(A3));
        if (EKG =="!")
        {
          EKG = "0";
        } 
        banyak += EKG;
      }
      
      if (i >= 1) {
        String EKG = String(analogRead(A3));
        if (EKG =="!")
        {
          EKG = "0";
        } 
        String ba = "&data" + String(i) + "=" + EKG;
        banyak += ba; 
      }
      
      /// Koneksi ke database
      if (i == 49) {
      Serial.print("connecting to ");
      Serial.println(host);
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
      }
      
      Serial.print("Requesting URL: ");
      Serial.println(banyak);
      String Link4;
      HTTPClient http;
      Link4 = "http://" + String(host) + String(banyak);
      http.begin(wifiClient, Link4);
      http.GET();
      http.end();

      client.print(String("GET ") + banyak + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
      unsigned long timeout4 = millis();
      while (client.available() == 0) {
        if (millis() - timeout4 > 1000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
        }
      }
        Serial.println("closing connection");
      }
    }
    delay(1);
    }
  }
}
}
