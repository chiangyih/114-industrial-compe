' ============================================================================
' 專案名稱: 113學年度工業類科學生技藝競賽 電腦修護職種 - 藍芽通訊系統
' 檔案名稱: Form1.vb
' 功能說明: 主要表單程式，處理藍芽連線、CPU監測、EEPROM寫入等功能
' 版本: v2.4
' 最後更新: 2025年1月
' ============================================================================

Imports System.IO.Ports  ' 匯入序列埠通訊所需的命名空間

Public Class Form1
    ' ========== 狀態管理 ==========
    ' 定義連線狀態列舉型別
    Private Enum ConnectionState
        Disconnected  ' 未連線狀態
        Connected     ' 已連線狀態
    End Enum

    ' 目前的連線狀態，預設為未連線
    Private currentState As ConnectionState = ConnectionState.Disconnected

    ' ========== COM Port 管理 ==========
    ' COM Port 緩衝區陣列，儲存可用的序列埠名稱（最多10個）
    Private com_buf(10) As String
    ' COM Port 數量計數器
    Private com_cnt As Integer
    ' COM Port 定期檢查計時器（每2秒檢查一次序列埠變化）
    Private portCheckTimer As Timer

    ' ========== CPU 監測 ==========
    ' CPU 效能計數器物件，用於取得 CPU 使用率
    Private cpuCounter As PerformanceCounter
    ' CPU 監測計時器（每1秒更新一次）
    Private cpuTimer As Timer

    ' ========== 設定 ==========
    ' 崗位號碼，比賽時需修改此值
    Private SeatNo As String = "24"

    ' ============================================================================
    ' 函式名稱: Form1_Load
    ' 功能說明: 表單載入時的初始化程序
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        ' 設定表單標題，顯示比賽資訊和崗位號碼
        Me.Text = "113 學年度 工業類科學生技藝競賽 電腦修護職種 台中高工 第二站 崗位號碼：" & SeatNo

        ' 初始化 CPU 監測功能
        InitializeCpuMonitor()

        ' 設定初始 UI 狀態為未連線
        SetDisconnectedState()

        ' 掃描並顯示可用的 COM Port
        ComboBox1_Ports()

        ' 啟動 COM Port 定期檢查功能
        StartPeriodicPortCheck()
    End Sub

    ' ============================================================================
    ' 函式名稱: InitializeCpuMonitor
    ' 功能說明: 初始化 CPU 監測功能
    ' 參數: 無
    ' 回傳值: 無
    ' ============================================================================
    Private Sub InitializeCpuMonitor()
        Try
            ' 建立 CPU 效能計數器物件（監測處理器總使用率）
            cpuCounter = New PerformanceCounter("Processor", "% Processor Time", "_Total")
            ' 第一次呼叫 NextValue() 永遠回傳 0，需先呼叫一次
            cpuCounter.NextValue()

            ' 建立 CPU 監測計時器
            cpuTimer = New Timer()
            ' 設定計時器間隔為 1000 毫秒（1秒）
            cpuTimer.Interval = 1000
            ' 綁定計時器 Tick 事件處理程序
            AddHandler cpuTimer.Tick, AddressOf CpuTimer_Tick
        Catch ex As Exception
            ' 如果初始化失敗，顯示錯誤訊息
            MsgBox("CPU 監測初始化失敗: " & ex.Message)
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: StartPeriodicPortCheck
    ' 功能說明: 啟動定期檢查 COM Port 的功能（每2秒檢查一次）
    ' 參數: 無
    ' 回傳值: 無
    ' ============================================================================
    Private Sub StartPeriodicPortCheck()
        ' 建立 COM Port 檢查計時器
        portCheckTimer = New Timer()
        ' 設定計時器間隔為 2000 毫秒（2秒）
        portCheckTimer.Interval = 2000
        ' 綁定計時器 Tick 事件處理程序
        AddHandler portCheckTimer.Tick, AddressOf PortCheckTimer_Tick
        ' 啟動計時器
        portCheckTimer.Start()
    End Sub

    ' ============================================================================
    ' 函式名稱: PortCheckTimer_Tick
    ' 功能說明: COM Port 定期檢查事件，檢測序列埠的新增或移除
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub PortCheckTimer_Tick(sender As Object, e As EventArgs)
        ' 儲存目前選擇的 COM Port
        Dim currentSelection As String = ""
        ' 檢查是否有選擇項目且索引值有效
        If ComboBox1.SelectedIndex >= 0 AndAlso ComboBox1.SelectedIndex < com_cnt Then
            ' 取得目前選擇的 COM Port 名稱
            currentSelection = com_buf(ComboBox1.SelectedIndex)
        End If

        ' 更新 COM Port 清單（重新掃描）
        ComboBox1_Ports()

        ' 嘗試恢復之前的選擇
        If currentSelection <> "" Then
            ' 在新的清單中尋找相同的 COM Port
            For i As Integer = 0 To com_cnt - 1
                If com_buf(i) = currentSelection Then
                    ' 找到相同的 COM Port，恢復選擇
                    ComboBox1.SelectedIndex = i
                    Exit For  ' 離開迴圈
                End If
            Next
        End If

        ' 檢查已連線的序列埠是否仍然可用
        If currentState = ConnectionState.Connected AndAlso SerialPort1.IsOpen Then
            Try
                ' 測試序列埠是否仍然有效（透過讀取 IsOpen 屬性）
                Dim testOpen = SerialPort1.IsOpen
            Catch ex As Exception
                ' 序列埠已中斷連線
                SetDisconnectedState()  ' 設定為未連線狀態
                MsgBox("裝置已中斷連線")  ' 顯示警告訊息
            End Try
        End If
    End Sub

    ' ============================================================================
    ' 函式名稱: Timer1_Tick
    ' 功能說明: 時間顯示計時器事件（每秒更新一次）
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub Timer1_Tick(sender As Object, e As EventArgs) Handles Timer1.Tick
        ' 更新時間標籤，顯示目前的日期和時間
        Label_time.Text = "Current Time : " & DateAndTime.Now
    End Sub

    ' ============================================================================
    ' 函式名稱: OPEN_Click
    ' 功能說明: OPEN 按鈕點擊事件，開啟序列埠連線
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub OPEN_Click(sender As Object, e As EventArgs) Handles OPEN.Click
        ' 檢查是否有選擇 COM Port
        If ComboBox1.SelectedIndex < 0 Then
            MsgBox("請選擇 COM Port")  ' 顯示提示訊息
            Return  ' 結束函式
        End If

        ' 設定序列埠名稱為選擇的 COM Port
        SerialPort1.PortName = com_buf(ComboBox1.SelectedIndex)
        Try
            ' 嘗試開啟序列埠
            SerialPort1.Open()
            ' 開啟成功，設定為已連線狀態
            SetConnectedState()
        Catch ex As Exception
            ' 連線失敗，顯示錯誤訊息
            MsgBox("連線失敗: " & ex.Message)
            ' 設定為未連線狀態
            SetDisconnectedState()
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: Close_Click
    ' 功能說明: Close 按鈕點擊事件，關閉序列埠連線
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub Close_Click(sender As Object, e As EventArgs) Handles Close.Click
        Try
            ' 如果序列埠已開啟，則關閉它
            If SerialPort1.IsOpen Then SerialPort1.Close()
        Catch ex As Exception
            ' 忽略關閉時的錯誤（例如序列埠已被關閉）
        End Try
        ' 設定為未連線狀態
        SetDisconnectedState()
    End Sub

    ' ============================================================================
    ' 函式名稱: SetConnectedState
    ' 功能說明: 設定 UI 為已連線狀態
    ' 參數: 無
    ' 回傳值: 無
    ' ============================================================================
    Private Sub SetConnectedState()
        ' 更新連線狀態為已連線
        currentState = ConnectionState.Connected
        ' 停用 COM Port 選單（連線後不可變更）
        ComboBox1.Enabled = False
        ' 停用 OPEN 按鈕
        OPEN.Enabled = False
        ' 啟用 Close 按鈕
        Close.Enabled = True
        ' 啟用 EEPROM 寫入功能區（需要連線才能寫入）
        GroupBox3.Enabled = True
        ' 更新連線狀態標籤文字
        Label2.Text = "Connected"
        ' 設定連線狀態標籤背景為綠色
        Label2.BackColor = Color.Green
        ' 啟動時間顯示計時器
        Timer1.Enabled = True
    End Sub

    ' ============================================================================
    ' 函式名稱: SetDisconnectedState
    ' 功能說明: 設定 UI 為未連線狀態
    ' 參數: 無
    ' 回傳值: 無
    ' ============================================================================
    Private Sub SetDisconnectedState()
        ' 更新連線狀態為未連線
        currentState = ConnectionState.Disconnected
        ' 啟用 COM Port 選單
        ComboBox1.Enabled = True
        ' 啟用 OPEN 按鈕
        OPEN.Enabled = True
        ' 停用 Close 按鈕
        Close.Enabled = False
        ' 停用 EEPROM 寫入功能區（未連線時無法寫入）
        GroupBox3.Enabled = False
        ' 更新連線狀態標籤文字
        Label2.Text = "Disconnect"
        ' 設定連線狀態標籤背景為紅色
        Label2.BackColor = Color.Red
        ' 停止時間顯示計時器
        Timer1.Enabled = False
        ' 重置時間顯示文字
        Label_time.Text = "Current Time :"
    End Sub

    ' ============================================================================
    ' 函式名稱: Write_Data
    ' 功能說明: 透過序列埠寫入資料（通用函式，保留供未來擴充使用）
    ' 參數: RS - 命令字元, W_data - 要寫入的資料字串
    ' 回傳值: 無
    ' ============================================================================
    Sub Write_Data(ByVal RS As Char, ByVal W_data As String)
        ' 檢查是否已連線
        If currentState <> ConnectionState.Connected Then
            MsgBox("請先開啟連線")  ' 顯示提示訊息
            Return  ' 結束函式
        End If

        Try
            ' 如果序列埠已開啟
            If SerialPort1.IsOpen Then
                ' 寫入命令字元
                SerialPort1.Write(RS)
                ' 寫入資料字串
                SerialPort1.Write(W_data)
            End If
        Catch ex As TimeoutException
            ' 寫入逾時錯誤
            MsgBox("寫入逾時，請檢查裝置連線")
            ' 設定為未連線狀態
            SetDisconnectedState()
        Catch ex As Exception
            ' 其他傳輸錯誤
            MsgBox("傳輸錯誤: " & ex.Message)
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: Read_Data
    ' 功能說明: 透過序列埠讀取資料（通用函式，保留供未來擴充使用）
    ' 參數: RS - 命令字元
    ' 回傳值: 讀取到的資料字串，失敗時回傳空字串
    ' ============================================================================
    Function Read_Data(ByVal RS As Char) As String
        ' 檢查是否已連線
        If currentState <> ConnectionState.Connected Then
            MsgBox("請先開啟連線")  ' 顯示提示訊息
            Return ""  ' 回傳空字串
        End If

        ' 儲存讀取到的資料
        Dim R_data As String = ""
        ' 逾時計數器
        Dim timeout As Integer = 0
        ' 最大逾時次數（50次 × 100ms = 5秒）
        Dim maxTimeout As Integer = 50

        Try
            ' 如果序列埠已開啟
            If SerialPort1.IsOpen Then
                ' 寫入命令字元（要求裝置回傳資料）
                SerialPort1.Write(RS)

                ' 等待接收資料，直到收到資料或逾時
                Do While R_data = "" And timeout < maxTimeout
                    ' 暫停 100 毫秒
                    System.Threading.Thread.Sleep(100)
                    ' 讀取序列埠現有的資料
                    R_data = SerialPort1.ReadExisting()
                    ' 增加逾時計數
                    timeout += 1
                Loop

                ' 檢查是否逾時
                If timeout >= maxTimeout Then
                    MsgBox("讀取逾時")  ' 顯示逾時訊息
                    Return ""  ' 回傳空字串
                End If
            End If
        Catch ex As TimeoutException
            ' 讀取逾時錯誤
            MsgBox("讀取逾時，請檢查裝置連線")
            ' 設定為未連線狀態
            SetDisconnectedState()
            Return ""  ' 回傳空字串
        Catch ex As Exception
            ' 其他讀取錯誤
            MsgBox("讀取錯誤: " & ex.Message)
            Return ""  ' 回傳空字串
        End Try

        ' 回傳讀取到的資料
        Return R_data
    End Function

    ' ============================================================================
    ' 函式名稱: BT_Write__Click
    ' 功能說明: EEPROM Write 按鈕點擊事件，寫入二進位資料到 EEPROM
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub BT_Write__Click(sender As Object, e As EventArgs) Handles BT_Write.Click
        ' 取得輸入框的文字並移除前後空白
        Dim input As String = BT_Data.Text.Trim()

        ' 驗證二進位輸入格式（必須是4位元的0或1）
        If Not ValidateBinaryInput(input) Then
            ' 格式錯誤，清空輸入框
            BT_Data.Text = ""
            ' 顯示錯誤訊息
            MsgBox("Not BIN Format - 請輸入 4 位二進位數字 (0 或 1)")
            Return  ' 結束函式
        End If

        ' 將二進位字串轉換為十進位數值（例如："1010" → 10）
        Dim decimalValue As Integer = Convert.ToInt32(input, 2)

        ' 發送 EEPROM 寫入命令
        SendEepromWrite(decimalValue)
    End Sub

    ' ============================================================================
    ' 函式名稱: ValidateBinaryInput
    ' 功能說明: 驗證二進位輸入格式是否正確
    ' 參數: input - 要驗證的輸入字串
    ' 回傳值: True-格式正確, False-格式錯誤
    ' ============================================================================
    Private Function ValidateBinaryInput(input As String) As Boolean
        ' 檢查長度是否為 4 個字元
        If input.Length <> 4 Then Return False

        ' 檢查每個字元是否只包含 0 或 1
        For Each c As Char In input
            ' 如果字元不是 '0' 也不是 '1'
            If c <> "0"c And c <> "1"c Then Return False
        Next

        ' 格式正確
        Return True
    End Function

    ' ============================================================================
    ' 函式名稱: SendEepromWrite
    ' 功能說明: 發送 EEPROM 寫入命令封包
    ' 參數: value - 要寫入的十進位數值（0-15）
    ' 回傳值: 無
    ' 通訊協定: [SOF] [CMD] [LEN] [Value] [Checksum] [EOF]
    ' ============================================================================
    Private Sub SendEepromWrite(value As Integer)
        ' 建立封包陣列（6個位元組）
        Dim packet(5) As Byte
        packet(0) = &HAA  ' SOF - 封包起始符號（Start Of Frame）
        packet(1) = &H2   ' CMD - 命令碼 0x02 表示 EEPROM 寫入
        packet(2) = &H1   ' LEN - 資料長度（1個位元組）
        packet(3) = CByte(value)  ' Value - 要寫入的十進位數值
        ' Checksum - 檢查碼（CMD + LEN + Value 的總和，取最低位元組）
        packet(4) = CByte((packet(1) + packet(2) + packet(3)) And &HFF)
        packet(5) = &H55  ' EOF - 封包結束符號（End Of Frame）

        Try
            ' 如果序列埠已開啟
            If SerialPort1.IsOpen Then
                ' 發送整個封包
                SerialPort1.Write(packet, 0, packet.Length)
                ' 顯示寫入成功訊息
                MsgBox("EEPROM 寫入成功: " & value.ToString())
            End If
        Catch ex As Exception
            ' 寫入失敗，顯示錯誤訊息
            MsgBox("EEPROM 寫入失敗: " & ex.Message)
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: BT_Read_Click
    ' 功能說明: Start 按鈕點擊事件，啟動 CPU 監測
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' 註記: v2.4 版本修改 - CPU 監測不需要等待藍芽連線
    ' ============================================================================
    Private Sub BT_Read_Click(sender As Object, e As EventArgs) Handles BT_Read.Click
        ' 檢查 CPU 計時器是否已建立
        If cpuTimer IsNot Nothing Then
            ' 啟動 CPU 監測計時器
            cpuTimer.Start()
            ' 停用 Start 按鈕（避免重複啟動）
            BT_Read.Enabled = False
            ' 啟用 Stop 按鈕
            btnStop.Enabled = True
        End If
    End Sub

    ' ============================================================================
    ' 函式名稱: btnStop_Click
    ' 功能說明: Stop 按鈕點擊事件，停止 CPU 監測
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        ' 檢查 CPU 計時器是否已建立
        If cpuTimer IsNot Nothing Then
            ' 停止 CPU 監測計時器
            cpuTimer.Stop()
        End If
        ' 啟用 Start 按鈕
        BT_Read.Enabled = True
        ' 停用 Stop 按鈕
        btnStop.Enabled = False

        ' 重置顯示
        TextBox1.Text = ""  ' 清空備用文字框
        TextBox1.BackColor = Color.White  ' 重置文字框背景為白色
        lblCpuPercent.Text = "0%"  ' 重置 CPU 百分比顯示
        pnlCpuColor.BackColor = Color.White  ' 重置顏色方塊為白色
    End Sub

    ' ============================================================================
    ' 函式名稱: CpuTimer_Tick
    ' 功能說明: CPU 監測計時器事件（每秒執行一次）
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' 註記: v2.4 版本修改 - 使用 Math.Round 提高準確度
    ' ============================================================================
    Private Sub CpuTimer_Tick(sender As Object, e As EventArgs)
        Try
            ' 取得 CPU 使用率（浮點數，例如：75.6）
            Dim cpuUsage As Single = cpuCounter.NextValue()
            ' 四捨五入轉換為整數（例如：75.6 → 76）
            Dim cpuPercent As Integer = CInt(Math.Round(cpuUsage))

            ' 更新 CPU 百分比標籤顯示
            lblCpuPercent.Text = cpuPercent.ToString() & "%"
            ' 更新備用文字框顯示（雖然隱藏，但保留功能）
            TextBox1.Text = cpuPercent.ToString() & "%"

            ' 宣告 RGB 顏色變數
            Dim r As Byte, g As Byte, b As Byte
            ' 宣告顯示顏色變數
            Dim displayColor As Color

            ' 根據 CPU 使用率決定顏色
            If cpuPercent <= 50 Then
                ' CPU 使用率 0-50%：綠色
                r = 0 : g = 255 : b = 0
                displayColor = Color.Green
            ElseIf cpuPercent <= 84 Then
                ' CPU 使用率 51-84%：黃色
                r = 255 : g = 255 : b = 0
                displayColor = Color.Yellow
            Else
                ' CPU 使用率 85%以上：紅色
                r = 255 : g = 0 : b = 0
                displayColor = Color.Red
            End If

            ' 更新備用文字框背景顏色
            TextBox1.BackColor = displayColor
            ' 更新顏色方塊背景顏色
            pnlCpuColor.BackColor = displayColor

            ' 如果已連線，則發送 WS2812 LED 顏色命令
            ' （未連線時只顯示，不傳送資料）
            If currentState = ConnectionState.Connected Then
                SendColorCommand(cpuPercent, r, g, b)
            End If

        Catch ex As Exception
            ' 發生錯誤時的處理
            lblCpuPercent.Text = "ERR"  ' 顯示錯誤
            TextBox1.Text = "ERR"  ' 備用顯示也顯示錯誤
            ' 顯示錯誤訊息對話框
            MsgBox("CPU 監測錯誤: " & ex.Message, MsgBoxStyle.Exclamation)
            ' 停止計時器（避免持續錯誤）
            If cpuTimer IsNot Nothing Then cpuTimer.Stop()
            ' 啟用 Start 按鈕
            BT_Read.Enabled = True
            ' 停用 Stop 按鈕
            btnStop.Enabled = False
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: SendColorCommand
    ' 功能說明: 發送 WS2812 LED 顏色控制命令封包
    ' 參數: percent - CPU 使用率百分比, r - 紅色值, g - 綠色值, b - 藍色值
    ' 回傳值: 無
    ' 通訊協定: [SOF] [CMD] [LEN] [Percent] [R] [G] [B] [Checksum] [EOF]
    ' ============================================================================
    Private Sub SendColorCommand(percent As Integer, r As Byte, g As Byte, b As Byte)
        ' 建立封包陣列（9個位元組）
        Dim packet(8) As Byte
        packet(0) = &HAA  ' SOF - 封包起始符號
        packet(1) = &H1   ' CMD - 命令碼 0x01 表示 WS2812 LED 控制
        packet(2) = &H4   ' LEN - 資料長度（4個位元組）
        packet(3) = CByte(percent)  ' Percent - CPU 使用率百分比
        packet(4) = r  ' R - 紅色亮度（0-255）
        packet(5) = g  ' G - 綠色亮度（0-255）
        packet(6) = b  ' B - 藍色亮度（0-255）
        ' Checksum - 檢查碼（CMD + LEN + Percent + R + G + B 的總和，取最低位元組）
        packet(7) = CByte((packet(1) + packet(2) + packet(3) + packet(4) + packet(5) + packet(6)) And &HFF)
        packet(8) = &H55  ' EOF - 封包結束符號

        Try
            ' 如果序列埠已開啟
            If SerialPort1.IsOpen Then
                ' 發送整個封包
                SerialPort1.Write(packet, 0, packet.Length)
            End If
        Catch ex As Exception
            ' 發送失敗時靜默處理（不顯示錯誤訊息）
            ' 避免中斷 CPU 監測的連續運作
        End Try
    End Sub

    ' ============================================================================
    ' 函式名稱: ComboBox1_Ports
    ' 功能說明: 掃描並更新 COM Port 清單
    ' 參數: 無
    ' 回傳值: 無
    ' ============================================================================
    Private Sub ComboBox1_Ports()
        ' 清空下拉選單的所有項目
        ComboBox1.Items.Clear()
        ' 重置 COM Port 計數器
        com_cnt = 0
        ' 掃描系統中所有可用的序列埠
        For Each com As String In My.Computer.Ports.SerialPortNames
            ' 將序列埠名稱加入下拉選單
            ComboBox1.Items.Add(com)
            ' 將序列埠名稱儲存到緩衝區陣列
            com_buf(com_cnt) = com
            ' 增加計數器
            com_cnt += 1
        Next

        ' 如果有可用的序列埠，預設選擇第一個
        If ComboBox1.Items.Count > 0 Then
            ComboBox1.SelectedIndex = 0
        End If
    End Sub

    ' ============================================================================
    ' 函式名稱: Label2_Click
    ' 功能說明: 連線狀態標籤點擊事件（隱藏功能：重新整理 COM Port 清單）
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub Label2_Click(sender As Object, e As EventArgs) Handles Label2.Click
        ' 重新掃描並更新 COM Port 清單
        ComboBox1_Ports()
    End Sub

    ' ============================================================================
    ' 函式名稱: btnExit_Click
    ' 功能說明: Exit 按鈕點擊事件，結束應用程式
    ' 參數: sender - 事件來源物件, e - 事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub btnExit_Click(sender As Object, e As EventArgs) Handles btnExit.Click
        ' 結束應用程式
        Application.Exit()
    End Sub

    ' ============================================================================
    ' 函式名稱: Form1_FormClosing
    ' 功能說明: 表單關閉前的清理程序
    ' 參數: sender - 事件來源物件, e - 表單關閉事件參數
    ' 回傳值: 無
    ' ============================================================================
    Private Sub Form1_FormClosing(sender As Object, e As FormClosingEventArgs) Handles MyBase.FormClosing
        Try
            ' 停止並釋放 CPU 監測計時器
            If cpuTimer IsNot Nothing Then
                cpuTimer.Stop()  ' 停止計時器
                cpuTimer.Dispose()  ' 釋放資源
            End If

            ' 釋放 CPU 效能計數器
            If cpuCounter IsNot Nothing Then
                cpuCounter.Dispose()  ' 釋放資源
            End If

            ' 停止並釋放 COM Port 檢查計時器
            If portCheckTimer IsNot Nothing Then
                portCheckTimer.Stop()  ' 停止計時器
                portCheckTimer.Dispose()  ' 釋放資源
            End If

            ' 關閉序列埠連線
            If SerialPort1.IsOpen Then
                SerialPort1.Close()  ' 關閉序列埠
            End If
        Catch ex As Exception
            ' 忽略清理過程中的任何錯誤
            ' （例如資源已被釋放或序列埠已關閉）
        End Try
    End Sub
End Class

' ============================================================================
' 程式碼結束
' ============================================================================
