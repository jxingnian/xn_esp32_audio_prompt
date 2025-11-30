/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-11-22 13:43:50
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-11-30 18:14:39
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

/* 应用日志标签 */
static const char *TAG = "app";

/**
 * @brief 音频提示音演示任务
 * 
 * 每秒播放一次蜂鸣提示音（如果已加载）
 * 
 * @param arg 任务参数（未使用）
 */
static void audio_prompt_demo_task(void *arg)
{
    (void)arg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // 每 1 秒触发一次
        if (audio_prompt_is_loaded(AUDIO_PROMPT_BEEP)) {
            audio_prompt_play(AUDIO_PROMPT_BEEP);  // 播放蜂鸣音
        }
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

    /* 创建提示音演示任务（核心 0，优先级 5） */
    xTaskCreatePinnedToCore(audio_prompt_demo_task,
                            "prompt_demo",
                            4096,
                            NULL,
                            5,
                            NULL,
                            0);

    /* 初始化完成，此工程作为音效组件 Demo：每秒播放一次提示音 */
    ESP_LOGI(TAG, "audio prompt demo ready: beep every second");
}
