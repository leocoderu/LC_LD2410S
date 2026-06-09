#include "lc_ld2410s.h"

// Преобразование двух байт в число Int
uint16_t twoByteToInt(uint8_t firstByte, uint8_t secondByte) {
    return (int16_t)(secondByte << 8) + firstByte;
}

// Получение сведений о двичении из свойства класса
uint8_t lc_ld2410s::getMotion() const {
    return _MOTION;
}

// Получение сведений о дистанции из свойства класса
uint16_t lc_ld2410s::getDistance() const {
    return _DISTANCE;
}

// Получение сведений об энергетической матрице из свойства класса
void lc_ld2410s::getEnergyMatrix(uint32_t *matrix) const {
    memcpy(matrix, _ENERGY, 64); // 16 * 4
}

// Сравнение фреймов памяти
bool framecmp(const uint8_t* data, const uint8_t &delta, const uint8_t* tamplate, const uint8_t &tSize) {
    bool result = false;                                    // Результат сравнения, по умолчанию false
    uint8_t* buff = (uint8_t*) malloc(tSize);               // Создаем буфер для фрейма, размером с шаблон
    
    memcpy(buff, data + delta, tSize);                      // Копируем в буфер сегмент данных, из общего массива данных с смещением, размером с шаблон
    if (memcmp(buff, tamplate, tSize) == 0) result = true;  // Сравниваем сегмент данных с шаблоном, если данные равны, результат true
    
    free(buff);                                             // Освобождаем выделенную под буфер память
    return result;                                          // Возвращаем результат
}

// return:  -1 - Phrase dosn't find, 0 or more - position of phrase
int16_t getPosition(
    const uint8_t* buff,            // data     |   data 
    const uint8_t  &bSize,          // 100      |   100
    const uint8_t* bPhrase,         // {6E}     |   {F4, F3, F2. F1}
    const uint8_t  &bLen,           // 1        |   4
    const uint8_t* ePhrase,         // {62}     |   {F8, F7, F6, F5}     
    const uint8_t  &eLen,           // 1        |   4
    const uint8_t  &delta,          // 4        |   72
    const uint8_t  &bPos = 0        // 0
) {
    int8_t res = -1;
    for (uint8_t i = bPos; i < (bSize - delta); i++) {       
        if (framecmp(buff, i, bPhrase, bLen)  && framecmp(buff, i + delta, ePhrase, eLen)) {
            res = i;
            break;
        }
    }
    return res;
}

void parse_minimal(uint8_t* _buffer, uint8_t _size, uint8_t &out_motion, uint16_t &out_distance) {
    uint8_t  pos = 0;                   // Position of cursor in massive
    uint8_t  qBlock = 1;                // Quantity of blocks found
    uint16_t distance = 0;              // Distance to object 
    uint8_t  motion = 0;                // Presents motions
    const uint8_t bPhrase[] = {0x6E};
    const uint8_t ePhrase[] = {0x62};
    const uint8_t delta = 5-1;          // Общая длинна фрейма минус длинна конечной фразы
    
    while (pos < _size) {
        int8_t res = getPosition(_buffer, _size, bPhrase, sizeof(bPhrase), ePhrase, sizeof(ePhrase), delta, pos);
        if (res >= 0) {
            if (_buffer[res+1] > 0) motion = _buffer[res+1];    // Что бы он на 0 не возвращался
            distance += (static_cast<uint16_t>(_buffer[res+3]) << 8) | _buffer[res+2];
            qBlock++;
            pos = res + delta;
        }
        pos++;
    }

    out_motion = motion;
    out_distance = (uint16_t)(distance/qBlock); 

    //esp_event_post(LD2410_EVENTS, LD2410_EVENT_NEW_DATA, NULL, 0, portMAX_DELAY);
}

