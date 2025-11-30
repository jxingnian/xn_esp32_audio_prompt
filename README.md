# xn_esp32_audio_prompt

ESP32-S3 音频演示工程，集成：

- 语音唤醒 / VAD / 录音回放一体的音频管理器 `xn_audio_manager`
- Web WiFi 配网与网页资源 `xn_web_wifi_manger`
- 基于 SPIFFS 的音效播放组件 `xn_audio_prompt`

本项目在原始“录音回环播放”示例基础上，新增了**每隔 1 秒播放一次提示音效**的 Demo，音效数据存放在独立的 SPIFFS 分区中。

---

## 目录结构概览

- `main/`
  - `main.c`：应用入口，初始化音频管理器，启动录音回放逻辑，并创建 1 秒音效播放任务
  - `audio_config_app.c`：封装音频硬件参数、唤醒词/VAD/AFE 等配置
- `components/xn_audio_manager/`：统一封装录音、播放、唤醒词和 VAD 的音频管理器
- `components/xn_audio_prompt/`
  - `src/audio_prompt.c`：音效播放实现（预加载到 PSRAM）
  - `prompt_spiffs/*.pcm`：各类提示音 PCM 文件
  - `README.md`：音效组件的详细说明
- `components/xn_web_wifi_manger/`
  - `wifi_spiffs/`：网页资源（`index.html`, `app.js`, `app.css`）
- `partitions.csv`：分区表配置（包含 SPIFFS 分区）

---

## 分区表与 SPIFFS 配置

项目使用 CSV 分区表 `partitions.csv`，核心相关分区如下：

```csv
# Name,        Type, SubType, Offset,  Size, Flags
nvs,           data, nvs,     ,        0x6000,
phy_init,      data, phy,     ,        0x1000,
otadata,       data, ota,     ,        0x2000,
ota_0,         app,  ota_0,   ,        3M,
ota_1,         app,  ota_1,   ,        3M,
model,         data, spiffs,  ,        2M,
wifi_spiffs,   data, spiffs,  ,        1M,
prompt_spiffs, data, spiffs,  ,        1M,
```

- **`wifi_spiffs` 分区**
  - 用途：存放 Web 配网页面的静态资源
  - 数据来源：`components/xn_web_wifi_manger/wifi_spiffs/`
  - CMake：`components/xn_web_wifi_manger/CMakeLists.txt` 中有：
    ```cmake
    spiffs_create_partition_image(wifi_spiffs wifi_spiffs FLASH_IN_PROJECT)
    ```

- **`prompt_spiffs` 分区**
  - 用途：存放音效 PCM 文件
  - 数据来源：`components/xn_audio_prompt/prompt_spiffs/` 下的 `*.pcm`
  - CMake：`components/xn_audio_prompt/CMakeLists.txt` 中有：
    ```cmake
    spiffs_create_partition_image(prompt_spiffs prompt_spiffs FLASH_IN_PROJECT)
    ```
  - 运行时由 `audio_prompt` 组件挂载：
    - 分区标签：`"prompt_spiffs"`
    - 挂载路径：`"/prompt_spiffs"`

编译时，ESP-IDF 会自动将上述目录打包为 SPIFFS 镜像，烧录到对应分区中，无需额外手动命令。

---

## 1 秒间隔音效播放 Demo

在 `main/main.c` 中：

1. 通过 `audio_config_app_build()` 组装音频配置
2. 初始化并启动音频管理器：
   - `audio_manager_init()`
   - `audio_manager_start()`
   - `audio_manager_start_playback()`
3. 初始化音效模块：
   - `audio_prompt_init()`
   - 会挂载 `prompt_spiffs` 分区，并预加载 `prompt_spiffs/*.pcm` 至 PSRAM 缓存
4. 创建一个 FreeRTOS 任务 `audio_prompt_demo_task`：
   - 周期为 `vTaskDelay(pdMS_TO_TICKS(1000))`
   - 每 1 秒检测 `AUDIO_PROMPT_BEEP` 是否已加载：`audio_prompt_is_loaded(AUDIO_PROMPT_BEEP)`
   - 若已加载，则调用 `audio_prompt_play(AUDIO_PROMPT_BEEP)` 播放短促蜂鸣音

因此，设备上电后，在串口看到音频初始化完成日志后，扬声器会**每秒播放一次提示音效**（前提是 `beep.pcm` 成功打包进 `prompt_spiffs`）。

原有的**录音回环播放**逻辑（VAD 结束后将录音数据回放到扬声器）依然保留，与音效播放 Demo 可以同时工作。

如需了解音效组件更多能力（自定义音效、格式要求等），请参考：

- `components/xn_audio_prompt/README.md`

---

## 编译与烧录

1. 准备环境
   - 安装并配置好 ESP-IDF（支持 CMake 的版本，如 4.x/5.x）
   - 建议使用 ESP32-S3 开发板（工程音频 BSP 针对 S3 设计）

2. 进入工程目录
   ```bash
   cd xn_esp32_audio_prompt
   ```

3. 选择目标芯片（如 ESP32-S3）
   ```bash
   idf.py set-target esp32s3
   ```

4. 编译
   ```bash
   idf.py build
   ```

5. 烧录并打开串口监视
   ```bash
   idf.py flash monitor
   ```

烧录完成并复位后，你应能在串口日志中看到：

- 音频管理器初始化成功相关日志
- 音效模块挂载和加载 PCM 文件的日志

同时，扬声器会每秒播放一次蜂鸣提示音，配合原有的录音回放功能，可以验证整套音频通路是否正常。

