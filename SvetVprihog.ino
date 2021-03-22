#include "RTClib.h"
#include <SPI.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiSoftSpi.h"
#include <Bounce2.h>
#include <EEPROM.h>
//пины spi oled 0.96 экрана
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
#define BUTTON_PIN 3       //кнопка изменения времени
#define BUTTON_PIN_DVER 4
#define pressed_long 1500   //время длительного нажатия
unsigned long pressed_moment; //сколько жмем
#define LAMP 8
#define LED 6
#define PIR 7
#define PIR1 5

Bounce button = Bounce();
Bounce buttondver = Bounce();

SSD1306AsciiSoftSpi displays;          //структуры экрана
RTC_DS3231 rtc;                         //структуры времени
unsigned long previousMillis = 0;
const long interval = 100;              //как часто выполнять код проверки срабатывания датчика движения

byte rowin = 0;                         //для хранения строки двоеточия в часах
byte colin = 0;                         //для хранения столбца двоеточия в часах
byte timsec = 0;                   //мигнули или нет
bool lights = false;  //включен свет или выключен
unsigned long timeDiodOn = 0;
unsigned long timeDiodOff = 6;
bool lightsOnAlways = false;

unsigned long delayLight = 10000;
unsigned long lightOnTime = 0 ;
unsigned long AlwaysOnTime = 0 ;

