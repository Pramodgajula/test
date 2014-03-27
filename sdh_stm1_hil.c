/******************************************************************************
 * HIL Library
 *
 * Hardware Interface Library
 *
 * Copyright (C) CarrierComm Inc.
 *****************************************************************************/

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

/* module includes */
#include "pal_lib.h"
#include "../core/include/sdh_stm1_enums.h"
#include "../core/include/sdh_stm1_pm_data_struct.h"
#include "sdh_stm1_config_int_ext_lib.h"
#include "sdh_stm1_hil.h"
#include "sdh_stm1_cel.h"
#include "sdh_stm1_ail.h"
#include "sdh_stm1_adm_register_info.h"


/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/

/* To store the STM1 Hardware configuration received from card manager */
Stm1_Card_Manager_S 	hil_sdh_hw_info;

/* To store the STM1 Port configuration received from user */
Stm1_Port_Config_S 	hil_stm_port_config;

/* To store the STM1 Core configuration received from user */
Stm1_Core_Config_S	hil_sdh_core_config;

/* To store the STM1 Chip configuration received from user */
Stm1_Chip_Set_S 	chip_setparms;

/* To store the STM1 Mux-Demux configuration received from user */
Sdh_Stm1_Ch_Core_Info_S		port_core_info[MAX_NO_STM_PROTECTION];


/* create the sonet register info */
#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Stm_Mux_Demux_Info_S	sdh_mux_demux_reg_info[] =
{
	STM1_MUX_DEMUX_REG_PARAM_LIST
};
#undef PARM_INIT

#define E1T1_CP_HW_PORT_INIT(enume, internal_offset, start_port, end_port, card_type) {internal_offset, start_port, end_port, card_type},

e1t1_port_info_S 	e1t1_port_info[] =
{
	E1T1_CP_HW_PORT_CONFIG
};
#undef E1T1_CP_HW_PORT_INIT

#define STM1_CP_HW_PORT_INIT(enume, internal_offset, start_port, end_port, card_type) {internal_offset, start_port, end_port, card_type},

stm1_port_info_S 	stm1_port_info[] =
{
	STM1_CP_HW_PORT_CONFIG
};
#undef STM1_CP_HW_PORT_INIT

#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Sdh_Stm1_Pm_Reg_Info_S	sdh_stm1_pm_reg_info[] ={	SDH_STM1_PM_REG_PARAM_LIST };
#undef PARM_INIT

#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Sdh_Stm1_Prot_Reg_Info_S	sdh_stm1_prot_reg_info[] ={	SDH_STM1_PROT_SW_REG_PARAM_LIST };
#undef PARM_INIT

#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Sdh_Stm1_Cp_Reg_Info_S	sdh_stm1_cp_reg_info[] ={	SDH_STM1_CP_REG_PARAM_LIST };
#undef PARM_INIT

#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Sdh_Stm1_Timing_Mod_Reg_Info_S	sdh_stm1_timing_mod_reg_info[] ={SDH_STM1_TIMING_MOD_REG_PARAM_LIST };
#undef PARM_INIT

#define PARM_INIT_EXT_MEM_MAP(section_id, perf_data_offset, element_size) {perf_data_offset,element_size},
Sdh_Stm1_Perf_data_Offset_S	stm_perf_data_offset[] = {SDH_STM1_EXTERNAL_MEM_MAP_LIST};
#undef PARM_INIT_EXT_MEM_MAP

#define STM1_ADM_INTR_SUMMARY1_INIT(enume, set_bit, mask_app, priority) {set_bit, mask_app, priority},
Stm1_Adm_Intr_Summary1_List_S	stm1_adm_intr_summary1[] ={	STM1_ADM_INTR_SUMMARY1_LIST };
#undef STM1_ADM_INTR_SUMMARY1_INIT

#define STM1_SNCP_INTR_SUMMARY0_INIT(enume, set_bit, mask_app, priority) {set_bit, mask_app, priority},
Stm1_Sncp_Intr_Summary0_List_S	stm1_sncp_intr_summary0[] ={ STM1_SNCP_INTR_SUMMARY0_LIST };
#undef STM1_SNCP_INTR_SUMMARY0_INIT

#define STM1_MSP_INTR_SUMMARY0_INIT(enume, set_bit, mask_app, priority) {set_bit, mask_app, priority},
Stm1_Msp_Intr_Summary0_List_S	stm1_msp_intr_summary0[] ={ STM1_MSP_INTR_SUMMARY0_LIST };
#undef STM1_MSP_INTR_SUMMARY0_INIT

#define STM1_PM_INTR_SUMMARY0_INIT(enume, set_bit, mask_app, priority) {set_bit, mask_app, priority},
Stm1_Pm_Intr_Summary0_List_S	stm1_pm_intr_summary0[] ={ STM1_PM_INTR_SUMMARY0_LIST };
#undef STM1_PM_INTR_SUMMARY0_INIT
/*
 * a syndrome table is generated & used for
 * calculating CRC-7
 */
static PAL_UCHAR crc7_syndrome_table[256];

/*****************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/


/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/
PAL_INT_32 hil_sdh_stm1_init_buffer(PAL_VOID)
{
	memset(&hil_sdh_hw_info, 0, sizeof(hil_sdh_hw_info));
	memset(&hil_stm_port_config, 0, sizeof(hil_stm_port_config));
	memset(&hil_sdh_core_config, 0, sizeof(hil_sdh_core_config));
	return SUCCESS;
}


/******************************************************************************
 * Description   : This function is used to initialize the HIL Params Database
 *                                      configuration
 * Inputs        : None
 * Output        : None
 * Return Value : STM1_SUCCESS on success/STM1_FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_database_init()
{
        int ret_val = STM1_SUCCESS;

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PORT_CONFIG,\
                                                                        &hil_stm_port_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the HIL Port Config database \n", __func__);
        }
        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_MUX_DEMUX_CONFIG,\
                                                                        &hil_sdh_core_config.md_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the HIL Mux-Demux Config database \n", __func__);
        }

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_TRACE_MSG_CONFIG,\
                                                                        &hil_sdh_core_config.tm_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the HIL Trace Msg Config database \n", __func__);
        }

        return ret_val;
}

/******************************************************************************
 * Description		: This function will:
 *					  1.Allocate the memory dynamically for the number of ports
 *						supported by the LIU and SDH Mux-Demux
 *					  2.Call the relevant functions to process it
 * Inputs     		:
 * Output    		: To assign the memory for the number of ports supported
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_alloc_mem(Stm1_Sdh_Hw_Max_Interface_S block_info, int config)
{
	PAL_INT_32 retval = FAILURE;

	printf(" Entered %s \n", __FUNCTION__);
	switch(config)
	{
		case PORT_CONFIG:
		{
			printf(" Entered PORT_CONFIG case \n");

			/* Allocate memory for Port Configuration */
			if((retval = dbl_sdh_stm1_cfg_update(&hil_stm_port_config, block_info, 1, STM1_PORT_CONFIG)) < 0)
			{
				printf(" dbl_sdh_stm1_cfg_update() failed \n");
				return retval;
			}

#if 0
			/* Allocate memory for port configuration */
			if((retval = sdh_stm1_port_cfg_alloc_mem(block_info,&hil_stm_port_config))<0)
			{
				printf("sdh_stm1_port_cfg_alloc_mem failed \n");
				return retval;
			}
#endif
			/*sdh_stm1_port_cfg_alloc_mem(block_info,&def_sdh_config_db.port_config);*/
		}
		break;
		case CORE_CONFIG:
		{
			printf(" Entered CORE_CONFIG case \n");

            /* Allocate memory for Mux-Demux Configuration */
            if((retval = dbl_sdh_stm1_cfg_update(&hil_sdh_core_config.md_config, block_info, 1, STM1_MUX_DEMUX_CONFIG)) < 0)
            {
                printf(" dbl_sdh_stm1_cfg_update() failed \n");
                return retval;
            }
            /* Allocate memory for Trace Msg Configuration */
            if((retval = dbl_sdh_stm1_cfg_update(&hil_sdh_core_config.tm_config, block_info, 1, STM1_TRACE_MSG_CONFIG)) < 0)
            {
                printf(" dbl_sdh_stm1_cfg_update() failed \n");
                return retval;
            }
#if 0
			/* Allocate memory for Mux-Demux Configurations */
			if((retval = sdh_stm1_mux_demux_alloc_mem(block_info,&hil_sdh_core_config.md_config))<0)
			{
				printf("sdh_stm1_mux_demux_alloc_mem failed \n");
				return retval;
			}
			/* Allocate memory for trace messages */
			if((retval = sdh_stm1_trace_msg_alloc_mem(block_info,&hil_sdh_core_config.tm_config))<0)
			{
				printf("sdh_stm1_trace_msg_alloc_mem failed \n");
				return retval;
			}
#endif
		}
		break;
		default:
		break;
	}

	printf(" Leaving %s \n",__FUNCTION__);

	return SUCCESS;

} /* end of hil_sdh_stm1_alloc_mem() */

PAL_INT_32 hil_sdh_stm1_cleanup(PAL_VOID)
{
	// Release Hardware Configuration Database
	dbl_sdh_stm1_release_config_buffer(STM1_PORT_CONFIG, &hil_stm_port_config);
	dbl_sdh_stm1_release_config_buffer(STM1_MUX_DEMUX_CONFIG, &hil_sdh_core_config.md_config);
	dbl_sdh_stm1_release_config_buffer(STM1_TRACE_MSG_CONFIG, &hil_sdh_core_config.tm_config);

#if 0
	/* Clean the memory for STM1 Hardware Configuration database */
	sdh_stm1_port_cfg_free_mem(&hil_stm_port_config);
	sdh_stm1_mux_demux_free_mem(&hil_sdh_core_config.md_config);
	sdh_stm1_trace_msg_free_mem(&hil_sdh_core_config.tm_config);
#endif

   return SUCCESS;
}

/********************************************************************************
*Description  : Generate a table of CRC-7 syndromes for x^7 * each possible
*       input byte
*Input        : None
*Output       : None
*********************************************************************************/

PAL_VOID hil_sdh_stm1_gen_crc7_syndrome_table (PAL_VOID)
{
	printf(" Entered %s \n", __FUNCTION__);
    PAL_INT_32 i, j, syndrome;
    for (i = 0;  i < 256;  ++i)
    {
        //printf("\n syndrome table generation \n");
        syndrome = ((i & 0x80) != 0)? i ^ CRC7_POLYNOMIAL : i;

        for (j = 0;  j < 7;  ++j)
        {

            if (((syndrome <<= 1) & 0x80) != 0)
            {

                syndrome ^= CRC7_POLYNOMIAL;

			}
		}

        crc7_syndrome_table[i] = (PAL_UCHAR) syndrome;
    }
	printf(" Leaving %s \n", __FUNCTION__);
}

/********************************************************************************
*Description  : Insert the CRC-7 stm trace msg
*Input        : None
*Output       : None
*********************************************************************************/
PAL_VOID insert_crc7 (PAL_UCHAR *stm_trace_msg,PAL_INT_32 count)
{
	PAL_UCHAR crc7_accum = 0;
	PAL_INT_32 i;
	PAL_UCHAR  temp_start[count];


	/* newly added */
	memset(&temp_start,0,sizeof(PAL_UCHAR)*count);

	memcpy(&temp_start[1],stm_trace_msg,sizeof(PAL_UCHAR)*(count-1));
	temp_start[0] = 0xf8; /* any value greater than 0x80 */
	temp_start[0] &= ~0x7f;

	for (i = 0;  i < count;  ++i)
	{
		crc7_accum = crc7_syndrome_table[(crc7_accum << 1)^ temp_start[i]];
	}
	temp_start[0] ^= crc7_accum;

	printf("CRC Calculated Hex:%0x, char %c\n",temp_start[0], temp_start[0]);
	memcpy(stm_trace_msg,temp_start,sizeof(PAL_UCHAR)*count);
	printf("CRC Inserted   Hex:%0x, char %c\n",stm_trace_msg[0], stm_trace_msg[0]);

}

/******************************************************************************
 * Description		: This function is used to verify the STM1 SDH Hardware
 *					  configuration read from card manager is valid
 *					  and store in hil global structure
 * Inputs     		: Hardware information from card manager
 * Output    		: Provided the STM1 SDH hardware cfg to check in CIL and CEL
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_verify_hw_info(Stm1_Card_Manager_S *sdh_stm1_card_info,\
							Stm1_Sdh_Hw_Max_Interface_S *ptr_sdh_stm1_hw_cfg)
{
	PAL_UINT_32 liu_index=0,chip_index=0;
	PAL_UCHAR card_index = 0;
	PAL_USHORT_16 max_stm1_ports = 0;
	Stm1_Sdh_Port_Info_S *ptr_sdh_stm1_info = NULL;

	/* Read the Hardware specific information from Card Manager */
	memcpy(&hil_sdh_hw_info, sdh_stm1_card_info, sizeof(hil_sdh_hw_info));

	ptr_sdh_stm1_hw_cfg->card_num = hil_sdh_hw_info.num_cards;

	/* Free the already allocated memory */
	if(ptr_sdh_stm1_hw_cfg->interface_info != NULL)
		free(ptr_sdh_stm1_hw_cfg->interface_info);

	/* Allocate memory to store the maximum number of stm1 port wrt cards */
	ptr_sdh_stm1_hw_cfg->interface_info = malloc(hil_sdh_hw_info.num_cards * \
													sizeof(Interface_Support_Info_S));

	for(card_index = 0;card_index < \
				hil_sdh_hw_info.num_cards; card_index++)
	{
		max_stm1_ports = 0;
		ptr_sdh_stm1_info = &hil_sdh_hw_info.card_hw_info[card_index].sdh_stm1_info;

		for(liu_index = 0;liu_index < \
					ptr_sdh_stm1_info->num_stm1_liu; liu_index++)
		{
			/* Verify the number of STM1 Ports supported in LIU is valid */
			if((ptr_sdh_stm1_info->liu_info[liu_index].liu_type > MAX_LIU_TYPE) || \
				(ptr_sdh_stm1_info->liu_info[liu_index].driver_access > MAX_LIU_ACCESS))
			{
				return FAILURE;
			}
			max_stm1_ports += ptr_sdh_stm1_info->liu_info[liu_index].stm1_ports;
		}

		/* Verify the max STM1 ports supported are valid */
		if(max_stm1_ports != ptr_sdh_stm1_info->max_stm1_ports)
		{
			return FAILURE;
		}

		/* Update the Maximum Interface support information */
		ptr_sdh_stm1_hw_cfg->interface_info[card_index].card_type = \
											hil_sdh_hw_info.card_hw_info[card_index].card_type;
		ptr_sdh_stm1_hw_cfg->interface_info[card_index].max_sdh_cores = \
											ptr_sdh_stm1_info->max_sdh_cores;
		ptr_sdh_stm1_hw_cfg->interface_info[card_index].max_stm1_ports = \
											ptr_sdh_stm1_info->max_stm1_ports;
		ptr_sdh_stm1_hw_cfg->interface_info[card_index].max_stm1_ports_supported = \
											hil_sdh_hw_info.card_hw_info[card_index].max_stm1_ports_supported;
		ptr_sdh_stm1_hw_cfg->interface_info[card_index].max_sdh_cores_supported = \
											hil_sdh_hw_info.card_hw_info[card_index].max_sdh_cores_supported;
	}

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function is used to get the LIU and port information in
 *					LIU for the user configured STM1 port information
 * Inputs     	 : card information, port number, LIU information to store
 * Output    	 : None
 * Return Value  : Provide the LIU specific information
 ******************************************************************************/
STM1_Liu_Info_S hil_sdh_stm1_get_liu_info(PAL_UCHAR card_index, PAL_UCHAR portIndex,\
													PAL_UCHAR *liu_index)
{
	int liu_count = 0,portCount = 0, card_cnt = 0;
	Stm1_Sdh_Port_Info_S *ptr_stm1_port = NULL;
	unsigned short stm1_port_offset = 0;

	/* Compute the STM1 Port offset for the card index */
	if(card_index == 0)
	{
		stm1_port_offset = 0;
	}
	else
	{
		for(card_cnt = 1;card_cnt <= card_index;card_cnt++)
		{
			stm1_port_offset += hil_sdh_hw_info.card_hw_info[card_cnt-1].sdh_stm1_info.max_stm1_ports;
		}
	}
	/* Compute the STM1 Port number wrt the card and LIU information */
	ptr_stm1_port = &hil_sdh_hw_info.card_hw_info[card_index].sdh_stm1_info;
	if(ptr_stm1_port->num_stm1_liu == 1)
	{
		*liu_index = portIndex;
		return ptr_stm1_port->liu_info[0];
	}

	/* Compute the port index wrt the card type */
	portIndex = portIndex - stm1_port_offset;

	if(portIndex < hil_sdh_hw_info.card_hw_info[card_index].sdh_stm1_info.max_stm1_ports)
	{
		/* Loops and identifies the LIU specific to the port configured by user */
		for(liu_count = 0;liu_count < ptr_stm1_port->num_stm1_liu;liu_count++)
		{
			portCount += ptr_stm1_port->liu_info[liu_count].stm1_ports;
			if(portIndex < portCount)
			{
				/*Identify the exact port number in LIU to configure */
				*liu_index = portIndex - (portCount - \
					ptr_stm1_port->liu_info[liu_count].stm1_ports);
				return ptr_stm1_port->liu_info[liu_count];
			}
		}
	}
	else
	{
		*liu_index = -1;
	}
}

