#include <dryos.h>
#include <property.h>

#define PROP_CFN1 0x80010004 
#define PROP_CFN2 0x80010005 // 5 bytes
#define PROP_CFN3 0x80010006 // 14 bytes
#define PROP_CFN4 0x80010007 
#define CFN1_LEN 
#define CFN2_LEN 5
#define CFN3_LEN 14
#define CFN4_LEN 

uint32_t cfn1[];
uint32_t cfn2[2];
uint32_t cfn3[4];
uint32_t cfn4[];

PROP_HANDLER( PROP_CFN1 )
{
	//~ cfn1[0] = buf[0];
	//~ cfn1[1] = buf[1];
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN2 )
{
	cfn2[0] = buf[0];
	cfn2[1] = buf[1] & 0xFF;
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN3 )
{
	cfn3[0] = buf[0];
	cfn3[1] = buf[1];
	cfn3[2] = buf[2];
	cfn3[3] = buf[3] & 0xFFFF;
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN4 )
{
	//~ cfn4[0] = buf[0];
	//~ cfn4[1] = buf[1] & 0xFFFF;
	return prop_cleanup( token, property );
}


int get_htp()
{
	if (cfn2[0] & 0x1000000) return 1;
	return 0;
}

void set_htp(int enable)
{
	if (enable) cfn2[0] |= 0x1000000;
	else cfn2[0] &= ~0x1000000;
	prop_request_change(PROP_CFN2, cfn2, CFN2_LEN);
}

void set_mlu(int enable)
{
	if (enable) cfn3[1] |= 0x10000;
	else cfn3[1] &= ~0x10000;
	prop_request_change(PROP_CFN3, cfn3, CFN3_LEN);
}
int get_mlu()
{
	return cfn3[1] & 0x10000;
}

void cfn_set_af_button(int value)
{
	//~ cfn[2] &= ~0xF00;
	//~ cfn[2] |= (value << 8) & 0xF00;
	//~ prop_request_change(PROP_CFN, cfn, CFN1_LEN);
}

int cfn_get_af_button_assignment()
{
	//~ return (cfn[2] & 0xF00) >> 8;
}