void setup()   {
  //Serial.begin(9600);
  digitalWrite(LED, LOW);  //выкл
  digitalWrite(LAMP, LOW); //выкл
  displays.begin(&Adafruit128x64, OLED_CS, OLED_DC, OLED_CLK, OLED_MOSI, OLED_RESET);
  displays.setContrast(0);
  displays.setFont(Adafruit5x7);
  pinMode(PIR, INPUT); //детектор движения
  pinMode(PIR1, INPUT); //детектор движения1
  pinMode(LAMP, OUTPUT); //Лампа
  pinMode(LED, OUTPUT);   //Светодиод
  pinMode(3, INPUT);    //кнопка
  button.attach(BUTTON_PIN);     //дебоунсинг кнопки
  button.interval(20);

    pinMode(BUTTON_PIN_DVER, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  buttondver.attach(BUTTON_PIN_DVER);
  buttondver.interval(500); // interval in ms





  EEPROM.get(0,delayLight);
  if (delayLight > 300000) {
    delayLight = 10000;
    EEPROM.put(0, delayLight);
  }

  EEPROM.get(4,timeDiodOn);
  if (timeDiodOn >= 24) {
    timeDiodOn = 0;
    EEPROM.put(4, timeDiodOn);
  }


  EEPROM.get(8,timeDiodOff);
  if (timeDiodOn >= 24) {
    timeDiodOff = 6;
    EEPROM.put(8, timeDiodOff);
  }

  displays.clear();
  if (! rtc.begin()) {
    displays.println("Couldn't find RTC");
    while (1);
    delay(1000);
  }

}
void loop() {
  if (buttondver.update()) {
    if (button.read() == 0 and digitalRead(BUTTON_PIN_DVER) == 1) { //если кнопка отпущена - дверь открыта
      digitalWrite(LAMP, HIGH); //включить лампу
      digitalWrite(LED, LOW);  //выключить светодиод
      printtime();          //вывести время дату
      lights = true;        //запомним что свет включен
      lightOnTime = millis();  //Время включения света запомним для подсчета задержки влюченного света
    }

  }


  if (button.update()) { //если произошло событие
    if (button.read() == 1) { //если кнопка нажата
      pressed_moment = millis(); // запоминаем время нажатия
    }
    else   { // кнопка отпущена
      if ((millis() - pressed_moment) < pressed_long) {  // если кнопка была нажата кратковременно
        //включаем свет на постоянно
        digitalWrite(LED, LOW);  //выключить светодиод
        digitalWrite(LAMP, HIGH); //включить лампу постоянно
        bool lightsOnW = true;
        displays.clear();
        displays.set2X();
        displays.setFont(System5x7);
        displays.println(utf8rus("  включен"));
        displays.println(utf8rus(" на зо мин"));
        displays.setFont(Adafruit5x7);
        AlwaysOnTime = millis();
        while (lightsOnW) {


          if (millis() - AlwaysOnTime >= 1800000) {     //Если прошло полчаса, пора отключать постоянно включенный свет
            lightsOnW = false;
            displays.clear();
            displays.setFont(System5x7);
            displays.println(utf8rus("  включен"));
            displays.println(utf8rus("  сенсор"));
            displays.println(utf8rus("   авто"));
            displays.setFont(Adafruit5x7);
            delay(3000);
            break;   //Прерываем цикл в этом месте
          }




          if (button.update()) { //если произошло событие
            if (button.read() == 1) { //если кнопка нажата
              //ничего не делаем
            }
            else {
              lightsOnW = false;
              displays.clear();
              displays.setFont(System5x7);
              displays.println(utf8rus("  включен"));
              displays.println(utf8rus("  сенсор"));
              displays.println(utf8rus("   авто"));
              displays.setFont(Adafruit5x7);
              delay(3000);
            }
          }
        }


        displays.clear();
        digitalWrite(LED, LOW);  //выключить светодиод
        digitalWrite(LAMP, LOW); //выключить лампу
        lights = false;         //света нет
      }
      else
      { // кнопка удерживалась долго, начинаем установку времени и даты
        int years, months, days, hours, minuts;
        years = 2019;
        months = 12;
        days = 8;
        hours = 22;
        minuts = 12;
        EEPROM.get(0,delayLight);
        EEPROM.get(4,timeDiodOn);
        EEPROM.get(8,timeDiodOff);
        displays.clear();
        displays.set2X();
        displays.setFont(System5x7);
        displays.println(utf8rus("установка"));
        displays.println(utf8rus("дня"));
        displays.setFont(Adafruit5x7);
        bool setday = true;
        while (setday) {         //цикл пока не установили день и нажали длинно кнопку
          displays.setCursor(0, 5);
          if (days < 10) {
            displays.print("0");
          }
          displays.print(days);


          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                days++; // увеличиваем день
                if (days >= 32) days = 1;
              }
              else
              { // кнопка удерживалась долго, переходим к установке месяца
                setday = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus(" установка"));
                displays.println(utf8rus("  месяца"));
                displays.setFont(System5x7);
                break;
              }
            }
          }
        }     //Закончили с датой

        bool setmonth = true;

        while (setmonth) {         //цикл пока не установили месяц и нажали длинно кнопку
          displays.setCursor(0, 5);
          switch (months) {
            case 1:
              displays.print(utf8rus("январь"));
              break;
            case 2:
              displays.print(utf8rus("февраль"));
              break;
            case 3:
              displays.print(utf8rus("март"));
              break;
            case 4:
              displays.print(utf8rus("апрель"));
              break;
            case 5:
              displays.print(utf8rus("май"));
              break;
            case 6:
              displays.print(utf8rus("июнь"));
              break;
            case 7:
              displays.print(utf8rus("июль"));
              break;
            case 8:
              displays.print(utf8rus("август"));
              break;
            case 9:
              displays.print(utf8rus("сентябрь"));
              break;
            case 10:
              displays.print(utf8rus("октябрь"));
              break;
            case 11:
              displays.print(utf8rus("ноябрь"));
              break;
            case 12:
              displays.print(utf8rus("декабрь"));
              break;
          }
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                months++; // увеличиваем месяц
                if (months >= 13) months = 1;
                displays.setCursor(0, 5);
                displays.clearToEOL();
              }
              else
              { // кнопка удерживалась долго, переходим к установке года
                setmonth = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus("установка"));
                displays.println(utf8rus("года"));
                displays.setFont(Adafruit5x7);
                break;
              }
            }
          }
        }     //Закончили с месяцем


        bool setyear = true;
        while (setyear) {         //цикл пока не установили год и нажали длинно кнопку
          displays.setCursor(0, 5);
          displays.print(years);
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                years++; // увеличиваем год
                if (years >= 2045) years = 2017;
              }
              else
              { // кнопка удерживалась долго, переходим дальше
                setyear = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus("установка"));
                displays.println(utf8rus("часа"));
                displays.setFont(Adafruit5x7);
                break;
              }
            }
          }
        }     //Закончили с годом

        bool sethours = true;
        while (sethours) {         //цикл пока не установили час и нажали длинно кнопку
          displays.setCursor(0, 5);
          if (hours < 10) {
            displays.print("0");
          }
          displays.print(hours);
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                hours++; // увеличиваем час
                if (hours >= 24) hours = 0;
              }
              else
              { // кнопка удерживалась долго
                sethours = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus("установка"));
                displays.println(utf8rus("минут"));
                displays.setFont(Adafruit5x7);
                break;
              }
            }
          }
        }     //Закончили с часом

        bool setminuts = true;
        while (setminuts) {         //цикл пока не установили минуты и нажали длинно кнопку
          displays.setCursor(0, 5);
          if (minuts < 10) {
            displays.print("0");
          }
          displays.print(minuts);
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                minuts++; // увеличиваем минуты
                if (minuts >= 60) minuts = 0;
              }
              else
              { // кнопка удерживалась долго
                setminuts = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus("установка"));
                displays.println(utf8rus("задержки"));
                displays.setFont(Adafruit5x7);
                break;
              }
            }
          }
        }     //Закончили с минутами
        
        bool setdelay = true;
        int clears = 0;
        while (setdelay) {         //цикл пока не установили задержку и нажали длинно кнопку
          displays.setCursor(0, 5);
          displays.print(delayLight / 1000);
          displays.setFont(System5x7);
          displays.print(utf8rus(" сек"));
          displays.setFont(Adafruit5x7);
          displays.print(".");
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                delayLight = delayLight + 10000; // увеличиваем задержку
                if (delayLight >= 300000) {
                  delayLight = 10000;
                  displays.setCursor(0, 5);
                  displays.print("        ");
                }

              }
              else
              { // кнопка удерживалась долго
                setdelay = false;
                pressed_moment = 0;
                displays.clear();
                displays.setFont(System5x7);
                displays.println(utf8rus("установка"));
                displays.println(utf8rus("ноч свет"));
                displays.setFont(Adafruit5x7);
                EEPROM.put(0, delayLight);
                break;
              }
            }
          }
        }     //Закончили с задержкой





        bool setStartDiodOn = true;
        while (setStartDiodOn) {         //цикл пока не установили время скоторого начинает работать диод и нажали длинно кнопку
          displays.setCursor(0, 5);
          displays.print("c ");
          if (timeDiodOn < 10) {
            displays.print("0");
          }
          displays.print(timeDiodOn);
          displays.setFont(System5x7);
          displays.println(utf8rus(" час"));
          displays.setFont(Adafruit5x7);
          
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                timeDiodOn = timeDiodOn + 1; // увеличиваем задержку
                if (timeDiodOn >= 24) {
                  timeDiodOn = 0;
                  displays.setCursor(0, 5);
                  displays.print("        ");
                }

              }
              else
              { // кнопка удерживалась долго
                setStartDiodOn = false;
                pressed_moment = 0;
                displays.setCursor(0, 5);
                displays.print("        ");
                EEPROM.put(4, timeDiodOn);
                break;
              }
            }
          }
        }


        bool setStartDiodOff = true;
        while (setStartDiodOff) {         //цикл пока не установили время до которого будет работать диод и нажали длинно кнопку
          displays.setCursor(0, 5);
          displays.setFont(System5x7);
          displays.print(utf8rus("до "));
          displays.setFont(Adafruit5x7);
          if (timeDiodOff < 10) {
            displays.print("0");
          }
          else if (timeDiodOff >= 24) {
            timeDiodOff = 0;
          }
          displays.print(timeDiodOff);
          displays.setFont(System5x7);
          displays.print(utf8rus(" час"));
          displays.setFont(Adafruit5x7);
          if (button.update())   { //если произошло событие
            if (button.read() == 1)
            { //если кнопка нажата
              pressed_moment = millis(); // запоминаем время нажатия
            }
            else
            { // кнопка отпущена
              if ((millis() - pressed_moment) < pressed_long)
              { // если кнопка была нажата кратковременно
                timeDiodOff = timeDiodOff + 1; // увеличиваем задержку
                if (timeDiodOff >= 24) {
                  timeDiodOff = 6;
                  displays.setCursor(0, 5);
                  displays.print("        ");
                }

              }
              else
              { // кнопка удерживалась долго
                setStartDiodOff = false;
                pressed_moment = 0;
                EEPROM.put(8, timeDiodOff);
                break;
              }
            }
          }
        }







        rtc.adjust(DateTime(years, months, days, hours, minuts, 0)); //Устанавливаем время

        displays.clear();
        displays.setFont(System5x7);
        displays.println(utf8rus("время"));
        displays.println(utf8rus("установлено"));
        displays.setFont(Adafruit5x7);
        delay(3000);
        displays.clear();
        digitalWrite(LED, LOW);  //выключить светодиод
        digitalWrite(LAMP, LOW); //выключить лампу
        lights = false;         //света нет
        delay(3000);
      }

    }

  }

  unsigned long currentMillis = millis();

  // if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;
  byte val1 = digitalRead(PIR1); //digitalRead(PIR1);//смотрим не включен ли датчик движения1
  byte val = digitalRead(PIR);//смотрим не включен ли датчик движения

  DateTime now = rtc.now();  //получаем время и дату

  if (now.hour() >= 0 and now.hour() <= 6) { //Если ночь и сработал датчик движения и светодиод не включен
    if (val == HIGH or val1 == HIGH) {
      if (lights == false) {
        digitalWrite(LED, HIGH);  //включить светодиод
        printtime();              //включить экран вывести время
        lights = true;            //запомним что свет включен
        lightOnTime = millis();  //Время включения света запомним для подсчета задержки влюченного света
      }
    }
  }



  if (now.hour() >= 0 and now.hour() <= 6) { //если ночь
    if ( val == LOW and val1 == LOW ) {  //и датчик движения никого не видит
      if (lights == true and digitalRead(BUTTON_PIN_DVER) == 0) {  //и светодиод включен и дверь закрыта
        if (millis() - lightOnTime >= delayLight) {  //  30сек или больше
          digitalWrite(LED, LOW);  //выключить светодиод
          if (digitalRead(LAMP) == HIGH) digitalWrite(LAMP, LOW);         //выключить лампу при переходе за 0 часов
          lights = false;         //запомним что свет выключен
          displays.clear();       //выключим экран
          lightOnTime = 0 ;        //сбросим время начала включения света
        }
      }
    }
  }

  if (now.hour() > 6) { //если день
    if (val == HIGH or val1 == HIGH) {//и cработал датчик движения
      if (lights == false) {//и свет не включен
        digitalWrite(LAMP, HIGH); //включить лампу
        printtime();          //вывести время дату
        lights = true;        //запомним что свет включен
        lightOnTime = millis();  //Время включения света запомним для подсчета задержки влюченного света
      }
    }
  }




  if (now.hour() > 6) {         //если день
    if (val == LOW and val1 == LOW) { //и никого не видно
      if (lights == true and digitalRead(BUTTON_PIN_DVER) == 0) { //и лампа включена и дверь закрыта
        if (millis() - lightOnTime >= delayLight) {//30 или больше сек
          digitalWrite(LAMP, LOW);         //выключить лампу
          if (digitalRead(LED) == HIGH) digitalWrite(LED, LOW);  //выключить светодиод при переходе за 6 часов
          lights = false;                  //запомним что свет выключен
          displays.clear();               //выключить экран
          lightOnTime = 0 ;               //сбросим время начала включения света
        }
      }
    }
  }

  timsec++;
  //мигаем двоеточием в месте его вывода в часах если экран включен
  if (lights == true and timsec == 5) {

    displays.setCursor(colin, rowin);
    displays.print(":");
  }
  if (lights == true and timsec >= 10) {
    displays.setCursor(colin, rowin);
    displays.print(" ");


  }
  if (timsec == 10) {
    timsec = 0;
  }
  /////////////////



  // }
}

