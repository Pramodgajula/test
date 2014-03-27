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
 * Version 1.0   (22/Jul/2013) Mujahid Ali
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
#include "../core/include/sdh_stm1_enums.h"
#include "../core/include/sdh_stm1_pm_data_struct.h"
#include "sdh_stm1_config_int_ext_lib.h"
#include "sdh_stm1_hil.h"
#include "sdh_stm1_cel.h"
#include "ces_common.h"

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 ******************************************************************************/

/* STM Internal Configuration database */
Stm1_Config_S			sdh_config_db;

/*********** Global Local Db ************/
Stm1_Chip_Config_S	chip_config;

/*********** Client Data Structure  ************/
gal_socket_client_S	galPtr;

/*********** Global Default Values ************/

extern Stm1_Sdh_Hw_Max_Interface_S		sdh_stm1_hw_cfg;

/* STM Default Configuration database */
extern Stm1_Config_S			def_stm1_config_db;

stm1_cp_config_hw_S		stm1_cp_data;
stm1_param_map_list_S	stm1_param_map_list;
stm1_cp_map_filter_S	stm1_cp_map_filter;
e1t1_cp_map_filter_S	e1t1_cp_map_filter;
stm1_cp_limit_S		stm1_cp_limit;
extern signed int  sdh_stm1_mux_cfg_chg;

#define E1T1_CP_INIT(enume,interface,interface_type,interface_offset,interface_locn,port_no_locn,max_ports,priority,port_str)    \
                            {interface,interface_type,interface_locn,port_no_locn,interface_offset,max_ports,priority,port_str},
E1T1_CP_Info_S e1t1_cp_info[]={E1T1_CP_INTERFACE};
#undef E1T1_CP_INIT
/******************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/

/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/
/******************************************************************************
 * Description :This function does binary search to find out an element from a
 * 				given array.
 * Inputs 	   :PAL_UINT_32 search_array[] -> array which will be searched in.
 *				PAL_UINT_32 array_size -> Size of the array to be searche in.
 *				PAL_UINT_32 search_alement -> element to be searched.
 * Outputs	   :NA.
 ******************************************************************************/
PAL_INT_32 binary_search(PAL_INT_32 *search_array,PAL_INT_32 array_size,PAL_INT_32 search_alement)
{
	PAL_INT_32 low_index,high_index,mid_index;

	low_index = 0;
	high_index = array_size;
	mid_index = (low_index + (high_index - 1))/2;

	while((high_index>=low_index))
	{
		if(search_array[mid_index] == search_alement)
		{
			return mid_index;
		}
		else if(search_array[mid_index] > search_alement)
		{
			high_index = mid_index - 1;
		}
		else if(search_array[mid_index] < search_alement)
		{
			low_index = mid_index + 1;
		}
		mid_index = (low_index + high_index)/2;
	}
	return -1;
}

/******************************************************************************
 * Description :This function will short the crosspoint mapping in ascending
 *				order with respect to the input port.
 * Inputs      :Crosspoint mapping Valid, start and end mapping number
 * Outputs     :Success or Failure.
 ******************************************************************************/
static PAL_UINT_32 qsort_stm1_crosspoint_map(stm1_cp_map_S *stm1_cp_map, PAL_UINT_32 start, PAL_UINT_32 end)
{
	PAL_UINT_32 pivot,start_loc,end_loc;
	stm1_cp_map_S temp_stm1_cp_map;
	PAL_UINT_32 port_cnt = 0;

    if (end < 2)
        return;

    if(start<end)
    {
        pivot=start;
        start_loc=start;
        end_loc=end;

        while(start_loc<end_loc)
        {
			for(port_cnt = 0; port_cnt<stm1_cp_map[start].max_sub_elements; port_cnt++)
			{
				if(stm1_cp_map[start_loc].map_status[port_cnt] == COMBINATION_MAPPING)
				{
					break;
				}
			}
			while(stm1_cp_map[start_loc].map_array[port_cnt]<=stm1_cp_map[pivot].map_array[port_cnt]&&start_loc<end)
				 start_loc++;
            while(stm1_cp_map[end_loc].map_array[port_cnt]>stm1_cp_map[pivot].map_array[port_cnt])
                 end_loc--;
			if(start_loc<end_loc)
			{
				memcpy(&temp_stm1_cp_map, &stm1_cp_map[start_loc], sizeof(stm1_cp_map_S));
				memcpy(&stm1_cp_map[start_loc], &stm1_cp_map[end_loc], sizeof(stm1_cp_map_S));
			  	memcpy(&stm1_cp_map[end_loc], &temp_stm1_cp_map, sizeof(stm1_cp_map_S));
			}
		}
		memcpy(&temp_stm1_cp_map, &stm1_cp_map[pivot], sizeof(stm1_cp_map_S));
		memcpy(&stm1_cp_map[pivot], &stm1_cp_map[end_loc], sizeof(stm1_cp_map_S));
		memcpy(&stm1_cp_map[end_loc], &temp_stm1_cp_map, sizeof(stm1_cp_map_S));


		qsort_stm1_crosspoint_map(stm1_cp_map,start,end_loc-1);
		qsort_stm1_crosspoint_map(stm1_cp_map,end_loc+1,end);
	}
}

/******************************************************************************
 * Description :This function will short the crosspoint mapping in ascending
 *				order with respect to the working channel.
 * Inputs      :SNCP mapping Valid, start and end mapping number
 * Outputs     :Success or Failure.
 ******************************************************************************/
static PAL_UINT_32 qsort_sncp_map(Sdh_Sncp_Config_S *sncp_parms, PAL_INT_32 start, PAL_INT_32 end)
{
	PAL_INT_32 pivot,start_loc,end_loc;
	Sdh_Sncp_Parms_S sncp_prot_data;
	PAL_INT_32 port_cnt = 0;

    if (end < 2)
        return;

    if(start<end)
    {
        pivot=start;
        start_loc=start;
        end_loc=end;

        while(start_loc<end_loc)
        {
			while(sncp_parms->sncp_prot_data[start_loc].sncp_couplet.working_channel<=sncp_parms->sncp_prot_data[pivot].sncp_couplet.working_channel&&start_loc<end)
				 start_loc++;
            while(sncp_parms->sncp_prot_data[end_loc].sncp_couplet.working_channel>sncp_parms->sncp_prot_data[pivot].sncp_couplet.working_channel)
                 end_loc--;
			if(start_loc<end_loc)
			{
				memcpy(&sncp_prot_data, &sncp_parms->sncp_prot_data[start_loc], sizeof(Sdh_Sncp_Parms_S));
				memcpy(&sncp_parms->sncp_prot_data[start_loc], &sncp_parms->sncp_prot_data[end_loc], sizeof(Sdh_Sncp_Parms_S));
			  	memcpy(&sncp_parms->sncp_prot_data[end_loc], &sncp_prot_data, sizeof(Sdh_Sncp_Parms_S));
			}
		}
		memcpy(&sncp_prot_data, &sncp_parms->sncp_prot_data[pivot], sizeof(Sdh_Sncp_Parms_S));
		memcpy(&sncp_parms->sncp_prot_data[pivot],&sncp_parms->sncp_prot_data[end_loc], sizeof(Sdh_Sncp_Parms_S));
		memcpy(&sncp_parms->sncp_prot_data[end_loc], &sncp_prot_data, sizeof(Sdh_Sncp_Parms_S));


		qsort_sncp_map(sncp_parms,start,end_loc-1);
		qsort_sncp_map(sncp_parms,end_loc+1,end);
	}
}

/******************************************************************************
 * 			CIL Buffer Allocation and initialization
 *****************************************************************************/
PAL_UINT_32 cil_sdh_stm1_init_buffer()
{
	memset(&sdh_config_db, 0, sizeof(sdh_config_db));
	memset(&stm1_cp_limit,0,sizeof(stm1_cp_limit_S));
    
    return SUCCESS;
}


/******************************************************************************
 * Description   : This function is used to initialize the CIL Params Database
 *                                      configuration
 * Inputs        : None
 * Output        : None
 * Return Value : STM1_SUCCESS on success/STM1_FAILURE on failure
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_database_init()
{
        int ret_val = STM1_SUCCESS;
    
	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PORT_CONFIG, &sdh_config_db.port_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Port Config database \n", __func__);
        }

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_MUX_DEMUX_CONFIG,\
                                                                        &sdh_config_db.core_config.md_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Mux-Demux Config database \n", __func__);
        }

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_TRACE_MSG_CONFIG,\
                                                                        &sdh_config_db.core_config.tm_config);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Trace Msg Config database \n", __func__);
        }

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PROTECTION_SW_CONFIG,\
                                                                        &sdh_config_db.chip_config.prot_parms);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Protection Switch Config database \n", __func__);
        }

        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_SYNC_CONFIG,\
                                                                        &sdh_config_db.chip_config.stm1_sync);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Time Sync Config database \n", __func__);
        }


        ret_val = dbl_sdh_stm1_init_config_buffer(STM1_CROSSPOINT_CONFIG,\
                                                                        &sdh_config_db.chip_config.stm1_cp);
        if(STM1_SUCCESS != ret_val)
        {
                printf(" %s: Failed to initialize the CIL Crosspoint Config database \n", __func__);
        }


        return ret_val;
}


/******************************************************************************
 * Description		: This function will:
 *					  1.Allocate the memory dynamically for the number of ports
 *						supported by the LIU
 *					  2.Call the relevant functions to process it
 * Inputs     		:
 * Output    		: To assign the memory for the number of ports supported
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

PAL_INT_32 cil_sdh_stm1_alloc_mem(Stm1_Sdh_Hw_Max_Interface_S block_info, Sdh_Stm1_Config_Type_E config)
{
	PAL_INT_32 retval = FAILURE;

	printf(" Entered %s \n", __func__);

	switch(config)
	{
		case PORT_CONFIG:
		{
			printf(" Entered PORT_CONFIG case \n");
			
			/* Allocate memory for Port Configuration */
			dbl_sdh_stm1_cfg_update(&sdh_config_db.port_config, block_info, 1, STM1_PORT_CONFIG);

#if 0
			/* Allocate memory for port configuration */
			if((retval = sdh_stm1_port_cfg_alloc_mem(block_info, &sdh_config_db.port_config))<0)
			{
				return retval;
			}
#endif
		}
		break;
		case CORE_CONFIG:
		{
			printf(" Entered CORE_CONFIG case \n");

			/* Allocate memory for Mux-Demux Configurations */
			dbl_sdh_stm1_cfg_update(&sdh_config_db.core_config.md_config, block_info, 1, STM1_MUX_DEMUX_CONFIG);

			/* Allocate memory for Trace messages Configuration */
                        dbl_sdh_stm1_cfg_update(&sdh_config_db.core_config.tm_config, block_info, 1, STM1_TRACE_MSG_CONFIG);

#if 0
			/* Allocate memory for Mux-Demux Configurations */
			if((retval = sdh_stm1_mux_demux_alloc_mem(block_info,&sdh_config_db.core_config.md_config))<0)
			{
				return retval;
			}

			/* Allocate memory for trace messages */
			if((retval = sdh_stm1_trace_msg_alloc_mem(block_info,&sdh_config_db.core_config.tm_config))<0)
			{
				return retval;
			}
#endif
		}
		break;

		default:
			printf("Invalid configuration \n");	
		break;
	}

	printf(" Leaving %s \n",__func__);

	return SUCCESS;
} /* end of cil_sdh_stm1_alloc_mem() */

PAL_UINT_32 cil_sdh_stm1_cleanup()
{
	/* Clean the memory for STM1 Local Configuration database */
	dbl_sdh_stm1_release_config_buffer(STM1_PORT_CONFIG, &sdh_config_db.port_config);
	dbl_sdh_stm1_release_config_buffer(STM1_MUX_DEMUX_CONFIG, &sdh_config_db.core_config.md_config);
	dbl_sdh_stm1_release_config_buffer(STM1_TRACE_MSG_CONFIG, &sdh_config_db.core_config.tm_config);

#if 0	
	sdh_stm1_port_cfg_free_mem(&sdh_config_db.port_config);
	sdh_stm1_mux_demux_free_mem(&sdh_config_db.core_config.md_config);
	sdh_stm1_trace_msg_free_mem(&sdh_config_db.core_config.tm_config);
#endif

}

// will be updated when crossopint will be tested.
#if 0
/*******************************************************************************
 *	Description:This function is used to Get the Stm1 Protection Mappings.
 *
 *	Inputs:		none
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/
PAL_INT_32 cil_sdh_stm1_update_prot_sw_chmap(PAL_VOID)
{
	PAL_INT_32 retval = SUCCESS;
	PAL_USHORT_16	map_cnt=0, i;


	/* Get the channel map count : COMMENTED FOR TESTING PURPOSE */
