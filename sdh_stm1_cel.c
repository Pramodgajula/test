/******************************************************************************
 * CEL Library
 *
 * Configuration External Library
 *
 * Copyright (C) CarrierComm Inc.
 *****************************************************************************/

/******************************************************************************
 * sdh_stm1_cel.c
 *
 * Configuration External Library for Sedona
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
#include "ces_common.h"
#include "ces_sockets.h"

/*module includes*/
#include "pal_lib.h"
#include "sdh_stm1_config_int_ext_lib.h"
#include "../core/include/sdh_stm1_enums.h"
#include "../core/include/sdh_stm1_pm_data_struct.h"
#include "sdh_stm1_hil.h"
#include "sdh_stm1_cel.h"
#include "sdh_stm1_dbl.h"

// #include "sqlite.h" TBD Item

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/

/* This variable value is maintained in sdh_module.c */
extern Sdh_Stm1_Auth_Feature_S	sdh_stm1_feature_auth[MAX_SDH_STM1_FEATURE_LIST];

/* STM1 External Configuration database */
Stm1_Config_S			ext_stm1_config_db;

/* STM1 Default Configuration database */
Stm1_Config_S			def_stm1_config_db;

Sdh_Stm1_Ext_Feature_S  ext_sdh_stm1_feature;

//Pramod: Unused Global Varaible
//extern Stm1_Sdh_Hw_Max_Interface_S		sdh_stm1_hw_cfg;

/******************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/

/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/

/******************************************************************************
 * Buffer Initialiation functions
 *****************************************************************************/
PAL_INT_32 cel_sdh_stm1_init_buffer()
{
	memset(&ext_stm1_config_db,0,sizeof(ext_stm1_config_db));
	memset(&def_stm1_config_db,0,sizeof(def_stm1_config_db));
	memset(&ext_sdh_stm1_feature,0,sizeof(ext_sdh_stm1_feature));

    return SUCCESS;
}

/******************************************************************************
 * Description   : This function is used to initialize the CEL external and
 *                                      default database configuration
 *
 * Inputs        : None
 * Output        : None
 * Return Value : STM1_SUCCESS on success/STM1_FAILURE on failure
 ******************************************************************************/
