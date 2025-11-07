# PC 端 Visual Basic 專案完整架構規格書（plan.md）
## 113 學年度 全國高級中等學校 工業類科技藝競賽
### 電腦修護職類 第二站 — USB / BLE 介面卡控制系統（PC 端 Windows Visual Studio .NET 2022 專案規格）

---

## 目錄（TOC）
1. [專案目標與範圍](#專案目標與範圍)  
2. [系統整體架構](#系統整體架構)  
3. [專案資料夾與模組組成](#專案資料夾與模組組成)  
4. [UI / 介面設計規格](#ui--介面設計規格)  
5. [系統流程設計](#系統流程設計)  
6. [狀態機（State Machine）設計](#狀態機state-machine設計)  
7. [序列埠（COM Port）管理規格](#序列埠com-port管理規格)  
8. [藍牙通訊協定規格（PC → ATmega328P）](#藍牙通訊協定規格pc--atmega328p)  
9. [CPU Loading 監測與顏色對應規格](#cpu-loading-監測與顏色對應規格)  
10. [EEPROM 寫入邏輯與驗證機制](#eeprom-寫入邏輯與驗證機制)  
11. [錯誤處理與例外狀況規格](#錯誤處理與例外狀況規格)  
12. [測試案例（Test Cases）](#測試案例test-cases)  
13. [維護與擴充建議](#維護與擴充建議)

---

## 專案目標與範圍
PC 端程式需透過 USB / BLE 與 ATmega328P 介面卡進行資料交換，達成：
- BLE/USB 介面卡連線管理  
- COM Port 即時偵測與更新  
- CPU Loading 取得與顏色顯示  
- WS2812 色彩控制（送給介面卡）  
- 二進位輸入、轉換與 EEPROM 寫入  
- 正確顯示競賽指定格式之標題列字串  
- 所有行為符合競賽官方規格（113 學年度第二站題目）  

---

## 系統整體架構
```
+---------------------------------------------------------------+
|                         Windows PC                            |
|                      (Visual Basic .NET)                      |
+---------------------------------------------------------------+
             |                                      |
             | USB / Virtual COM Port              | BLE SPP
             v                                      v
+---------------------------------------------------------------+
|                ATmega328P USB / BLE 介面卡                    |
| - OLED 顯示器 control                                         |
| - WS2812 RGB LEDs                                             |
| - EEPROM 存取                                                 |
| - MENU 選單系統                                               |
+---------------------------------------------------------------+
```

PC 端 VB 主要功能：
- 掃描可用 COM Port  
- 建立 SerialPort 連線  
- 解析、封裝 BLE 協定封包  
- 顯示即時 CPU Loading  
- 依照 Loading 傳回 MCU 控制 WS2812 顏色  
- 將 4-bit 二進位數值轉十進位後寫入 EEPROM  
- 透過 UI 管理所有行為  

---

## 專案資料夾與模組組成
```
TCIVS.BLEControl
│
├─ Forms/
│   └─ MainForm.md               # 主 UI 介面規格
│
├─ Services/
│   ├─ SerialPortManager.md      # 序列埠管理（Open/Close/Scan）
│   ├─ ComPortWatcher.md         # COM 清單動態更新/偵測
│   ├─ CpuLoadProvider.md        # CPU Loading 取得
│   ├─ BleProtocol.md            # PC <-> MCU 通訊協定
│   ├─ EepromService.md          # 二進位驗證/十進位轉換/寫入流程
│   ├─ ValidationService.md      # 輸入格式驗證
│   ├─ TitleService.md           # 競賽規格標題列產生器
│   └─ UiDispatcher.md           # UI Thread 控制
│
├─ Domain/
│   ├─ AppConfig.md              # 啟動設定（SeatNo、預設 COM Port）
│   ├─ ColorMapper.md            # CPU% → 顏色映射規格
│   └─ Enums.md                  # 系統狀態列舉定義
│
└─ Infrastructure/
    └─ Logging.md                # 記錄模組（可選）
```

---

## UI / 介面設計規格

### 1. 主視窗標題列（必考）
```
113 學年度 工業類科學生技藝競賽 電腦修護職種 台中高工 第二站 崗位號碼：XX
```
上述字串不可錯誤，需在 `Form_Load` 時載入。

### 2. UI 區塊配置

#### (A) 連線管理區
| 元件 | 說明 |
|------|------|
| ComboBox `cboComPorts` | 顯示可用 COM Port（需支援熱插拔更新） |
| Button `btnOpen` | 按下後 → 開啟 COM Port → 自動顯示 Connected |
| Button `btnClose` | 按下後 → 顯示 Disconnect → 關閉 Write / Start 等功能 |
| Label `lblConnState` | 顯示 Connected / Disconnect |

#### (B) CPU Loading 區
| 元件 | 說明 |
|------|------|
| Label `lblCpuPercent` | 顯示 CPU Loading（如 73%） |
| Panel `pnlCpuColor` | 顯示顏色（綠 / 黃 / 紅） |
| Button `btnStart` | 啟動 CPU Loading 監測（Open 後啟用） |
| Button `btnStop` | 停止 CPU 監測 |

#### (C) EEPROM 區
| 元件 | 說明 |
|------|------|
| TextBox `txtBinInput` | 只能輸入 4 位 0/1（二進位格式） |
| Button `btnWrite` | 將二進位轉十進位 → 發送 EEPROM write 指令 |
| Button `btnExit` | 關閉程式 |

#### (D) 訊息提示區
| 元件 | 說明 |
|------|------|
| Label `lblMessage` | 顯示狀態（例如：Connected、Not BIN Format） |

---

## 系統流程設計

### 1. 程式啟動流程
```
讀取 SeatNo
↓
MainForm 載入
↓
設定標題列字串
↓
啟動 ComPortWatcher（掃描 COM）
↓
更新 UI 狀態為 Disconnect 模式
```

### 2. COM Port Open 流程
```
使用者按下 Open
↓
SerialPort.Open()
↓
成功：
    設定 Connected
    啟用 Write、Start
    顯示 "Connected"
↓
失敗：
    顯示錯誤訊息
    維持 Disconnect 模式
```

### 3. CLOSE 流程
```
按下 Close
↓
SerialPort.Close()
↓
設定 Disconnect
↓
停用 Write / Start / CPU Loading / Now Time
```

### 4. CPU Loading Start 流程
```
按下 Start
↓
啟動 CpuLoadProvider.Timer
↓
每週期取得 CPU%
↓
轉成顏色
↓
更新 UI + 發送 BLE 封包
```

### 5. 二進位輸入與 EEPROM Write 流程
```
按 Write 鈕
↓
資料驗證：
    若不符合 4 位二進位：
        清空輸入欄位
        顯示 "Not BIN Format"
        中斷流程
↓
二進位轉十進位
↓
封裝成 BLE 封包
↓
透過 SerialPort 傳送給 ATmega328P
↓
顯示寫入成功訊息
```

---

## 狀態機（State Machine）設計
```
+------------------+
|   DISCONNECTED   |
+---------+--------+
          |
          | Open 成功
          v
+------------------+
|    CONNECTED     |
+---------+--------+
          |
          | Close()
          v
+------------------+
|   DISCONNECTED   |
+------------------+
```
- CONNECTED：允許 Write、Start、發送 BLE 封包  
- DISCONNECTED：Write = Disabled、Start = Disabled、CPU Loading Timer 停止

---

## 序列埠（COM Port）管理規格

### 1. COM Port 清單更新
- 定時掃描 (`SerialPort.GetPortNames()`) 或 `ManagementEventWatcher` 偵測熱插拔  
- **視窗不需關閉** 即可更新 COM Port 清單  
- 當埠消失（藍牙斷線）→ 自動轉為 Disconnect 模式

### 2. 開啟埠（Open）
- Open 成功 → 自動顯示 Connected  
- **不需任何額外鍵盤或滑鼠事件**  

### 3. 關閉埠（Close）
- 立即顯示 Disconnect  
- 停用 Write / Now Time / CPU Loading  

---

## 藍牙通訊協定規格（PC → ATmega328P）

### 1. 建議封包格式
```
[SOF] [CMD] [LEN] [Payload...] [Checksum] [EOF]
```
| Byte | 說明 |
|------|------|
| SOF | 0xAA |
| CMD | 指令代號 |
| LEN | Payload 長度（不含 checksum） |
| Payload | 有效資料 |
| Checksum | (CMD + LEN + 所有 Payload) & 0xFF |
| EOF | 0x55 |

### 2. 核心指令列表
- **CMD 0x01 — 設定 WS2812 顏色**  
  Payload：`[ Percent ] [ R ] [ G ] [ B ]`

- **CMD 0x02 — EEPROM Write**  
  Payload：`[ DecimalValue ]`（0~255；PC 將 4-bit 二進位轉十進位後送出）

---

## CPU Loading 監測與顏色對應規格

| CPU% | 顏色顯示 |
|------|----------|
| 0~50% | 綠色 (Green) |
| 51~84% | 黃色 (Yellow) |
| ≥85% | 紅色 (Red) |

PC 端 UI 顯示：
- 背景色變化（pnlCpuColor）  
- BLE 封包需同步送出 WS2812 顯示顏色  

---

## EEPROM 寫入邏輯與驗證機制

### 1. 輸入格式要求
TextBox 僅允許 **四位二進位字串**：`^[01]{4}$`

若不符合：
- 清空輸入欄位  
- 顯示 "Not BIN Format"  
- 停止後續寫入流程  

### 2. 二進位轉十進位
- 例如 `"1010" → 10`、`"0011" → 3`

### 3. 寫入流程要求
- PC 封包送到 MCU  
- MCU 將十進位數值寫入 EEPROM  
- 評分時流程：拔除 USB → 重新插入 → 裝置端「EEPROM」選單讀出 → OLED 顯示十進位（PC 僅負責送對資料）

---

## 錯誤處理與例外狀況規格

| 錯誤類型 | PC 程式行為 |
|---------|-------------|
| Open 失敗 | 顯示錯誤訊息；保持 Disconnect |
| SerialPort 中斷 | 自動切至 Disconnect |
| 非法二進位 | 清空欄位 + "Not BIN Format" |
| CPU 監測錯誤 | 顯示 "CPU Failed"，不中止程式 |
| 封包傳輸例外 | 顯示 "Transmission Error" |

---

## 測試案例（Test Cases）

### 1. 開機測試
- 標題列是否正確  
- COM Port 是否即時列出  

### 2. 連線測試
- 開啟埠→自動 Connected  
- 關閉埠→ Disconnect  

### 3. CPU Loading 測試
- 執行多個 loop.bat → 顏色是否正確切換  
- WS2812 接收到正確 RGB  

### 4. EEPROM 測試
| 輸入 | 行為 |
|------|------|
| 1010 | 轉 10 → 寫入 |
| 0091 | 清空 + Not BIN Format |
| 11 | 清空 + Not BIN Format |

---

## 維護與擴充建議
- 新增「串流監測視窗」解析 MCU 回傳封包  
- 支援 PC → MCU 雙向心跳  
- 封包格式可擴充為 JSON 或 CBOR  
- 支援裝置自動重連、BLE 多裝置快速切換  
- 撰寫自動化測試（UI 自動化 + 模擬序列埠）

---

**版本**：v1.0  
**作者**：PC 端 Visual Basic 專案架構（符合 113 學年度第二站題意）