//	map_cnt = stm1_parse_generic_rule(ptr, stm1_cp_map, &chmap, &system_info);
	/* Loop through all mappings and extract the Protection Mapping */
	for(i=0; i<map_cnt; i++)
	{
		//printf("stm1_cp_map[%d].max_sub_elements : %d \n", i, stm1_cp_map[i].max_sub_elements);
		for(sub_map_cnt = 0; sub_map_cnt<stm1_cp_map[i].max_sub_elements; sub_map_cnt++)
		{
			printf("stm1_cp_map[%d].map_element[%d] = %s \n", i, sub_map_cnt, stm1_cp_map[i].map_element[sub_map_cnt]);
			printf("stm1_cp_map[%d].map_status[%d]  = %d \n", i, sub_map_cnt, stm1_cp_map[i].map_status[sub_map_cnt]);
			printf("stm1_cp_map[%d].map_array[%d]  	= %d \n", i, sub_map_cnt,stm1_cp_map[i].map_array[sub_map_cnt]);


			/* 0-Input Mapping,  1- Combination Mapping(Output),  2- Protection Mapping */
			if( 2 == stm1_cp_map[i].map_status[sub_map_cnt] ) && ( STM1_CP_VALID == stm1_cp_map[i].status )
			{
				 chip_config.prot_parms.sncp_parms.sncp_prot_data[i].sncp_couplet.prot_channel = \
				 													stm1_cp_map[i].map_array[sub_map_cnt];

				/* If 'sub_map_cnt' is protection mapping then 'sub_map_cnt-1' will be working path  */
				 chip_config.prot_parms.sncp_parms.sncp_prot_data[i].sncp_couplet.working_channel = \
				 													stm1_cp_map[i].map_array[sub_map_cnt-1];
			}




		}
		printf("stm1_cp_map[%d].status  = %d \n", i, stm1_cp_map[i].status);
	}

 return retval;
}
#endif

/******************************************************************************
 * 			CIL SDH configuration update related functions
 *****************************************************************************/
PAL_UINT_32 cil_sdh_stm1_copy_complete_dB(PAL_VOID)
{
	PAL_UINT_32 feature_index = STM1_PORT_CONFIG;
	for(feature_index = STM1_PORT_CONFIG;\
			feature_index < MAX_SDH_STM1_FEATURE_LIST;feature_index++)
	{
		/* Switch case to copy sdh features configurations between database */
		switch(feature_index)
		{
			case STM1_PORT_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.port_config);
			}
			break;

			case STM1_MUX_DEMUX_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.core_config.md_config);
			}
			break;

			case STM1_TRACE_MSG_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.core_config.tm_config);
			}
			break;

			case STM1_PROTECTION_SW_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.chip_config.prot_parms);
			}
			break;

			case STM1_PROTECTION_SW_LCR_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.chip_config.prot_parms.lcr_parms);
			}
			break;
			case STM1_SYNC_CONFIG:
			{
				/* Update the external dB value to local dB */
				cel_sdh_stm1_read_external_cfg(feature_index,\
								&sdh_config_db.chip_config.stm1_sync);
			}
			break;
			case STM1_CROSSPOINT_CONFIG:
			{
				cel_sdh_stm1_read_external_cfg(feature_index,\
												&sdh_config_db.chip_config.stm1_cp);
			}
			break;

			default:
			{
				printf(" Entering the default case \n");
			}
			break;
		} /* end of the switch(feature_index) */
	}
	return SUCCESS;
}

