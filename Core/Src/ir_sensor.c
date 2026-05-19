#include "ir_sensor.h"                                    // 引入本模組標頭，取得常數、extern 變數宣告與 API 原型
#include "adc.h"                                         // 引入 CubeMX 產生的 ADC 設定，取得 hadc1 與 HAL 函式

/* ⭐ 改動 1：IR_Min 全域宣告改用 GCC 範圍指定初始化，從 declaration 階段就是 IR_ADC_MAX
 *    防止 IR_Calibrate() 比 IR_Init() 早被呼叫時，IR_Min 永遠停在 0 的 bug */
uint16_t IR[IR_TOTAL_COUNT]     = {0};                   // 13 顆 IR 的最新 ADC 讀值，初始全為 0
uint16_t IR_Max[IR_TOTAL_COUNT] = {0};                   // 校正時記錄到的最大值，IR_Init 中保持為 0、後續被讀值往上推
uint16_t IR_Min[IR_TOTAL_COUNT] = { [0 ... IR_TOTAL_COUNT-1] = IR_ADC_MAX }; // ⭐ 起始全 4095 → 被「比小」往下壓
uint16_t IR_Nor[IR_TOTAL_COUNT] = {0};   // ⭐ 正規化後的 IR 值（只有 [1..11] 會被更新）
/*uint16_t IR_Min[IR_TOTAL_COUNT] = {0};                   // 校正時記錄到的最小值，IR_Init 中會改成 IR_ADC_MAX 以便取小
*/

/* ⭐ 改動 2：新增 DWT 初始化函式（放在 ir_delay_us 之前） */
/* === DWT cycle counter 初始化 ===
 * Cortex-M4 內建的硬體 cycle 計數器，啟用後可以做精確 μs 延遲。
 * 只需在程式開頭呼叫一次。其他模組想用也可以共用同一個 DWT。
 * 注意：DWT 是 Cortex-M3 以後的功能，確保你的 STM32F4 系列 MCU 支援。
 */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   // 啟用 trace 子系統
    DWT->CYCCNT = 0;                                  // 計數器歸零
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             // 啟動 cycle counter
}

/* ⭐ 改動 3：ir_delay_us 換成 DWT 版本，並加上自我保護 init */
/* === 精確的微秒延遲（用 DWT cycle counter） ===
 * 前置條件：
 *   1. SystemClock_Config() 必須已執行（CubeMX 預設順序就對）
 *      否則 SystemCoreClock 還停在 HSI 16 MHz，延遲會變短約 10 倍
 *   2. DWT 必須已啟用 — 為了保險，函式內會自動檢查並補 init
 * 精度：1 個 cycle（在 168 MHz 下約 6 ns），自動跟著 SystemCoreClock 走。
 */
static inline void ir_delay_us(uint32_t us)
{
    /* 防呆：若 IR_Init() 還沒呼叫過、或 DWT 沒被啟用 → 自己補 init */
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        dwt_init();
    }
    uint32_t cycles = us * (SystemCoreClock / 1000000);   // us → cycles
    uint32_t start  = DWT->CYCCNT;                        // 記錄起始時間
    while ((DWT->CYCCNT - start) < cycles);               // 等到差值達到目標
}
/*
static inline void ir_delay_us(uint32_t us)              // 模組內部用的粗略微秒延遲（static：只在本檔可見）
{                                                        // 函式起始
    volatile uint32_t count = us * 30;                   // volatile 防止被優化掉；30 是經驗常數，依 MCU 時脈微調
    while (count--) { __NOP(); }                         // 空轉 + NOP 燒掉時間，達到約 us 微秒的延遲
}                                                        // 函式結束
*/

/* === 觸發序列中的「下一個」rank，等待完成，回傳讀值 ===
 * ⚠️ Discontinuous Mode 重點：
 *   此函式內絕對不能呼叫 HAL_ADC_Stop()。
 *   一旦 Stop 就會清掉 ADON、下次 Start 時序列指標歸零，
 *   結果所有讀值都會是 rank 1 的值。
 *   Stop 必須等整個序列跑完後、在外層函式裡才呼叫。
 */
static uint16_t adc_trigger_one(void)                    // 觸發一次 ADC、取得目前 rank 的轉換結果（內部 helper）
{                                                        // 函式起始
    uint16_t v = 0;                                      // 預設回傳 0（若 PollForConversion 逾時就回 0）
    HAL_ADC_Start(&hadc1);                               // 啟動 ADC；Discontinuous Mode 下會轉換序列中的「下一個」rank
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) // 輪詢等待轉換完成，最多等 10ms
    {                                                    // 轉換成功才讀值
        v = HAL_ADC_GetValue(&hadc1);                    // 取得 12-bit 轉換結果（0~4095）
    }                                                    // if 結束
    return v;  // ← 沒有 Stop！
}                                                        // 函式結束