PAL_INT_32 cel_sdh_stm1_database_init()
{
	int ret_val = STM1_SUCCESS;

	//ext_stm1_config_db

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PORT_CONFIG, &ext_stm1_config_db.port_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Port Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_MUX_DEMUX_CONFIG,\
			&ext_stm1_config_db.core_config.md_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Mux-Demux Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_TRACE_MSG_CONFIG,\
			&ext_stm1_config_db.core_config.tm_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Trace Msg Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PROTECTION_SW_CONFIG,\
			&ext_stm1_config_db.chip_config.prot_parms);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Protection Switch Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_SYNC_CONFIG,\
			&ext_stm1_config_db.chip_config.stm1_sync);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Time Sync Config database \n", __func__);
	}


	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_CROSSPOINT_CONFIG,\
			&ext_stm1_config_db.chip_config.stm1_cp);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL EXT Crosspoint Config database \n", __func__);
	}

	// def_stm1_config_db

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PORT_CONFIG, &def_stm1_config_db.port_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Port Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_MUX_DEMUX_CONFIG,\
			&def_stm1_config_db.core_config.md_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Mux-Demux Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_TRACE_MSG_CONFIG,\
			&def_stm1_config_db.core_config.tm_config);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Trace Msg Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_PROTECTION_SW_CONFIG,\
			&def_stm1_config_db.chip_config.prot_parms);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Protection Switch Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_SYNC_CONFIG,\
			&def_stm1_config_db.chip_config.stm1_sync);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Time Sync Config database \n", __func__);
	}

	ret_val = dbl_sdh_stm1_init_config_buffer(STM1_CROSSPOINT_CONFIG,\
			&def_stm1_config_db.chip_config.stm1_cp);
	if(STM1_SUCCESS != ret_val)
	{
		printf(" %s: Failed to initialize the CEL DEFAULT Crosspoint Config database \n", __func__);
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
PAL_INT_32 cel_sdh_stm1_alloc_mem(Stm1_Sdh_Hw_Max_Interface_S block_info, PAL_INT_32 config)
{
	PAL_INT_32 retval = FAILURE;

	printf(" Entered %s \n", __FUNCTION__);
	switch(config)
	{
		case PORT_CONFIG:
			{
				printf(" Entered PORT_CONFIG case \n");

				/* Allocate memory for port configuration */
				if((retval = dbl_sdh_stm1_cfg_update(&ext_stm1_config_db.port_config, block_info, 1, STM1_PORT_CONFIG)) < 0)
				{
					return retval;
				}
				if((retval = dbl_sdh_stm1_cfg_update(&def_stm1_config_db.port_config, block_info, 1, STM1_PORT_CONFIG))< 0)
				{
					return retval;
				}

#if 0
				/* Allocate memory for port configuration */
				if((retval = sdh_stm1_port_cfg_alloc_mem(block_info,&ext_stm1_config_db.port_config))<0)
				{
					return retval;
				}
				if((retval = sdh_stm1_port_cfg_alloc_mem(block_info,&def_stm1_config_db.port_config))<0)
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
				if((retval = dbl_sdh_stm1_cfg_update(&ext_stm1_config_db.core_config.md_config, block_info, 1, STM1_PORT_CONFIG)) < 0)
				{
					return retval;
				}
				if((retval = dbl_sdh_stm1_cfg_update(&def_stm1_config_db.core_config.md_config, block_info, 1, STM1_PORT_CONFIG))< 0)
				{
					return retval;
				}
				/* Allocate memory for Trace Msg Configurations */
				if((retval = dbl_sdh_stm1_cfg_update(&ext_stm1_config_db.core_config.tm_config, block_info, 1, STM1_PORT_CONFIG)) < 0)
				{
					return retval;
				}
				if((retval = dbl_sdh_stm1_cfg_update(&def_stm1_config_db.core_config.tm_config, block_info, 1, STM1_PORT_CONFIG))< 0)
				{
					return retval;
				}

#if 0
				/* Allocate memory for Mux-Demux Configurations */
				if((retval = sdh_stm1_mux_demux_alloc_mem(block_info,&ext_stm1_config_db.core_config.md_config))<0)
				{
					return retval;
				}
				if((retval = sdh_stm1_mux_demux_alloc_mem(block_info,&def_stm1_config_db.core_config.md_config))<0)
				{
					return retval;
				}
				/* Allocate memory for trace messages */
				if((retval = sdh_stm1_trace_msg_alloc_mem(block_info,&ext_stm1_config_db.core_config.tm_config))<0)
				{
					return retval;
				}
				if((retval = sdh_stm1_trace_msg_alloc_mem(block_info,&def_stm1_config_db.core_config.tm_config))<0)
				{
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
} /* end of cel_sdh_stm1_alloc_mem() */

PAL_INT_32 cel_sdh_stm1_cleanup(PAL_VOID)
{
        // Release Port Configuration memory 
	dbl_sdh_stm1_release_config_buffer(STM1_PORT_CONFIG, &ext_stm1_config_db.port_config);
	dbl_sdh_stm1_release_config_buffer(STM1_PORT_CONFIG, &def_stm1_config_db.port_config);

        // Release Mux-Demux Configuration memory 
	dbl_sdh_stm1_release_config_buffer(STM1_MUX_DEMUX_CONFIG, &ext_stm1_config_db.core_config.md_config);
	dbl_sdh_stm1_release_config_buffer(STM1_MUX_DEMUX_CONFIG, &def_stm1_config_db.core_config.md_config);

        // Release Trace Msg Configuration memory 
	dbl_sdh_stm1_release_config_buffer(STM1_TRACE_MSG_CONFIG, &ext_stm1_config_db.core_config.tm_config);
	dbl_sdh_stm1_release_config_buffer(STM1_TRACE_MSG_CONFIG, &def_stm1_config_db.core_config.tm_config);

#if 0
	/* Clean the memory for STM1 External and default configuration database */
	sdh_stm1_port_cfg_free_mem(&ext_stm1_config_db.port_config);
	sdh_stm1_port_cfg_free_mem(&def_stm1_config_db.port_config);

	sdh_stm1_mux_demux_free_mem(&ext_stm1_config_db.core_config.md_config);
	sdh_stm1_mux_demux_free_mem(&def_stm1_config_db.core_config.md_config);

	sdh_stm1_trace_msg_free_mem(&ext_stm1_config_db.core_config.tm_config);
	sdh_stm1_trace_msg_free_mem(&def_stm1_config_db.core_config.tm_config);

	if(NULL != ext_sdh_stm1_feature.ext_stm_feature_info)
	{
		free(ext_sdh_stm1_feature.ext_stm_feature_info);
		ext_sdh_stm1_feature.ext_stm_feature_info = NULL;
	}
#endif 

}

/******************************************************************************
 * SDH STM1 Default configuration functions
 *****************************************************************************/
/*******************************************************************************
 *	Description:This function is used to configure all user configuration.
 *				On getting getting any Get or Set request from the UI I/F Task
 *				this function will be called.
 *
 *	Inputs:		LIU Type,
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/
PAL_INT_32 cel_sdh_stm1_default_config_update(PAL_VOID)
{
	printf(" Entered %s \n", __FUNCTION__);
	PAL_INT_32 retval = 0,index=0;

	/* Update the default STM1 Port configuration values for first core */
	cel_sdh_stm1_default_port_config(&def_stm1_config_db.port_config);

	/* Update the default mux-demux message values for first core */
	cel_sdh_stm1_default_mux_demux_config(&def_stm1_config_db.core_config.md_config);

	/* Update the default trace message values for first core */
	cel_sdh_stm1_default_trace_msg_data(&def_stm1_config_db.core_config.tm_config);

	/* Update the default timimng module (stm1 synchronization module) core */
	cel_sdh_stm1_default_sync_config(&def_stm1_config_db.chip_config.stm1_sync);

	/* Update the default values for protection switching configuration */
	cel_sdh_stm1_default_prot_config(&def_stm1_config_db.chip_config.prot_parms);

	/* Update the default values for protection lcr configuration */
	cel_sdh_stm1_default_prot_lcr_config(&def_stm1_config_db.chip_config.prot_parms.lcr_parms);

	/* Update the default values for STM1 Crosspoint configuration */
	cel_sdh_stm1_default_crosspoint_config(&def_stm1_config_db.chip_config.stm1_cp);
	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

int get_stm1_mux_demux_cfg(Stm1_Mux_Demux_Cfg_S *ptr_core_cfg)
{
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Stm1_Mux_Demux_Cfg_S def_mux_Cfg = {SDH_STM1_MUX_CFG_LIST};
	Sdh_Rs_Parms_S def_mux_rs_cfg = {SDH_STM1_MUX_RS_CFG_LIST};
	Sdh_Ms_Parms_S def_mux_ms_cfg = {SDH_STM1_MUX_MS_CFG_LIST};
	Sdh_Hp_Parms_S def_mux_hp_cfg = {SDH_STM1_MUX_HP_CFG_LIST};
	Sdh_Lp_Parms_S def_mux_lp_cfg = {SDH_STM1_MUX_LP_CFG_LIST};

#undef SDH_STM1_PARAM_INIT

	memcpy(ptr_core_cfg, &def_mux_Cfg,sizeof(def_mux_Cfg));

	memcpy(&ptr_core_cfg->rs_parms, &def_mux_rs_cfg,sizeof(def_mux_rs_cfg));
	memcpy(&ptr_core_cfg->ms_parms, &def_mux_ms_cfg,sizeof(def_mux_ms_cfg));
	memcpy(&ptr_core_cfg->hp_parms, &def_mux_hp_cfg,sizeof(def_mux_hp_cfg));
	memcpy(&ptr_core_cfg->lp_parms, &def_mux_lp_cfg,sizeof(def_mux_lp_cfg));
}

PAL_INT_32 cel_sdh_stm1_default_mux_demux_config(Stm1_Core_Mux_Demux_S *ptr_sdh_mux_demux_cfg)
{
	PAL_INT_32 index = 0;
	PAL_UINT_32 max_sdh_port_core = 0;

	ptr_sdh_mux_demux_cfg->version	= sdh_stm1_feature_auth[STM1_MUX_DEMUX_CONFIG].db_version;

	if((NULL == ptr_sdh_mux_demux_cfg->sdh_card_info.elems) || (NULL == ptr_sdh_mux_demux_cfg->ptr_core_cfg.elems))
	{
		printf(" %s: No Buffer to allocate \n", __FUNCTION__);
		return FAILURE;
	}

	for(index=0; index < ptr_sdh_mux_demux_cfg->num_cards;index++)
	{
		max_sdh_port_core += ptr_sdh_mux_demux_cfg->sdh_card_info.elems[index].max_stm_port_core;
	}

	get_stm1_mux_demux_cfg(&ptr_sdh_mux_demux_cfg->ptr_core_cfg.elems[0]);

	for (index=1;index < max_sdh_port_core ;index++)
	{
		/* Copy the same default values for other cores also */
		ptr_sdh_mux_demux_cfg->ptr_core_cfg.elems[index] = ptr_sdh_mux_demux_cfg->ptr_core_cfg.elems[0];
	}
	return SUCCESS;
}

PAL_INT_32 sdh_write_default_trace_msg_values(Stm1_Core_Trace_Msg_S *ptr_sdh_trace_data, PAL_UINT_32 adm_core_index)
{
	PAL_INT_32 index = 0;
	PAL_CHAR_8 def_string[MAX_DATA_WIDTH] = DEFAULT_TRACE_DATA;

	/* Check ADM Index validity */
	if((adm_core_index < 0) || (adm_core_index >= ptr_sdh_trace_data->sdh_card_info.elems[0].max_stm_port_core))
	{
		printf("%s: ADM Core index out of range\n", __FUNCTION__);
		return FAILURE;
	}

	/*initialise before writing*/
	memset((PAL_CHAR_8 *)&ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data,0,sizeof(Sdh_Tm_Data_S));

	/* Updated the Trace Message and CRC configuration */
	strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.tm_data,def_string);
	strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_data,def_string);
	ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.tm_crc = '1';
	ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tm_crc = '1';

	for(index=0 ; index<MAX_TU_DATA ; index++)
	{
		strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.tu_data[index],def_string);
		strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_data[index],def_string);
		ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.tu_crc[index] = '1';
		ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_tu_crc[index] = '1';
	}

	for(index=0;index<MAX_VC_DATA;index++)
	{
		strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.vc_data[index],def_string);
		strcpy(ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_data[index],def_string);
		ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.vc_crc[index] = '1';
		ptr_sdh_trace_data->ptr_trace_cfg.elems[adm_core_index].trace_data.cmp_vc_crc[index] = '1';
	}

	if((write_trace_messge_string_file(*ptr_sdh_trace_data, adm_core_index))<0)
	{
		printf("Unable to Write Default Values in database \n");
		return FAILURE;
	}

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_trace_msg_data(Stm1_Core_Trace_Msg_S *ptr_sdh_trace_data)
{
	PAL_INT_32 index = 0;
	PAL_UINT_32 max_sdh_port_core = 0;

	ptr_sdh_trace_data->version	= sdh_stm1_feature_auth[STM1_TRACE_MSG_CONFIG].db_version;

	if((NULL == ptr_sdh_trace_data->sdh_card_info.elems) || (NULL == ptr_sdh_trace_data->ptr_trace_cfg.elems))
	{
		printf(" %s: No Buffer to allocate \n", __FUNCTION__);
		return FAILURE;
	}

	for(index=0; index < ptr_sdh_trace_data->num_cards;index++)
	{
		max_sdh_port_core += ptr_sdh_trace_data->sdh_card_info.elems[index].max_stm_port_core;
	}

	/* Update the default trace message values for first core */
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].rs_parms.rs_tm_align_type	= RS_TM_ALIGN_ALIGN;
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].rs_parms.rs_tm_length		= RS_TM_LENGTH_16BYTES;
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].hp_parms.hp_tm_align_type	= HP_TM_ALIGN_ALIGN;
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].hp_parms.hp_tm_length		= HP_TM_LENGTH_16BYTES;
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].lp_parms.lp_tm_align_type	= LP_TM_ALIGN_NONE;
	ptr_sdh_trace_data->ptr_trace_cfg.elems[0].lp_parms.lp_tm_length		= LP_TM_LENGTH_16BYTES;
	sdh_write_default_trace_msg_values(ptr_sdh_trace_data, 0);

	for (index=1;index < max_sdh_port_core ;index++)
	{
		/* Copy the same default values for other cores also */
		ptr_sdh_trace_data->ptr_trace_cfg.elems[index] = ptr_sdh_trace_data->ptr_trace_cfg.elems[0];

	}
	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_sync_config(Stm1_Sync_Parms_S *ptr_sdh_sync_data)
{
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Stm1_Sync_Parms_S sdh_sync_data = {sdh_stm1_feature_auth[STM1_SYNC_CONFIG].db_version,
		SDH_STM1_TIME_SYNC_LIST};
#undef SDH_STM1_PARAM_INIT
	memcpy(ptr_sdh_sync_data,&sdh_sync_data,sizeof(Stm1_Sync_Parms_S));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_prot_sncp_config(Sdh_Sncp_Parms_S *ptr_sdh_prot_sncp_data)
{

#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Sdh_Sncp_Parms_S def_sncp_cfg = {SDH_STM1_SNCP_CFG_PARAM_LIST};
#undef SDH_STM1_PARAM_INIT

#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Sdh_sncp_couplet_S def_sncp_couplet = {SDH_STM1_SNCP_COUPLET_INST_LIST};
#undef SDH_STM1_PARAM_INIT

	memcpy(ptr_sdh_prot_sncp_data, &def_sncp_cfg,sizeof(Sdh_Sncp_Parms_S));
	memcpy(&ptr_sdh_prot_sncp_data->sncp_couplet, &def_sncp_couplet,sizeof(Sdh_sncp_couplet_S));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_prot_msp_config(Sdh_Msp_Parms_S *ptr_sdh_prot_msp_data)
{
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Sdh_Msp_Parms_S def_msp_cfg = {SDH_STM1_MSP_CFG_INST_LIST};
#undef SDH_STM1_PARAM_INIT
	memcpy(ptr_sdh_prot_msp_data, &def_msp_cfg, sizeof(def_msp_cfg));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_prot_config(Stm1_Prot_Parms_S *ptr_sdh_prot_data)
{
	PAL_INT_32 index = 0;

	ptr_sdh_prot_data->prot_algo					= SDH_STM1_DISABLE;
	ptr_sdh_prot_data->max_sncp_config				= 0;

	/* Update the default values for sncp configuration */
	cel_sdh_stm1_default_prot_sncp_config(&ptr_sdh_prot_data->sncp_parms.sncp_prot_data[0]);
	for (index=1; index < MAX_NO_STM_PROTECTION; index++)
	{
		ptr_sdh_prot_data->sncp_parms.sncp_prot_data[index] = \
								      ptr_sdh_prot_data->sncp_parms.sncp_prot_data[0];
	}

	/* Update the default values for msp configuration  */
	cel_sdh_stm1_default_prot_msp_config(&ptr_sdh_prot_data->msp_parms);

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_prot_lcr_config(Sdh_Prot_Sw_lcr_S *ptr_sdh_prot_lcr_data)
{
	memset(ptr_sdh_prot_lcr_data,0,sizeof(Sdh_Prot_Sw_lcr_S));
	/* YTD */
	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_default_crosspoint_config(Stm1_Cp_Config_S *ptr_sdh_cp_data)
{

	if(ptr_sdh_cp_data->chmap_data != NULL)
	{
		memset(ptr_sdh_cp_data->chmap_data,0,\
				ptr_sdh_cp_data->chmap_data_size);
	}
	ptr_sdh_cp_data->chmap_data_size = 0;

	if(ptr_sdh_cp_data->chmap_data != NULL)
	{
		free(ptr_sdh_cp_data->chmap_data);
		ptr_sdh_cp_data->chmap_data = NULL;
	}

	/* YTD */
	return SUCCESS;
}
/*******************************************************************************
 *	Description:This function is used to DEFAULT Port configuration when db version
 *				mismatches or when user requests.
 *
 *	Inputs:		pointer to the received buffer (UI Request)
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/
PAL_INT_32 cel_sdh_stm1_default_port_config(Stm1_Port_Config_S *ptr_port_cfg)
{
	PAL_INT_32 retval = SUCCESS,index=0;
	PAL_INT_32 max_sdh_port_core = 0;

	/* Structure to store the STM Ports configuration */
#define STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
	Stm1_Port_Params_S def_port_cfg = {STM1_PORT_CFG_INST_LIST};
#undef STM1_PARAM_INIT

	/* Update the database version for STM1 Port configuration */
	ptr_port_cfg->version = sdh_stm1_feature_auth[STM1_PORT_CONFIG].db_version;

	/* Compute the maximum STM1 port supported */
	for(index=0; index < ptr_port_cfg->num_cards;index++)
	{
		max_sdh_port_core += ptr_port_cfg->sdh_card_info.elems[index].max_stm_port_core;
	}

	memset(ptr_port_cfg->port_params.elems, 0, sizeof(Stm1_Port_Params_S)*max_sdh_port_core);

	/* Update the Default value for STM1 Port configuration */
	for (index=1;index < max_sdh_port_core;index++)
	{
		ptr_port_cfg->port_params.elems[index] = def_port_cfg;
	}
	return retval;
}

PAL_INT_32 cel_sdh_stm1_default_config(Sdh_Stm1_Features_E stm1_feature, Stm1_Config_S *ptr_stm1_cfg)
{
	PAL_INT_32 retval=0;
	printf(" Entered %s \n", __FUNCTION__);

	/* Switch case to update the default values to the local database */
	switch(stm1_feature)
	{
		case STM1_PORT_CONFIG:
			{
				retval = cel_sdh_stm1_default_port_config(&ptr_stm1_cfg->port_config);
				break;
			}
		case STM1_PROTECTION_SW_CONFIG:
			{
				retval = cel_sdh_stm1_default_prot_config(&ptr_stm1_cfg->chip_config.prot_parms);
				break;
			}
		case STM1_PROTECTION_SW_LCR_CONFIG:
			{
				retval = cel_sdh_stm1_default_prot_lcr_config(&ptr_stm1_cfg->chip_config.prot_parms.lcr_parms);
				break;
			}
		case STM1_CROSSPOINT_CONFIG:
			{
				retval = cel_sdh_stm1_default_crosspoint_config(&ptr_stm1_cfg->chip_config.stm1_cp);
				break;
			}
		case STM1_MUX_DEMUX_CONFIG:
			{
				retval = cel_sdh_stm1_default_mux_demux_config(&ptr_stm1_cfg->core_config.md_config);
				break;
			}
		case STM1_TRACE_MSG_CONFIG:
			{
				retval = cel_sdh_stm1_default_trace_msg_data(&ptr_stm1_cfg->core_config.tm_config);
				break;
			}
		case STM1_SYNC_CONFIG:
			{
				retval = cel_sdh_stm1_default_sync_config(&ptr_stm1_cfg->chip_config.stm1_sync);
				break;
			}

		default:
			break;
	}

	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

/******************************************************************************
 * SDH STM1 Database configuration copy functions
 *****************************************************************************/
/* STM1 Copy functions */
PAL_INT_32 sdh_stm1_copy_mux_demux(Stm1_Core_Mux_Demux_S *ptr_dst_port_cfg, \
		Stm1_Core_Mux_Demux_S *ptr_src_port_cfg)
{
	PAL_UINT_32 copied_dst_mux_cfg = 0, copied_src_mux_cfg = 0;
	PAL_UINT_32 dst_card_index = 0, src_card_index = 0;
	PAL_USHORT_16 max_cur_core_cfg = 0;
	/* Loop wrt the cards supported currently */
	for(dst_card_index = 0;dst_card_index < ptr_dst_port_cfg->num_cards;dst_card_index++)
	{
		copied_src_mux_cfg = 0;
		/* Loop wrt the cards supported previously */
		for(src_card_index = 0;src_card_index < ptr_src_port_cfg->num_cards;src_card_index++)
		{
			/* Check whether card type are same to copy */
			if(ptr_src_port_cfg->sdh_card_info.elems[src_card_index].card_type == \
					ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].card_type)
			{
				/* Defined the number of SDH Mux-demux  cfg to be copied */
				max_cur_core_cfg = ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
				if(ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core >
						ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core)
				{
					max_cur_core_cfg = ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
				}
				/* Copy the SDH Mux-demux configuration */
				memcpy(&ptr_dst_port_cfg->ptr_core_cfg.elems[copied_dst_mux_cfg],\
						&ptr_src_port_cfg->ptr_core_cfg.elems[copied_src_mux_cfg],\
						max_cur_core_cfg*sizeof(Stm1_Mux_Demux_Cfg_S));
			}
			/* Variable to maintain the SDH Mux-demux copied wrt cards */
			copied_src_mux_cfg += ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
		}
		/* Variable to maintain the SDH Mux-demux copied wrt cards */
		copied_dst_mux_cfg += ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
	}
}

PAL_INT_32 cel_sdh_stm1_copy_mux_demux(Stm1_Core_Mux_Demux_S *ptr_dst_mux_demux_cfg,\
		Stm1_Core_Mux_Demux_S *ptr_src_mux_demux_cfg, PAL_INT_32 sync)
{
	PAL_USHORT_16 max_core_mux_cfg = 0;
	PAL_INT_32 card_cnt = 0;

	if((ptr_dst_mux_demux_cfg->num_cards != ptr_src_mux_demux_cfg->num_cards) || \
			(memcmp(ptr_dst_mux_demux_cfg->sdh_card_info.elems, ptr_src_mux_demux_cfg->sdh_card_info.elems, \
				sizeof(Stm1_Card_Info_S)*ptr_dst_mux_demux_cfg->num_cards) != 0))
	{
		if(sync)
		{
			if(ptr_dst_mux_demux_cfg->sdh_card_info.elems != NULL)
			{
				free(ptr_dst_mux_demux_cfg->sdh_card_info.elems);
				ptr_dst_mux_demux_cfg->sdh_card_info.elems = NULL;
			}
			if(ptr_dst_mux_demux_cfg->ptr_core_cfg.elems != NULL)
			{
				free(&ptr_dst_mux_demux_cfg->ptr_core_cfg.elems);
				ptr_dst_mux_demux_cfg->ptr_core_cfg.elems = NULL;
			}

			ptr_dst_mux_demux_cfg->num_cards = ptr_src_mux_demux_cfg->num_cards;
			ptr_dst_mux_demux_cfg->sdh_card_info.elems = malloc(ptr_src_mux_demux_cfg->num_cards * \
					sizeof(Stm1_Card_Info_S));

			for(card_cnt = 0;card_cnt < ptr_dst_mux_demux_cfg->num_cards; card_cnt++)
			{
				max_core_mux_cfg += ptr_dst_mux_demux_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
			}

			ptr_dst_mux_demux_cfg->ptr_core_cfg.elems = malloc(sizeof(Stm1_Mux_Demux_Cfg_S)*max_core_mux_cfg);

			/* Copy the required card information from external database */
			memcpy(ptr_dst_mux_demux_cfg->sdh_card_info.elems, ptr_src_mux_demux_cfg->sdh_card_info.elems, sizeof(Stm1_Card_Info_S)*ptr_src_mux_demux_cfg->num_cards);

			/* Copy the required value from external database */
			memcpy(ptr_dst_mux_demux_cfg->ptr_core_cfg.elems ,ptr_src_mux_demux_cfg->ptr_core_cfg.elems,\
					sizeof(Stm1_Mux_Demux_Cfg_S)*max_core_mux_cfg);
		}
		else
		{
			sdh_stm1_copy_mux_demux(ptr_dst_mux_demux_cfg, ptr_src_mux_demux_cfg);
		}
	}
	else
	{
		for(card_cnt = 0;card_cnt < ptr_dst_mux_demux_cfg->num_cards; card_cnt++)
		{
			max_core_mux_cfg += ptr_dst_mux_demux_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
		}

		/* Copy the required value from external database */
		memcpy(ptr_dst_mux_demux_cfg->ptr_core_cfg.elems ,ptr_src_mux_demux_cfg->ptr_core_cfg.elems,\
				sizeof(Stm1_Mux_Demux_Cfg_S)*max_core_mux_cfg);
	}

	return SUCCESS;
}

PAL_INT_32 sdh_stm1_copy_trace_cfg(Stm1_Core_Trace_Msg_S *ptr_dst_trace_msg_cfg, \
		Stm1_Core_Trace_Msg_S *ptr_src_trace_msg_cfg)
{
	PAL_UINT_32 copied_dst_trace_cfg = 0, copied_src_trace_cfg = 0;
	PAL_UINT_32 dst_card_index = 0, src_card_index = 0;
	PAL_USHORT_16 max_cur_trace_cfg = 0;
	/* Loop wrt the cards supported currently */
	for(dst_card_index = 0;dst_card_index < ptr_dst_trace_msg_cfg->num_cards;dst_card_index++)
	{
		copied_src_trace_cfg = 0;
		/* Loop wrt the cards supported previously */
		for(src_card_index = 0;src_card_index < ptr_src_trace_msg_cfg->num_cards;src_card_index++)
		{
			/* Check whether card type are same to copy */
			if(ptr_src_trace_msg_cfg->sdh_card_info.elems[src_card_index].card_type == \
					ptr_dst_trace_msg_cfg->sdh_card_info.elems[dst_card_index].card_type)
			{
				/* Defined the number of SDH Trace cfg to be copied */
				max_cur_trace_cfg = ptr_src_trace_msg_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
				if(ptr_src_trace_msg_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core >
						ptr_dst_trace_msg_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core)
				{
					max_cur_trace_cfg = ptr_dst_trace_msg_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
				}
				/* Copy the SDH Trace configuration */
				memcpy(&ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems[copied_dst_trace_cfg],\
						&ptr_src_trace_msg_cfg->ptr_trace_cfg.elems[copied_src_trace_cfg],\
						max_cur_trace_cfg*sizeof(Stm1_Trace_Msg_Cfg_S));
			}
			/* Variable to maintain the SDH Trace ports copied wrt cards */
			copied_src_trace_cfg += ptr_src_trace_msg_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
		}
		/* Variable to maintain the SDH Trace copied wrt cards */
		copied_dst_trace_cfg += ptr_dst_trace_msg_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
	}
}

PAL_INT_32 cel_sdh_stm1_copy_trace_msg_data(Stm1_Core_Trace_Msg_S *ptr_dst_trace_msg_cfg,\
		Stm1_Core_Trace_Msg_S *ptr_src_trace_msg_cfg, PAL_INT_32 sync)
{
	PAL_USHORT_16 max_core_trace_cfg = 0;
	PAL_INT_32 card_cnt = 0;

	if((ptr_dst_trace_msg_cfg->num_cards != ptr_src_trace_msg_cfg->num_cards) || \
			(memcmp(ptr_dst_trace_msg_cfg->sdh_card_info.elems, ptr_src_trace_msg_cfg->sdh_card_info.elems, \
				sizeof(Stm1_Card_Info_S)*ptr_dst_trace_msg_cfg->num_cards) != 0))
	{
		if(sync)
		{
			if(ptr_dst_trace_msg_cfg->sdh_card_info.elems!= NULL)
			{
				free(ptr_dst_trace_msg_cfg->sdh_card_info.elems);
				ptr_dst_trace_msg_cfg->sdh_card_info.elems = NULL;
			}
			if(ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems != NULL)
			{
				free(ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems);
				ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems = NULL;
			}

			ptr_dst_trace_msg_cfg->num_cards = ptr_src_trace_msg_cfg->num_cards;
			ptr_dst_trace_msg_cfg->sdh_card_info.elems = malloc(ptr_src_trace_msg_cfg->num_cards * \
					sizeof(Stm1_Card_Info_S));

			for(card_cnt = 0;card_cnt < ptr_dst_trace_msg_cfg->num_cards; card_cnt++)
			{
				max_core_trace_cfg += ptr_dst_trace_msg_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
			}

			ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems = malloc(max_core_trace_cfg * sizeof(Stm1_Trace_Msg_Cfg_S));

			/* Copy the required card information from external database */
			memcpy(ptr_dst_trace_msg_cfg->sdh_card_info.elems, ptr_src_trace_msg_cfg->sdh_card_info.elems, sizeof(Stm1_Card_Info_S)*ptr_src_trace_msg_cfg->num_cards);

			/* Copy the required value from external database */
			memcpy(ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems, ptr_src_trace_msg_cfg->ptr_trace_cfg.elems,\
					sizeof(Stm1_Trace_Msg_Cfg_S)*max_core_trace_cfg);
		}
		else
		{
			sdh_stm1_copy_trace_cfg(ptr_dst_trace_msg_cfg, ptr_src_trace_msg_cfg);
		}
	}
	else
	{
		for(card_cnt = 0;card_cnt < ptr_dst_trace_msg_cfg->num_cards; card_cnt++)
		{
			max_core_trace_cfg += ptr_dst_trace_msg_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
		}

		/* Copy the required value from external database */
		memcpy(ptr_dst_trace_msg_cfg->ptr_trace_cfg.elems, ptr_src_trace_msg_cfg->ptr_trace_cfg.elems,\
				sizeof(Stm1_Trace_Msg_Cfg_S)*max_core_trace_cfg);
	}

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_copy_protection_cfg_data(Stm1_Prot_Parms_S *ptr_dst_stm1_prot_cfg,\
		Stm1_Prot_Parms_S *ptr_src_stm1_prot_cfg)
{
	/* Copy the required value from external database */
	memcpy(ptr_dst_stm1_prot_cfg,ptr_src_stm1_prot_cfg,sizeof(Stm1_Prot_Parms_S));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_copy_protection_lcr_cfg_data(Sdh_Prot_Sw_lcr_S *ptr_dst_stm1_prot_lcr_cfg,\
		Sdh_Prot_Sw_lcr_S *ptr_src_stm1_prot_lcr_cfg)
{
	/* Copy the required value from external database */
	memcpy(ptr_dst_stm1_prot_lcr_cfg,ptr_src_stm1_prot_lcr_cfg,sizeof(Sdh_Prot_Sw_lcr_S));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_copy_time_sync_cfg_data(Stm1_Sync_Parms_S *ptr_dst_stm1_time_sync_cfg,\
		Stm1_Sync_Parms_S *ptr_src_stm1_time_sync_cfg)
{
	/* Copy the required value from external database */
	memcpy(ptr_dst_stm1_time_sync_cfg,ptr_src_stm1_time_sync_cfg,sizeof(Stm1_Sync_Parms_S));

	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_copy_crosspoint_cfg_data(Stm1_Cp_Config_S *ptr_dst_stm1_cp_cfg,\
		Stm1_Cp_Config_S *ptr_src_stm1_cp_cfg)
{
	PAL_INT_32 chmap_data_size = 0;

	ptr_dst_stm1_cp_cfg->channelmap_type 		= ptr_src_stm1_cp_cfg->channelmap_type;
	ptr_dst_stm1_cp_cfg->num_of_stm1_mapping 	= ptr_src_stm1_cp_cfg->num_of_stm1_mapping;
	ptr_dst_stm1_cp_cfg->chmap_data_size 		= ptr_src_stm1_cp_cfg->chmap_data_size;
	chmap_data_size 							= ptr_dst_stm1_cp_cfg->chmap_data_size;

	if(NULL != ptr_dst_stm1_cp_cfg->chmap_data)
		free(ptr_dst_stm1_cp_cfg->chmap_data);

	ptr_dst_stm1_cp_cfg->chmap_data 			= malloc(chmap_data_size);

	memcpy(ptr_dst_stm1_cp_cfg->chmap_data, ptr_src_stm1_cp_cfg->chmap_data, chmap_data_size);

	return SUCCESS;
}

/******************************************************************************************************
 *	Description:This function is used to get the system startup time, if its COLD or WARM
 *
 *	Inputs:		Buffer to store the value
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32	cel_sdh_stm1_read_user_config(Sdh_Stm1_Features_E sdh_feature,Stm1_Config_S *sdh_stm1_db,\
		Sdh_Stm1_Auth_Feature_S feature_info)
{
	PAL_INT_32	retval=0;

	/*********** Global Default Values ************/

	printf(" Entered %s \n", __FUNCTION__);

	/* Switch case to read the user configuration from external database */
	switch(sdh_feature)
	{
		case STM1_PORT_CONFIG:
			{
				Stm1_Port_Config_S	ext_port_data;

				memset(&ext_port_data, 0, sizeof(ext_port_data));

				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_port_data);

				/* Function to read the port configuration data from external database */
				/* Get the data from SQLite Database : TBD Item*/
				//retval = sdh_stm1_port_config_dB(&ext_port_data,STM1_GET,1);
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_port_data, STM1_GET);
 
				if((FAILURE == retval) || (ext_port_data.version != feature_info.db_version))
				{
					/* Read from External database is failue, so default the config */
					cel_sdh_stm1_default_port_config(&sdh_stm1_db->port_config);
				}
				else
				{
					/* Copy the Port configuartion to the Local database */
					cel_sdh_stm1_copy_port_config(&sdh_stm1_db->port_config, &ext_port_data,0);
				}
				//free(ext_port_data.sdh_card_info);
				//free(ext_port_data.port_config);
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_port_data);
			}
			break;

		case STM1_MUX_DEMUX_CONFIG:
			{
				Stm1_Core_Mux_Demux_S	ext_md_config;

				memset(&ext_md_config,0,sizeof(ext_md_config));

				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_md_config);

				/* Read the SDH Mux-Demux configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_md_config, STM1_GET);
				/* Check whether the versions are matching and dB read is Ok */
				if((FAILURE == retval) || (ext_md_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_mux_demux_config(&sdh_stm1_db->core_config.md_config);
				}
				else
				{
					cel_sdh_stm1_copy_mux_demux(&sdh_stm1_db->core_config.md_config,\
							&ext_md_config,1);
				}
				//free(ext_md_config.sdh_card_info);
				//free(ext_md_config.ptr_core_cfg);
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_md_config);

				break;
			}

		case STM1_TRACE_MSG_CONFIG:
			{
				Stm1_Core_Trace_Msg_S	ext_tm_config;

				memset(&ext_tm_config,0,sizeof(ext_tm_config));

				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_tm_config);

				/* Read the SDH Trace Message configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_tm_config, STM1_GET);
				/* Check whether the versions are matching */
				if((FAILURE == retval) || (ext_tm_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_trace_msg_data(&sdh_stm1_db->core_config.tm_config);
				}
				else
				{
					cel_sdh_stm1_copy_trace_msg_data(&sdh_stm1_db->core_config.tm_config,\
							&ext_tm_config,1);
				}
				//free(ext_tm_config.sdh_card_info);
				//free(ext_tm_config.ptr_trace_cfg);
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_tm_config);
				break;
			}
		case STM1_PROTECTION_SW_CONFIG:
			{
				Stm1_Prot_Parms_S		ext_prot_sw_config;

				memset(&ext_prot_sw_config,0,sizeof(ext_prot_sw_config));
				
				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_prot_sw_config);

				/* Read the STM1 Protection configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_prot_sw_config, STM1_GET);

				/* Check whether the versions are matching */
				if((FAILURE == retval) || (ext_prot_sw_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_prot_config(&sdh_stm1_db->chip_config.prot_parms);
				}
				else
				{
					cel_sdh_stm1_copy_protection_cfg_data(&sdh_stm1_db->chip_config.prot_parms,\
							&ext_prot_sw_config);
				}
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_prot_sw_config);
			}
			break;
		case STM1_PROTECTION_SW_LCR_CONFIG:
			{
				Sdh_Prot_Sw_lcr_S		ext_prot_lcr_config;

				memset(&ext_prot_lcr_config,0,sizeof(Sdh_Prot_Sw_lcr_S));
				
				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_prot_lcr_config);

				/* Read the STM1 Protection LCR configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_prot_lcr_config, STM1_GET);

				/* Check whether the versions are matching */
				if((FAILURE == retval) || (ext_prot_lcr_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_prot_lcr_config(&sdh_stm1_db->chip_config.prot_parms.lcr_parms);
				}
				else
				{
					cel_sdh_stm1_copy_protection_lcr_cfg_data(&sdh_stm1_db->chip_config.prot_parms.lcr_parms,\
							&ext_prot_lcr_config);
				}
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_prot_lcr_config);
			}
			break;
		case STM1_SYNC_CONFIG:
			{
				Stm1_Sync_Parms_S		ext_time_sync_config;

				memset(&ext_time_sync_config,0,sizeof(ext_time_sync_config));
				
				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_time_sync_config);

				/* Read the STM1 Time Sync configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_time_sync_config, STM1_GET);

				/* Check whether the versions are matching */
				if((FAILURE == retval) || (ext_time_sync_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_sync_config(&sdh_stm1_db->chip_config.stm1_sync);
				}
				else
				{
					cel_sdh_stm1_copy_time_sync_cfg_data(&sdh_stm1_db->chip_config.stm1_sync,\
							&ext_time_sync_config);
				}
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_time_sync_config);
			}
			break;

		case STM1_CROSSPOINT_CONFIG:
			{
				Stm1_Cp_Config_S		ext_cp_config;

				memset(&ext_cp_config,0,sizeof(ext_cp_config));

				dbl_sdh_stm1_init_config_buffer(sdh_feature, &ext_cp_config);

				/* Read the STM1 Crosspoint configuration data from External database */
				retval = sdh_stm1_configuration_dB(sdh_feature, &ext_cp_config, STM1_GET);

				/* Check whether the versions are matching */
				if((FAILURE == retval) || (ext_cp_config.version != feature_info.db_version))
				{
					/* Default the configuration, if version check fails */
					cel_sdh_stm1_default_crosspoint_config(&sdh_stm1_db->chip_config.stm1_cp);
				}
				else
				{
					cel_sdh_stm1_copy_crosspoint_cfg_data(&sdh_stm1_db->chip_config.stm1_cp,\
							&ext_cp_config);
				}
                                dbl_sdh_stm1_release_config_buffer(sdh_feature, &ext_cp_config);

			}
			break;

		default:
			break;
	}
	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

