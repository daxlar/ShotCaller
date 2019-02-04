#include <ESP8266WiFi.h>

const char* ssid     = "";
const char* password = "";
const char* host = "192.168.43.96";
uint16_t     testBuffer[10] = {256,257,258,259,260,261,262,263,264,265};
uint8_t  packetData[4];
char fft_frequency = 0;
char fft_magnitude = 0;
char fft_phase = 0;
char analog_amplitude = 0;

WiFiClient client;

void setup() {
  
  Serial.begin(115200);
  Serial.flush();
  
  pinMode(16,OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(16, HIGH);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".....");
  }
  Serial.println("i'm connected");
  digitalWrite(2, LOW);
  delay(1000);
  const int httpPort = 55555;
  while(!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    Serial.println(client.connect(host, httpPort));
  }
  digitalWrite(16, LOW);
  
}
void loop() {
  /*
   * Order of data input is :
   * fft_frequency
   * fft_mangitude
   * fft_phase
   * analog_amplitude
   * wait until all 4 pieces of data arrive
   * data is reduced by +-255 resolution (because of decimal decimation) and neglience LOL
   * future revisions should correct this
   */
  if(Serial.available() == 4)
  {
    /*debugging lines
    Serial.print(Serial.read() * 255);
    Serial.print(" ");
    Serial.print(Serial.read() * 255);
    Serial.print(" ");
    Serial.print(Serial.read());
    Serial.print(" ");
    Serial.println(Serial.read() * 255);
    Serial.flush();
    */
    packetData[0] = Serial.read();
    packetData[1] = Serial.read();
    packetData[2] = Serial.read();
    packetData[3] = Serial.read();
    client.write((const uint8_t *)packetData,4);
    yield();
    Serial.flush();
  }
}