/*******************************************************************************
*	Description:This function is used to configure the port configuration.
*
*	Inputs:		Port Configuration done by user
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_port_config_set(PAL_UCHAR card_index, PAL_UCHAR portIndex,\
							Stm1_Port_Params_S stm1PortCfg, PAL_UCHAR no_check)
{
	PAL_INT_32 retval = SUCCESS;
	PAL_CHAR_8 liuPortIndex = 0;
	PAL_UCHAR liuCfg_value = 0;
	STM1_Liu_Info_S liuInfo;

	/* Check for the valid LIU Info */
	if(0 == hil_sdh_hw_info.card_hw_info[card_index].sdh_stm1_info.num_stm1_liu)
	{
		return FAILURE;
	}

	/* Get the LIU to be configured and its relevant index information */
	liuInfo = hil_sdh_stm1_get_liu_info(card_index,portIndex,&liuPortIndex);
	if(liuPortIndex == -1)
	{
		return FAILURE;
	}
	//if((hil_stm_port_config.port_parms[portIndex].loopback != stm1PortCfg.loopback) || no_check)
	if((hil_stm_port_config.port_params.elems[portIndex].loopback != stm1PortCfg.loopback) || no_check)
	{
		liuCfg_value = stm1PortCfg.loopback;
		retval = hal_sdh_stm1_cfg_liu(LIU_LOOPBACK_CFG,liuInfo.liu_type,liuInfo.driver_access,\
							liuPortIndex,HAL_STM1_WRITE,&liuCfg_value);
		if(SUCCESS == retval)
			hil_stm_port_config.port_params.elems[portIndex].loopback = stm1PortCfg.loopback;
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to configure the port configuration.
*
*	Inputs:		Port Configuration done by user
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_global_config_set(PAL_UCHAR card_index, PAL_UCHAR portIndex)
{
	PAL_INT_32 retval = FAILURE;
	PAL_CHAR_8 liuPortIndex = 0;
	PAL_UCHAR liuCfg_value = 0;
	STM1_Liu_Info_S liuInfo;

	/* Check for the valid LIU Info */
	if(0 == hil_sdh_hw_info.card_hw_info[card_index].sdh_stm1_info.num_stm1_liu)
	{
		return FAILURE;
	}

	/* Get the LIU to be configured and its relevant index information */
	liuInfo = hil_sdh_stm1_get_liu_info(card_index,portIndex,&liuPortIndex);
	if(liuPortIndex == -1)
	{
		return FAILURE;
	}

	retval = hal_sdh_stm1_cfg_liu(LIU_GBL_CFG,liuInfo.liu_type,liuInfo.driver_access,\
						liuPortIndex,HAL_STM1_WRITE,&liuCfg_value);

	return retval;
}



/******************************************************************************
 *TRACE MESSAGE SECTION
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_trace_msg_cfg_update(PAL_UCHAR card_index,PAL_UCHAR adm_core_index, \
						Stm1_Trace_Msg_Cfg_S tm_config_parms,PAL_UCHAR no_cfg_check)
{
	PAL_INT_32 retval = 0;
	printf("Configure RS Section\n");
	/*
	 *  Verify if the RS Section Configuration has Changed
	 */

	if((no_cfg_check) || (hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].rs_parms.rs_tm_length != \
											tm_config_parms.rs_parms.rs_tm_length) || \
		(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].rs_parms.rs_tm_align_type != \
											tm_config_parms.rs_parms.rs_tm_align_type))
	{
		/* Send the Rs data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_tm_rs_config_set(&tm_config_parms.rs_parms, adm_core_index);
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].rs_parms.rs_tm_length = \
												tm_config_parms.rs_parms.rs_tm_length;
		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].rs_parms.rs_tm_align_type = \
												tm_config_parms.rs_parms.rs_tm_align_type;
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tm_data,\
										tm_config_parms.trace_data.tm_data,sizeof(tm_config_parms.trace_data.tm_data)) != 0) || \
		(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tm_crc != tm_config_parms.trace_data.tm_crc))
	{
		/* Configure the RS section Tx Trace Message */
		retval = hil_sdh_stm1_tm_tx_exp_msg_set(tm_config_parms.trace_data.tm_data, \
					&tm_config_parms.trace_data.tm_crc, \
					(RS_TM_LENGTH_16BYTES == tm_config_parms.rs_parms.rs_tm_length)?16:64, \
					TRACE_TX_DATA_WRITE, adm_core_index);

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tm_data,\
						tm_config_parms.trace_data.tm_data,sizeof(tm_config_parms.trace_data.tm_data));
		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tm_crc = \
																	tm_config_parms.trace_data.tm_crc;
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_data,\
										tm_config_parms.trace_data.cmp_tm_data,sizeof(tm_config_parms.trace_data.cmp_tm_data)) != 0) || \
		(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_crc != tm_config_parms.trace_data.cmp_tm_crc))
	{
		/* Configure the RS section Expected Trace Message */
		retval = hil_sdh_stm1_tm_tx_exp_msg_set(tm_config_parms.trace_data.cmp_tm_data, \
					&tm_config_parms.trace_data.cmp_tm_crc, \
					(RS_TM_LENGTH_16BYTES == tm_config_parms.rs_parms.rs_tm_length)?16:64, \
					TRACE_EXP_DATA_WRITE, adm_core_index);

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_data,\
						tm_config_parms.trace_data.cmp_tm_data,sizeof(tm_config_parms.trace_data.cmp_tm_data));
		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_crc = \
																	tm_config_parms.trace_data.cmp_tm_crc;
	}

	printf("Configure HP Section\n");
	/*
	 *  Verify if the HP Section Configuration has Changed
	 */

	if( (no_cfg_check) ||  (hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].hp_parms.hp_tm_length != \
										tm_config_parms.hp_parms.hp_tm_length) || \
		(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].hp_parms.hp_tm_align_type != \
										tm_config_parms.hp_parms.hp_tm_align_type))
	{
		/* Send the HP data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_tm_hp_config_set( &tm_config_parms.hp_parms, adm_core_index );
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].hp_parms.hp_tm_length = \
												tm_config_parms.hp_parms.hp_tm_length;
		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].hp_parms.hp_tm_align_type = \
												tm_config_parms.hp_parms.hp_tm_align_type;
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.vc_data,\
										tm_config_parms.trace_data.vc_data,sizeof(tm_config_parms.trace_data.vc_data)) != 0) || \
		(memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.vc_crc,\
										tm_config_parms.trace_data.vc_crc,sizeof(tm_config_parms.trace_data.vc_crc)) != 0))
	{
		/* Configure the HP Layer 1 section Tx Trace Message */
		/* Compilation Error Fix:  retval = hil_sdh_stm1_vc_tx_exp_msg_set(tm_config_parms.trace_data.vc_data, \
					tm_config_parms.trace_data.vc_crc, \
					(HP_TM_LENGTH_16BYTES == tm_config_parms.hp_parms.hp_tm_length)?16:64, TRACE_TX_DATA_WRITE, adm_core_index, 1);*/

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.vc_data,\
						tm_config_parms.trace_data.vc_data,sizeof(tm_config_parms.trace_data.vc_data));
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.vc_crc,\
						tm_config_parms.trace_data.vc_crc,sizeof(tm_config_parms.trace_data.vc_crc));
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_data,\
										tm_config_parms.trace_data.cmp_vc_data,sizeof(tm_config_parms.trace_data.cmp_vc_data)) != 0) || \
		(memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_crc,\
										tm_config_parms.trace_data.cmp_vc_crc,sizeof(tm_config_parms.trace_data.cmp_vc_crc)) != 0))
	{
		/* Configure the HP Layer 1 section Expected Trace Message */
		/* Compilation Error Fix:  retval = hil_sdh_stm1_vc_tx_exp_msg_set(tm_config_parms.trace_data.cmp_vc_data, \
					tm_config_parms.trace_data.cmp_vc_crc, \
					(HP_TM_LENGTH_16BYTES == tm_config_parms.hp_parms.hp_tm_length)?16:64, TRACE_EXP_DATA_WRITE, adm_core_index,1);*/

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_data,\
						tm_config_parms.trace_data.cmp_vc_data,sizeof(tm_config_parms.trace_data.cmp_vc_data));
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_crc,\
						tm_config_parms.trace_data.cmp_vc_crc,sizeof(tm_config_parms.trace_data.cmp_vc_crc));
	}

	printf("Configure LP Section\n");
	/*
	 *  Verify if the LP Section Configuration has Changed
	 */

	if( (no_cfg_check) ||  (hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].lp_parms.lp_tm_length != \
										tm_config_parms.lp_parms.lp_tm_length) || \
		(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].lp_parms.lp_tm_align_type != \
										tm_config_parms.lp_parms.lp_tm_align_type))
	{
		/* Send the LP data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_tm_lp_config_set( &tm_config_parms.lp_parms, adm_core_index );
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].lp_parms.lp_tm_length = \
												tm_config_parms.lp_parms.lp_tm_length;
		hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].lp_parms.lp_tm_align_type = \
												tm_config_parms.lp_parms.lp_tm_align_type;
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tu_data,\
										tm_config_parms.trace_data.tu_data,sizeof(tm_config_parms.trace_data.tu_data)) != 0) || \
		(memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tu_crc,\
										tm_config_parms.trace_data.tu_crc,sizeof(tm_config_parms.trace_data.tu_crc)) != 0))
	{
		/* Configure the LP Layer 1..84 section Tx Trace Message */
		/* Compilation Error Fix: retval = hil_sdh_stm1_tu_tx_exp_msg_set(tm_config_parms.trace_data.tu_data, \
					&tm_config_parms.trace_data.tu_crc, \
					(LP_TM_LENGTH_16BYTES == tm_config_parms.lp_parms.lp_tm_length)?16:64, TRACE_TX_DATA_WRITE, adm_core_index, 0);*/

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tu_data,\
						tm_config_parms.trace_data.tu_data,sizeof(tm_config_parms.trace_data.tu_data));
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.tu_crc,\
						tm_config_parms.trace_data.tu_crc,sizeof(tm_config_parms.trace_data.tu_crc));
	}

	if( (no_cfg_check) ||  (memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_data,\
										tm_config_parms.trace_data.cmp_tu_data,sizeof(tm_config_parms.trace_data.cmp_tu_data)) != 0) || \
		(memcmp(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_crc,\
										tm_config_parms.trace_data.cmp_tu_crc,sizeof(tm_config_parms.trace_data.cmp_tu_crc)) != 0))
	{
		/* Configure the LP Layer 1..84 section Expected Trace Message */
		/* Compilation Error Fix:  retval = hil_sdh_stm1_tu_tx_exp_msg_set(tm_config_parms.trace_data.cmp_tu_data, \
					&tm_config_parms.trace_data.cmp_tu_crc, \
					(LP_TM_LENGTH_16BYTES == tm_config_parms.lp_parms.lp_tm_length)?16:64, TRACE_EXP_DATA_WRITE, adm_core_index, 1);*/

		if(FAILURE == retval)
			return retval;

		/* Copy the Trace Message data to local buffer */
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_data,\
						tm_config_parms.trace_data.cmp_tu_data,sizeof(tm_config_parms.trace_data.cmp_tu_data));
		memcpy(hil_sdh_core_config.tm_config.ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_crc,\
						tm_config_parms.trace_data.cmp_tu_crc,sizeof(tm_config_parms.trace_data.cmp_tu_crc));
	}
	return retval;
}

/*******************************************************************************
*	Description: This function is used to configure the Trace Message RS section configuration.
*				 This function breaks up the full RS configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 RS Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_tm_rs_config_set(Sdh_Tm_Rs_Parms_S *tm_data, Sdh_ADM_Core_E adm_core_index )
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[RS_CTRL].addr;
	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.tm_64 = \
				 (RS_TM_LENGTH_64BYTES == tm_data->rs_tm_length) ? 1:0;

	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.tm_align = \
				 (RS_TM_ALIGN_NONE == tm_data->rs_tm_align_type) ? 0:1;

	val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_tm_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return SUCCESS;
}
/*******************************************************************************
*	Description: This function is used to configure the Trace Message HP section configuration.
*				 This function breaks up the full HP configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 HP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_tm_hp_config_set(Sdh_Tm_Hp_Parms_S *hp_data, Sdh_ADM_Core_E adm_core_index )
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[HP_GLBL_CTRL].addr;

	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_tm_64 = \
				 (HP_TM_LENGTH_64BYTES == hp_data->hp_tm_length) ? 1:0;

	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_tm_align = \
				 (HP_TM_ALIGN_NONE == hp_data->hp_tm_align_type) ? 0:1;

	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_tm_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return SUCCESS;
}

/*******************************************************************************
*	Description: This function is used to configure the Trace Message LP section configuration.
*				 This function breaks up the full LP configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 LP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_tm_lp_config_set(Sdh_Tm_Lp_Parms_S *lp_data, Sdh_ADM_Core_E adm_core_index )
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[LP_GLBL_CTRL].addr;

	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_tm_64 = \
				 (LP_TM_LENGTH_64BYTES == lp_data->lp_tm_length) ? 1:0;

	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_tm_align = \
				 (LP_TM_ALIGN_NONE == lp_data->lp_tm_align_type) ? 0:1;

	val = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_tm_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_tm_tx_exp_msg_set(PAL_CHAR_8 *tm_data, PAL_CHAR_8 *tm_crc, \
				PAL_UINT_32 tm_data_len, Trace_Msg_Type_E msg_type,Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr=0, val=0, msg_cnt=0;
	PAL_INT_32 access_info = 0;

	addr = sdh_mux_demux_reg_info[RS_TM_DATA].addr;

	if('1' == *tm_crc)
	{
		insert_crc7(tm_data,tm_data_len);
	}
	if(0 != tm_data_len)
	{
		sdh_db.core_set_parms[adm_core_index].rs_reg.rs_tm_data.reg_bits.tm_act = msg_type;
		for(msg_cnt = 0; msg_cnt<tm_data_len; msg_cnt++)
		{
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_tm_data.reg_bits.tm_index = msg_cnt;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_tm_data.reg_bits.tm_data = *(tm_data + msg_cnt);

			if(msg_cnt == (tm_data_len - 1))
				access_info = 1;

			val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_tm_data.regvalue;

			/* Hal function to configure the value in respective register */
			if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, access_info))<0)
			{
				printf("%s hal_sdh_stm1_tm_config_set Failed \n",__FUNCTION__);
				return FAILURE;
			}
		}
	}
	return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_vc_tx_exp_msg_set(PAL_CHAR_8 **vc_data, PAL_CHAR_8 *vc_crc, \
		PAL_UINT_32 vc_data_len,Trace_Msg_Type_E msg_type,Sdh_ADM_Core_E adm_core_index, PAL_INT_32 vc_index)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 access_info = 0, start_cnt = 0,vc_cnt = 0;
	PAL_INT_32 addr=0, val=0, msg_cnt=0;

	if(vc_index == 0)
	{
		vc_index = MAX_VC_DATA;
		start_cnt = 0;
	}
	else
	{
		start_cnt = vc_index - 1;
	}
	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_index.reg_bits.tm_vc = vc_index;
	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_data.reg_bits.tm_act = msg_type;
	for(vc_cnt = start_cnt; vc_cnt<vc_index; vc_cnt++)
	{
		if('1' == vc_crc[vc_cnt])
		{
			insert_crc7(vc_data[vc_cnt],vc_data_len);
		}

		if(0 != vc_data_len)
		{
			for(msg_cnt = 0; msg_cnt<vc_data_len; msg_cnt++)
			{
				sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_index.reg_bits.tm_index = msg_cnt;
				sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_data.reg_bits.tm_data = *(vc_data[vc_cnt]+msg_cnt);

				addr = sdh_mux_demux_reg_info[HP_TM_INDEX].addr;
				val	 = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_index.regvalue;

				/* Hal function to configure the value in respective register */
				if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, access_info))<0)
				{
					printf("%s hal_sdh_stm1_tm_config_set HP_TM_INDEX Failed \n",__FUNCTION__);
					return FAILURE;
				}

				if(msg_cnt == (vc_data_len - 1))
					access_info = 1;

				addr = sdh_mux_demux_reg_info[HP_TM_DATA].addr;
				val	 = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_tm_data.regvalue;
				/* Hal function to configure the value in respective register */
				if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, access_info))<0)
				{
					printf("%s hal_sdh_stm1_tm_config_set HP_TM_DATA Failed \n",__FUNCTION__);
					return FAILURE;
				}
			}
		}
	}
	return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_tu_tx_exp_msg_set(PAL_CHAR_8 **tu_data, PAL_CHAR_8 *tu_crc, \
		PAL_UINT_32 tu_data_len,Trace_Msg_Type_E msg_type,Sdh_ADM_Core_E adm_core_index, PAL_INT_32 tu_index)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 access_info = 0, start_cnt = 0,tu_cnt = 0;
	PAL_INT_32 addr=0, val=0, msg_cnt=0;
	if(tu_index == 0)
	{
		tu_index = MAX_TU_DATA;
		start_cnt = 0;
	}
	else
	{
		start_cnt = tu_index - 1;
	}
	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_index.reg_bits.tm_vc = tu_index;
	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_data.reg_bits.tm_act = msg_type;
	for(tu_cnt = start_cnt; tu_cnt<tu_index; tu_cnt++)
	{
		if('1' == tu_crc[tu_cnt])
		{
			insert_crc7(tu_data[tu_cnt],tu_data_len);
		}

		if(0 != tu_data_len)
		{
			for(msg_cnt = 0; msg_cnt<tu_data_len; msg_cnt++)
			{
				sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_index.reg_bits.tm_index = msg_cnt;
				sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_data.reg_bits.tm_data = *(tu_data[tu_cnt] + msg_cnt);

				addr = sdh_mux_demux_reg_info[LP_TM_INDEX].addr;
				val	 = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_index.regvalue;

				/* Hal function to configure the value in respective register */
				if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, access_info))<0)
				{
					printf("%s hal_sdh_stm1_tm_config_set HP_TM_INDEX Failed \n",__FUNCTION__);
					return FAILURE;
				}

				if(msg_cnt == (tu_data_len - 1))
					access_info = 1;

				addr = sdh_mux_demux_reg_info[LP_TM_DATA].addr;
				val	 = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_tm_data.regvalue;
				/* Hal function to configure the value in respective register */
				if((retval = hal_sdh_stm1_tm_config_set(addr, val, adm_core_index, access_info))<0)
				{
					printf("%s hal_sdh_stm1_tm_config_set HP_TM_DATA Failed \n",__FUNCTION__);
					return FAILURE;
				}
			}
		}
	}
	return retval;
}