PAL_INT_32 cel_sdh_stm1_read_external_cfg(Sdh_Stm1_Features_E sdh_feature, PAL_VOID *ptr_config_data)
{
	PAL_INT_32 ret_val = SUCCESS;

	/* Switch case to read the user configuration from external database */
	switch(sdh_feature)
	{
		case STM1_PORT_CONFIG:
			{
				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.port_config, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}
/*
				cel_sdh_stm1_copy_port_config((Stm1_Port_Config_S *)ptr_config_data,\
						&ext_stm1_config_db.port_config,1);
*/
			}
			break;

		case STM1_MUX_DEMUX_CONFIG:
			{
				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.core_config.md_config, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}
/*
				cel_sdh_stm1_copy_mux_demux((Stm1_Core_Mux_Demux_S *)ptr_config_data,\
						&ext_stm1_config_db.core_config.md_config,1);\
*/
			}
			break;

		case STM1_TRACE_MSG_CONFIG:
			{

				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.core_config.tm_config, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}
/*
				cel_sdh_stm1_copy_trace_msg_data((Stm1_Core_Trace_Msg_S *)ptr_config_data,\
						&ext_stm1_config_db.core_config.tm_config,1);
*/
			}
			break;

		case STM1_PROTECTION_SW_CONFIG:
			{

				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.chip_config.prot_parms, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}

