#include "lc_ld2410s.h"

// Читаем данные о задержке заслонок из модуля
esp_err_t lc_ld2410s::readHoldThreshold() {                             // Подготавливаем запрос на запись
    const uint8_t cmd[] = {0x22, 0x00, 0x77, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x0B, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x0E, 0x00, 0x0F, 0x00};
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x44, 0x00, 0x77, 0x01, 0x00, 0x00}; // Response to the request
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                for (int i = 0; i < 16; i++) 
                    _holding_thresholds[i] = ack[((i * 4) + 10)];       // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "The Hold Threshold parameters command were read successfully");
                   } else { ESP_LOGE(TAG, "Error getting Hold Threshold parameters");} // Выводим в LOG результат
    return result;
}

// Устанавливаем новые значения задержек заслонок
esp_err_t lc_ld2410s::setHoldThreshold(uint8_t* data, const uint8_t &sizeData) {
    // Проверяем переданные данные в метод
    if (sizeData < 16) {                                                // Если принимающий буфер меньше 16, выходим с ошибкой
        ESP_LOGE(TAG, "Buffer of data has a small size");
        return ESP_FAIL;
    }
    
    if (memcmp(_holding_thresholds, data, sizeData) == 0) {             // Если идентичные данные уже установлены, выходим без изменений 
        ESP_LOGI(TAG, "That Configuration already set");
        return ESP_OK;
    }

    enableConfigurationCommand();                                       // Входим в Конфигурационный режим

    // Подготавливаем запрос на запись
    uint8_t wbuf[100];
    const uint8_t command[] = {0x62, 0x00, 0x76, 0x00};
    memcpy(wbuf, command, 4);
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t res[2] = {0x00, 0x00};
        const uint8_t gate_tamplate[] = {0x00, 0x00, 0x00};
        res[0] = i;
        memcpy(wbuf + ((i * 6) + 4), res, 2);
        wbuf[(i + 1) * 6] = data[i];
        memcpy(wbuf + (((i + 1) * 6) + 1), gate_tamplate, 3);    
    }
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x76, 0x01, 0x00, 0x00}; // Response to the request  
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(wbuf, sizeof(wbuf)) == ESP_OK) {               // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                memcpy(_holding_thresholds, data, 16);                  // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            } 
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Holder Threshold parameters set successful");
                   } else { ESP_LOGE(TAG, "Error setting Holder Threshold parameters");} // Выводим в LOG результат
    
    endConfigurationCommand();                                          // Выходим из Конфигурационного режима
    return result;
}

// Получаем значения задержек заслонок из свойства класса
esp_err_t lc_ld2410s::getHoldThreshold(uint8_t* data, const uint8_t &sizeData) {
    if (sizeData < 16) {                                                // Если принимающий буфер меньше 16, выходим с ошибкой
        ESP_LOGE(TAG, "Buffer of data has a small size");
        return ESP_FAIL;
    }
    memcpy(data, _holding_thresholds, 16);                              // Выводим значение свойства класса
    return ESP_OK;
}