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

/* SocketCAN driver */

#include <stdint.h>
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/socket.h>
#include <bits/socket.h>

#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/drivers/can.h>

#ifdef CSP_HAVE_LIBSOCKETCAN
#include <libsocketcan.h>
#endif

struct csp_socketcan {
	pthread_t rx_thread; /** RX thread handle */
	int socket;          /** SocketCAN socket handle */
};

static void * csp_socketcan_rx_thread(void * param)
{
	struct can_frame frame;
	int nbytes;
	csp_iface_t *ifc = param;
	struct csp_socketcan *sc = csp_if_get_driver_priv(ifc);

	while (1) {
		/* Read CAN frame */
		nbytes = read(sc->socket, &frame, sizeof(frame));
		if (nbytes < 0) {
			csp_log_error("read: %s", strerror(errno));
			continue;
		}

		if (nbytes != sizeof(frame)) {
			csp_log_warn("Read incomplete CAN frame");
			continue;
		}

		/* Frame type */
		if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG) || !(frame.can_id & CAN_EFF_FLAG)) {
			/* Drop error and remote frames */
			csp_log_warn("Discarding ERR/RTR/SFF frame");
			continue;
		}

		/* Strip flags */
		frame.can_id &= CAN_EFF_MASK;

		/* Call RX callback */
		csp_can_rx_frame(ifc, (csp_can_frame_t *)&frame, NULL);
	}

	/* We should never reach this point */
	pthread_exit(NULL);
}

static int csp_socketcan_send(csp_iface_t *ifc, uint32_t id, uint8_t data[], uint8_t dlc)
{
	struct can_frame frame;
	int i, tries = 0;
	struct csp_socketcan *sc = csp_if_get_driver_priv(ifc);

	if (dlc > 8)
		return -1;

	/* Copy identifier */
	frame.can_id = id | CAN_EFF_FLAG;

	/* Copy data to frame */
	for (i = 0; i < dlc; i++)
		frame.data[i] = data[i];

	/* Set DLC */
	frame.can_dlc = dlc;

	/* Send frame */
	while (write(sc->socket, &frame, sizeof(frame)) != sizeof(frame)) {
		if (++tries < 1000 && errno == ENOBUFS) {
			/* Wait 10 ms and try again */
			usleep(10000);
		} else {
			csp_log_error("write: %s", strerror(errno));
			break;
		}
	}

	return 0;
}

static int csp_socketcan_setup(csp_iface_t *ifc, uint32_t id, uint32_t mask)
{
	struct csp_socketcan *sc = csp_if_get_driver_priv(ifc);

	/* Set promiscuous mode */
	if (mask) {
		struct can_filter filter;
		filter.can_id   = id;
		filter.can_mask = mask;
		if (setsockopt(sc->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
			csp_log_error("setsockopt: %s", strerror(errno));
			return -1;
		}
	}

	return 0;
}

struct csp_can_driver driver = {
	.setup = csp_socketcan_setup,
	.send = csp_socketcan_send,
};

csp_iface_t *csp_socketcan_init(const char *dev, uint32_t bitrate)
{
	struct ifreq ifr;
	struct sockaddr_can addr;
	csp_iface_t *ifc;
	struct csp_socketcan *sc;

#ifdef CSP_HAVE_LIBSOCKETCAN
	/* Set interface up */
	if (bitrate > 0) {
		can_do_stop(dev);
		can_set_bitrate(dev, bitrate);
		can_do_start(dev);
	}
#endif

	/* Allocate driver private data */
	sc = malloc(sizeof(*sc));
	if (!sc) {
		csp_log_error("failed to allocate socketcan driver data");
		return NULL;
	}

	/* Create socket */
	if ((sc->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		csp_log_error("socket: %s", strerror(errno));
		return NULL;
	}

	/* Locate interface */
	strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
	if (ioctl(sc->socket, SIOCGIFINDEX, &ifr) < 0) {
		csp_log_error("ioctl: %s", strerror(errno));
		return NULL;
	}

	/* Bind the socket to CAN interface */
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(sc->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		csp_log_error("bind: %s", strerror(errno));
		return NULL;
	}

	/* Allocate CAN interface */
	ifc = csp_can_alloc(&driver);
	if (!ifc) {
		csp_log_error("failed to allocate CAN interface");
		return NULL;
	}

	csp_if_set_driver_priv(ifc, sc);

	/* Create receive thread */
	if (pthread_create(&sc->rx_thread, NULL, csp_socketcan_rx_thread, ifc) != 0) {
		csp_log_error("pthread_create: %s", strerror(errno));
		return NULL;
	}

	/* Setup CAN ID mask */
	csp_can_setup(ifc);

	/* Regsiter interface */
	csp_if_register(ifc);

	return ifc;
}