/*
				cel_sdh_stm1_copy_protection_cfg_data((Stm1_Prot_Parms_S *)ptr_config_data,\
						&ext_stm1_config_db.chip_config.prot_parms);
*/
			}
			break;

		case STM1_PROTECTION_SW_LCR_CONFIG:
			{

				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.chip_config.prot_parms.lcr_parms, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}

/*
				cel_sdh_stm1_copy_protection_lcr_cfg_data((Sdh_Prot_Sw_lcr_S *)ptr_config_data,\
						&ext_stm1_config_db.chip_config.prot_parms.lcr_parms);
*/
			}
			break;

		case STM1_SYNC_CONFIG:
			{

				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.chip_config.stm1_sync, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}

/*
				cel_sdh_stm1_copy_time_sync_cfg_data((Stm1_Sync_Parms_S *)ptr_config_data,\
						&ext_stm1_config_db.chip_config.stm1_sync);
*/
			}
			break;

		case STM1_CROSSPOINT_CONFIG:
			{

				if( (ret_val = dbl_sdh_stm1_copy_cfg(ptr_config_data, &ext_stm1_config_db.chip_config.stm1_cp, sdh_feature)) < 0)
				{
					ret_val = FAILURE;
				}

/*
				cel_sdh_stm1_copy_crosspoint_cfg_data((Stm1_Cp_Config_S *)ptr_config_data,\
						&ext_stm1_config_db.chip_config.stm1_cp);
*/
			}
			break;

		default:
			{
				printf(" Entering the default case \n");
			}
			break;
	} /* end of the switch(sdh_feature) */
	return ret_val;
}

