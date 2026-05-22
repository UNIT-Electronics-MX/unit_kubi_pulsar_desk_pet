#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "SparkFun_BMI270_Arduino_Library.h"

// =====================================================
// RTC SYNC OPTIONS
// =====================================================
// Descomenta la siguiente línea UNA VEZ, compila y sube,
// luego vélvela a comentar y vuelve a subir para que no
// se reescriba la hora cada vez que arranque.
#define FORCE_RTC_UPDATE

// También puedes enviar por Serial Monitor (115200, sin LF/CR extra):
//   SET YYYY-MM-DD HH:MM:SS    -> ajusta fecha y hora
//   TIME                       -> imprime hora actual

// =====================================================
// OLED CONFIG
// =====================================================

static const int WIDTH = 128;
static const int HEIGHT = 64;
static const int OLED_RESET = -1;
static const uint8_t OLED_ADDR = 0x3C;

static const int I2C_SDA_PIN = 12;
static const int I2C_SCL_PIN = 22;

Adafruit_SSD1306 oled(WIDTH, HEIGHT, &Wire, OLED_RESET);

// =====================================================
// BMI270 y RTC - Dasay Mochi Desk Pet
// =====================================================

BMI270 imu;
RTC_DS3231 rtc;
uint8_t i2cAddress = BMI2_I2C_SEC_ADDR; // 0x69

// =====================================================
// GLOBALS
// =====================================================

int z_offset = 0;
int vomit_offset = 0;
int excited_offset = 0;
int idle_counter = 0;
String display_emotion = "sleepy";
String pending_emotion = "";
int pending_count = 0;

static const int EMOTION_SWITCH_FRAMES = 2;
static const int ANGRY_RELEASE_FRAMES = 1;
static const int WAKE_EMOTION_FRAMES = 3;
static const int MIN_EMOTION_HOLD_FRAMES = 2;
static const int MIN_VOMIT_HOLD_FRAMES = 5;

float ax_zero = 0.0;
float ay_zero = 0.0;
float az_zero = 0.0;
int debug_print_div = 0;
int sleep_frames = 0;
String wake_emotion = "";
int wake_timer = 0;
int circular_counter = 0;
int angry_cooldown = 0;
int emotion_hold_frames = 0;
bool hasRTC = false;

// =====================================================
// SUSPEND MODE - después de 5 min de inactividad
// =====================================================
static const unsigned long SUSPEND_TIMEOUT_MS = 5UL * 60UL * 1000UL;  // 5 minutos
static const float SUSPEND_WAKE_ACCEL_THR = 0.35;  // ~0.35g de cambio para despertar
static const float SUSPEND_WAKE_GYRO_THR = 80.0;   // ~80 dps de movimiento

bool suspended = false;
unsigned long last_activity_ms = 0;
float suspend_ax_ref = 0.0;
float suspend_ay_ref = 0.0;
float suspend_az_ref = 1.0;

// =====================================================
// GRAPHICS HELPERS
// =====================================================

void fill_circle(int x0, int y0, int r, int color) {
  for (int y = -r; y < r; y++) {
    for (int x = -r; x < r; x++) {
      if ((x * x + y * y) <= (r * r)) {
        oled.drawPixel(x0 + x, y0 + y, color);
      }
    }
  }
}

void draw_arc(int x0, int y0, int r, int start_angle, int end_angle, int color) {
  for (int a = start_angle; a < end_angle; a++) {
    float angle = a * DEG_TO_RAD;
    int x = (int)(x0 + r * cosf(angle));
    int y = (int)(y0 + r * sinf(angle));
    oled.drawPixel(x, y, color);
  }
}

void draw_big_digit(int x, int y, char digit, int w = 14, int h = 28, int t = 3, int color = SSD1306_WHITE) {
  static const uint8_t seg[10][7] = {
    {1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 1, 0, 0, 1},
    {0, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 1, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1},
  };

  int idx = (digit >= '0' && digit <= '9') ? (digit - '0') : 0;

  if (seg[idx][0]) oled.fillRect(x + t, y, w - 2 * t, t, color);
  if (seg[idx][1]) oled.fillRect(x + w - t, y + t, t, (h / 2) - t, color);
  if (seg[idx][2]) oled.fillRect(x + w - t, y + (h / 2), t, (h / 2) - t, color);
  if (seg[idx][3]) oled.fillRect(x + t, y + h - t, w - 2 * t, t, color);
  if (seg[idx][4]) oled.fillRect(x, y + (h / 2), t, (h / 2) - t, color);
  if (seg[idx][5]) oled.fillRect(x, y + t, t, (h / 2) - t, color);
  if (seg[idx][6]) oled.fillRect(x + t, y + (h / 2) - (t / 2), w - 2 * t, t, color);
}

