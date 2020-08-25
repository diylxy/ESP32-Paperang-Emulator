/*#include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
*/
#include <SPI.h>
#include "esp_task_wdt.h"

/*
  const char* host = "esp32-Printer";
  const char* ssid = "ssid";
  const char* password = "password";
  const char* softAP_ssid = "Printer";
  const char* softAP_password = "myprinter";
*/
#define PIN_MOTOR_AP 23
#define PIN_MOTOR_AM 22
#define PIN_MOTOR_BP 21
#define PIN_MOTOR_BM 19

#define PIN_LAT 18
#define PIN_SCK 5
#define PIN_SDA 4

#define PIN_STB1 26
#define PIN_STB2 25
#define PIN_STB3 33
#define PIN_STB4 32
#define PIN_STB5 35		//这里可能需要修改
#define PIN_STB6 34

#define PIN_BUZZER 15
#define BUZZER_FREQ 2000
#define startBeep() ledcWrite(0, 127)
#define stopBeep() ledcWrite(0,0)

#define PIN_STATUS 13
#define PIN_VHEN 2

#define MOTOR_STEP_PER_LINE 3
#define PRINT_TIME 1500
#define PRINT_TIME_ 200
#define MOTOR_TIME 4000
float addTime[6] = {0};
float tmpAddTime = 0;
//根据打印头打印效果修改
#define kAddTime 0.001      //点数-增加时间系数, 见sendData函数
#define STB1_ADDTIME 0      //STB1额外打印时间,下面类似, 单位: 微秒
#define STB2_ADDTIME 0      //根据打印头实际情况修改
#define STB3_ADDTIME 0
#define STB4_ADDTIME 0
#define STB5_ADDTIME 0
#define STB6_ADDTIME 0
extern uint8_t heat_density;
//WebServer server(80);