/* Function to read/write to the external database */
PAL_INT_32 cel_sdh_stm1_user_configure(Sdh_Stm1_Features_E sdh_feature, \
		Sdh_Stm1_Ext_Db_Op_E stm1_dB_op, PAL_VOID *ptr_cfg_data)
{
	PAL_INT_32 ret_val = 0;
	switch(sdh_feature)
	{
		case STM1_PORT_CONFIG:
			{
				cel_sdh_stm1_copy_port_config(&ext_stm1_config_db.port_config,\
						(Stm1_Port_Config_S *)ptr_cfg_data,1);
				/* If the last argument is 0 means not to allocate memory for
				 * the configuration to store during read operation
				 */
				ret_val = sdh_stm1_port_config_dB(\
						(Stm1_Port_Config_S *)ptr_cfg_data,stm1_dB_op,0);
			}
			break;
		case STM1_MUX_DEMUX_CONFIG:
			{
				cel_sdh_stm1_copy_mux_demux(&ext_stm1_config_db.core_config.md_config,\
						(Stm1_Core_Mux_Demux_S *)ptr_cfg_data,1);
				ret_val = sdh_stm1_mux_demux_dB(\
						(Stm1_Core_Mux_Demux_S *)ptr_cfg_data,stm1_dB_op,0);
			}
			break;
		case STM1_TRACE_MSG_CONFIG:
			{
				cel_sdh_stm1_copy_trace_msg_data(&ext_stm1_config_db.core_config.tm_config,\
						(Stm1_Core_Trace_Msg_S *)ptr_cfg_data,1);
				ret_val = sdh_stm1_trace_msg_dB(\
						(Stm1_Core_Trace_Msg_S *)ptr_cfg_data,stm1_dB_op,0);
			}
			break;
		case STM1_PROTECTION_SW_CONFIG:
			{
				cel_sdh_stm1_copy_protection_cfg_data(&ext_stm1_config_db.chip_config.prot_parms,\
						(Stm1_Prot_Parms_S *)ptr_cfg_data);
				ret_val = sdh_stm1_protection_cfg_dB(\
						(Stm1_Prot_Parms_S *)ptr_cfg_data,stm1_dB_op);
			}
			break;
		case STM1_PROTECTION_SW_LCR_CONFIG:
			{
				cel_sdh_stm1_copy_protection_lcr_cfg_data(&ext_stm1_config_db.chip_config.prot_parms.lcr_parms,\
						(Sdh_Prot_Sw_lcr_S *)ptr_cfg_data);
				ret_val = sdh_stm1_protection_lcr_cfg_dB(\
						(Sdh_Prot_Sw_lcr_S *)ptr_cfg_data,stm1_dB_op);
			}
			break;
		case STM1_SYNC_CONFIG:
			{
				cel_sdh_stm1_copy_time_sync_cfg_data(&ext_stm1_config_db.chip_config.stm1_sync,\
						(Stm1_Sync_Parms_S *)ptr_cfg_data);
				ret_val = sdh_stm1_time_sync_cfg_dB(\
						(Stm1_Sync_Parms_S *)ptr_cfg_data,stm1_dB_op);
			}
			break;
		default:
			{
			}
			break;
	}
	return ret_val;
}

