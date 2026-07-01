/*
 * Bridge.h
 *
 *  The C <-> C++ boundary. Generated C (main.c, usbd_cdc_if.c) calls these;
 *  Bridge.cpp implements them on top of the C++ System / CdcTransport.
 */

#ifndef BRIDGE_H_
#define BRIDGE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bring up the application threads (called once from StartDefaultTask). */
void BridgeSystemInit(void);

/** 2-bit board id from the PB4/PB5 straps (0..3). Read + cached on first call;
 *  used both to pick the profile (System) and to build the USB serial (usbd_desc). */
uint8_t Board_MasterId(void);

/** CDC OUT packet received (called from CDC_Receive_FS, USB IRQ). */
void Cdc_onRxISR(uint8_t* buf, uint32_t len);

/** CDC IN transfer complete (called from CDC_TransmitCplt_FS, USB IRQ). */
void Cdc_onTxCpltISR(void);

/** Build the USB serial string "NOVA<board-id>" into buf (called from
 *  USBD_CDC_SerialStrDescriptor in usbd_desc.c). */
void Bridge_UsbSerial(uint8_t* buf, uint16_t* length);

#ifdef __cplusplus
}
#endif

#endif /* BRIDGE_H_ */