PAL_INT_32 cil_sdh_stm1_port_configure(PAL_UCHAR card_index, PAL_UCHAR startPort,\
					PAL_UCHAR numPorts, PAL_UCHAR no_check)
{
	PAL_UINT_32 ret_val = SUCCESS;
	PAL_UCHAR portIndex = startPort, maxPorts = startPort+numPorts;

	for(portIndex = startPort;portIndex < maxPorts;portIndex++)
	{
		if((no_check) || (ACTIVE == sdh_config_db.port_config.port_params.elems[portIndex].status))
		{
			/* Check the STM1 port is configured by user for data transmission */
			ret_val = hil_sdh_stm1_port_config_set(card_index,portIndex,\
				sdh_config_db.port_config.port_params.elems[portIndex],no_check);
		}
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_global_configure(PAL_UCHAR card_index, PAL_UCHAR startPort,\
					PAL_UCHAR numPorts, PAL_UCHAR no_check)
{
	PAL_UINT_32 ret_val = SUCCESS;
	PAL_UCHAR portIndex = startPort, maxPorts = startPort+numPorts;

	for(portIndex = startPort;portIndex < maxPorts;portIndex++)
	{
		if((no_check) || (ACTIVE == sdh_config_db.port_config.port_params.elems[portIndex].status))
		{
			/* Check the STM1 port is configured by user for data transmission */
			ret_val = hil_sdh_stm1_global_config_set(card_index,portIndex);
		}
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_mux_demux_configure(PAL_UCHAR card_index, PAL_UCHAR startCore, \
					PAL_UCHAR numCores, PAL_UCHAR no_check)
{
	PAL_UINT_32 ret_val = SUCCESS;
	PAL_UCHAR coreIndex = startCore, maxCores = startCore+numCores;
	PAL_UCHAR bit_set[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

	for(coreIndex = startCore;coreIndex < maxCores;coreIndex++)
	{
		if((no_check) || (ACTIVE == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[coreIndex].status))
		{
			/* Check the STM1 Mux-Demux is configured by user for data transmission */
			ret_val = hil_sdh_stm1_mux_demux_config_update(card_index,coreIndex,\
				sdh_config_db.core_config.md_config.ptr_core_cfg.elems[coreIndex],no_check);

			if(SUCCESS == ret_val)
			{
				sdh_stm1_mux_cfg_chg |= bit_set[coreIndex];
			}
		}
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_trace_msg_configure(PAL_UCHAR card_index, PAL_UCHAR startCore, \
					PAL_UCHAR numCores, PAL_UCHAR no_check)
{
	PAL_UINT_32 ret_val = SUCCESS;
	PAL_UCHAR coreIndex = startCore, maxCores = startCore+numCores;

	for(coreIndex = startCore;coreIndex < maxCores;coreIndex++)
	{
		if((no_check) || (ACTIVE == sdh_config_db.core_config.tm_config.ptr_trace_cfg.elems[coreIndex].status))
		{
			/* Check the STM1 Mux-Demux is configured by user for data transmission */
			ret_val = hil_sdh_stm1_trace_msg_cfg_update(card_index,coreIndex,\
				sdh_config_db.core_config.tm_config.ptr_trace_cfg.elems[coreIndex],no_check);
		}
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_prot_sw_cfg_update(Stm1_Prot_Parms_S prot_data, PAL_SHORT_16 no_cfg_check)
{
	PAL_INT_32 retval = 0;

	if(SDH_STM1_SNCP == prot_data.prot_algo )
	{
		retval = hil_sdh_stm1_prot_sncp_config_update(prot_data, prot_data.max_sncp_config,STM1_PROT_SET);
		if(FAILURE == retval)
		{
			printf(" Function hil_sdh_stm1_prot_sncp_config_set() retuned FAILURE ");
		}
	}
	else if(SDH_STM1_MSP == prot_data.prot_algo )
	{
		retval = hil_sdh_stm1_prot_msp_config_update(prot_data, prot_data.max_sncp_config);
		if(FAILURE == retval)
		{
			printf(" Function hil_sdh_stm1_prot_sncp_config_set() retuned FAILURE ");
		}
	}
	else if(SDH_STM1_DISABLE == prot_data.prot_algo )
	{
		/* Disable Protection switching */
		retval = hil_sdh_stm1_prot_disable(prot_data.prot_algo);
		if(FAILURE == retval)
		{
			printf(" Function hil_sdh_stm1_prot_disable() retuned FAILURE ");
		}
	}
	return retval;
}

PAL_INT_32 cil_sdh_stm1_prot_lcr_cfg_update(Stm1_Prot_Parms_S prot_lcr_data,PAL_SHORT_16 no_cfg_check)
{
	PAL_INT_32 retval = 0;

	switch(prot_lcr_data.prot_algo)
	{
		case SDH_STM1_MSP:
		{
			retval = hil_sdh_prot_msp_cmd_req_set(prot_lcr_data.lcr_parms.msp_lcr_cmd);
			if(FAILURE == retval)
			{
				printf(" Function hil_sdh_prot_msp_cmd_req_set() retuned FAILURE \n");
			}
		}
		break;
		case SDH_STM1_SNCP:
		{
			retval == hil_sdh_prot_sncp_cmd_req_set(prot_lcr_data.lcr_parms.sncp_lcr_cmd, prot_lcr_data.max_sncp_config);
			if(FAILURE == retval)
			{
				printf(" Function hil_sdh_prot_sncp_cmd_req_set() retuned FAILURE \n");
			}
		}
		break;
		default:
		break;
	}

	return retval;
}

PAL_INT_32 cil_sdh_stm1_time_sync_update(Stm1_Sync_Parms_S time_sync_data,\
						PAL_UCHAR adm_core_index,PAL_SHORT_16 no_cfg_check)
{
	PAL_INT_32 retval = 0;

	retval = hil_sdh_stm1_sync_config_set(&time_sync_data);
	if(FAILURE == retval)
	{
		printf(" Function hil_sdh_stm1_sync_config_set() retuned FAILURE ");
	}

	return retval;
}

PAL_INT_32 cil_sdh_stm1_cfg_update(Sdh_Stm1_Features_E feature_info)
{
	PAL_UINT_32 count = 0, retval = FAILURE;
	PAL_UCHAR card_index = 0,stm1_port_core_offset = 0, max_stm1_port_core = 0;

	printf(" Entered %s \n", __FUNCTION__);
	switch(feature_info)
	{
		case STM1_MUX_DEMUX_CONFIG:
		{
			stm1_port_core_offset = 0;
			for(card_index = 0;card_index < sdh_config_db.core_config.md_config.num_cards;card_index++)
			{
				max_stm1_port_core = sdh_config_db.core_config.md_config.sdh_card_info.elems[card_index].max_stm_port_core;
				/* To configure for STM1 Mux-Demux configuration for all cores */
				if((retval = cil_sdh_stm1_mux_demux_configure(card_index,stm1_port_core_offset,max_stm1_port_core,1))<0)
				{
					return FAILURE;
				}
				stm1_port_core_offset += max_stm1_port_core;
			}

			break;
		}
		case STM1_TRACE_MSG_CONFIG:
		{
			stm1_port_core_offset = 0;
			for(card_index = 0;card_index < sdh_config_db.core_config.tm_config.num_cards;card_index++)
			{
				max_stm1_port_core = sdh_config_db.core_config.tm_config.sdh_card_info.elems[card_index].max_stm_port_core;
				/* To configure for STM1 Trace Message configuration for all cores */
				if((retval = cil_sdh_stm1_trace_msg_configure(card_index,stm1_port_core_offset,max_stm1_port_core,1))<0)
				{
					return FAILURE;
				}
				stm1_port_core_offset += max_stm1_port_core;
			}


			break;
		}
		case STM1_PORT_CONFIG:
		{
			stm1_port_core_offset = 0;
			for(card_index = 0;card_index < sdh_config_db.port_config.num_cards;card_index++)
			{
				/* STM1 LIU Global configuration updates */
				max_stm1_port_core = sdh_config_db.port_config.sdh_card_info.elems[card_index].max_stm_port_core;
				if((retval = cil_sdh_stm1_global_configure(card_index,stm1_port_core_offset,max_stm1_port_core,1))<0)
				{
					return FAILURE;
				}

				/* To configure for port configuration for all ports */
				if((retval = cil_sdh_stm1_port_configure(card_index,stm1_port_core_offset,max_stm1_port_core,1))<0)
				{
					return FAILURE;
				}
				stm1_port_core_offset += max_stm1_port_core;
			}
			break;
		}

		case STM1_PROTECTION_SW_CONFIG:
		{
			if((retval = cil_sdh_stm1_prot_sw_cfg_update(sdh_config_db.chip_config.prot_parms, 1))<0)
			{
				return FAILURE;
			}
			break;
		}

		case STM1_PROTECTION_SW_LCR_CONFIG:
		{
			if((retval = cil_sdh_stm1_prot_lcr_cfg_update(sdh_config_db.chip_config.prot_parms, 1))<0)
			{
				return FAILURE;
			}
			break;
		}

		case STM1_SYNC_CONFIG:
		{
			/* YTD - What is the use of ADM core index */
			if((retval = cil_sdh_stm1_time_sync_update(sdh_config_db.chip_config.stm1_sync,0, 1))<0)
			{
				return FAILURE;
			}
			break;
		}

		case STM1_CROSSPOINT_CONFIG:
		{
			if((retval = cil_sdh_stm1_crosspoint_cfg_update())<0)
			{
				return FAILURE;
			}
			break;
		}

		default:
			break;
	}
	printf(" Leaving %s \n", __FUNCTION__);
}

/******************************************************************************
 * Description	: This function is used to verify the request and initiates
 * 					the configuration and if it is failed to configure, it
 *					will restores the previous configuration
 * Inputs     	: Request information and number of ports to configure
 * Output    	: None
 * Return Value : SUCCESS on success, FAILURE on failure
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_port_cfg_update(STM1_Ui_Req_S user_request,PAL_USHORT_16 stm1_port_offset,
									PAL_USHORT_16 max_stm1_ports)
{
	PAL_INT_32 ret_val = 0;

	if(0 == user_request.element_info)
	{
		ret_val = cil_sdh_stm1_port_configure(user_request.card_index,\
									stm1_port_offset,max_stm1_ports,0);
	}
	else
	{
		ret_val = cil_sdh_stm1_port_configure(user_request.card_index,\
									stm1_port_offset + user_request.element_info - 1, 1, 0);
	}

	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_PORT_CONFIG,\
								&sdh_config_db.port_config);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_PORT_CONFIG,STM1_SET,\
								&sdh_config_db.port_config);
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_mux_demux_cfg_update(STM1_Ui_Req_S user_request,PAL_USHORT_16 stm1_port_offset,
									PAL_USHORT_16 max_stm1_ports)
{
	PAL_INT_32 ret_val = 0;

	if(0 == user_request.element_info)
	{
		ret_val = cil_sdh_stm1_mux_demux_configure(user_request.card_index,\
									stm1_port_offset,max_stm1_ports,0);
	}
	else
	{
		ret_val = cil_sdh_stm1_mux_demux_configure(user_request.card_index,\
									user_request.element_info-1,1,0);
	}

	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_MUX_DEMUX_CONFIG,\
								&sdh_config_db.core_config.md_config);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_MUX_DEMUX_CONFIG,STM1_SET,\
								&sdh_config_db.core_config.md_config);
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_trace_msg_cfg_update(STM1_Ui_Req_S user_request,PAL_USHORT_16 stm1_port_offset,
									PAL_USHORT_16 max_stm1_ports)
{
	PAL_INT_32 ret_val = 0;

	if(0 == user_request.element_info)
	{
		ret_val = cil_sdh_stm1_trace_msg_configure(user_request.card_index,\
									stm1_port_offset,max_stm1_ports,0);
	}
	else
	{
		ret_val = cil_sdh_stm1_trace_msg_configure(user_request.card_index,\
									user_request.element_info-1,1,0);
	}

	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_TRACE_MSG_CONFIG,\
								&sdh_config_db.core_config.tm_config);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_TRACE_MSG_CONFIG,STM1_SET,\
								&sdh_config_db.core_config.tm_config);
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_protection_cfg_update()
{
	PAL_INT_32 ret_val = 0;

	ret_val = cil_sdh_stm1_prot_sw_cfg_update(sdh_config_db.chip_config.prot_parms, 0);
	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_PROTECTION_SW_CONFIG,\
								&sdh_config_db.chip_config.prot_parms);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_PROTECTION_SW_CONFIG,STM1_SET,\
								&sdh_config_db.chip_config.prot_parms);
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_protection_lcr_cfg_update()
{
	PAL_INT_32 ret_val = 0;

	ret_val = cil_sdh_stm1_prot_lcr_cfg_update(sdh_config_db.chip_config.prot_parms, 0);
	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_PROTECTION_SW_LCR_CONFIG,\
								&sdh_config_db.chip_config.prot_parms);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_PROTECTION_SW_LCR_CONFIG,STM1_SET,\
								&sdh_config_db.chip_config.prot_parms);
	}
	return ret_val;
}

PAL_INT_32 cil_sdh_stm1_time_sync_cfg_update()
{
	PAL_INT_32 ret_val = 0;

	ret_val = cil_sdh_stm1_time_sync_update(sdh_config_db.chip_config.stm1_sync, 0,0);
	/* Revert back the configuration, if the configuration update is failed */
	if(SUCCESS != ret_val)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_SYNC_CONFIG,\
								&sdh_config_db.chip_config.stm1_sync);
		ret_val = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_SYNC_CONFIG,STM1_SET,\
								&sdh_config_db.chip_config.stm1_sync);
	}
	return ret_val;
}

/******************************************************************************
 * Description :This function will create a list of mapping that need to be
 *				deleted when user updateds new crosspoint mapping.
 * Inputs      :Crosspoint mapping, pointer to delete mapping, Valid and delete
 *				map count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 delete_stm1_crosspoint_map(stm1_cp_map_S *stm1_cp_map, stm1_cp_map_S *del_stm1_cp_map, \
													PAL_INT_32 valid_map_cnt, PAL_INT_32 *delete_map_cnt)
{
	PAL_INT_32 retval = 0, map_cnt = 0, port_cnt = 0;
	PAL_INT_32 search_array[512];
	PAL_INT_32 key = 0, loc = -1;
	PAL_INT_32 l_delete_map_cnt = 0;

	printf("Entering : %s \n", __FUNCTION__);

	memset(search_array, 0, sizeof(search_array));
	/* Extract Search array for comparision */
	for(map_cnt = 0; map_cnt<valid_map_cnt; map_cnt++)
	{
		for(port_cnt = 0; port_cnt<stm1_cp_map[port_cnt].max_sub_elements; port_cnt++)
		{
			if(stm1_cp_map[map_cnt].map_status[port_cnt] == COMBINATION_MAPPING)
			{
				search_array[map_cnt] = stm1_cp_map[map_cnt].map_array[port_cnt];
			}
			printf("search_array[ %d] : %d  stm1_cp_map[%d].map_array[%d] : %d \n", \
					map_cnt, search_array[map_cnt], map_cnt, port_cnt, stm1_cp_map[map_cnt].map_array[port_cnt]);
		}
	}

	//printf("Compare Prev crosspoint map with new crosspoint map : %s \n", __FUNCTION__);
	/* Compare Prev crosspoint map with new crosspoint map and extract
	   the map that are not avilable in new crosspoint map compared
	   with old crosspoint map*/
	printf("%s stm1_cp_data.prev_num_stm1_map : %d \n", __FUNCTION__, stm1_cp_data.prev_num_stm1_map);
	for(map_cnt = 0; map_cnt<stm1_cp_data.prev_num_stm1_map; map_cnt++)
	{
		//printf("%s stm1_cp_data.prev_stm1_cp_map : %d \n", __FUNCTION__, stm1_cp_data.prev_stm1_cp_map);
		//printf("%s stm1_cp_data.prev_stm1_cp_map[%d].max_sub_elements : %d \n", __FUNCTION__, map_cnt, stm1_cp_data.prev_stm1_cp_map[map_cnt].max_sub_elements);
		for(port_cnt = 0; port_cnt<stm1_cp_data.prev_stm1_cp_map[map_cnt].max_sub_elements; port_cnt++)
		{
			if(stm1_cp_data.prev_stm1_cp_map[map_cnt].map_status[port_cnt] == COMBINATION_MAPPING)
			{
				break;
			}
		}
		printf("stm1_cp_data.prev_stm1_cp_map[%d].map_array[%d] : %d\n", \
				map_cnt, port_cnt, stm1_cp_data.prev_stm1_cp_map[map_cnt].map_array[port_cnt]);
		key = stm1_cp_data.prev_stm1_cp_map[map_cnt].map_array[port_cnt];

		printf("%s Key : %d \n", __FUNCTION__, key);

		loc = binary_search(search_array, valid_map_cnt, key);
		printf("%s loc : %d \n", __FUNCTION__, loc);
		/* If inport present then check for all mapping with respect to the inport */
		if(loc>=0)
		{
			if(memcmp(&stm1_cp_map[loc], &stm1_cp_data.prev_stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S)))
			{
				printf("%s loc are not same: %d \n", __FUNCTION__, loc);
				memcpy(&del_stm1_cp_map[l_delete_map_cnt], &stm1_cp_data.prev_stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
				l_delete_map_cnt++;
			}
		}
		/* If the mapping not present then delete the configuration */
		else
		{
			memcpy(&del_stm1_cp_map[l_delete_map_cnt], &stm1_cp_data.prev_stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
			printf("del_stm1_cp_map[%d].map_string = %s \n", l_delete_map_cnt, del_stm1_cp_map[l_delete_map_cnt].map_string);
			l_delete_map_cnt++;
		}
	}
	*delete_map_cnt = l_delete_map_cnt;

	printf("%s *delete_map_cnt = %d\n", __FUNCTION__, *delete_map_cnt);

	return 0;
}

/******************************************************************************
 * Description :This function will create a list of mapping that need to be
 *				added when user updateds new crosspoint mapping.
 * Inputs      :Crosspoint mapping, pointer to new mapping, Valid, new and
 *				unused map count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 add_stm1_crosspoint_map(stm1_cp_map_S *stm1_cp_map, stm1_cp_map_S *new_stm1_cp_map, stm1_cp_map_S *unused_stm1_cp_map,\
													PAL_INT_32 valid_map_cnt, PAL_INT_32 *new_map_cnt, PAL_INT_32 *unused_map_cnt)
{
	PAL_INT_32 retval = 0, map_cnt = 0, port_cnt = 0;
	PAL_INT_32 search_array[512];
	PAL_INT_32 key = 0, loc = 0;
	PAL_INT_32 l_new_map_cnt = 0, l_unused_map_cnt = 0;

	printf("Entering : %s \n", __FUNCTION__);

	memset(search_array, 0, sizeof(search_array));
	/* Extract Search array for comparision */
	for(map_cnt = 0; map_cnt<stm1_cp_data.prev_num_stm1_map; map_cnt++)
	{
		for(port_cnt = 0; port_cnt<stm1_cp_data.prev_stm1_cp_map[port_cnt].max_sub_elements; port_cnt++)
		{
			if(stm1_cp_data.prev_stm1_cp_map[map_cnt].map_status[port_cnt] == COMBINATION_MAPPING)
			{
				search_array[map_cnt] = stm1_cp_data.prev_stm1_cp_map[map_cnt].map_array[port_cnt];
			}
			printf("search_array[ %d] : %d  stm1_cp_data.prev_stm1_cp_map[%d].map_array[%d] : %d \n", map_cnt, search_array[map_cnt], map_cnt, \
												port_cnt, stm1_cp_data.prev_stm1_cp_map[map_cnt].map_array[port_cnt]);
		}
	}
	/* Compare Prev crosspoint map with new crosspoint map and extract
	   the map that are not avilable in new crosspoint map compared
	   with old crosspoint map*/
	for(map_cnt = 0; map_cnt<valid_map_cnt; map_cnt++)
	{
		/* Find the inport from the sub elements */
		for(port_cnt = 0; port_cnt<stm1_cp_map[map_cnt].max_sub_elements; port_cnt++)
		{
			if(stm1_cp_map[map_cnt].map_status[port_cnt] == COMBINATION_MAPPING)
			{
				break;
			}
		}
		/* Provide the input element as key */
		key = stm1_cp_map[map_cnt].map_array[port_cnt];

		printf("%s Key : %d \n", __FUNCTION__, key);

		/* Do a binary search */
		loc = binary_search(search_array, stm1_cp_data.prev_num_stm1_map, key);
		printf("%s loc : %d \n", __FUNCTION__, loc);

		/* If inport present then check for all mapping with respect to the inport */
		if(loc>=0)
		{
			if(memcmp(&stm1_cp_data.prev_stm1_cp_map[loc], &stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S)))
			{
				memcpy(&new_stm1_cp_map[l_new_map_cnt], &stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
				l_new_map_cnt++;
			}
			else
			{

				/* Mapping are same so we won't configure thisw in hardware but we will copy this to
				   previous mapping */
				memcpy(&unused_stm1_cp_map[l_unused_map_cnt], &stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
				l_unused_map_cnt++;
			}

		}
		/* If the mapping not present then do the configuration */
		else
		{
			memcpy(&new_stm1_cp_map[l_new_map_cnt], &stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
			l_new_map_cnt++;
		}
	}
	*new_map_cnt 	= l_new_map_cnt;
	*unused_map_cnt = l_unused_map_cnt;

	return 0;
}

static PAL_INT_32 update_stm1_cp_map_filter_db(stm1_cp_map_S *stm1_cp_map, PAL_INT_32 valid_map_cnt,\
										PAL_INT_32 delete_map_cnt, PAL_INT_32 new_map_cnt, PAL_INT_32 unused_map_cnt)
{
	PAL_INT_32 retval = 0;
	PAL_INT_32 map_cnt = (delete_map_cnt + new_map_cnt + unused_map_cnt);

	/* Free the STM1 Filter Database if present */
	if(stm1_cp_map_filter.cp_data != NULL)
	{
		free(stm1_cp_map_filter.cp_data);
	}

	/* Allocate memory for STM1 Channelmap Filter database */
	if((stm1_cp_map_filter.cp_data = malloc(sizeof(stm1_cp_map_S)*(map_cnt))) == NULL)
	{
		return retval;
	}

	/* Update the STM1 Channelmap Filter database */
	stm1_cp_map_filter.valid_map_cnt 	= valid_map_cnt;
	stm1_cp_map_filter.delete_map_cnt 	= delete_map_cnt;
	stm1_cp_map_filter.new_map_cnt	 	= new_map_cnt;
	stm1_cp_map_filter.unused_map_cnt 	= unused_map_cnt;

	memcpy(stm1_cp_map_filter.cp_data, stm1_cp_map, sizeof(stm1_cp_map_S)*(map_cnt));

	return retval;
}
/******************************************************************************
 * Description :This function will filter mapping based on configuration.
 * Inputs      :Crosspoint mapping, delete, new and	unused map count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 stm1_crosspoint_map_filter(stm1_cp_map_S *stm1_cp_map, PAL_INT_32 *valid_map_cnt, \
										PAL_INT_32 *delete_map_cnt, PAL_INT_32 *new_map_cnt, PAL_INT_32 *unused_map_cnt)
{
	stm1_cp_map_S *del_stm1_cp_map = NULL, *new_stm1_cp_map = NULL, *unused_stm1_cp_map = NULL;
	PAL_INT_32 retval = 0;
	PAL_INT_32 map_cnt = 0;


	//printf(" Entering : %s \n", __FUNCTION__);
	if((del_stm1_cp_map = malloc(sizeof(stm1_cp_map_S)*(*valid_map_cnt+stm1_cp_data.prev_num_stm1_map))) == NULL)
	{
		return retval;
	}

	if((new_stm1_cp_map = malloc(sizeof(stm1_cp_map_S)*(*valid_map_cnt+stm1_cp_data.prev_num_stm1_map))) == NULL)
	{
		free(del_stm1_cp_map);
		return retval;
	}

	if((unused_stm1_cp_map = malloc(sizeof(stm1_cp_map_S)*(*valid_map_cnt+stm1_cp_data.prev_num_stm1_map))) == NULL)
	{
		free(new_stm1_cp_map);
		free(del_stm1_cp_map);
		return retval;
	}

	printf(" stm1_cp_data.prev_num_stm1_map : %d *valid_map_cnt: %d\n", stm1_cp_data.prev_num_stm1_map, *valid_map_cnt);
	/* Compare OLD and New Map */
	if(stm1_cp_data.prev_num_stm1_map != *valid_map_cnt)
	{
		printf("Previous Valid Map count and current Valid Map count are different \n");
		/* Collect the Crosspoint mapping that need to be deleted */
		if((retval = delete_stm1_crosspoint_map(stm1_cp_map, del_stm1_cp_map, *valid_map_cnt, delete_map_cnt))<0)
		{
			free(del_stm1_cp_map);
			free(new_stm1_cp_map);
			free(unused_stm1_cp_map);
			return retval;
		}
		/* Collect the Crosspoint mapping that need to be configured */
		if((retval = add_stm1_crosspoint_map(stm1_cp_map, new_stm1_cp_map, unused_stm1_cp_map, *valid_map_cnt, new_map_cnt, unused_map_cnt))<0)
		{
			free(del_stm1_cp_map);
			free(new_stm1_cp_map);
			free(unused_stm1_cp_map);
			return retval;
		}
	}
	else
	{
		printf("Previous Valid Map count and current Valid Map count are same : %d\n", stm1_cp_data.prev_num_stm1_map);
		if(memcmp(stm1_cp_data.prev_stm1_cp_map, stm1_cp_map, sizeof(stm1_cp_map_S)*stm1_cp_data.prev_num_stm1_map))
		{
			printf("But Crosspoint maps are different \n");
			/* Collect the Crosspoint mapping that need to be deleted */
			if((retval = delete_stm1_crosspoint_map(stm1_cp_map, del_stm1_cp_map, *valid_map_cnt, delete_map_cnt))<0)
			{
				free(del_stm1_cp_map);
				free(new_stm1_cp_map);
				free(unused_stm1_cp_map);
				return retval;
			}
			for(map_cnt=0; map_cnt<*delete_map_cnt; map_cnt++)
			{
				printf("3.del_stm1_cp_map[%d].map_string = %s \n", map_cnt, del_stm1_cp_map[map_cnt].map_string);
			}
			/* Collect the Crosspoint mapping that need to be configured */
			if((retval = add_stm1_crosspoint_map(stm1_cp_map, new_stm1_cp_map, unused_stm1_cp_map, *valid_map_cnt, new_map_cnt, unused_map_cnt))<0)
			{
				free(del_stm1_cp_map);
				free(new_stm1_cp_map);
				free(unused_stm1_cp_map);
				return retval;
			}
		}
	}

	memset(stm1_cp_map, 0, sizeof(stm1_cp_map_S)*(*valid_map_cnt+stm1_cp_data.prev_num_stm1_map));
	memcpy(stm1_cp_map, del_stm1_cp_map, sizeof(stm1_cp_map_S) * (*delete_map_cnt));
	memcpy(&stm1_cp_map[*delete_map_cnt], new_stm1_cp_map, sizeof(stm1_cp_map_S)*(*new_map_cnt));
	memcpy(&stm1_cp_map[*delete_map_cnt+*new_map_cnt], unused_stm1_cp_map, sizeof(stm1_cp_map_S)*(*unused_map_cnt));

	*valid_map_cnt = *delete_map_cnt+*new_map_cnt;
	printf("Mapping to configure in Hardware \n");

	for(map_cnt=0; map_cnt<*valid_map_cnt; map_cnt++)
	{
		printf("stm1_cp_map[%d].map_string = %s \n", map_cnt, stm1_cp_map[map_cnt].map_string);
	}
	printf(" *delete_map_cnt : %d *new_map_cnt: %d\n", *delete_map_cnt, *new_map_cnt);
	printf(" *valid_map_cnt : %d stm1_cp_data.prev_num_stm1_map: %d\n", *valid_map_cnt, stm1_cp_data.prev_num_stm1_map);

	free(del_stm1_cp_map);
	free(new_stm1_cp_map);
	free(unused_stm1_cp_map);
	printf(" Leaving : %s \n", __FUNCTION__);

	return retval;
}

/******************************************************************************
 * Description :This function will create the list of valid mapp from user
 *				configured map list, which contains valid, unused and incomplete
 *				mapping.
 * Inputs      :Crosspoint mapping, total map count.
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 get_stm1_cp_valid_map_list(stm1_cp_map_S *stm1_cp_map, PAL_INT_32 total_map_cnt)
{
	PAL_INT_32 map_cnt = 0, valid_map_cnt = 0;

	for(map_cnt = 0; map_cnt<total_map_cnt; map_cnt++)
	{
		printf("stm1_cp_map[%d].map_string : %s \n", map_cnt, stm1_cp_map[map_cnt].map_string);
		if(stm1_cp_map[map_cnt].status != 0)
		{
			memset(&stm1_cp_map[map_cnt], -1, sizeof(stm1_cp_map_S));
		}
		else
		{
			memcpy(&stm1_cp_map[valid_map_cnt], &stm1_cp_map[map_cnt], sizeof(stm1_cp_map_S));
			valid_map_cnt++;
		}
	}
}

PAL_INT_32 update_stm1_cp_cfg_backup_db(PAL_INT_32 chmap_cnt, stm1_cp_map_S *chmap_data)
{
	stm1_cp_data.prev_num_stm1_map = chmap_cnt;
	if(stm1_cp_data.prev_stm1_cp_map != NULL)
		free(stm1_cp_data.prev_stm1_cp_map);

	stm1_cp_data.prev_stm1_cp_map = malloc(sizeof(stm1_cp_map_S)*chmap_cnt);
	memset(stm1_cp_data.prev_stm1_cp_map, 0, sizeof(stm1_cp_map_S)*chmap_cnt);
	memcpy(stm1_cp_data.prev_stm1_cp_map, chmap_data, sizeof(stm1_cp_map_S)*chmap_cnt);

	return SUCCESS;
}

PAL_INT_32 stm1_crosspoint_map_config(Ui_If_Config_Data_S *ui_config,PAL_INT_32 valid_mapping)
{
	PAL_CHAR_8 				*stm1_crosspoint;
	stm1_cp_map_S			*stm1_cp_map;
	PAL_INT_32 				map_cnt = 0, valid_map_cnt = 0, delete_map_cnt = 0, new_map_cnt = 0, unused_map_cnt = 0, total_map_cnt = 0;
	PAL_INT_32					retval = -1;

	/***** INTIALIZE THE MEMORY TO COPY UI DATA FOR STM1 CROSS-POINT *****/
	map_cnt 			= ui_config->extra_info;
	stm1_crosspoint 	= malloc(ui_config->config_size);
	memset(stm1_crosspoint, -1, ui_config->config_size);
	memset(&stm1_param_map_list, 0, sizeof(stm1_param_map_list));
	stm1_cp_map = malloc(sizeof(stm1_cp_map_S)*(ui_config->extra_info + stm1_cp_data.prev_num_stm1_map));
	memset(stm1_cp_map, 0, sizeof(stm1_cp_map_S)*(ui_config->extra_info + stm1_cp_data.prev_num_stm1_map));
	/* Copy the Crosspoint data to local buffer */
	memcpy(stm1_crosspoint,ui_config->config_data,ui_config->config_size);

	/***** PROCESS THE STM1 CROSSPOINT MAPPING *****/
	if(0 == valid_mapping)
	{
		/* Get the Parse List */
		retval = cil_sdh_stm1_parse_crosspoint(stm1_crosspoint, stm1_cp_map, &stm1_param_map_list, ui_config->extra_info);
		if(SUCCESS != retval)
		{
			free(stm1_crosspoint);
			free(stm1_cp_map);
			return FAILURE;
		}
	}
	else
	{
		memcpy(stm1_cp_map,ui_config->config_data,ui_config->config_size);
	}

	/* Update Valid Map count after processing */
	valid_map_cnt = stm1_param_map_list.valid_map_cnt;
	total_map_cnt = stm1_param_map_list.valid_map_cnt + \
						stm1_param_map_list.invalid_map_cnt + \
							stm1_param_map_list.unused_map_cnt;

	/* Get only the Valid mapping from total mapping */
	get_stm1_cp_valid_map_list(stm1_cp_map, total_map_cnt);

	/* Initialize unused and invalid mappings */
	memset(&stm1_cp_map[valid_map_cnt], 0, sizeof(stm1_cp_map_S)*(ui_config->extra_info + (stm1_cp_data.prev_num_stm1_map - valid_map_cnt)));

	/***** CONFIGURE THE VALID MAPPINGS *****/
	if(valid_map_cnt>0)
	{
		/* Arrange Map in sorted order with respect to inport */
		qsort_stm1_crosspoint_map(stm1_cp_map, 0, valid_map_cnt-1);

		/* Check Config Type and filter the configuration */
		if(SDH_STARTUP_WARM == ui_config->config_type)
		{
			/* Filter the Mapping based on Previous mappings to configure the
			   hardware only with required mappings or new mappings */
			retval = stm1_crosspoint_map_filter(stm1_cp_map, &valid_map_cnt, &delete_map_cnt, &new_map_cnt, &unused_map_cnt);
			if(SUCCESS != retval)
			{
				free(stm1_crosspoint);
				free(stm1_cp_map);
				return FAILURE;
			}
		}
		else
		{
			new_map_cnt = valid_map_cnt;
		}

		update_stm1_cp_map_filter_db(stm1_cp_map, valid_map_cnt, delete_map_cnt, new_map_cnt, unused_map_cnt);
		/* Send the Cross Point Config to HIL if we have valid mappings after filteration */
		if(valid_map_cnt>0)
		{
			retval = hil_sdh_stm1_crosspoint_config_set(stm1_cp_map, valid_map_cnt, delete_map_cnt, unused_map_cnt, ui_config->config_type);
			if(SUCCESS == retval)
			{
				/* Copy back the configured stm1 cross-point */
				free(ui_config->config_data);
				ui_config->config_size = strlen(stm1_param_map_list.map_list);
				ui_config->config_data = malloc(sizeof(PAL_CHAR_8) * ui_config->config_size);

				memcpy(ui_config->config_data, stm1_param_map_list.map_list, ui_config->config_size);

				/* Take cross-point backup */
				printf("#### new_map_cnt+unused_map_cnt : %d #####\n", (new_map_cnt+unused_map_cnt));
				if((new_map_cnt+unused_map_cnt)>0)
				{
					update_stm1_cp_cfg_backup_db(new_map_cnt+unused_map_cnt, stm1_cp_map);
				}
			}
			else
			{
				printf("#### Channel Update Failure  #####\n");
				free(stm1_crosspoint);
				free(stm1_cp_map);
				return FAILURE;
			}
		}
	}
	free(stm1_crosspoint);
	free(stm1_cp_map);
	return SUCCESS;
}

PAL_INT_32 cil_sdh_stm1_crosspoint_cfg_update(PAL_VOID)
{
	PAL_INT_32 retval = 0;
	Ui_If_Config_Data_S temp_cfg;

	memset(&temp_cfg,0,sizeof(temp_cfg));

	temp_cfg.extra_info 	= sdh_config_db.chip_config.stm1_cp.num_of_stm1_mapping;
	temp_cfg.config_size 	= sdh_config_db.chip_config.stm1_cp.chmap_data_size;
	temp_cfg.config_data 	= malloc(temp_cfg.config_size);
	memcpy(temp_cfg.config_data,\
		sdh_config_db.chip_config.stm1_cp.chmap_data,temp_cfg.config_size);
	temp_cfg.config_type 	= SDH_STARTUP_COLD;

	retval = stm1_crosspoint_map_config(&temp_cfg,0);

	if(SUCCESS != retval)
	{
		/* Update the external dB value to local dB */
		cel_sdh_stm1_read_external_cfg(STM1_CROSSPOINT_CONFIG,\
								&sdh_config_db.chip_config.stm1_cp);
		retval = UI_RESULT_FAILED;
	}
	else
	{
		/* Copy the configured the data from local to external buffer */
		cel_sdh_stm1_user_configure(STM1_CROSSPOINT_CONFIG,STM1_SET,\
								&sdh_config_db.chip_config.stm1_cp);
	}

	free(temp_cfg.config_data);
	return retval;
}

/******************************************************************************
 * Description : This function is used to check whether to monitor
 *               STM Timing Reference Unlocked Alarm for
 *               Primary/Secondary/Tertiary clock sources.
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_VOID cil_sdh_stm1_check_timing_monitor_conditions(bool *stm1_timing_monitor_conditions)
{
   if(sdh_config_db.chip_config.stm1_sync.sync_mode == STM1_SYNC_MANUAL)
   {
        if(sdh_config_db.chip_config.stm1_sync.primary != TIME_REF_STM1_FREE_RUNNING)
        {
            stm1_timing_monitor_conditions[STM1_SYNC_PRIMARY] = TRUE;
        }

        if((sdh_config_db.chip_config.stm1_sync.secondary != TIME_REF_STM1_HOLDOVER) && \
            (sdh_config_db.chip_config.stm1_sync.secondary!= TIME_REF_STM1_FREE_RUNNING))
        {
            stm1_timing_monitor_conditions[STM1_SYNC_SECONDARY] = TRUE;
        }

        if((sdh_config_db.chip_config.stm1_sync.tertiary != TIME_REF_STM1_HOLDOVER) && \
            (sdh_config_db.chip_config.stm1_sync.tertiary != TIME_REF_STM1_FREE_RUNNING))
        {
            stm1_timing_monitor_conditions[STM1_SYNC_TERTIARY] = TRUE;
        }
    }
	return;
}

/******************************************************************************
 *  						SOCKET RELATED APIs
 ******************************************************************************/

#ifdef USE_IPC_UDP_SOCKET
/******************************************************************************
 * Description : Initializing alarm udp socket to send processed alarm from
 *				 logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_client_udp_init(gal_socket_client_S *galPtr, PAL_INT_32 ipc_queue_cnt, Alarm_IPC_Type_E ipc_type)
{
	gal_socket_client_S gal_socket_client;

	memset(&gal_socket_client, 0, sizeof(gal_socket_client_S));

	switch(ipc_type)
	{
		case ALARM_QUEUEING:
		{
			switch(ipc_queue_cnt)
			{
				case SDH_STM1_ALARM_QUEUE:
				{
					/* Set Socket Path */
					gal_socket_client.sock_path 	= UDP_SDH_ALARM_QUEUE_SOCKET;
					/* Set Socket Timeout */
					gal_socket_client.tv.tv_sec 	= 0;
					gal_socket_client.tv.tv_usec 	= 1000;
				}
				break;

				default:
					return FAILURE;
			}
		}

		case ALARM_LOGGING:
		{
			switch(ipc_queue_cnt)
			{
				case SDH_STM1_ALARM_LOG:
				{
					/* Set Socket Path */
					gal_socket_client.sock_path 	= UDP_SDH_ALARM_LOG_SOCKET;
					/* Set Socket Timeout */
					gal_socket_client.tv.tv_sec 	= 0;
					gal_socket_client.tv.tv_usec 	= 1000;
				}
				break;

				default:
					return FAILURE;

			}
		}
		default:
			return FAILURE;
	}

	/* create the socket now */
	if(create_udp_send_socket(&gal_socket_client))
			galPtr = &gal_socket_client;
	else
	{
		galPtr = NULL;
		return -1;
	}
}

