#include <DataHandle.h>
#include <LCD12864RSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TimeLib.h>

#define RST_PIN         9           // RFID RES_PIN
#define SS_PIN          10          // RFID SS_PIN
#define ONE_WIRE_BUS    2           // DS18B20 PIN
#define led             5
#define PWMpin          3

const uint8_t block = 1;                   //RFIDCord Zone
const uint8_t len = 18;
//PID控制系数
const float kp=0.2;   
const float ki=0.1;
const float kd=0.4;
const float SetTemperature=4;

bool flagUartRx = false;
byte buffer2[20];                          //RFID information save

//串口接收的数据，时间数据
typedef struct {
  uint8_t d;           //1:[0}
  uint8_t h;           //1:[1}
  uint8_t m;           //1:[2}
  uint8_t s;           //1:[3}
} rxData_t;
rxData_t rxData = {0, 0, 0, 0};
//串口发送数据
typedef struct
{
  float temperature;
  uint16_t info;
  uint16_t len;
}SendData;
SendData d={0,0,sizeof(SendData)};

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.

void setup(void)
{
  Serial.begin(9600);
  LCDA.Initialise();
  SPI.begin();                  // Init SPI bus
  mfrc522.PCD_Init();
  sensors.begin();
  pinMode(led,OUTPUT);
  pinMode(PWMpin,OUTPUT);
  
  while (!flagUartRx) 
  {
    uartHandle();     //串口收数
    delay(50);
  }
  setTime(rxData.h, rxData.m, rxData.s, 2, 4, 19); // set time to Saturday 8:29:00am Jan 1 2011
  LCDA.CLEAR();
  LCDA.DisplayString(0, 0, "T:", 2);
  LCDA.DisplayString(0, 4, "ERR:", 2);
  LCDA.DisplayString(1, 0, "Date:",5);
  LCDA.DisplayString(2, 0, "Time:", 5);
  LCDA.DisplayString(3, 0, "Present:", 8);
  sensors.setWaitForConversion(false); //set conversion no wait
}

void loop(void)
{
  static uint32_t preTime = millis();
  uint32_t curTime = millis();//当前CPU时间
  uint8_t i;
  static bool RFIDflag = 0;
  float PWMvalue;
  
  uartHandle();
  //DS18B20 function
  sensors.requestTemperatures();
  d.temperature=sensors.getTempCByIndex(0);
  ShowTemperature(d.temperature);
  
  //PID控制方程
  PWMvalue=PWMValue(d.temperature);
  analogWrite(PWMpin,PWMvalue*255);
  
  ShowDate();
  if (RFIDflag)
  {
    d.info=ByteArrayToNumber(buffer2 + 1);
    if((*buffer2=='v')||(*buffer2=='c'))
        LCDA.DisplayString(2, 4, buffer2, 7);
    if((*buffer2=='t')||(*buffer2=='r'))
        LCDA.DisplayString(3, 4, buffer2, 7);
    RFIDflag = 0;
  }

  if (curTime - preTime >= 1000)
  {
    byte p[8];
    preTime = curTime;
    memcpy(p,(uint8_t *)&d,8);
    //uint8_t *p=(uint8_t *)&d;
    for (i = 0; i < 8; i++)
      Serial.write(p[i]);
  }

  //RFID function
  MFRC522::MIFARE_Key key;
  for (i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  MFRC522::StatusCode status;
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    RFIDflag = 1;
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}


void uartHandle() {
  uint8_t buf[16];
  uint8_t *pBuf = buf;

  if (Serial .available())
  {
    while (Serial .available())
    {
      uint8_t c = Serial .read();
      // Serial.write(c);
      *pBuf++ = c;
    }
    uint8_t len = pBuf - buf;
    if (len == sizeof(rxData))
    {
      memcpy(&rxData , buf, len );
      flagUartRx = true;
    }
  }
}

void ShowDate()
{
  uint8_t pd[12],pt[10];
  DateToString(day(),pd);
  TimeToString(hour(),minute(),second(),pt);
  LCDA.DisplayString(1,3,pd, 10);
  LCDA.DisplayString(2,3,pt, 8);
 }

void ShowTemperature(float x)
{
  byte buf[5]="No  ";
  if(x>=12)
  {
    digitalWrite(led,HIGH);
    buf[0]='h';
    buf[1]='i';
    buf[2]='j';
    buf[3]='h';
  }
  else if(x<=2)
  {
    digitalWrite(led,HIGH);
    buf[0]='l';
    buf[1]='o';
    buf[2]='w';
    buf[3]=' ';
    }
  else
  {
    digitalWrite(led,LOW);
  }
  LCDA.DisplayDouble(0,1,x);
  LCDA.DisplayString(0,6,buf,4);
}
 
 float Detu(float x)
 {
    static float tp1=0,tp2=0;  //误差  
    float t;
    t=kp*(x-tp2)+ki*x+kd*(x-2*tp2+tp1);
    tp1=tp2;
    tp2=x;
    return t;
  }

  float PWMValue(float x)
  {
    float pwmv,du;
    pwmv=x-SetTemperature;
    du=Detu(pwmv);
    if(x<=SetTemperature)
     pwmv=0;
    else if((x-SetTemperature)>=3)
      pwmv=1;
    else
    {
      pwmv=0.5+du;
    }
    if(pwmv>1)
      pwmv=1;
    else if(pwmv<0)
      pwmv=0;
    return pwmv;
  }