void draw_big_colon(int x, int y, int h = 28, int t = 3, int color = SSD1306_WHITE) {
  int dot_size = t;
  int top_y = y + (h / 3) - (dot_size / 2);
  int bottom_y = y + ((2 * h) / 3) - (dot_size / 2);

  oled.fillRect(x, top_y, dot_size, dot_size, color);
  oled.fillRect(x, bottom_y, dot_size, dot_size, color);
}

void draw_big_time(int hours, int minutes, int seconds, int y = 18) {
  char d[5];
  snprintf(d, sizeof(d), "%02d%02d", hours, minutes);

  int digit_w = 14;
  int digit_h = 28;
  int digit_space = 2;
  int colon_w = 3;
  int block_space = 2;

  int total_w = (digit_w * 4) + (digit_space * 3) + colon_w + (block_space * 2);
  int x = (WIDTH - total_w) / 2;

  draw_big_digit(x, y, d[0], digit_w, digit_h);
  x += digit_w + digit_space;
  draw_big_digit(x, y, d[1], digit_w, digit_h);
  x += digit_w + block_space;

  if ((seconds % 2) == 0) {
    draw_big_colon(x, y, digit_h, 3);
  }

  x += colon_w + block_space;
  draw_big_digit(x, y, d[2], digit_w, digit_h);
  x += digit_w + digit_space;
  draw_big_digit(x, y, d[3], digit_w, digit_h);
}

// =====================================================
// CLOCK MODE
// =====================================================

void draw_clock() {
  oled.clearDisplay();

  int hours = 0, minutes = 0, seconds = 0;
  char date_str[16];
  
  if (hasRTC) {
    DateTime now = rtc.now();
    hours = now.hour();
    minutes = now.minute();
    seconds = now.second();
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  } else {
    uint32_t elapsed = millis() / 1000;
    hours = (elapsed / 3600) % 24;
    minutes = (elapsed / 60) % 60;
    seconds = elapsed % 60;
    snprintf(date_str, sizeof(date_str), "SIN FECHA");
  }

  oled.drawRect(0, 0, WIDTH, HEIGHT, SSD1306_WHITE);
  oled.fillRect(2, 2, WIDTH - 4, 12, SSD1306_WHITE);

  oled.setTextSize(1);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(4, 4);
  oled.print("CLOCK");

  oled.setCursor(104, 4);
  oled.print(hasRTC ? "RTC" : "UP");

  oled.setTextColor(SSD1306_WHITE);
  draw_big_time(hours, minutes, seconds, 18);

  char sec_str[8];
  snprintf(sec_str, sizeof(sec_str), "%02ds", seconds);
  oled.setCursor(96, 16);
  oled.print(sec_str);

  int date_len = (int)strlen(date_str);
  int date_x = (WIDTH - (date_len * 6)) / 2;
  oled.setCursor(date_x, 52);
  oled.print(date_str);

  oled.display();
}

// =====================================================
// DRAW EMOTIONS
// =====================================================