/******************************************************************************
 * Description : Initializing alarm udp socket to receive processed alarm from
 *				 logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_receive_service_udp_init(gal_socket_server_S *galPtr)
{
	int retval = -1;
	gal_socket_server_S	*gal_socket_server = NULL;

	gal_socket_server = (gal_socket_server_S*)galPtr;

	if(gal_socket_server->udp_service.sock_path != NULL)
	{
		printf("create_udp_receive_socket for alarm queue\n");
		/* Set up the UDP listening socket */
		if((retval = create_udp_receive_socket(&(gal_socket_server->udp_service)))<=0)
		{
			printf("Cannot Create UDP Receive Socket for alarm queue\n");
			return retval;
		}
		gal_socket_server->service_status[UDP_LISTENER] = TRUE;
	}
	return retval;
}

/******************************************************************************
 * Description: The various functionalities carried out by this function are.
 *
 * 1) Sends an alarm message to Alarm Action Task.  Creates the socket, if necessary
 *
 * Inputs:
 *		1) Msg  - The alarm message structure to be logged.
 *
 * Outputs: Success/Failure,Send the alarm message to the alarm log server.
 *
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_send_alm_to_action_task(Sdh_Alm_Mntr_Frmt_S msgbuf, PAL_VOID *ptr, PAL_INT_32 ipc_queue_cnt, Alarm_IPC_Type_E ipc_type)
{
	gal_socket_client_S gal_socket_client;
	gal_socket_client_S *galPtr = NULL;
	int retval = -1;

	galPtr = (gal_socket_client_S *)ptr;

	if(galPtr->fd == -1)
	{
		/* Initialize UDP Socket */
		if((retval = cil_sdh_stm1_client_udp_init(&gal_socket_client, ipc_queue_cnt, ipc_type))<0)
		{
			return retval;
		}
		galPtr = &gal_socket_client;
	}
	if(galPtr)
	{
		/* Send the message */
		while(((retval = sendto(galPtr->fd, &msgbuf, sizeof(Sdh_Alm_Mntr_Frmt_S), 0,\
						galPtr->udp_sock.saPtr, galPtr->udp_sock.addrSize))<0) && (errno == EINTR))
		{
			printf("Failed to send errno =%d :%s\n",errno,strerror(errno));
			continue;
		}

		/* Close the socket if we had to create it here */
		if(galPtr == &gal_socket_client)
			close(galPtr->fd);
	}
	else
	{
		printf("galPtr is NULL\n");
		return ERROR_CANT_OPEN_SOCKET;
	}
	return retval;
} /* end of if_queue_alarm_from_logger */

