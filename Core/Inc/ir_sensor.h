#ifndef __IR_SENSOR_H              // 若尚未定義 __IR_SENSOR_H 才繼續往下（防止重複 include）
#define __IR_SENSOR_H              // 標記此標頭已被 include 過

#ifdef __cplusplus                 // 若使用 C++ 編譯器
extern "C" {                       // 用 C 連結方式（避免 C++ name mangling，讓 C/C++ 可共用）
#endif                             // 結束 __cplusplus 判斷

#include "main.h"                  // 引入主標頭，取得 uint16_t、HAL 型別等

/* === 設定常數（陣列大小、範圍、ADC 解析度、延遲時間等可調參數） === */
#define IR_TOTAL_COUNT    13      // 總感測器數量
#define IR_HEAD_COUNT     11      // 車頭線陣列數量
#define IR_HEAD_START     1       // 車頭陣列起始 index（IR[1]=IR1）
#define IR_HEAD_END       11      // 車頭陣列結束 index（IR[11]=IR11）
#define IR_NOR_MAX        1000    // ⭐ 新增：正規化輸出範圍上限
#define IR_MIN_RANGE      50      // ⭐ 新增：Max-Min 至少要這麼大，才算有校正過（避免 Max、Min 太接近導致除以小數放大雜訊）

#define IR_ADC_MAX        4095    // 12-bit ADC 最大值
#define IR_SETTLE_US      50      // IR 開啟後的穩定時間 (us)

/* === 索引別名（為常用的 IR 位置取易讀名稱，避免程式裡到處出現魔術數字） === */
#define IR_IDX_SIDE_L     0       // 左側 marker
#define IR_IDX_SIDE_R     12      // 右側 marker
#define IR_IDX_HEAD_CENTER 6      // 車頭中心 (IR6)

/* === Marker 偵測設定 === 
 * 比對對象：IR_Nor[0] / IR_Nor[12]（已正規化到 0~1000 空間）
 * 取 600 的理由：
 *   - 偏向 marker 端（接近 1000），對邊緣抖動有抗噪餘裕
 *   - 避開防呆回傳值 500，未校正時 (IR_Nor=500 < 600) 不會誤觸發
 *
 * 實測值（YYYY-MM-DD，賽道 XX）：               ← 量完後填這裡
 *   黑底：    IR_Nor[0] ≈ ___,  IR_Nor[12] ≈ ___
 *   壓 marker：IR_Nor[0] ≈ ___,  IR_Nor[12] ≈ ___
 *   依實測微調，建議區間 600~700
 */
#define MARKER_THRESHOLD    600    // ⭐ v2: 暫定門檻值（詳細說明見上方 block 註解，normalized 0~1000 空間）
                                    // 舊版註解（raw 空間的舊公式，已不適用）：通常是 (IR_Min + IR_Max) / 2 附近

/* === 對外變數（在 ir_sensor.c 內定義，其他 .c 透過 extern 共用） === */
extern uint16_t IR[IR_TOTAL_COUNT];      // 目前 ADC 讀值（13 顆 IR 的原始值）
extern uint16_t IR_Max[IR_TOTAL_COUNT];  // 校正後的最大值（白線/反射強）
extern uint16_t IR_Min[IR_TOTAL_COUNT];  // 校正後的最小值（黑線/反射弱）
extern uint16_t IR_Nor[IR_TOTAL_COUNT];  // ⭐ 新增：正規化後的值（0 ~ 1000）
extern uint16_t IR_Marker_L_Count;  // ⭐ 累計左 marker 偵測到的次數
extern uint16_t IR_Marker_R_Count;  // ⭐ 累計右 marker 偵測到的次數
extern uint8_t  IR_Marker_L_State;  // ⭐ 目前是否正在通過左 marker（1=正在通過, 0=沒有）
extern uint8_t  IR_Marker_R_State;  // ⭐ 目前是否正在通過右 marker

/* === API (Application Programming Interface，應用程式介面：模組對外公開的函式清單) === */
void IR_Init(void);          // 初始化 IR 模組（GPIO/ADC 預設值等）
void IR_ReadHead(void);      // 只讀車頭 11 顆（給循跡用）
void IR_ReadSide(void);      // 只讀左右 2 顆（給 marker 偵測用）
void IR_ReadAll(void);       // 一次讀全部 13 顆（測試/校正用）
void IR_Calibrate(void);     // 全讀並更新 Max/Min
void IR_ResetCalibration(void);  // ⭐ 新增：重置校正資料（不影響 IR[] 即時讀值）
void IR_Normalize(void);   // ⭐ v2: 把全部 13 顆 IR[0..12] 正規化到 IR_Nor[0..12]（含側邊 marker）
/* 舊版：void IR_Normalize(void);   // ⭐ 新增：把 IR[1..11] 正規化到 IR_Nor[1..11] */
void IR_DetectMarker(void);          // ⭐ v2: 偵測 marker（用 edge detection），需在 IR_Normalize 之後呼叫（依賴 IR_Nor[]）
/* 舊版：void IR_DetectMarker(void);          // ⭐ 偵測 marker（用 edge detection），需在 IR_ReadSide 之後呼叫 */
void IR_ResetMarkerCount(void);      // ⭐ 重置計數（例如每圈開始時呼叫）

#ifdef __cplusplus                 // 若使用 C++ 編譯器
}                                  // 結束 extern "C" 區塊
#endif                             // 結束 __cplusplus 判斷

#endif                             // 結束最外層 #ifndef __IR_SENSOR_H