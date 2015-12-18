/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _CAN_H_
#define _CAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>

/* The can_frame_t and can_id_t types intentionally matches the
 * can_frame struct and can_id types in include/linux/can.h
 */

/** CAN Frame */
typedef struct {
	/** 32 bit CAN identifier */
	uint32_t id;
	/** Data Length Code */
	uint8_t dlc;
	/**< Frame Data - 0 to 8 bytes */
	union __attribute__((aligned(8))) {
		uint8_t data[8];
		uint16_t data16[4];
		uint32_t data32[2];
	};
} csp_can_frame_t;

int csp_can_rx_frame(csp_iface_t *ifc, csp_can_frame_t *frame, CSP_BASE_TYPE *task_woken);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CAN_H_ */
