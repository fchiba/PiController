/*
 * Thread-safe gamepad report sharing between cores
 * Core 1 (Bluetooth) sets the report
 * Core 0 (USB) gets the report
 */

#ifndef _REPORT_H_
#define _REPORT_H_

#include "switch_descriptors.h"

// Set the gamepad report (called from Core 1 - Bluetooth)
void set_global_gamepad_report(const SwitchOutReport *report);

// Get the gamepad report (called from Core 0 - USB)
void get_global_gamepad_report(SwitchOutReport *report);

#endif /* _REPORT_H_ */
