idf_component_register(SRCS "main.c"
                            "audio_config_app/audio_config_app.c"
                       PRIV_REQUIRES 
                                    xn_web_wifi_manger 
                                    xn_audio_manager 
                                    xn_audio_prompt
                                    xn_lottie_manager
                       INCLUDE_DIRS "" 
                                    "audio_config_app"
                       )
