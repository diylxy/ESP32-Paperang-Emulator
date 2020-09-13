#include <BluetoothSerial.h>
#include "Arduino_CRC32.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "esp_task_wdt.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define MOTOR_TIME 3000

extern uint8_t *printData;
extern uint32_t printDataCount;

#define START_BYTE 0x02
#define END_BYTE 0x03
#define PRINTER_SN "P1001705253855"
#define PRINTER_NAME "P1"
uint8_t PRINTER_BATTERY = 90;
#define COUNTRY_NAME "CN"
char *CMD_42_DATA PROGMEM = "BK3432";
uint8_t CMD_7F_DATA[] PROGMEM = {0x76, 0x33, 0x2e, 0x33, 0x38, 0x2e, 0x31, 0x39, 0, 0, 0, 0};
uint8_t CMD_81_DATA[] PROGMEM = {0x48, 0x4d, 0x45, 0x32, 0x33, 0x30, 0x5f, 0x50, 0x32, 0, 0, 0, 0, 0, 0, 0};
uint8_t PRINTER_VERSION[] PROGMEM = {0x08, 0x01, 0x01};
uint8_t *CMD_40_DATA PROGMEM = {0x00};
uint32_t power_down_time = 3600;
uint8_t heat_density = 75;

#define PRINT_DATA 0
#define PRINT_DATA_COMPRESS 1
#define FIRMWARE_DATA 2
#define USB_UPDATE_FIRMWARE 3
#define GET_VERSION 4
#define SENT_VERSION 5
#define GET_MODEL 6
#define SENT_MODEL 7
#define GET_BT_MAC 8
#define SENT_BT_MAC 9
#define GET_SN 10
#define SENT_SN 11
#define GET_STATUS 12
#define SENT_STATUS 13
#define GET_VOLTAGE 14
#define SENT_VOLTAGE 15
#define GET_BAT_STATUS 16
#define SENT_BAT_STATUS 17
#define GET_TEMP 18
#define SENT_TEMP 19
#define SET_FACTORY_STATUS 20
#define GET_FACTORY_STATUS 21
#define SENT_FACTORY_STATUS 22
#define SENT_BT_STATUS 23
#define SET_CRC_KEY 24
#define SET_HEAT_DENSITY 25
#define FEED_LINE 26
#define PRINT_TEST_PAGE 27
#define GET_HEAT_DENSITY 28
#define SENT_HEAT_DENSITY 29
#define SET_POWER_DOWN_TIME 30
#define GET_POWER_DOWN_TIME 31
#define SENT_POWER_DOWN_TIME 32
#define FEED_TO_HEAD_LINE 33
#define PRINT_DEFAULT_PARA 34
#define GET_BOARD_VERSION 35
#define SENT_BOARD_VERSION 36
#define GET_HW_INFO 37
#define SENT_HW_INFO 38
#define SET_MAX_GAP_LENGTH 39
#define GET_MAX_GAP_LENGTH 40
#define SENT_MAX_GAP_LENGTH 41
#define GET_PAPER_TYPE 42
#define SENT_PAPER_TYPE 43
#define SET_PAPER_TYPE 44
#define GET_COUNTRY_NAME 45
#define SENT_COUNTRY_NAME 46
#define DISCONNECT_BT 47
#define GET_DEV_NAME 48
#define SENT_DEV_NAME 49
#define CMD_39 57
#define CMD_40 64
#define CMD_41 65
#define CMD_42 66
#define CMD_43 67
#define CMD_7F 127
#define CMD_80 128
#define CMD_81 129
#define CMD_82 130

BluetoothSerial SerialBT;
Arduino_CRC32 crc32;
uint8_t dataPack[512];
uint32_t dataPack_len;
uint32_t crc32_result;
void paperang_send(void)
{
  SerialBT.write(dataPack, dataPack_len);
}

void paperang_send_ack(uint8_t type)
{
  uint8_t ackcrc = 0;
  dataPack[0] = START_BYTE;
  dataPack[1] = type;
  dataPack[2] = 0x00;
  dataPack[3] = 0x01;
  dataPack[4] = 0x00;
  dataPack[5] = 0x00;
  crc32_result = crc32.calc(&ackcrc, 1);
  memcpy(dataPack + 6, (uint8_t *)&crc32_result, 4);
  dataPack[10] = END_BYTE;
  dataPack_len = 11;
  paperang_send();
}