/********************************************************************************
 * STM1 Timing Synchronization
 ********************************************************************************/
PAL_INT_32 hil_sdh_stm1_sync_config_set(Stm1_Sync_Parms_S *stm1_sync_config)
{
	printf(" Entered %s \n", __FUNCTION__);
    Sdh_Stm1_Timing_Module_Reg_S stm1_timing_module_cfg;
    PAL_INT_32 retval = 0;
    PAL_INT_32 addr = 0, val = 0;

    if( STM1_SYNC_AUTOMATIC == stm1_sync_config->sync_mode )
    {
        stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.stm1_syncronization_mode = 0x0;
    }
    else if( STM1_SYNC_MANUAL == stm1_sync_config->sync_mode )
    {
        stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.stm1_syncronization_mode = 0x1;

        if( SONET_MEDIUM_SONET == stm1_sync_config->medium_type)
        {
            stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.sdh_sonet_select = 0x0;
        }
        else /* Default to SDH */
        {
            stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.sdh_sonet_select = 0x1;
        }

        /* Primary */
        stm1_timing_module_cfg.timing_reg_1.timing_reg1_conf_bits.ref_clock_1 = stm1_sync_config->primary;

        /* Clear the Primary Unlock Alarm */
        if(stm1_sync_config->primary == TIME_REF_STM1_FREE_RUNNING)
        {

        }

        /* Secondary */
        stm1_timing_module_cfg.timing_reg_1.timing_reg1_conf_bits.ref_clock_2 = stm1_sync_config->secondary;

        /* Clear the Secondary Unlock Alarm */
        if((stm1_sync_config->secondary == TIME_REF_STM1_HOLDOVER) || \
            (stm1_sync_config->secondary == TIME_REF_STM1_FREE_RUNNING))
        {
			// Fillout the alarm and send it.
        }

        /* Tertiary */
        stm1_timing_module_cfg.timing_reg_1.timing_reg1_conf_bits.ref_clock_3 = stm1_sync_config->tertiary;

        /* Clear the Tertiary Unlock Alarm */
        if((stm1_sync_config->tertiary == TIME_REF_STM1_HOLDOVER) || \
            (stm1_sync_config->tertiary == TIME_REF_STM1_FREE_RUNNING))
        {
           // Fillout the alarm and send it.
        }
    }

    /* This section is applicable to  both 'Automatic' and Manual  modes of 'STM1 Synchronization Mode'  */
    if(SONET_MEDIUM_SONET == stm1_sync_config->medium_type)
    {
        stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.sdh_sonet_select = 0x0;
        if(stm1_sync_config->e1_clock_quality < MAX_STATIC_E1_CLOCK_SONET_CODE)
        {
            stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.bits_clock_quality_level = \
                																stm1_sync_config->e1_clock_quality;
        }
        else
        {
           stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.bits_clock_quality_level = \
                															SONET_SYNCHRONIZED_TRACEABILITY_UNKNOWN;
        }
    }
    else /* Default to SDH */
    {
        stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.sdh_sonet_select = 0x1;
        if( stm1_sync_config->e1_clock_quality < MAX_STATIC_E1_CLOCK_SDH_CODE)
        {
			stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.bits_clock_quality_level = \
                																stm1_sync_config->e1_clock_quality;
        }
        else
        {
            stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.bits_clock_quality_level = \
                																SDH_QUALITY_UNKNOWN;
        }
    }

    /* Set Register if STM1-E1/STM1-E2/STM-W1/STM-E2 are selected to any one timing reference : TBD For Sedona */
    stm1_timing_module_cfg.timing_reg_2.timing_reg2_conf_bits.bit_bp_clock_recovery = 0x1;

    /* Set Timing 1 Reg */
    addr = sdh_stm1_timing_mod_reg_info[STM1_TIMING_CFG_REG1].addr;
    val  = stm1_timing_module_cfg.timing_reg_1.regvalue;
    /* Write the configuration in the hardware */
    if((retval = hal_sdh_stm1_sync_config_set(addr, val, 0))<0)
    {
		printf(" Function hal_sdh_stm1_sync_config_set() returned FAILURE \n");
		return FAILURE;
	}

    /* Set Timing 2 Reg */
    addr = sdh_stm1_timing_mod_reg_info[STM1_TIMING_CFG_REG2].addr;
    val  = stm1_timing_module_cfg.timing_reg_2.regvalue;
    /* Write the configuration in the hardware */
    if((retval = hal_sdh_stm1_sync_config_set(addr, val, 1))<0)
    {
		printf(" Function hal_sdh_stm1_sync_config_set() returned FAILURE \n");
		return FAILURE;
	}

	/* Copy the configured values in local hw database */
	sdh_db.chip_setparms.stm1_timing_mod = stm1_timing_module_cfg;

	// NOT CLEAR : sonet_tx_disable_rx_enable(stm1_timing_module_cfg);

	printf(" Leaving %s \n", __FUNCTION__);
    return retval;
}

/******************************************************************************
 * STM1 Performance Monitoring
 ******************************************************************************/

/******************************************************************************
 * Description	 : This function wil get the active/passive bank information
 *
 * Inputs     	 : ADM Core Number
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
static PAL_INT_32 sdh_stm1_pm_current_interval_data_get(Sdh_ADM_Core_E adm_core_index, PAL_UINT_32 bank_offset_info, \
									Sdh_Stm1_Perf_Data_Offset_E data_section, PAL_UINT_32 *ptr_perf_data, PAL_UINT_32 path_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0, path_offset = 0;

	addr 		= stm_perf_data_offset[data_section].data_section_offset+bank_offset_info;
	access_info = stm_perf_data_offset[data_section].element_size;
	/* In case of HP or LP path, the address should be calculated wrt the path index */
	path_offset = path_index*access_info;
	addr 		= addr + path_offset;

	val			= ptr_perf_data;

    /* Get the Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	return SUCCESS;
}

/***********************************************************************************
 * Description 	:	Read SDH Interval Performance Parameters of the specified ADM
 *					Core from the Hw.
 *
 * Input		:	ADM Core Index
 * Output		:	Success/Failure
 ***********************************************************************************/
PAL_INT_32 sdh_stm1_pm_interval_data_get(Sdh_ADM_Core_E adm_core_index, \
								PAL_UINT_32 bank_offset_info, Stm1_Perf_Data_S *ptr_perf_data)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;

	addr 		= stm_perf_data_offset[LP_SECTION_ID].data_section_offset+bank_offset_info;
	access_info = sizeof(ptr_perf_data->layer);
	val			= (PAL_UINT_32 *)&ptr_perf_data->layer;

	/* Get the data for the interval for this section */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	addr 		= bank_offset_info;
	access_info = sizeof(ptr_perf_data->timeElapsed);
	val			= (PAL_UINT_32 *)&ptr_perf_data->timeElapsed;

    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}
	return retval;
}

/******************************************************************************
 * Description	 : This function wil get the pm interval data from the passive bank
 *
 * Inputs     	 : ADM Core Number
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
 /* We have another method to read the bank status it is from Bit 11 of SDH_STM1_PM_FN_CNTRL(0x04 ASIC_CL1) register*/
static PAL_INT_32 sdh_stm1_pm_bank_status_get(PAL_UINT_32 *bank_offset_info, PAL_UCHAR bank, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, val = 0, access_info = 0;
	Sdh_Stm1_Pm_Bank_Status_S pm_sram_bank_status;

	addr = STM1_ADM_PM_BANK1_OFFSET;
	access_info = sizeof(PAL_INT_32);

    /* Read the PM Information */
    if((retval = hal_sdh_stm1_pm_data_get(addr, &val, adm_core_index, access_info))<0)
    {
		printf(" Function hal_sdh_stm1_pm_data_get() returned FAILURE \n");
		return FAILURE;
	}

	memcpy(&pm_sram_bank_status, &val, sizeof(Sdh_Stm1_Pm_Bank_Status_S));

	switch(bank)
	{
		case PM_PASSIVE_BANK:
		{
			if(pm_sram_bank_status.status_flag == 1)
			{
				*bank_offset_info = STM1_ADM_PM_BANK1_OFFSET;
			}
			else
			{
				*bank_offset_info = STM1_ADM_PM_BANK2_OFFSET;
			}
		}
		break;

		case PM_ACTIVE_BANK:
		{
			if(pm_sram_bank_status.status_flag == 1)
			{
				*bank_offset_info = STM1_ADM_PM_BANK2_OFFSET;
			}
			else
			{
				*bank_offset_info = STM1_ADM_PM_BANK1_OFFSET;
			}
		}
		default:
			return FAILURE;
	}
	return pm_sram_bank_status.status_flag;
}

/******************************************************************************
 * Description	 : This function will get the current pm data from hw
 *
 * Inputs     	 : Card Index, ADM Core Number, Section Number, pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_perf_current_data_get(unsigned char card_index, unsigned char adm_core_index,
 					Sdh_Stm1_Pm_Data_E data_section,Sdh_15mins_Pm_data_S *pm_cur_data, unsigned int path_index)
 {
 	PAL_INT_32  retval = SUCCESS;
 	PAL_UINT_32 bank_offset_info = 0;
 	PAL_UINT_32 *data_buffer = (PAL_UINT_32 *)pm_cur_data;

 	Sdh_Stm1_Perf_Data_Offset_E hal_sec_info[] = {RS_SECTION_ID,MS_SECTION_ID,HP_SECTION_ID,\
 							LP_SECTION_ID,FAR_MS_SECTION_ID,FAR_HP_SECTION_ID,FAR_LP_SECTION_ID};

 	/* Validate the card and core supported in software before process to read */
 	if((SEDONA_IO_1 != hil_sdh_hw_info.card_hw_info[card_index].card_type) || \
 		(adm_core_index > 2))
 	{
 		return FAILURE;
	}

	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf(" Failed to read the bank status from HAL - sdh_stm1_pm_bank_status_get \n");
	}

	if((retval = sdh_stm1_pm_current_interval_data_get(adm_core_index, bank_offset_info, hal_sec_info[data_section],\
													data_buffer, path_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_adm_rs_current_data_get() returned Failure ");
		return FAILURE;
	}

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function will
 *					1) read the bank status of Stm1 performance
 *
 * Inputs     	 : pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_pm_interval_data_get(PAL_UCHAR card_index, Sdh_ADM_Core_E adm_core_index,
							PAL_UINT_32 *current_active_bank, Stm1_Perf_Data_S *pm_interval_data)
{
	PAL_INT_32  retval = SUCCESS;
	unsigned int bank_offset_info = 0;

	/* Validate the card and core supported in software before process to read */
	if((SEDONA_IO_1 != hil_sdh_hw_info.card_hw_info[card_index].card_type) || \
		(adm_core_index > 2))
	{
		return FAILURE;
	}

	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_PASSIVE_BANK, adm_core_index))<0)
	{
		printf(" Failed to read the bank status from HAL - sdh_stm1_pm_bank_status_get \n");
	}
	*current_active_bank = retval;

	/* Identify the passive bank to read the interval data */
	if((retval = sdh_stm1_pm_interval_data_get(adm_core_index, bank_offset_info, pm_interval_data))<0)
	{
		printf(" Failed to read the interval data from HAL - hal_sdh_stm1_performance_data_read \n");
	}

	return retval;
}

#if 0
/******************************************************************************
 * Description	 : This function wil get the current RS section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_rs_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}

	addr 		= stm_core_offset[RS_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.regSection);
	val			= pm_sram_buf;

    /* Get the RS Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.regSection), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.regSection));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current MS section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_ms_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}

	addr 		= stm_core_offset[MS_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.muxLine);
	val			= pm_sram_buf;

    /* Get the MS Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.muxLine), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.muxLine));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current HP section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_hp_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}

	addr 		= stm_core_offset[HP_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.highOrderPath);
	val			= pm_sram_buf;

    /* Get the HP Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.highOrderPath), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.highOrderPath));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current LP section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_lp_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}

	addr 		= stm_core_offset[LP_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.lowOrderPath);
	val			= pm_sram_buf;

    /* Get the LP Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.lowOrderPath), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.lowOrderPath));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current far MS section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_far_ms_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}

	addr 		= stm_core_offset[FAR_MS_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farMuxLine);
	val			= pm_sram_buf;

    /* Get the Far MS Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.farMuxLine), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farMuxLine));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current far HP section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_far_hp_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}
	addr 		= stm_core_offset[FAR_HP_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farHighOrderPath);
	val			= pm_sram_buf;

    /* Get the Far HP Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.farHighOrderPath), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farHighOrderPath));

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function wil get the current far LP section data from PM
 *				   SRAM
 *
 * Inputs     	 : ADM Core Number
 *
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pm_adm_far_lp_current_data_get(Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 0;
	PAL_UINT_32 bank_offset_info = 0;
	PAL_UINT_32 pm_sram_buf[MAX_SDH_STM1_CORE_SIZE/sizeof(PAL_UINT_32)];

	/* Get the Passive Bank and Calculate the Active Bank Info */
	if((retval = sdh_stm1_pm_bank_status_get(&bank_offset_info, PM_ACTIVE_BANK, adm_core_index))<0)
	{
		printf("Function hal_sdh_stm1_pm_interval_data_get() returned Failure ");
		return FAILURE;
	}
	addr 		= stm_core_offset[FAR_LP_SECTION_ID].core_offset[adm_core_index-1]+bank_offset_info;
	access_info = sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farLowOrderPath);
	val			= pm_sram_buf;

    /* Get the Far LP Current interval data from SRAM */
    if((retval = hal_sdh_stm1_pm_data_get(addr, val, adm_core_index, access_info))<0)
    {
		printf(" %s hal_sdh_stm1_pm_data_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	memcpy(&(current_data.pm_current_data[adm_core_index].bank[0].layer.farLowOrderPath), val,\
									sizeof(current_data.pm_current_data[adm_core_index].bank[0].layer.farLowOrderPath));

	return SUCCESS;
}
#endif
/******************************************************************************
 * Description	 : This function will
					1) read the status of Stm1 performance (if enable or disable)
					2) read the 15 min sync prob

 * Inputs     	 : pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_pm_status_get(PAL_UINT_32 *pm_status)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, *val = NULL, access_info = 1;

	addr = sdh_stm1_pm_reg_info[SDH_STM1_PM_FN_CNTRL].addr;
	val  = pm_status;
	if((retval = hal_sdh_stm1_pm_status_get(addr, val, access_info))<0)
	{
		printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	*pm_status = (*pm_status & STM1_ADM_PM_STATUS)?SDH_STM1_PM_ENABLE:SDH_STM1_PM_DISABLE;
	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function will update the 15 min sync prob register

 * Inputs     	 : pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_pm_status_set(PAL_UINT_32 pm_status)
{
	PAL_INT_32  retval = FAILURE;
	PAL_INT_32  addr = 0, val = 0, access_info = 1;

	addr = sdh_stm1_pm_reg_info[SDH_STM1_PM_FN_CNTRL].addr;

	if( SDH_STM1_PM_ENABLE == pm_status )
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.enable = 1;
	}
	else
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.enable = 0;
	}
	val = sdh_db.chip_setparms.pm_reg.pm_config.regvalue;

	if((retval = hal_sdh_stm1_pm_status_set(addr, val, access_info))<0)
	{
		printf(" %s hal_sdh_stm1_pm_status_set() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function will
 *					1) read the bank status of Stm1 performance
 *
 * Inputs     	 : pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_pm_bank_status_get(PAL_UINT_32 *current_inactive_bank,Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32  retval = FAILURE;

	if((retval = sdh_stm1_pm_bank_status_get(current_inactive_bank, PM_PASSIVE_BANK, adm_core_index))<0)
	{
		printf(" %s hal_sdh_stm1_pm_bank_status_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	printf(" current_inactive_bank = %d \n",*current_inactive_bank);
	return SUCCESS;
}

/******************************************************************************
 * Description	 : This function will toggle 15 min interval data buffer

 * Inputs     	 : pointer to buffer
 * Return Value  : SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_perf_toggle_bank(PAL_VOID)
{
	PAL_INT_32  retval = SUCCESS;
	PAL_INT_32 access_info = 1;
	PAL_INT_32 addr = 0, *val = NULL;

	sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.sw_adj_init = 1;
	sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.sw_adj_time = 0;

	addr = sdh_stm1_pm_reg_info[SDH_STM1_PM_FN_CNTRL].addr;
	val  = (PAL_INT_32 *) sdh_db.chip_setparms.pm_reg.pm_config.regvalue;

	if((retval = hal_sdh_stm1_pm_data_set(addr, val, access_info))<0)
	{
		printf(" %s hal_sdh_stm1_pm_data_set() returned FAILURE \n", __FUNCTION__);
		return FAILURE;
	}

	return retval;
}

/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for MSP switching.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_pm_intr_mask_set(PAL_INT_32 section)
{
	PAL_INT_32 retval=SUCCESS,index=0,addr=0,val=0;

	for(index=MSP_SW_STATE_A_EVENT;index<MAX_STM1_PM_INTR_SUMMARY0;index++)
	{
		if( PRI_ONE == stm1_pm_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_pm_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[0].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 |= \
							stm1_pm_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[0].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_pm_intr_summary0[index].set_bit));
		}
		else if( PRI_TWO == stm1_pm_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_pm_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[0].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 |= \
							stm1_pm_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[0].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_pm_intr_summary0[index].set_bit));
		}
		else if( PRI_THREE == stm1_pm_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_pm_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[0].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 |= \
							stm1_pm_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[0].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_pm_intr_summary0[index].set_bit));
		}
	}
	addr = sdh_mux_demux_reg_info[INTR_MASK_0A].addr;
	val = sdh_db.core_set_parms[0].mask_reg.mask_0a.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0B].addr;
	val = sdh_db.core_set_parms[0].mask_reg.mask_0b.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0C].addr;
	val = sdh_db.core_set_parms[0].mask_reg.mask_0c.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration for PM.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_pn_intr_cntrl_set(Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_stm1_pm_reg_info[SDH_STM1_PM_FN_CNTRL].addr;

	sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.unused			= 0;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.intr_cntrl		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.adj_intr_cntrl	= (UNSET_ALL == section )? 0x0 : 0x2;
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.chip_setparms.pm_reg.pm_config.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_pm_data_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}
/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration for PM.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_pm_config_set(pm_config_S pm_config)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr=0, val=0;

	addr = sdh_stm1_pm_reg_info[SDH_STM1_PM_FN_CNTRL].addr;

	/* Configure PM Monitoring Status */
	if(SDH_STM1_PM_ENABLE == pm_config.enable)
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.enable = 1;
	}
	else
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.enable = 0;
	}

	/* Configure PM Timing */
	if(pm_config.adj_time>0)
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.sw_adj_init = 1;
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.sw_adj_time = pm_config.adj_time;
	}
	sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.num_sec_interval = pm_config.interval_time;

	/* Configure PM restart */
	if(pm_config.restart)
	{
		sdh_db.chip_setparms.pm_reg.pm_config.reg_bits.init_pulse = 1;
	}

	val = sdh_db.chip_setparms.pm_reg.pm_config.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_pm_data_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

