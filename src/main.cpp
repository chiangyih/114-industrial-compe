// ==================== 標頭檔引入 ====================
#include "Engnin_comp_2025.h"  // 引入自訂標頭檔（包含 NeoPixel、TFT 相關函式庫）
#include <string.h>            // 引入字串處理函式庫

// ==================== 腳位定義 ====================
// NeoPixel LED 燈條相關腳位
#define LED_PIN 5        // NeoPixel 資料腳（連接 WS2812B 資料輸入）
#define RLED_PIN 13      // 板載 LED 閃爍燈（Arduino Uno 內建 LED）
#define LED_COUNT 8      // NeoPixel LED 燈條的 LED 顆數

// TFT LCD 螢幕相關腳位（ST7735 控制器）
#define TFT_CS 10        // TFT LCD 的 CS（晶片選擇）腳位
#define TFT_DC 8         // TFT DC（資料/指令選擇，又稱 A0 或 RS）腳位
#define TFT_MOSI 18      // TFT SDA（SPI 資料輸出）腳位
#define TFT_SCLK 19      // TFT SCK（SPI 時鐘）腳位
#define TFT_RST 9        // TFT Reset（硬體重置，低電位觸發）腳位
#define TFT_BL 6         // TFT 背光控制（PWM 調光）腳位

// ==================== 全域變數 ====================
// 螢幕上 8 個圓形 LED 圖示的 X 座標陣列（用於繪製 LED 狀態指示）
int16_t Circle_X[] = { 10, 30, 50, 70, 90, 110, 130, 150 };

// NeoPixel 燈條的顏色陣列（目前未使用，保留供未來擴充）
uint32_t stripcolor[8];

// 1 秒計時旗標（由 Timer1 中斷切換，用於控制 LED 閃爍）
bool cnt_1s;

// ==================== 硬體物件初始化 ====================
// 建立 NeoPixel 物件（8 顆 LED，資料腳 PIN5，GRB 色彩順序，800KHz 頻率）
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 建立 TFT LCD 物件（使用軟體 SPI，指定 CS、DC、MOSI、SCLK、RST 腳位）
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ==================== Timer1 中斷服務程式 ====================
// Timer1 溢位中斷服務（ISR），每 0.5 秒觸發一次
ISR(TIMER1_OVF_vect) {
  TCNT1 = timer1_counter;  // 重新載入計時器預設值（維持固定週期）
  cnt_1s ^= 1;             // 切換 1 秒旗標（XOR 運算：0→1, 1→0）
  digitalWrite(13, cnt_1s); // 控制板載 LED（PIN13）閃爍（每秒切換一次）
}
// ==================== TFT 文字顯示函式 ====================
// 功能：在 TFT 螢幕上顯示文字，支援多種字體大小及反色顯示
// 參數：x=X座標, y=Y座標, pt=字體大小(9/12/18), color=文字顏色, wd=顯示字串, inv=是否反色
void tft_w(uint16_t x, uint16_t y, uint8_t pt, uint16_t color, String wd, boolean inv) {
  uint16_t bg = 0;  // 背景顏色（預設為黑色）
  
  // 如果啟用反色模式（黑底白字 ↔ 白底黑字）
  if (inv) {
    bg = color;     // 背景色改為原文字顏色
    color = 0;      // 文字顏色改為黑色
  }
  
  // 清除文字顯示區域的背景（整行寬度 160 像素，高度依字體大小調整）
  tft.fillRect(0, y - pt - 2, 160, pt + 5, bg);
  
  tft.setCursor(x, y);         // 設定文字游標位置
  tft.setTextColor(color);     // 設定文字顏色
  
  // 根據字體大小參數選擇對應的字型
  switch (pt) {
    case 9:  // 9pt 字體
      tft.setFont(&FreeMono9pt7b);
      break;
    case 12: // 12pt 字體
      tft.setFont(&FreeMono12pt7b);
      break;
    case 18: // 18pt 字體
      tft.setFont(&FreeMono18pt7b);
      break;
      // case 24:  // 24pt 字體（已註解，未啟用）
      //   tft.setFont(&FreeMono24pt7b);
      //   break;
  }
  
  tft.print(wd);  // 在螢幕上輸出字串
}

