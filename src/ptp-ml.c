/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "ptp-ml.h"
#include "tasks.h"
#include "menu.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"
#include "version.h"

static void fill_uint32(char* data, uint32_t value) {
	uint8_t* d = (uint8_t*)data;
	*d = value & 0xFF; value >>= 8; d++;
	*d = value & 0xFF; value >>= 8; d++;
	*d = value & 0xFF; value >>= 8; d++;
	*d = value & 0xFF;
}

uint32_t menu_data_size(struct menu_entry * m2) {
	uint32_t size = 0;
	int i;
	size += 28 + strlen(m2->name);
	// id,min,max,flags*2,priv,name_size
	if (m2->choices) {
		for (i=0;i<=m2->max;i++) {
			// string for the choice
			size += 4 + strlen(m2->choices[i]);
		}
	}
	return size;
}

char* menu_data_fill(char* dptr, struct menu_entry * m2) {
	// id,min,max,flags*2,priv,name_size
	uint32_t strl = strlen(m2->name);
	uint32_t flags = 0;
	int i;
	if (m2->select) flags |= PTP_ML_SUBMENU_HAS_SELECT;
	//if (m2->select_reverse) flags |= PTP_ML_SUBMENU_HAS_SELECT_REVERSE;
	if (m2->select_Q) flags |= PTP_ML_SUBMENU_HAS_SELECT_Q;
	if (IS_VISIBLE(m2)) flags |= PTP_ML_SUBMENU_IS_ESSENTIAL; // probably needs fixing somewhere else too
	if (m2->choices) flags |= PTP_ML_SUBMENU_HAS_CHOICE;
	if (m2->children) flags |= PTP_ML_SUBMENU_HAS_SUBMENU;
	if (m2->selected) flags |= PTP_ML_SUBMENU_IS_SELECTED;
	//if (m2->display) flags |= PTP_ML_SUBMENU_HAS_DISPLAY;
	flags |= (m2->edit_mode&0xF) << PTP_ML_SUBMENU_EDIT_MODE_SHIFT;
	flags |= (m2->icon_type&0xF) << PTP_ML_SUBMENU_ICON_TYPE_SHIFT;
	flags |= (m2->unit&0xF) << PTP_ML_SUBMENU_UNIT_SHIFT;
	/*fill_uint32(dptr, m2->id);*/     dptr+=4;
	fill_uint32(dptr, m2->min);    dptr+=4;
	fill_uint32(dptr, m2->max);    dptr+=4;
	fill_uint32(dptr, flags);      dptr+=8; //4 bytes reserved for later use
	if (m2->priv) fill_uint32(dptr, MEM(m2->priv)); else fill_uint32(dptr, -1);   dptr+=4;
	fill_uint32(dptr, strl);       dptr+=4;
	// memcpy doesn't work with unaligned data
	my_memcpy(dptr, m2->name, strl);
	dptr+=strl;
	if (m2->choices) {
		for (i=0;i<=m2->max;i++) {
			strl = strlen(m2->choices[i]);
			fill_uint32(dptr, strl);       dptr+=4;
			my_memcpy(dptr, m2->choices[i], strl);
			dptr+=strl;
		}
	}
	return dptr;
}

