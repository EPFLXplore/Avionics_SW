/*
 * transport_layer.h
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

#ifndef INC_TRANSPORT_LAYER_H_
#define INC_TRANSPORT_LAYER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool   cubemx_transport_open(struct uxrCustomTransport * transport);
bool   cubemx_transport_close(struct uxrCustomTransport * transport);
size_t cubemx_transport_write(struct uxrCustomTransport* transport,const uint8_t * buf, size_t len, uint8_t * err);
size_t cubemx_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err);
void   cubemx_transport_flush(void);

#ifdef __cplusplus
}
#endif


#endif /* INC_TRANSPORT_LAYER_H_ */