// ==================== LED 圖示繪製函式 ====================
// 功能：在 TFT 螢幕上繪製 8 個空心圓形（代表 LED 燈的外框）
// 參數：Draw_Y=圓形的Y座標, TFT_color=圓形邊框顏色
void LED_on(int16_t Draw_Y, int16_t TFT_color) {
  for (int i = 0; i < 8; i++) {  // 迴圈繪製 8 個圓形
    // 在預定義的 X 座標上繪製空心圓（半徑 9 像素）
    tft.drawCircle(Circle_X[i], Draw_Y, 9, TFT_color);
  }
}

// ==================== 主選單項目陣列 ====================
// 定義主選單的 4 個功能選項名稱
String page0_md[] = { "Time", "BLE", "EEPROM", "Light" };

// ==================== LED 狀態顯示函式 ====================
// 功能：根據位元組值顯示 LED 的亮滅狀態（點亮或熄滅對應的圓形）
// 參數：LED=位元組狀態值(bit0~7代表8顆LED), Draw_Y=圓形Y座標, TFT_color=點亮的顏色
void show_LED(int8_t LED, int16_t Draw_Y, int16_t TFT_color) {
  for (int i = 0; i < 8; i++) {  // 迴圈處理 8 顆 LED
    if (bitRead(LED, i)) {       // 如果該位元為 1（LED 應該亮）
      // 繪製填滿的圓形（半徑 9 像素，指定顏色）
      tft.fillCircle(Circle_X[i], Draw_Y, 9, TFT_color);
    } else {                     // 如果該位元為 0（LED 應該暗）
      // 繪製黑色填滿圓形（半徑 8 像素，稍小以保留邊框）
      tft.fillCircle(Circle_X[i], Draw_Y, 8, 0);
    }
  }
}

// ==================== 初始化設定函式（開機時執行一次）====================
void setup() {
  // ========== 藍牙序列埠初始化（HC-05 通訊）==========
  Serial.begin(9600);         // 初始化序列埠，鮑率 9600 bps（與 VB 程式一致）
  Serial.setTimeout(1000);    // 設定接收逾時 1000ms（與 VB 的 ReadTimeout 一致）
  
  // ========== 計時器初始化 ==========
  timer_ini(34286);  // 初始化 Timer1，預載值 34286（產生 2Hz 中斷，配合 cnt_1s 形成 1 秒週期）
  
  // ========== GPIO 腳位設定 ==========
  pinMode(RLED_PIN, OUTPUT);  // 設定 PIN13 為輸出（板載 LED 閃爍控制）
  
  // ========== NeoPixel 初始化 ==========
  strip.begin();              // 初始化 NeoPixel 燈條物件（必要步驟）
  strip.show();               // 立即關閉所有 LED（避免開機時亮起）
  strip.setBrightness(100);   // 設定亮度為 100/255（約 40% 亮度，避免過亮）
  
  // ========== TFT LCD 初始化 ==========
  tft.initR(INITR_BLACKTAB);  // 初始化 ST7735S 晶片（黑色 TAB 版本）
  tft.fillScreen(ST77XX_BLACK); // 清除螢幕，填滿黑色
  tft.setRotation(3);         // 設定螢幕旋轉方向（3=左轉90度，橫向顯示）
                              // 旋轉選項：0=原始, 1=右轉90度, 2=180度, 3=左轉90度
  
  // ========== 螢幕背光控制 ==========
  pinMode(TFT_BL, OUTPUT);      // 設定背光腳位為輸出
  analogWrite(TFT_BL, 168);     // 使用 PWM 調整背光亮度（168/255 ≈ 66% 亮度）
  
  // ========== 顯示開機畫面 ==========
  tft_w(25, 50, 18, ST77XX_RED, "CYIVS", 0);   // 顯示學校名稱（紅色，18pt）
  tft_w(30, 100, 18, ST77XX_BLUE, "C217", 0);  // 顯示班級代碼（藍色，18pt）
  delay(2000);  // 延遲 2 秒（讓使用者看到開機畫面）
  
  // ========== 顯示主選單 ==========
  tft.fillScreen(ST77XX_BLACK);  // 清除螢幕
  tft_w(30, 25, 18, ST77XX_RED, "MENU", 0);          // 顯示 "MENU" 標題（紅色，18pt）
  tft_w(0, 70, 12, ST77XX_WHITE, page0_md[0], 0);    // 顯示第一個選單項目 "Time"（白色，12pt）
  
  // ========== 藍牙就緒通知 ==========
  delay(500);  // 等待藍牙模組穩定
  Serial.println("READY");              // 通知 VB 程式系統已就緒
  Serial.println("VERSION:1.0");        // 發送版本資訊
  Serial.println("DEVICE:CYIVS_C217");  // 發送裝置識別碼
}

