#include "lc_ld2410s.h"

// Чтение Серийного номера
esp_err_t lc_ld2410s::readSerialNumber() {
    const uint8_t cmd[] = {0x02, 0x00, 0x11, 0x00};                     // Подготавливаем запрос на запись
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x0E, 0x00, 0x11, 0x01, 0x00, 0x00}; // Response to the request
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(cmd, sizeof(cmd)) == ESP_OK) {                 // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом
                for (uint8_t i = 0; i < 8; i++) 
                    _SERIAL_NUMBER[i] = static_cast<char>(ack[12 + i]); // Обновляем свойство класса
                _SERIAL_NUMBER[8] = '\0';
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Serial Number: %s", _SERIAL_NUMBER);
                   } else { ESP_LOGE(TAG, "Error getting Serial number");} // Выводим в LOG результат
    return result;
}

// Установка нового Серийного номера
esp_err_t lc_ld2410s::setSerialNumber(char* number) {
    // Проверяем переданные данные в метод    
    if (strlen(number) != 8) {                  // Если длинна серийного номера отличается от 8, то выходим с ошибкой
        ESP_LOGE(TAG, "Serial number must contain 8 symbols");
        return ESP_FAIL;
    }

    if (strcmp(_SERIAL_NUMBER, number) == 0) {  // Если уже установлен токой же серийный номер выводим сообщение и выходим
        ESP_LOGI(TAG, "This Serial number already set");
        return ESP_OK;
    }

    enableConfigurationCommand();                                       // Входим в Конфигурационный режим

    // Подготавливаем запрос на запись
    const uint8_t command[] = {0x0C, 0x00, 0x10, 0x00, 0x08, 0x00};
    uint8_t wbuf[sizeof(command) + strlen(number)];
    memcpy(wbuf, command, sizeof(command));                             // Объединяем запрос с данными
    memcpy(wbuf + sizeof(command), number, strlen(number));
    const uint8_t answer[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x10, 0x01, 0x00, 0x00}; // Response to the request
    
    esp_err_t result = ESP_FAIL;                                        // Переменная результата

    uint8_t* ack = (uint8_t*) malloc(BUF_SIZE);                         // Выделяем память под буфер
    for (uint8_t attempt = 0; attempt < MAX_ATTEMPT; attempt++) {       // Цикл попыток получения данных
        if (serial_write(wbuf, sizeof(wbuf)) == ESP_OK) {               // Записываем запрос в UART
            const int rxBytes = uart_read_bytes(_UART_PORT, ack, BUF_SIZE, RX_DELAY / portTICK_PERIOD_MS); // Читаем ответ
            if (buffcmp(ack, rxBytes, answer, sizeof(answer))) {        // Если ответ больше нуля и совпадает с протоколом                    
                strcpy(_SERIAL_NUMBER, number);                         // Обновляем свойство класса
                _SERIAL_NUMBER[8] = '\0';                 
                result = ESP_OK;                                        // Результат OK
                break;                                                  // Выход из цикла
            }
        }
        vTaskDelay(ATTEMPT_DELAY / portTICK_PERIOD_MS);                 // Задержка между попытками
    }
    free(ack);                                                          // Освобождаем выделенную память

    if (result == ESP_OK) { ESP_LOGI(TAG, "Serial Number: %s", _SERIAL_NUMBER);
                   } else { ESP_LOGE(TAG, "Error setting the Serial number");} // Выводим в LOG результат
    
    endConfigurationCommand();                                          // Выходим из Конфигурационного режима
    return result;
}

// Получение Серийного номера из свойства класса
esp_err_t lc_ld2410s::getSerialNumber(char* number, const uint8_t &sizeData) {
    if (sizeData < 9) {                                                 // Если принимающий буфер меньше 9, выходим с ошибкой
        ESP_LOGE(TAG, "Buffer of data has a small size");
        return ESP_FAIL;
    }
    strcpy(number, _SERIAL_NUMBER);                                     // Выводим значение свойства класса
    return ESP_OK;
}