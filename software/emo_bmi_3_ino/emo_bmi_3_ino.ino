#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <time.h>

// ==========================================
// OLED CONFIG
// ==========================================

static const int WIDTH = 128;
static const int HEIGHT = 64;
static const int OLED_RESET = -1;
static const uint8_t OLED_ADDR = 0x3C;

// Keep the same pins used in the MicroPython version.
static const int I2C_SDA_PIN = 12;
static const int I2C_SCL_PIN = 22;

Adafruit_SSD1306 oled(WIDTH, HEIGHT, &Wire, OLED_RESET);

// ==========================================
// BMI160 REGS / COMMANDS
// ==========================================

static const uint8_t BMI160_ADDR = 0x69;
static const uint8_t REG_GYRO_X_L = 0x0C;
static const uint8_t REG_GYRO_RANGE = 0x43;
static const uint8_t REG_ACCEL_RANGE = 0x41;
static const uint8_t REG_CMD = 0x7E;

static const uint8_t CMD_SOFT_RESET = 0xB6;
static const uint8_t CMD_ACC_NORMAL = 0x11;
static const uint8_t CMD_GYR_NORMAL = 0x15;

static const uint8_t GYRO_RANGE_250 = 0x03;
static const uint8_t ACCEL_RANGE_2G = 0x03;

// ==========================================
// GLOBALS
// ==========================================

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

int32_t ax_zero = 0;
int32_t ay_zero = 0;
int32_t az_zero = 0;
int debug_print_div = 0;
int sleep_frames = 0;
String wake_emotion = "";
int wake_timer = 0;
int circular_counter = 0;
int angry_cooldown = 0;
int emotion_hold_frames = 0;

// ==========================================
// LOW LEVEL BMI160
// ==========================================

bool bmiWriteReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool bmiReadRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  uint8_t got = Wire.requestFrom((int)BMI160_ADDR, (int)len);
  if (got != len) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

bool initBMI160() {
  if (!bmiWriteReg(REG_CMD, CMD_SOFT_RESET)) {
    return false;
  }
  delay(2);

  uint8_t dummy = 0;
  bmiReadRegs(0x7F, &dummy, 1);
  delay(2);

  if (!bmiWriteReg(REG_CMD, CMD_ACC_NORMAL)) {
    return false;
  }
  delay(50);

  if (!bmiWriteReg(REG_CMD, CMD_GYR_NORMAL)) {
    return false;
  }
  delay(100);

  if (!bmiWriteReg(REG_GYRO_RANGE, GYRO_RANGE_250)) {
    return false;
  }

  if (!bmiWriteReg(REG_ACCEL_RANGE, ACCEL_RANGE_2G)) {
    return false;
  }

  return true;
}

static inline int16_t le16(uint8_t lo, uint8_t hi) {
  return (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
}

bool readMotion6(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
  uint8_t data[12];
  if (!bmiReadRegs(REG_GYRO_X_L, data, sizeof(data))) {
    return false;
  }

  gx = le16(data[0], data[1]);
  gy = le16(data[2], data[3]);
  gz = le16(data[4], data[5]);
  ax = le16(data[6], data[7]);
  ay = le16(data[8], data[9]);
  az = le16(data[10], data[11]);

  return true;
}

// ==========================================
// GRAPHICS HELPERS
// ==========================================

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

void getClockTimeComponents(int &hours, int &minutes, int &seconds, bool &hasRtcDate) {
  time_t now = time(nullptr);

  if (now > 1704067200) {
    struct tm t;
    localtime_r(&now, &t);
    hours = t.tm_hour;
    minutes = t.tm_min;
    seconds = t.tm_sec;
    hasRtcDate = true;
    return;
  }

  uint32_t elapsed = millis() / 1000;
  hours = (elapsed / 3600) % 24;
  minutes = (elapsed / 60) % 60;
  seconds = elapsed % 60;
  hasRtcDate = false;
}

void calibrate_accel_baseline(int samples = 40, int delay_ms = 10) {
  int64_t sum_ax = 0;
  int64_t sum_ay = 0;
  int64_t sum_az = 0;
  int ok = 0;

  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    if (readMotion6(ax, ay, az, gx, gy, gz)) {
      sum_ax += ax;
      sum_ay += ay;
      sum_az += az;
      ok++;
    }
    delay(delay_ms);
  }

  if (ok > 0) {
    ax_zero = (int32_t)(sum_ax / ok);
    ay_zero = (int32_t)(sum_ay / ok);
    az_zero = (int32_t)(sum_az / ok);
  }
}

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

  // Keep each face visible briefly so fast motion changes do not flicker emotions.
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

// ==========================================
// CLOCK MODE
// ==========================================

void draw_clock() {
  oled.clearDisplay();

  int hours, minutes, seconds;
  bool hasRtcDate;
  getClockTimeComponents(hours, minutes, seconds, hasRtcDate);

  char date_str[16];
  if (hasRtcDate) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
  } else {
    snprintf(date_str, sizeof(date_str), "SIN FECHA");
  }

  oled.drawRect(0, 0, WIDTH, HEIGHT, SSD1306_WHITE);
  oled.fillRect(2, 2, WIDTH - 4, 12, SSD1306_WHITE);

  oled.setTextSize(1);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(4, 4);
  oled.print("CLOCK");

  oled.setCursor(104, 4);
  oled.print(hasRtcDate ? "RTC" : "UP");

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

// ==========================================
// DRAW EMOTIONS
// ==========================================

