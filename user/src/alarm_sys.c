/**
 * @file    alarm_sys.c
 * @brief   报警系统模块实现。
 *          - blink_led() / beep() 为阻塞式工具函数，与原 main.c 一致。
 *          - alarm_sys_func() 是非阻塞状态机，需在主循环中反复调用，
 *            根据 sys_mode_t 模式执行不同行为。
 *
 *          模式说明：
 *          MODE_STANDBY — 待机，无输出
 *          MODE_RUNNING — 正常流水灯
 *          MODE_ALARM  — 流水灯加速 + 蜂鸣器间歇报警
 *
 *          加入 Keil 工程方法：
 *          1. 右键 user 分组 → Add Existing Files → 选择本文件
 *          2. 如果还没加 Include Path：Options → C/C++ → Include Paths
 *             添加 ..\user\inc （本工程已添加）
 */
#include "alarm_sys.h"
#include "buzzer.h"
#include "led.h"

/* ====================== blink_led（阻塞式） ====================== */

/**
 * @brief  阻塞式 LED 闪烁：点亮 → 延时 → 熄灭 → 延时，重复 times 次。
 * @note   此函数使用 HAL_Delay()，调用期间 CPU 被占用，不返回。
 */
void blink_led(uint8_t led_num, uint16_t times, uint32_t delay_ms)
{
    uint16_t i;

    if (led_num == 0U || led_num > LED_COUNT)
    {
        return; /* 非法编号，直接返回 */
    }

    for (i = 0U; i < times; i++)
    {
        led_on(led_num);
        HAL_Delay(delay_ms);
        led_off(led_num);
        HAL_Delay(delay_ms);
    }
}

/* ====================== beep（阻塞式） ====================== */

/**
 * @brief  阻塞式蜂鸣器鸣叫。
 */
void beep(uint32_t beep_ms)
{
    buzzer_on();
    HAL_Delay(beep_ms);
    buzzer_off();
}

/* ====================== alarm_sys_func（非阻塞状态机） ====================== */

/**
 * @brief  报警系统主循环函数（非阻塞状态机）。
 *
 *         设计思路：
 *         - 使用 static 变量保存状态，每次调用根据 HAL_GetTick() 判断
 *           是否到了执行下一步的时刻。
 *         - 流水灯逻辑（RUNNING / ALARM 共用）：
 *           LED1 闪 1 次 → LED2 闪 2 次 → LED3 闪 3 次 → LED4 闪 4 次 → 循环
 *         - 报警模式额外加入蜂鸣器翻转，与流水灯互不阻塞。
 *
 * @param  mode  当前系统模式（见 sys_mode_t）
 */
void alarm_sys_func(sys_mode_t mode)
{
    /* -------- 静态状态变量（跨调用保持） -------- */
    static uint8_t  s_current_led   = 1U;   /* 当前操作的是哪颗 LED（1~4） */
    static uint8_t  s_led_on        = 0U;   /* LED 当前状态：0=灭, 1=亮   */
    static uint16_t s_blink_cnt     = 0U;   /* 当前 LED 已完成闪烁次数     */
    static uint32_t s_last_led_tick = 0U;   /* 上次 LED 状态切换时刻       */

    static uint8_t  s_buzzer_on     = 0U;   /* 蜂鸣器当前开关状态          */
    static uint32_t s_last_buzz_tick = 0U;  /* 上次蜂鸣器翻转时刻          */

    uint32_t now       = HAL_GetTick();
    uint32_t led_delay = DELAY_MS;  /* 默认：运行模式延时      */

    /* ==================== 模式分支 ==================== */
    switch (mode)
    {
    /* ---- 待机：关闭一切，重置状态 ---- */
    case MODE_STANDBY:
        led_off(1); led_off(2); led_off(3); led_off(4);
        buzzer_off();

        s_current_led   = 1U;
        s_led_on        = 0U;
        s_blink_cnt     = 0U;
        s_buzzer_on     = 0U;
        return; /* 待机模式直接返回，不做任何事 */

    /* ---- 运行：只做流水灯 ---- */
    case MODE_RUNNING:
        buzzer_off();          /* 确保蜂鸣器关闭       */
        s_buzzer_on = 0U;
        break;                 /* 继续执行流水灯逻辑   */

    /* ---- 报警：流水灯加速 + 蜂鸣器间歇 ---- */
    case MODE_ALARM:
        led_delay = DELAY_MS_FAST;

        /* 蜂鸣器间隔翻转（非阻塞） */
        if ((now - s_last_buzz_tick) >= BEEP_INTERVAL_MS)
        {
            s_last_buzz_tick = now;
            s_buzzer_on = !s_buzzer_on;       /* 翻转开关状态 */

            if (s_buzzer_on)  buzzer_on();
            else              buzzer_off();
        }
        break;

    default:
        return;
    }

    /* ==================== 非阻塞流水灯（RUNNING & ALARM 共用） ==================== */

    /* 检查是否到了切换 LED 状态的时刻 */
    if ((now - s_last_led_tick) < led_delay)
    {
        return; /* 还没到时间，什么都不做 */
    }

    s_last_led_tick = now;

    if (!s_led_on)
    {
        /* 当前 LED 处于熄灭状态 → 点亮它 */
        led_on(s_current_led);
        s_led_on = 1U;
    }
    else
    {
        /* 当前 LED 处于点亮状态 → 熄灭它，计一次闪烁 */
        led_off(s_current_led);
        s_led_on = 0U;
        s_blink_cnt++;

        /* 当前 LED 闪烁次数够了？ */
        if (s_blink_cnt >= s_current_led)
        {
            s_blink_cnt = 0U;

            /* 切换到下一颗 LED */
            s_current_led++;
            if (s_current_led > LED_COUNT)
            {
                s_current_led = 1U; /* LED4 之后回到 LED1 */
            }
        }
    }
}