PAL_INT_32 hil_sdh_stm1_asic_version_get(PAL_INT_32 curr_sdh_asic_ver)
{
	/* TBD */
        return SUCCESS;
}


PAL_INT_32 hil_sdh_stm1_msp_status_register_get(PAL_UINT_32 *sd_sf_alm_status)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_liu_signal_control_reg_get(PAL_UCHAR *liu_loc_alm_status)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_sync_register_get(PAL_UINT_32 *sync_alm_status, Stm1_SQ_Type_E status_query_type)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_prot_disable(Sdh_Stm1_Prot_Algo_E prot_algo)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_port_config_ltype_get(Stm1_Medium_Line_Coding_E line_coding)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_port_config_lcoding_get(Stm1_Medium_Line_Type_E line_type)
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 sdh_stm1_configure_exp_cp_switch()
{
	/* TBD */
        return SUCCESS;
}

PAL_INT_32 hw_configure_stm1_cp()
{
	/* TBD */
        return SUCCESS;
}



/*************************************************************************
 * ADM Configurations
 *************************************************************************/
/*******************************************************************************
*	Description: This function is used to configure the mux-demux RS section configuration.
*				 This function breaks up the full RS configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 RS Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_rs_config_set(Stm1_Mux_Demux_Cfg_S md_data, \
												Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 access_info = 0;
	PAL_INT_32 addr = 0,val = 0, hp_vc_info = 0;

	/* Configure HP Ctrl register wrt the RS rate value */
	addr = sdh_mux_demux_reg_info[HP_GLBL_CTRL].addr;
	if(RS_SDH_RATE_STM0 == md_data.rs_parms.rs_sdh_rate)
	{
		hp_vc_info = SDH_VC3;
	}
	else
	{
		hp_vc_info = SDH_VC4;
	}
	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_vcsize = \
													(SDH_VC3 == hp_vc_info)?0x0:0x01;
	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.regvalue;
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[RS_CTRL].addr;

	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.rs_dscramb = \
							(RS_DESCRAMBLER_DISABLE == md_data.rs_parms.rs_descrambler)?0x1:0x0;
	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.rs_rate =
							(RS_SDH_RATE_STM0 == md_data.rs_parms.rs_sdh_rate)?0x0:0x1;
	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.rs_sonet = \
							(SONET_MEDIUM_SONET == md_data.medium_type)? 0x01:0x00;
	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.regen_mode = \
							(SDH_OPMODE_REGENERATOR == md_data.op_mode)? 0x01:0x00;

	val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.regvalue;

	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}
	return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_md_ses_threshold_set(Stm1_Mux_Demux_Cfg_S md_data, \
												Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 access_info = 0;
	PAL_UINT_32 ses_threshold[MAX_MD_SECTION];
	PAL_INT_32 addr = 0, val = 0;

	memset(ses_threshold,0,sizeof(ses_threshold));

	switch(md_data.ses_thresh_val)
	{
		case SDH_SES_THRESH_OTHER:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_OTHER_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_OTHER_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_OTHER_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_OTHER_VALUE;
			break;
		}
		case SDH_SES_THRESH_BELLCORE1991:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_BELLCORE1991_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_BELLCORE1991_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_BELLCORE1991_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_BELLCORE1991_VALUE;
			break;
		}
		case SDH_SES_THRESH_ANSI1993:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_ANSI1993_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_ANSI1993_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_ANSI1993_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_ANSI1993_VALUE;
			break;
		}
		case SDH_SES_THRESH_ITU1995:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_ITU1995_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_ITU1995_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_ITU1995_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_ITU1995_VALUE;
			break;
		}
		case SDH_SES_THRESH_ANSI1997:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_ANSI1997_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_ANSI1997_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_ANSI1997_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_ANSI1997_VALUE;
			break;
		}
		case SDH_SES_THRESH_ITU_G829:
		{
			ses_threshold[RS_SECTION] = SONET_SES_RS_THRESH_ITU_G829_VALUE;
			ses_threshold[MS_SECTION] = SONET_SES_MS_THRESH_ITU_G829_VALUE;
			ses_threshold[HP_SECTION] = SONET_SES_HP_THRESH_ITU_G829_VALUE;
			ses_threshold[LP_SECTION] = SONET_SES_LP_E1_THRESH_ITU_G829_VALUE;
			break;
		}
		default:
			return FAILURE;
	}

	/* RS Threshold Set */
	addr = sdh_mux_demux_reg_info[RS_THRESHOLD].addr;
	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_set_thresh.reg_bits.rs_thresh = ses_threshold[RS_SECTION];
	val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_set_thresh.regvalue;
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 0))<0)
	{
		return FAILURE;
	}

	/* MS Threshold Set */
	addr = sdh_mux_demux_reg_info[MS_THRESHOLD].addr;
	sdh_db.core_set_parms[adm_core_index].ms_reg.ms_set_thresh.reg_bits.ms_thresh = ses_threshold[MS_SECTION];
	val = sdh_db.core_set_parms[adm_core_index].ms_reg.ms_set_thresh.regvalue;
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 0))<0)
	{
		return FAILURE;
	}

	/* HP Threshold Set */
	addr = sdh_mux_demux_reg_info[HP_THRESHOLD].addr;
	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_set_thresh.reg_bits.hp_thresh = ses_threshold[HP_SECTION];
	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_set_thresh.regvalue;
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 0))<0)
	{
		return FAILURE;
	}

	/* LP Threshold Set and configure all threshold*/
	addr = sdh_mux_demux_reg_info[LP_THRESHOLD].addr;
	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_set_thresh.reg_bits.lp_thresh = ses_threshold[LP_SECTION];
	val = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_set_thresh.regvalue;
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}

	return SUCCESS;
}

