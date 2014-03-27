/******************************************************************************
 * AIL Library
 *
 * Alarm Interface Library
 *
 * Copyright (C) CarrierComm Inc.
 *****************************************************************************/
/******************************************************************************
 *                              Include Files
 ******************************************************************************/

/*Project includes*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <mqueue.h>
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

/*module includes*/
#include "pal_lib.h"
#include "sdh_stm1_cel.h"
#include "sdh_stm1_ail.h"
#include "../core/include/sdh_stm1_enums.h"
#include "../core/include/sdh_stm1_pm_data_struct.h"
#include "sdh_stm1_config_hw_int_lib.h"

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/
Sdh_Alm_Mntr_Frmt_S		alarm_status;
#define SDH_ALARM_INIT(enum_num,alarm_name, reserve) {alarm_name},
Sdh_Stm1_Alarm_S	alarm_info[] = {SDH_ALARM_LIST};
#undef SDH_ALARM_INIT

/* Hardcoding Module Name as it wont change here */
//PAL_UCHAR module_name_str[MAX_MOD_SIZE] = {"SDH"};
PAL_UCHAR check_msp_alarms[MAX_STM1_ADM_CORE][MAX_HIGH_ORDER_PATHS];
PAL_UCHAR check_sncp_alarms[MAX_STM1_ADM_CORE][MAX_HIGH_ORDER_PATHS];

/******************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/


/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/

PAL_INT_32 ail_sdh_stm1_init_buffer()
{
	memset(&alarm_status,0,sizeof(alarm_status));

	return SUCCESS;
}
/*********************************************************************************
 * 								ALARM MONITORING TASK
 *********************************************************************************/

/*******************************************************************************
*	Description:This function is used to get RS section Alarm info for provided core
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_rs_alarm_get(Sdh_Rs_Alarms_S *rs_alarms_info, Sdh_ADM_Core_E adm_core_index, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32 retval = 0;
	PAL_INT_32 l_rs_alm_status;
	/* Store alarm and status register values */
	if((retval = hil_sdh_stm1_md_rs_alarm_status_get(&l_rs_alm_status, status_query_type, adm_core_index))<0)
	{
		printf("Function hil_sdh_stm1_md_rs_alarm_status_get() returned FAILURE\n");
		return retval;
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.rs_time);

	/* Check for Change in the Alarm status.If there is any change we'll process the alarm otherwise do nothing.  */
	if(memcmp(&l_rs_alm_status, &prev_rs_alm_status[adm_core_index], sizeof(PAL_UINT_32)))
	{
		/* If the stattus ios changed make current status as previous status */
		prev_rs_alm_status[adm_core_index] = l_rs_alm_status;

		/* Filter according to Sedona Alarm Filter Table */
		rs_alarms_info->los = ((l_rs_alm_status & SDH_RS_LOS_STATUS) ? 1 : 0 );
		rs_alarms_info->lof = ((l_rs_alm_status & SDH_RS_LOF_STATUS) ? 1 : 0 );
		if((!rs_alarms_info->los) || (!rs_alarms_info->lof))
		{
			rs_alarms_info->tm_stable = ((l_rs_alm_status & SDH_RS_TM_STABLE_STATUS) ? 1 : 0 );
			rs_alarms_info->oof = ((l_rs_alm_status & SDH_RS_OOF_STATUS) ? 1 : 0 );
			if((!rs_alarms_info->oof) || (!rs_alarms_info->tm_stable) )
			{
				rs_alarms_info->tm_mis = ((l_rs_alm_status & SDH_RS_TM_MIS_STATUS) ? 1 : 0 );
				rs_alarms_info->tm_new = ((l_rs_alm_status & SDH_RS_TM_NEW_STATUS) ? 1 : 0 );
				if((!rs_alarms_info->tm_mis) || (!rs_alarms_info->tm_new) )
				{
					rs_alarms_info->b1 = ((l_rs_alm_status & SDH_RS_ALM_B1_STATUS) ? 1 : 0 );
				}
			}
		}
		/* Update global alarm status structure */
		if((l_rs_alm_status & (SDH_RS_LOS_STATUS | SDH_RS_LOF_STATUS | SDH_RS_OOF_STATUS | SDH_RS_TM_STABLE_STATUS | SDH_RS_TM_NEW_STATUS )) \
					|| (l_rs_alm_status & SDH_RS_TM_MIS_STATUS))
		{
			sdh_global_alarm_status.any_rs_alarm = TRUE;
		}
	}
	return retval;
}

/*******************************************************************************
*	Description:This function is used to get MS section Alarm info for provided core
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_ms_alarm_get(Sdh_Ms_Alarms_S *ms_alarms_info,Sdh_ADM_Core_E adm_core_index, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32	retval=0;
	PAL_INT_32	l_ms_alm_status=0;

	/* Store alarm and status register values */
	retval = hil_sdh_stm1_md_ms_alarm_status_get(&l_ms_alm_status, status_query_type, adm_core_index);
	if(FAILURE == retval)
	{
		printf("Function hil_sdh_stm1_md_ms_alarm_status_get() returned FAILURE\n");
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.ms_time);

	/* Check for Change in the Alarm status.If there any change we'll process the alarm otherwise do nothing.  */
	if(memcmp(&l_ms_alm_status,&prev_ms_alm_status,sizeof(PAL_UINT_32)))
	{
		/* If the stattus is changed make current status as previous status */
		prev_ms_alm_status[adm_core_index] = l_ms_alm_status;
		/* Update MS section global alarm status */
		sdh_global_alarm_status.any_mux_alarm = \
						sdh_global_alarm_status.any_rs_alarm;

		if(!sdh_global_alarm_status.any_rs_alarm)
		{
			ms_alarms_info->ais |= ((l_ms_alm_status & SDH_MS_AIS_STATUS) ? 1 : 0 );

/* TBD: Clarification
			if(!ms_alarms_info->ais)
			{
				ms_alarms_info.b2 |= ((*ms_alm_status & SDH_MS_B2_STATUS) ? 1 : 0 );
			}
			*/
		}

		/* Filter for RDI and REI */
		if(!((sdh_alarms.rs_alarms_info.lof) || (sdh_alarms.rs_alarms_info.oof) || (sdh_alarms.rs_alarms_info.los)))
		{
			ms_alarms_info->rdi 	|= ((l_ms_alm_status & SDH_MS_RDI_STATUS) ? 1 : 0 );
			/* TBD: Clarification
			ms_alarms_info->rei |= ((l_ms_alm_status & SDH_MS_REI_STATUS) ? 1 : 0 );
			*/
		}
	}

 	return retval;
}

