#include "lc_ld2410s.h"

// Сравнение двух чисел double
bool dblcmp(double a, double b) {
    char _a[10];
    char _b[10];
    sprintf(_a, "%4.4f", a); _a[9] = '\0';
    sprintf(_b, "%4.4f", b); _b[9] = '\0';
    return strcmp(_a, _b) == 0;
}

// Преобразование числа uint32_t в массив из четырех байт 
void uint32ToBytes(const uint32_t& data, uint8_t *res) {
    res[0] = (uint8_t)(data << 24 >> 24);
    res[1] = (uint8_t)(data << 16 >> 24);
    res[2] = (uint8_t)(data << 8 >> 24);
    res[3] = (uint8_t)(data >> 24);
}

// Сравнение двух объектов конфигурации
bool compareConfigurations(lc_ld2410s::general_configuration config1, lc_ld2410s::general_configuration config2) {
    return ((dblcmp(config1.FARTHEST_DISTANCE, config2.FARTHEST_DISTANCE))
         && (dblcmp(config1.NEAREST_DISTANCE, config2.NEAREST_DISTANCE))
         && (config1.UNATTENDED_DELAY == config2.UNATTENDED_DELAY)
         && (dblcmp(config1.FREQUENCY_DISTANCE, config2.FREQUENCY_DISTANCE))
         && (dblcmp(config1.FREQUENCY_STATUS, config2.FREQUENCY_STATUS))
         && (config1.RESPOND_SPEED == config2.RESPOND_SPEED));
}

// Читаем общие конфигурвционные данные из модуля
esp_err_t lc_ld2410s::readGenericParameterCommands() {
    const uint8_t cmd[] = {0x0E, 0x00, 0x71, 0x00, 0x05, 0x00, 0x0A, 0x00, 0x06, 0x00, 0x02, 0x00, 0x0C, 0x00, 0x0B, 0x00}; // Подготавливаем запрос на запись
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x1C, 0x00, 0x71, 0x01, 0x00, 0x00}; // Response to the request
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом                  
                    // Обновляем свойство класса
                _CONFIGURATION.FARTHEST_DISTANCE = (float)(((static_cast<uint32_t>(ack[13]) << 24) | (static_cast<uint32_t>(ack[12]) << 16) | (static_cast<uint32_t>(ack[11]) << 8) | ack[10]) * 0.7);
                _CONFIGURATION.NEAREST_DISTANCE  = (float)(((static_cast<uint32_t>(ack[17]) << 24) | (static_cast<uint32_t>(ack[16]) << 16) | (static_cast<uint32_t>(ack[15]) << 8) | ack[14]) * 0.7);
                _CONFIGURATION.UNATTENDED_DELAY  = (static_cast<uint32_t>(ack[21]) << 24) | (static_cast<uint32_t>(ack[20]) << 16) | (static_cast<uint32_t>(ack[19]) << 8) | ack[18];
                _CONFIGURATION.FREQUENCY_STATUS  = (float)(((static_cast<uint32_t>(ack[25]) << 24) | (static_cast<uint32_t>(ack[24]) << 16) | (static_cast<uint32_t>(ack[23]) << 8) | ack[22]) / 10);
                _CONFIGURATION.FREQUENCY_DISTANCE  = (float)(((static_cast<uint32_t>(ack[29]) << 24) | (static_cast<uint32_t>(ack[28]) << 16) | (static_cast<uint32_t>(ack[27]) << 8) | ack[26]) / 10);
                _CONFIGURATION.RESPOND_SPEED  = ((static_cast<uint32_t>(ack[33]) << 24) | (static_cast<uint32_t>(ack[32]) << 16) | (static_cast<uint32_t>(ack[31]) << 8) | ack[30]) == 10;

                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками    
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "The general configuration were read successfully");
                   } else { ESP_LOGE(TAG, "Error getting General Configuration");} // Выводим в LOG результат
    return result;
}

// Устанавливаем новую конфигурацию
esp_err_t lc_ld2410s::setConfiguration(lc_ld2410s::general_configuration config) {
    // Если уже установлена токая же конфигурация выводим сообщение и выходим, нет смысла переписывать его
    if (compareConfigurations(_CONFIGURATION, config)) {
        ESP_LOGI(TAG, "That Configuration already set");
        return ESP_OK;
    }

    enableConfigurationCommand();                                       // Входим в Конфигурационный режим

    // Подготавливаем запрос на запись
    uint8_t res[4];
    uint8_t wbuf[40];
    const uint8_t command[]        = {0x26, 0x00, 0x70, 0x00};
    const uint8_t far_distance[]   = {0x05, 0x00};  // 0x0C - 8.4 m
    const uint8_t near_distance[]  = {0x0A, 0x00};  // 0x00 - 0 m
    const uint8_t time_delay[]     = {0x06, 0x00};  // 0x0A - 10 sec
    const uint8_t freq_trig_rate[] = {0x02, 0x00};  // 0x50 - 8.0Hz
    const uint8_t freq_main_rate[] = {0x0C, 0x00};  // 0x50 - 8.0Hz
    const uint8_t responce_rate[]  = {0x0B, 0x00};  // 0x05 - Normal, 0x0A - Fast
    
    memcpy(wbuf, command, 4);

    memcpy(wbuf + 4, far_distance, 2);
    uint32ToBytes((uint32_t)(config.FARTHEST_DISTANCE / 0.7), res);
    memcpy(wbuf + 6, res, 4);

    memcpy(wbuf + 10, near_distance, 2);
    uint32ToBytes((uint32_t)(config.NEAREST_DISTANCE / 0.7), res);
    memcpy(wbuf + 12, res, 4);

    memcpy(wbuf + 16, time_delay, 2);
    uint32ToBytes(config.UNATTENDED_DELAY, res);
    memcpy(wbuf + 18, res, 4);

    memcpy(wbuf + 22, freq_trig_rate, 2);
    uint32ToBytes((uint32_t)(config.FREQUENCY_STATUS * 10), res);
    memcpy(wbuf + 24, res, 4);

    memcpy(wbuf + 28, freq_main_rate, 2);
    uint32ToBytes((uint32_t)(config.FREQUENCY_DISTANCE * 10), res);
    memcpy(wbuf + 30, res, 4);
    
    memcpy(wbuf + 34, responce_rate, 2);
    if (config.RESPOND_SPEED) {
        uint32ToBytes(10, res); // Fast
    } else {
        uint32ToBytes(5, res);  // Normal
    }
    memcpy(wbuf + 36, res, 4);
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x70, 0x01, 0x00, 0x00}; // Response to the request

    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(wbuf, sizeof(wbuf)) == ESP_OK) {               // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом                          
                _CONFIGURATION = config;                                // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }
        } 
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Configuration set successful"); // Выводим в LOG результат
                   } else { ESP_LOGE(TAG, "Error setting General Configuration");} // Выводим в LOG результат
    
    endConfigurationCommand();                                          // Выходим из Конфигурационного режима
    return result;
}

// Получаем конфигурацию из свойства класса
lc_ld2410s::general_configuration lc_ld2410s::getConfiguration() {
    return _CONFIGURATION;                                              // Выводим значение свойства класса
}