/*******************************************************************************
*	Description: This function is used to configure the mux-demux MS section configuration.
*				 This function breaks up the full MS configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 MS Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_ms_config_set(Sdh_Ms_Parms_S ms_ctrl_cfg, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 access_info = 1;
	PAL_INT_32 addr = 0, val = 0;

	addr = sdh_mux_demux_reg_info[MS_CTRL].addr;

	switch(ms_ctrl_cfg.ms_rei_ctrl_type)
	{
		case MS_REI_DO_NOT_SEND:
		default:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rei_ctl = 0x0;
			break;
		}
		case MS_REI_SEND_ON_B2:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rei_ctl = 0x1;
			break;
		}
		case MS_REI_FORCED_SEND:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rei_ctl = 0x2;
			break;
		}
	}

	switch(ms_ctrl_cfg.ms_rdi_ctrl_type)
	{
		case MS_RDI_DO_NOT_SEND:
		default:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rdi_ctl = 0x0;
			break;
		}
		case MS_RDI_SEND_ON_ALARMS:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rdi_ctl = 0x1;
			break;
		}
		case MS_RDI_FORCED_SEND:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.reg_bits.ms_rdi_ctl = 0x2;
			break;
		}
	}
	val	 = sdh_db.core_set_parms[adm_core_index].ms_reg.ms_ctrl.regvalue;

	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}

	return SUCCESS;
}

/*******************************************************************************
*	Description: This function is used to configure the mux-demux HP section configuration.
*				 This function breaks up the full HP configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 HP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_hp_config_set(Sdh_Hp_Parms_S hp_glbl_ctrl_cfg, Sdh_ADM_Core_E adm_core_index )
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 access_info = 1;
	PAL_INT_32 addr = 0, val = 0;

	addr = sdh_mux_demux_reg_info[HP_GLBL_CTRL].addr;
	switch(hp_glbl_ctrl_cfg.hp_rei_ctrl_type)
	{
		case HP_REI_DO_NOT_SEND:
		default:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rei_ctrl = 0x0;
			break;
		case HP_REI_SEND_ON_B3:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rei_ctrl = 0x1;
			break;
		case HP_REI_FORCED_SEND:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rei_ctrl = 0x2;
			break;
	}

	switch(hp_glbl_ctrl_cfg.hp_rdi_ctrl_type)
	{
		case HP_RDI_DO_NOT_SEND:
		default:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rdi_ctrl = 0x0;
			break;
		case HP_RDI_SEND_ON_ALARMS:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rdi_ctrl = 0x1;
			break;
		case HP_RDI_FORCED_SEND:
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_rdi_ctrl = 0x2;
			break;
	}
	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.reg_bits.hp_pl_exp = hp_glbl_ctrl_cfg.hp_path_label;
	val	 = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_glbl_ctrl.regvalue;

	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}
	return retval;
}


/*******************************************************************************
*	Description: This function is used to configure the mux-demux LP section configuration.
*				 This function breaks up the full LP configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 LP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_lp_config_set(Sdh_Lp_Parms_S lp_gbl_ctrl_cfg, Sdh_ADM_Core_E adm_core_index )
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 access_info = 1;
	PAL_INT_32 addr = 0, val = 0;

	addr = sdh_mux_demux_reg_info[LP_GLBL_CTRL].addr;

	switch(lp_gbl_ctrl_cfg.lp_rei_ctrl_type)
	{
		case LP_REI_DO_NOT_SEND:
		default:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rei_ctrl = 0x0;
			break;
		case LP_REI_SEND_ON_BIP2:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rei_ctrl = 0x1;
			break;
		case LP_REI_FORCED_SEND:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rei_ctrl = 0x2;
			break;
	}

	switch(lp_gbl_ctrl_cfg.lp_rdi_ctrl_type)
	{
		case LP_RDI_DO_NOT_SEND:
		default:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rdi_ctrl = 0x0;
			break;
		case LP_RDI_SEND_ON_ALARMS:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rdi_ctrl = 0x1;
			break;
		case LP_RDI_FORCED_SEND:
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_rdi_ctrl = 0x2;
			break;
	}
	sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.reg_bits.lp_pl_exp = lp_gbl_ctrl_cfg.lp_path_label;
	val	 = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_glbl_ctrl.regvalue;

	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		return FAILURE;
	}
	return SUCCESS;
}

PAL_INT_32 hil_sdh_stm1_mux_demux_config_update(PAL_UCHAR card_index, Sdh_ADM_Core_E adm_core_index, \
						 Stm1_Mux_Demux_Cfg_S md_config_parms, PAL_UCHAR no_cfg_check)
{
	PAL_INT_32 retval = 0;

	if((SEDONA_IO_1 != hil_sdh_hw_info.card_hw_info[card_index].card_type) || \
		(adm_core_index > 2))
	{
		return FAILURE;
	}

	/*
	 *  Verify if the RS configuration has Changed
	 */
	if((no_cfg_check) || (hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode !=
												md_config_parms.op_mode) ||
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].medium_type !=
												md_config_parms.medium_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].rs_parms.rs_sdh_rate != \
												md_config_parms.rs_parms.rs_sdh_rate) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].rs_parms.rs_descrambler != \
												md_config_parms.rs_parms.rs_descrambler))
	{
		/* Send the Rs data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_md_rs_config_set(md_config_parms, adm_core_index);
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode = \
														md_config_parms.op_mode;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].medium_type = \
														md_config_parms.medium_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].rs_parms.rs_sdh_rate = \
														md_config_parms.rs_parms.rs_sdh_rate;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].rs_parms.rs_descrambler = \
														md_config_parms.rs_parms.rs_descrambler;
	}

	printf("Configure SES Threshold\n");
	/*
	 * If the SES Threshold is updated then update the new value in local database.
	 */
	if((no_cfg_check) || (hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ses_thresh_val != \
																md_config_parms.ses_thresh_val))
	{
		retval = hil_sdh_stm1_md_ses_threshold_set(md_config_parms, adm_core_index);
		if(FAILURE == retval)
			return retval;

		/* Copy the SES threshold values. */
		printf("SES Threshold is changed val = %d\n",md_config_parms.ses_thresh_val);
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ses_thresh_val = \
														md_config_parms.ses_thresh_val;
	}

	/*
	 * Verify if the Multiplex Section Configuration has changed
	 */
	if((no_cfg_check) || (hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ms_parms.ms_rei_ctrl_type != \
		md_config_parms.ms_parms.ms_rei_ctrl_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ms_parms.ms_rdi_ctrl_type != \
		md_config_parms.ms_parms.ms_rdi_ctrl_type))
	{
		/* Send the MS data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_md_ms_config_set(md_config_parms.ms_parms, adm_core_index );
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ms_parms.ms_rei_ctrl_type = \
												md_config_parms.ms_parms.ms_rei_ctrl_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].ms_parms.ms_rdi_ctrl_type = \
												md_config_parms.ms_parms.ms_rdi_ctrl_type;
	}

	/*
	 * Verify if the Higher Order Section Configuration has changed
	 */
	if((no_cfg_check) || (hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_rei_ctrl_type != \
		md_config_parms.hp_parms.hp_rei_ctrl_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_rdi_ctrl_type != \
		md_config_parms.hp_parms.hp_rdi_ctrl_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_path_label != \
		md_config_parms.hp_parms.hp_path_label) )
	{
		/* Send the HP data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_md_hp_config_set(md_config_parms.hp_parms, adm_core_index );
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_rei_ctrl_type = \
												md_config_parms.hp_parms.hp_rei_ctrl_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_rdi_ctrl_type = \
												md_config_parms.hp_parms.hp_rdi_ctrl_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].hp_parms.hp_path_label = \
												md_config_parms.hp_parms.hp_path_label;
	}

	/*
	 * Verify if the Lower Order Section Configuration has changed
	 */
	if((no_cfg_check) || (hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_rei_ctrl_type != \
		md_config_parms.lp_parms.lp_rei_ctrl_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_rdi_ctrl_type != \
		md_config_parms.lp_parms.lp_rdi_ctrl_type) || \
		(hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_path_label != \
		md_config_parms.lp_parms.lp_path_label))
	{
		/* Send the LP data to HIL (which will internally send the data to HAL) to configure in the hardware */
		retval = hil_sdh_stm1_md_lp_config_set(md_config_parms.lp_parms, adm_core_index );
		if(FAILURE == retval)
			return retval;

		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_rei_ctrl_type = \
												md_config_parms.lp_parms.lp_rei_ctrl_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_rdi_ctrl_type = \
												md_config_parms.lp_parms.lp_rdi_ctrl_type;
		hil_sdh_core_config.md_config.ptr_core_cfg.elems[adm_core_index].lp_parms.lp_path_label = \
												md_config_parms.lp_parms.lp_path_label;
	}
	return SUCCESS;
}

/*******************************************************************************
*	Description: This function is used to set the rs se-scrambler Status register
*				 values.
*
*	Inputs:		 Data buffer, ADM Core index
*
*	Outputs:	 Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_rs_dscramb_config_set(PAL_INT_32 dscramb_val , Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[RS_CTRL].addr;
	if(RS_DESCRAMBLER_ENABLE == dscramb_val )
	{
		sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.rs_dscramb = 0x0;
	}
	else
	{
		sdh_db.core_set_parms[adm_core_index].rs_reg.rs_ctrl.reg_bits.rs_dscramb = 0x1;
	}

	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_status0.regvalue;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Rs Status register values.
*
*	Inputs:		 Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_rs_alarm_status_get(PAL_UINT_32 *rs_alm_status, Stm1_SQ_Type_E status_query_type, Sdh_ADM_Core_E adm_core_index)
{

	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr = 0;
	PAL_INT_32 *val = NULL;

	addr = sdh_mux_demux_reg_info[RS_STATUS_RC].addr;

	val = rs_alm_status;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Ms Status register values.
*
*	Inputs:		 Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_ms_alarm_status_get(PAL_INT_32 *ms_alm_status, Stm1_SQ_Type_E status_query_type, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr = 0;
	PAL_INT_32 *val = NULL;

	addr = sdh_mux_demux_reg_info[MS_STATUS_RC].addr;

	val = ms_alm_status;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 sdh_stm1_md_hp_alarm_status_poll_get(PAL_INT_32 index, PAL_UINT_32 *hp_alm_status, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 	retval=FAILURE;
	PAL_INT_32 	addr = 0;
	PAL_INT_32 	*val = NULL;
	PAL_UCHAR 	klm_index;

	addr = sdh_mux_demux_reg_info[HP_STAT_INDEX].addr;
	/*convert "index" to "KLM" */
	hal_sdh_stm1_convert_port_to_klm(index, &klm_index);
	*val = klm_index;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, *val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;

	addr = sdh_mux_demux_reg_info[HP_STATUS0].addr+(klm_index*4);;
	val  = hp_alm_status;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 sdh_stm1_md_hp_alarm_status_intr_get(PAL_INT_32 index, PAL_UINT_32 *hp_alm_status, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 	retval=FAILURE;
	PAL_INT_32 	addr = 0;
	PAL_UINT_32 	*val = NULL;

	addr = sdh_mux_demux_reg_info[HP_STATUS0].addr+(index*4);
	val  = hp_alm_status;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_hp_alarm_status_get(PAL_USHORT_16 max_hop, PAL_UINT_32 *hp_alm_status, \
													Stm1_SQ_Type_E status_query_type, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 		retval=SUCCESS;
	PAL_UCHAR 		index;

	if(hp_alm_status == NULL)
		return FAILURE;

	/* Store alarm and status register values */
	for(index=0; index<max_hop; index++ )
	{
		switch(status_query_type)
		{
			case STM1_STATUS_QUERY_POLL:
				if((retval = sdh_stm1_md_hp_alarm_status_poll_get(index, hp_alm_status, adm_core_index))<0)
				{
					return retval;
				}
				break;

			case STM1_STATUS_QUERY_INTR:
				if((retval = sdh_stm1_md_hp_alarm_status_intr_get(index, hp_alm_status, adm_core_index))<0)
				{
					return retval;
				}
				break;

			default:
				return FAILURE;
		}
	}
	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 sdh_stm1_md_lp_alarm_status_poll_get(PAL_INT_32 index, PAL_UINT_32 *lp_alm_status, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 	retval=FAILURE;
	PAL_INT_32 	addr = 0;
	PAL_UINT_32 	*val = NULL;
	PAL_UCHAR 	klm_index;

	addr = sdh_mux_demux_reg_info[LP_STAT_INDEX].addr;
	/*convert "index" to "KLM" */
	hal_sdh_stm1_convert_port_to_klm(index, &klm_index);
	*val  = klm_index;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, *val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;

	addr = sdh_mux_demux_reg_info[LP_STATUS_RC].addr+(klm_index*4);
	val  = &lp_alm_status[index];

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 sdh_stm1_md_lp_alarm_status_intr_get(PAL_INT_32 index, PAL_UINT_32 *lp_alm_status, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 	retval=FAILURE;
	PAL_INT_32 	addr = 0;//, klm_addr = 0;
	PAL_UINT_32 	*val = NULL;
	PAL_UCHAR 	klm_index;

	/*convert "index" to "KLM" */
	hal_sdh_stm1_convert_port_to_klm(index, &klm_index);

	addr = sdh_mux_demux_reg_info[LP_STATUS_RC].addr+(klm_index*4);
	val  = &lp_alm_status[index];

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_get(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}

	return retval;
}

/*******************************************************************************
*	Description:This function is used to Get the mux-demux Hp Status register values.
*
*	Inputs:		 index, Data buffer, Access Type, ADM Core index
*
*	Outputs:	Fills up the data and returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_md_lp_alarm_status_get(PAL_USHORT_16 max_lop, PAL_CHAR_8 *lp_ch_bitmap, PAL_UINT_32 *lp_alm_status, \
										Stm1_SQ_Type_E status_query_type, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 	retval=FAILURE;
	PAL_INT_32	index=0,byte_index=0;

	if(lp_alm_status == NULL)
		return FAILURE;

	for(index=0; index<max_lop; index++)
	{
		/*  Get the byte information based on channel number(index) */
		byte_index = ((index / 8) + ((index % 8) ? 1 : 0));

		if(lp_ch_bitmap[byte_index] & (0x80 >> (index % 8)))
		{
			switch(status_query_type)
			{
				case STM1_STATUS_QUERY_POLL:
					if((retval = sdh_stm1_md_lp_alarm_status_poll_get(index, lp_alm_status, adm_core_index))<0)
					{
						return retval;
					}
					break;

				case STM1_STATUS_QUERY_INTR:
					if((retval = sdh_stm1_md_lp_alarm_status_intr_get(index, lp_alm_status, adm_core_index))<0)
					{
						return retval;
					}
					break;

				default:
					return FAILURE;
			}
		}
	}
	return retval;
}


/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for ADM.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_intr_mask_set(Sdh_ADM_Core_E adm_core_index,PAL_INT_32 section)
{
	int retval=FAILURE, index=0, addr=0, val=0;
	int max_adm_intr_summary1 = 0;

	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
			max_adm_intr_summary1 = MAX_STM1_ADM_INTR_SUMMARY1;
			break;
		case SET_REGENERATOR:
			max_adm_intr_summary1 = RS_LOS_EVENT+1;
			break;

		default:
			return FAILURE;
	}

	for(index=RS_B1_EVENT;index<max_adm_intr_summary1;index++)
	{
		if( PRI_ONE == stm1_adm_intr_summary1[index].priority )
		{
			if(INTR_MASK_SET == stm1_adm_intr_summary1[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 |= \
							stm1_adm_intr_summary1[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_adm_intr_summary1[index].set_bit));
		}
		else if( PRI_TWO == stm1_adm_intr_summary1[index].priority )
		{
			if(INTR_MASK_SET == stm1_adm_intr_summary1[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 |= \
							stm1_adm_intr_summary1[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_adm_intr_summary1[index].set_bit));
		}
		else if( PRI_THREE == stm1_adm_intr_summary1[index].priority )
		{
			if(INTR_MASK_SET == stm1_adm_intr_summary1[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 |= \
							stm1_adm_intr_summary1[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_adm_intr_summary1[index].set_bit));
		}
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_1A].addr;
	val  = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_1a.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_1B].addr;
	val  = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_1b.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_1C].addr;
	val  = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_1c.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	return retval;
}

/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for ADM.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_intr_summary1_get(PAL_INT_32 *summary1)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr = 0;
	PAL_INT_32 *val = NULL;

	addr = sdh_mux_demux_reg_info[INTR_SUMMARY1].addr;

	val = summary1;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_intr_status_get(addr, val, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for ADM.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_intr_summary0_get(PAL_INT_32 *summary0)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr = 0;
	PAL_INT_32 *val = NULL;

	addr = sdh_mux_demux_reg_info[INTR_SUMMARY0].addr;

	val = summary0;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_intr_status_get(addr, val, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_md_config_get Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_rs_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[RS_INTEN_CTRL].addr;

	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.reserved		= 0x0;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.rs_b1_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.tm_new_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.tm_mis_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.tm_stable_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.rs_oof_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.rs_lof_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.reg_bits.rs_los_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_inten_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	retval = hal_sdh_stm1_intr_config_set(addr, val, 1);
	if(FAILURE == retval)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}
/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_ms_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[MS_INTEN_CTRL].addr;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.reg_bits.reserved	= 0x0;
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.reg_bits.ms_rei_ie	= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.reg_bits.ms_b2_ie	= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.reg_bits.ms_ais_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.reg_bits.ms_rdi_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.core_set_parms[adm_core_index].ms_reg.ms_inten_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}


/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_hp_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[HP_INTEN_CTRL].addr;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.reserved		= 0x0;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_b3_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_tm_new_ie	= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_rei_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_rdi_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_plm_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_tm_stable_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_tm_tim_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.hp_uneq_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.au_lop_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.reg_bits.au_ais_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_inten_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_lp_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index,Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[LP_INTEN_CTRL].addr;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.reserved		= 0x0;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_rfi_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_bip2_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_tm_new_ie	= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_rei_ie		= (UNSET_ALL == section )? 0x0 : 0x2;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_rdi_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_lofm_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_plm_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_tm_tim_ie	= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.lp_uneq_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.tu_lop_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
			sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.reg_bits.tu_ais_ie		= (UNSET_ALL == section )? 0x0 : 0x3;
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.core_set_parms[adm_core_index].lp_reg.lp_inten_ctrl.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}


/******************************************************************************
 * Description		: This function will update the interrupt path register bit
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_rs_intr_en_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[RS_STATUS_RC].addr;

	sdh_db.core_set_parms[adm_core_index].rs_reg.rs_status_rc.reg_bits.rs_path_intr_en = (UNSET_ALL == section )? 0x0 : 0x1;

	val = sdh_db.core_set_parms[adm_core_index].rs_reg.rs_status_rc.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt path register bit
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_ms_intr_en_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[MS_STATUS_RC].addr;

	sdh_db.core_set_parms[adm_core_index].ms_reg.ms_status_rc.reg_bits.ms_path_intr_en = (UNSET_ALL == section )? 0x0 : 0x1;

	val = sdh_db.core_set_parms[adm_core_index].ms_reg.ms_status_rc.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}


/******************************************************************************
 * Description		: This function will update the interrupt path register bit
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_hp_intr_en_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_mux_demux_reg_info[HP_STATUS0].addr;  // HP_STATUS0 is for VC0

	sdh_db.core_set_parms[adm_core_index].hp_reg.hp_status0.reg_bits.hp_path_intr_en = (UNSET_ALL == section )? 0x0 : 0x1;

	val = sdh_db.core_set_parms[adm_core_index].hp_reg.hp_status0.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt path register bit
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_adm_lp_intr_en_set(PAL_USHORT_16 max_lop, Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section, PAL_CHAR_8 *lp_ch_bitmap)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0,index,byte_index=0, klm_index = 0;

	addr = sdh_mux_demux_reg_info[LP_STATUS_RC].addr;  // LP_STATUS_RC

	/* Loop through all the channels */
	for( index = 0;index<max_lop;index++ )
	{
		/*  Get the byte information based on channel number(index) */
		byte_index = ((index / 8) + ((index % 8) ? 1 : 0));

		if(lp_ch_bitmap[byte_index] & (0x80 >> (index % 8)))
		{
			sdh_db.core_set_parms[adm_core_index].lp_reg.event_status_rc[index].reg_bits.lp_path_intr_en = (UNSET_ALL == section )? 0x0 : 0x1;
			val = sdh_db.core_set_parms[adm_core_index].lp_reg.event_status_rc[index].regvalue;
			hal_convert_port_to_klm(index, &klm_index);
			addr = sdh_mux_demux_reg_info[LP_STATUS_RC].addr + klm_index;
			/* Hal function to configure the value in respective register */
			if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
			{
				printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
			}
		}
	}
	return retval;
}


/******************************************************************************
 * Description		: This function will call hal function to write the provided data to the Hw
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_intr_control_set(Sdh_ADM_Core_E adm_core_index, PAL_INT_32 section)
{
	int retval=SUCCESS;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			/* Configure interrupt controller bits */
			retval = hil_sdh_stm1_adm_rs_intr_cntrl_set(adm_core_index, section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_rs_intr_cntrl_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_ms_intr_cntrl_set(adm_core_index, section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_ms_intr_cntrl_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_hp_intr_cntrl_set(adm_core_index, section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_hp_intr_cntrl_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_lp_intr_cntrl_set(adm_core_index, section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_lp_intr_cntrl_set Failed \n",__FUNCTION__);
			}
		}
		break;
		case SET_REGENERATOR:
		{
			/* Configure interrupt controller bits */
			retval = hil_sdh_stm1_adm_rs_intr_cntrl_set(adm_core_index, section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_rs_intr_cntrl_set Failed \n",__FUNCTION__);
			}
			/* Since mux-demux has been configured in REGENERATOR MODE we have to UNSET all other registers */
			retval = hil_sdh_stm1_adm_ms_intr_cntrl_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_ms_intr_cntrl_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_hp_intr_cntrl_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_hp_intr_cntrl_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_lp_intr_cntrl_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_lp_intr_cntrl_set Failed \n",__FUNCTION__);
			}
		}
		break;
		default:
		break;
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will call hal function to write the provided data to the Hw
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_adm_intr_en_set(PAL_USHORT_16 max_lop, Sdh_ADM_Core_E adm_core_index, PAL_INT_32 section,PAL_CHAR_8 *lp_ch_bitmap )
{
	int retval=SUCCESS;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			/* Configure interrupt controller bits */
			retval = hil_sdh_stm1_adm_rs_intr_en_set(adm_core_index,section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_rs_intr_en_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_ms_intr_en_set(adm_core_index,section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_ms_intr_en_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_hp_intr_en_set(adm_core_index,section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_hp_intr_en_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_lp_intr_en_set(max_lop, adm_core_index,section,lp_ch_bitmap);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_lp_intr_en_set Failed \n",__FUNCTION__);
			}
		}
		break;
		case SET_REGENERATOR:
		{
			/* Configure interrupt controller bits */
			retval = hil_sdh_stm1_adm_rs_intr_en_set(adm_core_index,section);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_rs_intr_en_set Failed \n",__FUNCTION__);
			}
			/* Since mux-demux has been configured in REGENERATOR MODE we have to UNSET all other registers */
			retval = hil_sdh_stm1_adm_ms_intr_en_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_ms_intr_en_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_hp_intr_en_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_hp_intr_en_set Failed \n",__FUNCTION__);
			}

			retval = hil_sdh_stm1_adm_lp_intr_en_set(max_lop, adm_core_index,UNSET_ALL,lp_ch_bitmap);
			if(FAILURE == retval)
			{
				printf("%s hil_sdh_stm1_adm_lp_intr_en_set Failed \n",__FUNCTION__);
			}
		}
		break;
		default:
		return FAILURE;
	}
	return retval;
}


/******************************************************************************
 * Protection Configuration
 ******************************************************************************/
 //MSP
 /******************************************************************************
  * Description	 : This function will
 					1) read the status of Stm1 performance (if enable or disable)
 					2) read the 15 min sync prob

  * Inputs     	 : pointer to buffer
  * Return Value  : SUCCESS on success/FAILURE on failure
  ******************************************************************************/

 static PAL_INT_32 sdh_stm1_protection_set(PAL_UINT_32 addr, PAL_UINT_32 val, PAL_UINT_32 access)
 {
 	PAL_INT_32  retval = 0;

 	if((retval = hal_sdh_stm1_protection_config_set(addr, val, access))<0)
 	{
 		printf("%s hal_sdh_stm1_protection_config_set Failed \n",__FUNCTION__);
 	}

 	return SUCCESS;
}

/************************************************************************************
 * Description 	:	On a STM MSP Protection Local command Request Configuration update,
 *					this function will configure the registers.
 *
 * Input		:	Command Value
 * Output		:	Success/Failure
 ***********************************************************************************/
static PAL_INT_32 sdh_stm1_prot_msp_lcr_set(PAL_UCHAR msp_lcr_val)
{
	PAL_INT_32  retval = 0;
	PAL_UINT_32 data_val, data_val_temp;
	PAL_UINT_32	address = 0;
	PAL_UINT_32 size = 0;

	printf(" Entering Function %s\n",__FUNCTION__);

	if(msp_lcr_val != sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.prot_cmd)
	{
		address = sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_CNTRL].addr;
		size	= sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_CNTRL].datasize;

		if(STM1_CLEAR_REQUEST != msp_lcr_val)
		{
			chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.prot_cmd = STM1_CLEAR_REQUEST;
			data_val_temp = chip_setparms.msp_reg.prot_fn.msp_switch.regvalue;
			retval = sdh_stm1_protection_set(address, data_val_temp, 0);
			if(retval <0)
			{
				return FAILURE;
			}
			printf(" lcr data_read_temp   :%x\n",data_val_temp);
		}

		sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.prot_cmd = msp_lcr_val;
		data_val = chip_setparms.msp_reg.prot_fn.msp_switch.regvalue;
		retval = sdh_stm1_protection_set(address, data_val, 1);
		printf(" lcr data_read   :%x\n",data_val);
		if(retval <0)
		{
			return FAILURE;
		}

		printf(" lcr prot_cmd   :%x\n",chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.prot_cmd);
	}

	printf(" Leaving Function %s\n",__FUNCTION__);
	return retval;
}

