from machine import Pin, I2C
from ssd1306 import SSD1306_I2C
from bmi160 import init_bmi160, read_motion6
import time
import math

# ==========================================
# OLED CONFIG
# ==========================================

WIDTH = 128
HEIGHT = 64

i2c = I2C(
    0,
    scl=Pin(22),
    sda=Pin(12),
    freq=400000
)

oled = SSD1306_I2C(WIDTH, HEIGHT, i2c)

# ==========================================
# BMI160 INIT
# ==========================================

init_bmi160(i2c)

# ==========================================
# GLOBALS
# ==========================================

z_offset = 0
vomit_offset = 0
excited_offset = 0
idle_counter = 0
display_emotion = "sleepy"
pending_emotion = ""
pending_count = 0
EMOTION_SWITCH_FRAMES = 2
ANGRY_RELEASE_FRAMES = 1
WAKE_EMOTION_FRAMES = 3
ax_zero = 0
ay_zero = 0
az_zero = 0
debug_print_div = 0
sleep_frames = 0
wake_emotion = ""
wake_timer = 0
circular_counter = 0

# ==========================================
# GRAPHICS HELPERS
# ==========================================

def fill_circle(x0, y0, r, color):

    for y in range(-r, r):
        for x in range(-r, r):

            if x * x + y * y <= r * r:
                oled.pixel(x0 + x, y0 + y, color)

def draw_arc(x0, y0, r, start_angle, end_angle, color):

    for a in range(start_angle, end_angle):

        angle = math.radians(a)

        x = int(x0 + r * math.cos(angle))
        y = int(y0 + r * math.sin(angle))

        oled.pixel(x, y, color)


