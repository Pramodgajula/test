/******************************************************************************
 * sdh_stm1_hal.c
 *
 * Hardware Abstraction Layer for Sedona
 *
 * Shared Libraries Used:
 *		None
 * Static Libraries Used:
 *		None
 * Unit Test Drivers (Binary):
 *		None
 * Revision History	:
 *		None
 * Version 1.0   (20/Jul/2013) Mujahid Ali
 *		Initial Release
 * (License Info, if any)
 *
 * Copyright (C)
 ******************************************************************************/
/******************************************************************************
 * 								Include Files
 ******************************************************************************/
 /*Project includes*/
 #include <stdio.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <string.h>
 #include <stdbool.h>
 #include <mqueue.h>
 #include <time.h>
 #include <sys/types.h>
 #include <sys/ipc.h>
 #include <sys/msg.h>
 #include <stdarg.h>
 #include <arpa/inet.h>
 #include <sys/time.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <sys/wait.h>
 #include <unistd.h>
 #include <errno.h>
 #include <signal.h>
 #include <termios.h>
 #include <fcntl.h>
 #include <pthread.h>
 #include"ces_common.h"
 #include"ces_sockets.h"

/*module includes*/
 #include "pal_lib.h"
 #include "sdh_stm1_cel.h"
 #include "sdh_stm1_config_int_ext_lib.h"
 #include "../core/include/sdh_stm1_enums.h"
 #include "sdh_stm1_hil.h"
 #include "../core/include/sdh_stm1_pm_data_struct.h"
 #include "sdh_stm1_adm_register_info.h"
 #include "sdh_stm1_hal_liu.h"
 #include "sdh_stm1_hal.h"

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/

/* create the sonet register info */
#define SDH_HAL_LIU_INIT(id, addr, datasize, default_val) {addr, datasize, default_val},
SonetLIUReg_Info_S	sonet_reg_info[] ={	SONET_REG_PARAM_LIST };
#undef SDH_HAL_LIU_INIT

/* Buffer to store hte */
sonet_database_S stm1_liu_database;

/******************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/
PAL_INT_32 hal_sdh_stm1_liu_config(Hal_operation_E , unsigned char ,\
							unsigned char ,unsigned char , unsigned char *);
static PAL_INT_32 hw_write_sonet_liu(PAL_UCHAR , PAL_UCHAR , PAL_UCHAR );
static int hw_read_sonet_liu(unsigned char , unsigned char ,unsigned char *);
/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/

int hal_sdh_stm1_liu_init()
{
#define SDH_HAL_LIU_INIT(id, regaddr, regsize,  default_val) {regaddr,default_val},
	Regmap_S default_cfg_value[] = {SONET_REG_PARAM_LIST};
#undef SDH_HAL_LIU_INIT
	memcpy(&stm1_liu_database.defaultparms, default_cfg_value, \
						sizeof(Regmap_S)*(MAX_SONET_REG_PARAM-1));
	return SUCCESS;
}

int hal_sdh_stm1_liu_mem_alloc(int max_stm1_ports)
{
	unsigned char liuPortIndex;

	if(stm1_liu_database.sonet_liu_database != NULL)
	{
		free(stm1_liu_database.sonet_liu_database);
	}

	stm1_liu_database.sonet_liu_database =\
		malloc(sizeof(sonet_liu_database_S)*max_stm1_ports);
 
	for(liuPortIndex = 0; liuPortIndex < max_stm1_ports; liuPortIndex++) 
        {        
        	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.master_ctrl.regvalue = \
                                stm1_liu_database.defaultparms.mscr.reg_value;
	        stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue = 0;
        	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue = \
                                stm1_liu_database.defaultparms.sgcr.reg_value;
	        stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl2.regvalue = \
                                stm1_liu_database.defaultparms.mcr2.reg_value;
	}	

	return SUCCESS;
}

PAL_INT_32 hal_default_port_cfg(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;

	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.master_ctrl.regvalue = \
				stm1_liu_database.defaultparms.mscr.reg_value;
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue = 0;
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue = \
				stm1_liu_database.defaultparms.sgcr.reg_value;
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl2.regvalue = \
				stm1_liu_database.defaultparms.mcr2.reg_value;

	return SUCCESS;
}

/******************************************************************************
 * Description :This function will reset LOLOR Circuitry,on the STM Mini IO Card.
 * Inputs      :channel no and ui configurable parameter structure
 * Outputs     :database for sonet
 ******************************************************************************/