#endif


#ifdef USE_IPC_MQ
/******************************************************************************
 * Description : Initializing alarm message queue to receive processed alarm from
 *				 logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_client_mq_init(mqd_t *mmq, PAL_INT_32 ipc_queue_cnt, Alarm_IPC_Type_E ipc_type)
{
	PAL_INT_32 oflag = (O_RDWR| O_CREAT| O_NONBLOCK);
	mode_t mode = (S_IRUSR| S_IWUSR);
	struct mq_attr *attr = NULL;

	switch(ipc_type)
	{
		case ALARM_QUEUEING:
		{
			switch(ipc_queue_cnt)
			{
				case SDH_STM1_ALARM_QUEUE:
				{
					/*Unlink Before Open */
					mq_unlink(UDP_SDH_ALARM_QUEUE);
					/*Attributes can be udjusted if required */
					#ifdef SET_MAX_MQ_MSG_100
						attr->mq_maxmsg  = 100;
					#endif
					#ifdef SET_MAX_MQ_SIZE_8K
						attr->mq_msgsize = 8192;
					#endif

					printf("Open mailer message queue\n");
					if ((*mmq = mq_open(MQ_SDH_ALARM_QUEUE, oflag, mode, attr)) <0)
					{
						printf("ERROR: mq_open errno(%d): %s\n", errno,strerror(errno));
						return ERROR_CANT_OPEN_MQ;
					}
				}
				break;

				default:
					return FAILURE;
			}

		}

		case ALARM_LOGGING:
		{
			switch(ipc_queue_cnt)
			{
				case SDH_STM1_ALARM_LOG:
				{
					/*Unlink Before Open */
					mq_unlink(UDP_SDH_ALARM_LOG_QUEUE);
					/*Attributes can be udjusted if required */
					#ifdef SET_MAX_MQ_MSG_100
						attr->mq_maxmsg  = 100;
					#endif
					#ifdef SET_MAX_MQ_SIZE_8K
						attr->mq_msgsize = 8192;
					#endif

					printf("Open mailer message queue\n");
					if ((*mmq = mq_open(MQ_SDH_ALARM_LOG_QUEUE, oflag, mode, attr)) <0)
					{
						printf("ERROR: mq_open errno(%d): %s\n", errno,strerror(errno));
						return ERROR_CANT_OPEN_MQ;
					}
				}
				break;

				default
					return FAILURE;
			}
		}
		default:
			return FAILURE;
	}

	return SUCCESS;
}

/******************************************************************************
 * Description : Initializing alarm message queue to receive preocessed alarm from
 *				 logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
int cil_sdh_stm1_receive_service_mq_init(mqd_t *mmq)
{
	int oflag = (O_RDONLY | O_NONBLOCK);
	mode_t mode = (S_IRUSR| S_IWUSR);
	struct mq_attr *attr = NULL;

	/*Attributes can be udjusted if required */
	#ifdef SET_MAX_MQ_MSG_100
		attr->mq_maxmsg  = 100;
	#endif
	#ifdef SET_MAX_MQ_SIZE_8K
		attr->mq_msgsize = 8192;
	#endif

	printf("Open debug message queue\n");
	if ((*mmq = mq_open(ALARM_QUEUE, oflag, mode, attr)) <0)
	{
		printf("ERROR: mq_open errno(%d): %s\n", errno,strerror(errno));
		return ERROR_CANT_OPEN_MQ;
	}
}

/******************************************************************************
 * Description : Queuing the processed alarm from logger through message queue

 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
/* Compilation Error Fix: */
int cil_sdh_stm1_send_alm_to_action_task(Sdh_Alm_Mntr_Frmt_S msgbuf, PAL_VOID *ptr, PAL_INT_32 ipc_queue_cnt, Alarm_IPC_Type_E ipc_type)
{
	int retval = -1;
	mqd_t lmmq;
	unsigned int txmsgpri = 1;
	mqd_t *mmq = NULL;

	mmq = (mqd_t *)ptr;

	if(!mmq)
	{
		/* Initialize message queue */
		if((retval = cil_sdh_stm1_client_mq_init(&lmmq, ipc_queue_cnt, ipc_type))<)
		{
			return retval;
		}
		mmq = &lmmq;
	}

	if(mmq)
	{
		/* Retry when the Failure is due to interrupt */
		while(((retval = mq_send(*mmq, &msgbuf, sizeof(Sdh_Alm_Mntr_Frmt_S), txmsgpri))<0) && (errno == EINTR))
		{
			printf("ERROR: mq_send errno(%d): %s\n", errno,strerror(errno));
			continue;
		}

		/* Close the message queue if we had to create it here */
		if(mmq == &lmmq)
			mq_close(*mmq);
	}
	else
	{
		printf("mmq is NULL\n");
		return ERROR_CANT_OPEN_MQ;
	}
	return retval;
} /* end of if_queue_alarm_from_logger */
#endif

/******************************************************************************
 * Description : Initializing alarm ipc for send the queued and processed alarm
 *				 information from logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_ipc_client_init(PAL_VOID *ptr, PAL_INT_32 ipc_queue_cnt, Alarm_IPC_Type_E ipc_type)
{
	PAL_INT_32 retval = 0;

	#ifdef USE_IPC_UDP_SOCKET
	gal_socket_client_S *galPtr = NULL;

	galPtr = (gal_socket_client_S *)ptr;
	if((retval = cil_sdh_stm1_client_udp_init(galPtr, ipc_queue_cnt, ipc_type))<0)
	{
		return retval;
	}
	#endif

	#ifdef USE_IPC_MQ
	mqd_t *mmq = NULL;

	mmq = (mqd_t *)ptr;
	if((retval = cil_sdh_stm1_client_mq_init(mmq, ipc_queue_cnt, ipc_type))<0)
	{
		return retval;
	}
	#endif
	return retval;
}

/******************************************************************************
 * Description : Initializing alarm ipc for receive the queued and processed alarm
 *				 information from logger
 *
 * Inputs      :
 *
 * Outputs     :
 ******************************************************************************/
int cil_sdh_stm1_ipc_service_init(void *ptr)
{
	int retval = 0;

	#ifdef USE_IPC_UDP_SOCKET
	gal_socket_server_S *galPtr = NULL;

	galPtr = (gal_socket_server_S *)ptr;
	if((retval = cil_sdh_stm1_receive_service_udp_init(galPtr))<0)
	{
		return retval;
	}
	#endif

	#ifdef USE_IPC_MQ
	mqd_t *mmq = NULL;

	mmq = (mqd_t *)ptr;
	if((retval = cil_sdh_stm1_receive_service_mq_init(mmq))<)
	{
		return retval;
	}
	#endif
	return retval;
}

