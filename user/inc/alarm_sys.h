/**
 * @file    alarm_sys.h
 * @brief   报警系统模块头文件。
 *          声明 blink_led() / beep() / alarm_sys_func() 和系统模式枚举。
 *          对应的 .c 文件加入 Keil 工程后即可使用。
 */
#ifndef ALARM_SYS_H
#define ALARM_SYS_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================== 宏定义 ======================== */
#define LED_COUNT         4U     /* 板载 LED 数量                    */
#define BLINK_TIMES       5U     /* 每颗 LED 默认闪烁次数（备用）    */
#define DELAY_MS          500U   /* 运行模式 LED 亮/灭延时 (ms)      */
#define BEEP_MS           300U   /* 蜂鸣器单次鸣叫时长 (ms)          */

/* 报警模式加速参数 */
#define DELAY_MS_FAST     150U   /* 报警模式 LED 亮/灭延时 (ms)      */
#define BEEP_INTERVAL_MS  500U   /* 报警模式蜂鸣器翻转间隔 (ms)      */

/* ======================== 类型定义 ======================== */

/** 系统工作模式 */
typedef enum {
    MODE_STANDBY = 0,   /* 待机：蜂鸣器不响，流水灯不工作           */
    MODE_RUNNING,       /* 运行：只运行流水灯                       */
    MODE_ALARM          /* 报警：流水灯加速运行 + 蜂鸣器间隔报警     */
} sys_mode_t;

/* ======================== 函数声明 ======================== */

/**
 * @brief  阻塞式 LED 闪烁（与原 main.c 一致）。
 * @param  led_num   LED 编号（1~4）
 * @param  times     闪烁次数
 * @param  delay_ms  亮/灭持续时间（毫秒）
 */
void blink_led(uint8_t led_num, uint16_t times, uint32_t delay_ms);

/**
 * @brief  阻塞式蜂鸣器鸣叫。
 * @param  beep_ms  鸣叫时长（毫秒）
 */
void beep(uint32_t beep_ms);

/**
 * @brief  报警系统主循环函数（非阻塞，需在主循环中反复调用）。
 *         根据当前模式执行不同的行为：
 *         - MODE_STANDBY: 关闭所有输出，重置状态
 *         - MODE_RUNNING: 非阻塞流水灯（LED1~4 依次闪烁 1~4 次）
 *         - MODE_ALARM:   流水灯加速 + 蜂鸣器间歇报警
 * @param  mode  系统工作模式（见 sys_mode_t）
 */
void alarm_sys_func(sys_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_SYS_H */