// ==================== 主迴圈函式（持續執行）====================
void loop() {
  // ========== 靜態變數宣告（保持變數值在函式呼叫之間不被重置）==========
  static uint32_t kt = millis();  // 上次按鍵掃描的時間戳記（毫秒）
  static int8_t page0 = 0;        // 目前選單位置（0=Time, 1=BLE, 2=EEPROM, 3=Light）
  static boolean kf0;             // 按鍵旗標（防止重複觸發）

  // ========== 全域藍牙指令監聽 ==========
  // 隨時監聽 VB 程式的藍牙指令（不影響按鍵操作）
  processVBCommand();

  // ========== 按鍵掃描（每 20ms 檢查一次）==========
  if ((millis() - kt) > 20)  // 如果距離上次掃描超過 20ms（防彈跳處理）
  {
    kt = millis();  // 更新時間戳記
    
    // --- 按鍵放開檢測 ---
    if (kf0) {  // 如果旗標已設定（表示之前有按鍵被按下）
      // 檢查所有按鍵是否都已放開（keyC 回傳非 0 表示放開）
      if (keyC(0) && keyC(1) && keyC(2) && keyC(3)) 
        kf0 = 0;  // 重置旗標，允許下次按鍵觸發
    } 
    // --- Key0 處理：下一個選單項目 ---
    else if (!keyC(0)) {  // 如果 Key0 被按下（回傳 0）
      if (++page0 > 3) page0 = 0;  // 選單索引 +1，超過 3 則循環回 0
      kf0 = 1;  // 設定旗標防止連續觸發
      tft_w(0, 70, 12, ST77XX_WHITE, page0_md[page0], 0);  // 更新螢幕顯示新選項
      
      // 通知 VB 程式選單變更
      Serial.print("MENU_CHANGE:");
      Serial.println(page0_md[page0]);
    } 
    // --- Key1 處理：上一個選單項目 ---
    else if (!keyC(1)) {  // 如果 Key1 被按下
      if (--page0 < 0) page0 = 3;  // 選單索引 -1，小於 0 則循環到 3
      kf0 = 1;  // 設定旗標防止連續觸發
      tft_w(0, 70, 12, ST77XX_WHITE, page0_md[page0], 0);  // 更新螢幕顯示新選項
      
      // 通知 VB 程式選單變更
      Serial.print("MENU_CHANGE:");
      Serial.println(page0_md[page0]);
    } 
    // --- Key2 處理：確認進入選定的功能 ---
    else if (!keyC(2)) {  // 如果 Key2 被按下
      kf0 = 1;  // 設定旗標
      tft.fillScreen(ST77XX_BLACK);  // 清除螢幕
      tft_w(0, 25, 12, ST77XX_WHITE, page0_md[page0], 0);  // 顯示功能名稱標題
      
      // 根據選單位置執行對應的功能
      switch (page0) {
        case 0:  // Time 功能
          Time();
          break;
        case 1:  // BLE 功能
          BLE();
          break;
        case 2:  // EEPROM 功能
          EEPROM();
          break;
        case 3:  // Light 功能
          Light();
          break;
      }
      
      // 功能執行完畢後返回主選單
      tft.fillScreen(ST77XX_BLACK);  // 清除螢幕
      tft_w(30, 25, 18, ST77XX_RED, "MENU", 0);          // 顯示 MENU 標題
      tft_w(0, 70, 12, ST77XX_WHITE, page0_md[page0], 0); // 顯示目前選項
    }
  }
}