void paperang_send_msg(uint8_t type, const uint8_t* dat, uint16_t len)
{
  dataPack[0] = START_BYTE;
  dataPack[1] = type;
  dataPack[2] = 0x00;
  memcpy(dataPack + 3, (uint8_t *)&len, 2);
  memcpy(dataPack + 5, dat, len);
  dataPack_len = 5 + len;
  crc32_result = crc32.calc(dat, len);
  memcpy(dataPack + dataPack_len, (uint8_t *)&crc32_result, 4);
  dataPack[dataPack_len + 4] = END_BYTE;
  dataPack_len += 5;
  paperang_send();
}

struct {
  uint8_t packType;
  uint8_t packIndex;
  uint16_t dataLen;
} packHeader;
uint8_t gotStartByte = 0;
uint8_t c;
uint16_t readpos = 0;
uint8_t dataPack_read[2048];
//#define PRINT_DATA_CACHE_SIZE 10240
//uint8_t printDataCache[PRINT_DATA_CACHE_SIZE + 1024];
//volatile uint16_t cacheOverflowed = 0;
//volatile uint16_t printDataCacheHead = 0;
//volatile uint16_t printDataCacheFoot = 0;
uint16_t dataPack_read_pos = 0;
//volatile uint8_t cacheLock = 0;
void paperang_process_data()
{
  uint32_t tmp32;
  switch (packHeader.packType)
  {
    case PRINT_DATA:
      /*
        //while (cacheLock);
        //cacheLock = 1;
        memcpy(printDataCache + printDataCacheHead, dataPack_read, packHeader.dataLen);
        printDataCacheHead += packHeader.dataLen;
        if (printDataCacheHead >= PRINT_DATA_CACHE_SIZE)
        {
          printDataCacheHead = 0;
          cacheOverflowed = printDataCacheHead - PRINT_DATA_CACHE_SIZE;
        }
        //cacheLock = 0;*/
      return;
    case SET_CRC_KEY:
      tmp32 = dataPack_read[0] << 24 + dataPack_read[1] << 16 + dataPack_read[2] << 8 + dataPack_read[3];
      crc32.init(tmp32);
      break;
    case GET_VERSION:
      paperang_send_msg(SENT_VERSION, PRINTER_VERSION, 3);
      break;
    case GET_DEV_NAME:
      paperang_send_msg(SENT_DEV_NAME, (uint8_t *)PRINTER_NAME, 2);
      break;
    case GET_SN:
      paperang_send_msg(SENT_SN, (uint8_t *)PRINTER_SN, strlen((char *)PRINTER_SN));
    case GET_POWER_DOWN_TIME:
      paperang_send_msg(SENT_POWER_DOWN_TIME, (uint8_t *)&power_down_time, 2);
      break;
    case GET_BAT_STATUS:
      paperang_send_msg(SENT_BAT_STATUS, &PRINTER_BATTERY, 1);
      break;
    case GET_COUNTRY_NAME:
      paperang_send_msg(SENT_COUNTRY_NAME, (uint8_t *)COUNTRY_NAME, 2);
      break;
    case CMD_42:
      paperang_send_msg(CMD_43, (uint8_t *)CMD_42_DATA, strlen(CMD_42_DATA));
      break;
    case CMD_7F:
      paperang_send_msg(CMD_80, CMD_7F_DATA, 12);
      break;
    case CMD_81:
      paperang_send_msg(CMD_82, CMD_81_DATA, 16);
      break;
    case CMD_40:
      paperang_send_msg(CMD_41, CMD_40_DATA, 1);
      break;
    case SET_POWER_DOWN_TIME:
      power_down_time = dataPack_read[0] << 8 + dataPack_read[1];
      break;
    case SET_HEAT_DENSITY:
      heat_density = dataPack_read[0];
      break;
    case GET_HEAT_DENSITY:
      paperang_send_msg(SENT_HEAT_DENSITY, &heat_density, 1);
      break;
    case FEED_LINE:
      if (printDataCount / 48 != 0) {
        startPrint();
        goFront(200, MOTOR_TIME);
      }
      printDataCount = 0;
      break;
    default:
      break;
  }
  paperang_send_ack(packHeader.packType);
}
/*
  void paperang_copy(void *pvParameters)
  {
  (void) pvParameters;
  esp_task_wdt_init(1024000, false);
  uint32_t newDataCount;
  uint16_t tmp16;
  while (1)
  {
    if (printDataCacheHead != printDataCacheFoot)
    {
      //while (cacheLock);
      //cacheLock = 1;
      if (cacheOverflowed)
      {
        tmp16 = printDataCacheFoot;
        newDataCount = PRINT_DATA_CACHE_SIZE - printDataCacheFoot + cacheOverflowed;
        printDataCacheFoot = 0;
        cacheOverflowed = 0;
        memcpy(printData + printDataCount, printDataCache + tmp16, newDataCount);
        printDataCount += newDataCount;
        tmp16 = printDataCacheHead;
        memcpy(printData + printDataCount, printDataCache, tmp16);
        printDataCount += tmp16;
      }
      else
      {
        newDataCount = printDataCacheHead - printDataCacheFoot;
        tmp16 = printDataCacheFoot;
        printDataCacheFoot = printDataCacheHead;
        memcpy(printData + printDataCount, printDataCache + tmp16, newDataCount);
        printDataCount += newDataCount;
      }
      //cacheLock = 0;
    }
    //vTaskDelay(20);
  }
  }
*/
void paperang_core0()
{
  /*
    xTaskCreatePinnedToCore(
    paperang_copy
    ,  "Paperang_copy"   // A name just for humans
    ,  4096
    ,  NULL
    ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  0);
  */

}

