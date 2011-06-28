#include "../../dryos.h"
#include "../../property.h"

#define PROP_CFN 0x80010004 // 13 bytes
#define CFN1_LEN 13

uint32_t cfn[4];
PROP_HANDLER( PROP_CFN )
{
	cfn[0] = buf[0];
	cfn[1] = buf[1];
	cfn[2] = buf[2];
	cfn[3] = buf[3] & 0xFF;
	//~ bmp_printf(FONT_MED, 0, 450, "cfn: %x/%x/%x/%x", cfn[0], cfn[1], cfn[2], cfn[3]);
	return prop_cleanup( token, property );
}

int get_htp()
{
	if (cfn[1] & 0x10000) return 1;
	return 0;
}

void set_htp(int enable)
{
	if (enable) cfn[1] |= 0x10000;
	else cfn[1] &= ~0x10000;
	prop_request_change(PROP_CFN, cfn, 0xD);
}

void set_mlu(int enable)
{
	if (enable) cfn[2] |= 0x1;
	else cfn[2] &= ~0x1;
	prop_request_change(PROP_CFN, cfn, 0xD);
}
int get_mlu()
{
	return cfn[2] & 0x1;
}