/* ⭐ 改動 4：IR_Init 開頭呼叫 dwt_init()；IR_Min 那行拿掉（已在全域宣告處理） */
void IR_Init(void)
{
    dwt_init();                                           // ⭐ 新增：啟用 DWT cycle counter

    for (int i = 0; i < IR_TOTAL_COUNT; i++)
    {
        IR[i]     = 0;
        IR_Max[i] = 0;
        // ⭐ IR_Min[i] 不用再設，已在全域宣告時就是 IR_ADC_MAX
        /*IR_Min[i] = IR_ADC_MAX;*/  // ⭐ 已改用 GCC 範圍指定初始化，這裡不需要再設一次了
    }
}
/*
void IR_Init(void)                                       // 對外 API：初始化 IR 模組的暫存陣列
{                                                        // 函式起始
    for (int i = 0; i < IR_TOTAL_COUNT; i++)             // 走訪 13 顆 IR
    {                                                    // for 起始
        IR[i]     = 0;                                   // 當前讀值清 0
        IR_Max[i] = 0;                                   // 最大值起始 0，校正過程中會被「比大」更新
        IR_Min[i] = IR_ADC_MAX;                          // 最小值起始 4095，校正過程中會被「比小」更新
    }                                                    // for 結束
}                                                        // 函式結束
*/

/* ⭐ 改動 5：新增「重新校正」函式，給按鈕重置等場景用 */
/* === 重置校正資料（不影響 IR[] 即時讀值） === */
void IR_ResetCalibration(void)
{
    for (int i = 0; i < IR_TOTAL_COUNT; i++)
    {
        IR_Max[i] = 0;
        IR_Min[i] = IR_ADC_MAX;
    }
}

/* === 只讀車頭 11 顆 (IR[1]~IR[11]) === */
void IR_ReadHead(void)                                   // 對外 API：循跡用，只讀中間 11 顆 IR
{                                                        // 函式起始
    HAL_GPIO_WritePin(IR_MOS_HEAD_GPIO_Port, IR_MOS_HEAD_Pin, GPIO_PIN_SET);   // 打開車頭 IR MOSFET，點亮 11 顆 LED
    ir_delay_us(IR_SETTLE_US);                           // 等 IR 輸出穩定（約 50us）後再開始採樣

    (void)adc_trigger_one();                            // rank 1: IR_L → 丟掉
    for (int i = IR_HEAD_START; i <= IR_HEAD_END; i++)   // 依序讀 rank 2~12，對應 IR[1]~IR[11]
    {                                                    // for 起始
        IR[i] = adc_trigger_one();                      // rank 2~12: IR1~IR11
    }                                                    // for 結束
    (void)adc_trigger_one();                            // rank 13: IR_R → 丟掉

    HAL_ADC_Stop(&hadc1);                               // ⭐ 13 個 rank 全跑完才 Stop

    HAL_GPIO_WritePin(IR_MOS_HEAD_GPIO_Port, IR_MOS_HEAD_Pin, GPIO_PIN_RESET); // 關閉車頭 IR，省電並避免發熱
}                                                        // 函式結束

/* === 只讀側邊 2 顆 (IR[0], IR[12]) === */
void IR_ReadSide(void)                                   // 對外 API：marker 偵測用，只讀左右兩顆 IR
{                                                        // 函式起始
    HAL_GPIO_WritePin(IR_MOS_SIDE_GPIO_Port, IR_MOS_SIDE_Pin, GPIO_PIN_SET);   // 打開側邊 IR MOSFET
    ir_delay_us(IR_SETTLE_US);                           // 等 IR 輸出穩定

    IR[IR_IDX_SIDE_L] = adc_trigger_one();              // rank 1: IR_L
    for (int i = 0; i < IR_HEAD_COUNT; i++)              // 中間 11 個 rank 對應車頭，不需要 → 全部丟掉
    {                                                    // for 起始
        (void)adc_trigger_one();                        // rank 2~12: HEAD → 丟掉
    }                                                    // for 結束
    IR[IR_IDX_SIDE_R] = adc_trigger_one();              // rank 13: IR_R

    HAL_ADC_Stop(&hadc1);                               // ⭐ 同上

    HAL_GPIO_WritePin(IR_MOS_SIDE_GPIO_Port, IR_MOS_SIDE_Pin, GPIO_PIN_RESET); // 關閉側邊 IR
}                                                        // 函式結束

