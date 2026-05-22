# CYD Ghost in the Shell Display

**攻殼機動隊風格氣象/系統監視器** — ESP32-2432S028 (Cheap Yellow Display)

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-green)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

## 概述

在 2.8 吋 320×240 TFT 螢幕上呈現《攻殼機動隊》(Ghost in the Shell) 風格的戰術 HUD 介面，結合即時天氣資料與動態視覺效果。

### 設計風格

- **神經直連 AR 投射** — 多層疊加視窗、角括號框架模擬瞳孔 HUD
- **擬真與線稿交織** — 雷達掃描弧、網格線圖、義體參數面板
- **故障藝術 Glitch Art** — 隨機訊號干擾、文字腐蝕殘影
- **機械排版** — Orbitron 幾何字型、密集座標碼、雙欄資料流

### 功能

- 即時時間顯示（NTP 同步，台北 UTC+8）
- Open-Meteo 天氣 API（溫度/風速/天氣碼）
- 天氣圖示顯示（Weather Icons 字型，依天氣碼自動切換）
- 旋轉雷達動畫（外環 + 反向內環）
- 腦波信號波形（即時正弦波 + 雜訊）
- 滾動資料攔截流（Section-9 風格）
- 笑臉男 (Laughing Man) 半透明底圖
- 義體參數即時模擬（SYNC/BARRIER/PROSTHETIC）
- WiFi 設定外部化（SPIFFS config.json）

## 硬體需求

| 項目 | 規格 |
|------|------|
| 開發板 | ESP32-2432S028 (Cheap Yellow Display) |
| 螢幕 | 2.8" ILI9341 320×240 TFT (SPI) |
| 觸控 | XPT2046 (SPI) |
| 背光 | GPIO 21 |
| Flash | 4MB |

## 軟體依賴

- [PlatformIO](https://platformio.org/)
- LVGL 8.3.11
- TFT_eSPI 2.5.43
- ArduinoJson 7.x
- Orbitron 字型（已含轉換後 C 檔）
- Weather Icons 字型（已含轉換後 C 檔）

## 快速開始

### 1. 複製專案

```bash
git clone https://github.com/YOUR_USERNAME/CYD-Cyberpunk-Display.git
cd CYD-Cyberpunk-Display
```

### 2. 設定 WiFi

複製範例設定檔並填入你的 WiFi 資訊：

```bash
cp data/config.json.example data/config.json
```

編輯 `data/config.json`：

```json
{
    "wifi_ssid": "YOUR_WIFI_SSID",
    "wifi_pass": "YOUR_WIFI_PASSWORD"
}
```

### 3. 上傳檔案系統

```bash
pio run --target uploadfs
```

### 4. 編譯並上傳韌體

```bash
pio run --target upload
```

## 專案結構

```
CYD-Cyberpunk-Display/
├── data/
│   └── config.json          # WiFi 設定（不進版控）
├── fonts/
│   ├── Orbitron.ttf         # 原始字型檔
│   └── weathericons.ttf     # Weather Icons 字型檔
├── src/
│   ├── main.cpp             # 主程式
│   ├── lv_conf.h            # LVGL 設定
│   ├── orbitron_28.c        # Orbitron 28pt 字型（LVGL 格式）
│   ├── orbitron_10.c        # Orbitron 10pt 字型（LVGL 格式）
│   ├── weather_icons_28.c   # Weather Icons 28pt 字型（LVGL 格式）
│   └── laughing_man.c       # 笑臉男底圖（LVGL 圖片格式）
├── tools/
│   └── convert_img.py       # 圖片轉 LVGL C 陣列工具
├── platformio.ini           # PlatformIO 專案設定
└── README.md
```

## 配色方案

| 角色 | 色碼 | 用途 |
|------|------|------|
| 電子藍 | `#0088FF` | 日期、雷達、系統主色 |
| 青綠 | `#00FFEE` | 時間、資料流 |
| 深紫 | `#8844FF` | AR 框架、強調 |
| 霓虹粉紅 | `#FF0088` | 波形、警告標題 |
| 亮橘 | `#FF6600` | 溫度 |
| 警示紅 | `#FF1133` | 錯誤/斷線 |
| 暗黑 | `#020408` | 背景 |

## 自訂字型

本專案使用：
- [Orbitron](https://fonts.google.com/specimen/Orbitron)（Google Fonts, OFL 授權）— 主顯示字型
- [Weather Icons](https://erikflowers.github.io/weather-icons/)（SIL OFL 授權）— 天氣圖示字型

如需更換字型或大小，使用 `lv_font_conv`：

```bash
npm install -g lv_font_conv

# 文字字型（ASCII 範圍）
lv_font_conv --font fonts/YourFont.ttf -r 0x20-0x7F --size 28 --format lvgl --bpp 4 --no-compress -o src/your_font_28.c --lv-include "lvgl.h"

# 圖示字型（PUA 範圍）
lv_font_conv --font fonts/weathericons.ttf --range 0xF002-0xF01E --size 28 --format lvgl --bpp 4 --no-compress -o src/weather_icons_28.c --lv-include "lvgl.h"
```

> **注意：** 必須使用 `--no-compress` 參數，LVGL 8.3 對壓縮字型格式有相容性問題。

### 天氣圖示對照

| 天氣碼 (WMO) | 圖示 | 說明 |
|---|---|---|
| 0 | ☀ U+F00D | 晴天 |
| 1-2 | 🌤 U+F002 | 晴時多雲 |
| 3 | ☁ U+F013 | 陰天 |
| 45, 48 | 🌫 U+F014 | 霧 |
| 51-57 | 🌦 U+F01C | 毛毛雨 |
| 61-67 | 🌧 U+F019 | 雨 |
| 71-77 | ❄ U+F01B | 雪 |
| 80-82 | 🌧 U+F01A | 陣雨 |
| 95-99 | ⛈ U+F01E | 雷暴 |

## 自訂底圖

使用 `tools/convert_img.py` 轉換 PNG 圖片：

```bash
python tools/convert_img.py
```

可修改腳本中的 `target_height` 和 `alpha_level` 調整大小與透明度。

## 授權

MIT License — 詳見 [LICENSE](LICENSE)

Orbitron 字型採用 [SIL Open Font License](https://scripts.sil.org/OFL)。

Weather Icons 字型採用 [SIL Open Font License](https://scripts.sil.org/OFL)。

## 致謝

- 《攻殼機動隊》(Ghost in the Shell) — 士郎正宗 / Production I.G
- 笑臉男 (Laughing Man) — 《攻殼機動隊 S.A.C.》
- [LVGL](https://lvgl.io/) — 嵌入式圖形庫
- [Orbitron](https://fonts.google.com/specimen/Orbitron) — Matt McInerney
- [Weather Icons](https://erikflowers.github.io/weather-icons/) — Erik Flowers
- [Open-Meteo](https://open-meteo.com/) — 免費天氣 API
