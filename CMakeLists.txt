idf_component_register(SRCS "src/lc_ld2410s.cpp" 
                            "src/lc_ld2410s_generic_config.cpp" 
                            "src/lc_ld2410s_serial_number.cpp" 
                            "src/lc_ld2410s_common.cpp" 
                            "src/lc_ld2410s_threshold_trigger.cpp"
                            "src/lc_ld2410s_threshold_hold.cpp"
                            "src/lc_ld2410s_threshold_auto.cpp"
                            
                    PRIV_REQUIRES esp_driver_uart esp_driver_gpio esp_event
                    INCLUDE_DIRS "include")