void draw_emotion(const String &emotion) {
  oled.clearDisplay();

  if (emotion == "happy") {
    fill_circle(40, 32, 15, SSD1306_WHITE);
    fill_circle(88, 32, 15, SSD1306_WHITE);
    fill_circle(40, 32, 5, SSD1306_BLACK);
    fill_circle(88, 32, 5, SSD1306_BLACK);
    draw_arc(64, 48, 10, 0, 180, SSD1306_WHITE);
  } 
  else if (emotion == "sad") {
    fill_circle(40, 36, 12, SSD1306_WHITE);
    fill_circle(88, 36, 12, SSD1306_WHITE);
    fill_circle(40, 40, 4, SSD1306_BLACK);
    fill_circle(88, 40, 4, SSD1306_BLACK);
    oled.drawLine(28, 20, 52, 28, SSD1306_WHITE);
    oled.drawLine(76, 28, 100, 20, SSD1306_WHITE);
    draw_arc(64, 54, 10, 180, 360, SSD1306_WHITE);
  } 
  else if (emotion == "angry") {
    fill_circle(40, 32, 12, SSD1306_WHITE);
    fill_circle(88, 32, 12, SSD1306_WHITE);
    fill_circle(40, 28, 5, SSD1306_BLACK);
    fill_circle(88, 28, 5, SSD1306_BLACK);
    oled.drawLine(28, 20, 52, 24, SSD1306_WHITE);
    oled.drawLine(76, 24, 100, 20, SSD1306_WHITE);
    oled.drawFastHLine(54, 50, 20, SSD1306_WHITE);
  } 
  else if (emotion == "surprised") {
    fill_circle(40, 32, 18, SSD1306_WHITE);
    fill_circle(88, 32, 18, SSD1306_WHITE);
    fill_circle(40, 32, 6, SSD1306_BLACK);
    fill_circle(88, 32, 6, SSD1306_BLACK);
    oled.drawRect(58, 48, 10, 10, SSD1306_WHITE);
  } 
  else if (emotion == "sleepy") {
    oled.drawFastHLine(28, 32, 24, SSD1306_WHITE);
    oled.drawFastHLine(76, 32, 24, SSD1306_WHITE);
    oled.drawFastHLine(54, 50, 20, SSD1306_WHITE);

    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(100, 20 - z_offset);
    oled.print("Z");
    oled.setCursor(108, 10 - (z_offset / 2));
    oled.print("z");

    z_offset++;
    if (z_offset > 30) z_offset = 0;
  } 
  else if (emotion == "excited") {
    int jump = excited_offset % 3;

    fill_circle(40, 31 - jump, 16, SSD1306_WHITE);
    fill_circle(88, 31 - jump, 16, SSD1306_WHITE);
    fill_circle(40, 29 - jump, 6, SSD1306_BLACK);
    fill_circle(88, 29 - jump, 6, SSD1306_BLACK);
    oled.fillRect(38, 27 - jump, 2, 2, SSD1306_WHITE);
    oled.fillRect(86, 27 - jump, 2, 2, SSD1306_WHITE);

    oled.drawLine(28, 16, 50, 20, SSD1306_WHITE);
    oled.drawLine(78, 20, 100, 16, SSD1306_WHITE);

    draw_arc(64, 45, 16, 10, 170, SSD1306_WHITE);
    draw_arc(64, 46, 16, 10, 170, SSD1306_WHITE);

    int sparkle_shift = excited_offset % 4;
    oled.drawPixel(18 + sparkle_shift, 18, SSD1306_WHITE);
    oled.drawPixel(22 + sparkle_shift, 14, SSD1306_WHITE);
    oled.drawPixel(106 - sparkle_shift, 18, SSD1306_WHITE);
    oled.drawPixel(102 - sparkle_shift, 14, SSD1306_WHITE);

    oled.setCursor(44, 2);
    oled.print("WOW!");

    excited_offset++;
    if (excited_offset > 8) excited_offset = 0;
  } 
  else if (emotion == "vomit") {
    fill_circle(40, 32, 14, SSD1306_WHITE);
    fill_circle(88, 32, 14, SSD1306_WHITE);

    oled.drawLine(34, 26, 46, 38, SSD1306_BLACK);
    oled.drawLine(46, 26, 34, 38, SSD1306_BLACK);
    oled.drawLine(82, 26, 94, 38, SSD1306_BLACK);
    oled.drawLine(94, 26, 82, 38, SSD1306_BLACK);

    oled.fillRect(56, 46, 16, 8, SSD1306_WHITE);
    oled.fillRect(59, 48, 10, 4, SSD1306_BLACK);

    for (int y = 0; y < 18; y++) {
      int width = 6 - (y / 4);
      for (int x = -width; x < width; x++) {
        oled.drawPixel(64 + x + ((vomit_offset % 2) * 2), 54 + y, SSD1306_WHITE);
      }
    }

    oled.fillRect(58, 62 - vomit_offset, 2, 2, SSD1306_WHITE);
    oled.fillRect(70, 66 - (vomit_offset * 2), 2, 2, SSD1306_WHITE);

    vomit_offset++;
    if (vomit_offset > 8) vomit_offset = 0;
  } 
  else if (emotion == "clock") {
    draw_clock();
    return;
  }

  oled.display();
}

