#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include "html_page.h"
#include "SparkFun_BMI270_Arduino_Library.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const char* AP_SSID = "Kubi-Control";
const char* AP_PASS = "12345678";
WebServer server(80);

BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_SEC_ADDR;

String currentEmotion = "happy";
String pendingEmotion  = "happy";
String currentMessage  = "";

int animFrame    = 0;
int lookX        = 0;
int lookY        = 0;
int blinkState   = 0;
int blinkCounter = 0;
int textScrollX  = 128;

unsigned long lastScroll   = 0;
unsigned long emotionTimer = 0;
unsigned long lastHit      = 0;

bool autoMode   = true;
int  hitCounter = 0;

int32_t ax_zero = 0, ay_zero = 0, az_zero = 0;

const int eyeLX = 38, eyeRX = 90, eyeY = 32;
const int eyeW = 15, eyeH = 12, pupilR = 5;

// ---- Pomodoro ----
unsigned long pomodoroTotal   = 25UL * 60UL * 1000UL;
unsigned long pomodoroStart   = 0;
unsigned long pomodoroElapsed = 0;
bool pomodoroRunning  = false;
bool pomodoroDone     = false;
unsigned long pomodoroDoneAt = 0;

// ---- Parpadeo ----

void actualizarParpadeo() {
  blinkCounter++;
  if      (blinkCounter > 60 && blinkCounter < 63) blinkState = 1;
  else if (blinkCounter < 66)                       blinkState = 2;
  else if (blinkCounter < 69)                       blinkState = 3;
  else                                              blinkState = 0;
  if (blinkCounter > 120) blinkCounter = 0;
}

int calcApertura(int base, bool parpadear) {
  if (!parpadear)      return base;
  if (blinkState == 1) return base / 2;
  if (blinkState == 2) return 1;
  if (blinkState == 3) return base / 2;
  return base;
}

// ---- Ojo ----

void drawEye(int cx, int cy, int aperturaH, int px, int py) {
  if (aperturaH <= 1) {
    u8g2.drawLine(cx - eyeW, cy,     cx + eyeW, cy);
    u8g2.drawLine(cx - eyeW, cy + 1, cx + eyeW, cy + 1);
    return;
  }
  for (int dy = -aperturaH; dy <= aperturaH; dy++) {
    float ratio = 1.0f - (float)(dy * dy) / (float)(aperturaH * aperturaH);
    int hw = (int)(eyeW * sqrt(ratio));
    u8g2.drawLine(cx - hw, cy + dy, cx + hw, cy + dy);
  }
  u8g2.setDrawColor(0);
  int pcx = cx + px, pcy = cy + py;
  for (int dy = -pupilR; dy <= pupilR; dy++)
    for (int dx = -pupilR; dx <= pupilR; dx++)
      if (dx * dx + dy * dy <= pupilR * pupilR)
        u8g2.drawPixel(pcx + dx, pcy + dy);
  u8g2.setDrawColor(1);
  u8g2.drawPixel(pcx - 2, pcy - 2);
}

// ---- Emociones ----

void ojosHappy() {

  int ap = calcApertura(eyeH, true);

  u8g2.clearBuffer();

  drawEye(
    eyeLX,
    eyeY,
    ap,
    lookX,
    lookY
  );

  drawEye(
    eyeRX,
    eyeY,
    ap,
    lookX,
    lookY
  );

  // Boca feliz
  // Boca feliz
u8g2.drawCircle(64, 46, 8, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);

  u8g2.sendBuffer();
}

void ojosSad() {
  int ap = calcApertura(eyeH - 2, true);
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY + 2, ap, 0, 2);
  drawEye(eyeRX, eyeY + 2, ap, 0, 2);
  u8g2.drawLine(20, 18, 45, 10);
  u8g2.drawLine(83, 10, 108, 18);
  u8g2.sendBuffer();
}

void ojosAngry() {
  int ap = calcApertura(eyeH - 3, false);
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, ap, lookX, lookY);
  drawEye(eyeRX, eyeY, ap, lookX, lookY);
  u8g2.drawLine(20, 10, 45, 20);
  u8g2.drawLine(83, 20, 108, 10);
  u8g2.sendBuffer();
}

void ojosSleepy() {
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, 3, 0, 0);
  drawEye(eyeRX, eyeY, 3, 0, 0);
  u8g2.setFont(u8g2_font_6x10_tr);
  int z = animFrame % 18;
  u8g2.drawStr(100, 40 - z,     "z");
  u8g2.drawStr(107, 34 - z / 2, "Z");
  u8g2.drawStr(114, 28 - z / 3, "Z");
  u8g2.sendBuffer();
}

