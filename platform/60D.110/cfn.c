#include <dryos.h>
#include <property.h>

#define PROP_CFN1 0x80010004 // 8 bytes
#define PROP_CFN2 0x80010005 // 4 bytes
#define PROP_CFN3 0x80010006 // 6 bytes
#define PROP_CFN4 0x80010007 // 6 bytes
#define CFN1_LEN 8
#define CFN2_LEN 4
#define CFN3_LEN 6
#define CFN4_LEN 6

uint32_t cfn1[2];
uint32_t cfn2[1];
uint32_t cfn3[2];
uint32_t cfn4[2];
PROP_HANDLER( PROP_CFN1 )
{
	cfn1[0] = buf[0];
	cfn1[1] = buf[1];
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN2 )
{
	cfn2[0] = buf[0];
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN3 )
{
	cfn3[0] = buf[0];
	cfn3[1] = buf[1] & 0xFFFF;
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_CFN4 )
{
	cfn4[0] = buf[0];
	cfn4[1] = buf[1] & 0xFFFF;
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
	if (enable) cfn3[1] |= 0x100;
	else cfn3[1] &= ~0x100;
	prop_request_change(PROP_CFN3, cfn3, CFN3_LEN);
}
int get_mlu()
{
	return cfn3[1] & 0x100;
}

// used for showing AF patterns
// todo: find a more elegant method to trigger AF points display in viewfinder 
int af_button_assignment = -1;
void assign_af_button_to_halfshutter()
{
	af_button_assignment = cfn4[0] & 0xF00;
	cfn4[0] &= ~0xF00;
	prop_request_change(PROP_CFN4, cfn4, CFN4_LEN);
}

// for stack focus
void assign_af_button_to_star_button()
{
	af_button_assignment = cfn4[0] & 0xF00;
	cfn4[0] &= ~0xF00;
	cfn4[0] |= 0x400;
	prop_request_change(PROP_CFN4, cfn4, CFN4_LEN);
}

void restore_af_button_assignment()
{
	if (af_button_assignment == -1) return;
	cfn4[0] &= ~0xF00;
	cfn4[0] |= af_button_assignment;
	af_button_assignment = -1;
	prop_request_change(PROP_CFN4, cfn4, CFN4_LEN);
}