// =====================================================
// CALIBRATION
// =====================================================

void calibrate_accel_baseline(int samples = 40, int delay_ms = 10) {
  float sum_ax = 0.0;
  float sum_ay = 0.0;
  float sum_az = 0.0;
  int ok = 0;

  for (int i = 0; i < samples; i++) {
    imu.getSensorData();
    sum_ax += imu.data.accelX;
    sum_ay += imu.data.accelY;
    sum_az += imu.data.accelZ;
    ok++;
    delay(delay_ms);
  }

  if (ok > 0) {
    ax_zero = sum_ax / ok;
    ay_zero = sum_ay / ok;
    az_zero = sum_az / ok;
  }

  Serial.print("Calibración completada: AX=");
  Serial.print(ax_zero, 3);
  Serial.print(" AY=");
  Serial.print(ay_zero, 3);
  Serial.print(" AZ=");
  Serial.println(az_zero, 3);
}

// =====================================================
// STABILIZE EMOTION
// =====================================================

String stabilize_emotion(const String &next_emotion) {
  if (emotion_hold_frames > 0) {
    emotion_hold_frames--;
  }

  if (next_emotion == "vomit" || next_emotion == "clock") {
    display_emotion = next_emotion;
    emotion_hold_frames = (next_emotion == "vomit") ? MIN_VOMIT_HOLD_FRAMES : 0;
    pending_emotion = "";
    pending_count = 0;
    return display_emotion;
  }

  if (next_emotion == display_emotion) {
    pending_emotion = "";
    pending_count = 0;
    return display_emotion;
  }

  if (next_emotion != pending_emotion) {
    pending_emotion = next_emotion;
    pending_count = 1;
    return display_emotion;
  }

  if (emotion_hold_frames > 0) {
    return display_emotion;
  }

  pending_count++;

  int required_frames = EMOTION_SWITCH_FRAMES;
  if (display_emotion == "angry" && next_emotion != "angry") {
    required_frames = ANGRY_RELEASE_FRAMES;
  } else if (display_emotion == "sleepy" && next_emotion != "sleepy") {
    required_frames = 1;
  }

  if (pending_count >= required_frames) {
    display_emotion = pending_emotion;
    emotion_hold_frames = (display_emotion == "vomit") ? MIN_VOMIT_HOLD_FRAMES : MIN_EMOTION_HOLD_FRAMES;
    pending_emotion = "";
    pending_count = 0;
  }

  return display_emotion;
}

// =====================================================
// DETECT EMOTION
// =====================================================

String detect_emotion() {
  imu.getSensorData();
  
  // Convertir a valores similares al código original (multiplicar por ~16384 para 2G range)
  float ax = imu.data.accelX;
  float ay = imu.data.accelY;
  float az = imu.data.accelZ;
  float gx = imu.data.gyroX;
  float gy = imu.data.gyroY;
  float gz = imu.data.gyroZ;

  float ax_rel = ax - ax_zero;
  float ay_rel = ay - ay_zero;
  float az_rel = az - az_zero;

  // Convertir gyro a valores aproximados (250 dps range, ~131 LSB/dps)
  float motion = (abs(gx) + abs(gy) + abs(gz)) * 131.0;
  float accel_activity = (abs(ax_rel) + abs(ay_rel) + abs(az_rel)) * 16384.0;

  if (angry_cooldown > 0) {
    angry_cooldown--;
  }

  if (motion < 1500) {
    idle_counter++;
  } else {
    idle_counter = 0;
  }

  // Solo entrar en modo reloj si el RTC fue detectado.
  // Si no hay RTC, solo funcionan giroscopio + caritas.
  if (hasRTC && idle_counter > 50) {
    return "clock";
  }

  if (motion > 85000) {
    sleep_frames = 0;
    wake_timer = 0;
    return "vomit";
  }

  bool in_sleep_state = (motion < 2500 && accel_activity < 9000);

  if (in_sleep_state) {
    sleep_frames++;
  } else {
    if (sleep_frames >= 12) {
      if (angry_cooldown == 0 && (motion > 45000 || accel_activity > 32000)) {
        wake_emotion = "angry";
        wake_timer = 1;
        angry_cooldown = 40;
      } else if (motion > 3500 || accel_activity > 10000) {
        wake_emotion = "happy";
        wake_timer = WAKE_EMOTION_FRAMES;
      }
    }
    sleep_frames = 0;
  }

  if (wake_timer > 0) {
    wake_timer--;
    return wake_emotion;
  }

  float happy_thr = 5500.0 / 16384.0;
  float sad_thr = -5500.0 / 16384.0;

  if (ax_rel >= happy_thr) {
    return "happy";
  }

  if (ax_rel <= sad_thr) {
    return "sad";
  }

  float gx_abs = abs(gx) * 131.0;
  float gy_abs = abs(gy) * 131.0;
  float gz_abs = abs(gz) * 131.0;

  bool circular_candidate = (6000 < gx_abs && gx_abs < 32000 &&
                             6000 < gy_abs && gy_abs < 32000 &&
                             gz_abs < 26000);

  if (circular_candidate) {
    circular_counter++;
  } else {
    circular_counter = 0;
  }

  if (circular_counter >= 3) {
    return "excited";
  } else if (motion > 45000) {
    return "excited";
  } else if (az_rel > 7000.0 / 16384.0) {
    return "surprised";
  } else if (motion < 3000) {
    return "sleepy";
  }

  return "excited";
}