// ==================== VB 藍牙指令處理函式 ====================
// 功能：處理 VB 程式透過 HC-05 傳來的指令
// VB 協定：# 開頭為寫入指令，@ 開頭為讀取請求
void processVBCommand() {
  if (Serial.available() > 0) {
    char controlChar = Serial.read();  // 讀取控制字元（# 或 @）
    
    // ========== 寫入模式：# 開頭 ==========
    if (controlChar == '#') {
      String command = "";
      unsigned long startTime = millis();
      
      // 讀取完整指令（最多等待 100ms）
      while (millis() - startTime < 100) {
        if (Serial.available() > 0) {
          char c = Serial.read();
          if (c == '\n' || c == '\r') break;  // 遇到換行結束
          command += c;
        }
        delay(2);  // 短暫延遲等待下一個字元
      }
      
      // 在 TFT 螢幕顯示接收到的指令（除錯用）
      String displayCmd = command;
      if (displayCmd.length() > 15) {
        displayCmd = displayCmd.substring(0, 15) + "...";  // 限制顯示長度
      }
      tft_w(0, 110, 9, ST77XX_CYAN, "VB:" + displayCmd + "   ", 0);
      
      // ===== 解析並執行指令 =====
      
      // LED 亮度控制：LED數字 (例如：LED100)
      if (command.startsWith("LED") && command.length() > 3) {
        String brightnessStr = command.substring(3);
        int brightness = brightnessStr.toInt();
        
        if (brightness >= 0 && brightness <= 255) {
          strip.setBrightness(brightness);
          strip.show();
          Serial.println("OK:LED_SET");
        } else {
          Serial.println("ERROR:BRIGHTNESS_RANGE:0-255");
        }
      }
      
      // 顏色設定：COLOR:RRGGBB (例如：COLOR:FF0000)
      else if (command.startsWith("COLOR:")) {
        String colorStr = command.substring(6);
        if (colorStr.length() == 6) {
          uint32_t color = strtol(colorStr.c_str(), NULL, 16);
          strip.fill(color, 0, LED_COUNT);
          strip.show();
          Serial.print("OK:COLOR_SET:");
          Serial.println(colorStr);
        } else {
          Serial.println("ERROR:FORMAT:COLOR:RRGGBB");
        }
      }
      
      // 時間設定：TIME:HH:MM:SS (例如：TIME:14:30:00)
      else if (command.startsWith("TIME:")) {
        String timeStr = command.substring(5);
        if (timeStr.length() == 8) {
          tft_w(0, 100, 9, ST77XX_YELLOW, "T:" + timeStr, 0);
          Serial.print("OK:TIME_SET:");
          Serial.println(timeStr);
        } else {
          Serial.println("ERROR:FORMAT:TIME:HH:MM:SS");
        }
      }
      
      // 模式切換：MODE:0-3 (例如：MODE:2)
      else if (command.startsWith("MODE:")) {
        int mode = command.substring(5).toInt();
        if (mode >= 0 && mode <= 3) {
          Serial.print("OK:MODE:");
          Serial.println(page0_md[mode]);
        } else {
          Serial.println("ERROR:MODE_RANGE:0-3");
        }
      }
      
      // 未知指令
      else {
        Serial.print("ERROR:UNKNOWN_CMD:");
        Serial.println(command);
      }
    }
    
    // ========== 讀取模式：@ 開頭 ==========
    else if (controlChar == '@') {
      // VB 程式請求讀取系統狀態
      Serial.print("STATUS:RUNNING");
      Serial.print(",BRIGHTNESS:");
      Serial.print(strip.getBrightness());
      Serial.print(",LED_COUNT:");
      Serial.println(LED_COUNT);
    }
  }
}

// ==================== Time 功能（時間相關功能）====================
// 功能：預留給時間設定或顯示功能（目前未實作）
// 操作：按下 Key3 即可返回主選單
void Time() {
  while (keyC(3)) delay(10);  // 等待 Key3 被放開（避免重複觸發）
}

