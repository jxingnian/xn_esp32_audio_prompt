/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-11-22 13:43:50
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-11-30 18:29:20
 * @FilePath: \xn_esp32_audio_prompt\main\main.c
 * @Description:
 *  - 初始化 WiFi 管理模块，确保联网能力正常
 *  - 初始化音频管理器，注册状态机回调
 *  - 当唤醒 + 讲话结束后，将录音回环到扬声器
 *
 * 运行步骤：
 *  1. 供电后等待 "loopback test ready" 日志
 *  2. 说出唤醒词（默认"小鸭小鸭"）或按键唤醒
 *  3. 在 VAD 窗口讲话，结束后会立即播放刚才的录音
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "audio_manager.h"
#include "audio_config_app.h"
#include "audio_prompt.h"
#include "bsp_touch_spd2010.h"
#include "xn_lottie_manager.h"

/* 应用日志标签 */
static const char *TAG = "app";

/**
 * @brief 触摸触发的音效 + Lottie 演示任务
 * 
 * 行为：
 *  - 轮询 SPD2010 触摸屏，当检测到从“未按下”到“按下”的边沿事件时：
 *      1. 打断当前音频播放
 *      2. 播放一次提示音效（AUDIO_PROMPT_BEEP）
 *      3. 播放一次青蛙 Lottie 动画（/lottie/frog.json）
 * 
 * @param arg 任务参数（未使用）
 */
static void touch_prompt_task(void *arg)
{
    (void)arg;

    bool last_pressed = false;

    uint16_t touch_x[TOUCH_MAX_POINTS];
    uint16_t touch_y[TOUCH_MAX_POINTS];
    uint8_t touch_count = 0;

    while (1) {
        bool pressed = Touch_Get_xy_Official(touch_x, touch_y, NULL, &touch_count, TOUCH_MAX_POINTS);

        if (pressed && touch_count > 0) {
            if (!last_pressed) {
                // 检测到新的触摸按下事件
                ESP_LOGI(TAG, "touch down: (%d, %d)", touch_x[0], touch_y[0]);

                // 1. 打断当前音频播放
                audio_prompt_stop();
                audio_manager_clear_playback_buffer();

                // 2. 播放一次提示音效
                if (audio_prompt_is_loaded(AUDIO_PROMPT_BEEP)) {
                    audio_prompt_play(AUDIO_PROMPT_BEEP);
                }

                // 3. 播放青蛙 Lottie 动画
                lottie_manager_stop_anim(-1);  // 停止当前所有动画
                lottie_manager_play("/lottie/frog.json", 256, 256);
            }
            last_pressed = true;
        } else {
            last_pressed = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // 触摸轮询与防抖
    }
}

/**
 * @brief 应用程序主入口
 * 
 * 初始化流程：
 * 1. WiFi 管理模块（已注释）
 * 2. 音频管理器（麦克风、扬声器、唤醒词、VAD）
 * 3. 注册录音回调和事件回调
 * 4. 启动音频管理器和播放任务
 * 5. 初始化音频提示音模块
 * 6. 创建提示音演示任务
 */
void app_main(void)
{
    /* 构建音频管理器配置：此处不使用事件和回环录音，仅做播放通路初始化 */
    audio_mgr_config_t audio_cfg;
    audio_config_app_build(&audio_cfg, NULL, NULL);

    /* 初始化音频管理器 */
    ESP_LOGI(TAG, "init audio manager");
    ESP_ERROR_CHECK(audio_manager_init(&audio_cfg));

    /* 设置播放音量为 100% */
    audio_manager_set_volume(100);

    /* 启动音频管理器与播放任务（用于驱动扬声器输出） */
    ESP_ERROR_CHECK(audio_manager_start());
    ESP_ERROR_CHECK(audio_manager_start_playback());

    /* 初始化音频提示音模块（挂载 SPIFFS 并预加载音效） */
    ESP_ERROR_CHECK(audio_prompt_init());

    /* 初始化 Lottie 管理器（包含 LVGL + SPD2010 屏幕与触摸） */
    ESP_ERROR_CHECK(xn_lottie_manager_init(NULL));

    /* 创建触摸触发音效与青蛙动画演示任务（核心 0，优先级 5） */
    xTaskCreatePinnedToCore(touch_prompt_task,
                            "touch_prompt",
                            4096,
                            NULL,
                            5,
                            NULL,
                            0);

    /* 初始化完成：点击屏幕会打断旧音效并播放新音效 + 青蛙 Lottie 动画 */
    ESP_LOGI(TAG, "touch prompt demo ready: tap screen to play beep + frog animation");
}