void paperang_app()
{
  uint16_t i = 0;
  SerialBT.begin("Paperang"); //Bluetooth device name
  //重新设置class of device
  esp_bt_cod_t cod;
  cod.major = 6;                   //主设备类型
  cod.minor = 0b100000;            //次设备类型
  cod.service = 0b00000100000;     //服务类型
  esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);
  crc32.init(0x35769521);
  packHeader.dataLen = 0;
  Serial.println();
  Serial.println("3秒内输入要测试的STB序号开始打印测试页（1-6）: ");
  delay(3000);
  if(Serial.available())
  {
    char chr = Serial.read();
    if(chr >= '1' && chr <= '6')
    {
      chr -= 0x30;
      testPage((uint8_t)chr-1);
    }
    else if(chr == 'a')              //确认STB位置
    {
      testSTB();
    }
    else if(chr == 'A')              //确认STB位置
    {
      testPage(0);
      testPage(1);
      testPage(2);
      testPage(3);
      testPage(4);
      testPage(5);
    }
    Serial.flush();
  }
  while (1) {
    if (SerialBT.available()) {
      c = SerialBT.read();
      if (c == START_BYTE && gotStartByte == 0)
      {
        gotStartByte = 1;
        readpos = 0;
      }
      else if (readpos == 1)
      {
        packHeader.packType = c;
        //Serial.println(packHeader.packType);
        packHeader.packIndex = SerialBT.read();
        packHeader.dataLen = SerialBT.read();
        packHeader.dataLen += SerialBT.read() << 8;
        //Serial.println(packHeader.dataLen);
      }
      else if (readpos == 2 && packHeader.dataLen != 0 && packHeader.dataLen < 2048)
      {
        i = packHeader.dataLen - 1;
        if (packHeader.packType == PRINT_DATA)
        {
          printData[printDataCount++] = c;
          while (i) {
            while (SerialBT.available() == 0);
            printData[printDataCount++] = SerialBT.read();
            --i;
          }
        }
        else
        {
          dataPack_read[dataPack_read_pos++] = c;
          while (i) {
            dataPack_read[dataPack_read_pos++] = SerialBT.read();
            --i;
          }
        }
      }
      else if (readpos < 7)
      {
        ;
      }
      else if (c == END_BYTE && readpos == 7)
      {
        paperang_process_data();
        gotStartByte = 0;
        dataPack_read_pos = 0;
        readpos = 0;
        packHeader.dataLen = 0;
      }
      else
      {
        Serial.println("ERROR");
        gotStartByte = 0;
        dataPack_read_pos = 0;
        readpos = 0;
        packHeader.dataLen = 0;
      }
      if (gotStartByte == 1)
        ++readpos;
    }
  }
}