// ==================== LED 顏色陣列 ====================
// 定義各模式使用的 LED 顏色（24-bit RGB 格式：0xRRGGBB）
// 索引 [0]=紅色(mod1), [1]=綠色(mod2), [2]=藍色(mod3), [3]=白色, [4][5]=紅色
uint32_t strip_col[] = { 0xff0000, 0x00ff00, 0x0000ff, 0x1f1f1f ,0xff0000,0xff0000};

// ==================== NeoPixel 模式顯示函式 ====================
// 功能：顯示指定索引的 LED 燈效，並在螢幕上繪製對應的圖示和文字
// 參數：n=LED索引(0-7), wd=顯示的文字說明
void stripmd(uint8_t n,String wd){
  uint8_t led = 0;        // LED 狀態位元組（初始化為 0）
  bitWrite(led,n,1);      // 將第 n 個位元設為 1（點亮第 n 顆 LED）
  
  // ========== 螢幕顯示 ==========
  tft.fillScreen(ST77XX_BLACK);  // 清除螢幕
  LED_on(60, convert24to16(strip_col[n]));       // 繪製 8 個空心圓形外框（Y=60）
  show_LED(led,60, convert24to16(strip_col[n])); // 填滿第 n 個圓形（顯示點亮狀態）
  
  tft.setCursor(0, 25);              // 設定文字游標位置
  tft.setFont(&FreeMono9pt7b);       // 使用 9pt 字體
  tft.setTextColor(convert24to16(strip_col[n]));  // 設定文字顏色（與 LED 同色）
  tft.print(wd);                     // 顯示文字說明
  
  // ========== 實體 LED 控制 ==========
  strip.setBrightness(50);           // 設定 NeoPixel 亮度為 50/255（約 20%）
  strip.setPixelColor(n, strip_col[n]);  // 設定第 n 顆 LED 的顏色
  strip.show();                      // 更新 LED 燈條顯示
  
  // ========== 等待返回 ==========
  while (keyC(3)) delay(10);  // 等待 Key3 被放開
  
  // ========== 關閉 LED ==========
  strip.clear();  // 清除所有 LED（關閉燈光）
  strip.show();   // 更新顯示
}

// ==================== BLE 子選單項目陣列 ====================
// 定義 BLE 功能的兩個子選項
String page_change[] = { "Change Time", "Change EEPROM" };