int hw_reset_sonet_liu(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval=0;

    /* Reset LOLOR and FRST (FIFO Reset) */

    /* Set the LOLOR and FRST bits */
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.lolor = 1;
    stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.frst = 1;
    /* configure signal control register in sonet liu */
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.sgcr.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

    /* Reset the LOLOR and FRST bits */
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.lolor = 0;
    stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.frst = 0;
    /* configure signal control register in sonet liu */
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.sgcr.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to configure the loopback type for a
*	given port no w.r.t LIU_TYPE1.
*
*	Inputs:		 loopback_type and liuPortIndex
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hal_sdh_stm1_port_loopback_config(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	PAL_INT_32 retval = FAILURE;
	PAL_UCHAR reg_addr=0;
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;

	if(HAL_STM1_WRITE & ptr_liu_cfg_value->liuOperation)
	{
		/* configure loopback select bit in all channels */
		switch (*(char *)(ptr_liu_cfg_value->data_ptr))
		{
			case SDH_LB_NORMAL:
			default:
				printf(" SONET_LB_NORMAL\n");
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.llbk = 0;
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.rlbk = 0;
				break;

			case SDH_LB_ANALOG:
				printf(" SONET_LB_ANALOG\n");
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.llbk = 1;
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.rlbk = 0;
				break;

			case SDH_LB_REMOTE:
				printf(" SONET_LB_REMOTE\n");
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.llbk = 0;
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.reg_bits.rlbk = 1;
				break;
		}
	}
	printf(" *** stm1_liu_database.sonet_liu_database[liuPortIndex].signal_ctrl.regvalue = %d \n", \
				stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue);
	printf(" *** sonet_reg_info[SGCR_REG].addr = %x \n", sonet_reg_info[SGCR_REG].addr);
	printf(" *** %s: driver type = %d \n", __FUNCTION__, ptr_liu_cfg_value->liu_access);

	/* configure signal control register in sonet liu */

	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[SGCR_REG].addr, \
					&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue);

	retval = hw_reset_sonet_liu(ptr_liu_cfg_value);

	printf(" Leaving %s \n", __FUNCTION__);

	return retval;
}

/*******************************************************************************
*	Description:This function will read the status monitor reg, which will be
*				used by the application to monitor the Loss Of Signal.
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_monitor_sonet_statusreg(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = SUCCESS;

	retval = hal_sdh_stm1_liu_config(HAL_STM1_READ,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[STAT_REG].addr, \
					(char *)ptr_liu_cfg_value->data_ptr);

	if(retval < 0)
	{
		return FAILURE;
	}

	return retval;
}

/*******************************************************************************
*	Description:This function will read the modecontrol reg to check pdtx
*				and pdrx.
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_read_sonet_modecontrolreg(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = SUCCESS;

	retval = hal_sdh_stm1_liu_config(HAL_STM1_READ,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[MDCR_REG].addr, \
					(char *)ptr_liu_cfg_value->data_ptr);
	if(retval < 0)
	{
		return FAILURE;
	}
	return retval;
}

/*******************************************************************************
*	Description:This function will read the signal control reg to check
*				the loopback.
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_read_sonet_signalcontrolreg(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = SUCCESS;

	retval = hal_sdh_stm1_liu_config(HAL_STM1_READ,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[SGCR_REG].addr, \
					(char *)ptr_liu_cfg_value->data_ptr);
	if(retval < 0)
	{
		return FAILURE;
	}

	return retval;
}

/*******************************************************************************
*	Description:This function will set the pdtx and pdrx bits of modecontrol reg
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_powerdown_tx_rx_sonet(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = 0;

	/* Power down the corresponding channel */
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdtx = 1;
	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdrx = 1;

	/* configure Master control register in sonet LIU */
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[MSCR_REG].addr, \
					&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.master_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[MDCR_REG].addr, \
					&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	return retval;
}

