#include "lc_ld2410s.h"

// Запись в UART
esp_err_t lc_ld2410s::serial_write(const uint8_t *buff, uint8_t size) {
    static const uint8_t head[4] = {0xFD, 0xFC, 0xFB, 0xFA};
    static const uint8_t tail[4] = {0x04, 0x03, 0x02, 0x01};

    uint8_t result[sizeof(head) + size + sizeof(tail)];

    memcpy(result, head, sizeof(head));
    memcpy(result + sizeof(head), buff, size);
    memcpy(result + size + sizeof(head), tail, sizeof(tail));

    int len = uart_write_bytes(_UART_PORT, result, size + sizeof(head) + sizeof(tail));

    if (len != (size + sizeof(head) + sizeof(tail))) {
        ESP_LOGE(TAG, "Error write to UART");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Сравнение выделенных участков памяти
bool lc_ld2410s::buffcmp(const uint8_t* data, const uint8_t &dSize, const uint8_t* tamplate, const uint8_t &tSize) {
    if (dSize < tSize) return false;
    
    bool result = false;                                    // Результат сравнения, по умолчанию false    
    uint8_t* buff = (uint8_t*) malloc(tSize);               // Создаем буфер для фрейма, размером с шаблон
    
    memcpy(buff, data, tSize);                      // Копируем в буфер сегмент данных, из общего массива данных с смещением, размером с шаблон
    if (memcmp(buff, tamplate, tSize) == 0) result = true;  // Сравниваем сегмент данных с шаблоном, если данные равны, результат true
    
    free(buff);                                             // Освобождаем выделенную под буфер память
    return result;                                          // Возвращаем результат
}

// Вход в Конфигурационный режим
esp_err_t lc_ld2410s::enableConfigurationCommand() {
    const uint8_t cmd[] = {0x04, 0x00, 0xFF, 0x00, 0x01, 0x00};         // Подготавливаем запрос на запись
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xFF, 0x01, 0x00, 0x00};
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                _PROTOCOL_VERSION = (static_cast<uint16_t>(ack[11]) << 8) | ack[10];    // Обновляем свойство класса
                _BUFFER_SIZE = (static_cast<uint16_t>(ack[13]) << 8) | ack[12];         // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            } 
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }    
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Configuration mode is enabled");
                   } else { ESP_LOGE(TAG, "Error enabling Сonfiguration mode");} // Выводим в LOG результат
    return result;
}

// Выход из Конфигурационного режима
esp_err_t lc_ld2410s::endConfigurationCommand() {
    const uint8_t cmd[] = {0x02, 0x00, 0xFE, 0x00};                     // Подготавливаем запрос на запись
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFE, 0x01, 0x00};
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            } 
        }    
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Configuration mode is disabled");
                   } else { ESP_LOGE(TAG, "Error disabling the Configuration mode");}  // Выводим в LOG результат
    return result;
}

// Пердаем структуру с данными из локальных переменных
lc_ld2410s::firmware_version lc_ld2410s::getFirmwareVersion() {
    return _FIRMWARE_VERION;
}

// Чтение версии прошивки
esp_err_t lc_ld2410s::readFirmwareVersion() {
    const uint8_t cmd[] = {0x02, 0x00, 0x00, 0x00};                     // Подготавливаем запрос на запись
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x0E, 0x00, 0x00, 0x01}; // Response to the request
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                _FIRMWARE_VERION.MAJOR = (static_cast<uint16_t>(ack[15]) << 8) | ack[14]; // Обновляем свойство класса
                _FIRMWARE_VERION.MINOR = (static_cast<uint16_t>(ack[17]) << 8) | ack[16]; // Обновляем свойство класса
                _FIRMWARE_VERION.PATCH = (static_cast<uint16_t>(ack[19]) << 8) | ack[18]; // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            } 
        } 
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Firmware version data: %d.%d.%d", _FIRMWARE_VERION.MAJOR, _FIRMWARE_VERION.MINOR, _FIRMWARE_VERION.PATCH);
                   } else { ESP_LOGE(TAG, "Error getting Firmware version");} // Выводим в LOG результат  
    return result;
}

// Установка режима вывода
esp_err_t lc_ld2410s::setModeOutput(lc_ld2410s::mode_type mode) {
    enableConfigurationCommand();                                       // Входим в Конфигурационный режим

    // Подготавливаем запрос на запись
    const uint8_t command[] = {0x08, 0x00, 0x7A, 0x00};    
    uint8_t parameter[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (mode == lc_ld2410s::mode_type::standart) {
        uint8_t standart[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
        memcpy(parameter, standart, 6);
    }
    uint8_t wbuf[10];
    memcpy(wbuf, command, 4);                                           // Объединяем запрос с данными
    memcpy(wbuf + 4, parameter, 6);

    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x7A, 0x01, 0x00, 0x00}; // Response to the request    
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(wbuf, sizeof(wbuf)) == ESP_OK) {               // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом          
                _MODE_OUTPUT = mode;                                    // Обновляем свойство класса
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }  
        } 
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) {
        if (mode == lc_ld2410s::mode_type::minimal) { ESP_LOGI(TAG, "Set Minimal output mode");
                                             } else { ESP_LOGI(TAG, "Set Standart output mode");}
    } else { ESP_LOGE(TAG, "Error setting the output mode");}           // Выводим в LOG результат
    
    endConfigurationCommand();                                          // Выходим из Конфигурационного режима
    return result;
}

// Получение режима вывода из свойства класса
lc_ld2410s::mode_type lc_ld2410s::getModeOutput() const {
    return _MODE_OUTPUT;                                                // Выводим значение свойства класса
}