/*******************************************************************************
*	Description:This function is used to get HP section Alarm info for provided core
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_hp_alarm_get(Sdh_Hp_Alarms_S *hp_alarms_info,Sdh_ADM_Core_E adm_core_index, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32		retval=0,hp_index=0;
	PAL_USHORT_16	max_higher_order_path=MAX_HIGH_ORDER_PATHS;
	PAL_UINT_32 	*l_hp_alm_status;

	/* Get the max number of Lower order paths */
	cil_sdh_stm1_max_hp_get(&max_higher_order_path);
	max_higher_order_path = 1;

	/* Local Memory Initialization */
	l_hp_alm_status = malloc(sizeof(PAL_UINT_32) *max_higher_order_path);
	memset((PAL_UCHAR*)l_hp_alm_status, 0, sizeof(PAL_UINT_32) *max_higher_order_path);
	memset(check_msp_alarms[adm_core_index], 0, sizeof(char)*max_higher_order_path);
	/* Update HP Global Status Variable */
	sdh_global_alarm_status.any_hp_alarm = sdh_global_alarm_status.any_mux_alarm;

	retval = hil_sdh_stm1_md_hp_alarm_status_get(max_higher_order_path, l_hp_alm_status, status_query_type, adm_core_index);
	if(FAILURE == retval)
	{
		printf("Function hal_sdh_stm1_mux_demux_hp_alarms_register_get() returned FAILURE\n");
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.hp_time);

	/* Store alarm and status register values */
	for(hp_index=0; hp_index<max_higher_order_path; hp_index++ )
	{
		/* Check for Change in the Alarm status.If there any change we'll process the alarm otherwise do nothing.  */
		if(memcmp(&l_hp_alm_status[hp_index],&prev_hp_alm_status[hp_index], sizeof(PAL_UINT_32)))
		{
			/* If the stattus is changed make current status as previous status */
			memcpy(&prev_hp_alm_status[hp_index], &l_hp_alm_status[hp_index], sizeof(PAL_UINT_32));

			/* Filter according to Alarm Filter Table */
			if(!sdh_global_alarm_status.any_mux_alarm)
			{
				hp_alarms_info[hp_index].au_lop = ((l_hp_alm_status[hp_index] & SDH_HP_AU_LOP_STATUS) ? 1 : 0 );
				if(!sdh_alarms.hp_alarms_info[hp_index].au_lop)
				{
					sdh_alarms.hp_alarms_info[hp_index].au_ais |= ((l_hp_alm_status[hp_index] & SDH_HP_AU_AIS_STATUS) ? 1 : 0 );
					if(!sdh_alarms.hp_alarms_info[hp_index].au_ais)
					{
						hp_alarms_info[hp_index].uneq 		= ((l_hp_alm_status[hp_index] & SDH_HP_UNEQ_STATUS) ? 1 : 0 );
						hp_alarms_info[hp_index].tm_tim 	= ((l_hp_alm_status[hp_index] & SDH_HP_TM_TIM_STATUS) ? 1 : 0 );
						hp_alarms_info[hp_index].plm 		= ((l_hp_alm_status[hp_index] & SDH_HP_PLM_STATUS) ? 1 : 0 );
						hp_alarms_info[hp_index].rdi 		= ((l_hp_alm_status[hp_index] & SDH_HP_RDI_STATUS) ? 1 : 0 );
						//hp_alarms_info[hp_index].b3 		= ((l_hp_alm_status[hp_index] & SDH_HP_B3_STATUS) ? 1 : 0 );
						//hp_alarms_info[hp_index].rei 		= ((l_hp_alm_status[hp_index] & SDH_HP_REI_STATUS) ? 1 : 0 );
						check_msp_alarms[adm_core_index][hp_index] = 1;
						/* TBD: Clarification
						hp_alarms_info[hp_index].tm_stable 	= ((l_hp_alm_status[hp_index] & SDH_HP_TM_STABLE_STATUS) ? 1 : 0 );
						hp_alarms_info[hp_index].tm_new 	= ((l_hp_alm_status[hp_index] & SDH_HP_TM_NEW_STATUS) ? 1 : 0 );
						*/
					}
				}
			}
		}
	}//End of for loop
 return retval;
}