/*******************************************************************************
*	Description: This function is used to configure the Local Command Request
*				 for MSP switching
*
*	Inputs:		 Command
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_prot_msp_cmd_req_set(Sdh_Stm1_Lcr_Cmd_E local_command_req)
{
	PAL_INT_32 retval=-1;

	PAL_UCHAR local_command_val;

	switch(local_command_req)
	{
		case STM1_CLEAR_REQUEST:
			local_command_val = 0x10;
			break;

		case STM1_LOCKOUT_SWITCH:
			local_command_val = 0x08;
			break;

		case STM1_FORCED_SWITCH:
			local_command_val = 0x04;
			break;

		case STM1_MANUAL_SWITCH:
			local_command_val = 0x02;
			break;

		case STM1_EXERCISE_SWITCH:
			local_command_val = 0x01;
			break;

		default:
			return retval;
	}

	chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.prot_cmd = local_command_val;
	retval = sdh_stm1_prot_msp_lcr_set(local_command_val);
	return retval;
}

/****************************************************************************************************
*	Description: This function is used to configure the sncp switching configuration.
*				 This function breaks up the full sncp configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 SNCP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
*****************************************************************************************************/
PAL_INT_32 hil_sdh_stm1_prot_msp_config_update(Stm1_Prot_Parms_S prot_parms, PAL_UCHAR max_sncp_config)
{
	PAL_INT_32	port_cnt=0,retval=0;
	printf(" Entered Function: %s\n", __FUNCTION__);

	/* Protection Algorithm Register Settings */
	if(SDH_STM1_MSP == prot_parms.prot_algo)
	{
		chip_setparms.sncp_reg.prot_algo = 0x00;
		chip_setparms.msp_reg.prot_algo  = 0x01;
		chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot = 0x01;
	}

	for(port_cnt =0; port_cnt<max_sncp_config; port_cnt++)
	{
		chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.enable_prot =0x00;
	}

	/* MSP Register Settings*/
	chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.wtrd = prot_parms.msp_parms.msp_wait_res_period;

	if(STM1_PROTECTION_ENABLE == prot_parms.msp_parms.msp_revertive)
	{
		chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.revt_switch = 0x01;
	}
	else
	{
		chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.revt_switch = 0x00;
	}

	if(STM1_PROTECTION_PORT2_PORT1 == prot_parms.msp_parms.msp_working_prot)
	{
		chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.working_ch = 0x01;
	}
	else
	{
		chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.working_ch = 0x00;
	}

	chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.ex_err_th = prot_parms.msp_parms.msp_ex_err_defect;

	chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.dg_signal_th = prot_parms.msp_parms.msp_degrade_sig_defect;

	/* Set the msp configuation in the system */
	hil_sdh_stm1_prot_msp_config_set(prot_parms);

	printf(" Leaving Function: %s\n", __FUNCTION__);
	return retval;
}


/*******************************************************************************
*	Description: This function is used to configure the msp switching configuration.
*				 This function breaks up the full msp configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 MSP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_stm1_prot_msp_config_set(Stm1_Prot_Parms_S prot_parms)
{
	PAL_INT_32 /*port_cnt = 0,*/retval=0;
	PAL_UCHAR  max_sncp_config = 0;

	printf(" Entered Function: %s\n", __FUNCTION__ );
	max_sncp_config = prot_parms.max_sncp_config;
	/* Disable SNCP on MSP Configuration*/
	if(sdh_db.chip_setparms.sncp_reg.prot_algo != chip_setparms.sncp_reg.prot_algo)
	{
		retval = hil_sdh_stm1_prot_sncp_config_update(prot_parms, max_sncp_config, STM1_PROT_SET);
		if(retval <0)
		{
			return FAILURE;
		}
	}

	/* Write Protection Algorithm Register */
	if((sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.regvalue & 0xC000) != \
						(chip_setparms.msp_reg.prot_fn.msp_switch.regvalue & 0xC000))
	{
		if(sdh_db.chip_setparms.msp_reg.prot_algo != chip_setparms.msp_reg.prot_algo)
		{
			sdh_db.chip_setparms.msp_reg.prot_algo = chip_setparms.msp_reg.prot_algo;
			sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot = \
													chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot;
			/* NEEDED FROM VENKY : Cross Point Configuartion*/
			retval = hw_configure_stm1_cp();
			if(retval <0)
			{
				return FAILURE;
			}
		}

		printf("3.MSP sdh_prot_parms_db_notify->command regvalue = %x \n",chip_setparms.msp_reg.prot_fn.msp_switch.regvalue);
		retval = sdh_stm1_protection_set(sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_CNTRL].addr,
												chip_setparms.msp_reg.prot_fn.msp_switch.regvalue, 1);
		if(retval <0)
		{
			return FAILURE;
		}

		sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.regvalue = chip_setparms.msp_reg.prot_fn.msp_switch.regvalue;
		printf("1.MSP regvalue = %x \n",chip_setparms.msp_reg.prot_fn.msp_switch.regvalue);

	}

	printf(" Leaving Function: %s\n", __FUNCTION__ );
	return retval;

}

PAL_INT_32 hil_sdh_stm1_msp_alarm_status_get(PAL_INT_32 *msp_alarm_status, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr = 0, val = 0, access_info = 1;
	//PAL_INT_32 start_port_index = 0;
	//PAL_UCHAR  path;


	addr = sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_CNTRL].addr;

	if((retval = hal_sdh_stm1_protection_config_get(addr, &val, access_info))<0)
	{
		printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	chip_setparms.msp_reg.prot_fn.msp_switch.regvalue = (PAL_UINT_32)val;

	*msp_alarm_status = val;

	printf("msp_switch.regvalue is %x\n", chip_setparms.msp_reg.prot_fn.msp_switch.regvalue);

	return SUCCESS;
}