// =====================================================
// WAKEUP ANIMATION
// =====================================================

void wakeup_sequence() {
  for (int i = 0; i < 10; i++) {
    draw_emotion("sleepy");
    delay(120);
  }

  for (int i = 0; i < 8; i++) {
    draw_emotion("surprised");
    delay(80);
  }

  for (int i = 0; i < 15; i++) {
    draw_emotion("excited");
    delay(80);
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Dasay Mochi - BMI270 + RTC Desk Pet");

  // Inicializar I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  // Inicializar OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Error inicializando OLED");
    while (true) delay(1000);
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 20);
  oled.print("Iniciando...");
  oled.display();
  
  // Inicializar BMI270
  Serial.println("Inicializando BMI270...");
  while(imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("Error: BMI270 no conectado!");
    oled.clearDisplay();
    oled.setCursor(0, 20);
    oled.print("BMI270 ERROR");
    oled.display();
    delay(1000);
  }
  Serial.println("BMI270 conectado!");

  // Inicializar RTC
  Serial.println("Inicializando RTC...");
  if (rtc.begin()) {
    hasRTC = true;
    Serial.println("RTC DS3231 encontrado");

#ifdef FORCE_RTC_UPDATE
    Serial.println("FORCE_RTC_UPDATE activo -> sincronizando con hora de compilación");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
#else
    if (rtc.lostPower()) {
      Serial.println("RTC perdió energía, configurando hora...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
#endif

    DateTime now = rtc.now();
    Serial.print("Hora actual: ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
    Serial.println("Tip: envía 'SET 2026-05-21 17:14:00' por Serial para ajustar hora");
  } else {
    Serial.println("RTC no encontrado - usando uptime");
    hasRTC = false;
  }

  // Animación de inicio
  wakeup_sequence();
  
  // Calibrar acelerómetro
  Serial.println("Calibrando acelerómetro...");
  calibrate_accel_baseline();

  last_activity_ms = millis();
  Serial.println("Sistema listo!");
}

// =====================================================
// SERIAL COMMAND PARSER (sincronizar RTC)
// =====================================================

void handle_serial_commands() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  // Comando: TIME -> imprime hora actual
  if (line.equalsIgnoreCase("TIME")) {
    if (!hasRTC) {
      Serial.println("RTC no disponible");
      return;
    }
    DateTime now = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    Serial.print("Hora RTC: ");
    Serial.println(buf);
    return;
  }

  // Comando: SET YYYY-MM-DD HH:MM:SS
  if (line.startsWith("SET ") || line.startsWith("set ")) {
    if (!hasRTC) {
      Serial.println("RTC no disponible");
      return;
    }
    int y, mo, d, h, mi, s;
    int n = sscanf(line.c_str() + 4, "%d-%d-%d %d:%d:%d",
                   &y, &mo, &d, &h, &mi, &s);
    if (n != 6) {
      Serial.println("Formato: SET YYYY-MM-DD HH:MM:SS");
      return;
    }
    rtc.adjust(DateTime((uint16_t)y, (uint8_t)mo, (uint8_t)d,
                        (uint8_t)h, (uint8_t)mi, (uint8_t)s));
    Serial.print("RTC ajustado a: ");
    Serial.println(line.substring(4));
    return;
  }

  Serial.print("Comando desconocido: ");
  Serial.println(line);
  Serial.println("Comandos: TIME | SET YYYY-MM-DD HH:MM:SS");
}

// =====================================================
// SUSPEND / WAKE HELPERS
// =====================================================

void enter_suspend() {
  Serial.println(">> Entrando en modo SUSPEND (solo IMU)");
  suspended = true;

  // Tomar referencia actual de aceleración como reposo
  imu.getSensorData();
  suspend_ax_ref = imu.data.accelX;
  suspend_ay_ref = imu.data.accelY;
  suspend_az_ref = imu.data.accelZ;

  // Apagar OLED
  oled.clearDisplay();
  oled.display();
  oled.ssd1306_command(SSD1306_DISPLAYOFF);
}

void exit_suspend() {
  Serial.println("<< Saliendo de SUSPEND, despertando");
  suspended = false;
  last_activity_ms = millis();

  // Reset estado de emociones
  idle_counter = 0;
  sleep_frames = 0;
  pending_emotion = "";
  pending_count = 0;
  emotion_hold_frames = 0;
  display_emotion = "surprised";

  // Encender OLED y animación de wake
  oled.ssd1306_command(SSD1306_DISPLAYON);

  for (int i = 0; i < 6; i++) {
    draw_emotion("surprised");
    delay(80);
  }
  for (int i = 0; i < 8; i++) {
    draw_emotion("excited");
    delay(80);
  }
}

bool check_suspend_wake() {
  // Lee solo el IMU y verifica si hay movimiento suficiente para despertar
  imu.getSensorData();

  float dax = imu.data.accelX - suspend_ax_ref;
  float day = imu.data.accelY - suspend_ay_ref;
  float daz = imu.data.accelZ - suspend_az_ref;
  float accel_delta = fabsf(dax) + fabsf(day) + fabsf(daz);

  float gyro_mag = fabsf(imu.data.gyroX) + fabsf(imu.data.gyroY) + fabsf(imu.data.gyroZ);

  return (accel_delta > SUSPEND_WAKE_ACCEL_THR) || (gyro_mag > SUSPEND_WAKE_GYRO_THR);
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  // Procesar comandos por Serial (sincronizar RTC, etc.)
  handle_serial_commands();

  // ----- MODO SUSPEND: solo IMU -----
  if (suspended) {
    if (check_suspend_wake()) {
      exit_suspend();
    } else {
      delay(150);  // Poll lento para ahorrar energía
    }
    return;
  }

  // ----- MODO NORMAL -----
  String emotion = detect_emotion();

  // Detectar actividad real (no "sleepy" ni "clock") para resetear timer
  bool is_active = (emotion != "sleepy" && emotion != "clock");
  if (is_active) {
    last_activity_ms = millis();
  }

  // Entrar en suspend tras 5 min sin actividad
  if ((millis() - last_activity_ms) > SUSPEND_TIMEOUT_MS) {
    enter_suspend();
    return;
  }

  debug_print_div++;
  if (debug_print_div >= 4) {
    debug_print_div = 0;

    if (hasRTC) {
      DateTime now = rtc.now();
      Serial.print(now.hour());
      Serial.print(":");
      Serial.print(now.minute());
      Serial.print(":");
      Serial.print(now.second());
      Serial.print(" | ");
    }

    Serial.print("ACC: ");
    Serial.print(imu.data.accelX, 3);
    Serial.print(" ");
    Serial.print(imu.data.accelY, 3);
    Serial.print(" ");
    Serial.print(imu.data.accelZ, 3);

    Serial.print(" | GYR: ");
    Serial.print(imu.data.gyroX, 3);
    Serial.print(" ");
    Serial.print(imu.data.gyroY, 3);
    Serial.print(" ");
    Serial.print(imu.data.gyroZ, 3);

    Serial.print(" | EMO: ");
    Serial.println(emotion);
  }

  String stable = stabilize_emotion(emotion);
  draw_emotion(stable);

  delay(80);
}