// ==================== BLE 功能（HC-05 藍牙通訊）====================
// 功能：顯示藍牙連接選單，可選擇修改時間或 EEPROM 資料
// 同時處理來自 VB 程式的藍牙指令
void BLE() {
  tft_w(0, 50, 9, ST77XX_RED, "BT Connect", 0);  // 顯示 "BT Connect" 提示（紅色，9pt）
  
  // ========== 靜態變數初始化 ==========
  static uint32_t kt = millis();  // 按鍵掃描時間戳記
  int8_t page1 = 0;               // 子選單索引（0=Change Time, 1=Change EEPROM）
  static boolean kf0;             // 按鍵旗標
  
  tft_w(0, 70, 9, ST77XX_GREEN, page_change[page1], 0);  // 顯示第一個子選單項目
  
  // 通知 VB 程式進入藍牙模式
  Serial.println("ENTER:BLE_MODE");
  
  // ========== 無窮迴圈（直到按 Key3 退出）==========
  while (1) {
    // ========== 處理 VB 藍牙指令 ==========
    processVBCommand();
    
    // --- 按鍵掃描（每 20ms）---
    if ((millis() - kt) > 20)  
    {
      kt = millis();  // 更新時間戳記
      
      // --- 按鍵放開檢測 ---
      if (kf0) {
        if (keyC(0) && keyC(1) && keyC(2) && keyC(3)) kf0 = 0;  // 所有鍵放開則重置旗標
      } 
      // --- Key0：下一個子選項 ---
      else if (!keyC(0)) {
        if (++page1 > 1) page1 = 0;  // 索引 +1，超過 1 則循環回 0
        kf0 = 1;
        tft_w(0, 70, 9, ST77XX_GREEN, page_change[page1], 0);  // 更新顯示
        
        // 回報按鍵事件給 VB
        Serial.print("KEY:0,BLE_MENU:");
        Serial.println(page1);
      } 
      // --- Key1：上一個子選項 ---
      else if (!keyC(1)) {
        if (--page1 < 0) page1 = 1;  // 索引 -1，小於 0 則循環到 1
        kf0 = 1;
        tft_w(0, 70, 9, ST77XX_GREEN, page_change[page1], 0);  // 更新顯示
        
        // 回報按鍵事件給 VB
        Serial.print("KEY:1,BLE_MENU:");
        Serial.println(page1);
      } 
      // --- Key2：確認執行 ---
      else if (!keyC(2)) {
        kf0 = 1;
        
        // 回報按鍵事件給 VB
        Serial.print("KEY:2,ACTION:");
        Serial.println(page_change[page1]);
        
        stripmd(page1,page_change[page1]);  // 執行對應的 LED 顯示效果
        
        // 返回 BLE 選單畫面
        tft.fillScreen(ST77XX_BLACK);
        tft_w(15, 25, 12, ST77XX_WHITE, "BLE", 0);           // 顯示 BLE 標題
        tft_w(0, 50, 9, ST77XX_RED, "BT Connect", 0);        // 顯示連接狀態
        tft_w(0, 70, 9, ST77XX_GREEN, page_change[page1], 0); // 顯示目前選項
      } 
      // --- Key3：返回主選單 ---
      else if (!keyC(3)) {
        // 通知 VB 程式離開藍牙模式
        Serial.println("EXIT:BLE_MODE");
        break;  // 跳出無窮迴圈
      }
    }
  }
  delay(100);  // 延遲防止按鍵重複觸發
}

// ==================== EEPROM 功能（倒數計時器 + LED 閃爍）====================
// 功能：10 秒倒數計時，倒數結束後 LED 閃爍紫色 3 次
void EEPROM() {
  // ========== 變數初始化 ==========
  static uint32_t kt = millis();  // 按鍵掃描時間戳記
  uint8_t count = 6;              // LED 閃爍次數（6 次 = 3 個亮暗週期）
  uint8_t ss = 10;                // 倒數秒數（從 10 開始）
  boolean tf = 1;                 // 計時器啟動旗標（1=執行, 0=停止）
  static boolean kf0;             // 按鍵旗標
  int8_t timecount = 50;          // 時間計數器（50 x 20ms = 1 秒）
  
  // ========== 初始顯示 ==========
  tft_w(20, 70, 12, ST77XX_WHITE, "00:00:" + String(10), 0);  // 顯示初始時間 00:00:10
  
  // ========== 主迴圈（Key3 放開且計時器執行中才持續）==========
  while (keyC(3) || (tf)) {
    // --- 按鍵掃描（每 20ms）---
    if ((millis() - kt) > 20)  
    {
      kt = millis();  // 更新時間戳記
      
      // --- 按鍵放開檢測 ---
      if (kf0) {
        if (keyC(0) && keyC(1) && keyC(2) && keyC(3)) kf0 = 0;
      } 
      // --- Key2：暫停/繼續計時器 ---
      else if (!keyC(2)) {
        kf0 = 1;
        tf ^= 1;  // 切換計時器狀態（XOR：1↔0）
      }
      
      // ========== 時間計數（每 1 秒觸發）==========
      if (--timecount <= 0) {  // 計數器遞減，達到 0 時（經過 1 秒）
        timecount = 50;        // 重置計數器
        
        // --- 倒數計時更新 ---
        if (tf && ss != 0) {   // 如果計時器啟動且秒數未歸零
          tft.setFont(&FreeMono12pt7b);  // 設定字體
          tft.setCursor(20, 70);         // 設定游標位置
          
          // 先用黑色清除舊數字（覆蓋法）
          tft.setTextColor(ST77XX_WHITE);
          tft.print("00:00:");
          tft.setTextColor(ST77XX_BLACK);
          tft.print(String(ss / 10) + String(ss % 10));  // 清除舊秒數
          
          if (--ss == 0) tf = 0;  // 秒數遞減，歸零時停止計時器
          
          // 顯示新的時間
          tft.setCursor(20, 70);
          tft.setTextColor(ST77XX_WHITE);
          tft.print("00:00:" + String(ss / 10) + String(ss % 10));  // 顯示新秒數
        }
        
        // --- LED 閃爍控制（倒數結束後）---
        if (ss == 0 && count != 0) {  // 如果倒數結束且閃爍次數未完成
          if (!(count % 2))  // 偶數次：點亮
            strip.fill(0xff00ff, 0, 8);  // 填滿紫色（0xff00ff）到所有 8 顆 LED
          else               // 奇數次：熄滅
            strip.clear();   // 清除所有 LED
          strip.show();      // 更新顯示
          count--;           // 閃爍次數遞減
        }
      }
    }
  }
  delay(10);  // 延遲防止重複觸發
}