/*******************************************************************************
*	Description:This function is used to get LP section Alarm info for provided core
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_lp_alarm_get(Sdh_Lp_Alarms_S *lp_alarms_info, PAL_CHAR_8 *lp_ch_bitmap, Sdh_ADM_Core_E adm_core_index, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32		retval=0, lp_index=0;
	PAL_USHORT_16	max_lower_order_path=MAX_LOW_ORDER_PATHS;
	PAL_UINT_32 	*l_lp_alm_status;

	/* Get the max number of Lower order paths */
	cil_sdh_stm1_max_lp_get(&max_lower_order_path);

	/* Local Memory Initialization */
	l_lp_alm_status = malloc(sizeof(PAL_UINT_32) *max_lower_order_path);
	memset(&l_lp_alm_status, 0, sizeof(PAL_UINT_32) *max_lower_order_path);
	memset(check_sncp_alarms[adm_core_index], 0, sizeof(char)*max_lower_order_path);
	/* Store alarm and status register values */
	if((retval = hil_sdh_stm1_md_lp_alarm_status_get(max_lower_order_path, lp_ch_bitmap, \
						l_lp_alm_status, status_query_type, adm_core_index))<0)
	{
		printf("Function hil_sdh_stm1_md_lp_alarm_status_get() returned FAILURE\n");
		free(l_lp_alm_status);
		return retval;
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.lp_time);

	for(lp_index=0; lp_index<max_lower_order_path; lp_index++ )
	{
		/* Check for Change in the Alarm status.If there any change we'll process the alarm otherwise do nothing.  */
		if(memcmp(&l_lp_alm_status[lp_index],&prev_lp_alm_status[adm_core_index][lp_index], sizeof(PAL_UINT_32)))
		{
			/* If the stattus is changed make current status as previous status */
			memcpy(&prev_lp_alm_status[adm_core_index][lp_index], &l_lp_alm_status[lp_index], sizeof(PAL_UINT_32));

			/* Filter according to Alarm Filter Table */
			if(!sdh_global_alarm_status.any_hp_alarm)
			{
				lp_alarms_info[lp_index].lomf = ((l_lp_alm_status[lp_index] & SDH_LP_LOMF_STATUS) ? 1 : 0 );
				if(!sdh_alarms.lp_alarms_info[lp_index].lomf)
				{
					lp_alarms_info[lp_index].tu_ais = ((l_lp_alm_status[lp_index] & SDH_LP_AIS_STATUS) ? 1 : 0 );
					if(!sdh_alarms.lp_alarms_info[lp_index].tu_ais)
					{
						lp_alarms_info[lp_index].tu_lop = ((l_lp_alm_status[lp_index] & SDH_LP_LOP_STATUS) ? 1 : 0 );
						if(!sdh_alarms.lp_alarms_info[lp_index].tu_lop)
						{
							lp_alarms_info[lp_index].uneq = ((l_lp_alm_status[lp_index] & SDH_LP_UNEQ_STATUS) ? 1 : 0 );
							if(!sdh_alarms.lp_alarms_info[lp_index].uneq)
							{
								lp_alarms_info[lp_index].tm_tim |= ((l_lp_alm_status[lp_index] & SDH_LP_TM_TIM_STATUS) ? 1 : 0 );
								lp_alarms_info[lp_index].plm |= ((l_lp_alm_status[lp_index] & SDH_LP_PLM_STATUS) ? 1 : 0 );
								lp_alarms_info[lp_index].rdi |= ((l_lp_alm_status[lp_index] & SDH_LP_RDI_STATUS) ? 1 : 0 );

								check_sncp_alarms[adm_core_index][lp_index] = 1;
								/* TBD: Clarification
								lp_alarms_info[lp_index].rfi |= ((l_lp_alm_status[lp_index] & SDH_LP_RFI_STATUS) ? 1 : 0 );
								lp_alarms_info[lp_index].tm_new |= ((l_lp_alm_status[lp_index] & SDH_LP_TM_NEW_STATUS) ? 1 : 0 );
								lp_alarms_info[lp_index].rei |= ((l_lp_alm_status[lp_index] & SDH_LP_REI_STATUS) ? 1 : 0 );
								lp_alarms_info[lp_index].bip2 |= ((l_lp_alm_status[lp_index] & SDH_LP_BIP2_STATUS) ? 1 : 0 );
								*/
							}
						}
					}
				}
			}
		}// End of if(memcmp(lp_alm_status,prev_lp_alm_status[lp_index],(sizeof(prev_lp_alm_status)/sizeof(PAL_UINT_32))))
	}//End of for loop
 	free(l_lp_alm_status);
 	return retval;
}


/*******************************************************************************
*	Description:This function is used to get sd and sf Alarm info for SNCP
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_sncp_alarm_get(Sdh_Sncp_Alarms_S *sncp_alarms_info, Stm1_Prot_Parms_S prot_parms, \
											Sdh_ADM_Core_E adm_core_index, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32 		retval=0, sncp_index=0;
	PAL_USHORT_16	max_lower_order_path=MAX_LOW_ORDER_PATHS;
	PAL_UINT_32 	*l_sncp_alm_info;
	/* Store alarm and status register values */

	/* Get the max number of Lower order paths */
	cil_sdh_stm1_max_lp_get(&max_lower_order_path);

	/* Local Memory Initialization */
	l_sncp_alm_info = malloc(sizeof(PAL_UINT_32) *max_lower_order_path);
	memset(l_sncp_alm_info, 0, sizeof(PAL_UINT_32) *max_lower_order_path);

	if((retval = hil_sdh_stm1_sncp_alarm_status_get(l_sncp_alm_info, prot_parms, status_query_type, adm_core_index))<0)
	{
		printf("Function hil_sdh_stm1_sncp_status_register_get() returned FAILURE\n");
		free(l_sncp_alm_info);
		return retval;
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.sncp_alm_time);
	/* Check for Change in the Alarm status.If there is any change we'll process the alarm otherwise do nothing.  */

	for(sncp_index=0; sncp_index<max_lower_order_path; sncp_index++ )
	{
		/* Check for Change in the Alarm status.If there any change we'll process the alarm otherwise do nothing.  */
		if(memcmp(&l_sncp_alm_info[sncp_index], &prev_sncp_alarm_status[sncp_index], sizeof(PAL_UINT_32)))
		{
			/* If the status is changed make current status as previous status */
			memcpy(&prev_sncp_alarm_status[sncp_index], &l_sncp_alm_info[sncp_index], sizeof(PAL_UINT_32));
		}
		/* Update the actual SNCP sf/sd Alarm status */

		if(check_sncp_alarms[adm_core_index][sncp_index])
		{
			sncp_alarms_info[sncp_index].sncp_sf_alm = (( l_sncp_alm_info[sncp_index] & SDH_SNCP_SF_ALM) ? 1 : 0 );
			if(!sncp_alarms_info[sncp_index].sncp_sf_alm)
			{
				sncp_alarms_info[sncp_index].sncp_sd_alm = (( l_sncp_alm_info[sncp_index] & SDH_SNCP_SD_ALM) ? 1 : 0 );
			}
		}
		sncp_alarms_info[sncp_index].sncp_sw_state = (( l_sncp_alm_info[sncp_index] & SDH_SNCP_SWITCH_STATE) ? 1 : 0 );
	}

	free(l_sncp_alm_info);
 	return retval;
}

/*******************************************************************************
*	Description:This function is used to get sd and sf Alarm info for MSP
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_msp_alarm_get(Sdh_Msp_Alarms_S *msp_alarms_info, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32 retval=0, num_core_support = MAX_STM1_ADM_CORE, adm_core_index = 0;
	PAL_INT_32 l_msp_alm_info;
	PAL_UCHAR  switch_state = 0;
	/* Store alarm and status register values */
	retval = hil_sdh_stm1_msp_alarm_status_get(l_msp_alm_info, status_query_type);
	if(FAILURE == retval)
	{
		printf("Function hil_sdh_stm1_msp_status_register_get() returned FAILURE\n");
		return retval;
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.msp_alm_time);
	/* Check for Change in the Alarm status.If there is any change we'll process the alarm otherwise do nothing.  */
	if(memcmp(&l_msp_alm_info, &prev_msp_sd_sf_alm_status, sizeof(PAL_INT_32)))
	{
		/* If the stattus is changed make current status as previous status */
		memcpy(&prev_msp_sd_sf_alm_status , &l_msp_alm_info, sizeof(PAL_INT_32));

		/* Update the actual MSP sf/sd Alarm status */
		cil_sdh_stm1_max_core_num_get(&num_core_support);
		for(adm_core_index = 0; adm_core_index<num_core_support; adm_core_index++)
		{
			if(check_msp_alarms[adm_core_index])
			{
				msp_alarms_info->msp_sd_alm = (( l_msp_alm_info & SDH_MSP_SD_ALM) ? 1 : 0 );
				msp_alarms_info->msp_sf_alm = (( l_msp_alm_info & SDH_MSP_SF_ALM) ? 1 : 0 );
			}
			msp_alarms_info->msp_sw_state = (( l_msp_alm_info & SDH_MSP_SWITCH_STATE) ? 1 : 0 );
		}
	}
 	return retval;
}


