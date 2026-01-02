/*
 * USB HID Task for Nintendo Switch output
 * Runs on Core 0
 */

#ifndef _USB_TASK_H_
#define _USB_TASK_H_

// Main USB task - runs on Core 0, does not return
void usb_core_task(void);

#endif /* _USB_TASK_H_ */