void ojosSurprised() {

  u8g2.clearBuffer();

  drawEye(
    eyeLX,
    eyeY,
    eyeH + 4,
    0,
    0
  );

  drawEye(
    eyeRX,
    eyeY,
    eyeH + 4,
    0,
    0
  );


  // Boca sorprendida
  for(int y = -4; y <= 4; y++) {

    for(int x = -4; x <= 4; x++) {

      if(x*x + y*y <= 16) {

        u8g2.drawPixel(
          64 + x,
          50 + y
        );
      }
    }
  }

  u8g2.sendBuffer();
}

void ojosVomit() {
  u8g2.clearBuffer();
  u8g2.drawLine(28, 22, 42, 36); u8g2.drawLine(42, 22, 28, 36);
  u8g2.drawLine(80, 22, 94, 36); u8g2.drawLine(94, 22, 80, 36);
  u8g2.drawBox(56, 46, 16, 8);
  u8g2.setDrawColor(0); u8g2.drawBox(59, 48, 10, 4); u8g2.setDrawColor(1);
  int vo = animFrame % 4;
  for (int y = 0; y < 18; y++) {
    int w = 6 - (y / 4);
    for (int x = -w; x < w; x++) u8g2.drawPixel(64 + x + vo, 54 + y);
  }
  int d = animFrame % 8;
  u8g2.drawBox(58, 62 - d, 2, 2);
  u8g2.drawBox(70, 66 - d * 2, 2, 2);
  u8g2.sendBuffer();
}

void mostrarEmocion() {
  actualizarParpadeo();
  if      (currentEmotion == "angry")     ojosAngry();
  else if (currentEmotion == "sad")       ojosSad();
  else if (currentEmotion == "sleepy")    ojosSleepy();
  else if (currentEmotion == "surprised") ojosSurprised();
  else if (currentEmotion == "vomit")     ojosVomit();
  else                                    ojosHappy();
}

// ---- Helpers OLED estilo Fallout ----

void drawFalloutHeader(const char* label) {
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawHLine(1, 12, 126);
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(3, 9, label);
  if ((animFrame / 12) % 2 == 0) u8g2.drawBox(119, 3, 5, 7);
}

// ---- Reloj ----

void drawClock() {
  u8g2.clearBuffer();
  struct tm t; time_t now = time(nullptr); localtime_r(&now, &t);
  char tb[16], db[32];
  snprintf(tb, sizeof(tb), "%02d:%02d", t.tm_hour, t.tm_min);
  snprintf(db, sizeof(db), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);

  u8g2.drawFrame(0, 0, 122, 64);
  u8g2.drawFrame(3, 3, 116, 58);
  u8g2.drawLine(0, 7,   7,  0);
  u8g2.drawLine(114, 0, 121, 7);
  u8g2.drawLine(0, 56,  7,  63);
  u8g2.drawLine(114, 63, 121, 56);

  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(6, 11, "Hora actual");
  u8g2.drawStr(101, 11, "RTC");
  u8g2.drawHLine(5, 13, 112);

  u8g2.setFont(u8g2_font_logisoso28_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth(tb)) / 2, 44, tb);

  if ((t.tm_sec % 2) != 0) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(56, 16, 14, 28);
    u8g2.setDrawColor(1);
  }

  u8g2.drawHLine(5, 47, 112);
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth(db)) / 2, 56, db);
  u8g2.sendBuffer();
}

// ---- Mensaje ----

void drawMessage() {
  u8g2.clearBuffer();
  drawFalloutHeader("// INCOMING MSG //");

  u8g2.setFont(u8g2_font_10x20_tr);
  int tw = currentMessage.length() * 10;
  if (tw <= 120) {
    u8g2.drawStr(3, 38, currentMessage.c_str());
  } else {
    u8g2.drawStr(textScrollX, 38, currentMessage.c_str());
    if (millis() - lastScroll > 45) {
      lastScroll = millis();
      if (--textScrollX < -tw) textScrollX = 128;
    }
  }
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(3, 61, "> END OF MESSAGE_");
  u8g2.sendBuffer();
}

// ---- Pomodoro ----

