# 專案概況
1. 專案名稱：Andantino_V3
2. 核心目標：參與日本/國際賽規的競速自走車競賽，追求高速循跡與精準過彎。
3. 開發環境：VS Code + PlatformIO + STM32CubeMX。

# 硬體架構 (Hardware Specs)
1. MCU: STM32F405RGT6(核心頻率f_clk=168MHz)。
2. 馬達驅動：雙馬達差速控制，搭配PWM速度調節。
3. 感測器：13路紅外線循跡模組，讀取類比訊號(透過ADC轉換為12-bit數位數值處理)。
4. 反饋系統：雙增量式編碼器(Encoder)，用於速度閉環控制。

# 定時器詳細設定 (Timer Configurations)
1. 蜂鳴器控制 (Buzzer) - TIM2
   (1) 硬體配置：TIM2 Channel 4 (PWM Generation)。
   (2) 預設參數：`PSC = 0`, `ARR = 1000`, `Pulse = 500` (預設 50% 佔空比獲得最大音量)。
2. 馬達 PWM 驅動 (Motor Control) - TIM1
   (1) 硬體配置：TIM1 負責產生左右馬達的PWM訊號(決定車輪轉速)。
   (2) 預設參數：`PSC = 0`, `ARR = 3000`。
   (3) 物理意義：產生56kHz高頻PWM(168MHz/3000=56kHz)，此超音波頻率可完全避開馬達運轉時的刺耳噪音。
   (4) 控制解析度：速度輸出值範圍嚴格限制在 0 ~ 3000 之間。

3. 反饋系統 (Encoder 讀取) - TIM3 & TIM4
   (1) 硬體配置：
       - 右輪編碼器 (Right Motor): TIM3 (CH1 接 R-B，CH2 接 R-A)。
       - 左輪編碼器 (Left Motor): TIM4 (CH1 接 L-A，CH2 接 L-B)。
   (2) 模式設定：皆設定為 `Encoder Mode TI1 and TI2` (支援 4 倍頻高解析度讀取)。
   (3) 硬體濾波：已在 CubeMX 底層設定 Input Filter，自動過濾高頻雜訊。

4. 吸地風扇控制 (Suction Fan / EDF) - TIM8
   (1) 硬體配置：TIM8 Channel 4 (PWM Generation)。
   (2) 核心功能：控制吸地風扇轉速，產生強大下壓力 (Downforce) 以提升極速過彎的抓地力。
   (3) 參數設定：`PSC = 0`, `ARR = 3000`, 預設啟動 `Pulse = 0`。
   (4) 物理意義：產生 56 kHz 高頻 PWM，控制解析度範圍為 0 ~ 3000。

5. 系統心跳與中斷控制 (System Heartbeat) - TIM10
   (1) 硬體配置：TIM10 (Activated)，負責產生基準時間中斷。
   (2) 參數設定：`PSC = 168 - 1`, `ARR = 1000 - 1`。
   (3) 物理意義：將 168MHz 系統時脈分頻為 1MHz，產生精準的 1ms (1kHz) 週期中斷。(未來系統穩定後，可考慮將 ARR 改為 500-1 升級為 0.5ms 中斷)。

# 程式碼生成規範與限制 (AI Coding Constraints)
1. 【中斷架構限制】
   - 所有的定期控制邏輯（如 PID 計算、感測器掃描），必須且只能生成在 `HAL_TIM_PeriodElapsedCallback` 函數內部。
   - 必須使用 `if (htim->Instance == TIM10)` 來過濾系統心跳，防止中斷衝突。

2. 【效能與語法限制】
   - 更新 PWM 佔空比時，禁止頻繁呼叫 Start/Stop，統一使用 `__HAL_TIM_SET_COMPARE(&htimX, TIM_CHANNEL_Y, value)` 動態修改數值。

3. 【系統即時性 (Real-time) 禁忌】
   - 絕對嚴禁在任何中斷服務常式 (ISR) 或 Callback 中生成 `HAL_Delay()`、`printf()` 或任何會造成系統阻塞 (Blocking) 的程式碼。

4. 【安全防呆機制 (Failsafe)】
   - 嚴禁使用 bool 判斷斷線。必須實作計時器機制（如 `track_out_timer_ms`），在 TIM10 中斷內累加。
   - 若連續 500ms 讀取不到賽道黑線訊號，必須強制呼叫急停函式，將動力馬達 PWM 歸零。

5. 【記憶體與跨中斷防護 (Volatile Strict Rule)】
   - 凡是在 TIM10 中斷內被寫入，且在主迴圈讀取的變數或結構體（如 PID 控制器、感測器狀態），絕對必須加上 `volatile` 宣告，防止編譯器優化導致暴衝。

6. 【控制演算法規範】
   - 生成 PID 控制邏輯時，結構體內部必須包含 `integral_limit` (積分抗飽和上限) 與 `output_limit` (輸出限幅)。
   - 馬達輸出必須實作死區補償 (`MOTOR_DEADBAND`)，用以克服馬達靜摩擦力：
     - 當 PID 輸出 > 0：`final_pwm = pid_output + MOTOR_DEADBAND`
     - 當 PID 輸出 < 0：`final_pwm = pid_output − MOTOR_DEADBAND`
     - 當 PID 輸出 = 0（如急停或目標靜止）：`final_pwm = 0`，禁止疊加偏移
     - 補償後必須再次套用 `output_limit` (上下限為 ±3000) 保護，避免疊加後超出 PWM 解析度範圍。

7. 【自動修改後的報告義務】
   - 執行任何「自動修改程式碼」後，必須提供詳細的修改報告，包含：(1) 更動了哪些檔案 (2) 修改了什麼核心邏輯 (3) 工程考量或符合哪項規範。嚴禁默默修改。

# 檔案結構 (File Structure)
* `Core/Inc/motor.h`: 存放 `PID_Controller` 與 `Robot_State` 結構體定義，統一管理馬達、風扇與物理常數。
* `Core/Src/main.c`: 程式入口、系統初始化與硬體設定。
* `Core/Src/motor.c`: 馬達與風扇控制邏輯 (包含 PWM 設定、編碼器讀取、PID 運算與運動學解算)。
* `Core/Src/sensor.c`: 13 路紅外線數據處理與 ADC 讀取邏輯。
* `Drivers/`: STM32 HAL 庫與底層驅動 (已由 Git 追蹤，AI 無需修改)。