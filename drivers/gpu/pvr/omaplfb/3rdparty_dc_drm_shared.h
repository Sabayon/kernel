/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef __3RDPARTY_DC_DRM_SHARED_H__
#define __3RDPARTY_DC_DRM_SHARED_H__
#if defined(SUPPORT_DRI_DRM)

#define	PVR_DRM_DISP_CMD_ENTER_VT	1
#define	PVR_DRM_DISP_CMD_LEAVE_VT	2

#define	PVR_DRM_DISP_CMD_ON		3
#define	PVR_DRM_DISP_CMD_STANDBY	4
#define	PVR_DRM_DISP_CMD_SUSPEND	5
#define	PVR_DRM_DISP_CMD_OFF		6

#define	PVR_DRM_DISP_ARG_CMD		0
#define	PVR_DRM_DISP_ARG_DEV		1
#define	PVR_DRM_DISP_NUM_ARGS		2

#endif	
#endif 