// char* serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//步进电机驱动部分
uint8_t motorTable[8][4] =
{
  {1, 0, 0, 0},
  {1, 0, 1, 0},
  {0, 0, 1, 0},
  {0, 1, 1, 0},
  {0, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};
uint8_t motorPos = 0;

void goFront(uint32_t steps, uint16_t wait)
{
  ++steps;
  while (--steps)
  {
    digitalWrite(PIN_MOTOR_AP, motorTable[motorPos][0]);
    digitalWrite(PIN_MOTOR_AM, motorTable[motorPos][1]);
    digitalWrite(PIN_MOTOR_BP, motorTable[motorPos][2]);
    digitalWrite(PIN_MOTOR_BM, motorTable[motorPos][3]);
    ++motorPos;
    if (motorPos == 8)
    {
      motorPos = 0;
    }
    delayMicroseconds(wait);
  }
  digitalWrite(PIN_MOTOR_AP, 0);
  digitalWrite(PIN_MOTOR_AM, 0);
  digitalWrite(PIN_MOTOR_BP, 0);
  digitalWrite(PIN_MOTOR_BM, 0);
}
void goFront1()
{
  digitalWrite(PIN_MOTOR_AP, motorTable[motorPos][0]);
  digitalWrite(PIN_MOTOR_AM, motorTable[motorPos][1]);
  digitalWrite(PIN_MOTOR_BP, motorTable[motorPos][2]);
  digitalWrite(PIN_MOTOR_BM, motorTable[motorPos][3]);
  ++motorPos;
  if (motorPos == 8)
  {
    motorPos = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//打印头驱动部分
uint8_t *printData;
uint32_t printDataCount = 0;
SPIClass printerSPI = SPIClass(HSPI);
SPISettings printerSPISettings = SPISettings(1000000, SPI_MSBFIRST, SPI_MODE0);

void clearAddTime()
{
  addTime[0] = addTime[1] = addTime[2] = addTime[3] = addTime[4] = addTime[5] = 0;
}
void sendData(uint8_t *dataPointer)
{
  for (uint8_t i = 0; i < 6; ++i)
  {
    for (uint8_t j = 0; j < 8; ++j)
    {
      addTime[i] += dataPointer[i * 8 + j];
    } 
    tmpAddTime = addTime[i] * addTime[i];
    addTime[i] = kAddTime * tmpAddTime;
  }
  printerSPI.beginTransaction(printerSPISettings);
  printerSPI.transfer(dataPointer, 48);
  printerSPI.endTransaction();

  digitalWrite(PIN_LAT, 0);
  delayMicroseconds(1);
  digitalWrite(PIN_LAT, 1);
}

void clearData(void)
{
  printerSPI.beginTransaction(printerSPISettings);
  for (uint8_t i = 0; i < 48; ++i) {
    printerSPI.transfer(0);
  }
  printerSPI.endTransaction();
  clearAddTime();
  digitalWrite(PIN_LAT, 0);
  delayMicroseconds(1);
  digitalWrite(PIN_LAT, 1);
}

void startPrint()
{
  static unsigned char motor_add = 0;
  startBeep();
  delay(200);
  stopBeep();

  Serial.println("[INFO]正在打印...");
  Serial.printf("[INFO]共%u行\n", printDataCount / 48);
  digitalWrite(PIN_VHEN, 1);
  for (uint32_t pointer = 0; pointer < printDataCount; pointer += 48)
  {

    motor_add ++;
    if (motor_add != 40)
    {
      delayMicroseconds((PRINT_TIME) * ((double)heat_density / 100));
      goFront1();
    }
    else
    {
      motor_add = 0;
    }
    clearAddTime();
    sendData(printData + pointer);
    digitalWrite(PIN_STB1, 1);
    delayMicroseconds((PRINT_TIME + addTime[0] + STB1_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB1, 0);
    delayMicroseconds(PRINT_TIME_);

    digitalWrite(PIN_STB2, 1);
    delayMicroseconds((PRINT_TIME + addTime[1] + STB2_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB2, 0);
    delayMicroseconds(PRINT_TIME_);
    goFront1();

    digitalWrite(PIN_STB3, 1);
    delayMicroseconds((PRINT_TIME + addTime[2] + STB3_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB3, 0);
    delayMicroseconds(PRINT_TIME_);

    digitalWrite(PIN_STB4, 1);
    delayMicroseconds((PRINT_TIME + addTime[3] + STB4_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB4, 0);
    delayMicroseconds(PRINT_TIME_);
    goFront1();

    digitalWrite(PIN_STB5, 1);
    delayMicroseconds((PRINT_TIME + addTime[4] + STB5_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB5, 0);
    delayMicroseconds(PRINT_TIME_);

    digitalWrite(PIN_STB6, 1);
    delayMicroseconds((PRINT_TIME + addTime[5] + STB6_ADDTIME) * ((double)heat_density / 100));
    digitalWrite(PIN_STB6, 0);
    delayMicroseconds(PRINT_TIME_);
    goFront1();
  }

  digitalWrite(PIN_MOTOR_AP, 0);
  digitalWrite(PIN_MOTOR_AM, 0);
  digitalWrite(PIN_MOTOR_BP, 0);
  digitalWrite(PIN_MOTOR_BM, 0);
  clearAddTime();
  clearSTB();
  clearData();
  printDataCount = 0;
  Serial.println("[INFO]打印完成");
  digitalWrite(PIN_VHEN, 0);
  startBeep();
  delay(100);
  stopBeep();
  delay(50);
  startBeep();
  delay(100);
  stopBeep();
}

void setupPins() {
  pinMode(PIN_MOTOR_AP, OUTPUT);
  pinMode(PIN_MOTOR_AM, OUTPUT);
  pinMode(PIN_MOTOR_BP, OUTPUT);
  pinMode(PIN_MOTOR_BM, OUTPUT);
  digitalWrite(PIN_MOTOR_AP, 0);
  digitalWrite(PIN_MOTOR_AM, 0);
  digitalWrite(PIN_MOTOR_BP, 0);
  digitalWrite(PIN_MOTOR_BM, 0);

  pinMode(PIN_LAT, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_SDA, OUTPUT);

  pinMode(PIN_STB1, OUTPUT);
  pinMode(PIN_STB2, OUTPUT);
  pinMode(PIN_STB3, OUTPUT);
  pinMode(PIN_STB4, OUTPUT);
  pinMode(PIN_STB5, OUTPUT);
  pinMode(PIN_STB6, OUTPUT);
  digitalWrite(PIN_STB1, 0);
  digitalWrite(PIN_STB2, 0);
  digitalWrite(PIN_STB3, 0);
  digitalWrite(PIN_STB4, 0);
  digitalWrite(PIN_STB5, 0);
  digitalWrite(PIN_STB6, 0);

  pinMode(PIN_STATUS, OUTPUT);
  digitalWrite(PIN_STATUS, 0);
  pinMode(PIN_VHEN, OUTPUT);
  digitalWrite(PIN_VHEN, 0);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(PIN_BUZZER, 0);
  ledcWrite(0, 0);

  printerSPI.begin(PIN_SCK, -1, PIN_SDA, -1);
  printerSPI.setFrequency(2000000);
  clearSTB();
  clearData();

}

void clearSTB()
{
  digitalWrite(PIN_STB1, 0);
  digitalWrite(PIN_STB2, 0);
  digitalWrite(PIN_STB3, 0);
  digitalWrite(PIN_STB4, 0);
  digitalWrite(PIN_STB5, 0);
  digitalWrite(PIN_STB6, 0);

}

void setup(void) {
  Serial.begin(115200);
  setupPins();
  //下面是配置wifi，为了防止干扰蓝牙打印，关闭wifi功能
  /*
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("softAP Mode");
    WiFi.softAP(softAP_ssid, softAP_password);
    }

    server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
    });
    server.on("/upload", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "OK");
    startPrint();
    }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[INFO]开始上传\n文件名: %s\n", upload.filename.c_str());
      printDataCount = 0;
      digitalWrite(PIN_STATUS, 1);
      startBeep();
      delay(100);
      stopBeep();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      memcpy(printData + printDataCount, upload.buf, upload.currentSize);
      printDataCount += upload.currentSize;
    } else if (upload.status == UPLOAD_FILE_END) {
      Serial.printf("[INFO]上传完成: 收到：%u||发送：%u\n", printDataCount, upload.totalSize);
      digitalWrite(PIN_STATUS, 0);
    }
    });

    server.on("/lineFeed/{}", []() {
    uint32_t lines = atoi(server.pathArg(0).c_str()) * MOTOR_STEP_PER_LINE;
    goFront(lines, MOTOR_TIME);
    server.send(200, "OK");
    });
  */
  //server.begin();

  printData = (uint8_t*)ps_malloc(3 * 1024 * 1024);
  if (!printData) {
    startBeep();
    Serial.println("[ERROR]PSRAM Malloc 失败!\n    请确认esp32模组型号为wrover且须在arduino->开发板中选择ESP32 Wrover Module");
    delay(500);
    stopBeep();
    while (1);
  }
  startBeep();
  delay(50);
  stopBeep();
  //paperang_core0();
  paperang_app();
}

void loop(void) {
  //server.handleClient();
  //  delay(1);
}
