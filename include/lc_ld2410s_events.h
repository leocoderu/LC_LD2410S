#ifndef LC_LD2410S_EVENTS_H__
#define LC_LD2410S_EVENTS_H__

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declare an event base
ESP_EVENT_DECLARE_BASE(LD2410_EVENTS);      // declaration of the LD2410 events family

enum {                                      // declaration of the specific events under the LD2410 event family
    LD2410_EVENT_ALERT,
    LD2410_EVENT_DETECT,
    LD2410_EVENT_NEW_DATA,
};

#ifdef __cplusplus
}
#endif

#endif // LC_LD2410S_EVENTS_H__