/*******************************************************************************
*	Description:This function is used to get LOC Alarm info.
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns SUCCESS  or FAILURE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_loc_alarm_get(PAL_UCHAR *liu_loc_alm_status)
{
	PAL_INT_32 retval=0;
	/* Store alarm and status register values */
	retval = hil_sdh_stm1_liu_signal_control_reg_get(liu_loc_alm_status);
	if(FAILURE == retval)
	{
		printf("Function hil_sdh_stm1_liu_signal_control_reg_get() returned FAILURE\n");
		return retval;
	}
	/* Get the Time Stamp */
	time(&alarm_gen_time.liu_loc_alm_time);
	/* Check for Change in the Alarm status.If there is any change we'll process the alarm otherwise do nothing.  */
	if(memcmp(liu_loc_alm_status,&prev_liu_loc_alm_status,sizeof(PAL_UCHAR)))
	{
		/* If the stattus is changed make current status as previous status */
		prev_liu_loc_alm_status = *liu_loc_alm_status;
		/* Update the actual Loss of Clock Alarm status */
		sdh_alarms.liu_alarms_info.liu_loc_alm |= ((*liu_loc_alm_status & SDH_LIU_LOC_ALM) ? 1 : 0 );
	}

 	return retval;
}


/*******************************************************************************
*	Description:This function is used to get sd and sf Alarm info for SNCP
*
*	Inputs:		 pointer to buffer in which we'll store the status
*
*	Outputs:	Returns STATUS_CHANGED  or NO_STATUS_CHANGE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_sync_alarm_get(PAL_UINT_32 *sync_alm_status, Stm1_SQ_Type_E status_query_type)
{
	PAL_INT_32 retval=0;

    bool stm1_timing_monitor_conditions[MAX_CLOCK_REFERENCES] = { FALSE };
	/* Check the monitoring conditions based on the value */
    cil_sdh_stm1_check_timing_monitor_conditions(stm1_timing_monitor_conditions);

    if(((stm1_timing_monitor_conditions[STM1_SYNC_PRIMARY]) || \
         (stm1_timing_monitor_conditions[STM1_SYNC_SECONDARY]) || \
         (stm1_timing_monitor_conditions[STM1_SYNC_TERTIARY])))
    {
		/* Store alarm and status register values */
		retval = hil_sdh_stm1_sync_register_get(sync_alm_status, status_query_type);
		if(FAILURE == retval)
		{
			printf("Function hil_sdh_stm1_sync_register_get() returned FAILURE\n");
			return retval;
		}
		/* Get the Time Stamp */
		time(&alarm_gen_time.sync_alm_time);
	}
	/* Check for Change in the Alarm status.If there is any change we'll process the alarm otherwise do nothing.  */
	if(memcmp(sync_alm_status,&prev_sync_alm_status,sizeof(PAL_UINT_32)))
	{
		/* If the stattus is changed make current status as previous status */
		prev_sync_alm_status = *sync_alm_status;
		/* Update the actual Timing module Alarm status */
		sdh_alarms.sync_alarms_info.sync_prim_alm |= ((*sync_alm_status & PRIM_REF_UNLOCK) ? 1 : 0 );
		sdh_alarms.sync_alarms_info.sync_sec_alm |= ((*sync_alm_status & SEC_REF_UNLOCK) ? 1 : 0 );
		sdh_alarms.sync_alarms_info.sync_ter_alm |= ((*sync_alm_status & TER_REF_UNLOCK) ? 1 : 0 );
	}

 return retval;
}

/*********************************************************************************
 * 								ALARM ACTION TASK
 *********************************************************************************/

/*******************************************************************************
*	Description:This function is used to filter the RS section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_rs_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	Sdh_Stm1_Alarm_Status_S		rs_alm_status;
	PAL_INT_32	map_cnt=0,alarm_num=0,retval=0;
	memset(&rs_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	for (map_cnt=0;map_cnt<MAX_CP_MAPPING;map_cnt)
	{
		if(recved_alm->core_info == mapped_core_info[map_cnt])
		{
			Sdh_Rs_Alarms_S	*rs_alarms;

			rs_alarms = (Sdh_Rs_Alarms_S *)recved_alm->data;
			/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
			rs_alm_status.alarm_log.type=0;
			/* Update the status of alla the alarms and send to Logger */
			for(alarm_num=STM1_RS_LOS;alarm_num<STM1_RS_B1;alarm_num++)
			{
				/* Clear the data for next section */
				memset(&rs_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
				/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(rs_alm_status.alarm_log.name,alarm_info[alarm_num].alarm_name);
//				strcpy(rs_alm_status.module_name,module_name_str);
				rs_alm_status.alarm_log.name = alarm_num;
				rs_alm_status.alarm_log.time = recved_alm->gen_time;
				rs_alm_status.alarm_log.misc_info = recved_alm->core_info;
				rs_alm_status.alarm_log.no_sub_alarm = 0;
				memset(&rs_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

				switch(alarm_num)
				{
					case STM1_RS_LOS:
						rs_alm_status.alarm_log.status = (rs_alarms->los)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_LOF:
						rs_alm_status.alarm_log.status = (rs_alarms->lof)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_TM_STABLE:
						rs_alm_status.alarm_log.status = (rs_alarms->tm_stable)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_OOF:
						rs_alm_status.alarm_log.status = (rs_alarms->oof)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_TM_MIS:
						rs_alm_status.alarm_log.status = (rs_alarms->tm_mis)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_TM_NEW:
						rs_alm_status.alarm_log.status = (rs_alarms->tm_new)?ALARM_SET:ALARM_CLEAR;
						break;
					case STM1_RS_B1:
						rs_alm_status.alarm_log.status = (rs_alarms->b1)?ALARM_SET:ALARM_CLEAR;
						break;
					default:
						break;
				}

				/* Form Generic Alarm info packet to send to logger module */
				ail_sdh_stm1_generate_alarm(rs_alm_status, RS_SECTION, gal_socket_client);
			}
		}
	}
