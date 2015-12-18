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

#ifndef CSP_IFLIST_H_
#define CSP_IFLIST_H_

/**
 * Add interface to list
 * @param ifc Pointer to interface to add
 */
int csp_if_register(csp_iface_t *ifc);

/**
 * Lookup interface by name
 * @param name String with interface name
 * @return Pointer to interface or NULL if not found
 */
csp_iface_t * csp_if_get_by_name(char *name);

/**
 * Print list of interfaces to stdout
 */
void csp_iflist_print(void);

/**
 * - * Allocate new interface
 *   - */
csp_iface_t *csp_if_alloc(const char *name);

/**
 * Register allocated interface
 */
int csp_if_register(csp_iface_t *ifc);

/**
 * Set interface private data
 */
static inline void csp_if_set_iface_priv(csp_iface_t *ifc, void *priv)
{
	ifc->priv = priv;
}

/**
 * Get interface private data
 */
static inline void *csp_if_get_iface_priv(csp_iface_t *ifc)
{
	return ifc->priv;
}

/**
 * Set interface driver private data
 */
static inline void csp_if_set_driver_priv(csp_iface_t *ifc, void *priv)
{
	ifc->driver = priv;
}

/**
 * Get interface driver private data
 */
static inline void *csp_if_get_driver_priv(csp_iface_t *ifc)
{
	return ifc->driver;
}

#endif /* CSP_IFLIST_H_ */
