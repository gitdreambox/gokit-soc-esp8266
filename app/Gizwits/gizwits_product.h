/**
************************************************************
* @file         gizwits_product.h
* @brief        对应gizwits_product.c的头文件(包含产品软硬件版本定义)
* @author       Gizwits
* @date         2016-09-05
* @version      V03010201
* @copyright    Gizwits
* 
* @note         机智云.只为智能硬件而生
*               Gizwits Smart Cloud  for Smart Products
*               链接|增值ֵ|开放|中立|安全|自有|自由|生态
*               www.gizwits.com
*
***********************************************************/
#ifndef _GIZWITS_PRODUCT_H_
#define _GIZWITS_PRODUCT_H_

#include "osapi.h"
#include <stdint.h>
#include "gizwits_protocol.h"

/**
* MCU硬件版本号
*/
#define HARDWARE_VERSION                        "03000001"
/**
* MCU软件版本号
*/
#define SOFTWARE_VERSION                        "03000201"
/**
* gagent小版本号，用于OTA升级
*/
#define SDK_VERSION                             "13"

#ifndef SOFTWARE_VERSION
    #error "no define SOFTWARE_VERSION"
#endif

#ifndef HARDWARE_VERSION
    #error "no define HARDWARE_VERSION"
#endif

#define SIG_UPGRADE_DATA 0x01

/**@name Gizwits 用户API接口
* @{
*/
int8_t gizwitsEventProcess(eventInfo_t *info, uint8_t *data, uint32_t len);
/**@} */

#endif