return retval;
}

/***************************************************************************************
*	Description:This function is used to filter the MS section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_ms_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0,map_cnt=0;
	Sdh_Stm1_Alarm_Status_S		ms_alm_status;
	memset(&ms_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	for (map_cnt=0;map_cnt<MAX_CP_MAPPING;map_cnt)
	{
		if(recved_alm->core_info == mapped_core_info[map_cnt])
		{
			Sdh_Ms_Alarms_S	*ms_alarms;

			ms_alarms = (Sdh_Ms_Alarms_S *)recved_alm->data;

			/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
			ms_alm_status.alarm_log.type=0;
			/* Update the status of alla the alarms and send to Logger */
			for(alarm_num=STM1_MS_RDI;alarm_num<STM1_MS_REI;alarm_num++)
			{
				/* Clear the data for next section */
				memset(&ms_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
				/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(ms_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(ms_alm_status.module_name,module_name_str);
				ms_alm_status.alarm_log.name = alarm_num;
				ms_alm_status.alarm_log.time = recved_alm->gen_time;
				ms_alm_status.alarm_log.misc_info = recved_alm->core_info;
				ms_alm_status.alarm_log.no_sub_alarm = 0;
//				ms_alm_status.alarm_log.alarm_status_bitmap.bitmap = {0};
				memset(&ms_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

				switch(alarm_num)
				{
					case STM1_MS_AIS:
						ms_alm_status.alarm_log.status = (ms_alarms->ais)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_MS_B2:
						ms_alm_status.alarm_log.status = (ms_alarms->b2)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_MS_REI:
						ms_alm_status.alarm_log.status = (ms_alarms->rei)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_MS_RDI:
						ms_alm_status.alarm_log.status = (ms_alarms->rdi)?ALARM_SET:ALARM_CLEAR;
					break;
					default:
					break;
				}
			/* Form Generic Alarm info packet to send to logger module */
			ail_sdh_stm1_generate_alarm(ms_alm_status, MS_SECTION, gal_socket_client);
			}
		}
	}
return retval;
}

/*******************************************************************************
*	Description:This function is used to filter the HP section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
********************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_hp_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0,map_cnt=0;
	Sdh_Stm1_Alarm_Status_S		hp_alm_status;
	memset(&hp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	for (map_cnt=0;map_cnt<MAX_CP_MAPPING;map_cnt)
	{
		if(recved_alm->core_info == mapped_core_info[map_cnt])
		{
			Sdh_Hp_Alarms_S	*hp_alarms;

			hp_alarms = (Sdh_Hp_Alarms_S *)recved_alm->data;

			/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
			hp_alm_status.alarm_log.type=0;
			/* Update the status of alla the alarms and send to Logger */
			for(alarm_num=STM1_HP_AU_AIS;alarm_num<STM1_HP_B3;alarm_num++)
			{
				/* Clear the data for next section */
				memset(&hp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
				/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(hp_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(hp_alm_status.module_name,module_name_str);
				hp_alm_status.alarm_log.name = alarm_num;
				hp_alm_status.alarm_log.time = recved_alm->gen_time;
				hp_alm_status.alarm_log.misc_info = recved_alm->core_info;
				hp_alm_status.alarm_log.no_sub_alarm = 0;
				memset(&hp_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

				switch(alarm_num)
				{
					case STM1_HP_AU_LOP:
						hp_alm_status.alarm_log.status = (hp_alarms->au_lop)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_AU_AIS:
						hp_alm_status.alarm_log.status = (hp_alarms->au_ais)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_TM_TIM:
						hp_alm_status.alarm_log.status = (hp_alarms->tm_tim)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_UNEQ:
						hp_alm_status.alarm_log.status = (hp_alarms->uneq)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_PLM:
						hp_alm_status.alarm_log.status = (hp_alarms->plm)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_B3:
						hp_alm_status.alarm_log.status = (hp_alarms->b3)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_REI:
						hp_alm_status.alarm_log.status = (hp_alarms->rei)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_RDI:
						hp_alm_status.alarm_log.status = (hp_alarms->rdi)?ALARM_SET:ALARM_CLEAR;
					break;
					case STM1_HP_TM_STABLE:
						hp_alm_status.alarm_log.status = (hp_alarms->tm_stable)?ALARM_SET:ALARM_CLEAR;
					break;
					default:
					break;
				}
			/* Form Generic Alarm info packet to send to logger module */
			ail_sdh_stm1_generate_alarm(hp_alm_status, HP_SECTION, gal_socket_client);
			}
		}
	}
return retval;
}


