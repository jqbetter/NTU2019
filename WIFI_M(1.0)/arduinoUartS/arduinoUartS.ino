// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>


// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
#define LEDNUM 3
const uint8_t ledPin[LEDNUM] = {3, 4, 5};  //3个LED灯，接在D3，D4，D5脚
//串口发送的数据，定义数据长度是4的倍数
typedef struct {
  float t;      //4[0:3],温度
  float dat1;   //4[4:7]
  uint16_t dat2;     //2[8:9]
  uint16_t dat3;     //2[10:11]
} txData_t;
txData_t txData;
//00 00 5B 41 00 00 4A 42 64 00 00 00,13.7,50.5,100
//串口接收的数据，LED灯状态
typedef struct {
  uint8_t led[LEDNUM];
  uint8_t dat;
} rxData_t;
rxData_t rxData;   //led灯，1：不亮，0：亮
bool flagUartRx = false;

void setup(void)
{
  for (uint8_t i = 0; i < LEDNUM; i++) {
    pinMode(ledPin[i], OUTPUT);
  }
  memset(&rxData, 0, sizeof(rxData));
  updateLedStatus();
  txData.dat1 = 50.5;
  txData.dat2 = 100;
  Serial.begin(115200);
  // Start up the library
  sensors.begin();
  sensors.setWaitForConversion(false); //set conversion no wait
  sensors.requestTemperatures();     //request temperature convert
}

/*
   Main function, get and show the temperature
*/
void loop(void)
{
  static uint32_t preTime = millis();
  uint32_t curTime = millis();

  if (curTime - preTime >= 1000) {    //1s读一次温度，发送数据
    preTime = curTime;
    txData.t = sensors.getTempCByIndex(0);  //read temerature
    sensors.requestTemperatures();     //request temperature convert
    //    Serial.print(F("Temperatue=")); Serial.println(txData.t);
    uartSendData((uint8_t *)&txData, sizeof(txData)); //uart send data
  }
  uartHandle();     //串口收数
  if (flagUartRx) { //更新LED灯
    flagUartRx = false;
    updateLedStatus();
  }
}
void updateLedStatus() {
  for (uint8_t i = 0; i < LEDNUM; i++) {
    if (rxData.led[i]) {
      ledOff(ledPin[i]);
    } else {
      ledOn(ledPin[i]);
    }
  }
}
void uartHandle() {
  uint8_t buf[16];
  uint8_t *pBuf = buf;

  if (Serial.available()) {
    while (Serial.available()) {
      uint8_t c = Serial.read();
      *pBuf++ = c;
      delay(1);
    }
    uint8_t len = pBuf - buf;
    if (len==sizeof(rxData)) {
      memcpy(&rxData, buf, len);
      flagUartRx = true;
    }
    //    for(int i=0;i<LEDNUM;i++){
    //      Serial.write(rxData.led[i]);
    //    }
  }
}

void uartSendData(uint8_t *data, int len) {
  for (int i = 0; i < len; i++) {
    Serial.write(data[i]);
  }
}

void ledOn(uint8_t ledPin) {
  digitalWrite(ledPin, HIGH);
}

void ledOff(uint8_t ledPin) {
  digitalWrite(ledPin, LOW);
}