PAL_INT_32 sdh_stm1_copy_port_configuration(Stm1_Port_Config_S *ptr_dst_port_cfg, \
		Stm1_Port_Config_S *ptr_src_port_cfg)
{
	PAL_UINT_32 copied_dst_port_cfg = 0, copied_src_port_cfg = 0;
	PAL_UINT_32 dst_card_index = 0, src_card_index = 0;
	PAL_USHORT_16 max_cur_port_cfg = 0;

	if((NULL == ptr_src_port_cfg->port_params.elems) || (NULL == ptr_dst_port_cfg->port_params.elems))
		return FAILURE;

	/* Loop wrt the cards supported currently */
	for(dst_card_index = 0;dst_card_index < ptr_dst_port_cfg->num_cards;dst_card_index++)
	{
		copied_src_port_cfg = 0;
		/* Loop wrt the cards supported previously */
		for(src_card_index = 0;src_card_index < ptr_src_port_cfg->num_cards;src_card_index++)
		{
			/* Check whether card type are same to copy */
			if(ptr_src_port_cfg->sdh_card_info.elems[src_card_index].card_type == \
					ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].card_type)
			{
				/* Defined the number of STM1 Port cfg to be copied */
				max_cur_port_cfg = ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
				if(ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core >
						ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core)
				{
					max_cur_port_cfg = ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
				}
				/* Copy the STM1 Port configuration */
				memcpy(&ptr_dst_port_cfg->port_params.elems[copied_dst_port_cfg],\
						&ptr_src_port_cfg->port_params.elems[copied_src_port_cfg],\
						max_cur_port_cfg*sizeof(Stm1_Port_Params_S));
			}
			/* Variable to maintain the STM1 ports copied wrt cards */
			copied_src_port_cfg += ptr_src_port_cfg->sdh_card_info.elems[src_card_index].max_stm_port_core;
		}
		/* Variable to maintain the STM1 ports copied wrt cards */
		copied_dst_port_cfg += ptr_dst_port_cfg->sdh_card_info.elems[dst_card_index].max_stm_port_core;
	}
}

PAL_INT_32 cel_sdh_stm1_copy_port_config(Stm1_Port_Config_S *ptr_dst_port_cfg,\
		Stm1_Port_Config_S *ptr_src_port_cfg, PAL_INT_32 sync)
{
	PAL_USHORT_16 max_port_cfg = 0;
	PAL_INT_32 card_cnt = 0;
	if((ptr_dst_port_cfg->num_cards != ptr_src_port_cfg->num_cards) || \
			(NULL == ptr_dst_port_cfg->sdh_card_info.elems) || (NULL == ptr_src_port_cfg->sdh_card_info.elems) ||
			(memcmp(ptr_dst_port_cfg->sdh_card_info.elems, ptr_src_port_cfg->sdh_card_info.elems, \
				sizeof(Stm1_Card_Info_S)*ptr_dst_port_cfg->num_cards) != 0))
	{
		if(sync)
		{
			if(ptr_dst_port_cfg->sdh_card_info.elems != NULL)
			{
				free(ptr_dst_port_cfg->sdh_card_info.elems);
				ptr_dst_port_cfg->sdh_card_info.elems = NULL;
			}
			if(ptr_dst_port_cfg->port_params.elems != NULL)
			{
				free(ptr_dst_port_cfg->port_params.elems);
				ptr_dst_port_cfg->port_params.elems = NULL;
			}

			if((0 == ptr_src_port_cfg->num_cards) || \
					(NULL == ptr_src_port_cfg->sdh_card_info.elems))
			{
				printf("%s: No card info for copy operation \n", __FUNCTION__);
				return FAILURE;
			}

			ptr_dst_port_cfg->num_cards = ptr_src_port_cfg->num_cards;
			ptr_dst_port_cfg->sdh_card_info.elems = malloc(ptr_src_port_cfg->num_cards * \
					sizeof(Stm1_Card_Info_S));
			/* No card information */
			if(NULL == ptr_dst_port_cfg->sdh_card_info.elems)
			{
				printf("%s: Malloc failure for sdh_card_info \n", __FUNCTION__);
				return FAILURE;
			}

			memcpy(ptr_dst_port_cfg->sdh_card_info.elems, ptr_src_port_cfg->sdh_card_info.elems,\
					ptr_src_port_cfg->num_cards * \
					sizeof(Stm1_Card_Info_S));

			for(card_cnt = 0;card_cnt < ptr_dst_port_cfg->num_cards; card_cnt++)
			{
				max_port_cfg += ptr_dst_port_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
			}

			ptr_dst_port_cfg->port_params.elems = malloc(max_port_cfg * \
					sizeof(Stm1_Port_Config_S));

			if(NULL == ptr_dst_port_cfg->port_params.elems)
			{
				printf("%s: Malloc failure for port_config \n", __FUNCTION__);
				return FAILURE;
			}

			/* Copy the required value from external database */
			memcpy(ptr_dst_port_cfg->port_params.elems, ptr_src_port_cfg->port_params.elems,\
					sizeof(Stm1_Port_Config_S)*max_port_cfg);
		}
		else
		{
			sdh_stm1_copy_port_configuration(ptr_dst_port_cfg, ptr_src_port_cfg);
		}
	}
	else
	{
		for(card_cnt = 0;card_cnt < ptr_dst_port_cfg->num_cards; card_cnt++)
		{
			max_port_cfg += ptr_dst_port_cfg->sdh_card_info.elems[card_cnt].max_stm_port_core;
		}

		/* Copy the required value from external database */
		memcpy(ptr_dst_port_cfg->port_params.elems ,ptr_src_port_cfg->port_params.elems,\
				sizeof(Stm1_Port_Config_S)*max_port_cfg);
	}

	return SUCCESS;
}

