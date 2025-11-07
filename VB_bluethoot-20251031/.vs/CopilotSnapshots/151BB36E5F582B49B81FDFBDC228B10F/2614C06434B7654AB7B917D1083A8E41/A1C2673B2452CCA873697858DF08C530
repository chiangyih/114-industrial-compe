
Imports System.IO.Ports

Public Class Form1
    Dim com_buf(10) As String
    Dim com_cnt As Integer
    Dim b(10) As Char
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        ComboBox1.Enabled = True
        OPEN.Enabled = True
        Close.Enabled = False
        GroupBox2.Enabled = False
        ComboBox1_Ports()
    End Sub

    Private Sub Timer1_Tick(sender As Object, e As EventArgs) Handles Timer1.Tick
        Label_time.Text = "Current Time : " & DateAndTime.Now '可由時間控制TimeOfDay
    End Sub

    Private Sub OPEN_Click(sender As Object, e As EventArgs) Handles OPEN.Click
        SerialPort1.PortName = com_buf(ComboBox1.SelectedIndex)
        Try
            SerialPort1.Open()
            GroupBox2.Enabled = True
            ComboBox1.Enabled = False
            OPEN.Enabled = False
            Close.Enabled = True
            Label2.Text = "Device Online"
            Label2.BackColor = Color.Green
        Catch ex As Exception
            MsgBox("連線失敗")
        End Try
    End Sub

    Private Sub Close_Click(sender As Object, e As EventArgs) Handles Close.Click
        SerialPort1.Close()
        ComboBox1.Enabled = True
        GroupBox2.Enabled = False
        OPEN.Enabled = True
        Label2.Text = "Device Offline"
        Label2.BackColor = Color.Red
    End Sub

    Sub Write_Data(ByVal RS As Char, ByVal W_data As String)
        Try
            If SerialPort1.IsOpen Then
                SerialPort1.Write(RS)
                SerialPort1.Write(W_data)
            End If
        Catch ex As TimeoutException
            MsgBox("Write operation timed out. Check device connection.")
        End Try
    End Sub

    Function Read_Data(ByVal RS As Char)
        Dim R_data As String = ""
        SerialPort1.Write(RS)
        Do While R_data = ""
            R_data = SerialPort1.ReadExisting
        Loop
        Return R_data
    End Function
    Private Sub BT_Write__Click(sender As Object, e As EventArgs) Handles BT_Write.Click
        Dim out_data As String = BT_Data.Text
        Write_Data("#", out_data)
    End Sub
    Private Sub BT_Read_Click(sender As Object, e As EventArgs) Handles BT_Read.Click
        TextBox1.Text = Read_Data("@")
    End Sub


    Private Sub ComboBox1_Ports() '偵測comport清單
        ComboBox1.Items.Clear()
        com_cnt = 0
        For Each com As String In My.Computer.Ports.SerialPortNames
            SerialPort1.PortName = com
            Try
                'SerialPort1.Open()
                ComboBox1.Items.Add(com)
                com_buf(com_cnt) = com
                com_cnt = com_cnt + 1
            Catch ex As Exception
                'MsgBox("開啟失敗")
            End Try
            'SerialPort1.Close()
        Next
    End Sub
    Private Sub Label2_Click(sender As Object, e As EventArgs) Handles Label2.Click '點連線狀況可以重新更連接埠
        ComboBox1_Ports()
    End Sub
End Class