/* === 一次讀全部 13 顆（校正、debug 用） === */
void IR_ReadAll(void)                                    // 對外 API：校正/除錯時一次讀完所有 IR
{                                                        // 函式起始
    HAL_GPIO_WritePin(IR_MOS_HEAD_GPIO_Port, IR_MOS_HEAD_Pin, GPIO_PIN_SET);   // 打開車頭 IR
    HAL_GPIO_WritePin(IR_MOS_SIDE_GPIO_Port, IR_MOS_SIDE_Pin, GPIO_PIN_SET);   // 打開側邊 IR
    ir_delay_us(IR_SETTLE_US);                           // 等所有 IR 一起穩定

    for (int i = 0; i < IR_TOTAL_COUNT; i++)             // 依序讀 13 個 rank
    {                                                    // for 起始
        IR[i] = adc_trigger_one();                       // 直接把每個 rank 的讀值存進 IR[i]
    }                                                    // for 結束

    HAL_ADC_Stop(&hadc1);                               // ⭐ 同上

    HAL_GPIO_WritePin(IR_MOS_HEAD_GPIO_Port, IR_MOS_HEAD_Pin, GPIO_PIN_RESET); // 關閉車頭 IR
    HAL_GPIO_WritePin(IR_MOS_SIDE_GPIO_Port, IR_MOS_SIDE_Pin, GPIO_PIN_RESET); // 關閉側邊 IR
}                                                        // 函式結束

void IR_Calibrate(void)                                  // 對外 API：取一次全讀值，並更新每顆 IR 的 Max/Min
{                                                        // 函式起始
    IR_ReadAll();                                        // 先做一次全讀，結果放在 IR[]
    for (int i = 0; i < IR_TOTAL_COUNT; i++)             // 走訪每一顆
    {                                                    // for 起始
        if (IR[i] > IR_Max[i]) IR_Max[i] = IR[i];        // 比目前 Max 大 → 更新 Max（記錄最強反射）
        if (IR[i] < IR_Min[i]) IR_Min[i] = IR[i];        // 比目前 Min 小 → 更新 Min（記錄最弱反射）
    }                                                    // for 結束
}                                                        // 函式結束

/* === 把車頭 11 顆 IR 正規化到 0 ~ IR_NOR_MAX ===
 * 公式：IR_Nor[i] = (IR[i] - IR_Min[i]) / (IR_Max[i] - IR_Min[i]) * IR_NOR_MAX
 * 使用整數運算，無浮點、無 NaN/Inf 風險
 * 注意：呼叫前必須先做過校正（讓 IR_Max[i] - IR_Min[i] > IR_MIN_RANGE），
 *      否則未校正的感測器會回傳「中間值」當保護機制
 */
void IR_Normalize(void)
{
    for (int i = IR_HEAD_START; i <= IR_HEAD_END; i++)   // 只跑 IR[1] ~ IR[11]
    {
        /* 防呆 1：未校正或範圍太小 → 回傳中間值，避免 0 除錯誤或亂噴
         *   先檢查 IR_Max[i] <= IR_Min[i]（含未校正初始狀態 Max=0/Min=4095），
         *   否則 uint16_t 減法會 underflow 成 ~61000 而 bypass 此保護
         */
        if (IR_Max[i] <= IR_Min[i] || (IR_Max[i] - IR_Min[i]) < IR_MIN_RANGE)
        {
            IR_Nor[i] = IR_NOR_MAX / 2;                  // 回 500，PID 看起來是「中間」狀態
            continue;
        }
        uint16_t range = IR_Max[i] - IR_Min[i];          // 進到這行時保證 Max > Min，不會 underflow

        /* 防呆 2：IR[i] 跌破校正時的 Min（環境光減弱或飄移）→ 視為 0 */
        if (IR[i] <= IR_Min[i])
        {
            IR_Nor[i] = 0;
            continue;
        }

        /* 主計算：(IR[i] - IR_Min[i]) * IR_NOR_MAX / range
         * 用 uint32_t 計算避免溢位：
         *   最壞情況 (4095 - 0) * 1000 = 4,095,000 < 2^22，遠低於 uint32_t 上限
         */
        uint32_t result = (uint32_t)(IR[i] - IR_Min[i]) * IR_NOR_MAX / range;

        /* 防呆 3：IR[i] 超越校正時的 Max（環境光變強）→ 夾到 IR_NOR_MAX */
        if (result > IR_NOR_MAX)
        {
            result = IR_NOR_MAX;
        }

        IR_Nor[i] = (uint16_t)result;
    }
}