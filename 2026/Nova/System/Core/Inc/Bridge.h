/*
 * Bridge.h
 *
 *  The C <-> C++ boundary. Generated C (main.c, usbd_cdc_if.c) calls these;
 *  Bridge.cpp implements them on top of the C++ System / CdcTransport.
 */

#pragma once
#include <stdint.h>
#include "stm32g4xx_hal.h"   // TIM_HandleTypeDef, for the timer ISR hook below

#ifdef __cplusplus
extern "C" {
#endif

/** Bring up the application threads (called once from StartDefaultTask). */
void BridgeSystemInit(void);

/** Sample the PB4/PB5 straps and cache the board id. Call ONCE from main(),
 *  after MX_GPIO_Init/MX_SPI3_Init (which own those pins as SPI3 AF) and before
 *  MX_USB_Device_Init. Doing it here is what keeps the sample out of the USB
 *  IRQ: after this, Board_MasterId() only ever returns the cache. */
void Board_LatchMasterId(void);

/** 2-bit board id from the PB4/PB5 straps (0..3). Read + cached on first call;
 *  used both to pick the profile (System) and to build the USB serial (usbd_desc). */
uint8_t Board_MasterId(void);

/** CDC OUT packet received (called from CDC_Receive_FS, USB IRQ). Re-arms the
 *  endpoint only if the transport took the packet; if it refused, the endpoint
 *  is left un-armed so the host NAKs and retries. */
void Cdc_onRxISR(uint8_t* buf, uint32_t len);

/** Re-arm the CDC OUT endpoint after a packet was refused for want of ring
 *  space (called from CdcTransport::read once it has drained room). Safe to
 *  call only when a refusal is actually outstanding - arming twice would hand
 *  the stack a buffer it is already filling. */
void Cdc_ArmRx(void);

/** CDC IN transfer complete (called from CDC_TransmitCplt_FS, USB IRQ). */
void Cdc_onTxCpltISR(void);

/** CDC interface (re)configured by the host (called from CDC_Init_FS). Drops any
 *  in-flight TX left over from the previous link. */
void Cdc_onCdcInit(void);

/** Build the USB serial string "NOVA<board-id>" into buf (called from
 *  USBD_CDC_SerialStrDescriptor in usbd_desc.c). */
void Bridge_UsbSerial(uint8_t* buf, uint16_t* length);

/** WS2812 frame finished (called from HAL_TIM_PWM_PulseFinishedCallback in
 *  main.c, DMA IRQ). That callback fires for EVERY PWM channel that completes a
 *  transfer, so it has to be filtered - and only the driver knows which handle
 *  it was given, since that follows LED_STRIP_SLOT and main.c cannot see C++.
 *  Returns 1 when the interrupt belonged to the strip, 0 otherwise.
 *  Bridge.cpp holds only the C linkage; the filter itself is
 *  WS2812Driver::onFrameCompleteISR(), which is where the active instance
 *  lives. */
int WS2812_FrameCompleteISR(TIM_HandleTypeDef* htim);

#ifdef __cplusplus
}
#endif

