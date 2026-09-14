// Heltec WiFi LoRa 32 V4 (ESP32-S3 + SX1262) raw LoRa RX monitor.
// SF7 / BW500 / CR4-5 / sync 0x2B / 868.77 MHz  (matches our verified encoder).
// Prints a 1s heartbeat + every reception (RSSI/SNR/len/raw hex/CRC, incl CRC-fail).

#include <RadioLib.h>

SX1262 radio = new Module(8, 14, 12, 13);   // NSS=8 DIO1=14 RST=12 BUSY=13

volatile bool rxFlag = false;
void IRAM_ATTR onRx() { rxFlag = true; }
int radioState = -999;
unsigned long lastHB = 0;
float maxRssi = -200;

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Vext (GPIO36, active LOW) powers peripherals on Heltec
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(50);

  Serial.println();
  Serial.println("=== Heltec V4 LoRa RX monitor ===");

  SPI.begin(9, 11, 10, 8);   // SCK, MISO, MOSI, SS
  radioState = radio.begin(868.825, 500.0, 7, 5, 0x2B, 10, 16, 1.8);
  if (radioState != RADIOLIB_ERR_NONE)
    radioState = radio.begin(868.825, 500.0, 7, 5, 0x2B, 10, 16, 1.6);
  Serial.print("radio.begin = "); Serial.println(radioState);

  if (radioState == RADIOLIB_ERR_NONE) {
    radio.setCRC(2);
    radio.setDio1Action(onRx);
    int rs = radio.startReceive();
    Serial.print("startReceive = "); Serial.println(rs);
  }
  Serial.println("ready: SF7 BW500 CR4/5 sync0x2B @868.77");
}

void loop() {
  if (rxFlag) {
    rxFlag = false;
    uint8_t data[256];
    int len = radio.getPacketLength();
    int state = radio.readData(data, len);
    Serial.print(">>> RX len="); Serial.print(len);
    Serial.print(" rssi="); Serial.print(radio.getRSSI(), 1);
    Serial.print(" snr="); Serial.print(radio.getSNR(), 1);
    Serial.print(" crc="); Serial.print(state == RADIOLIB_ERR_NONE ? "OK" : (state == RADIOLIB_ERR_CRC_MISMATCH ? "FAIL" : "err"));
    Serial.print(" | ");
    for (int i = 0; i < len; i++) { if (data[i] < 16) Serial.print('0'); Serial.print(data[i], HEX); Serial.print(' '); }
    Serial.println();
    radio.startReceive();
  }
  // Continuously sample channel RSSI; report the MAX over each window so a brief
  // (~20 ms) transmission spike is never missed.
  float r = radio.getRSSI(false);
  if (r > maxRssi) maxRssi = r;
  if (millis() - lastHB > 300) {
    lastHB = millis();
    Serial.print("t="); Serial.print(millis() / 1000);
    Serial.print(" radio="); Serial.print(radioState);
    Serial.print(" maxRSSI="); Serial.println(maxRssi, 1);
    maxRssi = -200;
  }
}
