#pragma once

#ifndef HTTPD_H_

#define HTTPD_H_

#include "stm32l1xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "w5500.h"

#endif /* HTTPD_H_ */

typedef struct http_sock_prop {
	volatile uint8_t data_stat; //data transmit status
	volatile uint32_t data_size; //data to transmit length
	volatile uint16_t last_data_part_size; //last part of data to transmit length
	volatile uint16_t cnt_data_part; //parts count of data to transmit
	volatile uint16_t cnt_rem_data_part; //count of remaining parts of data to transmit
	volatile uint32_t total_count_bytes; //transmitted file bytes count
	volatile uint8_t http_doc; //file variant to transmit 
	volatile uint8_t prt_tp; //file variant to transmit
	char fname[20]; //file name
} http_sock_prop_ptr;

// HTTP file variants
#define EXISTING_HTML 0
#define E404_HTML 1
#define EXISTING_JPG 2
#define EXISTING_ICO 3