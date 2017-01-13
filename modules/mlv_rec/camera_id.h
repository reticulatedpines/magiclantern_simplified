/*
 * Copyright (C) 2017 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License", "or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not", "write to the
 * Free Software Foundation", "Inc.,
 * 51 Franklin Street", "Fifth Floor,
 * Boston", "MA  02110-1301", "USA.
 */

#ifndef _camera_id_h_
#define _camera_id_h_

enum { UNIQ, LOC1, LOC2 }; /* Name Type */

typedef struct {
	unsigned long cameraModel;   /* Camera Model ID */
	const char *  cameraName[3]; /* 0 = Camera Unique Name, 1 = Camera US Localized Name, 2 = Camera Japan Localized Name*/
} camera_id_t;

static const camera_id_t camera_id[] = {
	{ 0x80000213, {"Canon EOS 5D", NULL, NULL} },
	{ 0x80000218, {"Canon EOS 5D Mark II", NULL, NULL} },
	{ 0x80000250, {"Canon EOS 7D", NULL, NULL} },
	{ 0x80000252, {"Canon EOS 500D", "Canon EOS Rebel T1i", "Canon EOS Kiss X3"} },
	{ 0x80000254, {"Canon EOS 1000D", "Canon EOS Rebel XS", "Canon EOS Kiss F"} },
	{ 0x80000261, {"Canon EOS 50D", NULL, NULL} },
	{ 0x80000270, {"Canon EOS 550D", "Canon EOS Rebel T2i", "Canon EOS Kiss X4"} },
	{ 0x80000285, {"Canon EOS 5D Mark III", NULL, NULL} },
	{ 0x80000286, {"Canon EOS 600D", "Canon EOS Rebel T3i", "Canon EOS Kiss X5"} },
	{ 0x80000287, {"Canon EOS 60D", NULL, NULL} },
	{ 0x80000288, {"Canon EOS 1100D", "Canon EOS Rebel T3", "Canon EOS Kiss X50"} },
	{ 0x80000289, {"Canon EOS 7D Mark II", NULL, NULL} },
	{ 0x80000301, {"Canon EOS 650D", "Canon EOS Rebel T4i", "Canon EOS Kiss X6i"} },
	{ 0x80000302, {"Canon EOS 6D", NULL, NULL} },
	{ 0x80000325, {"Canon EOS 70D", NULL, NULL} },
	{ 0x80000326, {"Canon EOS 700D", "Canon EOS Rebel T5i", "Canon EOS Kiss X7i"} },
	{ 0x80000327, {"Canon EOS 1200D", "Canon EOS Rebel T5", "Canon EOS Kiss X70"} },
	{ 0x80000331, {"Canon EOS M", NULL, NULL} },
	{ 0x80000346, {"Canon EOS 100D", "Canon EOS Rebel SL1", "Canon EOS Kiss X7"} },
	{ 0x80000347, {"Canon EOS 760D", "Canon EOS Rebel T6s", "Canon EOS 8000D"} },
	{ 0x80000349, {"Canon EOS 5D Mark IV", NULL, NULL} },
	{ 0x80000350, {"Canon EOS 80D", NULL, NULL} },
	{ 0x80000355, {"Canon EOS M2", NULL, NULL} },
	{ 0x80000382, {"Canon EOS 5DS", NULL, NULL} },
	{ 0x80000393, {"Canon EOS 750D", "Canon EOS Rebel T6i", "Canon EOS Kiss X8i"} },
	{ 0x80000401, {"Canon EOS 5DS R", NULL, NULL} },
	{ 0x80000404, {"Canon EOS 1300D", "Canon EOS Rebel T6", "Canon EOS Kiss X80"} },
	{ 0x00000000, {NULL, NULL, NULL} }
};

/* Get camera name by model ID and name type (UNIQ, LOC1, LOC2). Returns NULL if camera not matched */
static const char * get_camera_name_by_id(unsigned long model_id, int name_type)
{
	int i = 0;
	const char * camName = NULL;

	while (camera_id[i].cameraModel)
	{
		if (camera_id[i].cameraModel == model_id)
		{
			camName = camera_id[i].cameraName[name_type];
			if(!camName)
			{
				camName = camera_id[i].cameraName[UNIQ];
			}
		}
		i++;
	}
	return camName;
};

#endif