void draw_emotion(const String &emotion) {
  oled.clearDisplay();

  if (emotion == "happy") {
    fill_circle(40, 32, 15, SSD1306_WHITE);
    fill_circle(88, 32, 15, SSD1306_WHITE);

    fill_circle(40, 32, 5, SSD1306_BLACK);
    fill_circle(88, 32, 5, SSD1306_BLACK);

    draw_arc(64, 48, 10, 0, 180, SSD1306_WHITE);
  } else if (emotion == "sad") {
    fill_circle(40, 36, 12, SSD1306_WHITE);
    fill_circle(88, 36, 12, SSD1306_WHITE);

    fill_circle(40, 40, 4, SSD1306_BLACK);
    fill_circle(88, 40, 4, SSD1306_BLACK);

    oled.drawLine(28, 20, 52, 28, SSD1306_WHITE);
    oled.drawLine(76, 28, 100, 20, SSD1306_WHITE);

    draw_arc(64, 54, 10, 180, 360, SSD1306_WHITE);
  } else if (emotion == "angry") {
    fill_circle(40, 32, 12, SSD1306_WHITE);
    fill_circle(88, 32, 12, SSD1306_WHITE);

    fill_circle(40, 28, 5, SSD1306_BLACK);
    fill_circle(88, 28, 5, SSD1306_BLACK);

    oled.drawLine(28, 20, 52, 24, SSD1306_WHITE);
    oled.drawLine(76, 24, 100, 20, SSD1306_WHITE);

    oled.drawFastHLine(54, 50, 20, SSD1306_WHITE);
  } else if (emotion == "surprised") {
    fill_circle(40, 32, 18, SSD1306_WHITE);
    fill_circle(88, 32, 18, SSD1306_WHITE);

    fill_circle(40, 32, 6, SSD1306_BLACK);
    fill_circle(88, 32, 6, SSD1306_BLACK);

    oled.drawRect(58, 48, 10, 10, SSD1306_WHITE);
  } else if (emotion == "sleepy") {
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
    if (z_offset > 30) {
      z_offset = 0;
    }
  } else if (emotion == "excited") {
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
    if (excited_offset > 8) {
      excited_offset = 0;
    }
  } else if (emotion == "vomit") {
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
    if (vomit_offset > 8) {
      vomit_offset = 0;
    }
  } else if (emotion == "clock") {
    draw_clock();
    return;
  }

  oled.display();
}

// ==========================================
// WAKEUP ANIMATION
// ==========================================

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

// ==========================================
// DETECT EMOTION
// ==========================================

String detect_emotion(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz) {
  int32_t ax_rel = (int32_t)ax - ax_zero;
  int32_t ay_rel = (int32_t)ay - ay_zero;
  int32_t az_rel = (int32_t)az - az_zero;

  int32_t motion = abs(gx) + abs(gy) + abs(gz);
  int32_t accel_activity = abs(ax_rel) + abs(ay_rel) + abs(az_rel);

  if (angry_cooldown > 0) {
    angry_cooldown--;
  }

  if (motion < 1500) {
    idle_counter++;
  } else {
    idle_counter = 0;
  }

  if (idle_counter > 50) {
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
      // Angry only for very abrupt wake-ups and with a cooldown to avoid loops.
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

  int happy_thr = 5500;
  int sad_thr = -5500;

  if (ax_rel >= happy_thr) {
    return "happy";
  }

  if (ax_rel <= sad_thr) {
    return "sad";
  }

  bool circular_candidate =
      (6000 < abs(gx) && abs(gx) < 32000 &&
       6000 < abs(gy) && abs(gy) < 32000 &&
       abs(gz) < 26000);

  if (circular_candidate) {
    circular_counter++;
  } else {
    circular_counter = 0;
  }

  if (circular_counter >= 3) {
    return "excited";
  } else if (motion > 45000) {
    return "excited";
  } else if (az_rel > 7000) {
    return "surprised";
  } else if (motion < 3000) {
    return "sleepy";
  }

  return "excited";
}

void drawBMIError(const char *msg) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 20);
  oled.print("BMI160 ERROR");
  oled.setCursor(0, 40);
  oled.print(msg);
  oled.display();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true) {
      delay(1000);
    }
  }

  oled.clearDisplay();
  oled.display();

  if (!initBMI160()) {
    drawBMIError("init failed");
    while (true) {
      delay(1000);
    }
  }

  wakeup_sequence();
  calibrate_accel_baseline();
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;

  if (!readMotion6(ax, ay, az, gx, gy, gz)) {
    drawBMIError("read failed");
    Serial.println("ERROR: BMI160 read failed");
    delay(80);
    return;
  }

  String emotion = detect_emotion(ax, ay, az, gx, gy, gz);

  debug_print_div++;
  if (debug_print_div >= 4) {
    debug_print_div = 0;
    Serial.print("ACC: ");
    Serial.print(ax);
    Serial.print(' ');
    Serial.print(ay);
    Serial.print(' ');
    Serial.print(az);

    Serial.print(" REL: ");
    Serial.print((int32_t)ax - ax_zero);
    Serial.print(' ');
    Serial.print((int32_t)ay - ay_zero);
    Serial.print(' ');
    Serial.print((int32_t)az - az_zero);

    Serial.print(" GYR: ");
    Serial.print(gx);
    Serial.print(' ');
    Serial.print(gy);
    Serial.print(' ');
    Serial.print(gz);

    Serial.print(" EMO: ");
    Serial.println(emotion);
  }

  String stable = stabilize_emotion(emotion);
  draw_emotion(stable);

  delay(80);
}