/*******************************************************************************
 *	Description:This function is used to check db version and authorisation of the
 *				external db and local db version.If it matches system configure it
 *  			in hardware otherwise it'll default the configuration and copy the
 *				default configuration in external db.
 *	Inputs:
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/

PAL_INT_32 cel_sdh_stm1_check_db_version(Sdh_Stm1_Auth_Feature_S *ptr_auth_info, PAL_INT_32 *status)
{
	printf(" Entered %s \n", __FUNCTION__);
	PAL_INT_32	retval=0,feature_index=0;

	/* Read db_version info from the external database */
	cel_sdh_stm1_db_version_get(&ext_sdh_stm1_feature);

	for (feature_index = STM1_PORT_CONFIG; feature_index < MAX_SDH_STM1_FEATURE_LIST; feature_index++)
	{
		status[feature_index] = SUCCESS;

		/* To check following conditions
		 * 1) version mismatch wrt local and external
		 * 2) default of the external database has less feature supported
		 * 3) If the feature is disabled by user
		 */
		printf(" feature_index = %d \n", feature_index);
		printf(" ext_sdh_stm1_feature.ext_stm_features = %d \n", ext_sdh_stm1_feature.ext_stm_features);

		if( ((feature_index < ext_sdh_stm1_feature.ext_stm_features) &&
					(ext_sdh_stm1_feature.ext_stm_feature_info[feature_index].db_version != \
					 ptr_auth_info[feature_index].db_version)) ||
				(feature_index > ext_sdh_stm1_feature.ext_stm_features) ||
				(DISABLE == ptr_auth_info[feature_index].status) )
		{
			printf(" %s: Verify %d configuration \n",__FUNCTION__,feature_index);
			cel_sdh_stm1_default_config(feature_index, &ext_stm1_config_db);
			status[feature_index] = FAILURE;
		} /* end of version is mismatched */
		else
		{
			/* To update the user configurations to the local database */
			status[feature_index] = cel_sdh_stm1_read_user_config(feature_index, \
					&ext_stm1_config_db,ptr_auth_info[feature_index]);
		} /* end of if version is matched */

	} /* end of for loop with respect to max. no. of E1/T1 features supported */

	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

PAL_INT_32 cel_sdh_stm1_read_mux_demux_cfg_pm(Sdh_Stm1_Pm_buffer_S *ptr_sdh_pm_buffer)
{
	PAL_USHORT_16 max_core_cfg = 0;
	PAL_INT_32 card_cnt = 0, cfg_cnt = 0;
	Stm1_Mux_Demux_Cfg_S mux_cfg;

	/* Verify the buffer size are same wrt number of cards and the ports supported in it */
	if((ptr_sdh_pm_buffer->num_cards != ext_stm1_config_db.core_config.md_config.num_cards) ||
			(ext_stm1_config_db.core_config.md_config.sdh_card_info.elems == NULL) ||
			(ptr_sdh_pm_buffer->ptr_sdh_card_info == NULL) ||
			(memcmp(ext_stm1_config_db.core_config.md_config.sdh_card_info.elems,\
				ptr_sdh_pm_buffer->ptr_sdh_card_info, sizeof(Stm1_Card_Info_S)*ptr_sdh_pm_buffer->num_cards) != 0))

	{
		return FAILURE;
	}

	for(card_cnt = 0;card_cnt < ptr_sdh_pm_buffer->num_cards; card_cnt++)
	{
		max_core_cfg += ptr_sdh_pm_buffer->ptr_sdh_card_info[card_cnt].max_stm_port_core;
	}

	memset(&mux_cfg, 0, sizeof(mux_cfg));

	for(cfg_cnt = 0;cfg_cnt < max_core_cfg; cfg_cnt++)
	{
		mux_cfg = ext_stm1_config_db.core_config.md_config.ptr_core_cfg.elems[cfg_cnt];
		ptr_sdh_pm_buffer->ptr_sdh_pm_data[cfg_cnt].mux_cfg_data.op_mode = \
										   mux_cfg.op_mode;
		ptr_sdh_pm_buffer->ptr_sdh_pm_data[cfg_cnt].mux_cfg_data.ses_thresh_val = \
											  mux_cfg.ses_thresh_val;
		ptr_sdh_pm_buffer->ptr_sdh_pm_data[cfg_cnt].mux_cfg_data.status =
			mux_cfg.status;
	}
	return SUCCESS;
}

/******************************************************************************
 * SDH STM1 Database functions
 *****************************************************************************/
/*******************************************************************************
 *	Description:This function is used to Set or Get the Port Configuration to external DB.
 *				If its a Get request this function will fillup the received
 *				buffer with latest external port configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/
PAL_INT_32 cel_sdh_stm1_port_config( Stm1_Port_Config_S *port_data, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	Stm1_Port_Config_S	ext_port_config;

	printf(" Entered %s \n", __FUNCTION__);

	if( UI_GET == operation)
	{
		retval = sdh_stm1_port_config_dB(port_data,STM1_GET,0);
		if(FAILURE == retval)
		{
			printf(" Function sdh_stm1_port_config_dB() returned FAILURE \n");
		}
	}
	else if( UI_SET == operation)
	{
		retval = sdh_stm1_port_config_dB(port_data,STM1_SET,0);
		if(FAILURE == retval)
		{
			printf(" Function sdh_stm1_port_config_dB() returned FAILURE ");
		}
	}
	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to Set or Get the Protection Switching Configuration to external DB.
 *				If its a Get request this function will fillup the received
 *				buffer with latest external Protection Switching configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_protection_config( Stm1_Prot_Parms_S *prot_config, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	if( UI_GET == operation)
	{
		retval = cel_sdh_stm1_protection_config_get(prot_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_protection_config_get() returned FAILURE ");
		}
	}
	else if( UI_SET == operation)
	{
		retval = cel_sdh_stm1_protection_config_set(prot_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_protection_config_set() returned FAILURE ");
		}
	}
	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to Set or Get the Local Command Request for Protection Switching Configuration
 *				to external DB.If its a Get request this function will fillup the received
 *				buffer with latest external LCR Protection Switching configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_protection_config_lcr( Stm1_Prot_Parms_S *prot_config, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	if(UI_GET == operation)
	{
		retval = cel_sdh_stm1_protection_config_lcr_get(prot_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_protection_config_lcr_get() returned FAILURE ");
		}
	}
	else if( UI_SET == operation)
	{
		retval = cel_sdh_stm1_protection_config_lcr_set(prot_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_protection_config_lcr_set() returned FAILURE ");
		}
	}
	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to Set or Get the Core(Mux-Demux) Configuration
 *				to external DB.If its a Get request this function will fillup the received
 *				buffer with latest external Core(Mux-Demux) configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_mux_demux_config(Stm1_Core_Mux_Demux_S *md_config, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	if( UI_GET == operation)
	{
		retval = sdh_stm1_mux_demux_dB(md_config,STM1_GET,0);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_mux_demux_config_get() returned FAILURE ");
		}
	}
	else if( UI_SET == operation)
	{
		retval = sdh_stm1_mux_demux_dB(md_config,STM1_SET,0);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_mux_demux_config_set() returned FAILURE ");
		}
	}
	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to Set or Get the Core(Mux-Demux) Configuration
 *				to external DB.If its a Get request this function will fillup the received
 *				buffer with latest external Core(Mux-Demux) configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_trace_msg_config(Stm1_Core_Trace_Msg_S	*tm_config, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	if( UI_GET == operation)
	{
		retval = sdh_stm1_trace_msg_dB(tm_config,STM1_GET,0);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_trace_msg_config_get() returned FAILURE ");
		}
	}
	else if( UI_SET == operation)
	{
		retval = sdh_stm1_trace_msg_dB(tm_config,STM1_SET,1);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_trace_msg_config_set() returned FAILURE ");
		}
	}
	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to Set or Get the Synchronization Configuration
 *				to external DB.If its a Get request this function will fillup the received
 *				buffer with latest external Synchronization configuration data.If its a Set then it'll
 *				update the external DB with local configuration.
 *
 *	Inputs:		pointer to the received buffer (UI Request), Operation
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_sync_config( Stm1_Sync_Parms_S  *sync_config, Ui_If_Action_E operation)
{
	PAL_INT_32 retval = SUCCESS;
	if( UI_GET == operation)
	{
		retval = cel_sdh_stm1_sync_config_get(sync_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_sync_config_get() returned FAILURE ");
		}
	}
	else if( UI_SET == operation)
	{
		retval = cel_sdh_stm1_sync_config_set(sync_config);
		if(FAILURE == retval)
		{
			printf(" Function cel_sdh_stm1_sync_config_set() returned FAILURE ");
		}
	}
	return retval;
}

PAL_INT_32 sdh_stm1_port_config_dB(Stm1_Port_Config_S *ptr_sdh_port_cfg, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation,\
		PAL_INT_32 create_buffer)
{
	/* This function read/write the STM1 Port Config from/to external database and
	 * updates in ptr_sdh_port_cfg. In case of STM1_GET operation, if this function
	 * should creates the buffer to store the dynamic information, then the create_buffer
	 * value should be updated as '1'
	 */
	return SUCCESS;
}


