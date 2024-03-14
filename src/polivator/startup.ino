void startup() {
  // я хз, хранить IPAddress в памяти приводит к exception
  // так что вытаскиваем в IPAddress
  /*if (data["ip0"] == "") data["ip0"] = 0;
    if (data["ip1"] == "") data["ip1"] = 0;
    if (data["ip2"] == "") data["ip2"] = 0;
    if (data["ip3"] == "") data["ip3"] = 0;
    IPAddress ip = IPAddress(data["ip0"], data["ip1"], data["ip2"], data["ip3"]);*/

  // таймер на 3 секунды перед подключением,
  // чтобы юзер успел кликнуть если надо
  Timer tmr_(3000);
  Serial.println("тычь");
  while (!tmr_.period()) {
    btn.tick();
    data.tick();
    yield();
    if (btn.click()) localPortal(); // клик - запускаем портал
    // дальше код не пойдёт, уйдем в перезагрузку
  }
  Serial.println("не тычь");

  // юзер не кликнул, пытаемся подключиться к точке
  Serial.println("Connecting...");
  WiFi.mode(WIFI_STA);
  Serial.println(data["ssid"]);
  WiFi.begin(data["ssid"], data["pass"]);

  tmr_.setPeriod(20000);
  tmr_.restart();

  while (WiFi.status() != WL_CONNECTED) {
    btn.tick();
    data.tick();
    yield();
    //Serial.print(".");
    // если клик по кнопке или вышел таймаут
    if (btn.click() || tmr_.period()) {
      WiFi.disconnect();  // отключаемся
      localPortal();    // открываем портал
      // дальше код не пойдёт, уйдем в перезагрузку
    }
  }
  Serial.println();
  Serial.print(F("Connected! IP: "));
  Serial.println(WiFi.localIP());
  /*
    // переписываем удачный IP себе в память
    if (ip != WiFi.localIP()) {
      ip = WiFi.localIP();
      for (int i = 0; i < 4; i++) data["ip"+i] = ip[i];
      memory.update();
    }*/
  randomSeed(micros());

  // стартуем хаб

  hub.mqtt.config("test.mosquitto.org", 1883);  // + MQTT
  hub.config(F("MyDevices"), F("ESP"));
  hub.onBuild(build);
  hub.setPIN(getPass());
  hub.begin();
}

// локальный запуск портала. При любом исходе заканчивается ресетом платы
void localPortal() {
  // создаём точку с именем WLamp и предыдущим успешным IP
  Serial.println(F("Create AP"));
  WiFi.mode(WIFI_AP);
  WiFi.softAP("polivator");

  hub.config(F("MyDevices"), F("ESP"));
  hub.onBuild(build);  // подключаем интерфейс
  hub.begin();

  while (1) {  // портал работает
    _loop();
    work();
    yield();
    // если нажали сохранить настройки или кликнули по кнопке
    // перезагружаем ESP
    if (btn.click()) RESET();
  }
}

int getPass() {
  int hpin = random(1000, 9999);
  if (data["hubpin"] == "") data["hubpin"] = String(hashCode(String(hpin)));
  //Serial.println(data["hubpin"]);
  Serial.print("PIN: ");
  uint32_t pinc = deshifr((uint32_t)data["hubpin"]);
  Serial.println(pinc);
  return pinc;
}

uint32_t minus(uint32_t a, uint32_t b) {
  if (a > b) return (uint32_t)a - b;
  else return 0;
}

uint32_t hashCode(String txt) {
  if (txt.length() == 0)
    return 0;
  uint32_t hash = 0;
  char* _txt = (char*)txt.c_str();
  for (int i = 0; i < txt.length(); i++) {
    hash = (uint32_t)minus((hash << 5), hash) + byte(_txt[i]);
  }
  return hash;
}
uint32_t deshifr(uint32_t val) {
  uint32_t hash;
  String nums;
  hash = val;
  char dig[2] = {48, 0};
  while (hash) {
    char a = 48;
    while ((hash - a) % 31) {
      a++;
    }
    dig[0] = a;
    nums += String(dig);
    hash = (hash - a) / 31;
    //Serial.print(hash);
    //Serial.print("/");
    //Serial.print(dig);
    //Serial.print(" ");
  }
  //Serial.println("?");
  String nums2;
  for (int i = nums.length() - 1; i >= 0; i--) {
    nums2 += nums[i];
    Serial.print(nums[i]);
    Serial.print("/");
    Serial.print(nums2);
    Serial.print(" ");
  }
  //Serial.println("|");
  return atol((char*)nums2.c_str());
}