//вывод русского текста прописного
#define maxString 21
char target[maxString + 1] = "";

char *utf8rus(char *source) {
  int i, j, k;
  unsigned char n;
  char m[2] = { '0', '\0' };

  strcpy(target, ""); k = strlen(source); i = j = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
            n = source[i]; i++;
            if (n == 0x81) {
              n = 0xA8;
              break;
            }
            if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
            break;
          }
        case 0xD1: {
            n = source[i]; i++;
            if (n == 0x91) {
              n = 0xB8;
              break;
            }
            if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
            break;
          }
      }
    }

    m[0] = n; strcat(target, m);
    j++; if (j >= maxString) break;
  }
  return target;
}

void printtime() {
  DateTime now = rtc.now();

  displays.clear();
  displays.set2X();
  displays.setFont(Adafruit5x7);
  displays.setCol(5);
  if  (now.day() >= 0 and now.day() < 10) {   //вывод ведущих нулей в дате и часах
    displays.print("0");
  }
  displays.print(now.day());
  displays.print('.');
  if  (now.month() >= 0 and now.month() < 10) {
    displays.print("0");
  }
  displays.print(now.month());
  displays.print('.');
  displays.println(now.year());
  displays.setFont(System5x7);
  displays.setCol(18);
  displays.setRow(3);
  switch (now.month()) {
    case 1:
      displays.println(utf8rus("январь"));
      break;
    case 2:
      displays.println(utf8rus("февраль"));
      break;
    case 3:
      displays.println(utf8rus("март"));
      break;
    case 4:
      displays.println(utf8rus("апрель"));
      break;
    case 5:
      displays.println(utf8rus("май"));
      break;
    case 6:
      displays.println(utf8rus("июнь"));
      break;
    case 7:
      displays.println(utf8rus("июль"));
      break;
    case 8:
      displays.println(utf8rus("август"));
      break;
    case 9:
      displays.println(utf8rus("сентябрь"));
      break;
    case 10:
      displays.println(utf8rus("октябрь"));
      break;
    case 11:
      displays.println(utf8rus("ноябрь"));
      break;
    case 12:
      displays.println(utf8rus("декабрь"));
      break;
  }
  displays.setFont(Adafruit5x7);
  displays.setCol(35);
  displays.setRow(6);
  if  (now.hour() >= 0 and now.hour() < 10) {
    displays.print("0");
  }
  displays.print(now.hour());
  rowin = displays.row();     //запомним где двоеточие в часах
  colin = displays.col();     //колонка и столбец
  displays.print(':');
  if  (now.minute() >= 0 and now.minute() < 10) {
    displays.print("0");
  }
  displays.print(now.minute());
}

// чтение
unsigned long EEPROM_ulong_read(int addr) {
  byte raw[4];
  for (byte i = 0; i < 4; i++) raw[i] = EEPROM.read(addr + i);
  unsigned long &num = (unsigned long&)raw;
  return num;
}

// запись
void EEPROM_ulong_write(int addr, unsigned long num) {
  byte raw[4];
  (unsigned long&)raw = num;
  for (byte i = 0; i < 4; i++) EEPROM.write(addr + i, raw[i]);
}
