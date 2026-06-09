#ifndef LC_LD2410S_H__
#define LC_LD2410S_H__

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define BUF_SIZE (80)
#define MAX_ATTEMPT 5
#define ATTEMPT_DELAY 500
#define RX_DELAY 1000

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "lc_ld2410s_events.h"

#ifdef __cplusplus
extern "C" {
#endif

class lc_ld2410s {
public:
    enum mode_type {minimal, standart}; // Режим вывода данных Minimal / Standart

    struct firmware_version {           // Версия прошивки модуля
        uint16_t MAJOR;                 // Mojor version
        uint16_t MINOR;                 // Minor version
        uint16_t PATCH;                 // Patch
    };

    struct general_configuration {
        double   FARTHEST_DISTANCE;     // 0.7 - 8.4 meters, step 0.7
        double   NEAREST_DISTANCE;      // 0.0 - 7.7 meters, step 0.7
        uint32_t UNATTENDED_DELAY;      // 10 - 255 seconds
        double   FREQUENCY_STATUS;      // 0.5Hz - 8.0Hz
        double   FREQUENCY_DISTANCE;    // 0.5Hz - 8.0Hz
        bool     RESPOND_SPEED;         // 0 - Normal, 1 - Fast
    };

    struct task_args {                  // Структура аргументов с ссылками передаваемая в цикл задачи 
        uart_port_t *UART_PORT;         // Ссылка на свойство номера порта UART
        mode_type   *MODE_OUTPUT;       // Ссылка на свойство режима вывода данных
        uint8_t     *MOTION;            // Ссылка на свойство состояния движения
        uint16_t    *DISTANCE;          // Ссылка на свойство дистанции до объекта
        uint32_t    *ENERGY;            // Ссылка на свойство энергетической матрицы
    };

    lc_ld2410s(uart_port_t PORT, uint16_t RX, uint16_t TX, int BAUDRATE, uint16_t STACK_SIZE); // Конструктор
    
    esp_err_t init();                                           // Инициализация объекта
    
    uint8_t  getMotion() const;                                 // Получение сведений о состоянии движения
    uint16_t getDistance() const;                               // Получение сведений о дистанции до объекта
    void getEnergyMatrix(uint32_t *matrix) const;               // Получение энергитической матрицы

    esp_err_t setModeOutput(lc_ld2410s::mode_type mode);        // Установка режима вывод данных Minimal / Standart
    lc_ld2410s::mode_type getModeOutput() const;                // Получение текущего режима вывод данных Minimal / Standart
    
    firmware_version getFirmwareVersion();                      // Получение версии прошивки модуля

    general_configuration getConfiguration();                   // Получение общей конфигурации модуля
    esp_err_t setConfiguration(general_configuration config);   // Установка общей конфигурации модуля

    esp_err_t getSerialNumber(char* number, const uint8_t &sizeData);       // Получение серийного номера модуля
    esp_err_t setSerialNumber(char* number);                                // Установка серийного номера модуля

    esp_err_t getTriggerThreshold(uint8_t* data, const uint8_t &sizeData);  // Получение данных о переключателях заслонок
    esp_err_t setTriggerThreshold(uint8_t* trig, const uint8_t &trig_size); // Установка новых уровней переключателей заслонок

    esp_err_t getHoldThreshold(uint8_t* data, const uint8_t &sizeData);     // Получение данных о задержках заслонок
    esp_err_t setHoldThreshold(uint8_t* data, const uint8_t &sizeData);     // Установка новых уровней задержек заслонок

    esp_err_t autoUpdateThreshold(uint8_t* data, const uint8_t &sizeData);  // Automatically update the threshold command

    ~lc_ld2410s();                                              // Деструктор

private:
    uart_port_t _UART_PORT;                                     // Порт UART
    uint16_t    _UART_RX;                                       // RX pin 
    uint16_t    _UART_TX;                                       // TX pin
    uint16_t    _UART_STACK_SIZE;                               // Размер стека данных
    int         _UART_BAUDRATE;                                 // Скорость порта
    
    uint16_t _PROTOCOL_VERSION;                                 // Версия протокола
    uint16_t _BUFFER_SIZE;                                      // Размер буфера
    char _SERIAL_NUMBER[9];                                     // Серийный номер модуля
    firmware_version _FIRMWARE_VERION;                          // Версия прошивки
    general_configuration _CONFIGURATION;                       // Общая конфигурация модуля

    mode_type _MODE_OUTPUT = mode_type::minimal;                // Minimal or Standart mode

    uint8_t  _MOTION = 0;                                       // Состояние движения
    uint16_t _DISTANCE = 0;                                     // Дистанция до объекта
    uint32_t _ENERGY[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};    // Элергитическая матрица

    uint8_t _trigger_thresholds[16];                            // Уровни переключателей заслонок
    uint8_t _holding_thresholds[16];                            // Уровни задержек заслонок
    
    esp_err_t serial_write(const uint8_t *buff, uint8_t size);  // Запись данных в UART
    bool buffcmp(const uint8_t* data, const uint8_t &delta, const uint8_t* tamplate, const uint8_t &tSize);
    esp_err_t enableConfigurationCommand();                     // Включение Конфигурационного режима
    esp_err_t endConfigurationCommand();                        // Выход из конфигурационного режима
    esp_err_t readFirmwareVersion();                            // Загрузка версии прошивки из модуля в свойство
    esp_err_t readSerialNumber();                               // Загрузка серийного номера из модуля в свойство
    esp_err_t readGenericParameterCommands();                   // Загрузка общих параметров из модуля в свойство
    esp_err_t readTriggerThreshold();                           // Загрузка данных об уровнях переключателей заслонок
    esp_err_t readHoldThreshold();                              // Загрузка данных об уровнях задержек заслонок

    /* Event source task related definitions */
    ESP_EVENT_DEFINE_BASE(LD2410_EVENTS);
    const char *TAG = "LD2410S";    

};

#ifdef __cplusplus
}
#endif

#endif //LC_LD2410S_H__