// ==================== Light 子選單項目陣列 ====================
// 定義 Light 功能的三個燈光模式
String page_strip[] = { "mod 1", "mod 2", "mod 3" };

// 亮度陣列（目前未使用，保留供未來擴充）
uint8_t strip_BL[] = { 1, 2, 3};

// ==================== Light 功能（LED 燈光控制）====================
// 功能：選擇並顯示不同的 LED 燈光模式（紅/綠/藍）
void Light() {
  // ========== 靜態變數初始化 ==========
  static uint32_t kt = millis();  // 按鍵掃描時間戳記
  int8_t page1 = 0;               // 子選單索引（0=mod1, 1=mod2, 2=mod3）
  static boolean kf0;             // 按鍵旗標
  
  tft_w(0, 70, 9, ST77XX_GREEN, page_strip[page1], 0);  // 顯示第一個模式選項
  
  // ========== 無窮迴圈（直到按 Key3 退出）==========
  while (1) {
    // --- 按鍵掃描（每 20ms）---
    if ((millis() - kt) > 20)  
    {
      kt = millis();  // 更新時間戳記
      
      // --- 按鍵放開檢測 ---
      if (kf0) {
        if (keyC(0) && keyC(1) && keyC(2) && keyC(3)) kf0 = 0;  // 所有鍵放開則重置旗標
      } 
      // --- Key0：下一個燈光模式 ---
      else if (!keyC(0)) {
        if (++page1 > 2) page1 = 0;  // 索引 +1，超過 2 則循環回 0
        kf0 = 1;
        tft_w(0, 70, 9, ST77XX_GREEN, page_strip[page1], 0);  // 更新顯示
        
        // 回報按鍵事件給 VB（選用）
        Serial.print("KEY:0,LIGHT_MODE:");
        Serial.println(page1);
      } 
      // --- Key1：上一個燈光模式 ---
      else if (!keyC(1)) {
        if (--page1 < 0) page1 = 2;  // 索引 -1，小於 0 則循環到 2
        kf0 = 1;
        tft_w(0, 70, 9, ST77XX_GREEN, page_strip[page1], 0);  // 更新顯示
        
        // 回報按鍵事件給 VB（選用）
        Serial.print("KEY:1,LIGHT_MODE:");
        Serial.println(page1);
      } 
      // --- Key2：確認顯示 LED 效果 ---
      else if (!keyC(2)) {
        kf0 = 1;
        stripmd(page1,page_strip[page1]);  // 執行對應的 LED 顯示效果
                                            // page1=0→紅色, 1→綠色, 2→藍色

        // 返回 Light 選單畫面
        tft.fillScreen(ST77XX_BLACK);
        tft_w(15, 25, 12, ST77XX_WHITE, "Light", 0);         // 顯示 Light 標題
        tft_w(0, 70, 9, ST77XX_GREEN, page_strip[page1], 0);  // 顯示目前選項
      } 
      // --- Key3：返回主選單 ---
      else if (!keyC(3)) break;  // 跳出無窮迴圈
    }
  }
  while (keyC(3)) delay(10);  // 等待 Key3 放開
}