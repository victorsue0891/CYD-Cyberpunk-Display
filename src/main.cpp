/**
 * ????????????????????????????????????????????????????????????????????
 * ?? ?餅挺璈???- Ghost in the Shell : NEURAL AUGMENTED REALITY     ??
 * ?? ESP32-2432S028 (Cheap Yellow Display) 320x240                  ??
 * ?? Style: AR overlay + Wireframe + Glitch + Dense Typography      ??
 * ??                                                                 ??
 * ?? 1. 蟡??湧?AR ?? - 憭?閬????祈?獢                      ??
 * ?? 2. ?祉???蝔蹂漱蝜?- ?琿????雯?潦儔擃???                   ??
 * ?? 3. ???? Glitch - 閮?撟脫畾蔣??閮???                   ??
 * ?? 4. 璈１?? - 撖?摨扳?蝣潦?蝒蒂??                             ??
 * ????????????????????????????????????????????????????????????????????
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <time.h>
#include <math.h>
#include <SPIFFS.h>

// Custom Orbitron font (Bank Gothic style)
LV_FONT_DECLARE(orbitron_28);
LV_FONT_DECLARE(orbitron_10);

// Weather Icons font
LV_FONT_DECLARE(weather_icons_28);

// Laughing Man background image
LV_IMG_DECLARE(laughing_man);

// ????????????????????????????????????????????????????????
// WiFi
// ????????????????????????????????????????????????????????
static char wifi_ssid[64] = "";
static char wifi_pass[64] = "";
static char timezone_str[64] = "Asia/Taipei";
static int gmt_offset = 8;
static char weather_url[256];

void build_weather_url() {
    char tz_encoded[64];
    int j = 0;
    for (int i = 0; timezone_str[i] && j < 60; i++) {
        if (timezone_str[i] == '/') {
            tz_encoded[j++] = '%'; tz_encoded[j++] = '2'; tz_encoded[j++] = 'F';
        } else {
            tz_encoded[j++] = timezone_str[i];
        }
    }
    tz_encoded[j] = '\0';
    snprintf(weather_url, sizeof(weather_url),
        "https://api.open-meteo.com/v1/forecast?"
        "latitude=25.033&longitude=121.5654"
        "&current_weather=true&timezone=%s", tz_encoded);
}


// ????????????????????????????????????????????????????????
// GITS Color Palette - ?餅挺?
// ?箇?銝餉嚗摮?/??/瘛梁換 ???啣?扳雿???
// ?啣?頛嚗?暺?瘛梁/頠? ??瞏格??唳??芯??賢?
// 擃?霅血?嚗??寧?蝝?鈭格?/霅衣內蝝???頝唾?瑁
// ????????????????????????????????????????????????????????
// ?箇?銝餉 (Electric Blue / Cyan / Purple)
#define GS_BLACK        lv_color_hex(0x020408)
#define GS_ELEC_BLUE    lv_color_hex(0x0088FF)
#define GS_CYAN         lv_color_hex(0x00FFEE)
#define GS_DIM_CYAN     lv_color_hex(0x004455)
#define GS_PURPLE       lv_color_hex(0x8844FF)
#define GS_DIM_PURPLE   lv_color_hex(0x2A1155)
// ?啣?頛 (Dark / Grey / Military Green)
#define GS_DARK_BG      lv_color_hex(0x080C10)
#define GS_MIL_GREEN    lv_color_hex(0x2A4A2A)
#define GS_DIM_GREEN    lv_color_hex(0x1A3320)
#define GS_DARK_GREY    lv_color_hex(0x1A1A22)
#define GS_GRID_LINE    lv_color_hex(0x0E1820)
#define GS_GRID_BRIGHT  lv_color_hex(0x1A2E3A)
// 擃??郎? (Neon Pink / Orange / Red)
#define GS_NEON_PINK    lv_color_hex(0xFF0088)
#define GS_ORANGE       lv_color_hex(0xFF6600)
#define GS_DIM_ORANGE   lv_color_hex(0x663300)
#define GS_RED          lv_color_hex(0xFF1133)
#define GS_GLITCH       lv_color_hex(0xFF0066)
// 頛
#define GS_WHITE        lv_color_hex(0xCCDDEE)
#define GS_GREEN        lv_color_hex(0x00FF88)

// ????????????????????????????????????????????????????????
// Display & LVGL
// ????????????????????????????????????????????????????????
TFT_eSPI tft = TFT_eSPI();
static const uint32_t SCREEN_W = 320;
static const uint32_t SCREEN_H = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_W * 20];

// ????????????????????????????????????????????????????????
// UI Elements - Dense Multi-Window
// ????????????????????????????????????????????????????????
// Main data
static lv_obj_t* lbl_time;
static lv_obj_t* lbl_date;
static lv_obj_t* lbl_temp;
static lv_obj_t* lbl_weather_icon;
static lv_obj_t* lbl_wind;
static lv_obj_t* lbl_status;
// Radar (3 concentric rings: large, mid, small)
static lv_obj_t* radar_arc;    // large
static lv_obj_t* radar_mid;    // medium
static lv_obj_t* radar_small;  // small
// Prosthetic/Biometric panel (蝢拚???～??
static lv_obj_t* lbl_cyber_brain;
static lv_obj_t* lbl_prosthetic;
static lv_obj_t* lbl_barrier;
static lv_obj_t* lbl_sync_rate;
// Data stream
static lv_obj_t* lbl_stream1;
static lv_obj_t* lbl_stream2;
static lv_obj_t* lbl_stream3;
static lv_obj_t* lbl_stream4;
// Glitch label (overlays randomly)
static lv_obj_t* lbl_glitch;
// Scan line
static lv_obj_t* scan_line;
// Waveform
static lv_obj_t* waveform_line;
static lv_point_t wave_pts[21];
// Corner brackets (AR window frame)
static lv_obj_t* lbl_corner_tl;
static lv_obj_t* lbl_corner_tr;
static lv_obj_t* lbl_corner_bl;
static lv_obj_t* lbl_corner_br;
// Dense coordinate labels
static lv_obj_t* lbl_coord1;
static lv_obj_t* lbl_coord2;
static lv_obj_t* lbl_uptime;

// ????????????????????????????????????????????????????????
// State
// ????????????????????????????????????????????????????????
static unsigned long last_weather_update = 0;
static const unsigned long WEATHER_INTERVAL = 600000;
static unsigned long boot_time;
static bool wifi_connected = false;
static int radar_angle_lg = 0;
static int radar_angle_md = 0;
static int radar_angle_sm = 0;
static int scan_y = 0;
static int glitch_counter = 0;
static bool glitch_active = false;
static int wave_phase = 0;

// ????????????????????????????????????????????????????????
// Weather Data
// ????????????????????????????????????????????????????????
struct WeatherData {
    float temperature;
    float wind_speed;
    int weather_code;
    bool valid;
} weather = {0, 0, 0, false};

// ????????????????????????????????????????????????????????
// Display flush
// ????????????????????????????????????????????????????????
void display_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// Weather Icons codepoints (erikflowers/weather-icons)
#define WI_SUNNY      "\xEF\x80\x8D"  // U+F00D
#define WI_CLOUDY_DAY "\xEF\x80\x82"  // U+F002
#define WI_CLOUDY     "\xEF\x80\x93"  // U+F013
#define WI_FOG        "\xEF\x80\x94"  // U+F014
#define WI_RAIN       "\xEF\x80\x99"  // U+F019
#define WI_SHOWERS    "\xEF\x80\x9A"  // U+F01A
#define WI_SNOW       "\xEF\x80\x9B"  // U+F01B
#define WI_SPRINKLE   "\xEF\x80\x9C"  // U+F01C
#define WI_STORM      "\xEF\x80\x9E"  // U+F01E
#define WI_HAIL       "\xEF\x80\x95"  // U+F015

// Map Open-Meteo WMO weather code to icon string
const char* weather_code_to_icon(int code) {
    if (code == 0) return WI_SUNNY;
    if (code <= 2) return WI_CLOUDY_DAY;
    if (code == 3) return WI_CLOUDY;
    if (code == 45 || code == 48) return WI_FOG;
    if (code >= 51 && code <= 57) return WI_SPRINKLE;
    if (code >= 61 && code <= 67) return WI_RAIN;
    if (code >= 71 && code <= 77) return WI_SNOW;
    if (code >= 80 && code <= 82) return WI_SHOWERS;
    if (code >= 85 && code <= 86) return WI_SNOW;
    if (code >= 95) return WI_STORM;
    return WI_CLOUDY;
}

// Fetch Weather
void fetch_weather() {
    if (!wifi_connected) return;
    build_weather_url();
    HTTPClient http;
    http.begin(weather_url);
    http.setTimeout(10000);

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            weather.temperature = doc["current_weather"]["temperature"] | 0.0f;
            weather.wind_speed = doc["current_weather"]["windspeed"] | 0.0f;
            weather.weather_code = doc["current_weather"]["weathercode"] | 0;
            weather.valid = true;
        }
    }
    http.end();
}

// ????????????????????????????????????????????????????????
// Generate hex stream
// ????????????????????????????????????????????????????????
void generate_hex_stream(char* buf, int len) {
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < len - 1; i++) {
        if (i % 3 == 2) buf[i] = ' ';
        else buf[i] = hex[random(16)];
    }
    buf[len - 1] = '\0';
}

// ????????????????????????????????????????????????????????
// Generate glitch text (corrupted characters)
// ????????????????????????????????????????????????????????
void generate_glitch_text(char* buf, int len) {
    const char glitch_chars[] = "!@#$%&*/<>{}[]|~^`0123456789";
    for (int i = 0; i < len - 1; i++) {
        buf[i] = glitch_chars[random(sizeof(glitch_chars) - 1)];
    }
    buf[len - 1] = '\0';
}

// ????????????????????????????????????????????????????????
// GITS data messages (Section-9 / Cyberbrain)
// ????????????????????????????????????????????????????????
static const char* stream_msgs[] = {
    "GHOST:SYNC 99.7% HASH:OK",
    "BARRIER.MAZE >> ACTIVE L7",
    "TACHIKOMA[4/4] NET.LINKED",
    "E-BRAIN:ENCRYPT AES-512",
    "THERMO-OPTIC CAMO:STANDBY",
    "PUPPET.MASTER >> /dev/null",
    "MOTOKO.PROC:0x7FFF2A4B",
    "SATELLITE UPLINK:LOCKED",
    "MEM.DUMP:0xDEAD 0xBEEF",
    "FIREWALL DEPTH:7 STATUS:OK",
    "NAV.SAT:25.033N 121.565E",
    "SIGINT INTERCEPT:PASSIVE",
    "DEEP.SCAN:SECTOR-7 CLEAR",
    "PROSTHETIC.CTRL:NOMINAL",
    "E-WARFARE:DECOY ACTIVE",
    "NODE.MESH:12/12 ONLINE",
};
static int stream_idx = 0;

const char* get_stream_msg() {
    const char* msg = stream_msgs[stream_idx];
    stream_idx = (stream_idx + 1) % 16;
    return msg;
}

// ????????????????????????????????????????????????????????
// Create AR-style corner brackets (閬?獢)
// ????????????????????????????????????????????????????????
void create_corner_brackets(lv_obj_t* parent) {
    // Top-left
    lbl_corner_tl = lv_label_create(parent);
    lv_label_set_text(lbl_corner_tl, "[");
    lv_obj_set_style_text_color(lbl_corner_tl, GS_PURPLE, 0);
    lv_obj_set_style_text_font(lbl_corner_tl, &orbitron_28, 0);
    lv_obj_set_pos(lbl_corner_tl, 0, 0);

    // Top-right
    lbl_corner_tr = lv_label_create(parent);
    lv_label_set_text(lbl_corner_tr, "]");
    lv_obj_set_style_text_color(lbl_corner_tr, GS_PURPLE, 0);
    lv_obj_set_style_text_font(lbl_corner_tr, &orbitron_28, 0);
    lv_obj_set_pos(lbl_corner_tr, 308, 0);

    // Bottom-left
    lbl_corner_bl = lv_label_create(parent);
    lv_label_set_text(lbl_corner_bl, "[");
    lv_obj_set_style_text_color(lbl_corner_bl, GS_DIM_PURPLE, 0);
    lv_obj_set_style_text_font(lbl_corner_bl, &orbitron_28, 0);
    lv_obj_set_pos(lbl_corner_bl, 0, 220);

    // Bottom-right
    lbl_corner_br = lv_label_create(parent);
    lv_label_set_text(lbl_corner_br, "]");
    lv_obj_set_style_text_color(lbl_corner_br, GS_DIM_PURPLE, 0);
    lv_obj_set_style_text_font(lbl_corner_br, &orbitron_28, 0);
    lv_obj_set_pos(lbl_corner_br, 308, 220);
}

// ????????????????????????????????????????????????????????
// Create dense grid overlay (蝬脫?)
// ????????????????????????????????????????????????????????
void create_grid_overlay(lv_obj_t* scr) {
    // Fine horizontal grid (every 30px)
    static lv_point_t h_pts[2] = {{0, 0}, {320, 0}};
    for (int i = 1; i < 8; i++) {
        lv_obj_t* gl = lv_line_create(scr);
        lv_line_set_points(gl, h_pts, 2);
        lv_obj_set_style_line_color(gl, GS_GRID_LINE, 0);
        lv_obj_set_style_line_width(gl, 1, 0);
        lv_obj_set_pos(gl, 0, i * 30);
    }
    // Fine vertical grid (every 32px)
    static lv_point_t v_pts[2] = {{0, 0}, {0, 240}};
    for (int i = 1; i < 10; i++) {
        lv_obj_t* gl = lv_line_create(scr);
        lv_line_set_points(gl, v_pts, 2);
        lv_obj_set_style_line_color(gl, GS_GRID_LINE, 0);
        lv_obj_set_style_line_width(gl, 1, 0);
        lv_obj_set_pos(gl, i * 32, 0);
    }
    // Bright center crosshair lines
    static lv_point_t ch_pts[2] = {{0, 0}, {320, 0}};
    lv_obj_t* chl = lv_line_create(scr);
    lv_line_set_points(chl, ch_pts, 2);
    lv_obj_set_style_line_color(chl, GS_GRID_BRIGHT, 0);
    lv_obj_set_style_line_width(chl, 1, 0);
    lv_obj_set_pos(chl, 0, 120);

    static lv_point_t cv_pts[2] = {{0, 0}, {0, 240}};
    lv_obj_t* cvl = lv_line_create(scr);
    lv_line_set_points(cvl, cv_pts, 2);
    lv_obj_set_style_line_color(cvl, GS_GRID_BRIGHT, 0);
    lv_obj_set_style_line_width(cvl, 1, 0);
    lv_obj_set_pos(cvl, 160, 0);
}

// ????????????????????????????????????????????????????????
// Create waveform (璅⊥?行郭/靽∟?瘜Ｗ耦)
// ????????????????????????????????????????????????????????
void create_waveform(lv_obj_t* scr) {
    // Initialize waveform points
    for (int i = 0; i < 21; i++) {
        wave_pts[i].x = i * 6;
        wave_pts[i].y = 15 + random(10);
    }
    waveform_line = lv_line_create(scr);
    lv_line_set_points(waveform_line, wave_pts, 21);
    lv_obj_set_style_line_color(waveform_line, GS_GREEN, 0);
    lv_obj_set_style_line_width(waveform_line, 2, 0);
    lv_obj_set_style_line_opa(waveform_line, LV_OPA_COVER, 0);
    lv_obj_set_pos(waveform_line, 140, 140);
}

// ????????????????????????????????????????????????????????
// Create GITS-style AR HUD
// ????????????????????????????????????????????????????????
void create_ui() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, GS_BLACK, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // ─── Layer -1: Laughing Man background (半透明底圖) ───
    lv_obj_t* bg_img = lv_img_create(scr);
    lv_img_set_src(bg_img, &laughing_man);
    lv_obj_set_pos(bg_img, 27, 0);  // centered: (320-267)/2 = 27
    lv_obj_clear_flag(bg_img, LV_OBJ_FLAG_CLICKABLE);

    // ─── Layer 0: Grid overlay ───
    create_grid_overlay(scr);

    // ??? Layer 1: Corner brackets (AR window frame) ???
    create_corner_brackets(scr);

    // ??? Layer 2: Radar (left panel) ???
    // Randomize start angles for each ring
    radar_angle_lg = random(0, 360);
    radar_angle_md = random(0, 360);
    radar_angle_sm = random(0, 360);

    // Large ring (110x110)
    radar_arc = lv_arc_create(scr);
    lv_obj_set_size(radar_arc, 110, 110);
    lv_obj_set_pos(radar_arc, 8, 18);
    lv_arc_set_rotation(radar_arc, 0);
    lv_arc_set_range(radar_arc, 0, 360);
    lv_arc_set_value(radar_arc, radar_angle_lg);
    lv_arc_set_bg_angles(radar_arc, 0, 360);
    lv_obj_set_style_arc_color(radar_arc, GS_DIM_CYAN, 0);
    lv_obj_set_style_arc_width(radar_arc, 2, 0);
    lv_obj_set_style_arc_opa(radar_arc, 140, 0);
    lv_obj_set_style_arc_color(radar_arc, GS_ELEC_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(radar_arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(radar_arc, 180, LV_PART_INDICATOR);
    lv_obj_remove_style(radar_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(radar_arc, LV_OBJ_FLAG_CLICKABLE);

    // Medium ring (76x76)
    radar_mid = lv_arc_create(scr);
    lv_obj_set_size(radar_mid, 76, 76);
    lv_obj_set_pos(radar_mid, 25, 35);
    lv_arc_set_rotation(radar_mid, 0);
    lv_arc_set_range(radar_mid, 0, 360);
    lv_arc_set_value(radar_mid, radar_angle_md);
    lv_arc_set_bg_angles(radar_mid, 0, 360);
    lv_obj_set_style_arc_color(radar_mid, GS_DIM_CYAN, 0);
    lv_obj_set_style_arc_width(radar_mid, 1, 0);
    lv_obj_set_style_arc_opa(radar_mid, 120, 0);
    lv_obj_set_style_arc_color(radar_mid, GS_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(radar_mid, 2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(radar_mid, 160, LV_PART_INDICATOR);
    lv_obj_remove_style(radar_mid, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(radar_mid, LV_OBJ_FLAG_CLICKABLE);

    // Small ring (44x44)
    radar_small = lv_arc_create(scr);
    lv_obj_set_size(radar_small, 44, 44);
    lv_obj_set_pos(radar_small, 41, 51);
    lv_arc_set_rotation(radar_small, 0);
    lv_arc_set_range(radar_small, 0, 360);
    lv_arc_set_value(radar_small, radar_angle_sm);
    lv_arc_set_bg_angles(radar_small, 0, 360);
    lv_obj_set_style_arc_color(radar_small, GS_DIM_CYAN, 0);
    lv_obj_set_style_arc_width(radar_small, 1, 0);
    lv_obj_set_style_arc_opa(radar_small, 100, 0);
    lv_obj_set_style_arc_color(radar_small, GS_PURPLE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(radar_small, 2, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(radar_small, 140, LV_PART_INDICATOR);
    lv_obj_remove_style(radar_small, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(radar_small, LV_OBJ_FLAG_CLICKABLE);

    // Radar crosshair center
    static lv_point_t rch[] = {{0, 0}, {16, 0}};
    lv_obj_t* rchl = lv_line_create(scr);
    lv_line_set_points(rchl, rch, 2);
    lv_obj_set_style_line_color(rchl, GS_ELEC_BLUE, 0);
    lv_obj_set_style_line_width(rchl, 1, 0);
    lv_obj_set_pos(rchl, 55, 73);

    static lv_point_t rcv[] = {{0, 0}, {0, 16}};
    lv_obj_t* rcvl = lv_line_create(scr);
    lv_line_set_points(rcvl, rcv, 2);
    lv_obj_set_style_line_color(rcvl, GS_ELEC_BLUE, 0);
    lv_obj_set_style_line_width(rcvl, 1, 0);
    lv_obj_set_pos(rcvl, 63, 65);

    // Radar label
    lv_obj_t* lbl_radar_tag = lv_label_create(scr);
    lv_label_set_text(lbl_radar_tag, "NAV.SAT");
    lv_obj_set_style_text_color(lbl_radar_tag, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_radar_tag, &orbitron_10, 0);
    lv_obj_set_pos(lbl_radar_tag, 40, 130);

    // ??? Layer 3: Top header (蝟餌絞???) ???
    lv_obj_t* lbl_sys = lv_label_create(scr);
    lv_label_set_text(lbl_sys, "SEC-9//AR.OVERLAY v2.7");
    lv_obj_set_style_text_color(lbl_sys, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_sys, &orbitron_10, 0);
    lv_obj_set_pos(lbl_sys, 14, 3);

    lbl_status = lv_label_create(scr);
    lv_label_set_text(lbl_status, "NEURAL:--");
    lv_obj_set_style_text_color(lbl_status, GS_DIM_ORANGE, 0);
    lv_obj_set_style_text_font(lbl_status, &orbitron_10, 0);
    lv_obj_set_pos(lbl_status, 246, 3);

    // Top separator
    static lv_point_t sep_pts[] = {{0, 0}, {296, 0}};
    lv_obj_t* sep = lv_line_create(scr);
    lv_line_set_points(sep, sep_pts, 2);
    lv_obj_set_style_line_color(sep, GS_ELEC_BLUE, 0);
    lv_obj_set_style_line_width(sep, 1, 0);
    lv_obj_set_style_line_opa(sep, LV_OPA_70, 0);
    lv_obj_set_pos(sep, 12, 15);

    // ??? Layer 4: Right panel - Time/Weather (銝駁＊蝷? ???
    // Date (dense format)
    lbl_date = lv_label_create(scr);
    lv_label_set_text(lbl_date, "2026.05.22");
    lv_obj_set_style_text_color(lbl_date, GS_ELEC_BLUE, 0);
    lv_obj_set_style_text_font(lbl_date, &orbitron_28, 0);
    lv_obj_set_pos(lbl_date, 124, 18);

    // Time (large, primary)
    lbl_time = lv_label_create(scr);
    lv_label_set_text(lbl_time, "00:00:00");
    lv_obj_set_style_text_color(lbl_time, GS_CYAN, 0);
    lv_obj_set_style_text_font(lbl_time, &orbitron_28, 0);
    lv_obj_set_pos(lbl_time, 124, 52);

    // Temperature
    lbl_temp = lv_label_create(scr);
    lv_label_set_text(lbl_temp, "--.- C");
    lv_obj_set_style_text_color(lbl_temp, GS_ORANGE, 0);
    lv_obj_set_style_text_font(lbl_temp, &orbitron_28, 0);
    lv_obj_set_pos(lbl_temp, 170, 86);

    // Weather Icon
    lbl_weather_icon = lv_label_create(scr);
    lv_label_set_text(lbl_weather_icon, WI_CLOUDY);
    lv_obj_set_style_text_color(lbl_weather_icon, GS_CYAN, 0);
    lv_obj_set_style_text_font(lbl_weather_icon, &weather_icons_28, 0);
    lv_obj_set_pos(lbl_weather_icon, 124, 86);

    // Wind
    lbl_wind = lv_label_create(scr);
    lv_label_set_text(lbl_wind, "WIND:-- km/h CODE:--");
    lv_obj_set_style_text_color(lbl_wind, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_wind, &orbitron_10, 0);
    lv_obj_set_pos(lbl_wind, 138, 92);

    // Coordinates dense
    lbl_coord1 = lv_label_create(scr);
    lv_label_set_text(lbl_coord1, "LAT:25.033N LON:121.565E");
    lv_obj_set_style_text_color(lbl_coord1, GS_DIM_GREEN, 0);
    lv_obj_set_style_text_font(lbl_coord1, &orbitron_10, 0);
    lv_obj_set_pos(lbl_coord1, 138, 104);

    lbl_coord2 = lv_label_create(scr);
    lv_label_set_text(lbl_coord2, "ALT:9m AZ:247.3 EL:62.1");
    lv_obj_set_style_text_color(lbl_coord2, GS_DIM_GREEN, 0);
    lv_obj_set_style_text_font(lbl_coord2, &orbitron_10, 0);
    lv_obj_set_pos(lbl_coord2, 138, 115);

    // ??? Layer 5: Left panel - 蝢拚? Prosthetic data ???
    lbl_cyber_brain = lv_label_create(scr);
    lv_label_set_text(lbl_cyber_brain, "E-BRAIN:ACTIVE");
    lv_obj_set_style_text_color(lbl_cyber_brain, GS_ELEC_BLUE, 0);
    lv_obj_set_style_text_font(lbl_cyber_brain, &orbitron_10, 0);
    lv_obj_set_pos(lbl_cyber_brain, 8, 138);

    lbl_prosthetic = lv_label_create(scr);
    lv_label_set_text(lbl_prosthetic, "PROSTH:98.2%");
    lv_obj_set_style_text_color(lbl_prosthetic, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_prosthetic, &orbitron_10, 0);
    lv_obj_set_pos(lbl_prosthetic, 8, 150);

    lbl_barrier = lv_label_create(scr);
    lv_label_set_text(lbl_barrier, "BARRIER:L7");
    lv_obj_set_style_text_color(lbl_barrier, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_barrier, &orbitron_10, 0);
    lv_obj_set_pos(lbl_barrier, 8, 162);

    lbl_sync_rate = lv_label_create(scr);
    lv_label_set_text(lbl_sync_rate, "SYNC:99.7%");
    lv_obj_set_style_text_color(lbl_sync_rate, GS_ELEC_BLUE, 0);
    lv_obj_set_style_text_font(lbl_sync_rate, &orbitron_10, 0);
    lv_obj_set_pos(lbl_sync_rate, 8, 174);

    // Uptime
    lbl_uptime = lv_label_create(scr);
    lv_label_set_text(lbl_uptime, "UP:0000s");
    lv_obj_set_style_text_color(lbl_uptime, GS_DIM_GREEN, 0);
    lv_obj_set_style_text_font(lbl_uptime, &orbitron_10, 0);
    lv_obj_set_pos(lbl_uptime, 80, 174);

    // ??? Layer 6: Waveform (?行郭/靽∟?) ???
    create_waveform(scr);

    // ??? Layer 7: Middle separator + data window ???
    static lv_point_t mid_sep[] = {{0, 0}, {180, 0}};
    lv_obj_t* msep = lv_line_create(scr);
    lv_line_set_points(msep, mid_sep, 2);
    lv_obj_set_style_line_color(msep, GS_DIM_CYAN, 0);
    lv_obj_set_style_line_width(msep, 1, 0);
    lv_obj_set_pos(msep, 130, 128);

    // ??? Layer 8: Bottom panel - Data stream (?鞈?瘚? ???
    static lv_point_t sep2_pts[] = {{0, 0}, {296, 0}};
    lv_obj_t* sep2 = lv_line_create(scr);
    lv_line_set_points(sep2, sep2_pts, 2);
    lv_obj_set_style_line_color(sep2, GS_CYAN, 0);
    lv_obj_set_style_line_width(sep2, 1, 0);
    lv_obj_set_style_line_opa(sep2, LV_OPA_60, 0);
    lv_obj_set_pos(sep2, 12, 186);

    // Window title tag
    lv_obj_t* lbl_win_tag = lv_label_create(scr);
    lv_label_set_text(lbl_win_tag, "[NET.INTERCEPT//STREAM]");
    lv_obj_set_style_text_color(lbl_win_tag, GS_NEON_PINK, 0);
    lv_obj_set_style_text_font(lbl_win_tag, &orbitron_10, 0);
    lv_obj_set_pos(lbl_win_tag, 14, 189);

    // 4 stream lines (denser than before)
    lbl_stream1 = lv_label_create(scr);
    lv_label_set_text(lbl_stream1, "GHOST:SYNC 99.7% HASH:OK");
    lv_obj_set_style_text_color(lbl_stream1, GS_CYAN, 0);
    lv_obj_set_style_text_font(lbl_stream1, &orbitron_10, 0);
    lv_obj_set_pos(lbl_stream1, 14, 200);
    lv_obj_set_width(lbl_stream1, 292);
    lv_label_set_long_mode(lbl_stream1, LV_LABEL_LONG_CLIP);

    lbl_stream2 = lv_label_create(scr);
    lv_label_set_text(lbl_stream2, "4A 7F B2 0C E8 91 3D A5 FF 02 6B C9 11");
    lv_obj_set_style_text_color(lbl_stream2, GS_DIM_CYAN, 0);
    lv_obj_set_style_text_font(lbl_stream2, &orbitron_10, 0);
    lv_obj_set_pos(lbl_stream2, 14, 212);
    lv_obj_set_width(lbl_stream2, 292);
    lv_label_set_long_mode(lbl_stream2, LV_LABEL_LONG_CLIP);

    lbl_stream3 = lv_label_create(scr);
    lv_label_set_text(lbl_stream3, "BARRIER.MAZE >> ACTIVE L7");
    lv_obj_set_style_text_color(lbl_stream3, GS_GREEN, 0);
    lv_obj_set_style_text_font(lbl_stream3, &orbitron_10, 0);
    lv_obj_set_pos(lbl_stream3, 14, 224);
    lv_obj_set_width(lbl_stream3, 145);
    lv_label_set_long_mode(lbl_stream3, LV_LABEL_LONG_CLIP);

    lbl_stream4 = lv_label_create(scr);
    lv_label_set_text(lbl_stream4, "0xFF2A:PROC.OK");
    lv_obj_set_style_text_color(lbl_stream4, GS_DIM_GREEN, 0);
    lv_obj_set_style_text_font(lbl_stream4, &orbitron_10, 0);
    lv_obj_set_pos(lbl_stream4, 165, 224);
    lv_obj_set_width(lbl_stream4, 142);
    lv_label_set_long_mode(lbl_stream4, LV_LABEL_LONG_CLIP);

    // ??? Layer 9: Scan line (??蝺? ???
    static lv_point_t scan_pts[] = {{0, 0}, {296, 0}};
    scan_line = lv_line_create(scr);
    lv_line_set_points(scan_line, scan_pts, 2);
    lv_obj_set_style_line_color(scan_line, GS_GREEN, 0);
    lv_obj_set_style_line_width(scan_line, 1, 0);
    lv_obj_set_style_line_opa(scan_line, LV_OPA_40, 0);
    lv_obj_set_pos(scan_line, 12, 200);

    // ??? Layer 10: Glitch overlay (????) ???
    lbl_glitch = lv_label_create(scr);
    lv_label_set_text(lbl_glitch, "");
    lv_obj_set_style_text_color(lbl_glitch, GS_GLITCH, 0);
    lv_obj_set_style_text_font(lbl_glitch, &orbitron_10, 0);
    lv_obj_set_style_text_opa(lbl_glitch, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(lbl_glitch, 50, 100);
    lv_obj_set_width(lbl_glitch, 220);
    lv_label_set_long_mode(lbl_glitch, LV_LABEL_LONG_CLIP);
}

// ????????????????????????????????????????????????????????
// Update weather UI
// ????????????????????????????????????????????????????????
void update_weather_ui() {
    if (weather.valid) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f C", weather.temperature);
        lv_label_set_text(lbl_temp, buf);

        // Update weather icon
        lv_label_set_text(lbl_weather_icon, weather_code_to_icon(weather.weather_code));

        snprintf(buf, sizeof(buf), "WIND:%.1f km/h CODE:%d",
            weather.wind_speed, weather.weather_code);
        lv_label_set_text(lbl_wind, buf);

        lv_label_set_text(lbl_status, "NEURAL:OK");
        lv_obj_set_style_text_color(lbl_status, GS_GREEN, 0);
    } else {
        lv_label_set_text(lbl_status, "NEURAL:--");
        lv_obj_set_style_text_color(lbl_status, GS_RED, 0);
    }
}

// ????????????????????????????????????????????????????????
// Update radar sweep + scan line
// ????????????????????????????????????????????????????????
void update_radar() {
    // Large ring: clockwise, moderate speed
    radar_angle_lg = (radar_angle_lg + 5) % 360;
    lv_arc_set_value(radar_arc, radar_angle_lg);

    // Medium ring: counter-clockwise, faster
    radar_angle_md = (radar_angle_md + 353) % 360;  // 360-7 = counter-clockwise 7 deg/tick
    lv_arc_set_value(radar_mid, radar_angle_md);

    // Small ring: clockwise, fastest
    radar_angle_sm = (radar_angle_sm + 9) % 360;
    lv_arc_set_value(radar_small, radar_angle_sm);

    // Scan line sweeps vertically through bottom panel
    scan_y = (scan_y + 2) % 50;
    lv_obj_set_pos(scan_line, 12, 188 + scan_y);
}

// ????????????????????????????????????????????????????????
// Update waveform (璅⊥?行郭/靽∟???)
// ????????????????????????????????????????????????????????
void update_waveform() {
    wave_phase += 2;
    for (int i = 0; i < 21; i++) {
        // Sine-based waveform with noise
        int base = (int)(8.0f * sinf((wave_phase + i * 20) * 0.05f));
        wave_pts[i].y = 15 + base + random(4);
    }
    lv_line_set_points(waveform_line, wave_pts, 21);
}

// ????????????????????????????????????????????????????????
// Update data stream (?鞈?瘚?
// ????????????????????????????????????????????????????????
void update_data_stream() {
    // Shift all lines up
    lv_label_set_text(lbl_stream1, lv_label_get_text(lbl_stream2));
    lv_obj_set_style_text_color(lbl_stream1,
        lv_obj_get_style_text_color(lbl_stream2, 0), 0);

    lv_label_set_text(lbl_stream2, lv_label_get_text(lbl_stream3));
    lv_obj_set_style_text_color(lbl_stream2,
        lv_obj_get_style_text_color(lbl_stream3, 0), 0);

    // Stream3 gets new content
    static int tick = 0;
    tick++;
    if (tick % 3 == 0) {
        lv_label_set_text(lbl_stream3, get_stream_msg());
        lv_color_t colors[] = {GS_CYAN, GS_ELEC_BLUE, GS_ORANGE, GS_PURPLE};
        lv_obj_set_style_text_color(lbl_stream3, colors[random(4)], 0);
    } else {
        char hex[42];
        generate_hex_stream(hex, 41);
        lv_label_set_text(lbl_stream3, hex);
        lv_obj_set_style_text_color(lbl_stream3, GS_DIM_CYAN, 0);
    }

    // Stream4 - secondary code column
    char addr[24];
    snprintf(addr, sizeof(addr), "0x%04X:PROC.%s",
        (unsigned int)random(0xFFFF), random(2) ? "OK" : ">>>");
    lv_label_set_text(lbl_stream4, addr);
}

// ????????????????????????????????????????????????????????
// Glitch effect (???? - 閮?撟脫)
// ????????????????????????????????????????????????????????
void update_glitch() {
    glitch_counter++;

    // Randomly trigger glitch (approx every 3-8 seconds)
    if (!glitch_active && random(100) < 5) {
        glitch_active = true;
        glitch_counter = 0;
        // Show glitch text
        char glitch_buf[25];
        generate_glitch_text(glitch_buf, 24);
        lv_label_set_text(lbl_glitch, glitch_buf);
        lv_obj_set_style_text_opa(lbl_glitch, LV_OPA_80, 0);
        // Random position
        lv_obj_set_pos(lbl_glitch, random(40, 200), random(30, 180));
        // Random glitch color
        lv_color_t gc[] = {GS_GLITCH, GS_NEON_PINK, GS_RED, GS_PURPLE};
        lv_obj_set_style_text_color(lbl_glitch, gc[random(4)], 0);
    }

    // Deactivate glitch after short duration (2-4 frames = 100-200ms)
    if (glitch_active && glitch_counter > 2 + random(3)) {
        glitch_active = false;
        lv_obj_set_style_text_opa(lbl_glitch, LV_OPA_TRANSP, 0);
    }

    // Occasionally corrupt a prosthetic reading for 1 frame
    if (random(100) < 3) {
        char g[16];
        generate_glitch_text(g, 12);
        lv_label_set_text(lbl_prosthetic, g);
        lv_obj_set_style_text_color(lbl_prosthetic, GS_GLITCH, 0);
    } else {
        // Restore normal
        char buf[20];
        float pv = 97.5f + (random(20) * 0.1f);
        snprintf(buf, sizeof(buf), "PROSTH:%.1f%%", pv);
        lv_label_set_text(lbl_prosthetic, buf);
        lv_obj_set_style_text_color(lbl_prosthetic, GS_DIM_CYAN, 0);
    }
}

// ????????????????????????????????????????????????????????
// Update biometric panel (蝢拚????
// ????????????????????????????????????????????????????????
void update_biometrics() {
    // Barrier level fluctuates
    char buf[20];
    int barrier_lvl = 6 + random(3);
    snprintf(buf, sizeof(buf), "BARRIER:L%d", barrier_lvl);
    lv_label_set_text(lbl_barrier, buf);

    // Sync rate
    float sync = 99.0f + (random(10) * 0.1f);
    snprintf(buf, sizeof(buf), "SYNC:%.1f%%", sync);
    lv_label_set_text(lbl_sync_rate, buf);

    // Uptime
    unsigned long up = (millis() - boot_time) / 1000;
    snprintf(buf, sizeof(buf), "UP:%lus", up);
    lv_label_set_text(lbl_uptime, buf);
}

// ????????????????????????????????????????????????????????
// Update clock
// ????????????????????????????????????????????????????????
void update_clock() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d.%02d.%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(lbl_date, buf);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        lv_label_set_text(lbl_time, buf);
    }
}

// ????????????????????????????????????????????????????????
// WiFi
// ????????????????????????????????????????????????????????
void load_config() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }
    fs::File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("config.json not found");
        return;
    }
    String content = configFile.readString();
    configFile.close();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (!err) {
        strlcpy(wifi_ssid, doc["wifi_ssid"] | "", sizeof(wifi_ssid));
        strlcpy(wifi_pass, doc["wifi_pass"] | "", sizeof(wifi_pass));
        strlcpy(timezone_str, doc["timezone"] | "Asia/Taipei", sizeof(timezone_str));
        gmt_offset = doc["gmt_offset"] | 8;
        Serial.printf("Config loaded: SSID=%s TZ=%s GMT+%d\n", wifi_ssid, timezone_str, gmt_offset);
    }
}

void connect_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    wifi_connected = (WiFi.status() == WL_CONNECTED);
}

// ????????????????????????????????????????????????????????
// SETUP
// ????????????????????????????????????????????????????????
void setup() {
    Serial.begin(115200);
    boot_time = millis();
    randomSeed(analogRead(34));  // seed from floating ADC pin for true randomness

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_W * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    create_ui();

    lv_label_set_text(lbl_status, "NEURAL:..");
    lv_obj_set_style_text_color(lbl_status, GS_ORANGE, 0);
    lv_timer_handler();

    load_config();
    connect_wifi();

    if (wifi_connected) {
        configTime(gmt_offset * 3600, 0, "pool.ntp.org", "time.nist.gov");
        fetch_weather();
        update_weather_ui();
        lv_label_set_text(lbl_cyber_brain, "E-BRAIN:LINKED");
    } else {
        lv_label_set_text(lbl_status, "NEURAL:XX");
        lv_obj_set_style_text_color(lbl_status, GS_RED, 0);
        lv_label_set_text(lbl_cyber_brain, "E-BRAIN:OFFLINE");
        lv_obj_set_style_text_color(lbl_cyber_brain, GS_RED, 0);
    }
    last_weather_update = millis();
}

// ????????????????????????????????????????????????????????
// LOOP
// ????????????????????????????????????????????????????????
void loop() {
    lv_timer_handler();

    static unsigned long last_radar = 0;
    static unsigned long last_stream = 0;
    static unsigned long last_clock = 0;
    static unsigned long last_wave = 0;
    static unsigned long last_glitch = 0;
    static unsigned long last_bio = 0;
    unsigned long now = millis();

    // Radar sweep every 50ms
    if (now - last_radar > 50) {
        last_radar = now;
        update_radar();
    }

    // Waveform every 80ms
    if (now - last_wave > 80) {
        last_wave = now;
        update_waveform();
    }

    // Glitch effect every 60ms (fast flicker)
    if (now - last_glitch > 60) {
        last_glitch = now;
        update_glitch();
    }

    // Data stream every 350ms
    if (now - last_stream > 350) {
        last_stream = now;
        update_data_stream();
    }

    // Biometrics every 2s
    if (now - last_bio > 2000) {
        last_bio = now;
        update_biometrics();
    }

    // Clock every 1s
    if (now - last_clock > 1000) {
        last_clock = now;
        update_clock();
    }

    // Weather every 10 min
    if (now - last_weather_update > WEATHER_INTERVAL) {
        last_weather_update = now;
        fetch_weather();
        update_weather_ui();
    }

    // WiFi reconnect
    if (WiFi.status() != WL_CONNECTED && wifi_connected) {
        wifi_connected = false;
        lv_label_set_text(lbl_status, "NEURAL:..");
        lv_obj_set_style_text_color(lbl_status, GS_RED, 0);
        connect_wifi();
        if (wifi_connected) {
            lv_label_set_text(lbl_status, "NEURAL:OK");
            lv_obj_set_style_text_color(lbl_status, GS_GREEN, 0);
        }
    }

    delay(5);
}