PAL_INT_32 sdh_stm1_mux_demux_dB(Stm1_Core_Mux_Demux_S *ptr_sdh_md_cfg, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation,\
		PAL_INT_32 create_buffer)
{
	/* This function read/write the STM MuxDemux data from/to external database and
	 * updates in ptr_sdh_md_cfg. In case of STM1_GET operation, if this function
	 * should creates the buffer to store the dynamic information, then the create_buffer
	 * value should be updated as '1'
	 */
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_trace_msg_dB(Stm1_Core_Trace_Msg_S *ptr_sdh_tm_data, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation,\
		PAL_INT_32 create_buffer)
{
	/* This function read/write the STM Trace message data from/to external database and
	 * updates in ptr_sdh_tm_data. In case of STM1_GET operation, if this function
	 * should creates the buffer to store the dynamic information, then the create_buffer
	 * value should be updated as '1'
	 */
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_protection_cfg_dB(Stm1_Prot_Parms_S *ptr_sdh_prot_data, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation)
{
	/* This function read/write the STM1 Protection Configuration data from/to external database and
	 * updates in ptr_sdh_prot_data.
	 */
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_protection_lcr_cfg_dB(Sdh_Prot_Sw_lcr_S *ptr_sdh_prot_lcr_data, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation)
{
	/* This function read/write the STM1 Protection LCR Configuration data from/to external database and
	 * updates in ptr_sdh_prot_lcr_data.
	 */
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_time_sync_cfg_dB(Stm1_Sync_Parms_S *ptr_sdh_time_sync_data, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation)
{
	/* This function read/write the STM1 Time Sync Configuration data from/to external database and
	 * updates in ptr_sdh_time_sync_data.
	 */
	return SUCCESS;
}

PAL_INT_32 sdh_stm1_crosspoint_cfg_dB(Stm1_Cp_Config_S *ptr_sdh_crosspoint_data, Sdh_Stm1_Ext_Db_Op_E sdh_dB_operation, \
		PAL_INT_32 create_buffer)
{
	/* This function read/write the STM1 Crosspoint Configuration data from/to external database and
	 * updates in ptr_sdh_crosspoint_data.
	 */
	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_protection_config_get(Stm1_Prot_Parms_S *prot_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_protection_config_set(Stm1_Prot_Parms_S *prot_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_protection_config_lcr_get(Stm1_Prot_Parms_S *prot_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_protection_config_lcr_set(Stm1_Prot_Parms_S *prot_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_sync_config_get(Stm1_Sync_Parms_S *sync_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_sync_config_set(Stm1_Sync_Parms_S *sync_config)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_cp_config( Stm1_Sync_Parms_S  *sync_config, Ui_If_Action_E operation)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_curr_vendor_get(PAL_CHAR_8 *curr_vendor)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_db_version_get(Sdh_Stm1_Ext_Feature_S *ext_sdh_stm1_feature)
{
	printf(" Entered %s \n", __FUNCTION__);
	/* TBD */
	printf(" Leaving %s \n", __FUNCTION__);
	return 0;
}

PAL_INT_32 write_trace_messge_string_file(Stm1_Core_Trace_Msg_S ptr_sdh_trace_data, PAL_UINT_32 adm_core_index)
{
	/* TBD */
	return 0;
}

PAL_INT_32 cel_sdh_stm1_card_manager_read(Stm1_Card_Manager_S *ptr_sdh_stm1_hw_info)
{
	/* This function will be called by the RPC to read and update the
	 * STM1 and SDH information supported in Sedona Hardware
	 */
}

PAL_INT_32 cel_sdh_stm1_card_max_info_read(Stm1_Card_Max_Info_S *ptr_sdh_stm1_max_info)
{
     return 0;
}

/***********************************************************************************
 * 					SDH related External configuration
 **********************************************************************************/
/*******************************************************************************
 *	Description:This function is used to get the liu line type and liu line code
 *				from external file.
 *
 *	Inputs:		pointer to the variables
 *
 *	Outputs:	Returns failure or Success
 ********************************************************************************/
PAL_INT_32 cel_sdh_stm1_port_config_ltype_lcode_get(PAL_INT_32 *ltype,PAL_INT_32 *lcode)
{
	PAL_INT_32 retval = 0;

	/* TBD : Based on the architecture of external file we'll read the data */

	return retval;
}

/******************************************************************************************************
 *	Description:This function is used to get the system startup time, if its COLD or WARM
 *
 *	Inputs:		Buffer to store the value
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
PAL_INT_32 cel_sdh_stm1_startup_type_get(Sdh_Stm1_Startup_Type_E *startup)
{
	printf(" Entered %s \n", __FUNCTION__);
	/*TBD*/
	printf(" Leaving %s \n", __FUNCTION__);
	return SUCCESS;
}


/* Read the authorization information from card manager */
PAL_INT_32 cel_sdh_stm1_read_auth_info(Sdh_Stm1_Auth_Feature_S *sdh_stm1_feature_auth)
{
	/* Update the sdh_stm1_feature_auth variable wrt the feature index with
	 * authorization data
	 */
	return SUCCESS;
}

PAL_INT_32 cel_sdh_stm1_time_zone_info_get(PAL_FLOAT * timezone)
{
	printf(" Entered %s \n", __FUNCTION__);


	return SUCCESS;
}
/******************************************************************************************************
 *	Description:This function is used to get the STM1 Crosspoint information from CardManager database
 *
 *	Inputs:		Buffer to store the value
 *
 *	Outputs:	Returns failure or Success
 *******************************************************************************************************/
/***********************************************************************************
 * 					SOCKET RELATED APIs
 **********************************************************************************/

/******************************************************************************
 * Description: The various functionalities carried out by this function are.
 *
 * 1) Sends an alarm message to Alarm LOG.  Creates the socket, if necessary
 *
 * Inputs:
 *		1) Msg  - The alarm message structure to be logged.
 *
 * Outputs: Success/Failure,Send the alarm message to the alarm log server.
 *
 ******************************************************************************/
PAL_INT_32 cel_sdh_stm1_send_alarm_to_logger(PAL_CHAR_8 *Msg, gal_socket_client_S *galPtr)
{
	gal_socket_client_S gal_socket_client;
	PAL_INT_32 retval = -1;

	if(!galPtr)
	{
		memset(&gal_socket_client, 0, sizeof(gal_socket_client_S));
		/* Set Socket Path */
		gal_socket_client.sock_path = ALARM_LOG_SOCKET;

		/* create the socket now */
		if(create_udp_send_socket(&gal_socket_client))
			galPtr = &gal_socket_client;
	}

	if(galPtr)
	{
		/* Send the message */
		while(((retval = sendto(galPtr->fd, Msg, sizeof(Msg), 0,\
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
	}
	return retval;
} /* end of send_alarm */