/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for SNCP switching.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_msp_intr_mask_set(Sdh_ADM_Core_E adm_core_index,PAL_INT_32 section)
{
	int retval=SUCCESS,index=0,addr=0,val=0;

	for(index=MSP_SW_STATE_A_EVENT;index<MAX_STM1_MSP_INTR_SUMMARY0;index++)
	{
		if( PRI_ONE == stm1_msp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_msp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 |= \
							stm1_msp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_msp_intr_summary0[index].set_bit));
		}
		else if( PRI_TWO == stm1_msp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_msp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 |= \
							stm1_msp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_msp_intr_summary0[index].set_bit));
		}
		else if( PRI_THREE == stm1_msp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_msp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 |= \
							stm1_msp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_msp_intr_summary0[index].set_bit));
		}
	}
	addr = sdh_mux_demux_reg_info[INTR_MASK_0A].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0B].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0C].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration for MSP.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_msp_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_INT_CNTRL].addr;

	sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.reg_bits.unused1			= 0;
	sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_sw_state	= (UNSET_ALL == section )? 0x0 : 0x3;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			if(STM1_ADM_MA_A == adm_core_index)
			{
				sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
			}

			if(STM1_ADM_MA_B == adm_core_index)
			{
				sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
			}
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.chip_setparms.msp_reg.intr_cntrl.cntrl_config.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_md_config_set(addr, val, adm_core_index, 1))<0)
	{
		printf("%s hal_sdh_stm1_md_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

//SNCP
/*******************************************************************************
*	Description:This function is used to Set SNCP Protection Configuration
*
*	Inputs:		Wait to Restore value
*
*	Outputs:	Returns failure or Success
********************************************************************************/
static PAL_INT_32 sdh_stm1_prot_sncp_conf_set(PAL_VOID)
{
	PAL_INT_32 retval = FAILURE, stm_prot_status = 0;

	/* Disable MSP on SNCP Configuration */
	if(sdh_db.chip_setparms.msp_reg.prot_algo != chip_setparms.msp_reg.prot_algo)
	{
		/* Write the confifguration to the hw */
		retval = sdh_stm1_protection_set(sdh_stm1_prot_reg_info[SDH_STM1_MSP_FN_CNTRL].addr,  \
												chip_setparms.msp_reg.prot_fn.msp_switch.regvalue, 1);

		if(retval <0)
		{
			return FAILURE;
		}

		sdh_db.chip_setparms.msp_reg.prot_algo = chip_setparms.msp_reg.prot_algo;
		sdh_db.chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot = chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot;
		/* NEEDED FROM VENKY */
		retval = sdh_stm1_configure_exp_cp_switch();
		if(retval <0)
		{
			return FAILURE;
		}
	}
	stm_prot_status = chip_setparms.sdh_prot_status;

	retval = sdh_stm1_configure_sncp_block(stm_prot_status);
	if(retval <0)
	{
		return FAILURE;
	}
	/* Update the main database */
//	hil_sdh_stm1_update_sncp_user_config(chip_setparms);
        return retval;      
}

static PAL_INT_32 sdh_stm1_sncp_config_verify(Sdh_Stm1_Ch_Core_Info_S *sncp_core_info, PAL_INT_32 port_cnt)
{
	/* Extract Working and Protection Path */
	sncp_core_info->working_core 		= port_core_info[port_cnt].working_core;
	sncp_core_info->working_channel 	= port_core_info[port_cnt].working_channel;

	sncp_core_info->prot_core 			= port_core_info[port_cnt].prot_core;
	sncp_core_info->prot_channel 		= port_core_info[port_cnt].prot_channel;

	if(sncp_core_info->working_core<0 || sncp_core_info->prot_core <0 || \
						sncp_core_info->working_channel<1 || sncp_core_info->prot_channel<1)
	{
		printf("############### return FAILURE due to negative value ###############\n");
		printf("## working_core = %d###\n", sncp_core_info->working_core);
		printf("## protection_core = %d###\n", sncp_core_info->prot_core);
		printf("## working_channel = %d###\n", sncp_core_info->working_channel);
		printf("## protection_channel = %d###\n", sncp_core_info->prot_channel);
		return FAILURE;
	}

	if(chip_setparms.sncp_reg.config_flag == 0)
	{
		return SNCP_CONFIG_PRESENT;
	}
	return SUCCESS;
}

static PAL_INT_32 sdh_stm1_sncp_cntrl_config(SNCP_Action_E sncp_config_path, PAL_INT_32 port_cnt)
{
	PAL_INT_32 retval = FAILURE;

	switch(sncp_config_path)
	{
		case SNCP_RX_BLOCK_WP:
		case SNCP_TX_BLOCK_WP:
		/* Set the control for SNCP Receiver Block Working path */
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.assignment = 0x0;
			break;

		case SNCP_RX_BLOCK_PP:
		case SNCP_TX_BLOCK_PP:
		/* Set the control for SNCP Receiver Block Working path */
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.assignment = 0x1;
			break;

		default:
			break;
	}

	/* Write SNCP Control Register for Working path */
	if((retval = sdh_stm1_protection_set(sdh_stm1_prot_reg_info[SDH_STM1_SNCP_FN_CNTRL].addr, \
													chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue, 0))<0)
	{
		return FAILURE;
	}
	printf("RW SDH_STM1_SNCP_FN_CNTRL Reg = %x \n",chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue);

	return SUCCESS;
}

static PAL_INT_32 sdh_stm1_sncp_ident_config(SNCP_Action_E sncp_config_path, PAL_INT_32 stm_prot_status, \
										Sdh_Stm1_Ch_Core_Info_S sncp_core_info)
{
	PAL_INT_32 retval = FAILURE;
	PAL_CHAR_8 working_core = 0, protection_core = 0;
	PAL_CHAR_8 working_channel = 0, protection_channel = 0;

	working_core 		= sncp_core_info.working_core ;
	working_channel 	= sncp_core_info.working_channel;
	protection_core		= sncp_core_info.prot_core;
	protection_channel	= sncp_core_info.prot_channel;

	/* Set Channelmap(Working Channel Number) for reference */
	if((stm_prot_status == STM1_PROT_RESET)|| (chip_setparms.sncp_reg.config_flag == -1))
	{
		/* Clear the STM ptotection channel Map before channal map update */
		chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_map = 0x0;
	}
	else
	{
		chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_map 	  = \
				e1t1_port_info[ADM_AMA_PORT_INDEX + working_core].start_port + (working_channel -1);
	}

	switch(sncp_config_path)
	{
		case SNCP_RX_BLOCK_WP:
			/* Set device id for SNCP Receiver Block */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.device_id = 0x01;
			/* Set Working Channel ID */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_id 	  = \
					e1t1_port_info[ADM_AMA_PORT_INDEX + working_core].start_port + (working_channel -1);

			break;

		case SNCP_TX_BLOCK_WP:
			/* Set device id for SNCP Receiver Block */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.device_id = 0x00;
			/* Set Working Channel ID */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_id 	  = \
					e1t1_port_info[ADM_AMA_PORT_INDEX + working_core].start_port + (working_channel -1);

			break;

		case SNCP_RX_BLOCK_PP:
			/* Set device id for SNCP Receiver Block */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.device_id = 0x01;
			/* Set Working Channel ID */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_id 	  = \
					e1t1_port_info[ADM_AMA_PORT_INDEX + protection_core].start_port + (protection_channel -1);

			break;

		case SNCP_TX_BLOCK_PP:
			/* Set device id for SNCP Receiver Block */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.device_id = 0x00;
			/* Set Working Channel ID */
			chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_id 	  = \
					e1t1_port_info[ADM_AMA_PORT_INDEX + protection_core].start_port + (protection_channel -1);

			break;

		default:
			break;
	}

	/* Write SNCP Control Register for Working path */
	if((retval = sdh_stm1_protection_set(sdh_stm1_prot_reg_info[SDH_STM1_SNCP_IDENT].addr, \
														chip_setparms.sncp_reg.prot_ident.sncp_cmd.regvalue, 0))<0)
	{
		return FAILURE;
	}
	printf("RW SDH_STM1_SNCP_IDENT Reg = %x \n",chip_setparms.sncp_reg.prot_ident.sncp_cmd.regvalue);

	return SUCCESS;
}

static PAL_INT_32 sdh_stm1_sncp_config_init(PAL_INT_32 port_cnt)
{
	PAL_INT_32 retval = FAILURE;

	chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.write_enable = 0x01;

	if((retval = sdh_stm1_protection_set(sdh_stm1_prot_reg_info[SDH_STM1_SNCP_FN_CNTRL].addr, \
												chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue, 0))<0)
	{
		return FAILURE;
	}
	printf("RW port_cnt :%d SNCP Command Reg = %x \n",port_cnt,chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue);
	return SUCCESS;
}

static PAL_INT_32 sdh_stm1_sncp_config_deinit(PAL_INT_32 port_cnt)
{
	chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.write_enable = 0x0;

	return SUCCESS;
}

/************************************************************************************
 * Description 	:	On a SNCP Configuration update and on E1-T1 Channel Map update,
 *					this function will configure receiver block of SNCP.
 *
 * Input		:	Prot status,  Port Index
 * Output		:	Success/Failure
 ***********************************************************************************/
static PAL_INT_32 sdh_stm1_configure_sncp_receiver_block(PAL_INT_32 stm_prot_status, PAL_INT_32  port_cnt)
{
	PAL_INT_32 retval = FAILURE;
	Sdh_Stm1_Ch_Core_Info_S sncp_core_info;

	/* Extract Channel and verify */
	if((retval =sdh_stm1_sncp_config_verify(&sncp_core_info, port_cnt))<0)
	{
		if(retval == SNCP_CONFIG_PRESENT)
		{
			printf("SNCP Configuration Already present, so retur without configuration \n");
			return SUCCESS;
		}
		else
			return FAILURE;
	}

	printf("********** SNCP_RECEIVER_BLOCK Working Path Configuration Start **********\n");
	/* De-Initialize SNCP Configuration */
	if((retval = sdh_stm1_sncp_config_deinit(port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Control Register */
	if((retval = sdh_stm1_sncp_cntrl_config(SNCP_RX_BLOCK_WP, port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Ident Register */
	if((retval = sdh_stm1_sncp_ident_config(SNCP_RX_BLOCK_WP, stm_prot_status, sncp_core_info))<0)
	{
		return FAILURE;
	}

	/* Initialize the SNCP Mapping */
	if((retval = sdh_stm1_sncp_config_init(port_cnt))<0)
	{
		return FAILURE;
	}
	printf("**********SNCP_RECEIVER_BLOCK Working Path Configuration End**********\n");


	printf("**********SNCP_RECEIVER_BLOCK Protection Path Configuration Start**********\n");
	/* De-Initialize SNCP Configuration */
	if((retval = sdh_stm1_sncp_config_deinit(port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Control Register */
	if((retval = sdh_stm1_sncp_cntrl_config(SNCP_RX_BLOCK_PP, port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Ident Register */
	if((retval = sdh_stm1_sncp_ident_config(SNCP_RX_BLOCK_PP, stm_prot_status, sncp_core_info))<0)
	{
		return FAILURE;
	}

	/* Initialize the SNCP Mapping */
	if((retval = sdh_stm1_sncp_config_init(port_cnt))<0)
	{
		return FAILURE;
	}
	printf("**********SNCP_RECEIVER_BLOCK Protection Path Configuration End**********\n");

	return SUCCESS;
}

/************************************************************************************
 * Description 	:	On a SNCP Configuration update and on E1-T1 Channel Map update,
 *					this function will configure Transmitter block of SNCP.
 *
 * Input		:	Prot status , port num
 * Output		:	Success/Failure
 ***********************************************************************************/
static PAL_INT_32 sdh_stm1_configure_sncp_transmitter_block(PAL_INT_32 stm_prot_status, PAL_INT_32  port_cnt)
{
	PAL_INT_32 retval = FAILURE;
	Sdh_Stm1_Ch_Core_Info_S sncp_core_info;

	/* Extract Channel and verify */
	if((retval =sdh_stm1_sncp_config_verify(&sncp_core_info, port_cnt))<0)
	{
		if(retval == SNCP_CONFIG_PRESENT)
		{
			printf("SNCP Configuration Already present, so retur without configuration \n");
			return SUCCESS;
		}
		else
			return FAILURE;
	}

	printf("********** SNCP_TRANSMITTER_BLOCK Working Path Configuration Start **********\n");
	/* De-Initialize SNCP Configuration */
	if((retval = sdh_stm1_sncp_config_deinit(port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Control Register */
	if((retval = sdh_stm1_sncp_cntrl_config(SNCP_TX_BLOCK_WP, port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Ident Register */
	if((retval = sdh_stm1_sncp_ident_config(SNCP_TX_BLOCK_WP, stm_prot_status, sncp_core_info))<0)
	{
		return FAILURE;
	}

	/* Initialize the SNCP Mapping */
	if((retval = sdh_stm1_sncp_config_init(port_cnt))<0)
	{
		return FAILURE;
	}
	printf("**********SNCP_RECEIVER_BLOCK Working Path Configuration End**********\n");


	printf("**********SNCP_RECEIVER_BLOCK Protection Path Configuration Start**********\n");
	/* De-Initialize SNCP Configuration */
	if((retval = sdh_stm1_sncp_config_deinit(port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Control Register */
	if((retval = sdh_stm1_sncp_cntrl_config(SNCP_TX_BLOCK_PP, port_cnt))<0)
	{
		return FAILURE;
	}

	/* Configure the SNCP Ident Register */
	if((retval = sdh_stm1_sncp_ident_config(SNCP_TX_BLOCK_PP, stm_prot_status, sncp_core_info))<0)
	{
		return FAILURE;
	}

	/* Initialize the SNCP Mapping */
	if((retval = sdh_stm1_sncp_config_init(port_cnt))<0)
	{
		return FAILURE;
	}
	printf("**********SNCP_RECEIVER_BLOCK Protection Path Configuration End**********\n");

	return SUCCESS;
}

/************************************************************************************
 * Description 	:	On a STM SNCP Protection Local command Request Configuration update,
 *					this function will configure the registers.
 *
 * Input		:	Command Value
 * Output		:	Success/Failure
 ***********************************************************************************/
static PAL_INT_32 sdh_stm1_prot_sncp_lcr_set(PAL_UCHAR *sncp_lcr_cmd, PAL_UCHAR max_sncp_config)
{
	PAL_INT_32 retval = 0,port_cnt = 0;

	printf("Entering Function %s\n",__FUNCTION__);

	for(port_cnt=0; port_cnt<max_sncp_config; port_cnt++)
	{
		if(sncp_lcr_cmd[port_cnt]!= \
				chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.prot_cmd)
		{
			printf(" SNCP LCR Port Count = %d\n",port_cnt);
			printf(" sncp_lcr_cmd[port_cnt] = %d\n",sncp_lcr_cmd[port_cnt]);
			sdh_db.chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.prot_cmd = \
																				sncp_lcr_cmd[port_cnt];

			retval = sdh_stm1_configure_sncp_receiver_block(STM1_PROT_SET, port_cnt);
			if(retval <0)
			{
				printf("sdh_stm1_configure_sncp_receiver_block() failed with retval=%d\n",retval);
				return FAILURE;
			}

			retval = sdh_stm1_configure_sncp_transmitter_block(STM1_PROT_SET, port_cnt);
			if(retval <0)
			{
				printf("sdh_stm1_configure_sncp_transmitter_block() failed with retval=%d\n",retval);
				return FAILURE;
			}
		}
	}

	printf(" port_cnt%d : sncp_lcr prot_cmd   :%x\n",port_cnt,chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.prot_cmd);
	printf(" port_cnt%d : lcr register sncp is %x\n",port_cnt,chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue);

	return retval;
}

/*******************************************************************************
*	Description: This function is used to configure the Local Command Request
*				 for SNCP switching
*
*	Inputs:		 Command
*
*	Outputs:	 Returns failure or Success
********************************************************************************/
PAL_INT_32 hil_sdh_prot_sncp_cmd_req_set(PAL_UCHAR *local_command_req, PAL_UCHAR max_sncp_config)
{
	PAL_INT_32 retval=-1,port_cnt=0;
	PAL_UCHAR local_command_val[MAX_NO_STM_PROTECTION];

	memset(local_command_val, 0, sizeof(local_command_val));

	for(port_cnt=0; port_cnt<max_sncp_config; port_cnt++)
	{
		switch(local_command_req[port_cnt])
		{
			case STM1_CLEAR_REQUEST:
			local_command_val[port_cnt] = 0x08;
			printf("STM1_CLEAR_REQUEST\n");
			break;

			case STM1_LOCKOUT_SWITCH:
			local_command_val[port_cnt] = 0x04;
			printf("STM1_LOCKOUT_SWITCH\n");
			break;

			case STM1_FORCED_SWITCH:
			local_command_val[port_cnt] = 0x02;
			printf("STM1_FORCED_SWITCH\n");
			break;

			case STM1_MANUAL_SWITCH:
			local_command_val[port_cnt] = 0x01;
			printf("STM1_MANUAL_SWITCH\n");
			break;

			case STM1_NO_REQUEST:
			local_command_val[port_cnt] = 0x00;
			printf("STM1_NO_REQUEST\n");
			break;

			default:
			return retval;
		}
	}
	retval = sdh_stm1_prot_sncp_lcr_set(local_command_val, max_sncp_config );
	return retval;
}

/*****************************************************************************************
*	Description: This function is used to configure the sncp switching configuration.
*				 This function breaks up the full sncp configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 SNCP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
******************************************************************************************/
static PAL_VOID extract_sncp_channel_core(Sdh_Sncp_Parms_S *sncp_prot_data, Sdh_Stm1_Ch_Core_Info_S *port_core_info, PAL_UCHAR port_cnt)
{
	PAL_INT_32 working_channel = 0;
	PAL_INT_32 prot_channel = 0;

	working_channel = sncp_prot_data[port_cnt].sncp_couplet.working_channel;
	prot_channel	= sncp_prot_data[port_cnt].sncp_couplet.prot_channel;

	switch(working_channel/1000)
	{
		case ADM_AMA_PORT:
			port_core_info[port_cnt].working_core = 0;
			break;

		case ADM_AMB_PORT:
			port_core_info[port_cnt].working_core = 1;
			break;

		default:
			break;
	}

	switch(prot_channel/1000)
	{
		case ADM_AMA_PORT:
			port_core_info[port_cnt].prot_core = 0;
			break;

		case ADM_AMB_PORT:
			port_core_info[port_cnt].prot_core = 1;
			break;

		default:
			break;
	}

	port_core_info[port_cnt].working_channel 	= (working_channel%1000);
	port_core_info[port_cnt].prot_channel 		= (prot_channel%1000);
}

/*****************************************************************************************
*	Description: This function is used to configure the sncp switching configuration.
*				 This function breaks up the full sncp configuration, parameter wise
*				 and send it to the HAL lib.
*
*	Inputs:		 SNCP Configuration, ADM Core num
*
*	Outputs:	 Returns failure or Success
******************************************************************************************/
PAL_INT_32 hil_sdh_stm1_prot_sncp_config_update(Stm1_Prot_Parms_S prot_parms, PAL_UCHAR max_sncp_config, PAL_INT_32 stm_prot_status)
{
	PAL_UCHAR port_cnt = 0;
	printf(" Entered Function: %s\n", __FUNCTION__ );

	/* Maximum number of protection mappings configured. */
	port_cnt = max_sncp_config;

	/* Protection Algorithm Register Settings */
	chip_setparms.sncp_reg.prot_algo = 0x01;
	chip_setparms.msp_reg.prot_algo  = 0x00;
	chip_setparms.msp_reg.prot_fn.msp_switch.reg_bits.enable_prot = 0x00;

	chip_setparms.sdh_prot_status = stm_prot_status;

	/* Update the SNCP configuration for each mapping. */
	for(port_cnt = 0;port_cnt < max_sncp_config;port_cnt++)
	{
		/* Setting for "Wait to restore" */
		chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.wtrd = prot_parms.sncp_parms.sncp_prot_data[port_cnt].sncp_wait_res_period;

		/* Setting for "Enable Protection" */
		if(STM1_PROTECTION_ENABLE == prot_parms.sncp_parms.sncp_prot_data[port_cnt].enable_protection)
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.enable_prot = 0x01;
		}
		else
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.enable_prot = 0x00;
		}

		/* Setting for "Revertive Switching" */
		if(STM1_PROTECTION_ENABLE == prot_parms.sncp_parms.sncp_prot_data[port_cnt].revertive)
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.revt_switch = 0x01;
		}
		else
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.revt_switch = 0x00;
		}

		/* Setting for "Sncp Mode" */
		if(STM1_PROTECTION_SNCP_I == prot_parms.sncp_parms.sncp_prot_data[port_cnt].mode)
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.mode = 0x01;
		}
		else
		{
			chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.mode =0x00;
		}

		chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.ex_err_th = \
																	prot_parms.sncp_parms.sncp_prot_data[port_cnt].ex_err_defect;

		chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.dg_signal_th = \
																	prot_parms.sncp_parms.sncp_prot_data[port_cnt].degrade_sig_defect;

		chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.reg_bits.hold_off_time= \
																	prot_parms.sncp_parms.sncp_prot_data[port_cnt].hold_off_time;

		chip_setparms.sncp_reg.config_flag	=	prot_parms.sncp_parms.sncp_prot_data[port_cnt].sncp_couplet.config_flag;

		extract_sncp_channel_core(prot_parms.sncp_parms.sncp_prot_data, port_core_info, port_cnt);
	}
	/* Update SNCP configuration */
	sdh_stm1_prot_sncp_conf_set();

	printf(" Leaving Function: %s\n", __FUNCTION__ );

    return SUCCESS;
}

static PAL_INT_32 sdh_stm1_configure_sncp_block(PAL_INT_32 stm_prot_status)
{
	PAL_INT_32 retval = FAILURE;
	PAL_UCHAR max_sncp_config=0;
    PAL_INT_32 port_cnt=0;

	printf(" Entering Function %s\n",__FUNCTION__);
	max_sncp_config = chip_setparms.max_sncp_config;
	//printf("maximum mappings in opt_stm_framer.c is %d\n",max_sncp_config);

	for(port_cnt=0;port_cnt<max_sncp_config;port_cnt++)
	{
		retval = sdh_stm1_configure_sncp_receiver_block(stm_prot_status, port_cnt);
		if(retval <0)
		{
			printf("sdh_stm1_configure_sncp_receiver_block failed with retval=%d\n",retval);
			return FAILURE;
		}

		retval = sdh_stm1_configure_sncp_transmitter_block(stm_prot_status, port_cnt);
		if(retval <0)
		{
			printf("sdh_stm1_configure_sncp_transmitter_block failed with retval=%d\n",retval);
			return FAILURE;
		}
	}

	printf(" Leaving Function %s\n",__FUNCTION__);
	return retval;
}


PAL_INT_32 hil_sdh_stm1_sncp_alarm_summary_poll_get(Sdh_Prot_Sncp_Ident_U *sncp_ident)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 addr1 = 0, addr2 = 0, val = 0, access_info = 1;
	PAL_INT_32 rmw_val = 0x7FFFFFFF;
	Rmw_Opt_E  opt_rmw = AND_OPT;

	addr1 = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_FN_CNTRL].addr;
	addr2 = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_FN_CNTRL].addr;

	if((retval = hal_sdh_stm1_protection_config_get_set(addr1, addr2, rmw_val, opt_rmw, access_info)) < 0)
	{
		printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n", __FUNCTION__);
		return FAILURE;
	}

	addr1 = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_IDENT].addr;

	if((retval = hal_sdh_stm1_protection_config_get(addr1, &val, access_info)) < 0)
	{
		printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n", __FUNCTION__);
		return FAILURE;
	}
	sncp_ident->regvalue = (PAL_UINT_32)val;

	return SUCCESS;
}

PAL_INT_32 sdh_stm1_sncp_alarm_status_poll_get(PAL_INT_32 *sncp_alarms_info, Stm1_Prot_Parms_S prot_parms, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 		retval = FAILURE;
	PAL_INT_32 		addr = 0, val = 0, access_info = 1;
	PAL_UCHAR  		core, channel;
	PAL_INT_32 		port_cnt = 0;
	PAL_INT_32 		start_port_index = 0;
	SNCP_Path_E  	path;

	addr 	= sdh_stm1_prot_reg_info[SDH_STM1_SNCP_IDENT].addr;

	if((retval = hal_sdh_stm1_protection_config_get(addr, &val, access_info))<0)
	{
		printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
		return FAILURE;
	}

	start_port_index 	= e1t1_port_info[ADM_AMA_PORT_INDEX + adm_core_index].start_port;
	chip_setparms.sncp_reg.prot_ident.sncp_cmd.regvalue = (PAL_UINT_32)val;
	printf("sncp_ident.regvalue is %x\n", chip_setparms.sncp_reg.prot_ident.sncp_cmd.regvalue);

	/* Select the data path to RXLUT */
	chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.device_id = 1;

	for(path = 0; path<MAX_SNCP_PATH_TYPE; path++)
	{
		for(port_cnt = 0; port_cnt<prot_parms.max_sncp_config; port_cnt++)
		{
			extract_sncp_channel_core(prot_parms.sncp_parms.sncp_prot_data, port_core_info, port_cnt);

			switch(path)
			{
				case WORKING_PATH:
					core 	= port_core_info[port_cnt].working_core;
					channel = port_core_info[port_cnt].working_channel;

					break;

				case PROTETION_PATH:
					core 	= port_core_info[port_cnt].prot_core;
					channel = port_core_info[port_cnt].prot_channel;

					break;

				default:
					return FAILURE;
			}

			if(core == adm_core_index)
			{
				if(channel<1)
				{
					return FAILURE;
				}

				/* Set Read Enable Bit */
				sdh_stm1_sncp_config_deinit(port_cnt);
				/* Get the Value to configure the channel in SNCP command Register */
				chip_setparms.sncp_reg.prot_ident.sncp_cmd.reg_bits.ch_id = start_port_index + (channel -1);;
				addr = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_IDENT].addr;
				val  = chip_setparms.sncp_reg.prot_ident.sncp_cmd.regvalue;

				if((retval = hal_sdh_stm1_protection_config_set(addr, val, access_info))<0)
				{
					printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
					return FAILURE;
				}

				/* Configure Read Enable */
				addr = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_FN_CNTRL].addr;
				val	 = chip_setparms.sncp_reg.prot_fn[port_cnt].sncp_conf.regvalue;
				if((retval = hal_sdh_stm1_protection_config_set(addr, val, access_info))<0)
				{
					printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
					return FAILURE;
				}

				/*Read SNCP Alarm Status */
				addr = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_ALARM_STATUS].addr;
				if((retval = hal_sdh_stm1_protection_config_get(addr, &val, access_info))<0)
				{
					printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
					return FAILURE;
				}
				chip_setparms.sncp_reg.prot_alarm.alarm_status.regvalue = val;

				sncp_alarms_info[channel-1] = val;
			}
		}
	}
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_sncp_alarm_status_intr_get(PAL_INT_32 *sncp_alarms_info, Stm1_Prot_Parms_S prot_parms, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 		retval = FAILURE;
	PAL_INT_32 		addr = 0, val = 0, access_info = 1;
	PAL_UCHAR  		core, channel;
	PAL_INT_32 		port_cnt = 0;
	SNCP_Path_E  	path;

	for(path = 0; path<MAX_SNCP_PATH_TYPE; path++)
	{
		for(port_cnt = 0; port_cnt<prot_parms.max_sncp_config; port_cnt++)
		{
			extract_sncp_channel_core(prot_parms.sncp_parms.sncp_prot_data, port_core_info, port_cnt);

			switch(path)
			{
				case WORKING_PATH:
					core 	= port_core_info[port_cnt].working_core;
					channel = port_core_info[port_cnt].working_channel;

					break;

				case PROTETION_PATH:
					core 	= port_core_info[port_cnt].prot_core;
					channel = port_core_info[port_cnt].prot_channel;

					break;

				default:
					return FAILURE;
			}

			if(core == adm_core_index)
			{
				if(channel<1)
				{
					return FAILURE;
				}
				addr = sdh_stm1_prot_reg_info[STM1_SNCP_INT_EVENT_STATUS_RC].addr+((channel-1)*4);
				if((retval = hal_sdh_stm1_protection_config_get(addr, &val, access_info))<0)
				{
					printf(" %s hal_sdh_stm1_pm_status_get() returned FAILURE \n",__FUNCTION__);
					return FAILURE;
				}
				chip_setparms.sncp_reg.prot_alarm.alarm_status.regvalue = val;

				sncp_alarms_info[channel-1] = val;
			}
		}

	}
	return retval;
}


PAL_INT_32 hil_sdh_stm1_sncp_alarm_status_get(PAL_INT_32 *sncp_alarms_info, Stm1_Prot_Parms_S prot_parms, \
												Stm1_SQ_Type_E status_query_type, Sdh_ADM_Core_E adm_core_index)
{
	PAL_INT_32 retval = FAILURE;

	switch(status_query_type)
	{
		case STM1_STATUS_QUERY_POLL:

			if((retval = sdh_stm1_sncp_alarm_status_poll_get(sncp_alarms_info, prot_parms, adm_core_index))<0)
			{
				printf(" %s sdh_stm1_sncp_alarm_status_poll_get() returned FAILURE \n",__FUNCTION__);
				return FAILURE;
			}
			break;

		case STM1_STATUS_QUERY_INTR:
			if((retval = sdh_stm1_sncp_alarm_status_intr_get(sncp_alarms_info, prot_parms, adm_core_index))<0)
			{
				return retval;
			}
			break;

		default:
			return FAILURE;
	}

	return SUCCESS;
}

/******************************************************************************
 * Description		: This function will configure the Interrupt Mask TDM Summary
 *					  Register for SNCP switching.
 * Inputs     		:
 * Output    		: val
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 hil_sdh_stm1_sncp_intr_mask_set(Sdh_ADM_Core_E adm_core_index, PAL_INT_32 section)
{
	int retval=SUCCESS,index=0,addr=0,val=0;

	for(index=SNCP_SW_STATE_A_BIT_SET;index<MAX_STM1_SNCP_INTR_SUMMARY0;index++)
	{
		if( PRI_ONE == stm1_sncp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_sncp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 |= \
							stm1_sncp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_sncp_intr_summary0[index].set_bit));
		}
		else if( PRI_TWO == stm1_sncp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_sncp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 |= \
							stm1_sncp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_sncp_intr_summary0[index].set_bit));
		}
		else if( PRI_THREE == stm1_sncp_intr_summary0[index].priority )
		{
			if(INTR_MASK_SET == stm1_sncp_intr_summary0[index].mask_app)
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 |= \
							stm1_sncp_intr_summary0[index].set_bit;
			else
				sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.reg_bits.tdmIntrSummary0 &= \
							(~(stm1_sncp_intr_summary0[index].set_bit));
		}
	}
	addr = sdh_mux_demux_reg_info[INTR_MASK_0A].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0a.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0B].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0b.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 0))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}

	addr = sdh_mux_demux_reg_info[INTR_MASK_0C].addr;
	val = sdh_db.core_set_parms[adm_core_index].mask_reg.mask_0c.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_intr_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
		return FAILURE;
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt Controller register
 *					  as per configuration for SNCP.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_sncp_intr_cntrl_set(Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section)
{
	PAL_INT_32 retval=FAILURE;
	PAL_INT_32 addr=0,val=0;

	addr = sdh_stm1_prot_reg_info[SDH_STM1_SNCP_INT_CNTRL].addr;

	sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.unused1				= 0;
	sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.unused2				= 0;
	switch(section)
	{
		case SET_ALL:
		case UNSET_ALL:
		{
			if(STM1_ADM_MA_A == adm_core_index)
			{
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_pp_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_pp_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_wp_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_wp_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_a_sw_state	= (UNSET_ALL == section )? 0x0 : 0x3;
			}

			if(STM1_ADM_MA_B == adm_core_index)
			{
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_pp_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_pp_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_wp_sd		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_wp_sf		= (UNSET_ALL == section )? 0x0 : 0x3;
				sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.reg_bits.adm_b_sw_state	= (UNSET_ALL == section )? 0x0 : 0x3;
			}
		}
		break;
		default:
			return FAILURE;
	}
	val = sdh_db.chip_setparms.sncp_reg.intr_cntrl.cntrl_config.regvalue;

	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_protection_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
	}
	return retval;
}

/******************************************************************************
 * Description		: This function will update the interrupt path register bit
 *					  as per configuration.
 * Inputs     		: core, section
 * Output    		: none
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_sncp_intr_en_set(PAL_USHORT_16 max_lop, Sdh_ADM_Core_E adm_core_index, Stm1_Mask_State_E section, PAL_CHAR_8 *lp_ch_bitmap)
{
	PAL_INT_32 retval=SUCCESS;
	PAL_INT_32 addr=0,val=0,index = 0,byte_index=0,klm_index=0;

	addr = sdh_stm1_prot_reg_info[STM1_SNCP_INT_EVENT_STATUS_RC].addr;

	/* Loop through all the channels */
	for( index = 0;index<max_lop;index++ )
	{
		/*  Get the byte information based on channel number(index) */
		byte_index = ((index / 8) + ((index % 8) ? 1 : 0));

		if(lp_ch_bitmap[byte_index] & (0x80 >> (index % 8)))
		{
			switch(section)
			{
				case SET_ALL:
				case UNSET_ALL:
				{
					if(STM1_ADM_MA_A == adm_core_index)
					{

						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_a_pp_sd_rc 		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_a_wp_sf_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_a_wp_sd_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_a_wp_sf_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_a_sw_state_rc 	= (UNSET_ALL == section )? 0x0 : 0x1;
					}
					else if(STM1_ADM_MA_B == adm_core_index)
					{

						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_b_pp_sd_rc 		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_b_wp_sf_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_b_wp_sd_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_b_wp_sf_rc		= (UNSET_ALL == section )? 0x0 : 0x1;
						sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.reg_bits.adm_b_sw_state_rc 	= (UNSET_ALL == section )? 0x0 : 0x1;
					}
				}
				break;
				default:
					return FAILURE;
			}

			hal_convert_port_to_klm(index, &klm_index);
			addr = sdh_stm1_prot_reg_info[STM1_SNCP_INT_EVENT_STATUS_RC].addr + klm_index;
			val  = sdh_db.chip_setparms.sncp_reg.event_status[index].es_config.regvalue;
			/* Hal function to configure the value in respective register */
			retval = hal_sdh_stm1_protection_config_set(addr, val, 1);
			if(FAILURE == retval)
			{
				printf("%s hal_sdh_stm1_intr_config_set Failed \n",__FUNCTION__);
			}
		}
	}
	return retval;
}

/******************************************************************************
 * Description :This function will be used to update crosspoint mapping to
 *				hardware register format.
 * Inputs      :Pointer to the crosspoint and hardware data structure, valid,
 *				delete, and unused map count.
 * Outputs     :NA
 ******************************************************************************/
static PAL_VOID sdh_stm1_convert_cp_map_hw_comb_map(hw_stm1_chmap_db_S *hw_stm1_chmap_db,\
							PAL_INT_32 *map_array, PAL_INT_32 map_cnt, PAL_INT_32 valid_map_cnt, PAL_INT_32 delete_map_cnt)
{
	if(map_array[COMBINATION_MAPPING] == map_array[COMBINATION_MAPPING+1])
	{
		hw_stm1_chmap_db->input_ports[hw_stm1_chmap_db->hw_map_cnt]  	= map_array[COMBINATION_MAPPING];
		hw_stm1_chmap_db->output_ports[hw_stm1_chmap_db->hw_map_cnt] 	= map_array[COMBINATION_MAPPING+1];
		if(map_cnt<delete_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = 1;
		}
		else if(map_cnt>=valid_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = -1;
		}
		hw_stm1_chmap_db->hw_map_cnt++;
	}
	else
	{
		hw_stm1_chmap_db->input_ports[hw_stm1_chmap_db->hw_map_cnt]  	= map_array[COMBINATION_MAPPING];
		hw_stm1_chmap_db->output_ports[hw_stm1_chmap_db->hw_map_cnt] 	= map_array[COMBINATION_MAPPING+1];
		if(map_cnt<delete_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = 1;
		}
		else if(map_cnt>=valid_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = -1;
		}
		hw_stm1_chmap_db->hw_map_cnt++;

		hw_stm1_chmap_db->input_ports[hw_stm1_chmap_db->hw_map_cnt]  	= map_array[COMBINATION_MAPPING+1];
		hw_stm1_chmap_db->output_ports[hw_stm1_chmap_db->hw_map_cnt] 	= map_array[COMBINATION_MAPPING];
		if(map_cnt<delete_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = 1;
		}
		else if(map_cnt>=valid_map_cnt)
		{
			hw_stm1_chmap_db->disable_cp[hw_stm1_chmap_db->hw_map_cnt] = -1;
		}
		hw_stm1_chmap_db->hw_map_cnt++;
	}
}

/******************************************************************************
 * Description :This function will be used to convert the port mapping from
 *				internal port number to the hardware port number.
 * Inputs      :Pointer to the hardware data structure.
 *
 * Outputs     :NA
 ******************************************************************************/
static PAL_VOID sdh_stm1_convert_cp_num_to_hw_num(hw_stm1_chmap_db_S *hw_stm1_chmap_db)
{
	PAL_INT_32 map_cnt = 0;
	PAL_INT_32 port_index = 0;

	for(map_cnt = 0; map_cnt<hw_stm1_chmap_db->hw_map_cnt; map_cnt++)
	{
		for(port_index=0; port_index<MAX_STM1_CP_PORT_INDEX; port_index++)
		{
			if((((hw_stm1_chmap_db->input_ports[map_cnt])/(stm1_port_info[port_index].index_offset)) == 1) && \
							(((hw_stm1_chmap_db->input_ports[map_cnt])%(stm1_port_info[port_index].index_offset))<10))
			{
				hw_stm1_chmap_db->input_ports[map_cnt]  = ((hw_stm1_chmap_db->input_ports[map_cnt])%10) + \
																			(stm1_port_info[port_index].start_port-1);
				hw_stm1_chmap_db->input_index[map_cnt]	= port_index;
			}
			if((((hw_stm1_chmap_db->output_ports[map_cnt])/(stm1_port_info[port_index].index_offset)) == 1) && \
							(((hw_stm1_chmap_db->output_ports[map_cnt])%(stm1_port_info[port_index].index_offset))<10))
			{
				hw_stm1_chmap_db->output_ports[map_cnt]  = ((hw_stm1_chmap_db->output_ports[map_cnt])%10) + \
																			(stm1_port_info[port_index].start_port-1);
				hw_stm1_chmap_db->output_index[map_cnt]	= port_index;
			}
		}
	}
}

/******************************************************************************
 * Description :This function will be used to update crosspoint mapping to
 *				hardware register format.
 * Inputs      :Pointer to the crosspoint and hardware data structure, valid,
 *				delete, and unused map count.
 * Outputs     :NA
 ******************************************************************************/
static PAL_VOID sdh_stm1_convert_cp_map_to_hw_map(stm1_cp_map_S *stm1_cp_map, hw_stm1_chmap_db_S *hw_stm1_chmap_db,\
															PAL_INT_32 valid_map_cnt, PAL_INT_32 delete_map_cnt, PAL_INT_32 unused_map_cnt)
{
	PAL_INT_32 map_cnt = 0, sub_map_cnt = 0;
	PAL_INT_32 map_array[MAX_SUB_ELEMENTS];

	for(map_cnt = 0; map_cnt<(valid_map_cnt+unused_map_cnt); map_cnt++)
	{
		memset(map_array, -1, sizeof(map_array));
		for(sub_map_cnt = 0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
		{
			map_array[stm1_cp_map[map_cnt].map_status[sub_map_cnt]] = stm1_cp_map[map_cnt].map_array[sub_map_cnt];
		}

		/* Convert the User Mappings to Hardware Mappings */
		sdh_stm1_convert_cp_map_hw_comb_map(hw_stm1_chmap_db, map_array, map_cnt, valid_map_cnt, delete_map_cnt);

		/* In Future if any Mapping type other than combinational mapping
		   needs to be added then add it here in each individual functions */
		/* Convert the Internal Numbering to Hardware Numbering */
	}
}

/******************************************************************************
 * Description :This function will be used to update crosspoint mapping to
 *				default(Disable) values.
 * Inputs      :N/A
 *
 * Outputs     :NA
 ******************************************************************************/
static PAL_INT_32 sdh_stm1_cp_default(PAL_VOID)
{
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpa = 0x08;
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpb = 0x08;
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_a  = 0x08;
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_b  = 0x08;
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_a   = 0x08;
	sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_b   = 0x08;
	return SUCCESS;
}

/******************************************************************************
 * Description :This function will be used to update crosspoint mapping to
 *				hardware register format.
 * Inputs      :Pointer to the crosspoint and hardware data structure, valid,
 *				delete, and unused map count.
 * Outputs     :NA
 ******************************************************************************/
static PAL_INT_32 sdh_stm1_cp_config(hw_stm1_chmap_db_S *hw_stm1_chmap_db)
{
	PAL_INT_32 map_cnt = 0;
	PAL_INT_32 port_index = 0;
	STM1_CP_HW_Port_E hw_port_index = 0;
	PAL_INT_32 outport_val = 0;
	PAL_INT_32 inport_val  = 0;

	/* Initialize the memory before configuration */
	memset(&sdh_db.chip_setparms.cp_reg.cp_config.regvalue, 0, sizeof(PAL_UINT_32));

	/* Reset all the configuration values as disable */
	sdh_stm1_cp_default();
	for(map_cnt = 0; map_cnt<hw_stm1_chmap_db->hw_map_cnt; map_cnt++)
	{
		for(port_index = 0; port_index<MAX_CP_PATH_TYPE; port_index++)
		{
			if(INPUT_PATH == port_index)
			{
				if(hw_stm1_chmap_db->disable_cp[map_cnt])
					outport_val = 0x08;
				else
					outport_val = hw_stm1_chmap_db->output_ports[map_cnt];

				hw_port_index = hw_stm1_chmap_db->input_index[map_cnt];
				switch(hw_port_index)
				{
					case STM1_MAC_FPA_PORT_INDEX:
						sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpa = outport_val;
						break;

					case STM1_MAC_FPB_PORT_INDEX:
						sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpb = outport_val;
						break;

					case STM1_MA_PORT_INDEX:
						if(hw_stm1_chmap_db->input_ports[map_cnt] == stm1_port_info[hw_port_index].start_port)
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_a = outport_val;
						else
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_b = outport_val;
						break;

					case STM1_ADM_MA_PORT_INDEX:
						if(hw_stm1_chmap_db->input_ports[map_cnt] == stm1_port_info[hw_port_index].start_port)
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_a = outport_val;
						else
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_b = outport_val;
						break;

					default:
						return FAILURE;
				}
			}
			else
			{
				if(hw_stm1_chmap_db->disable_cp[map_cnt])
					inport_val = 0x08;
				else
					inport_val = hw_stm1_chmap_db->input_ports[map_cnt];

				hw_port_index = hw_stm1_chmap_db->output_index[map_cnt];
				switch(hw_port_index)
				{
					case STM1_MAC_FPA_PORT_INDEX:
						sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpa = inport_val;
						break;

					case STM1_MAC_FPB_PORT_INDEX:
						sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.mac_fpb = inport_val;
						break;

					case STM1_MA_PORT_INDEX:
						if(hw_stm1_chmap_db->output_ports[map_cnt] == stm1_port_info[hw_port_index].start_port)
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_a = inport_val;
						else
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.stm1_b = inport_val;
						break;

					case STM1_ADM_MA_PORT_INDEX:
						if(hw_stm1_chmap_db->output_ports[map_cnt] == stm1_port_info[hw_port_index].start_port)
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_a = inport_val;
						else
							sdh_db.chip_setparms.cp_reg.cp_config.reg_bits.adm_b = inport_val;
						break;

					default:
						return FAILURE;
				}
			}
		}
	}
    return SUCCESS;
}
/******************************************************************************
 * Description :This function will configure the cross point mapping to hardware
 *              it deletes the previous map which not presnt in preset map
 *				and configure the new mapping that been newly added.
 *
 * Inputs      :Crosspoint mapping Valid, delete and unused Map count, bootup type
 *
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 hil_sdh_stm1_crosspoint_config_set(stm1_cp_map_S *stm1_cp_map, int valid_map_cnt, int delete_map_cnt, \
																		int unused_map_cnt, int bootup_type)
{
	hw_stm1_chmap_db_S 	hw_stm1_chmap_db;
	//PAL_INT_32 			hw_map_cnt = 0, hw_del_map_cnt;
	PAL_INT_32 			addr = 0, val =0;
	//PAL_INT_32 			cnt = 0; //queue_cnt = 0;
	PAL_INT_32 			retval = -1;//map_cnt = 0;

	memset(&hw_stm1_chmap_db, 0, sizeof(hw_stm1_chmap_db));

	/* Convert the User Mappings to Hardware Mappings */
	sdh_stm1_convert_cp_map_to_hw_map(stm1_cp_map, &hw_stm1_chmap_db, valid_map_cnt, delete_map_cnt, unused_map_cnt);

	/* Convert the Internal Numbering to Hardware Numbering */
	sdh_stm1_convert_cp_num_to_hw_num(&hw_stm1_chmap_db);

	/* Get the Crosspoint Hardware Configuration Value */
	if((retval = sdh_stm1_cp_config(&hw_stm1_chmap_db))<0)
	{
		printf("#### sdh_stm1_crosspoint_config failed #####\n");
		return retval;
	}

	addr = sdh_stm1_cp_reg_info[SDH_MSM_CROSS_POINT_SELECT].addr;
	val  = sdh_db.chip_setparms.cp_reg.cp_config.regvalue;
	/* Hal function to configure the value in respective register */
	if((retval = hal_sdh_stm1_cp_config_set(addr, val, 1))<0)
	{
		printf("%s hal_sdh_stm1_cp_config_set Failed \n",__FUNCTION__);
		return retval;
	}

	return retval;
}