void drawPomodoro() {
  u8g2.clearBuffer();
  drawFalloutHeader("// POMODORO //");

  unsigned long remaining;
  if (pomodoroDone) {
    remaining = 0;
  } else if (pomodoroRunning) {
    unsigned long elapsed = pomodoroElapsed + (millis() - pomodoroStart);
    remaining = (elapsed >= pomodoroTotal) ? 0 : (pomodoroTotal - elapsed);
    if (remaining == 0 && !pomodoroDone) {
      pomodoroDone  = true;
      pomodoroDoneAt = millis();
      pomodoroRunning = false;
      currentEmotion = "surprised";
    }
  } else {
    remaining = pomodoroTotal - pomodoroElapsed;
  }

  unsigned long secs = remaining / 1000;
  unsigned long mm   = secs / 60;
  unsigned long ss   = secs % 60;
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", mm, ss);

  // Tiempo grande
  u8g2.setFont(u8g2_font_logisoso28_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth(timeBuf)) / 2, 46, timeBuf);

  // Barra de progreso — área [3,50]..[125,56]
  u8g2.drawFrame(3, 48, 122, 6);
  if (pomodoroTotal > 0 && !pomodoroDone) {
    unsigned long elapsed = pomodoroElapsed + (pomodoroRunning ? (millis() - pomodoroStart) : 0);
    int filled = (int)(118UL * elapsed / pomodoroTotal);
    if (filled > 0) u8g2.drawBox(4, 49, filled, 4);
  }

  // Estado inferior
  u8g2.setFont(u8g2_font_04b_03_tr);
  if (pomodoroDone)          u8g2.drawStr(3, 61, "> TIME UP! WELL DONE_");
  else if (pomodoroRunning)  u8g2.drawStr(3, 61, "> FOCUS MODE ACTIVE_");
  else if (pomodoroElapsed > 0) u8g2.drawStr(3, 61, "> PAUSED_");
  else                       u8g2.drawStr(3, 61, "> READY_");

  u8g2.sendBuffer();
}

// ---- BMI270 ----

void initBMI() {
  while (imu.beginI2C(i2cAddress, Wire) != BMI2_OK) {
    Serial.println("BMI270 no encontrado, reintentando...");
    delay(1000);
  }
  Serial.println("BMI READY");
}

bool readMotion6(int16_t &ax, int16_t &ay, int16_t &az,
                 int16_t &gx, int16_t &gy, int16_t &gz) {
  imu.getSensorData();
  ax = (int16_t)(imu.data.accelX * 16384.0f);
  ay = (int16_t)(imu.data.accelY * 16384.0f);
  az = (int16_t)(imu.data.accelZ * 16384.0f);
  gx = (int16_t)(imu.data.gyroX  * 131.0f);
  gy = (int16_t)(imu.data.gyroY  * 131.0f);
  gz = (int16_t)(imu.data.gyroZ  * 131.0f);
  return true;
}

void calibrateBMI() {
  long sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 50; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    readMotion6(ax, ay, az, gx, gy, gz);
    sx += ax; sy += ay; sz += az;
    delay(20);
  }
  ax_zero = sx / 50; ay_zero = sy / 50; az_zero = sz / 50;
  Serial.println("BMI CALIBRATED");
}

// ---- Detección de emoción ----

unsigned long getEmotionDelay(const String &e) {
  if (e == "surprised" || e == "vomit") return 0;
  if (e == "angry")  return 300;
  if (e == "sad")    return 1000;
  if (e == "sleepy") return 2000;
  return 1000;
}

String detectEmotion(int16_t ax, int16_t ay, int16_t az,
                     int16_t gx, int16_t gy, int16_t gz) {
  int motion = abs(gx) + abs(gy) + abs(gz);

  String next;
  if      (motion > 55000)               next = "vomit";
  else if (az < -12000 && motion < 1000) next = "sleepy";
  else if (motion > 30000)               next = "angry";
  else if (motion > 20000)               next = "surprised";
  else                                   next = "happy";

  if (next != pendingEmotion) { pendingEmotion = next; emotionTimer = millis(); }
  if (millis() - emotionTimer > getEmotionDelay(pendingEmotion)) currentEmotion = pendingEmotion;
  return currentEmotion;
}

// ---- Web handlers ----

void handleRoot()    { server.send(200, "text/html", HTML_PAGE); }

void handleEmotion() {
  if (server.hasArg("name")) {
    currentEmotion = server.arg("name");
    Serial.println("Emotion: " + currentEmotion);
    mostrarEmocion();
  }
  server.send(200, "text/plain", "OK");
}

void handleClock() {
  currentEmotion = "clock";
  server.send(200, "text/plain", "CLOCK");
}

