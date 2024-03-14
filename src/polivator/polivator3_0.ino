// пример с хранением данных в базе Pairs
#include <Arduino.h>

#ifdef ESP32
#define RESET() ESP.restart()
#else
#define RESET() ESP.reset()
#endif

#define BTN_LEVEL 0 // eсли сенсорная - 1. А так 0

#define PIN_OUT 2 // 22
#define PIN_IN A0 // 34
#define _BTN 14    // 17

#define PERIOD 1000
#define PERIOD2 2000

#define GH_INCLUDE_PORTAL
#include <GyverHub.h>
GyverHub hub;

#include <PairsFile.h>
// pairsfile - автоматически сохраняет базу данных в файл
PairsFile data(&GH_FS, "/data.dat", 3000);
// Pairs data;  // есть без привязки к файлу

bool iswork;
bool isdr;
bool off;
int val, val2;
uint32_t tmrw;
bool f1w, f2w;

#include <EncButton.h>
ButtonT<_BTN> btn;//(BTN_LEVEL);

#include "Timer.h"

// билдер
void build(gh::Builder& b) {
  // для привязки к базе данных достаточно сделать именованный виджет
  // и передать базу данных по адресу. Данные будут сами писаться и читаться по ключу
  {
    gh::Row r(b);
    if (b.Button().label("Полить").click()) {
      work();
      f1w = true;
      f2w = true;
      iswork = true;
      tmrw = millis();
      hub.sendPush("Начался полив");
    }
    if (b.Button().label("Выключить").click()) {
      f1w = false;
      f2w = false;
      iswork = false;
      off = true;
      Serial.println("OFF");
      digitalWrite(PIN_OUT, (bool)data["rvsr"]);
    }
    /*if (*/b.Slider_("timework", &data).label("Время полива").size(5);//.click()) {
    //  Serial.println(data["timework"]);
    //}
  }
  {
    gh::Row r(b);
    b.Switch_("rvs", &data).label("Реверс датчика");
    b.Switch_("rvsr", &data).label("Реверс реле");
  }
  {
    gh::Row r(b);
    val2 = val;
    b.GaugeLinear_("value", &val2).range(0, 100, 1).label("Значение");
    if (b.Button().label("Обновить").click()) b.refresh();
  }
  {
    gh::Row r(b);
    b.LED_("led", &iswork).label("Индикатор полива (ожидание)");
    b.LED_("led2", &isdr).label("Индикатор работы (реальность)");
  }
  b.Slider_("step", &data).range(0, 100, 1).label("Порог");
  // Это всё!!
  /*
    // ещё парочку
    {
      gh::Row r(b);
      b.Slider_("slider", &data);
      b.Spinner_("spinner", &data);
      b.Switch_("switch", &data);
    }*/
  b.Input_("ssid", &data).label("SSID");
  b.Pass_("pass", &data).label("PASS");

  // выведем содержимое базы данных как текст
  b.Text_("pairs", data);

  // обновить текст при действиях на странице
  if (b.changed()) hub.update("pairs").value(data);
}

void setup() {
  delay(1000);
  Serial.begin(115200);         // запускаем сериал для отладки
  pinMode(PIN_OUT, OUTPUT);
  //pinMode(PIN_IN, INPUT);

  // запустить и прочитать базу из файла
  LittleFS.begin();
  data.begin();
  //Serial.println(data);
  startup();
}

void loop() {
  _loop();
  work();
}

void work() {
  if (millis() - tmrw >= (int)data["timework"] * 1000 && f1w) {
    f1w = false;
    iswork = false;
    hub.sendUpdate("led");
    Serial.println("OFF");
    digitalWrite(PIN_OUT, (bool)data["rvsr"]);
  }
  if (millis() - tmrw < (int)data["timework"] * 1000 && f2w) {
    f2w = false;
    Serial.println("ON");
    hub.sendUpdate("led");
    digitalWrite(PIN_OUT, !((bool)data["rvsr"]));
  }
  if (off) {
    hub.sendUpdate("led");
    off = false;
  }
}

void _loop() {
  hub.tick();
  btn.tick();

  // файл сам обновится по таймауту
  data.tick();

  static uint32_t tmr;
  if (millis() - tmr >= PERIOD) {
    tmr = millis();
    //Serial.print("Rabotaet  ");
    val = analogRead(PIN_IN) * 100 / 4096;
    if (data["rvs"]) val = 100 - val;
    //Serial.println(val);
  }
  static uint32_t tmr2;
  if (millis() - tmr2 >= PERIOD2) {
    tmr2 = millis();
    hub.sendUpdate("value");
    isdr = digitalRead(PIN_OUT);
    hub.sendUpdate("led2");
    //hub.refresh();
  }
  static bool f3w = true;
  if (val <= (int)data["step"] && f3w) {
    f3w = false;
    f1w = true;
    f2w = true;
    iswork = true;
    tmrw = millis();
    hub.update("led");
    //hub.refresh();
    hub.sendPush("Начался полив");
  }
  if (val > (int)data["step"] && !f3w) f3w = true;
}
