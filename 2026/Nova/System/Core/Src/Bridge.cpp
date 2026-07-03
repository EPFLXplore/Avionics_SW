/*
 * Bridge.cpp
 *
 *  The C <-> C++ boundary. ALL the work lives here; the generated USER CODE
 *  sections only *call* these functions (they contain no logic themselves).
 */

#include "Bridge.h"

#include "System.h"
#include "Transport.hpp"

#include "main.h"        // HAL, GPIOB, GPIO_PIN_4/5
#include "usb_device.h"  // MX_USB_Device_Init, USBD_HandleTypeDef
#include "usbd_cdc.h"    // USBD_CDC_SetRxBuffer / USBD_CDC_ReceivePacket
#include "usbd_core.h"   // USBD_GetString

/* Defined in usb_device.c (no header declares it). */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Called from main.c  (StartDefaultTask, right after MX_USB_Device_Init). */
void BridgeSystemInit(void) {
	System::init();
}

/* Called from usbd_cdc_if.c  (CDC_Receive_FS, USB IRQ) when a CDC OUT packet
 * arrives. Copies the bytes into the transport, then re-arms reception. */
void Cdc_onRxISR(uint8_t* buf, uint32_t len) {
	CdcTransport::dispatchRxISR(buf, static_cast<uint16_t>(len));
	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, buf);
	USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

/* Called from usbd_cdc_if.c  (CDC_TransmitCplt_FS, USB IRQ) when an IN transfer
 * completes. Chains the next queued TX packet. */
void Cdc_onTxCpltISR(void) {
	CdcTransport::dispatchTxCpltISR();
}

/* Called from usbd_desc.c  (USBD_CDC_SerialStrDescriptor). Builds the USB serial
 * string "NOVA<board-id>" into buf, so udev can name each master's port. */
void Bridge_UsbSerial(uint8_t* buf, uint16_t* length) {
	static char nova_serial[] = "NOVA0";
	nova_serial[4] = (char)('0' + (Board_MasterId() & 0x3));
	USBD_GetString((uint8_t*)nova_serial, buf, length);
}

/* Called from System.cpp (System::init) and from Bridge_UsbSerial (above).
 * 2-bit board id strapped on PB4 (bit0) / PB5 (bit1); read + configured once and
 * cached, so calls from the USB IRQ never re-run HAL_GPIO_Init. PB4 is NJTRST ->
 * the debug interface must be SWD for it to be a free GPIO.
 *
 * PB4's NJTRST pull-up is active from reset until the pin is reconfigured, so a
 * "low" strap starts charged HIGH. Internal pull-down + a settle spin make the
 * read deterministic: a high strap overrides the ~40k pull-down, an absent or
 * weak low strap no longer floats at whatever charge the pull-up left. The spin
 * (not HAL_Delay: this can run in the USB IRQ) covers the RC decay to the real
 * strap level. */
uint8_t Board_MasterId(void) {
	static int cached = -1;
	if (cached < 0) {
		GPIO_InitTypeDef g = {};
		g.Pin  = GPIO_PIN_4 | GPIO_PIN_5;
		g.Mode = GPIO_MODE_INPUT;
		g.Pull = GPIO_PULLDOWN;          // the board straps each line high or low
		HAL_GPIO_Init(GPIOB, &g);
		for (volatile uint32_t i = 0; i < 3000; ++i) { __NOP(); } // ~20-60 us settle
		const uint8_t b0 = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_SET) ? 1 : 0;
		const uint8_t b1 = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET) ? 1 : 0;
		cached = (int)((b1 << 1) | b0);
	}
	return (uint8_t)cached;
}
