# PC ? MCU 通訊協定實作文件

## 協定概述

基於 plan.md 第8節規格實作的二進位協定，用於 PC 與 ATmega328P 之間的資料交換。

## 封包格式

```
┌─────┬─────┬─────┬─────────────┬──────────┬─────┐
│ SOF │ CMD │ LEN │  Payload    │ Checksum │ EOF │
├─────┼─────┼─────┼─────────────┼──────────┼─────┤
│ 0xAA│ 1B  │ 1B  │  LEN bytes  │    1B    │0x55 │
└─────┴─────┴─────┴─────────────┴──────────┴─────┘
```

### 欄位說明

| 欄位 | 大小 | 說明 | 值 |
|------|------|------|-----|
| SOF | 1 Byte | Start of Frame（幀起始） | 固定 0xAA |
| CMD | 1 Byte | 指令代號 | 見下表 |
| LEN | 1 Byte | Payload 長度（不含 Checksum） | 0-255 |
| Payload | LEN Bytes | 有效資料 | 依指令而定 |
| Checksum | 1 Byte | 校驗和 | (CMD + LEN + 所有 Payload) & 0xFF |
| EOF | 1 Byte | End of Frame（幀結束） | 固定 0x55 |

## 指令列表

### CMD 0x01 - WS2812 顏色控制

**功能**: 設定 WS2812 RGB LED 顏色

**方向**: PC → MCU

**封包格式**:
```
AA 01 04 [Percent] [R] [G] [B] [Checksum] 55
```

**Payload 結構**:
| Offset | 大小 | 欄位 | 說明 | 範圍 |
|--------|------|------|------|------|
| 0 | 1B | Percent | CPU 使用率百分比 | 0-100 |
| 1 | 1B | R | 紅色分量 | 0-255 |
| 2 | 1B | G | 綠色分量 | 0-255 |
| 3 | 1B | B | 藍色分量 | 0-255 |

**Checksum 計算**:
```
Checksum = (0x01 + 0x04 + Percent + R + G + B) & 0xFF
```

**範例 1**: CPU 30%, 綠色 (R:0, G:255, B:0)
```
AA 01 04 1E 00 FF 00 22 55
    ↑  ↑  ↑  ↑  ↑   ↑  ↑  ↑
  │  │  │  │  │   │  │  └─ EOF
    │  │  │  │  │   │  └──── Checksum = (01+04+1E+00+FF+00) & FF = 22
    │  │  │  │  │   └─────── B = 0
 │  │  │  │  └─────────── G = 255
    │  │  │  └────────────── R = 0
    │  │  └───────────────── Percent = 30 (0x1E)
    │  └──────────────────── LEN = 4
    └─────────────────────── CMD = 0x01
```

**範例 2**: CPU 75%, 黃色 (R:255, G:255, B:0)
```
AA 01 04 4B FF FF 00 4F 55
Checksum = (01+04+4B+FF+FF+00) & FF = 4F
```

**範例 3**: CPU 90%, 紅色 (R:255, G:0, B:0)
```
AA 01 04 5A FF 00 00 5E 55
Checksum = (01+04+5A+FF+00+00) & FF = 5E
```

**PC 端實作** (`SendColorCommand`):
```vb
Private Sub SendColorCommand(percent As Integer, r As Byte, g As Byte, b As Byte)
    Dim packet(8) As Byte
    packet(0) = &HAA  ' SOF
    packet(1) = &H1 ' CMD
    packet(2) = &H4            ' LEN
    packet(3) = CByte(percent)   ' Percent
    packet(4) = r       ' R
packet(5) = g                ' G
    packet(6) = b     ' B
    packet(7) = CByte((packet(1) + packet(2) + packet(3) + packet(4) + packet(5) + packet(6)) And &HFF)
    packet(8) = &H55             ' EOF
    
    SerialPort1.Write(packet, 0, packet.Length)
End Sub
```

---

### CMD 0x02 - EEPROM 寫入

**功能**: 寫入數值到 EEPROM

**方向**: PC → MCU

**封包格式**:
```
AA 02 01 [Value] [Checksum] 55
```

**Payload 結構**:
| Offset | 大小 | 欄位 | 說明 | 範圍 |
|--------|------|------|------|------|
| 0 | 1B | Value | 要寫入的十進位數值 | 0-15 |

**Checksum 計算**:
```
Checksum = (0x02 + 0x01 + Value) & 0xFF
```

**範例 1**: 寫入 10 (二進位輸入: 1010)
```
AA 02 01 0A 0D 55
↑  ↑↑  ↑  ↑
    │  │  │  │  └─ EOF
    │  │  │  └──── Checksum = (02+01+0A) & FF = 0D
    │  │  └─────── Value = 10 (0x0A)
    │  └────────── LEN = 1
    └───────────── CMD = 0x02
```

**範例 2**: 寫入 3 (二進位輸入: 0011)
```
AA 02 01 03 06 55
Checksum = (02+01+03) & FF = 06
```

**範例 3**: 寫入 15 (二進位輸入: 1111)
```
AA 02 01 0F 12 55
Checksum = (02+01+0F) & FF = 12
```

**範例 4**: 寫入 0 (二進位輸入: 0000)
```
AA 02 01 00 03 55
Checksum = (02+01+00) & FF = 03
```