PTP_HANDLER( PTP_ML_CODE, 0 )
{
	struct ptp_msg msg = {
		.id        = PTP_RC_OK,
		.session    = session,
		.transaction    = transaction,
		.param_count    = 4,
		.param        = { 1, 2, 0xdeadbeef, 3 },
	};

	// handle command
	switch ( param1 )
	{

		case PTP_ML_USB_Version:
			msg.param_count = 2;
			msg.param[0] = PTP_ML_VERSION_MAJOR;
			msg.param[1] = PTP_ML_VERSION_MINOR;
			break;

		case PTP_ML_Version:
			msg.param_count = 0;
			switch (param2) {
				case 0: if ( !send_ptp_data(context, (char *) build_version, strlen(build_version) ) ) msg.id = PTP_RC_GeneralError; break;
				case 1: if ( !send_ptp_data(context, (char *) build_id, strlen(build_id) ) ) msg.id = PTP_RC_GeneralError; break;
				case 2: if ( !send_ptp_data(context, (char *) build_date, strlen(build_date) ) ) msg.id = PTP_RC_GeneralError; break;
				case 3: if ( !send_ptp_data(context, (char *) build_user, strlen(build_user) ) ) msg.id = PTP_RC_GeneralError; break;
				default: msg.id = PTP_RC_ParameterNotSupported;
			}
			break;

		case PTP_ML_GetMemory:
			msg.param_count = 0;
			if ( param2 == 0 || param3 < 1 ) // null pointer or invalid size?
			{
				msg.id = PTP_RC_GeneralError;
				break;
			}

			if ( !send_ptp_data(context, (char *) param2, param3) )
			{
				msg.id = PTP_RC_GeneralError;
			}
			break;

		case PTP_ML_SetMemory:
			msg.param_count = 0;
			if ( param2 == 0 || param3 < 1 ) // null pointer or invalid size?
			{
				msg.id = PTP_RC_GeneralError;
				break;
			}

			context->get_data_size(context->handle);
			if ( !recv_ptp_data(context,(char *) param2,param3) )
			{
				msg.id = PTP_RC_GeneralError;
			}
			break;
		case PTP_ML_GetMenus:
			{
				struct menu * m = menu_get_root();
				char *data, *dptr;
				int size = 0;
				msg.param_count = 0;
				for ( ; m ; m = m->next ) {
					// ID + string length + string
					size += 8 + strlen(m->name);
				}
				m = menu_get_root();
				data = SmallAlloc(size);
				if (!data) {
					msg.id = PTP_RC_GeneralError;
					break;
				}
				memset(data, '-', size);
				dptr = data;
				for ( ; m ; m = m->next ) {
					uint32_t strl = strlen(m->name);
					fill_uint32(dptr, m->id);
					dptr+=4;
					fill_uint32(dptr, strl);
					dptr+=4;
					// memcpy doesn't work with unaligned data
					my_memcpy(dptr, m->name, strl);
					dptr+=strl;
				}
				if ( !send_ptp_data(context, data, size ) ) {
					msg.id = PTP_RC_GeneralError;
				}
				SmallFree(data);
			}
			break;
		case PTP_ML_GetMenuStructs:
			{
				struct menu_entry *m, *m2;
				char *data, *dptr;
				int size = 0;
				msg.param_count = 0;

				/* ToDo: replacement    m = menu_find_by_id(param2); */
				if (!m) {
					msg.id = PTP_RC_GeneralError;
					break;
				}
				m2 = m;

				for ( ; m2 ; m2 = m2->next ) {
					size += menu_data_size(m2);
				}
				m2 = m;
				data = SmallAlloc(size);
				if (!data) {
					msg.id = PTP_RC_GeneralError;
					break;
				}
				memset(data, '-', size);
				dptr = data;
				for ( ; m2 ; m2 = m2->next ) {
					dptr = menu_data_fill(dptr, m2);
				}
				if ( !send_ptp_data(context, data, size ) ) {
					msg.id = PTP_RC_GeneralError;
				}
				SmallFree(data);
			}
			break;
		case PTP_ML_GetMenuStruct:
			{
				struct menu_entry *m;
				char *data;
				int size = 0;
				msg.param_count = 0;

				/* ToDo: replacement    m = menu_find_by_id(param2); */
				if (!m) {
					msg.id = PTP_RC_GeneralError;
					break;
				}
				size = menu_data_size(m);
				data = SmallAlloc(size);
				if (!data) {
					msg.id = PTP_RC_GeneralError;
					break;
				}
				memset(data, '-', size);
				menu_data_fill(data, m);
				if ( !send_ptp_data(context, data, size ) ) {
					msg.id = PTP_RC_GeneralError;
				}
				SmallFree(data);
			}
			break;

		case PTP_ML_SetMenu:
			{
				struct menu_entry *entry;
				/* ToDo: replacement    entry = menu_find_by_id(param2); */
                
				msg.param_count = 0;
				if (!entry) {
					msg.id = PTP_RC_GeneralError;
					break;
				}

				if(param3 == 1) // decrement
				{
					//if( entry->select_reverse ) entry->select_reverse( entry->priv, -1 );
					//else 
                    if (entry->select) entry->select( entry->priv, -1);
					else menu_numeric_toggle(entry->priv, -1, entry->min, entry->max);
				}
				else if (param3 == 2) // Q
				{
					if ( entry->select_Q ) entry->select_Q( entry->priv, 1);
				}
				else if (param3 == 0) // increment
				{
					if( entry->select ) entry->select( entry->priv, 1);
					else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
				}
				else if (param3 == 3) {
					if (entry->priv) MEM(entry->priv) = param4;
				}
				msg.param_count = 1;
				if (entry->priv) msg.param[0] = MEM(entry->priv); else msg.param[0] = -1;
			}
			break;

		default:
			msg.id = PTP_RC_ParameterNotSupported;
			break;
	}

	context->send_resp(
			context->handle,
			&msg
			);

	return 0;
}