/*******************************************************************************
*	Description:This function will set the pdtx and pdrx bits of modecontrol reg
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_sonet_tx_disable_rx_enable(hal_liu_cfg_info_S *ptr_liu_cfg_value)
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = 0;

	stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue = 0x80;

	/* keep transmitter disabled and enable receiver circuitry*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[MDCR_REG].addr, \
					&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue);

	if(retval < 0)
	{
		return FAILURE;
	}
	return retval;
}

/*******************************************************************************
*	Description:This function will set the pdtx and pdrx bits of modecontrol reg
*	Inputs:		channel no and data pointer.
*	Outputs:	Returns failure or Success
********************************************************************************/
int hw_sonet_medium_status(hal_liu_cfg_info_S *ptr_liu_cfg_value) \
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval =0;

	if((*(char *)(ptr_liu_cfg_value->data_ptr)) == 0)
	{
		/*Power Down LIU*/
		stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdtx = 1;
		stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdrx = 1;
	}
	else
	{
		/*Power UP LIU*/
		stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdtx = 0;
		stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.reg_bits.pdrx = 0;
	}

	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access, \
					sonet_reg_info[MDCR_REG].addr, \
					&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	return retval;
}

/******************************************************************************
 * Description :Configure the power down register of the SONET LIU depending
 *              on SONET Channel Map. If the given channel is not present in
 *              the SONET Channel map the corresponding LIU is powered down,
 *              else the corresponding LIU is powered up.
 *
 * Inputs      :Active channel list for sonet.
 *
 * Outputs     :sonet setparms - local config variables for sonet
 ******************************************************************************/
void tx_rx_activechannels_sonet_setparms(unsigned char stm1_port_index, unsigned char active)
{
	if(active == 0)
	{
		/* Power down the corresponding channel */
		stm1_liu_database.sonet_liu_database[stm1_port_index].setparms.mode_ctrl.reg_bits.pdtx = 1;
		stm1_liu_database.sonet_liu_database[stm1_port_index].setparms.mode_ctrl.reg_bits.pdrx = 1;
	}
	else
	{
		/* Power up the corresponding channel */
		stm1_liu_database.sonet_liu_database[stm1_port_index].setparms.mode_ctrl.reg_bits.pdtx = 0;
		stm1_liu_database.sonet_liu_database[stm1_port_index].setparms.mode_ctrl.reg_bits.pdrx = 0;
	}
}

void update_sonet_setparms_latestvalues(void)
{
    unsigned char port_index = 0;

    for(port_index = 0; port_index < stm1_liu_database.number_of_liu; port_index++)
    {
        /* ensure that E4 is disabled for all channels */
        stm1_liu_database.sonet_liu_database[port_index].setparms.master_ctrl.reg_bits.e4 = 0;

        /* check the loopback */
        switch(stm1_liu_database.sonet_liu_database[port_index].cfgparms.loopback)
        {
            case SDH_LB_NORMAL:
            default:
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.llbk = 0;
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.rlbk = 0;
                break;

            case SDH_LB_ANALOG:
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.llbk = 1;
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.rlbk = 0;
                break;

            case SDH_LB_REMOTE:
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.llbk = 0;
                stm1_liu_database.sonet_liu_database[port_index].setparms.signal_ctrl.reg_bits.rlbk = 1;
                break;
        }

        if(stm1_liu_database.sonet_liu_database[port_index].stm1_liu_interface_type == STM1_ELECTRICAL )
        {
            stm1_liu_database.sonet_liu_database[port_index].setparms.mode_ctrl2.reg_bits.cmi = 1;
        }
        else if(stm1_liu_database.sonet_liu_database[port_index].stm1_liu_interface_type == STM1_OPTICAL )
        {
            stm1_liu_database.sonet_liu_database[port_index].setparms.mode_ctrl2.reg_bits.cmi = 0;
        }
        /* Removed set_sfp_liu_interface_type() Function as per comment #108 and added below
           code to update the mode_ctrl2 register cmi bit to zero for both electrical and
           optical SFP type on boot up*/
        if(stm1_liu_database.sonet_liu_database[port_index].stm_connection_type == SFP_SDH_STM1 )
        {
            stm1_liu_database.sonet_liu_database[port_index].setparms.mode_ctrl2.reg_bits.cmi = 0;
        }
    }
    /* enable the tx and rx for active channels of sonet */
    /* YTBU - Sedona */
    //tx_rx_activechannels_sonet_setparms();
}