def draw_big_digit(x, y, digit, w=14, h=28, t=3, color=1):

    seg = {
        "0": (1, 1, 1, 1, 1, 1, 0),
        "1": (0, 1, 1, 0, 0, 0, 0),
        "2": (1, 1, 0, 1, 1, 0, 1),
        "3": (1, 1, 1, 1, 0, 0, 1),
        "4": (0, 1, 1, 0, 0, 1, 1),
        "5": (1, 0, 1, 1, 0, 1, 1),
        "6": (1, 0, 1, 1, 1, 1, 1),
        "7": (1, 1, 1, 0, 0, 0, 0),
        "8": (1, 1, 1, 1, 1, 1, 1),
        "9": (1, 1, 1, 1, 0, 1, 1),
    }

    s = seg.get(str(digit), seg["0"])

    # a, b, c, d, e, f, g segments
    if s[0]:
        oled.fill_rect(x + t, y, w - 2 * t, t, color)
    if s[1]:
        oled.fill_rect(x + w - t, y + t, t, (h // 2) - t, color)
    if s[2]:
        oled.fill_rect(x + w - t, y + (h // 2), t, (h // 2) - t, color)
    if s[3]:
        oled.fill_rect(x + t, y + h - t, w - 2 * t, t, color)
    if s[4]:
        oled.fill_rect(x, y + (h // 2), t, (h // 2) - t, color)
    if s[5]:
        oled.fill_rect(x, y + t, t, (h // 2) - t, color)
    if s[6]:
        oled.fill_rect(x + t, y + (h // 2) - (t // 2), w - 2 * t, t, color)


def draw_big_colon(x, y, h=28, t=3, color=1):

    dot_size = t
    top_y = y + (h // 3) - (dot_size // 2)
    bottom_y = y + ((2 * h) // 3) - (dot_size // 2)

    oled.fill_rect(x, top_y, dot_size, dot_size, color)
    oled.fill_rect(x, bottom_y, dot_size, dot_size, color)


def draw_big_time(hours, minutes, seconds, y=18):

    d = "{:02d}{:02d}".format(hours, minutes)

    digit_w = 14
    digit_h = 28
    digit_space = 2
    colon_w = 3
    block_space = 2

    total_w = (digit_w * 4) + (digit_space * 3) + colon_w + (block_space * 2)
    x = (WIDTH - total_w) // 2

    draw_big_digit(x, y, d[0], digit_w, digit_h)
    x += digit_w + digit_space
    draw_big_digit(x, y, d[1], digit_w, digit_h)
    x += digit_w + block_space

    if seconds % 2 == 0:
        draw_big_colon(x, y, digit_h, 3)

    x += colon_w + block_space
    draw_big_digit(x, y, d[2], digit_w, digit_h)
    x += digit_w + digit_space
    draw_big_digit(x, y, d[3], digit_w, digit_h)


def get_clock_time_components():

    # Prefer RTC/local time if it was already set by the firmware.
    now = time.localtime()

    if now[0] >= 2024:
        return now[3], now[4], now[5], "RTC"

    # Fallback: show uptime-based clock when RTC is not configured.
    elapsed = time.ticks_ms() // 1000

    hours = (elapsed // 3600) % 24
    minutes = (elapsed // 60) % 60
    seconds = elapsed % 60

    return hours, minutes, seconds, "UP"


def calibrate_accel_baseline(samples=40, delay_ms=10):

    global ax_zero
    global ay_zero
    global az_zero

    sum_ax = 0
    sum_ay = 0
    sum_az = 0

    ok = 0

    for _ in range(samples):
        try:
            ax, ay, az, _, _, _ = read_motion6(i2c)
            sum_ax += ax
            sum_ay += ay
            sum_az += az
            ok += 1
        except Exception:
            pass

        time.sleep_ms(delay_ms)

    if ok > 0:
        ax_zero = sum_ax // ok
        ay_zero = sum_ay // ok
        az_zero = sum_az // ok


def stabilize_emotion(next_emotion):

    global display_emotion
    global pending_emotion
    global pending_count

    # Keep critical states responsive.
    if next_emotion == "vomit" or next_emotion == "clock":
        display_emotion = next_emotion
        pending_emotion = ""
        pending_count = 0
        return display_emotion

    if next_emotion == display_emotion:
        pending_emotion = ""
        pending_count = 0
        return display_emotion

    if next_emotion != pending_emotion:
        pending_emotion = next_emotion
        pending_count = 1
        return display_emotion

    pending_count += 1

    # Angry should be easy to leave to avoid getting stuck.
    required_frames = EMOTION_SWITCH_FRAMES
    if display_emotion == "angry" and next_emotion != "angry":
        required_frames = ANGRY_RELEASE_FRAMES
    elif display_emotion == "sleepy" and next_emotion != "sleepy":
        required_frames = 1

    if pending_count >= required_frames:
        display_emotion = pending_emotion
        pending_emotion = ""
        pending_count = 0

    return display_emotion

# ==========================================
# CLOCK MODE
# ==========================================

def draw_clock():

    oled.fill(0)

    hours, minutes, seconds, source = get_clock_time_components()

    now = time.localtime()
    has_rtc_date = now[0] >= 2024

    if has_rtc_date:
        date_str = "{:02d}/{:02d}/{:04d}".format(now[2], now[1], now[0])
    else:
        date_str = "SIN FECHA"

    # Digital clock layout: big HH:MM and compact details.
    oled.rect(0, 0, WIDTH, HEIGHT, 1)
    oled.fill_rect(2, 2, WIDTH - 4, 12, 1)
    oled.text("CLOCK", 4, 4, 0)
    oled.text(source, 104, 4, 0)

    draw_big_time(hours, minutes, seconds, 18)

    sec_str = "{:02d}s".format(seconds)
    oled.text(sec_str, 96, 16, 1)

    date_x = (WIDTH - (len(date_str) * 8)) // 2
    oled.text(date_str, date_x, 52, 1)

    oled.show()

# ==========================================
# DRAW EMOTIONS
# ==========================================

def draw_emotion(emotion):

    global z_offset
    global vomit_offset
    global excited_offset

    oled.fill(0)

    # ======================================
    # HAPPY
    # ======================================

    if emotion == "happy":

        fill_circle(40, 32, 15, 1)
        fill_circle(88, 32, 15, 1)

        fill_circle(40, 32, 5, 0)
        fill_circle(88, 32, 5, 0)

        draw_arc(64, 48, 10, 0, 180, 1)

    # ======================================
    # SAD
    # ======================================

    elif emotion == "sad":

        fill_circle(40, 36, 12, 1)
        fill_circle(88, 36, 12, 1)

        fill_circle(40, 40, 4, 0)
        fill_circle(88, 40, 4, 0)

        oled.line(28, 20, 52, 28, 1)
        oled.line(76, 28, 100, 20, 1)

        draw_arc(64, 54, 10, 180, 360, 1)

    # ======================================
    # ANGRY
    # ======================================

    elif emotion == "angry":

        fill_circle(40, 32, 12, 1)
        fill_circle(88, 32, 12, 1)

        fill_circle(40, 28, 5, 0)
        fill_circle(88, 28, 5, 0)

        oled.line(28, 20, 52, 24, 1)
        oled.line(76, 24, 100, 20, 1)

        oled.hline(54, 50, 20, 1)

    # ======================================
    # SURPRISED
    # ======================================

    elif emotion == "surprised":

        fill_circle(40, 32, 18, 1)
        fill_circle(88, 32, 18, 1)

        fill_circle(40, 32, 6, 0)
        fill_circle(88, 32, 6, 0)

        oled.rect(58, 48, 10, 10, 1)

    # ======================================
    # SLEEPY
    # ======================================

    elif emotion == "sleepy":

        oled.hline(28, 32, 24, 1)
        oled.hline(76, 32, 24, 1)

        oled.hline(54, 50, 20, 1)

        oled.text("Z", 100, 20 - z_offset, 1)
        oled.text("z", 108, 10 - z_offset // 2, 1)

        z_offset += 1

        if z_offset > 30:
            z_offset = 0

    # ======================================
    # EXCITED
    # ======================================

    elif emotion == "excited":

        jump = excited_offset % 3

        # Big bright eyes with clearer pupils.
        fill_circle(40, 31 - jump, 16, 1)
        fill_circle(88, 31 - jump, 16, 1)

        fill_circle(40, 29 - jump, 6, 0)
        fill_circle(88, 29 - jump, 6, 0)
        oled.fill_rect(38, 27 - jump, 2, 2, 1)
        oled.fill_rect(86, 27 - jump, 2, 2, 1)

        # Eyebrows tilted up for excited expression.
        oled.line(28, 16, 50, 20, 1)
        oled.line(78, 20, 100, 16, 1)

        # Clear smiling mouth.
        draw_arc(64, 45, 16, 10, 170, 1)
        draw_arc(64, 46, 16, 10, 170, 1)

        # Small dynamic sparkles around the face.
        sparkle_shift = excited_offset % 4
        oled.pixel(18 + sparkle_shift, 18, 1)
        oled.pixel(22 + sparkle_shift, 14, 1)
        oled.pixel(106 - sparkle_shift, 18, 1)
        oled.pixel(102 - sparkle_shift, 14, 1)

        # Keep WOW text but away from eyes.
        oled.text("WOW!", 44, 2, 1)

        excited_offset += 1

        if excited_offset > 8:
            excited_offset = 0

    # ======================================
    # VOMIT
    # ======================================

    elif emotion == "vomit":

        fill_circle(40, 32, 14, 1)
        fill_circle(88, 32, 14, 1)

        oled.line(34, 26, 46, 38, 0)
        oled.line(46, 26, 34, 38, 0)

        oled.line(82, 26, 94, 38, 0)
        oled.line(94, 26, 82, 38, 0)

        oled.fill_rect(56, 46, 16, 8, 1)
        oled.fill_rect(59, 48, 10, 4, 0)

        for y in range(18):

            width = 6 - (y // 4)

            for x in range(-width, width):

                oled.pixel(
                    64 + x + ((vomit_offset % 2) * 2),
                    54 + y,
                    1
                )

        oled.fill_rect(58, 62 - vomit_offset, 2, 2, 1)
        oled.fill_rect(70, 66 - (vomit_offset * 2), 2, 2, 1)

        vomit_offset += 1

        if vomit_offset > 8:
            vomit_offset = 0

    # ======================================
    # CLOCK
    # ======================================

    elif emotion == "clock":

        draw_clock()
        return

    oled.show()

# ==========================================
# WAKEUP ANIMATION
# ==========================================

def wakeup_sequence():

    for _ in range(10):

        draw_emotion("sleepy")
        time.sleep(0.12)

    for _ in range(8):

        draw_emotion("surprised")
        time.sleep(0.08)

    for _ in range(15):

        draw_emotion("excited")
        time.sleep(0.08)

# ==========================================
# DETECT EMOTION
# ==========================================

def detect_emotion(ax, ay, az, gx, gy, gz):

    global idle_counter
    global ax_zero
    global ay_zero
    global az_zero
    global sleep_frames
    global wake_emotion
    global wake_timer
    global circular_counter

    ax_rel = ax - ax_zero
    ay_rel = ay - ay_zero
    az_rel = az - az_zero

    motion = abs(gx) + abs(gy) + abs(gz)
    accel_activity = abs(ax_rel) + abs(ay_rel) + abs(az_rel)

    # ======================================
    # IDLE CLOCK MODE
    # ======================================

    if motion < 1500:

        idle_counter += 1

    else:

        idle_counter = 0

    if idle_counter > 50:
        return "clock"

    # ======================================
    # STRONG SHAKE -> VOMIT (HIGHEST PRIORITY)
    # ======================================

    if motion > 85000:
        sleep_frames = 0
        wake_timer = 0
        return "vomit"

    # ======================================
    # SLEEP / WAKE CLASSIFICATION
    # ======================================

    in_sleep_state = (motion < 2500 and accel_activity < 9000)

    if in_sleep_state:
        sleep_frames += 1
    else:
        # Just woke up from a stable sleep state.
        if sleep_frames >= 12:
            # Quick and strong wake: angry for a short time.
            if motion > 22000 or accel_activity > 24000:
                wake_emotion = "angry"
                wake_timer = WAKE_EMOTION_FRAMES
            # Gentle wake: happy.
            elif motion > 3500 or accel_activity > 10000:
                wake_emotion = "happy"
                wake_timer = WAKE_EMOTION_FRAMES

        sleep_frames = 0

    # Keep wake transition emotion for a few frames, then release.
    if wake_timer > 0:
        wake_timer -= 1
        return wake_emotion

    # ======================================
    # ACCELEROMETER RANGES (PRIORITY)
    # ======================================

    # Relative tilt thresholds from startup baseline.
    happy_thr = 5500
    sad_thr = -5500

    # Happy / sad based on X tilt.
    if ax_rel >= happy_thr:
        return "happy"

    if ax_rel <= sad_thr:
        return "sad"

    # ======================================
    # CIRCULAR MOVEMENT -> EXCITED/ALEGRE
    # ======================================

    circular_candidate = (
        6000 < abs(gx) < 32000 and
        6000 < abs(gy) < 32000 and
        abs(gz) < 26000
    )

    if circular_candidate:
        circular_counter += 1
    else:
        circular_counter = 0

    if circular_counter >= 3:
        return "excited"

    # ======================================
    # EXCITED / STRONG GYRO
    # ======================================

    elif motion > 45000:
        return "excited"

    # ======================================
    # SURPRISED
    # ======================================

    elif az_rel > 7000:
        return "surprised"

    # ======================================
    # SLEEPY
    # ======================================

    elif motion < 3000:
        return "sleepy"

    return "excited"

# ==========================================
# STARTUP
# ==========================================

wakeup_sequence()

# Calibrate after startup animation to avoid movement during wakeup.
calibrate_accel_baseline()

# ==========================================
# MAIN LOOP
# ==========================================

while True:

    try:

        ax, ay, az, gx, gy, gz = read_motion6(i2c)

        emotion = detect_emotion(
            ax,
            ay,
            az,
            gx,
            gy,
            gz
        )

        # Throttled debug logs to inspect accelerometer behavior.
        debug_print_div += 1
        if debug_print_div >= 4:
            debug_print_div = 0
            print(
                "ACC:", ax, ay, az,
                "REL:", ax - ax_zero, ay - ay_zero, az - az_zero,
                "GYR:", gx, gy, gz,
                "EMO:", emotion
            )

        stable_emotion = stabilize_emotion(emotion)
        draw_emotion(stable_emotion)

    except Exception as e:

        oled.fill(0)

        oled.text("BMI160 ERROR", 10, 20)
        oled.text(str(e), 0, 40)

        oled.show()

        print("ERROR:", e)

    time.sleep(0.08)