**PC 端實作** (`SendEepromWrite`):
```vb
Private Sub SendEepromWrite(value As Integer)
    Dim packet(5) As Byte
    packet(0) = &HAA           ' SOF
    packet(1) = &H2    ' CMD
 packet(2) = &H1            ' LEN
    packet(3) = CByte(value)   ' Value
    packet(4) = CByte((packet(1) + packet(2) + packet(3)) And &HFF)
    packet(5) = &H55 ' EOF
    
    SerialPort1.Write(packet, 0, packet.Length)
End Sub
```

**二進位轉換流程**:
```vb
' 使用者輸入: "1010"
Dim input As String = "1010"
Dim decimalValue As Integer = Convert.ToInt32(input, 2)  ' = 10
SendEepromWrite(decimalValue)
```

---

## MCU 端實作建議

### 封包解析狀態機

```cpp
enum ParseState {
 WAIT_SOF,
    WAIT_CMD,
 WAIT_LEN,
    WAIT_PAYLOAD,
    WAIT_CHECKSUM,
    WAIT_EOF
};

ParseState state = WAIT_SOF;
uint8_t cmd, len, payload[256], checksum;
uint8_t payloadIndex = 0;
uint8_t calcChecksum = 0;

void parseSerialData(uint8_t byte) {
    switch (state) {
        case WAIT_SOF:
       if (byte == 0xAA) {
state = WAIT_CMD;
           calcChecksum = 0;
   }
 break;
       
        case WAIT_CMD:
    cmd = byte;
            calcChecksum += byte;
         state = WAIT_LEN;
            break;
 
        case WAIT_LEN:
       len = byte;
     calcChecksum += byte;
 payloadIndex = 0;
            state = (len > 0) ? WAIT_PAYLOAD : WAIT_CHECKSUM;
    break;
        
        case WAIT_PAYLOAD:
   payload[payloadIndex++] = byte;
     calcChecksum += byte;
    if (payloadIndex >= len) {
              state = WAIT_CHECKSUM;
 }
       break;
  
        case WAIT_CHECKSUM:
            checksum = byte;
     state = WAIT_EOF;
      break;
         
        case WAIT_EOF:
            if (byte == 0x55 && checksum == (calcChecksum & 0xFF)) {
       processCommand(cmd, payload, len);
      }
 state = WAIT_SOF;
            break;
    }
}

void processCommand(uint8_t cmd, uint8_t* data, uint8_t len) {
    switch (cmd) {
    case 0x01:  // WS2812 Color
    if (len == 4) {
           uint8_t percent = data[0];
     uint8_t r = data[1];
     uint8_t g = data[2];
         uint8_t b = data[3];
                setWS2812Color(r, g, b);
      }
    break;
         
        case 0x02:  // EEPROM Write
     if (len == 1) {
         uint8_t value = data[0];
      EEPROM.write(0, value);  // 寫入位址0
      }
      break;
    }
}
```

---

## 顏色對應表

PC 端根據 CPU 使用率自動選擇顏色：

| CPU% | 顏色 | R | G | B | 封包範例（CPU=值） |
|------|------|---|---|---|--------------------|
| 0-50% | 綠色 | 0 | 255 | 0 | `AA 01 04 32 00 FF 00 36 55` (50%) |
| 51-84% | 黃色 | 255 | 255 | 0 | `AA 01 04 42 FF FF 00 45 55` (66%) |
| 85-100% | 紅色 | 255 | 0 | 0 | `AA 01 04 64 FF 00 00 68 55` (100%) |

---

## 錯誤處理

### PC 端
- **Timeout**: WriteTimeout = 5000ms, ReadTimeout = 5000ms
- **中斷**: 自動轉 Disconnect 狀態
- **格式錯誤**: 清空輸入並提示使用者

### MCU 端建議
- **SOF 錯誤**: 重置狀態機
- **Checksum 錯誤**: 丟棄封包，重置狀態機
- **EOF 錯誤**: 丟棄封包，重置狀態機
- **LEN 過大**: 限制最大值，防止緩衝區溢位

---

## 測試封包產生器

使用以下程式碼產生測試封包：

```vb
' 測試 WS2812 - 綠色
SendColorCommand(30, 0, 255, 0)
' 輸出: AA 01 04 1E 00 FF 00 22 55

' 測試 EEPROM - 寫入 10
SendEepromWrite(10)
' 輸出: AA 02 01 0A 0D 55
```

---

## 序列埠設定

| 參數 | 值 |
|------|-----|
| BaudRate | 9600 |
| DataBits | 8 |
| Parity | None |
| StopBits | One |
| FlowControl | None |
| WriteTimeout | 5000ms |
| ReadTimeout | 5000ms |

---

## 版本資訊

- **協定版本**: 1.0
- **相容 MCU**: ATmega328P
- **PC 實作**: Visual Basic .NET
- **參考文件**: plan.md 第8節

---

## 擴充建議

未來可考慮新增的指令：

- **CMD 0x03**: MCU → PC 狀態回報
- **CMD 0x04**: PC → MCU 心跳封包
- **CMD 0x05**: MCU → PC EEPROM 讀取回應
- **CMD 0xFF**: 重置/復位指令

---

**注意**: MCU 端需實作對應的封包解析與處理邏輯