/******************************************************************************
 * Description: The various functionalities carried out by this function are.
 *
 * 1) Sends an mail message to mailer thread.  Creates the socket or messege queue
 *    or FIFO
 *
 * Inputs:
 *		1) Msg  - The mail message structure to be logged.
 *
 * Outputs: Send the mail message to the mailer.
 *
 ******************************************************************************/

PAL_INT_32 cil_sdh_stm1_recv_alm_from_monitor_task(Sdh_Alm_Mntr_Frmt_S *msgbuf, PAL_VOID *ptr)
{
	PAL_INT_32 retval = -1;

	#ifdef USE_IPC_MQ
	unsigned int rxmsgpri = 1;
	mqd_t *mmq = NULL;

	mmq = (mqd_t *)ptr;
	if(mmq)
	{
		/* Retry when the Failure is due to interrupt */
		while(((retval = mq_receive(*mmq, msgbuf, sizeof(Sdh_Alm_Mntr_Frmt_S), &rxmsgpri))<0) && (errno == EINTR))
		{
			printf("ERROR: mq_send errno(%d): %s\n", errno,strerror(errno));
			continue;
		}
	}
	else
	{
		printf("mmq is NULL\n");
		return ERROR_CANT_OPEN_MQ;
	}
	#endif

	#ifdef USE_IPC_UDP_SOCKET
    int fromlen;
    gal_socket_server_S *galPtr = NULL;

    galPtr = (gal_socket_server_S *)ptr;

	if(galPtr)
	{
		fromlen = sizeof(galPtr->udp_service.udp_sock.sun);
		while(((retval = recvfrom(galPtr->udp_service.udp_sock.fd, msgbuf, sizeof(Sdh_Alm_Mntr_Frmt_S),0, \
												(struct sockaddr*)&galPtr->udp_service.udp_sock.sun, &fromlen))<0) && (errno == EINTR))
		{
			printf("ERROR: recvfrom errno(%d): %s\n", errno,strerror(errno));
			continue;
		}
	}
	else
	{
		printf("galPtr is NULL\n");
		return ERROR_CANT_OPEN_MQ;
	}
	#endif
	return retval;
} /* end of receive_mail_from_logger */

/******************************************************************************
 * Description: This function registers the signals
 *
 * Inputs: Signal
 * Outputs: Success/Failure
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_register_signal(PAL_INT_32 sig)
{
	PAL_INT_32 retval=0;
	//TBD
	return retval;
}

/******************************************************************************
 * Description		: This function will:
 *					  1.Monitor the IPC continuously for data
 *					  2.Calls the relevant functions to process it
 * Inputs     		:
 * Output    		: To monitor the IPC for data
 * Return Value		: SUCCESS on success/FAILURE on failure
 ******************************************************************************/

 PAL_INT_32 cil_sdh_stm1_monitor_ipc(PAL_VOID)
 {
	printf(" Entered %s \n", __FUNCTION__);
	//TBD
	printf(" Leaving %s \n", __FUNCTION__);
	return SUCCESS;
 } /* end of cil_sdh_stm1_monitor_ipc() */


PAL_INT_32 cil_sdh_stm1_port_config_ltype_lcoding_get(Stm1_Port_Params_S *port_config )
{
	PAL_INT_32 retval=0;

	//retval = hil_sdh_stm1_port_config_ltype_get(port_config->line_type);
	retval = hil_sdh_stm1_port_config_ltype_get(port_config->line_type);
	if(FAILURE == retval )
	{
		printf("Function hil_sdh_stm1_port_config_ltype_get() returns FAILURE");
		return retval;
	}
	retval = hil_sdh_stm1_port_config_lcoding_get(port_config->line_coding);
	if(FAILURE == retval )
	{
		printf("Function hil_sdh_stm1_port_config_ltype_get() returns FAILURE");
		return retval;
	}
	return retval;
}


/******************************************************************************
 * FUNCTIONs to be rechecked to add or remove
 *****************************************************************************/
#if 0 // Needs to be checked
PAL_INT_32 cil_sdh_stm1_port_config_ltype_lcode_get(Stm1_Port_Params_S *ptr_port_config)
{
	PAL_INT_32 retval = 0,ltype=0,lcode=0;
	retval = cel_sdh_stm1_port_config_ltype_lcode_get(&ltype,&lcode);
	if(FAILURE == retval)
		return retval;

	/* Copy the data to local databse */
	ptr_port_config->line_type 		= ltype;
	ptr_port_config->line_coding 	= lcode;

	return retval;
}
#endif

/* Copied from kernel */
/*
 * change timeval to jiffies, trying to avoid the
 * most obvious overflows..
 *
 * The tv_*sec values are signed, but nothing seems to
 * indicate whether we really should use them as signed values
 * when doing itimers. POSIX doesn't mention this (but if
 * alarm() uses itimers without checking, we have to use unsigned
 * arithmetic).
 */

#define ULONG_MAX 0xFFFFFFFFUL

static unsigned long tvtojiffies(struct timeval *value)
{
	unsigned long sec = (unsigned) value->tv_sec;
	unsigned long usec = (unsigned) value->tv_usec;

	if (sec > (ULONG_MAX / HZ))
		return ULONG_MAX;
	/* We will not round off the usec */
	//usec += 1000000 / HZ - 1;
	usec /= 1000000 / HZ;
	return HZ*sec+usec;
}

/******************************************************************************
 * Description : This function gives the maximum number of cores present in current hw
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_VOID cil_sdh_stm1_max_core_num_get(PAL_UINT_32 *num_core)
{
	*num_core = sdh_config_db.core_config.md_config.sdh_card_info.elems[0].max_stm_port_core;
  	return;
}

PAL_VOID cil_sdh_stm1_max_lp_get(PAL_USHORT_16 *max_lower_order_path)
{
	printf(" Entered %s \n", __FUNCTION__);
	*max_lower_order_path = MAX_LOW_ORDER_PATHS;
	/* TBD */
	printf(" Leaving %s \n", __FUNCTION__);
}

PAL_VOID cil_sdh_stm1_max_hp_get(PAL_USHORT_16 *max_lower_order_path)
{
	printf(" Entered %s \n", __FUNCTION__);
	*max_lower_order_path = MAX_HIGH_ORDER_PATHS;
	/* TBD */
	printf(" Leaving %s \n", __FUNCTION__);
}

/*********** SNCP Recalculate Procedure on E1T1 Cross-Point update ************/

/******************************************************************************
 * Description :This function will very that protection mapping belongs to SNCP.
 * Inputs      :Map Count, Submap Count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 verify_sncp_chmap(PAL_INT_32 map_cnt, PAL_INT_32 sub_map_cnt)
{

	PAL_INT_32 working_channel = 0;
	PAL_INT_32 prot_channel = 0;

	working_channel = e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt-1];
	prot_channel	= e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt];

	switch(working_channel/1000)
	{
		case ADM_AMA_PORT:
		case ADM_AMB_PORT:
			break;

		default:
			return FAILURE;

	}

	switch(prot_channel/1000)
	{
		case ADM_AMA_PORT:
		case ADM_AMB_PORT:
			break;

		default:
			return FAILURE;

	}
	return SUCCESS;
}

/******************************************************************************
 * Description :This function will get all the mappings that needs to be removed
 *				from SNCP configuration.
 * Inputs      :Protection mapping, Pointer to store removal SNCP mapping, pointer
 *				to get the number of delete mappings
 * Outputs     :Success or Failure.
 ******************************************************************************/

PAL_INT_32 get_removable_sncp_chmap(Stm1_Prot_Parms_S *prot_parms, Sdh_Sncp_Parms_S *delete_sncp_parms, PAL_INT_32 *sncp_delete_cnt)
{
	PAL_INT_32 map_cnt = 0, sub_map_cnt = 0;
	PAL_INT_32 l_sncp_delete_cnt = 0;
	PAL_INT_32 retval;

	for(map_cnt=0; map_cnt<	e1t1_cp_map_filter.delete_map_cnt; map_cnt++)
	{
		for(sub_map_cnt = 0; sub_map_cnt<e1t1_cp_map_filter.cp_data[map_cnt].max_sub_elements; sub_map_cnt++)
		{
			printf("e1t1_cp_map_filter.cp_data[%d].map_element[%d] = %s \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_element[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_status[%d]  = %d \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_array[%d]  	= %d \n", map_cnt, sub_map_cnt,e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt]);

			/* 0-Input Mapping,  1- Combination Mapping(Output),  2- Protection Mapping */
			if((PROTECTOR_MAPPING+1) == e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt])
			{
				if((retval = verify_sncp_chmap(map_cnt, sub_map_cnt))<0)
				{
					break;
				}
				else
				{
					/* Default all the deleted protection mapping */
					cel_sdh_stm1_default_prot_sncp_config(&delete_sncp_parms[l_sncp_delete_cnt]);
					/* Set Back the Working and Protection Path */
					delete_sncp_parms[l_sncp_delete_cnt].sncp_couplet.working_channel 	= e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt-1];
					delete_sncp_parms[l_sncp_delete_cnt].sncp_couplet.prot_channel 		= e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt];
					/* Set the Config Flag to remove the mapping from hardware */
					delete_sncp_parms[l_sncp_delete_cnt].sncp_couplet.config_flag		= -1;
					l_sncp_delete_cnt++;

				}
			}
		}
	}
	*sncp_delete_cnt = l_sncp_delete_cnt;

	return retval;
}

/******************************************************************************
 * Description :This function will get all the new mappings that needs to be added
 *				for SNCP configuration.
 * Inputs      :Protection mapping, Pointer to store new SNCP mapping, pointer
 *				to get the number of new sncp mappings
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 get_new_sncp_chmap(Stm1_Prot_Parms_S *prot_parms, Sdh_Sncp_Parms_S *new_sncp_parms, PAL_INT_32 *sncp_new_cnt)
{
	PAL_INT_32 map_cnt = 0, sub_map_cnt = 0;
	PAL_INT_32 l_sncp_new_cnt = 0;
	PAL_INT_32 retval;

	for(map_cnt=e1t1_cp_map_filter.delete_map_cnt; map_cnt<\
						(e1t1_cp_map_filter.delete_map_cnt + e1t1_cp_map_filter.new_map_cnt); map_cnt++)
	{
		for(sub_map_cnt = 0; sub_map_cnt<e1t1_cp_map_filter.cp_data[map_cnt].max_sub_elements; sub_map_cnt++)
		{
			printf("e1t1_cp_map_filter.cp_data[%d].map_element[%d] = %s \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_element[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_status[%d]  = %d \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_array[%d]  	= %d \n", map_cnt, sub_map_cnt,e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt]);

			/* 0-Input Mapping,  1- Combination Mapping(Output),  2- Protection Mapping */
			if((PROTECTOR_MAPPING+1) == e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt])
			{
				if((retval = verify_sncp_chmap(map_cnt, sub_map_cnt))<0)
				{
					break;
				}
				else
				{
					/* Default all the new protection mapping */
					cel_sdh_stm1_default_prot_sncp_config(&new_sncp_parms[l_sncp_new_cnt]);
					/* Set Back the Working and Protection Path */
					new_sncp_parms[l_sncp_new_cnt].sncp_couplet.working_channel 	= e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt-1];
					new_sncp_parms[l_sncp_new_cnt].sncp_couplet.prot_channel 		= e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt];
					/* Set the Config Flag to add the mapping to hardware */
					new_sncp_parms[l_sncp_new_cnt].sncp_couplet.config_flag			= 1;
					l_sncp_new_cnt++;

				}
			}
		}
	}
	*sncp_new_cnt = l_sncp_new_cnt;

	return retval;
}