int configure_sonetliu_hw(hal_liu_cfg_info_S *ptr_liu_cfg_value )
{
	unsigned char liuPortIndex = ptr_liu_cfg_value->liuPortIndex;
	int retval = 0;

	/* configure Master control register in sonet liu*/

	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.mscr.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.master_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Interrupt control register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.intc.reg_addr,\
			&stm1_liu_database.defaultparms.intc.reg_value);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure IO control register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.iocr.reg_addr,\
			&stm1_liu_database.defaultparms.iocr.reg_value);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Mode control register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.mdcr.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Signal control register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.sgcr.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.signal_ctrl.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Advance Tx Control 1 register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.acr1.reg_addr,\
			&stm1_liu_database.defaultparms.acr1.reg_value);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Advance Tx Control 0 register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.acr0.reg_addr,\
			&stm1_liu_database.defaultparms.acr0.reg_value);
	if(retval < 0)
	{
		return FAILURE;
	}

	/* configure Mode Control 2 register in sonet liu*/
	retval = hal_sdh_stm1_liu_config(HAL_STM1_WRITE,liuPortIndex,ptr_liu_cfg_value->liu_access,\
			stm1_liu_database.defaultparms.mcr2.reg_addr,\
			&stm1_liu_database.sonet_liu_database[liuPortIndex].setparms.mode_ctrl2.regvalue);
	if(retval < 0)
	{
		return FAILURE;
	}

	/*<Bugzilla><771><26-April-2006>*/
    retval = hw_reset_sonet_liu(ptr_liu_cfg_value);

	return retval;
}

PAL_INT_32 hal_sdh_stm1_liu_config(Hal_operation_E liuOperation, unsigned char liuPortIndex,\
							unsigned char liu_access,unsigned char addr, unsigned char *data)
{
	int ret_val = 0;
	if(liuOperation & HAL_STM1_READ)
	{
		ret_val = hw_read_sonet_liu(liuPortIndex, addr, data);
	}

	if(liuOperation & HAL_STM1_WRITE)
	{
		ret_val = hw_write_sonet_liu(liuPortIndex, addr, *data);
	}

	return ret_val;
}

/*******************************************************************************
*	Description:This function is used to set the LIU configuration for a
*	given port no.
*
*	Inputs:		 port no, address, data
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hw_write_sonet_liu(PAL_UCHAR port_no, PAL_UCHAR reg_addr, PAL_UCHAR data)
{
	PAL_UCHAR data_buff[100], addr[100];
	PAL_INT_32 retval = FAILURE;
	Sdh_Stm1_Op_S sdh_stm1_liu_op;
	printf(" Entered %s \n", __FUNCTION__);

	/* fill the sdh_stm1 liu operation attributes */
	sdh_stm1_liu_op.device_id = SONET_ID;
	sdh_stm1_liu_op.operation = SONET_WRITE;
	sdh_stm1_liu_op.desaddr_chno = port_no;
	sdh_stm1_liu_op.regcount = 1;
	sdh_stm1_liu_op.regdata_size = 1;
	addr[0] = reg_addr;
	data_buff[0] = data;
	retval = form_packet_access_sonetliu(addr, data_buff, &sdh_stm1_liu_op);
	return retval;
}

