#ifndef _GIZWITS_PRODUCT_H_
#define _GIZWITS_PRODUCT_H_

#include "osapi.h"
#include <stdint.h>
#include "gizwits_protocol.h"

#define HARDWARE_VERSION                        "03000001"
#define SOFTWARE_VERSION                        "03000200"

#ifndef SOFTWARE_VERSION
    #error "no define SOFTWARE_VERSION"
#endif

#ifndef HARDWARE_VERSION
    #error "no define HARDWARE_VERSION"
#endif

void gizEventProcess(event_info_t * info, uint8_t * data);

#endif