/***************************************************************************************
*	Description:This function is used to filter the LP section alarms and send to Logger
*				Alarm will be sent for all 63/84 channels at one shot.
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_lp_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32 retval=0,alarm_num=0,byte_index=0,lp_index=0;
	PAL_UINT_32 e1_t1_mode=0,map_cnt=0;
	Sdh_Stm1_Alarm_Status_S		lp_alm_status;
	PAL_USHORT_16 max_lower_order_path=MAX_LOW_ORDER_PATHS;
	/* Get the max number of Lower order paths */
	cil_sdh_stm1_max_lp_get(&max_lower_order_path);

	memset(&lp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	for (map_cnt=0;map_cnt<MAX_CP_MAPPING;map_cnt)
	{
		if(recved_alm->core_info == mapped_core_info[map_cnt])
		{
			Sdh_Lp_Alarms_S	*lp_alarms;

			lp_alarms = (Sdh_Lp_Alarms_S *)recved_alm->data;

			/* Update the status of all the alarms and send to Logger */
			for(alarm_num=STM1_LP_AIS;alarm_num<STM1_LP_RFI;alarm_num++)
			{
				/* Clear the data for next section */
				memset(&lp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
				/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(lp_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(lp_alm_status.module_name,module_name_str);
				lp_alm_status.alarm_log.name = alarm_num;
				lp_alm_status.alarm_log.status = 0; // LP Status will be given in bitmap
				lp_alm_status.alarm_log.misc_info = recved_alm->core_info;
				lp_alm_status.alarm_log.no_sub_alarm = max_lower_order_path;

				/* Loop through all the channels */
				for( lp_index = 0;lp_index<max_lower_order_path;lp_index++ )
				{
					/*  Get the byte information based on channel number(lp_index) */
					byte_index = ((lp_index / 8) + ((lp_index % 8) ? 1 : 0));

					switch(alarm_num)
					{
						case STM1_LP_LOMF:
						{
							if(lp_alarms[lp_index].lomf)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_AIS:
						{
							if(lp_alarms[lp_index].tu_ais)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_LOP:
						{
							if(lp_alarms[lp_index].tu_lop)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_UNEQ:
						{
							if(lp_alarms[lp_index].uneq)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_TM_TIM:
						{
							if(lp_alarms[lp_index].tm_tim)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_TM_NEW:
						{
							if(lp_alarms[lp_index].tm_new)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_REI:
						{
							if(lp_alarms[lp_index].rei)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_RDI:
						{
							if(lp_alarms[lp_index].rdi)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_PLM:
						{
							if(lp_alarms[lp_index].plm)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_RFI:
						{
							if(lp_alarms[lp_index].rfi)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						case STM1_LP_BIP2:
						{
							if(lp_alarms[lp_index].bip2)
							{
								lp_alm_status.alarm_log.alarm_status_bitmap.bitmap[byte_index] |= (0x10 >> (lp_index % 8));
							}
						}
						break;
						default:
						break;
					} // End of Switch

				} // End of for( lp_index = 0;lp_index<max_lower_order_path;lp_index++ )
				/* Form Generic Alarm info packet to send to logger module */
				ail_sdh_stm1_generate_alarm(lp_alm_status, LP_SECTION, gal_socket_client);
			} // End of for(alarm_num=STM_TU_LOM;alarm_num<STM_LP_BIP2;alarm_num++)
		}
	}
return retval;
}

/***************************************************************************************
*	Description:This function is used to filter the SNCP section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_sncp_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0;
	Sdh_Stm1_Alarm_Status_S		sncp_alm_status;
	memset(&sncp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	Sdh_Sncp_Alarms_S	*sncp_alarms;

	sncp_alarms = (Sdh_Sncp_Alarms_S *)recved_alm->data;

	/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
	sncp_alm_status.alarm_log.type=0;
	/* Update the status of all the alarms and send to Logger */
	for(alarm_num=STM1_SNCP_SD;alarm_num<STM1_SNCP_SF;alarm_num++)
	{
		/* Clear the data for next section */
		memset(&sncp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
		/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(sncp_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(sncp_alm_status.module_name,module_name_str);
		sncp_alm_status.alarm_log.name = alarm_num;
		sncp_alm_status.alarm_log.time = recved_alm->gen_time;
		sncp_alm_status.alarm_log.misc_info = -1;
		sncp_alm_status.alarm_log.no_sub_alarm = 0;
//				sncp_alm_status.alarm_log.alarm_status_bitmap.bitmap = {0};
		memset(&sncp_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

		switch(alarm_num)
		{
			case STM1_SNCP_SD:
				sncp_alm_status.alarm_log.status = (sncp_alarms->sncp_sd_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			case STM1_SNCP_SF:
				sncp_alm_status.alarm_log.status = (sncp_alarms->sncp_sf_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			default:
			break;
		}
	/* Form Generic Alarm info packet to send to logger module */
	ail_sdh_stm1_generate_alarm(sncp_alm_status, SNCP_SECTION, gal_socket_client);
	}
return retval;
}


/***************************************************************************************
*	Description:This function is used to filter the MSP section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_msp_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0;
	Sdh_Stm1_Alarm_Status_S		msp_alm_status;
	memset(&msp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	Sdh_Msp_Alarms_S	*msp_alarms;

	msp_alarms = (Sdh_Msp_Alarms_S *)recved_alm->data;

	/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
	msp_alm_status.alarm_log.type=0;
	/* Update the status of all the alarms and send to Logger */
	for(alarm_num=STM1_MSP_SD;alarm_num<STM1_MSP_SF;alarm_num++)
	{
		/* Clear the data for next section */
		memset(&msp_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
		/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(msp_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(msp_alm_status.module_name,module_name_str);
		msp_alm_status.alarm_log.name = alarm_num;
		msp_alm_status.alarm_log.time = recved_alm->gen_time;
		msp_alm_status.alarm_log.misc_info = -1;
		msp_alm_status.alarm_log.no_sub_alarm = 0;
//				msp_alm_status.alarm_log.alarm_status_bitmap.bitmap = {0};
		memset(&msp_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

		switch(alarm_num)
		{
			case STM1_MSP_SD:
				msp_alm_status.alarm_log.status = (msp_alarms->msp_sd_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			case STM1_MSP_SF:
				msp_alm_status.alarm_log.status = (msp_alarms->msp_sf_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			default:
			break;
		}
	/* Form Generic Alarm info packet to send to logger module */
	ail_sdh_stm1_generate_alarm(msp_alm_status, MSP_SECTION, gal_socket_client);
	}
return retval;
}

/***************************************************************************************
*	Description:This function is used to filter the MSP section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_loc_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0;
	Sdh_Stm1_Alarm_Status_S		loc_alm_status;
	memset(&loc_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	Sdh_Liu_Alarms_S	*loc_alarms;

	loc_alarms = (Sdh_Liu_Alarms_S *)recved_alm->data;

	/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
	loc_alm_status.alarm_log.type=0;
	/* Update the status of all the alarms and send to Logger */
	for(alarm_num=STM1_MSP_SD;alarm_num<STM1_MSP_SF;alarm_num++)
	{
		/* Clear the data for next section */
		memset(&loc_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
		/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(loc_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(loc_alm_status.module_name,module_name_str);
		loc_alm_status.alarm_log.name 			= alarm_num;
		loc_alm_status.alarm_log.time 			= recved_alm->gen_time;
		loc_alm_status.alarm_log.misc_info 		= -1;
		loc_alm_status.alarm_log.no_sub_alarm 	= 0;
//				loc_alm_status.alarm_log.alarm_status_bitmap.bitmap = {0};
		memset(&loc_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

		switch(alarm_num)
		{
			case STM1_LOSS_OF_CLOCK:
				loc_alm_status.alarm_log.status = (loc_alarms->liu_loc_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			default:
			break;
		}
	/* Form Generic Alarm info packet to send to logger module */
	ail_sdh_stm1_generate_alarm(loc_alm_status, LOC_SECTION, gal_socket_client);
	}
return retval;
}

/***************************************************************************************
*	Description:This function is used to filter the MSP section alarms and send to Logger
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_filter_n_send_sync_alarm(Sdh_Alm_Mntr_Frmt_S *recved_alm, gal_socket_client_S *gal_socket_client)
{
	PAL_INT_32	retval=0,alarm_num=0;
	Sdh_Stm1_Alarm_Status_S		sync_alm_status;
	memset(&sync_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));

	Sdh_Sync_Alarms_S	*sync_alarms;

	sync_alarms = (Sdh_Sync_Alarms_S *)recved_alm->data;

	/* We'll always send alarms to logger module, so we're updating 'type' as 0 */
	sync_alm_status.alarm_log.type=0;
	/* Update the status of all the alarms and send to Logger */
	for(alarm_num=PRIM_SYNC_REF_UNLOCK;alarm_num<TER_SYNC_REF_UNLOCK;alarm_num++)
	{
		/* Clear the data for next section */
		memset(&sync_alm_status,0, sizeof(Sdh_Stm1_Alarm_Status_S));
		/* Fill the Alarm Status Structure with the appropriate data and send it to Logger */
//				strcpy(sync_alm_status.name,alarm_info[alarm_num].alarm_name);
//				strcpy(sync_alm_status.module_name,module_name_str);
		sync_alm_status.alarm_log.name 			= alarm_num;
		sync_alm_status.alarm_log.time 			= recved_alm->gen_time;
		sync_alm_status.alarm_log.misc_info 	= -1;
		sync_alm_status.alarm_log.no_sub_alarm 	= 0;
//				sync_alm_status.alarm_log.alarm_status_bitmap.bitmap = {0};
		memset(&sync_alm_status.alarm_log.alarm_status_bitmap.bitmap,0, sizeof(Stm1_Alm_Status_Bitmap_S));

		switch(alarm_num)
		{
			case PRIM_SYNC_REF_UNLOCK:
				sync_alm_status.alarm_log.status = (sync_alarms->sync_prim_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			case SEC_SYNC_REF_UNLOCK:
				sync_alm_status.alarm_log.status = (sync_alarms->sync_sec_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			case TER_SYNC_REF_UNLOCK:
				sync_alm_status.alarm_log.status = (sync_alarms->sync_ter_alm)?ALARM_SET:ALARM_CLEAR;
			break;
			default:
			break;
		}
	/* Form Generic Alarm info packet to send to logger module */
	ail_sdh_stm1_generate_alarm(sync_alm_status, SYNC_SECTION, gal_socket_client);
	}
return retval;
}


/***************************************************************************************
*	Description:This function is used to filter the LP section alarms and send to Logger
*				Alarm will be sent for all 63/84 channels at one shot.
*
*	Inputs:		 pointer to buffer in which alarm info is stored
*
*	Outputs:	Returns SUCCESS  or FAILURE
****************************************************************************************/
PAL_INT_32 ail_sdh_stm1_generate_alarm(Sdh_Stm1_Alarm_Status_S	stm1_alarm_info, PAL_INT_32 section_cnt, gal_socket_client_S *gal_socket_client)
{
	//SDH_STM1_AlarmdbInfo_S	sdh_stm1_alarm_info;
	LogAlarmdbInfo_S		sdh_stm1_alarm_info;
	PAL_INT_32 offset = 0,*len;
	PAL_CHAR_8 buf[512];

	switch(section_cnt)
	{
		case RS_SECTION:
		{
			sdh_stm1_alarm_info.log_alarm_info.name 									= stm1_alarm_info.alarm_log.name;
			sdh_stm1_alarm_info.log_alarm_info.module_name 								= SDH_STM1;
			sdh_stm1_alarm_info.log_alarm_info.time										= stm1_alarm_info.alarm_log.time;
			sdh_stm1_alarm_info.log_alarm_info.level 									= 2;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.generic_enum 				= GENERIC_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.status 						= stm1_alarm_info.alarm_log.status;
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.extra_id_enum			= EXTRA_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.extra_info 			= 1;
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.core_info 				= stm1_alarm_info.alarm_log.misc_info;
			//sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.valid					= 1;
			/* RPC Copies the Alarm in Subcribe Function in Char Buffer */
			memcpy(buf, &sdh_stm1_alarm_info.log_alarm_info, sizeof(LogAlarmInfo_S));
			offset += sizeof(LogAlarmInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.GenInfo, sizeof(AlarmGenericInfo_S));
			offset += sizeof(AlarmGenericInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo, sizeof(ModuleAlarmExtInfo_S));
			offset += sizeof(ModuleAlarmExtInfo_S);
		}
		break;
		case MS_SECTION:
		case HP_SECTION:
		{
			sdh_stm1_alarm_info.log_alarm_info.name 									= stm1_alarm_info.alarm_log.name;
			sdh_stm1_alarm_info.log_alarm_info.module_name 								= SDH_STM1;
			sdh_stm1_alarm_info.log_alarm_info.time										= stm1_alarm_info.alarm_log.time;
			sdh_stm1_alarm_info.log_alarm_info.level 									= 1;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.generic_enum 				= GENERIC_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.status 						= stm1_alarm_info.alarm_log.status;

			/* RPC Copies the Alarm in Subcribe Function in Char Buffer */
			memcpy(buf, &sdh_stm1_alarm_info.log_alarm_info, sizeof(LogAlarmInfo_S));
			offset += sizeof(LogAlarmInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.GenInfo, sizeof(AlarmGenericInfo_S));
			offset += sizeof(AlarmGenericInfo_S);

		}
		break;
		case LP_SECTION:
		{
			sdh_stm1_alarm_info.log_alarm_info.name 									= stm1_alarm_info.alarm_log.name;
			sdh_stm1_alarm_info.log_alarm_info.module_name 								= SDH_STM1;
			sdh_stm1_alarm_info.log_alarm_info.time										= stm1_alarm_info.alarm_log.time;
			sdh_stm1_alarm_info.log_alarm_info.level 									= 3;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.generic_enum 				= GENERIC_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.status 						= 0; // Lp Alarm Status will be read from the bitmap provided in level-3
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.extra_id_enum			= EXTRA_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.extra_info 			= 1;
			sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo.core_info 				= stm1_alarm_info.alarm_log.misc_info;

			sdh_stm1_alarm_info.log_alarm_data_info.SubAlarmSDHSTM1LPInfo.sub_id_enum	= SUB_ALARM_SDH_STM1_LP_INFO;
			memcpy(sdh_stm1_alarm_info.log_alarm_data_info.SubAlarmSDHSTM1LPInfo.sub_id, \
								stm1_alarm_info.alarm_log.alarm_status_bitmap.bitmap, sizeof(Stm1_Alm_Status_Bitmap_S));
			sdh_stm1_alarm_info.log_alarm_data_info.SubAlarmSDHSTM1LPInfo.valid			= 1;

			/* RPC Copies the Alarm in Subcribe Function in Char Buffer */
			memcpy(buf, &sdh_stm1_alarm_info.log_alarm_info, sizeof(LogAlarmInfo_S));
			offset += sizeof(LogAlarmInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.GenInfo, sizeof(AlarmGenericInfo_S));
			offset += sizeof(AlarmGenericInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.ExtAlarmInfo, sizeof(ModuleAlarmExtInfo_S));
			offset += sizeof(ModuleAlarmExtInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.SubAlarmSDHSTM1LPInfo, sizeof(SubAlarmSDHSTM1LPInfo_S));
			offset += sizeof(SubAlarmSDHSTM1LPInfo_S);

		}
		break;
		case SNCP_SECTION:
		case MSP_SECTION:
		case SYNC_SECTION:
		case LOC_SECTION:
		{
			sdh_stm1_alarm_info.log_alarm_info.name 									= stm1_alarm_info.alarm_log.name;
			sdh_stm1_alarm_info.log_alarm_info.module_name 								= SDH_STM1;
			sdh_stm1_alarm_info.log_alarm_info.time										= stm1_alarm_info.alarm_log.time;
			sdh_stm1_alarm_info.log_alarm_info.level 									= 1;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.generic_enum 				= GENERIC_ALARM_INFO;
			sdh_stm1_alarm_info.log_alarm_data_info.GenInfo.status 						= stm1_alarm_info.alarm_log.status;

			/* RPC Copies the Alarm in Subcribe Function in Char Buffer */
			memcpy(buf, &sdh_stm1_alarm_info.log_alarm_info, sizeof(LogAlarmInfo_S));
			offset += sizeof(LogAlarmInfo_S);
			memcpy(&buf[offset], &sdh_stm1_alarm_info.log_alarm_data_info.GenInfo, sizeof(AlarmGenericInfo_S));
			offset += sizeof(AlarmGenericInfo_S);

		}
		break;
		default:
		break;
	}
	*len = offset;

	/* Send alarm info to logger */
	/* Compilation Error Fix */
	cel_sdh_stm1_send_alarm_to_logger(buf, gal_socket_client);

	return 0;
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

PAL_INT_32 ail_sdh_stm1_alloc_mem(PAL_INT_32 block_info, PAL_INT_32 config, PAL_INT_32 extra_info)
{
	PAL_INT_32 num_core_support=MAX_STM1_ADM_CORE, core_num = 0;
	PAL_INT_32 retval = FAILURE;
	printf(" Entered %s \n", __FUNCTION__);
	switch(config)
	{
		case ALARM_CONFIG:
		{
			printf(" Entered ALARM_CONFIG \n");
			PAL_UINT_32 i=0,alm_size=0,*tmp_alm_status=0;
			alm_size = sizeof(PAL_UINT_32)*block_info;
			cil_sdh_stm1_max_core_num_get(&num_core_support);
			if(num_core_support == block_info)
			{
				return SUCCESS;
			}

			if((prev_rs_alm_status = malloc(alm_size))<0)
			{
				return FAILURE;
			}
			if((prev_ms_alm_status = malloc(alm_size))<0)
			{
				return FAILURE;
			}
			if((prev_hp_alm_status = malloc(alm_size))<0)
			{
				return FAILURE;
			}
			/* Allocate memory for lp section */
			/* Is this correct?*/
			if((prev_lp_alm_status = malloc(alm_size))<0)
			{
				return FAILURE;
			}

			for(core_num = 0; core_num<num_core_support; core_num++)
			{
				if((prev_lp_alm_status[core_num] = malloc(extra_info * sizeof(PAL_UINT_32)))<0)
				{
					return FAILURE;
				}
				memset(prev_lp_alm_status[core_num], 0, sizeof(extra_info * sizeof(PAL_UINT_32)));
			}

			sdh_alarms.lp_alarms_info = malloc(sizeof(Sdh_Lp_Alarms_S) * extra_info);
			memset(sdh_alarms.lp_alarms_info, 0, sizeof(Sdh_Lp_Alarms_S) * extra_info);

			sdh_alarms.hp_alarms_info = malloc(sizeof(Sdh_Hp_Alarms_S) * extra_info);
			memset(sdh_alarms.lp_alarms_info, 0, sizeof(Sdh_Hp_Alarms_S) * extra_info);

			prev_sncp_alarm_status = malloc(sizeof(PAL_INT_32) * extra_info);
			memset(prev_sncp_alarm_status, 0, sizeof(PAL_INT_32) * extra_info);
		}
		break;
		default:
		break;
	}

	printf(" Leaving %s \n",__FUNCTION__);

	return SUCCESS;
} /* end of ail_sdh_stm1_alloc_mem() */


PAL_INT_32 ail_sdh_stm1_adm_intr_summary1_get(PAL_INT_32 *summary1)
{
	PAL_INT_32 retval = FAILURE;

	retval = hil_sdh_stm1_adm_intr_summary1_get(summary1);
	if(FAILURE == retval)
	{
		printf("%s hil_sdh_stm1_intr_summary1_get() Failed \n",__FUNCTION__);
		return FAILURE;
	}

	return retval;
}

PAL_INT_32 ail_sdh_stm1_adm_intr_summary0_get(PAL_INT_32 *summary0)
{
	PAL_INT_32 retval = FAILURE;

	retval = hil_sdh_stm1_adm_intr_summary0_get(summary0);
	if(FAILURE == retval)
	{
		printf("%s hil_sdh_stm1_intr_summary0_get() Failed \n",__FUNCTION__);
		return FAILURE;
	}

	return retval;
}