/*******************************************************************************
*	Description:This function is used to read the data from the register
*				address of the given sonet channel.
*	Inputs:		chno-channel number,
*				addr - register address
*				data - register data to be stored.
*				sdh_stm1_liu_op -  holds all required values to be used by
*				sdh_stm1 driver
*	Outputs:	Returns failure or Success
********************************************************************************/
static int hw_read_sonet_liu(unsigned char chno, unsigned char addr,
								unsigned char *data)
{
	int retval = 0;
	unsigned char kbuf_data[100], kbuf_addr[100];
	Sdh_Stm1_Op_S sdh_stm1_liu_op;

	/* fill the sdh_stm1 liu operation attributes */
	sdh_stm1_liu_op.device_id = SONET_ID;
	sdh_stm1_liu_op.operation = SONET_READ;
	sdh_stm1_liu_op.desaddr_chno = chno;
	sdh_stm1_liu_op.regcount = 1;
	sdh_stm1_liu_op.regdata_ptr = data;
	sdh_stm1_liu_op.regdata_size = 1;
	kbuf_addr[0] = addr;
	retval = form_packet_access_sonetliu(kbuf_addr, kbuf_data,
					&sdh_stm1_liu_op);
	if(retval >= 0)
	{
		memcpy(data, kbuf_data, sdh_stm1_liu_op.regcount);
	}
	return retval;
}

/****************************************************************************
 *	Description:Form spi packet for sonet and send to the sonet liu.
 *				The Sonet LIU has a 2 byte frame.  The first byte is a 4-bit
 *				port address, a 3-bit sub address and a R/W control (1= read,
 *				0 = write). The second byte is data.  The bytes are
 *				transmitted LSB first (right to left).
 *				byte 1 -  Port_A<4> Sub_A<3> R/W<1>
 *				byte 2 - Data<8>
 *	Inputs:		data buffer, address buffer and Sdh_Stm1_Op_S
 *	Outputs:	Returns failure or Success
 ****************************************************************************/