/******************************************************************************
 * Description :This function will get all the existing mappings that needs to
 *				untouched in SNCP configuration.
 * Inputs      :Protection mapping, Pointer to store existing SNCP mapping,
 *				pointer to get the number of existing sncp mappings
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 get_unused_sncp_chmap(Stm1_Prot_Parms_S *prot_parms, Sdh_Sncp_Parms_S *unused_sncp_parms, PAL_INT_32 *sncp_unused_cnt)
{
	PAL_INT_32 map_cnt = 0, sub_map_cnt = 0;
	PAL_INT_32 key = 0, loc = -1;
	PAL_INT_32 *search_array;
	PAL_INT_32 l_sncp_unused_cnt = 0;
	PAL_INT_32 retval = 0;

	search_array = malloc(sizeof(PAL_INT_32)*(prot_parms->max_sncp_config));
	for(map_cnt=0; map_cnt<prot_parms->max_sncp_config; map_cnt++)
	{
		search_array[map_cnt] = prot_parms->sncp_parms.sncp_prot_data[map_cnt].sncp_couplet.working_channel;
	}

	for(map_cnt=(e1t1_cp_map_filter.delete_map_cnt+e1t1_cp_map_filter.new_map_cnt); map_cnt<\
						(e1t1_cp_map_filter.delete_map_cnt + e1t1_cp_map_filter.new_map_cnt + e1t1_cp_map_filter.unused_map_cnt); \
								map_cnt++)
	{
		for(sub_map_cnt = 0; sub_map_cnt<e1t1_cp_map_filter.cp_data[map_cnt].max_sub_elements; sub_map_cnt++)
		{
			printf("e1t1_cp_map_filter.cp_data[%d].map_element[%d] = %s \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_element[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_status[%d]  = %d \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_array[%d]  	= %d \n", map_cnt, sub_map_cnt,e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt]);

			/* 0-Input Mapping,  1- Combination Mapping(Output),  2- Protection Mapping */
			if((PROTECTOR_MAPPING+1) == e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt])
			{
				if((retval = verify_sncp_chmap(map_cnt, sub_map_cnt))<0)
				{
					break;
				}

				else
				{
					key = e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt-1];
					if((loc = binary_search(search_array, prot_parms->max_sncp_config, key))<0)
					{
						printf("SNCP Mapping Not found : %d \n", key);
					}
					else
					{
						/* Verify the Protection Path */
						if(prot_parms->sncp_parms.sncp_prot_data[loc].sncp_couplet.prot_channel == \
												e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt] )
						{
							/* Keep the previous Mapping */
							memcpy(&unused_sncp_parms[l_sncp_unused_cnt], \
									&prot_parms->sncp_parms.sncp_prot_data[loc], sizeof(Sdh_Sncp_Parms_S));
							/* Set the Config Flag not to configure the mapping to hardware */
							unused_sncp_parms[l_sncp_unused_cnt].sncp_couplet.config_flag		= 0;
							l_sncp_unused_cnt++;
						}
						else
						{
							printf("SNCP Prot Mapping Not Matched : %d \n", key);
						}
					}
				}
			}
		}
	}
	*sncp_unused_cnt = l_sncp_unused_cnt;
	free(search_array);

	return retval;
}

/******************************************************************************
 * Description :This function will get all the required SNCP mapping for
 *				SNCP configuration
 * Inputs      :Pointer to Protection mapping structure
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 extract_sncp_chmap(Stm1_Prot_Parms_S *prot_parms)
{
	PAL_INT_32 map_cnt = 0;
	PAL_INT_32 sncp_delete_cnt = 0;
	PAL_INT_32 sncp_new_cnt = 0;
	PAL_INT_32 sncp_unused_cnt = 0;
	PAL_INT_32 retval = 0;
	Sdh_Sncp_Parms_S *delete_sncp_parms;
	Sdh_Sncp_Parms_S *new_sncp_parms;
	Sdh_Sncp_Parms_S *unused_sncp_parms;

	delete_sncp_parms	= malloc(sizeof(Sdh_Sncp_Parms_S)*(e1t1_cp_map_filter.delete_map_cnt));
	new_sncp_parms		= malloc(sizeof(Sdh_Sncp_Parms_S)*(e1t1_cp_map_filter.new_map_cnt));
	unused_sncp_parms 	= malloc(sizeof(Sdh_Sncp_Parms_S)*(e1t1_cp_map_filter.unused_map_cnt));

	get_removable_sncp_chmap(prot_parms, delete_sncp_parms, &sncp_delete_cnt);
	get_new_sncp_chmap(prot_parms, new_sncp_parms, &sncp_new_cnt);
	get_unused_sncp_chmap(prot_parms, unused_sncp_parms, &sncp_unused_cnt);

	memset(&(prot_parms->sncp_parms), 0, sizeof(Sdh_Sncp_Config_S));

	memcpy(prot_parms->sncp_parms.sncp_prot_data, \
					delete_sncp_parms, sizeof(Sdh_Sncp_Parms_S)*sncp_delete_cnt);
	memcpy(&(prot_parms->sncp_parms.sncp_prot_data[sncp_delete_cnt]), \
						new_sncp_parms, sizeof(Sdh_Sncp_Parms_S)*sncp_new_cnt);
	memcpy(&(prot_parms->sncp_parms.sncp_prot_data[sncp_delete_cnt+sncp_new_cnt]), \
							unused_sncp_parms, sizeof(Sdh_Sncp_Parms_S)*sncp_unused_cnt);

	chip_config.prot_parms.max_sncp_config = (sncp_delete_cnt + sncp_new_cnt + sncp_unused_cnt);

/*	for(map_cnt = 0; map_cnt<chip_config.prot_parms.max_sncp_config; map_cnt++)
	{
		printf("prot_parms->sncp_parms.sncp_prot_data[%d].sncp_couplet.working_channel : %d \n", map_cnt, prot_parms->sncp_parms.sncp_prot_data[map_cnt].sncp_couplet.working_channel);
		printf("prot_parms->sncp_parms.sncp_prot_data[%d].sncp_couplet.prot_channel : %d \n", map_cnt, prot_parms->sncp_parms.sncp_prot_data[map_cnt].sncp_couplet.prot_channel);
	}*/

	free(unused_sncp_parms);
	free(new_sncp_parms);
	free(delete_sncp_parms);

	return retval;
}

/******************************************************************************
 * Description :This function will update the SNCP database with respect to
 *				the new E1T1 cross-point mapping
 * Inputs      :N/A
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_update_sncp_db(PAL_VOID)
{
	PAL_INT_32 retval = 0;

	/* Sort the Existing mapping */
	qsort_sncp_map(&chip_config.prot_parms.sncp_parms, 0, chip_config.prot_parms.max_sncp_config);

	/* Extract the SNCP Mapping */
	extract_sncp_chmap(&chip_config.prot_parms);

	return retval;
}

/******************************************************************************
 * Description :This function will re-update the SNCP database with respect to
 *				the new E1T1 cross-point mapping after configuring all mappings
 * Inputs      :Pointer to SNCP mapping structure, Pointer to SNCP Count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_reinit_sncp_db(Sdh_Sncp_Config_S *sncp_parms, PAL_UCHAR *sncp_cnt)
{
	PAL_INT_32 			retval = 0;
	PAL_INT_32 			map_cnt = 0;
	PAL_INT_32			sncp_valid_cnt = 0;
	Sdh_Sncp_Parms_S	*l_sncp_prot_data;


	l_sncp_prot_data	= malloc(sizeof(Sdh_Sncp_Parms_S)*(*sncp_cnt));

	for(map_cnt = 0; map_cnt<*sncp_cnt; map_cnt++)
	{
		if(sncp_parms->sncp_prot_data[map_cnt].sncp_couplet.config_flag>=0)
		{
			memcpy(&l_sncp_prot_data[sncp_valid_cnt], &sncp_parms->sncp_prot_data[map_cnt], sizeof(Sdh_Sncp_Parms_S));
			sncp_valid_cnt++;
		}
	}

	memset(sncp_parms->sncp_prot_data, 0, sizeof(Sdh_Sncp_Parms_S)* (*sncp_cnt));
	memcpy(sncp_parms->sncp_prot_data, l_sncp_prot_data, sizeof(Sdh_Sncp_Parms_S)*sncp_valid_cnt);

	*sncp_cnt = sncp_valid_cnt;

	free(l_sncp_prot_data);
	return retval;
}

/******************************************************************************
 * Description :This function will Configure the  SNCP mapping to the hardware
 *				and recalculate the current valid SNCP mapping and sort it
 * Inputs      :Pointer to SNCP mapping structure, Pointer to SNCP Count
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_sncp_configure(PAL_VOID)
{
	PAL_INT_32 retval = 0;

	if((retval = hil_sdh_stm1_prot_sncp_config_update(chip_config.prot_parms, chip_config.prot_parms.max_sncp_config, STM1_PROT_SET))<0)
	{
		printf(" Function hil_sdh_stm1_prot_sncp_config_set() retuned FAILURE ");
	}

	cil_sdh_stm1_reinit_sncp_db(&chip_config.prot_parms.sncp_parms, &chip_config.prot_parms.max_sncp_config);

	/* Sort the Existing mapping */
	qsort_sncp_map(&chip_config.prot_parms.sncp_parms, 0, chip_config.prot_parms.max_sncp_config);
/*	PAL_UINT_32 map_cnt = 0;
	for(map_cnt = 0; map_cnt<chip_config.prot_parms.max_sncp_config; map_cnt++)
	{
		printf("chip_config.prot_parms.sncp_parms.sncp_prot_data[%d].sncp_couplet.working_channel : %d \n", map_cnt, chip_config.prot_parms.sncp_parms.sncp_prot_data[map_cnt].sncp_couplet.working_channel);
		printf("chip_config.prot_parms.sncp_parms.sncp_prot_data[%d].sncp_couplet.prot_channel : %d \n", map_cnt, chip_config.prot_parms.sncp_parms.sncp_prot_data[map_cnt].sncp_couplet.prot_channel);
	}*/
	return retval;
}

/******************************************************************************
 * Description : This function will update the mask bits according to the configuraiton
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_pm_intr_mask_set(void)
{
	signed int retval=SUCCESS;

	if((retval = hil_sdh_stm1_adm_pm_intr_mask_set(SET_ALL))<0)
	{
		printf(" Function  hil_sdh_stm1_sncp_intr_mask_set() returned failure in ALL_SET case\n");
	}

	return retval;
}

/******************************************************************************
 * Description : This function will update the mask bits according to the configuraiton
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_intr_mask_set()
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		if( SDH_OPMODE_MUXDEMUX == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			retval = hil_sdh_stm1_adm_intr_mask_set(adm_core_index,SET_ALL);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_mask_set() returned failure in ALL_SET case\n");
			}
		}
		else if(SDH_OPMODE_REGENERATOR == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			retval = hil_sdh_stm1_adm_intr_mask_set(adm_core_index,SET_REGENERATOR);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_mask_set() returned failure in REGENERATOR_SET case\n");
			}
		}
		else
		{
			retval = hil_sdh_stm1_adm_intr_mask_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_mask_set() returned failure in ALL_UNSET case\n");
			}
		}
	}

 return retval;
}


/******************************************************************************
 * Description : This function will update the mask bits according to the configuraiton
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_sncp_intr_mask_set()
{
	PAL_INT_32 retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		retval = hil_sdh_stm1_sncp_intr_mask_set(adm_core_index,SET_ALL);
		if(FAILURE == retval)
		{
			printf(" Function  hil_sdh_stm1_sncp_intr_mask_set() returned failure in ALL_SET case\n");
		}
	}

	return retval;
}

/******************************************************************************
 * Description : This function will update the mask bits according to the configuraiton
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_msp_intr_mask_set()
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		retval = hil_sdh_stm1_msp_intr_mask_set(adm_core_index,SET_ALL);
		if(FAILURE == retval)
		{
			printf(" Function  hil_sdh_stm1_msp_intr_mask_set() returned failure in ALL_SET case\n");
		}
	}

 return retval;
}

/******************************************************************************
 * Description : This function will update the controll register bits.
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_pn_intr_control_set(void)
{
	int retval=SUCCESS;

	if((retval = hil_sdh_stm1_pn_intr_cntrl_set(SET_ALL))<0)
	{
		printf(" Function  hil_sdh_stm1_msp_intr_cntrl_set() returned failure  in ALL_SET case\n");
	}
 	return retval;
}

/******************************************************************************
 * Description : This function will update the controll register bits.
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_intr_control_set()
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		if( SDH_OPMODE_MUXDEMUX == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			retval = hil_sdh_stm1_adm_intr_control_set(adm_core_index,SET_ALL);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_control_set() returned failure  in ALL_SET case\n");
			}
		}
		else if(SDH_OPMODE_REGENERATOR == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			retval = hil_sdh_stm1_adm_intr_control_set(adm_core_index,SET_REGENERATOR);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_control_set() returned failure in REGENERATOR_SET case \n");
			}
		}
		else
		{
			retval = hil_sdh_stm1_adm_intr_control_set(adm_core_index,UNSET_ALL);
			if(FAILURE == retval)
			{
				printf(" Function  hil_sdh_stm1_adm_intr_control_set() returned failure in ALL_UNSET case \n");
			}
		}
	}

 return retval;
}


/******************************************************************************
 * Description : This function will update the controll register bits.
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_sncp_intr_control_set(void)
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		retval = hil_sdh_stm1_sncp_intr_cntrl_set(adm_core_index,SET_ALL);
		if(FAILURE == retval)
		{
			printf(" Function  hil_sdh_stm1_sncp_intr_cntrl_set() returned failure  in ALL_SET case\n");
		}
	}

 	return retval;
}

/******************************************************************************
 * Description : This function will update the controll register bits.
 *
 * Outputs     : None.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_msp_intr_control_set()
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		retval = hil_sdh_stm1_msp_intr_cntrl_set(adm_core_index,SET_ALL);
		if(FAILURE == retval)
		{
			printf(" Function  hil_sdh_stm1_msp_intr_cntrl_set() returned failure  in ALL_SET case\n");
		}
	}
 	return retval;
}

/******************************************************************************
 * Description :This function will set the given bit in a char array buffer
 * Inputs      :Pointer to buffer, bit position
 * Outputs     :none
 ******************************************************************************/
