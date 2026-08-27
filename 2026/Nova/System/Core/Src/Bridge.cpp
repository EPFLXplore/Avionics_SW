/*
 * Bridge.cpp
 *
 *  The C <-> C++ boundary. ALL the work lives here; the generated USER CODE
 *  sections only *call* these functions (they contain no logic themselves).
 */

#include "Bridge.h"

#include "System.h"
#include "Transport.h"
#include "WS2812Driver.h"

#include "main.h"        // HAL, GPIOB, GPIO_PIN_4/5
#include "usb_device.h"  // MX_USB_Device_Init, USBD_HandleTypeDef
#include "usbd_cdc.h"    // USBD_CDC_SetRxBuffer / USBD_CDC_ReceivePacket
#include "usbd_core.h"   // USBD_GetString


/* BRIDGE: self explanatory name: bridges C code generated from STM Cube IDE and C++ application code
 * Simple idea: need to call your code from main or any of the C code? Put it here and put the function
 * on the USER_CODE command blocks so it lives between ioc generations
 * */



/* Defined in usb_device.c (no header declares it). */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Called from main.c  (HAL_TIM_PWM_PulseFinishedCallback, DMA IRQ). That
 * callback fires for EVERY PWM channel that finishes a transfer, so the driver
 * filters it; this is only the C linkage. Returns 1 when the frame was the
 * strip's. */
int WS2812_FrameCompleteISR(TIM_HandleTypeDef* htim) {
	return WS2812Driver::onFrameCompleteISR(htim) ? 1 : 0;
}

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

/* Called from usbd_cdc_if.c (CDC_Init_FS) every time the host configures the
 * interface - so on first enumeration AND on every re-enumeration after a
 * replug. Drops the previous link's in-flight TX so a transfer that never
 * completed cannot latch the transport busy forever. USBD_CDC_Init resets its
 * own TxState immediately after calling us, so both halves clear together. */
void Cdc_onCdcInit(void) {
	CdcTransport::dispatchReset();
}

/* Called from usbd_desc.c  (USBD_CDC_SerialStrDescriptor). Builds the USB serial
 * string "NOVA<board-id>" into buf, so udev can name each master's port. */
void Bridge_UsbSerial(uint8_t* buf, uint16_t* length) {
	static char novaSerial[] = "NOVA0";
	novaSerial[4] = (char)('0' + (Board_MasterId() & 0x3));
	USBD_GetString((uint8_t*)novaSerial, buf, length);
}

// 2-bit board id coming from the switch. Used also for the Nova<id> names of /dev/tty*
static int boardMasterId = -1;   // -1 = not yet latched

/* Debugger handles: watch these in Live Expressions to see what the pins
 * actually did. boardStrapRaw is the IDR bits (bit0 = PB4, bit1 = PB5) taken
 * on the settled read, i.e. the same sample boardMasterId is built from. */
volatile uint8_t boardStrapRaw = 0xFF;

static uint8_t Board_ReadStraps(void) {
	GPIO_InitTypeDef gpio = {};
	gpio.Pin  = GPIO_PIN_4 | GPIO_PIN_5;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLDOWN;          // the board straps each line high or low
	HAL_GPIO_Init(GPIOB, &gpio);
	for (volatile uint32_t i = 0; i < 3000; ++i) { __NOP(); } // ~20-60 us settle
	const uint8_t b0 = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_SET) ? 1 : 0;
	const uint8_t b1 = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET) ? 1 : 0;
	boardStrapRaw = (uint8_t)((b1 << 1) | b0);
	return boardStrapRaw;
}

/* Called once from main(), before MX_USB_Device_Init. Two passes: the first
 * takes the pins off SPI3's alternate function and turns the pull-down on, the
 * second reads them once the NJTRST pull-up's charge has actually decayed.
 * HAL_Delay is legal here - this runs in main(), before the kernel, and never
 * from the USB IRQ. */
void Board_LatchMasterId(void) {
	if (boardMasterId >= 0) return;
	(void)Board_ReadStraps();   // claim the pins from AF6, enable the pull-down
	HAL_Delay(2);               // full RC settle, not a NOP-count guess
	boardMasterId = (int)Board_ReadStraps();
}

uint8_t Board_MasterId(void) {
	/* Fallback only: if something asks before main() latched, sample inline.
	 * The NOP spin (not HAL_Delay) is what makes that safe from an ISR. */
	if (boardMasterId < 0) boardMasterId = (int)Board_ReadStraps();
	return (uint8_t)boardMasterId;
}