int form_packet_access_sonetliu(unsigned char *kbuf_addr,
				unsigned char *kbuf_data, Sdh_Stm1_Op_S *sdh_stm1_liu_op)
{
	unsigned char spi_buf[100];
	unsigned short spibuf_len = 0;
	int retval =0;
	unsigned char i =0;


/*<Bugzilla><500><15-Jan-2007> code starts */
	unsigned char tempData =0;
	unsigned char data[100];
	unsigned char *buf_data = data;
	unsigned char retry = 0;
	printf("Sonet: Entered form_packet_access_sonetliu function \n");
/* <Bugzilla><500><15-Jan-2007> code ends*/
#if 0
	switch(sdh_stm1_liu_op->desaddr_chno)
	{
		case 1:
				spi_buf[spibuf_len] = OPT_LIU_CHIP1;
				spibuf_len++;
				break;
		case 2:
				/* PUMA_TODO: chip address to be changed */
				spi_buf[spibuf_len] = OPT_LIU_CHIP2;
				spibuf_len++;
				break;
		case 3:
				/* PUMA_TODO: chip address to be changed */
				spi_buf[spibuf_len] = DS3_SONET_MIN1;
				spibuf_len++;

/*<Bugzilla><500><15-Jan-2007> code starts */
/* For Mini Io card this code writes into LIU chip and then reads back and compares the data witten with
actual data. If it is not matching then does three retries.*/

				KDEBUG3("Sonet : Entered DS3_SONET_MIN1 case \n");

				if(sdh_stm1_liu_op->operation == SONET_READ)
				{
					KDEBUG3("Sonet : READ Operation \n");
					/* pass dummy kbuf_data during read operation */
					for(i=0; i<sdh_stm1_liu_op->regcount; i++)
					{
						/* Read bit is always set in address value (Bit 0) */

						kbuf_addr[i] |= 0x01;
						spi_buf[spibuf_len] = kbuf_addr[i];
						KDEBUG3(" Read Data Address = %x\n",kbuf_addr[i]);
						spibuf_len++;
						spi_buf[spibuf_len] = DUMMY_BYTE;
						spibuf_len++;
					}

					retval = read_write_sonetliu(spi_buf, spibuf_len, kbuf_data, sdh_stm1_liu_op);
					KDEBUG3(" Read Data Value = %x\n",kbuf_data[0]);
					return retval;
				}

				for (retry =0; retry < 3; retry++)
				{
					if(sdh_stm1_liu_op->operation == SONET_WRITE)
					{
						KDEBUG3("Sonet : WRITE Operation \n");
						/* pass valid kbuf_data pointer during write operation */
						for(i=0; i<sdh_stm1_liu_op->regcount; i++)
						{
							spi_buf[spibuf_len] = kbuf_addr[i];
							KDEBUG3(" Write Data Address = %x\n",kbuf_addr[i]);

							/* Clear the READ bit(Bit 0) in the address register to make
							it write operation */
							spi_buf[spibuf_len] &= SET_SONET_WRITE_OPERATION;
							spibuf_len++;
							spi_buf[spibuf_len] = kbuf_data[i];

							KDEBUG3(" Write Data value = %x\n",kbuf_data[i]);

							tempData =  kbuf_data[i]; // copy data
							spibuf_len++;
						}
					}
					retval = read_write_sonetliu(spi_buf, spibuf_len, kbuf_data, sdh_stm1_liu_op);

					/* read the value written and compare it with original value intended to be written */
					i = 0;
					spibuf_len = 1;
					sdh_stm1_liu_op->operation = SONET_READ;
					for(i=0; i<sdh_stm1_liu_op->regcount; i++)
					{
						/* Read bit is always set in address value (Bit 0) */
						kbuf_addr[i] |= 0x01;
						spi_buf[spibuf_len] = kbuf_addr[i];
						spibuf_len++;
						spi_buf[spibuf_len] = DUMMY_BYTE;
						spibuf_len++;
					}

					retval = read_write_sonetliu(spi_buf, spibuf_len, buf_data, sdh_stm1_liu_op);

					KDEBUG3(" Actual Written value = %x\n",buf_data[0]);

					/* compare it with original value intended to be written */
					if (buf_data[0] == tempData)
					{
						/* No Error */
						KDEBUG2("Sonet : Write operation successful \n");
						return retval;

					}
					else
					{
						/* Error */
						/* Do 2 more retries */
						KDEBUG3("Sonet : Write operation NOT successful \n");
						KDEBUG3(" Original data = %x\n",tempData);
						KDEBUG3(" Read data = %x\n",buf_data[0]);
						KDEBUG3(" Retry count = %d\n",retry);
						spibuf_len = 1;
						sdh_stm1_liu_op->operation = SONET_WRITE;
					}
				}

				retval = FRAMER_IOCTL_FAIL;
				KDEBUG3("Sonet : Write operation FAILED \n");
				return retval;


/* <Bugzilla><500><15-Jan-2007> code ends*/

				break;
		default:
				retval = FRAMER_IOCTL_FAIL;
				break;
	}
	if(retval == 0)
	{
		if(sdh_stm1_liu_op->operation == SONET_READ)
		{
			/* pass dummy kbuf_data during read operation */
			for(i=0; i<sdh_stm1_liu_op->regcount; i++)
			{
				/* Read bit is always set in address value (Bit 0) */
				if(sdh_stm1_liu_op->desaddr_chno !=DS3_SONET_MIN1)
				{
					#ifndef SONET_LIU_TEST
					kbuf_addr[i] |= 0x01;
					#endif
				}
				else
				{
				kbuf_addr[i] |= 0x01;
				}

				spi_buf[spibuf_len] = kbuf_addr[i];
				spibuf_len++;
				spi_buf[spibuf_len] = DUMMY_BYTE;
				spibuf_len++;
			}
		}
		else if(sdh_stm1_liu_op->operation == SONET_WRITE)
		{
			/* pass valid kbuf_data pointer during write operation */
			for(i=0; i<sdh_stm1_liu_op->regcount; i++)
			{
				spi_buf[spibuf_len] = kbuf_addr[i];

				/* Clear the READ bit(Bit 0) in the address register to make
				it write operation */
				if(sdh_stm1_liu_op->desaddr_chno !=DS3_SONET_MIN1)
				{
					#ifndef SONET_LIU_TEST
					spi_buf[spibuf_len] &= SET_SONET_WRITE_OPERATION;
					#endif
				}
				else
				{
				spi_buf[spibuf_len] &= SET_SONET_WRITE_OPERATION;
				}
				spibuf_len++;
				spi_buf[spibuf_len] = kbuf_data[i];
				spibuf_len++;
			}
		}

		retval = read_write_sonetliu(spi_buf, spibuf_len, kbuf_data, sdh_stm1_liu_op);

	}
#endif
	return retval;

}