void parse_standart(uint8_t* _buffer, uint8_t _size, uint8_t &out_motion, uint16_t &out_distance, uint32_t *out_energy) {
    uint8_t  pos = 0;            // Position of cursor in massive
    uint8_t  qBlock = 1;         // Quantity of blocks found
    uint16_t distance = 0;       // Distance to object 
    uint32_t energy[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    uint8_t  motion = 0;         // Presents motions
    const uint8_t bPhrase[] = {0xF4, 0xF3, 0xF2, 0xF1};
    const uint8_t ePhrase[] = {0xF8, 0xF7, 0xF6, 0xF5};
    const uint8_t delta = 80-4;  // Общая длинна фрейма минус длинна начальной и конечной фразы

    //ESP_LOG_BUFFER_HEXDUMP("PARSE_STANDART", _buffer, _size, ESP_LOG_WARN);
    
    while (pos < _size) {
        int8_t res = getPosition(_buffer, _size, bPhrase, sizeof(bPhrase), ePhrase, sizeof(ePhrase), delta, pos);
        if (res >= 0) {
            if (_buffer[res + 7] > 0) motion = _buffer[res + 7];    // Что бы он на 0 не возвращался
            distance += (static_cast<uint16_t>(_buffer[res + 9]) << 8) | _buffer[res+8];
            
            if (qBlock == 1) {                              // Если это первый или единственный фрейм в выборке, то заполняем энергитическую матрицу
                for (uint8_t n = 0; n < 16; n++) {
                    energy[n] = (static_cast<uint32_t>(_buffer[res + (n * 4) + 15]) << 24)
                        | (static_cast<uint32_t>(_buffer[res + (n * 4) + 14]) << 16)
                        | (static_cast<uint32_t>(_buffer[res + (n * 4) + 13]) << 8)
                        | _buffer[res + (n * 4) + 12];
                }
            } else {                                        // Иначе усредняем значения матрицы по гейтам
                for (uint8_t n = 0; n < 16; n++) {
                    uint32_t _en = (static_cast<uint32_t>(_buffer[res + (n * 4) + 15]) << 24)
                        | (static_cast<uint32_t>(_buffer[res + (n * 4) + 14]) << 16)
                        | (static_cast<uint32_t>(_buffer[res + (n * 4) + 13]) << 8)
                        | _buffer[res + (n * 4) + 12];

                    energy[n] = (uint32_t)((energy[n] + _en) / 2);
                }    
            }

            qBlock++;
            pos = res + delta;
        }
        pos++;
    }

    out_motion = motion;
    out_distance = (uint16_t)(distance/qBlock); 
    memcpy(out_energy, energy, sizeof(energy));

    //esp_event_post(LD2410_EVENTS, LD2410_EVENT_NEW_DATA, NULL, 0, portMAX_DELAY);
}

void loop(void* args) {
    const uint8_t _SIZE = 255;
    uint8_t* data = (uint8_t*) malloc(_SIZE);

    lc_ld2410s::task_args *taskArgs = (lc_ld2410s::task_args *)args;
    uart_port_t* port = taskArgs->UART_PORT;
    lc_ld2410s::mode_type* _mode_output = taskArgs->MODE_OUTPUT;
    uint16_t* _distance = taskArgs->DISTANCE;
    uint8_t* _motion = taskArgs->MOTION;
    uint32_t* _energy = taskArgs->ENERGY;

    while (1) {
        const int rxBytes = uart_read_bytes(*port, data, _SIZE, 100 / portTICK_PERIOD_MS);
        
        if (*_mode_output == lc_ld2410s::mode_type::minimal)  parse_minimal(data, rxBytes, *_motion, *_distance);
        if (*_mode_output == lc_ld2410s::mode_type::standart) parse_standart(data, rxBytes, *_motion, *_distance, _energy);
        
        vTaskDelay(CONFIG_SCAN_PERIOD / portTICK_PERIOD_MS);
    }
    free(data);
}

lc_ld2410s::lc_ld2410s(uart_port_t PORT, uint16_t RX, uint16_t TX, int BAUDRATE, uint16_t STACK_SIZE) {
    ESP_LOGI(TAG, "Create instance HLK-LD2410S");

    _UART_PORT = PORT;
    _UART_RX = RX;
    _UART_TX = TX;
    _UART_STACK_SIZE = STACK_SIZE;
    _UART_BAUDRATE = BAUDRATE;
}

esp_err_t lc_ld2410s::init() {
    ESP_LOGI(TAG, "Initialization HLK-LD2410S");

    // Configure parameters of an UART driver,
    // communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = _UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {
            .allow_pd = 0,
            .backup_before_sleep = 0,
        },
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install((uart_port_t)_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin((uart_port_t)_UART_PORT, _UART_TX, _UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    enableConfigurationCommand();
    readFirmwareVersion();
    readSerialNumber();
    readGenericParameterCommands();
    readTriggerThreshold();
    readHoldThreshold();
    endConfigurationCommand();

    setModeOutput(lc_ld2410s::standart);
    
    task_args taskArgs = {
        UART_PORT: &_UART_PORT,
        MODE_OUTPUT: &_MODE_OUTPUT,
        MOTION: &_MOTION,
        DISTANCE: &_DISTANCE,
        ENERGY: (uint32_t *)_ENERGY
    };
    
    xTaskCreate(loop, "ld2410_loop", _UART_STACK_SIZE, &taskArgs, 10, NULL);
    return ESP_OK;
}

lc_ld2410s::~lc_ld2410s() {}

//ESP_LOGI(TAG, "Read answer has length: %d", rxBytes);
//ESP_LOG_BUFFER_HEXDUMP(TAG, ack, rxBytes, ESP_LOG_INFO);

// void lc_ld2410s::floatToBytes(const float& data, uint8_t *res) {
//     uint32_t v = (uint32_t)(data / 0.7);
//     ESP_LOGW(TAG, "uint32_t v: %d", v);    
//     lc_ld2410s::uint32ToBytes(v, res);
// }

// void uint32ToBytes(const uint32_t& data, uint8_t *res) {
//     res[0] = (uint8_t)(data << 24 >> 24);
//     res[1] = (uint8_t)(data << 16 >> 24);
//     res[2] = (uint8_t)(data << 8 >> 24);
//     res[3] = (uint8_t)(data >> 24);
// }

// Сравнение двух чисел float
// bool floatcmp(float &a, float &b) {
//     char _a[10];
//     char _b[10];
//     sprintf(_a, "%4.4f", a); _a[9] = '\0';
//     sprintf(_b, "%4.4f", b); _b[9] = '\0';
//     ESP_LOGW("FLOATCMP", "%4.4f == %4.4f", _a, _b);
//     return strcmp(_a, _b) == 0;
// }

// bool buffcmp(const uint8_t* buff1, const uint8_t* buff2) {
//     return memcmp(buff1, buff2, 16) == 0;
// }