void handleMessage() {
  if (server.hasArg("text")) {
    currentMessage = server.arg("text");
    currentEmotion = "message";
    textScrollX = 128;
    Serial.println("MESSAGE: " + currentMessage);
  }
  server.send(200, "text/plain", "MESSAGE OK");
}

void handleSetTime() {
  if (server.hasArg("epoch")) {
    struct timeval tv = { (time_t)server.arg("epoch").toInt(), 0 };
    settimeofday(&tv, NULL);
    Serial.println("TIME UPDATED");
  }
  server.send(200, "text/plain", "TIME OK");
}

void handleAutoMode() {
  if (server.hasArg("state")) {
    autoMode = (server.arg("state") == "on");
    Serial.println(autoMode ? "AUTO MODE ON" : "AUTO MODE OFF");
  }
  server.send(200, "text/plain", autoMode ? "AUTO ON" : "AUTO OFF");
}

void handlePomodoro() {
  String action = server.hasArg("action") ? server.arg("action") : "";

  if (action == "start") {
    if (!pomodoroRunning && !pomodoroDone) {
      if (server.hasArg("minutes"))
        pomodoroTotal = (unsigned long)server.arg("minutes").toInt() * 60UL * 1000UL;
      pomodoroStart   = millis();
      pomodoroRunning = true;
      currentEmotion  = "pomodoro";
    }
  } else if (action == "pause") {
    if (pomodoroRunning) {
      pomodoroElapsed += millis() - pomodoroStart;
      pomodoroRunning  = false;
      currentEmotion   = "pomodoro";
    }
  } else if (action == "resume") {
    if (!pomodoroRunning && !pomodoroDone) {
      pomodoroStart   = millis();
      pomodoroRunning = true;
      currentEmotion  = "pomodoro";
    }
  } else if (action == "reset") {
    pomodoroRunning  = false;
    pomodoroDone     = false;
    pomodoroElapsed  = 0;
    pomodoroStart    = 0;
    if (server.hasArg("minutes"))
      pomodoroTotal = (unsigned long)server.arg("minutes").toInt() * 60UL * 1000UL;
    currentEmotion = "pomodoro";
  } else if (action == "show") {
    currentEmotion = "pomodoro";
  }

  server.send(200, "text/plain", "POMODORO OK");
}

// ---- Setup ----

void setup() {
  Serial.begin(115200);
  setenv("TZ", "CST6", 1); tzset();
  delay(1000);

  Wire.begin(6, 7);
  Wire.setClock(400000);
  u8g2.begin();
  u8g2.setContrast(255);
  currentEmotion = "sleepy";
  mostrarEmocion();

  initBMI();
  delay(500);
  calibrateBMI();

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("AP READY — IP: " + WiFi.softAPIP().toString());

  server.on("/",          handleRoot);
  server.on("/emotion",   handleEmotion);
  server.on("/clock",     handleClock);
  server.on("/message",   handleMessage);
  server.on("/setTime",   handleSetTime);
  server.on("/auto",      handleAutoMode);
  server.on("/pomodoro",  handlePomodoro);
  server.begin();
  Serial.println("WEB SERVER READY");
}

// ---- Loop ----

void loop() {
  server.handleClient();

  static unsigned long lastFrame = 0;
  if (millis() - lastFrame <= 40) return;
  lastFrame = millis();
  animFrame++;

  if (animFrame % 80 == 0) {
    int r = random(0, 5);
    lookX = (r == 0) ? -3 : (r == 1) ? 3 : 0;
    lookY = (r == 2) ? -2 : (r == 3) ? 2 : 0;
  }

  // Limpiar pantalla de pomodoro completado tras 4 segundos
  if (pomodoroDone && (millis() - pomodoroDoneAt > 4000)) {
    pomodoroDone    = false;
    pomodoroElapsed = 0;
    currentEmotion  = "happy";
  }

  if (autoMode && currentEmotion != "clock"    &&
                  currentEmotion != "message"  &&
                  currentEmotion != "pomodoro") {
    int16_t ax, ay, az, gx, gy, gz;
    if (readMotion6(ax, ay, az, gx, gy, gz))
      currentEmotion = detectEmotion(ax, ay, az, gx, gy, gz);
  }

  if      (currentEmotion == "clock")    drawClock();
  else if (currentEmotion == "message")  drawMessage();
  else if (currentEmotion == "pomodoro") drawPomodoro();
  else                                   mostrarEmocion();
}