PAL_INT_32 set_channel_in_bitmap(PAL_CHAR_8 *lp_ch_bitmap,PAL_SHORT_16 channel)
{
	PAL_INT_32 byte_info=0,bit_set=0;
	unsigned char bit_info[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	byte_info = channel/8;
	bit_set = channel%8;

	/* Set the corresponding lp channel bit which is mapped in 'channel map' */
	lp_ch_bitmap[byte_info] |=  bit_info[bit_set];

	return;
}
/******************************************************************************
 * Description :This function will get all the required SNCP mapping for
 *				SNCP configuration
 * Inputs      :Pointer to Protection mapping structure
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 sdh_stm1_normal_lp_ch_bitmap_get(Sdh_ADM_Core_E adm_core_index, PAL_CHAR_8 *lp_ch_bitmap, PAL_INT_32 lower_limit, PAL_INT_32 higher_limt)
{
	PAL_INT_32 map_cnt = 0;
	PAL_INT_32 retval = FAILURE;
	PAL_SHORT_16 channel=0,sub_map_cnt=0;

	for(map_cnt=e1t1_cp_map_filter.delete_map_cnt; map_cnt<\
						(e1t1_cp_map_filter.delete_map_cnt + e1t1_cp_map_filter.new_map_cnt); map_cnt++)
	{
		for(sub_map_cnt = 0; sub_map_cnt<e1t1_cp_map_filter.cp_data[map_cnt].max_sub_elements; sub_map_cnt++)
		{
			printf("e1t1_cp_map_filter.cp_data[%d].map_element[%d] = %s \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_element[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_status[%d]  = %d \n", map_cnt, sub_map_cnt, e1t1_cp_map_filter.cp_data[map_cnt].map_status[sub_map_cnt]);
			printf("e1t1_cp_map_filter.cp_data[%d].map_array[%d]  	= %d \n", map_cnt, sub_map_cnt,e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt]);

			if( (e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt] > lower_limit) &&\
						(e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt]	< higher_limt))
			{
				channel = e1t1_cp_map_filter.cp_data[map_cnt].map_array[sub_map_cnt] - lower_limit;
				if((retval  = set_channel_in_bitmap(lp_ch_bitmap,channel))<0)
				{
					return retval;
				}
			}
		}
	}
	return retval;
}

/******************************************************************************
 * Description :This function will get all the required SNCP mapping for
 *				SNCP configuration
 * Inputs      :Pointer to Protection mapping structure
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 sdh_stm1_sncp_lp_ch_bitmap_get(Sdh_ADM_Core_E adm_core_index, PAL_CHAR_8 *lp_ch_bitmap, PAL_INT_32 lower_limit, PAL_INT_32 higher_limt)
{
	PAL_INT_32 map_cnt = 0;
	PAL_INT_32 retval = FAILURE;
	PAL_SHORT_16 channel=0,sub_map_cnt=0;
	PAL_INT_32 working_channel = 0, prot_channel = 0;
	Sdh_Sncp_Config_S *sncp_parms = &chip_config.prot_parms.sncp_parms;

	for(map_cnt=0; map_cnt<chip_config.prot_parms.max_sncp_config; map_cnt++)
	{
		working_channel = sncp_parms->sncp_prot_data[map_cnt].sncp_couplet.working_channel;
		prot_channel 	= sncp_parms->sncp_prot_data[map_cnt].sncp_couplet.prot_channel;

		if((working_channel > lower_limit) && (working_channel < higher_limt))
		{
			channel = working_channel - lower_limit;
			if((retval  = set_channel_in_bitmap(lp_ch_bitmap, channel))<0)
			{
				return retval;
			}
		}

		if((prot_channel > lower_limit) && (prot_channel < higher_limt))
		{
			channel = prot_channel - lower_limit;
			if((retval  = set_channel_in_bitmap(lp_ch_bitmap, channel))<0)
			{
				return retval;
			}
		}

	}
	return retval;
}

/******************************************************************************
 * Description :This function will get all the required SNCP mapping for
 *				SNCP configuration
 * Inputs      :Pointer to Protection mapping structure
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_lp_ch_bitmap_get(Sdh_ADM_Core_E adm_core_index, PAL_CHAR_8 *lp_ch_bitmap, ADM_Map_Type_E adm_map_type)
{
	PAL_INT_32 core_offset_val_low = 0;
	PAL_INT_32 core_offset_val_high = 0;
	PAL_INT_32 retval = FAILURE;

	if(STM1_ADM_MA_A == adm_core_index)
	{
		core_offset_val_low =  e1t1_cp_info[ADM_AMA_PORT].offset;
		core_offset_val_high = e1t1_cp_info[ADM_AMA_PORT].offset+e1t1_cp_info[ADM_AMA_PORT].max_ports;
	}
	else if(STM1_ADM_MA_B == adm_core_index)
	{
		core_offset_val_low =  e1t1_cp_info[ADM_AMB_PORT].offset;
		core_offset_val_high = e1t1_cp_info[ADM_AMB_PORT].offset+e1t1_cp_info[ADM_AMB_PORT].max_ports;
	}


	switch(adm_map_type)
	{
		case NORMAL_ADM_MAP:
		{
			if((retval = sdh_stm1_normal_lp_ch_bitmap_get(adm_core_index, lp_ch_bitmap, core_offset_val_low, core_offset_val_high))<0)
			{
				return retval;
			}
		}
		break;

		case SNCP_ADM_MAP:
		{
			if((retval = sdh_stm1_sncp_lp_ch_bitmap_get(adm_core_index, lp_ch_bitmap, core_offset_val_low, core_offset_val_high))<0)
			{
				return retval;
			}
		}
		break;

		default:
			return FAILURE;
	}

	return retval;
}

/******************************************************************************
 * Description : This function will update the interrupt enable bits.
 *
 * Inputs      : NA
 *
 * Outputs     : SUCCESS/FAILURE.
 ******************************************************************************/
PAL_INT_32 sdh_stm1_adm_pm_config_get(Sdh_Stm1_Pm_Status_E pm_status, Sdh_Stm1_Pm_Config_Type_E conf_type, pm_config_S *pm_config)
{
	int retval=FAILURE;

	switch(conf_type)
	{
		case SDH_STM1_PM_BOOTUP_INIT:
		case SDH_STM1_PM_TIME_CH_INIT:
		{
			pm_config->enable 			= pm_status;
			pm_config->interval_time 	= PM_INTERVAL_TIME_DURATION-1;
			pm_config->adj_time 		= pml_calculate_remaining_15min_interval();
			pm_config->restart			= 1;
		}
		break;

		default:
			return FAILURE;
	}

	return 0;
}

/******************************************************************************
 * Description : This function will update the interrupt enable bits.
 *
 * Inputs      : NA
 *
 * Outputs     : SUCCESS/FAILURE.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_pm_config_set(Sdh_Stm1_Pm_Status_E pm_status, Sdh_Stm1_Pm_Config_Type_E conf_type)
{
	int retval=FAILURE;
	pm_config_S pm_config;

	memset(&pm_config, 0, sizeof(pm_config_S));

	sdh_stm1_adm_pm_config_get(pm_status, conf_type, &pm_config);

	if((retval = hil_sdh_stm1_adm_pm_config_set(pm_config))<0)
	{
		return retval;
	}
 	return retval;
}

/******************************************************************************
 * Description : This function will update the interrupt enable bits.
 *
 * Inputs      : NA
 *
 * Outputs     : SUCCESS/FAILURE.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_adm_intr_en_set(PAL_VOID)
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	Stm1_Lp_Channel_Bitmap_S *lp_channel_info;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	lp_channel_info = malloc(sizeof(Stm1_Lp_Channel_Bitmap_S) * max_num_core);
	memset(lp_channel_info, 0, sizeof(Stm1_Lp_Channel_Bitmap_S) * max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		cil_sdh_stm1_lp_ch_bitmap_get(adm_core_index, lp_channel_info[adm_core_index].lp_ch_bitmap, NORMAL_ADM_MAP);

		if( SDH_OPMODE_MUXDEMUX == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			hil_sdh_stm1_adm_intr_en_set(MAX_LOW_ORDER_PATHS, adm_core_index, SET_ALL, lp_channel_info[adm_core_index].lp_ch_bitmap);
		}
		else if(SDH_OPMODE_REGENERATOR == sdh_config_db.core_config.md_config.ptr_core_cfg.elems[adm_core_index].op_mode )
		{
			hil_sdh_stm1_adm_intr_en_set(MAX_LOW_ORDER_PATHS, adm_core_index,SET_REGENERATOR,lp_channel_info[adm_core_index].lp_ch_bitmap);
		}
		else
		{
			hil_sdh_stm1_adm_intr_en_set(MAX_LOW_ORDER_PATHS, adm_core_index,UNSET_ALL,lp_channel_info[adm_core_index].lp_ch_bitmap);
		}
	}
 	return retval;
}

/******************************************************************************
 * Description : This function will update the interrupt enable bits.
 *
 * Inputs      : NA
 *
 * Outputs     : SUCCESS/FAILURE.
 ******************************************************************************/
int cil_sdh_stm1_sncp_intr_en_set(PAL_VOID)
{
	int retval=SUCCESS,max_num_core=0;
	Sdh_ADM_Core_E adm_core_index=0;
	Stm1_Lp_Channel_Bitmap_S *lp_channel_info;
	/* get maximum number of core supported  */
	cil_sdh_stm1_max_core_num_get(&max_num_core);

	lp_channel_info = malloc(sizeof(Stm1_Lp_Channel_Bitmap_S) * max_num_core);
	memset(lp_channel_info, 0, sizeof(Stm1_Lp_Channel_Bitmap_S) * max_num_core);

	for(adm_core_index = 0;adm_core_index < max_num_core ; adm_core_index++)
	{
		cil_sdh_stm1_lp_ch_bitmap_get(adm_core_index, lp_channel_info[adm_core_index].lp_ch_bitmap, SNCP_ADM_MAP);
		hil_sdh_stm1_sncp_intr_en_set(MAX_LOW_ORDER_PATHS, adm_core_index,SET_ALL,lp_channel_info[adm_core_index].lp_ch_bitmap);
	}
 	return retval;
}

/******************************************************************************
 * Description :This function will update the system information required by
 *				crosspoint configuration and filtering
 * Inputs      :Pointer to Protection mapping structure
 * Outputs     :Success or Failure.
 ******************************************************************************/
PAL_INT_32 cil_sdh_stm1_update_cp_system_info(Stm1_Card_Manager_S *sdh_stm1_hw_info, Stm1_Sdh_Hw_Max_Interface_S sdh_stm1_hw_cfg, \
												 STM1AuthInfo_S stm1_auth_info)
{
	PAL_INT_32 card_index 	= 0, acm_level=0;

	acm_level 								= sdh_stm1_hw_info->current_acm_level;
	stm1_cp_limit.current_acm_level			= acm_level;
	stm1_cp_limit.max_acm_level_supported	= sdh_stm1_hw_info->max_acm_levels;
	stm1_cp_limit.vendor_info				= sdh_stm1_hw_info->vendor_info;
	stm1_cp_limit.stm1_vc_support_status	= sdh_stm1_hw_info->vc_sup_status;
	stm1_cp_limit.sncp_support				= stm1_auth_info.sncp_support;


	for(card_index = 0; card_index<MAX_SEDONA_IO; card_index++)
	{
		if(sdh_stm1_hw_info->card_hw_info[card_index].e1t1_status == PORT_TYPE_E1)
			stm1_cp_limit.max_m_field = 3;
		else
			stm1_cp_limit.max_m_field = 4;
		switch(sdh_stm1_hw_info->card_hw_info[card_index].card_type)
		{
			case SEDONA_IO1_IF:
			{
				if(sdh_stm1_hw_info->card_hw_info[card_index].sdh_stm1_info.num_stm1_adm_int<MAX_STM1_ADM_MA_PORT)
				{
					return FAILURE;
				}
				stm1_cp_limit.no_active_interface[acm_level][STM1_MAC_FPA_PORT] 	= \
						sdh_stm1_hw_info->card_hw_info[card_index].sdh_stm1_info.stm1_mac_info[0].max_stm1_mac_time_slot;
				stm1_cp_limit.no_active_interface[acm_level][STM1_MAC_FPB_PORT] 	= \
						sdh_stm1_hw_info->card_hw_info[card_index].sdh_stm1_info.stm1_mac_info[1].max_stm1_mac_time_slot;
				stm1_cp_limit.no_active_interface[acm_level][STM1_MA_PORT]	  		= sdh_stm1_hw_cfg.interface_info[card_index].max_stm1_ports_supported;
				stm1_cp_limit.no_active_interface[acm_level][STM1_ADM_MA_PORT]		= sdh_stm1_hw_cfg.interface_info[card_index].max_stm1_ports_supported;
				stm1_cp_limit.no_active_interface[acm_level][STM1_VC_SP_PORT]		= sdh_stm1_hw_info->card_hw_info[card_index].max_vc_ports_supported;
			}
			break;

			default:
				break;
		}
	}
	return SUCCESS;
}

PAL_INT_32 cil_sdh_stm1_get_cp_system_info(stm1_cp_limit_S	*temp_stm1_cp_limit)
{
	memcpy(temp_stm1_cp_limit, &stm1_cp_limit, sizeof(stm1_cp_limit_S));

	return SUCCESS;
}

