/*****************************************************************************
 * Performance Monitoring Library
 *
 * This header captures the attributes of the performance monitoring library.
 *
 * Copyright (C) CarrierComm Inc.
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


#include "../../core/include/sdh_stm1_enums.h"
#include "ces_common.h"
#include "ces_sockets.h"
#include "pal_lib.h"
#include "sdh_stm1_cel.h"
#include "sdh_stm1_pml.h"
#include "sdh_stm1_adm_register_info.h"
#include "sdh_stm1_config_int_ext_lib.h"
#include "sdh_stm1_hal.h"
#include "sdh_stm1_hil.h"
#include "sdh_stm1_cp.h"
#include "ces_util.h"
#include "../../core/include/sdh_stm1_pm_data_struct.h"


/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/
//Pramod: Unused variable
// static PAL_UCHAR 					pm_data_buf[2048]; 		  // To read the full stm pm data from hw
 Sdh_Stm1_Perf_Bank_E 				*inactive_bank,*current_inactive_bank;
 LogPerf_dateTime_S 				sdh_start_interval_dateTime;

 /* This flag will be updated on FRAMER REBOOT or any other suspect condition
  * which makes PM data suspect for respective CORE.
  */
// PAL_UINT_32 stm_adm_cores_suspect_flag; //Pramod: Unused variable

 PAL_CHAR_8		   	stm1_perf_interval_status;

extern Stm1_Sdh_Hw_Max_Interface_S		sdh_stm1_hw_cfg;

/* STM External Configuration database */ //Pramod: Unused Variable
//extern Stm1_Config_S			sdh_config_db;

Sdh_Stm1_Pm_buffer_S	sdh_pm_data_buffer;

unsigned char stm_adm_cores_suspect_flag_24hrs = 0;
unsigned char stm_adm_cores_suspect_flag_15mins = 0;

PAL_FLOAT time_zone_info=0;

/*****************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/
PAL_INT_32 pml_sdh_stm1_pm_request(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_interval_data_get(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_current_data_get(Ui_If_Pm_Data_S *pm_request);

PAL_INT_32 hal_sdh_stm1_pm_inactive_bank_info_get(PAL_UINT_32 *passive_bank);

PAL_INT_32 sdh_stm1_update_24Hr_pm_data(Sdh_ADM_Core_E adm_core_no);
PAL_VOID pml_sdh_stm1_pm_data_suspect_set(Sdh_ADM_Core_E adm_core_no);
PAL_INT_32 get_time_in_dateandtime_format(time_t current_actual_time, PerfMon_dateTime_S *date_and_time_buffer,float time_zone);
PAL_INT_32 cil_sdh_stm1_pm_alloc_mem(PAL_INT_32);
PAL_INT_32 pml_sdh_stm1_data_xml_get(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_far_end_interval_span_data_get(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_far_end_current_data_get(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_far_end_data_xml_get(Ui_If_Pm_Data_S *pm_request);
PAL_INT_32 pml_sdh_stm1_pm_update_dt_interval_data(LogPerf_dateTime_S *sdh_start_interval_dateTime);
PAL_INT_32 get_current_perf_notify_status(PAL_CHAR_8 *);
PAL_INT_32 pml_calculate_remaining_15min_interval(PAL_VOID);
PAL_INT_32 pml_sdh_stm1_adm_pm_config_set(Sdh_Stm1_Pm_Status_E, Sdh_Stm1_Pm_Config_Type_E);

/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/

PAL_INT_32 pml_calculate_remaining_15min_interval(PAL_VOID)
{
     return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_init_buffer(PAL_VOID)
{
	memset(&sdh_pm_data_buffer, 0, sizeof(sdh_pm_data_buffer));

	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_read_mux_cfg_for_pm_calc(PAL_VOID)
{
	PAL_INT_32 ret_val = SUCCESS;

	ret_val = cel_sdh_stm1_read_mux_demux_cfg_pm(&sdh_pm_data_buffer);

	return ret_val;
}

PAL_INT_32 update_perf_interval_start_time(PAL_USHORT_16 max_core_cfg)
{
	PAL_USHORT_16 cfg_cnt = 0, index = 0;
	PerfMon_dateTime_S pm_interval_start_time;
	Sdh_Stm1_Mux_Demux_Cfg_S sdh_mux_data;
	PAL_INT_32 ret_val = SUCCESS;

	/* Update the start time for the current interval */
	get_time_in_dateandtime_format(0,&pm_interval_start_time,time_zone_info);

	for(cfg_cnt = 0;cfg_cnt < max_core_cfg;cfg_cnt++)
	{
		sdh_mux_data = sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].mux_cfg_data;

		if((SDH_OPMODE_DISABLED != sdh_mux_data.op_mode) &&
		 	(ACTIVE == sdh_mux_data.status))
		{
			/* Update the start time for 15 minutes interval data */
			index = sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].pm_15mins_data.intervalIndex;
			sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].pm_15mins_data.interval[index].pm_date_time = \
					pm_interval_start_time;

			/* Update the start time for 24 hours interval data */
			index = sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].pm_24hrs_data.intervalIndex;
			sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].pm_24hrs_data.interval[index].pm_date_time = \
					pm_interval_start_time;
		}
	}

        return ret_val;
}

PAL_INT_32 pml_sdh_stm1_init_pm_calc(Sdh_Stm1_Pm_Status_E *pm_status)
{
	PAL_USHORT_16 max_core_cfg = 0;
	PAL_INT_32 card_cnt = 0, cfg_cnt = 0;
	Sdh_Stm1_Mux_Demux_Cfg_S sdh_mux_data;
	PAL_INT_32 ret_val = SUCCESS;

	for(card_cnt = 0;card_cnt < sdh_pm_data_buffer.num_cards; card_cnt++)
	{
		max_core_cfg += sdh_pm_data_buffer.ptr_sdh_card_info[card_cnt].max_stm_port_core;
	}

	/* Verify any core supports for mux-demux and it
	 * is mapped to STM1 interfaces
	 */
	for(cfg_cnt = 0;cfg_cnt < max_core_cfg; cfg_cnt++)
	{
		sdh_mux_data = sdh_pm_data_buffer.ptr_sdh_pm_data[cfg_cnt].mux_cfg_data;
		if((SDH_OPMODE_DISABLED != sdh_mux_data.op_mode) &&
		 	(ACTIVE == sdh_mux_data.status))
		{
			pml_sdh_stm1_pm_status_set(SDH_STM1_PM_ENABLE);
			*pm_status = SDH_STM1_PM_ENABLE;
			break;
		}
	}

	/* When none of the cores have not configured, disable the
	 * performance monitoring
	 */
	if(cfg_cnt >= max_core_cfg)
	{
		pml_sdh_stm1_pm_status_set(SDH_STM1_PM_DISABLE);
		*pm_status = SDH_STM1_PM_DISABLE;
	}
	else
	{
		/* Update the interval start for the active cores */
		update_perf_interval_start_time(max_core_cfg);
	}

	return ret_val;
}

PAL_INT_32 update_24hrs_stm1_perf_data_calc(unsigned char core_index,\
							Sdh_Stm1_Pm_data_S *ptr_sdh_pm_data)
{
	PAL_USHORT_16 index = 0, index_15mins = 0, i = 0;
	Sdh_15mins_Pm_data_S		perf_15mins_data;
	Sdh_24hrs_Pm_data_S 		*ptr_24hrs_data = NULL;
	PAL_INT_32 ret_val = SUCCESS;

	index = ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalIndex;
	index_15mins = ptr_sdh_pm_data[core_index].pm_15mins_data.intervalIndex;

	perf_15mins_data = ptr_sdh_pm_data[core_index].pm_15mins_data.interval[index_15mins];
	ptr_24hrs_data = &ptr_sdh_pm_data[core_index].pm_24hrs_data.interval[index];

	ptr_24hrs_data->timeElapsed += perf_15mins_data.timeElapsed;

	if(ptr_24hrs_data->timeElapsed <= (SEC_IN_24HRS))
	{
		/* For B1 countrs, update only RS section counters and for STM update RS, MS, HP and LP sections
				counters */
		/*Copy ES,SES & UAS data into buffer For B1,B2,B3,B4*/

		ptr_24hrs_data->pm_data.regSection.status |= perf_15mins_data.pm_data.regSection.status;
		ptr_24hrs_data->pm_data.regSection.es += perf_15mins_data.pm_data.regSection.es;
		ptr_24hrs_data->pm_data.regSection.ses += perf_15mins_data.pm_data.regSection.ses;
		ptr_24hrs_data->pm_data.regSection.uas += perf_15mins_data.pm_data.regSection.uas;
		if(SDH_OPMODE_MUXDEMUX == ptr_sdh_pm_data[core_index].mux_cfg_data.op_mode)
		{
			ptr_24hrs_data->pm_data.muxLine.status |= perf_15mins_data.pm_data.muxLine.status;
			ptr_24hrs_data->pm_data.muxLine.es += perf_15mins_data.pm_data.muxLine.es;
			ptr_24hrs_data->pm_data.muxLine.ses += perf_15mins_data.pm_data.muxLine.ses;
			ptr_24hrs_data->pm_data.muxLine.uas += perf_15mins_data.pm_data.muxLine.uas;

			/*printf("STM Core %d Interval 24HRS MS es:%d ses:%d uas:%d status:%x \n",
					core_index,ptr_24hrs_data->pm_data.muxLine.es,\
					ptr_24hrs_data->pm_data.muxLine.ses,ptr_24hrs_data->pm_data.muxLine.uas,
					ptr_24hrs_data->pm_data.muxLine.status);*/

			ptr_24hrs_data->pm_data.highOrderPath[0].status |= perf_15mins_data.pm_data.highOrderPath[0].status;
			ptr_24hrs_data->pm_data.highOrderPath[0].es += perf_15mins_data.pm_data.highOrderPath[0].es;
			ptr_24hrs_data->pm_data.highOrderPath[0].ses += perf_15mins_data.pm_data.highOrderPath[0].ses;
			ptr_24hrs_data->pm_data.highOrderPath[0].uas += perf_15mins_data.pm_data.highOrderPath[0].uas;

			/*printf("STM Core %d Interval 24HRS HP es:%d ses:%d uas:%d status:%x \n",
					core_index,ptr_24hrs_data->pm_data.highOrderPath[0].es,\
					ptr_24hrs_data->pm_data.highOrderPath[0].ses,ptr_24hrs_data->pm_data.highOrderPath[0].uas,
					ptr_24hrs_data->pm_data.highOrderPath[0].status);*/

			for(i = 0;i < MAX_LOW_ORDER_PATHS ; i++)
			{
				ptr_24hrs_data->pm_data.lowOrderPath[i].status |= perf_15mins_data.pm_data.lowOrderPath[i].status;
				ptr_24hrs_data->pm_data.lowOrderPath[i].es += perf_15mins_data.pm_data.lowOrderPath[i].es;
				ptr_24hrs_data->pm_data.lowOrderPath[i].ses += perf_15mins_data.pm_data.lowOrderPath[i].ses;
				ptr_24hrs_data->pm_data.lowOrderPath[i].uas += perf_15mins_data.pm_data.lowOrderPath[i].uas;

			/*printf("STM Core %d Interval 24HRS LP %d es:%d ses:%d uas:%d status:%x \n",
					core_index,i,ptr_24hrs_data->pm_data.lowOrderPath[i].es,\
					ptr_24hrs_data->pm_data.lowOrderPath[i].ses,ptr_24hrs_data->pm_data.lowOrderPath[i].uas,
					ptr_24hrs_data->pm_data.lowOrderPath[i].status);*/
			}
		}
	}
	else
	{
		ptr_24hrs_data->timeElapsed = SEC_IN_24HRS;
	}
	
	return ret_val;
}

PAL_INT_32 update_24hrs_stm1_perf_buffer(unsigned char core_index,\
			PerfMon_dateTime_S dateTime, Sdh_Stm1_Pm_data_S *ptr_sdh_pm_data)
{
	PAL_USHORT_16 index = 0, count = 0;
	Sdh_24hrs_Pm_data_S *ptr_24hrs_interval = NULL;
	unsigned char bit_set[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	PAL_INT_32 ret_val = SUCCESS;

	index = ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalIndex;
	count = ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalCount;

	/* Update the interval data information for the 24 hour */
	if(MAX_INTERVAL_24HRS == count)
	{
		PAL_USHORT_16 out_index = 0;

		out_index = (index + 1 ) % MAX_INTERVAL_24HRS_DATA;

		ptr_24hrs_interval = &ptr_sdh_pm_data[core_index].pm_24hrs_data.interval[out_index];

		if((PERF_VALID == ptr_24hrs_interval->valid_info.regSection.valid) && \
					(ptr_sdh_pm_data[core_index].pm_24hrs_data.validIntervals > 0))
		{
			ptr_sdh_pm_data[core_index].pm_24hrs_data.validIntervals--;
		}
		else if((PERF_INVALID == ptr_24hrs_interval->valid_info.regSection.valid) && \
						(ptr_sdh_pm_data[core_index].pm_24hrs_data.invalidIntervals > 0))
		{
			ptr_sdh_pm_data[core_index].pm_24hrs_data.invalidIntervals--;
		}
	}

	ptr_24hrs_interval = &ptr_sdh_pm_data[core_index].pm_24hrs_data.interval[index];
	/* Since the 24 hours data is already aggregated in handle_stm_log function, here we will
	 * just increment the index and count value with valid information of the data.
	 * And update the timeElapsed
	 */
	/* For STM Performance data and valid data, the index and count values are same */
	if( (stm_adm_cores_suspect_flag_24hrs & bit_set[core_index]) || (0 == count))
	{
		memset(&ptr_24hrs_interval->valid_info,PERF_INVALID,\
						sizeof(CurrentLayerValid_Interval_S));
	}
	else
	{
		memset(&ptr_24hrs_interval->valid_info,PERF_VALID,\
						sizeof(CurrentLayerValid_Interval_S));
	}
	stm_adm_cores_suspect_flag_24hrs &= ~bit_set[core_index];

	/* Update the valid and invalid intervals information for 24 hours data */
	if((PERF_VALID == ptr_24hrs_interval->valid_info.regSection.valid) && \
						(ptr_sdh_pm_data[core_index].pm_24hrs_data.validIntervals < MAX_INTERVAL_24HRS))
	{
		ptr_sdh_pm_data[core_index].pm_24hrs_data.validIntervals++;
	}
	else if((PERF_INVALID == ptr_24hrs_interval->valid_info.regSection.valid) && \
						(ptr_sdh_pm_data[core_index].pm_24hrs_data.invalidIntervals < MAX_INTERVAL_24HRS))
	{
		ptr_sdh_pm_data[core_index].pm_24hrs_data.invalidIntervals++;
	}

	printf(" Interval received : STM Core %d 24 HRS index = %d cound = %d  \n",core_index,count,index);

	/* Update Buffer of 24hours interval index and count */
	index++;
	index %= MAX_INTERVAL_24HRS_DATA;

	if(count < MAX_INTERVAL_24HRS)
		count++;

	/* Update the next interval start date and time for 24 hour interval data */
	memcpy(&ptr_sdh_pm_data[core_index].pm_24hrs_data.interval[index].pm_date_time,\
								&dateTime,sizeof(PerfMon_dateTime_S));

	ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalIndex = index;
	ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalCount = count;

	/* Clear the new index information to store the next 24 hour interval data */
	memset(&ptr_sdh_pm_data[core_index].pm_24hrs_data.interval[index].pm_data,\
								0,sizeof(Current24hrsLayerData_S));
	return ret_val;
}

PAL_INT_32 update_24hrs_stm1_perf_interval_data(unsigned char core_index,\
			PerfMon_dateTime_S dateTime, Sdh_Stm1_Pm_data_S *ptr_sdh_pm_data)
{
	PAL_UCHAR hour,minute,second;
	PAL_INT_32 ret_val = SUCCESS;

	/* Compute the 24 hours current interval data */
	update_24hrs_stm1_perf_data_calc(core_index,ptr_sdh_pm_data);

	hour = dateTime.date_time[4];
	minute = dateTime.date_time[5];
	second = dateTime.date_time[6];

	if((0 == hour) && (0 == minute) && (0 == second))
	{
		update_24hrs_stm1_perf_buffer(core_index, dateTime, \
										ptr_sdh_pm_data);
	}
	
	return ret_val;
}

PAL_INT_32 update_stm1_perf_interval_data(unsigned char core_index,Sdh_Stm1_Pm_data_S *ptr_sdh_pm_data,
						Stm1_Perf_Data_S perf_interval_data, time_t stm1_perf_current_time)
{
	PAL_USHORT_16 index = 0, count = 0;
	Sdh_15mins_Pm_data_S *ptr_interval_data = NULL;
	PerfMon_dateTime_S pm_interval_start_time;
	PAL_UCHAR bit_set[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	PAL_INT_32 ret_val = SUCCESS;
        

	index = ptr_sdh_pm_data[core_index].pm_15mins_data.intervalIndex;
	count = ptr_sdh_pm_data[core_index].pm_15mins_data.intervalCount;

	if(count == MAX_INTERVALS)
	{
		PAL_USHORT_16 out_index = 0;

		out_index = (index+1) % MAX_INTERVAL_DATA;

		ptr_interval_data = &ptr_sdh_pm_data[core_index].pm_15mins_data.interval[out_index];

		if((ptr_interval_data->valid_info.regSection.valid == PERF_VALID) && \
			( ptr_sdh_pm_data[core_index].pm_15mins_data.validIntervals > 0))
		{
			 ptr_sdh_pm_data[core_index].pm_15mins_data.validIntervals--;
		}
		else if((ptr_interval_data->valid_info.regSection.valid == PERF_INVALID) && \
				(ptr_sdh_pm_data[core_index].pm_15mins_data.invalidIntervals > 0))
		{
			ptr_sdh_pm_data[core_index].pm_15mins_data.invalidIntervals--;
		}
	}

	ptr_interval_data = &ptr_sdh_pm_data[core_index].pm_15mins_data.interval[index];

	printf("%s :The value of count and index are %d and %d \n",__FUNCTION__,count,index);

	/*copy the data in main pm db*/
	memcpy(&ptr_interval_data->pm_data, \
				&perf_interval_data.layer,sizeof(CurrentLayerData_S));
	ptr_interval_data->timeElapsed = perf_interval_data.timeElapsed;

	if((count == 0) || (PERF_INVALID == perf_interval_data.valid) || \
			(stm_adm_cores_suspect_flag_15mins & bit_set[core_index]))
	{
		memset(&ptr_interval_data->valid_info,PERF_INVALID,\
						sizeof(CurrentLayerValid_Interval_S));
		stm_adm_cores_suspect_flag_24hrs |= bit_set[core_index];
	}
	else if(PERF_VALID == perf_interval_data.valid)
	{
		memset(&ptr_interval_data->valid_info,PERF_VALID,\
						sizeof(CurrentLayerValid_Interval_S));
	}

	if((ptr_interval_data->valid_info.regSection.valid == PERF_VALID) && \
		(ptr_sdh_pm_data[core_index].pm_15mins_data.validIntervals < MAX_INTERVALS))
	{
		ptr_sdh_pm_data[core_index].pm_15mins_data.validIntervals++;
	}
	else if((ptr_interval_data->valid_info.regSection.valid == PERF_INVALID) && \
		(ptr_sdh_pm_data[core_index].pm_15mins_data.invalidIntervals < MAX_INTERVALS))
	{
		ptr_sdh_pm_data[core_index].pm_15mins_data.invalidIntervals++;
	}

	/* Update the start time for the current interval */
	get_time_in_dateandtime_format(stm1_perf_current_time,\
					&pm_interval_start_time,time_zone_info);

	update_24hrs_stm1_perf_interval_data(core_index,pm_interval_start_time, \
							ptr_sdh_pm_data);

	/* Update Buffer Index */
	index++;

	/* This is a FIFO circular buffer of size MAX_INTERVALS */
	index %= MAX_INTERVALS;

	if(count < MAX_INTERVALS)
		count++;

	/* Update the next interval start date and time for 15 minutes interval data */
	ptr_interval_data = &ptr_sdh_pm_data[core_index].pm_15mins_data.interval[index];
	memcpy(&ptr_interval_data->pm_date_time,&pm_interval_start_time, \
							sizeof(pm_interval_start_time));

	ptr_sdh_pm_data[core_index].pm_15mins_data.intervalCount = count;
	ptr_sdh_pm_data[core_index].pm_15mins_data.intervalIndex = index;

	return ret_val;
}

PAL_INT_32 pml_sdh_stm1_read_perf_interval_data(unsigned char card_index, unsigned char core_offset,
				unsigned char max_cores,time_t stm1_perf_current_time,\
				Sdh_Stm1_Pm_buffer_S *ptr_sdh_pm_data_buffer)
{
	Sdh_Stm1_Mux_Demux_Cfg_S mux_cfg;
	unsigned char max_cores_info = core_offset + max_cores, core_index = 0;
	unsigned int bank_status = 0;
	Stm1_Perf_Data_S perf_interval_data;
	PAL_FLOAT time_zone_info=0;
	PAL_INT_32 retval = SUCCESS;

	cel_sdh_stm1_time_zone_info_get(&time_zone_info);

	memset(&mux_cfg, 0, sizeof(mux_cfg));
	memset(&perf_interval_data, 0, sizeof(perf_interval_data));

	for(core_index = core_offset;core_index < max_cores_info;core_index++)
	{
		mux_cfg = ptr_sdh_pm_data_buffer->ptr_sdh_pm_data[core_index].mux_cfg_data;

		if((SDH_OPMODE_DISABLED == mux_cfg.op_mode) ||
			(ACTIVE == mux_cfg.status))
		{
			continue;
		}

		/* Read the performace data from tdmComplex */
		retval = hil_sdh_stm1_pm_interval_data_get(card_index,core_index,\
									&bank_status,&perf_interval_data);
		if(SUCCESS != retval)
		{
			printf(" %s : Failed to read the performance data \n", __FUNCTION__);
		}

		ptr_sdh_pm_data_buffer->ptr_sdh_pm_data[core_index].bank_status = bank_status;

		update_stm1_perf_interval_data(core_index, ptr_sdh_pm_data_buffer->ptr_sdh_pm_data,\
										perf_interval_data, stm1_perf_current_time);

	}
	return retval;
}

/* Copied required function from previous implementation */
/******************************************************************************
 * Description: Function to invalidate all the STM1 interval data when
 *
 *
 * 1) Updates Unavailable intervals of the STM1
 *
 * Inputs: The STM1 interval data structure
 *
 * Outputs: None, Updates STM1 interval data buffer.
 *
 *****************************************************************************/
PAL_VOID mark_stm1_perf_unavailable(PAL_INT_32 core_number)
{
/*	pm_data.sdh_pm_data[core_number].invalidIntervals = 0 ;
	pm_data.sdh_pm_data[core_number].validIntervals = 0;

	pm_data.sdh_pm_data[core_number].intervalCount = 0;
	pm_data.sdh_pm_data[core_number].intervalIndex = 0;*/

	/*<Bugzilla><2162><27-May-2010><RR>Clear Date and Time Buffer */
	/*pm_date_time.sdh_pm_date_time[core_number].intervalCount = 0;
	pm_date_time.sdh_pm_date_time[core_number].intervalIndex = 0;*/

	return ;
} /* End for mark_stm_perf_unavailable() */
PAL_INT_32 dateandtime_set_buf_from_vars(unsigned char *buf,
									unsigned int *bufsize,
									unsigned short year,
									unsigned char month,
									unsigned char day,
									unsigned char hour,
									unsigned char minutes,
									unsigned char seconds,
									unsigned char deci_seconds,
									PAL_INT_32 utc_offset_direction,
									unsigned char utc_offset_hours,
									unsigned char utc_offset_minutes)
{
    PAL_USHORT_16 tmp_year = htons(year);

    /*
     * if we have a utc offset, need 11 bytes. Otherwise we
     * just need 8 bytes.
     */
    if(utc_offset_direction) {
        if(*bufsize < 11)
            return FAILURE;

        /*
         * set utc offset data
         */
        buf[8] = (utc_offset_direction < 0) ? '-' : '+';
        buf[9] = utc_offset_hours;
        buf[10] = utc_offset_minutes;
        *bufsize = 11;
    }
    else if(*bufsize < 8)
        return FAILURE;
    else
        *bufsize = 8;

    /*
     * set basic date/time data
     */
    memcpy(buf, &tmp_year, sizeof(tmp_year));
    buf[2] = month;
    buf[3] = day;
    buf[4] = hour;
    buf[5] = minutes;
    buf[6] = seconds;
    buf[7] = deci_seconds;

    return SUCCESS;
}

PAL_INT_32 get_time_in_dateandtime_format(PAL_TIME_T current_actual_time, PerfMon_dateTime_S *date_and_time_buffer,PAL_FLOAT time_zone)
{
	struct tm *nowtime;
	struct tm timezone_min;
	PAL_UINT_32 size_DT = MAX_DATE_TIME_SIZE;
	PAL_USHORT_16 years;
	PAL_UCHAR hours,minutes;

	char date_time[6];
	time_t current_time;
	char *str_sam;
	PAL_INT_32 int_time_zone;

	if(current_actual_time == 0)
	{
		time(&current_time);
		nowtime = localtime(&current_time);
	}
	else
	{
		nowtime = localtime(&current_actual_time);
	}
	years = (unsigned short) ((unsigned short)(1900 + nowtime->tm_year));
	sprintf(date_time,"%2.2f",time_zone);
	str_sam = strstr(date_time,".");
	/*Check the user enterd value for timezone is 'nan', If it is 'nan' then change to 0.00*/
	if(str_sam != NULL)
	{
		strncpy (str_sam,":",1);
	}
	else
	{
		sprintf(date_time,"0:00");
	}
	str_to_time(date_time,&timezone_min);

	int_time_zone = (time_zone < 0) ? -1 : 1;

	hours = (unsigned char) ((time_zone < 0)? (time_zone = time_zone * (-1)) : time_zone);
	minutes =(unsigned char) timezone_min.tm_min;
    return dateandtime_set_buf_from_vars((unsigned char *)date_and_time_buffer, \
    										&size_DT, \
    										years, \
    										(unsigned char)(nowtime->tm_mon+1), \
    										(unsigned char)(nowtime->tm_mday), \
    										(unsigned char)(nowtime->tm_hour), \
    										(unsigned char)(nowtime->tm_min), \
    										(unsigned char)(nowtime->tm_sec), \
    										(unsigned char)((0) / 100000), \
    										(int_time_zone),hours,minutes);
}

/****************************************************************************
*   Description:This function will get the performance data from
*				the hardware.
*
*   Inputs:     None
*
*   Outputs:    None
****************************************************************************/
PAL_UINT_32 pml_sdh_stm1_pm_status_set(PAL_UINT_32 pm_status)
{
	PAL_UINT_32 retval = SUCCESS;
	PAL_UINT_32 current_pm_status = 0;

	/* Read the current pm status info (ENABLE/DISABLE) from the register */
	retval = hil_sdh_stm1_pm_status_get(&current_pm_status);
	if(FAILURE == retval )
	{
		printf("Function hil_sdh_stm1_pm_status_get() returned FAILURE ");
	}

	if(current_pm_status != pm_status)
	{
		/* Write the current pm status info (ENABLE/DISABLE) to the register */
		retval = hil_sdh_stm1_pm_status_set(pm_status);
		if(FAILURE == retval )
		{
			printf("Function hil_sdh_stm1_pm_status_set() returned FAILURE ");
		}
	}

	return retval;
}

PAL_INT_32 get_pm_buffer_core_index(unsigned int card_num, unsigned char core_num)
{
	PAL_UINT_32 card_cnt = 0;
	PAL_INT_32 num_cores = 0;

	if(card_num > sdh_stm1_hw_cfg.card_num)
	{
		return FAILURE;
	}

	if(0 == card_num)
	{
		num_cores = core_num;
		return num_cores;
	}

	num_cores = 0;
	for(card_cnt = 0;card_cnt < card_num;card_cnt++)
	{
		num_cores += sdh_stm1_hw_cfg.interface_info[card_cnt].max_sdh_cores_supported;
	}

	num_cores += core_num;

	return num_cores;
}

PAL_INT_32 pml_read_current_interval_data(unsigned int card_num, unsigned char core_num,\
					unsigned char core_index,Sdh_Stm1_Pm_Data_E data_section, void *ptr_path_index)
{
	PAL_USHORT_16 index = 0;
	Sdh_15mins_Pm_data_S *ptr_cur_interval = NULL;

	index = sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_15mins_data.intervalIndex;
	ptr_cur_interval = &sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_15mins_data.interval[index];

	if((GET_RS_DATA == data_section) || (GET_MS_DATA == data_section) || (GET_MS_FAR_DATA == data_section))
	{
		hil_sdh_stm1_perf_current_data_get(card_num,core_num,data_section,
				ptr_cur_interval,0);
	}
	else if((GET_HP_DATA == data_section) || (GET_LP_DATA == data_section))
	{
		perf_path_val_S *ptr_path_info = ptr_path_index;
		unsigned char path_index = 0;

		for(path_index = 0;path_index < ptr_path_info->path_count; path_index++)
		{
			hil_sdh_stm1_perf_current_data_get(card_num,core_num,data_section,
				ptr_cur_interval,\
				ptr_path_info->path_data[path_index].path_num);
		}
	}
	else if((GET_HP_FAR_DATA == data_section) || (GET_LP_FAR_DATA == data_section))
	{
		perf_far_path_val_S *ptr_far_path_info = ptr_path_index;
		unsigned char path_index = 0;

		for(path_index = 0;path_index < ptr_far_path_info->path_count;path_index++)
		{
			hil_sdh_stm1_perf_current_data_get(card_num,core_num,data_section,
				ptr_cur_interval,\
				ptr_far_path_info->far_path_data[path_index].path_num);
		}
	}

        return SUCCESS;
}

PAL_INT_32 get_pm_15mins_rs_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
					CurrentPmRegSection_S *rs_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_15mins_Pm_data_S interval_data;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index];
		rs_data[count] = interval_data.pm_data.regSection;
		count++;
	}
    return  SUCCESS;
}

PAL_INT_32 update_24hrs_rs_current_interval(PAL_INT_32 core, CurrentPm24hrsRegSection_S rs_24hrs_data,\
										CurrentPm24hrsRegSection_S *ptr_rs_24hrs_data)
{
	PAL_SHORT_16 index = 0;
	CurrentPmRegSection_S rs_15mins_data;

	memset(&rs_15mins_data, 0, sizeof(rs_15mins_data));

	index = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	rs_15mins_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.\
											interval[index].pm_data.regSection;

	ptr_rs_24hrs_data->status = rs_24hrs_data.status | rs_15mins_data.status;
	ptr_rs_24hrs_data->es = rs_24hrs_data.es + rs_15mins_data.es;
	ptr_rs_24hrs_data->ses = rs_24hrs_data.ses + rs_15mins_data.ses;
	ptr_rs_24hrs_data->uas = rs_24hrs_data.uas + rs_15mins_data.uas;

	return SUCCESS;
}

PAL_INT_32 update_24hrs_mux_current_interval(PAL_INT_32 core, CurrentPm24hrsLinePath_S mux_24hrs_data,\
										CurrentPm24hrsLinePath_S *ptr_mux_24hrs_data)
{
	PAL_SHORT_16 index = 0;
	CurrentPmLinePath_S mux_15mins_data;

	memset(&mux_15mins_data, 0, sizeof(mux_15mins_data));

	index = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	mux_15mins_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.\
											interval[index].pm_data.muxLine;

	ptr_mux_24hrs_data->status = mux_24hrs_data.status | mux_15mins_data.status ;
	ptr_mux_24hrs_data->es = mux_24hrs_data.es + mux_15mins_data.es ;
	ptr_mux_24hrs_data->ses = mux_24hrs_data.ses + mux_15mins_data.ses ;
	ptr_mux_24hrs_data->uas = mux_24hrs_data.uas + mux_15mins_data.uas ;

	return SUCCESS;
}

PAL_INT_32 update_24hrs_h1_current_interval(PAL_INT_32 core, Sdh_Stm1_Pm_Data_E data_info, \
		CurrentPm24hrsLinePath_S *hl_24hrs_data, perf_24hrs_path_val_S *ptr_hl_24hrs_data)
{
	PAL_SHORT_16 index = 0, path_index = 0, path_num = 0;
	CurrentPmLinePath_S *hp_lp_15mins_data = NULL;

	memset(hp_lp_15mins_data, 0, sizeof(hp_lp_15mins_data));

	index = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;

	if(GET_HP_DATA == data_info)
	{
		hp_lp_15mins_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.\
											interval[index].pm_data.highOrderPath;
	}
	else
	{
		hp_lp_15mins_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.\
											interval[index].pm_data.lowOrderPath;
	}

	for(path_index = 0;path_index < ptr_hl_24hrs_data->path_count;path_index++)
	{
		path_num = ptr_hl_24hrs_data->path_24hrs_data[path_index].path_num;
		ptr_hl_24hrs_data->path_24hrs_data[path_index].hl_path_data.status = \
					hl_24hrs_data[path_num].status | hp_lp_15mins_data[path_num].status ;
		ptr_hl_24hrs_data->path_24hrs_data[path_index].hl_path_data.es = \
					hl_24hrs_data[path_num].es + hp_lp_15mins_data[path_num].es ;
		ptr_hl_24hrs_data->path_24hrs_data[path_index].hl_path_data.ses = \
					hl_24hrs_data[path_num].ses + hp_lp_15mins_data[path_num].ses ;
		ptr_hl_24hrs_data->path_24hrs_data[path_index].hl_path_data.uas = \
					hl_24hrs_data[path_num].uas + hp_lp_15mins_data[path_num].uas ;
	}

	return SUCCESS;

}

PAL_INT_32 get_pm_24hrs_rs_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
					CurrentPm24hrsRegSection_S *rs_24hrs_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_24hrs_Pm_data_S interval_data;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Read the current 24 hours data wrt 15mins intervals */
	if(0 == start_int)
	{
		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];
		update_24hrs_rs_current_interval(core,interval_data.pm_data.regSection, &rs_24hrs_data[0]);

		start_int = 1;
		count = 1;
	}
	else
	{
		count = 0;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_24HRS_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];
		rs_24hrs_data[count] = interval_data.pm_data.regSection;
		count++;
	}

	return SUCCESS;
}

PAL_INT_32 get_pm_rs_mux_suspect_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
			pm_user_request_S *pm_request, CurrentLinePmPathValid_S *mux_suspect_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0;
	PAL_SHORT_16 index1 = 0, max_interval_data = 0;


	/* Read the current interval index and count for the core */
	if(PM_DATA_15_MIN == pm_request->pm_data_type)
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;
		max_interval_data = MAX_INTERVAL_DATA;
	}
	else
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;
		max_interval_data = MAX_INTERVAL_24HRS_DATA;
	}

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = max_interval_data - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1; count < max_count; index--)
	{
		if(index < 0)
			index = intervalCount;

		switch(pm_request->pm_data_type)
		{
			case PM_DATA_15_MIN:
			{
				CurrentLayerValid_Interval_S interval_data;

				interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index].valid_info;
				if(GET_RS_SUSPECT_DATA == pm_request->pm_data_section)
				{
					mux_suspect_data[count] = interval_data.regSection;
				}
				else if(GET_MS_SUSPECT_DATA == pm_request->pm_data_section)
				{
					mux_suspect_data[count] = interval_data.muxLine;
				}
				else
				{
					mux_suspect_data[count] = interval_data.farMuxLine;
				}
				break;
			}
			case PM_DATA_24_HOUR:
			{
				Current24hrsLayerValid_Interval_S interval_data;

				interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index].valid_info;
				if(GET_RS_SUSPECT_DATA == pm_request->pm_data_section)
				{
					mux_suspect_data[count] = interval_data.regSection;
				}
				else if(GET_MS_SUSPECT_DATA == pm_request->pm_data_section)
				{
					mux_suspect_data[count] = interval_data.muxLine;
				}
				else
				{
					printf(" %s : 24hrs pm_data_type %d not (FAR) supported\n", __FUNCTION__, pm_request->pm_data_type);
				}
				break;
			}
		}

		count++;

	}

	return SUCCESS;
}

PAL_INT_32 get_pm_interval_dateandtime(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
				Sdh_Stm1_Pm_Data_Type_E data_info,PerfMon_dateTime_S *dateAndTime)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0;
	PAL_SHORT_16 index1 = 0, max_interval_data = 0;

	/* Read the current interval index and count for the core */
	if(PM_DATA_15_MIN == data_info)
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;
		max_interval_data = MAX_INTERVAL_DATA;
	}
	else
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;
		max_interval_data = MAX_INTERVAL_24HRS_DATA;
	}

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = max_interval_data - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		if(PM_DATA_15_MIN == data_info)
		{
			dateAndTime[count] = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index].pm_date_time;
		}
		else
		{
			dateAndTime[count] = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index].pm_date_time;
		}
		count++;
	}

        return SUCCESS;
}

PAL_INT_32 get_pm_15mins_mux_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
					CurrentPmLinePath_S *ms_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_15mins_Pm_data_S interval_data;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index];
		ms_data[count] = interval_data.pm_data.muxLine;
		count++;
	}

        return SUCCESS;
}

PAL_INT_32 get_pm_24hrs_mux_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
									CurrentPm24hrsLinePath_S *ms_24hrs_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_24hrs_Pm_data_S interval_data;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Read the current 24 hours data wrt 15mins intervals */
	if(0 == start_int)
	{
		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];
		update_24hrs_mux_current_interval(core,interval_data.pm_data.muxLine, &ms_24hrs_data[0]);

		start_int = 1;
		count = 1;
	}
	else
	{
		count = 0;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_24HRS_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];
		ms_24hrs_data[count] = interval_data.pm_data.muxLine;
		count++;
	}

        return SUCCESS;
}

PAL_INT_32 get_pm_15mins_far_mux_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
					CurrentfarPmLinePath_S *far_ms_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_15mins_Pm_data_S interval_data;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index];
		far_ms_data[count] = interval_data.pm_data.farMuxLine;
		count++;
	}
    return SUCCESS;
}

PAL_INT_32 get_pm_15mins_path_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
										Sdh_Stm1_Pm_Data_E data_info, perf_path_val_S *hp_lp_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_15mins_Pm_data_S interval_data;
	PAL_UCHAR path_index = 0;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index];

		for(path_index = 0;path_index < hp_lp_data[count].path_count;path_index++)
		{
			if(GET_HP_DATA == data_info)
			{
				hp_lp_data[count].path_data[path_index].hl_path_data = \
					interval_data.pm_data.highOrderPath[hp_lp_data[count].path_data[path_index].path_num];
			}
			else
			{
				hp_lp_data[count].path_data[path_index].hl_path_data = \
					interval_data.pm_data.lowOrderPath[hp_lp_data[count].path_data[path_index].path_num];
			}
		}
		count++;
	}
        return SUCCESS;
}

PAL_INT_32 get_pm_24hrs_path_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
							Sdh_Stm1_Pm_Data_E data_info, perf_24hrs_path_val_S *hp_lp_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_24hrs_Pm_data_S interval_data;

	PAL_UCHAR path_index = 0;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Read the current 24 hours data wrt 15mins intervals */
	if(0 == start_int)
	{
		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];
		if(GET_HP_DATA == data_info)
		{
			update_24hrs_h1_current_interval(core,data_info,interval_data.pm_data.highOrderPath, &hp_lp_data[0]);
		}
		else
		{
			update_24hrs_h1_current_interval(core,data_info,interval_data.pm_data.lowOrderPath, &hp_lp_data[0]);
		}

		start_int = 1;
		count = 1;
	}
	else
	{
		count = 0;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_24HRS_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index];

		for(path_index = 0;path_index < hp_lp_data[count].path_count;path_index++)
		{
			if(GET_HP_DATA == data_info)
			{
				hp_lp_data[count].path_24hrs_data[path_index].hl_path_data = \
					interval_data.pm_data.highOrderPath[hp_lp_data[count].path_24hrs_data[path_index].path_num];
			}
			else
			{
				hp_lp_data[count].path_24hrs_data[path_index].hl_path_data = \
					interval_data.pm_data.lowOrderPath[hp_lp_data[count].path_24hrs_data[path_index].path_num];
			}
		}
		count++;
	}
        return SUCCESS;
}

PAL_INT_32 get_pm_path_interval_suspect(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
						pm_user_request_S *pm_request, perf_suspect_path_val_S *hp_lp_suspect)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0;
	PAL_SHORT_16 index1 = 0, max_interval_data = 0;
	PAL_UCHAR path_index = 0;

	/* Read the current interval index and count for the core */
	if(PM_DATA_15_MIN == pm_request->pm_data_type)
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;
		max_interval_data = MAX_INTERVAL_DATA;
	}
	else
	{
		intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalIndex;
		intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.intervalCount;
		max_interval_data = MAX_INTERVAL_24HRS_DATA;
	}

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = max_interval_data - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if(index < 0)
			index = intervalCount;

		switch(pm_request->pm_data_type)
		{
			case PM_DATA_15_MIN:
			{
				CurrentLayerValid_Interval_S interval_data;

				interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index].valid_info;
				for(path_index = 0;path_index < hp_lp_suspect[count].path_count;path_index++)
				{
					if(GET_HP_SUSPECT_DATA == pm_request->pm_data_section)
					{
						hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
							interval_data.highOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
					}
					else if(GET_LP_SUSPECT_DATA == pm_request->pm_data_section)
					{
						hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
							interval_data.lowOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
					}
					else if(GET_FAR_HP_SUSPECT_DATA == pm_request->pm_data_section)
					{
						hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
							interval_data.farHighOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
					}
					else
					{
						hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
							interval_data.farLowOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
					}

				}
				break;
			}
			case PM_DATA_24_HOUR:
			{
				Current24hrsLayerValid_Interval_S interval_data;

				interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_24hrs_data.interval[index].valid_info;
				if(GET_HP_SUSPECT_DATA == pm_request->pm_data_section)
				{
					hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
						interval_data.highOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
				}
				else if(GET_LP_SUSPECT_DATA == pm_request->pm_data_section)
				{
					hp_lp_suspect[count].path_data[path_index].path_suspect_data = \
						interval_data.lowOrderPath[hp_lp_suspect[count].path_data[path_index].path_num];
				}
				else
				{
					printf(" %s : 24hrs pm_data_type %d not (FAR) supported\n", __FUNCTION__, pm_request->pm_data_type);
				}
				break;
			}
		}

		count++;
	}
        return SUCCESS;
}

PAL_INT_32 get_pm_15mins_far_path_interval_data(PAL_INT_32 core, PAL_INT_32 start_int,PAL_INT_32 max_count,\
										Sdh_Stm1_Pm_Data_E data_info, perf_far_path_val_S *hp_lp_data)
{
	PAL_SHORT_16 intervalIndex = 0,intervalCount = 0, index = 0, count = 0, index1 = 0;
	Sdh_15mins_Pm_data_S interval_data;
	PAL_UCHAR path_index = 0;

	/* Read the current interval index and count for the core */
	intervalIndex = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalIndex;
	intervalCount = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.intervalCount;

	if(start_int > intervalCount)
	{
		return FAILURE;
	}

	/* Identify the interval index for the start interval number provided by user */
	if((start_int > intervalIndex) && (intervalIndex != intervalCount))
	{
		index1 = start_int - intervalIndex;
		index1 = MAX_INTERVAL_DATA - index1;
	}
	else
	{
		index1 = intervalIndex - start_int;
	}

	count = 0;
	for(index = index1;count < max_count;index--)
	{
		if (index<0)
			index = intervalCount;

		interval_data = sdh_pm_data_buffer.ptr_sdh_pm_data[core].pm_15mins_data.interval[index];

		for(path_index = 0;path_index < hp_lp_data[count].path_count;path_index++)
		{
			if(GET_HP_DATA == data_info)
			{
				hp_lp_data[count].far_path_data[path_index].far_hl_path_data = \
					interval_data.pm_data.farHighOrderPath[hp_lp_data[count].far_path_data[path_index].path_num];
			}
			else
			{
				hp_lp_data[count].far_path_data[path_index].far_hl_path_data = \
					interval_data.pm_data.farLowOrderPath[hp_lp_data[count].far_path_data[path_index].path_num];
			}
		}
		count ++;
	}
    return SUCCESS;
}

PAL_INT_32 pm_ms_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_INT_32 row_index = 0;

	switch(pm_request->pm_data_type)
	{
		case PM_DATA_15_MIN:
		{
			ModAPISTM1PerfMS_S *ptr_ms_data = pm_request->pm_databuffer;
			perf_mux_val_S 	   *ptr_mux_interval = NULL;

			for(row_index = 0;row_index < ptr_ms_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_ms_data->mux_interval[row_index].card_num, \
								ptr_ms_data->mux_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_ms_data->mux_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_mux_interval = &ptr_ms_data->mux_interval[row_index].perf_data;

				interval_index = ptr_ms_data->mux_interval[row_index].start_interval;

				/* Read the current MS performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_ms_data->mux_interval[row_index].card_num, \
							ptr_ms_data->mux_interval[row_index].core_num,core_index,GET_MS_DATA,NULL);
				}

				get_pm_15mins_mux_interval_data(core_index,interval_index,ptr_mux_interval->count,\
																	ptr_mux_interval->mux_data);
			}

			break;
		}
		case PM_DATA_24_HOUR:
		{
			ModAPISTM1Perf24hrsMS_S *ptr_ms_24hrs_data = pm_request->pm_databuffer;
			perf_24hrs_mux_val_S 	   *ptr_mux_24hrs_interval = NULL;

			for(row_index = 0;row_index < ptr_ms_24hrs_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_ms_24hrs_data->mux_24hrs_interval[row_index].card_num, \
								ptr_ms_24hrs_data->mux_24hrs_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_ms_24hrs_data->mux_24hrs_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_mux_24hrs_interval = &ptr_ms_24hrs_data->mux_24hrs_interval[row_index].perf_24hrs_data;

				interval_index = ptr_ms_24hrs_data->mux_24hrs_interval[row_index].start_interval;

				/* Read the current MS performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_ms_24hrs_data->mux_24hrs_interval[row_index].card_num, \
							ptr_ms_24hrs_data->mux_24hrs_interval[row_index].core_num,core_index,GET_MS_DATA,NULL);
				}

				get_pm_24hrs_mux_interval_data(core_index,interval_index,ptr_mux_24hrs_interval->count,\
																	ptr_mux_24hrs_interval->mux_data);
			}
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data type (%d) request \n", __FUNCTION__, pm_request->pm_data_type);
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_far_ms_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	switch(pm_request->pm_data_type)
	{
		case PM_DATA_15_MIN:
		{
			ModAPISTM1PerfFarMux_S *ptr_far_ms_data = pm_request->pm_databuffer;
			perf_far_mux_val_S 	   *ptr_far_mux_interval = NULL;

			for(row_index = 0;row_index < ptr_far_ms_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_far_ms_data->far_mux_interval[row_index].card_num, \
								ptr_far_ms_data->far_mux_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_far_ms_data->far_mux_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_far_mux_interval = &ptr_far_ms_data->far_mux_interval[row_index].far_perf_data;

				interval_index = ptr_far_ms_data->far_mux_interval[row_index].start_interval;

				/* Read the current Far Mux performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_far_ms_data->far_mux_interval[row_index].card_num, \
							ptr_far_ms_data->far_mux_interval[row_index].core_num,core_index,GET_MS_FAR_DATA,\
							NULL);
				}

				get_pm_15mins_far_mux_interval_data(core_index,interval_index,ptr_far_mux_interval->count,\
																	ptr_far_mux_interval->far_mux_data);
			}

			break;
		}
		case PM_DATA_24_HOUR:
		{
			printf(" %s: Far end data are not supported for 24 hours \n", __FUNCTION__);
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data type (%d) request \n", __FUNCTION__, pm_request->pm_data_type);
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_hp_lp_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	switch(pm_request->pm_data_type)
	{
		case PM_DATA_15_MIN:
		{
			ModAPISTM1PerfHP_LP_S *ptr_hp_lp_data = pm_request->pm_databuffer;
			perf_path_info_S 	   *ptr_hp_lp_interval = NULL;

			for(row_index = 0;row_index < ptr_hp_lp_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_hp_lp_data->path_interval[row_index].card_num, \
								ptr_hp_lp_data->path_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_hp_lp_data->path_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_hp_lp_interval = &ptr_hp_lp_data->path_interval[row_index].perf_data;

				interval_index = ptr_hp_lp_data->path_interval[row_index].start_interval;

				/* Read the current HP/LP performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_hp_lp_data->path_interval[row_index].card_num, \
							ptr_hp_lp_data->path_interval[row_index].core_num,core_index,pm_request->pm_data_section,\
							&ptr_hp_lp_interval->path_info[0]);
				}

				get_pm_15mins_path_interval_data(core_index,interval_index,ptr_hp_lp_interval->count,\
											pm_request->pm_data_section,ptr_hp_lp_interval->path_info);
			}

			break;
		}
		case PM_DATA_24_HOUR:
		{
			ModAPISTM1Perf24hrsHP_LP_S *ptr_24hrs_hp_lp_data = pm_request->pm_databuffer;
			perf_24hrs_path_info_S 	   *ptr_24hrs_hp_lp_interval = NULL;

			for(row_index = 0;row_index < ptr_24hrs_hp_lp_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_24hrs_hp_lp_data->path_interval[row_index].card_num, \
								ptr_24hrs_hp_lp_data->path_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_24hrs_hp_lp_data->path_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_24hrs_hp_lp_interval = &ptr_24hrs_hp_lp_data->path_interval[row_index].perf_24hrs_data;

				interval_index = ptr_24hrs_hp_lp_data->path_interval[row_index].start_interval;

				/* Read the current HP/LP performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_24hrs_hp_lp_data->path_interval[row_index].card_num, \
							ptr_24hrs_hp_lp_data->path_interval[row_index].core_num,core_index,pm_request->pm_data_section,\
							&ptr_24hrs_hp_lp_interval->path_24hrs_info[0]);
				}

				get_pm_24hrs_path_interval_data(core_index,interval_index,ptr_24hrs_hp_lp_interval->count,\
											pm_request->pm_data_section,ptr_24hrs_hp_lp_interval->path_24hrs_info);
			}
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data type (%d) request \n", __FUNCTION__, pm_request->pm_data_type);
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_hp_lp_suspect_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	ModAPISTM1PerfSuspectHP_LP_S *ptr_hp_lp_suspect = pm_request->pm_databuffer;
	perf_suspect_path_into_S 	   *ptr_hp_lp_interval = NULL;

	for(row_index = 0;row_index < ptr_hp_lp_suspect->num_rows;row_index++)
	{
		core_index = get_pm_buffer_core_index(ptr_hp_lp_suspect->suspect_info[row_index].card_num, \
						ptr_hp_lp_suspect->suspect_info[row_index].core_num);

		if(FAILURE == core_index)
		{
			ptr_hp_lp_suspect->suspect_info[row_index].status = FAILURE;
			printf(" %s :  Invalid card number \n", __FUNCTION__);
		}

		ptr_hp_lp_interval = &ptr_hp_lp_suspect->suspect_info[row_index].perf_data;

		interval_index = ptr_hp_lp_suspect->suspect_info[row_index].start_interval;

		get_pm_path_interval_suspect(core_index,interval_index,ptr_hp_lp_interval->count,\
									pm_request,ptr_hp_lp_interval->path_int_info);
	}

	return ret_val;
}

PAL_INT_32 pm_far_hp_lp_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	switch(pm_request->pm_data_type)
	{
		case PM_DATA_15_MIN:
		{
			ModAPISTM1PerfFarHP_LP_S *ptr_far_hp_lp_data = pm_request->pm_databuffer;
			perf_far_path_info_S 	   *ptr_far_hp_lp_interval = NULL;

			for(row_index = 0;row_index < ptr_far_hp_lp_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_far_hp_lp_data->far_path_interval[row_index].card_num, \
								ptr_far_hp_lp_data->far_path_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_far_hp_lp_data->far_path_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_far_hp_lp_interval = &ptr_far_hp_lp_data->far_path_interval[row_index].far_perf_data;

				interval_index = ptr_far_hp_lp_data->far_path_interval[row_index].start_interval;

				/* Read the current Far HP/LP performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_far_hp_lp_data->far_path_interval[row_index].card_num, \
							ptr_far_hp_lp_data->far_path_interval[row_index].core_num,core_index,\
							pm_request->pm_data_section,\
							&ptr_far_hp_lp_interval->far_path_info[0]);
				}

				get_pm_15mins_far_path_interval_data(core_index,interval_index,ptr_far_hp_lp_interval->count,\
											pm_request->pm_data_section,ptr_far_hp_lp_interval->far_path_info);
			}

			break;
		}
		case PM_DATA_24_HOUR:
		{
			printf(" %s: Far end data are not supported for 24 hours \n", __FUNCTION__);
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data type (%d) request \n", __FUNCTION__, pm_request->pm_data_type);
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_rs_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	switch(pm_request->pm_data_type)
	{
		case PM_DATA_15_MIN:
		{
			ModAPISTM1PerfRS_S *ptr_rs_data = pm_request->pm_databuffer;
			perf_rs_val_S 	   *ptr_rs_interval = NULL;

			for(row_index = 0;row_index < ptr_rs_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_rs_data->rs_interval[row_index].card_num, \
								ptr_rs_data->rs_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_rs_data->rs_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_rs_interval = &ptr_rs_data->rs_interval[row_index].perf_data;

				interval_index = ptr_rs_data->rs_interval[row_index].start_interval;

				/* Read the current RS performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_rs_data->rs_interval[row_index].card_num, \
							ptr_rs_data->rs_interval[row_index].core_num,core_index,GET_RS_DATA,NULL);
				}

				get_pm_15mins_rs_interval_data(core_index,interval_index,ptr_rs_interval->count,\
																	ptr_rs_interval->rs_data);
			}

			break;
		}
		case PM_DATA_24_HOUR:
		{
			ModAPISTM1Perf24hrsRS_S *ptr_24hrs_rs_data = pm_request->pm_databuffer;
			perf_24hrs_rs_val_S 	*ptr_rs_24hrs_interval = NULL;

			for(row_index = 0;row_index < ptr_24hrs_rs_data->num_rows;row_index++)
			{
				core_index = get_pm_buffer_core_index(ptr_24hrs_rs_data->rs_24hrs_interval[row_index].card_num, \
								ptr_24hrs_rs_data->rs_24hrs_interval[row_index].core_num);

				if(FAILURE == core_index)
				{
					ptr_24hrs_rs_data->rs_24hrs_interval[row_index].status = FAILURE;
					printf(" %s :  Invalid card number \n", __FUNCTION__);
				}

				ptr_rs_24hrs_interval = &ptr_24hrs_rs_data->rs_24hrs_interval[row_index].perf_24hrs_data;

				interval_index = ptr_24hrs_rs_data->rs_24hrs_interval[row_index].start_interval;

				/* Read the current RS performance data */
				if(0 == interval_index)
				{
					pml_read_current_interval_data(ptr_24hrs_rs_data->rs_24hrs_interval[row_index].card_num, \
							ptr_24hrs_rs_data->rs_24hrs_interval[row_index].core_num,core_index,GET_RS_DATA,NULL);
				}

				get_pm_24hrs_rs_interval_data(core_index,interval_index,ptr_rs_24hrs_interval->count,\
																	ptr_rs_24hrs_interval->rs_data);
			}
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data type (%d) request \n", __FUNCTION__, pm_request->pm_data_type);
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_rs_ms_suspect_interval_data_read(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	ModAPISTM1PerfMuxSupect_S *ptr_rs_mux_suspect_data = pm_request->pm_databuffer;
	perf_SuspectData_S 	   *ptr_rs_mux_suspect = NULL;

	for(row_index = 0;row_index < ptr_rs_mux_suspect_data->num_rows;row_index++)
	{
		core_index = get_pm_buffer_core_index(ptr_rs_mux_suspect_data->suspect_info[row_index].card_num, \
						ptr_rs_mux_suspect_data->suspect_info[row_index].core_num);

		if(FAILURE == core_index)
		{
			ptr_rs_mux_suspect_data->suspect_info[row_index].status = FAILURE;
			printf(" %s :  Invalid card number \n", __FUNCTION__);
		}

		ptr_rs_mux_suspect = &ptr_rs_mux_suspect_data->suspect_info[row_index].perf_suspect_info;

		interval_index = ptr_rs_mux_suspect_data->suspect_info[row_index].start_interval;

		get_pm_rs_mux_suspect_data(core_index,interval_index,ptr_rs_mux_suspect->count,\
				pm_request,ptr_rs_mux_suspect->suspect_data);
	}

	return ret_val;
}

PAL_INT_32 pm_interval_date_and_time(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	PAL_USHORT_16  interval_index = 0;
	PAL_UINT_32 row_index = 0;

	ModAPISTM1PerfDateTime_S *ptr_interval_dateTime = pm_request->pm_databuffer;
	perf_dateTime_S 	   *ptr_dateTime_data = NULL;

	for(row_index = 0;row_index < ptr_interval_dateTime->num_rows;row_index++)
	{
		core_index = get_pm_buffer_core_index(ptr_interval_dateTime->interval_time[row_index].card_num, \
						ptr_interval_dateTime->interval_time[row_index].core_num);

		if(FAILURE == core_index)
		{
			ptr_interval_dateTime->interval_time[row_index].status = FAILURE;
			printf(" %s :  Invalid card number \n", __FUNCTION__);
		}

		ptr_dateTime_data = &ptr_interval_dateTime->interval_time[row_index].perf_time;

		interval_index = ptr_interval_dateTime->interval_time[row_index].start_interval;

		get_pm_interval_dateandtime(core_index,interval_index,ptr_dateTime_data->count,\
								pm_request->pm_data_type,ptr_dateTime_data->start_time);
	}
	return ret_val;
}

PAL_INT_32 get_15min_interval_data(ModAPISTM1Perf15minsData_S *ptr_15mins_data)
{
	PAL_INT_32 ret_val = SUCCESS;
	pm_user_request_S  temp_pm_request;

	temp_pm_request.pm_data_type = PM_DATA_15_MIN;
	temp_pm_request.pm_select_info = STM1_PERFORMANCE_DATA;

	temp_pm_request.pm_data_section = GET_RS_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->rs_data;
	ret_val = pm_read_interval_data(&temp_pm_request);
    

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read RS section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_MS_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->ms_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read MS section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_HP_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->hp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read HP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_LP_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->lp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read LP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_MS_FAR_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_ms_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far MS section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_HP_FAR_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_hp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far HP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_LP_FAR_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_lp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far LP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_RS_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->rs_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read RS section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_MS_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->mux_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read MS section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_HP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->hp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read HP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_LP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->lp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read LP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_FAR_MS_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_mux_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far MS section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_FAR_HP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_hp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far HP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_FAR_LP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->far_lp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Far LP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_DATE_AND_TIME_DATA;
	temp_pm_request.pm_databuffer = &ptr_15mins_data->interval_start_time;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Date and Time Info \n", __FUNCTION__);
	}

	return ret_val;
}



PAL_INT_32 get_24hrs_interval_data(ModAPISTM1Perf24hrsData_S *ptr_24hrs_data)
{
	PAL_INT_32 ret_val = SUCCESS;
	pm_user_request_S  temp_pm_request;

	temp_pm_request.pm_data_type = PM_DATA_24_HOUR;
	temp_pm_request.pm_select_info = STM1_PERFORMANCE_DATA;

	temp_pm_request.pm_data_section = GET_RS_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->rs_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read RS section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_MS_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->ms_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read MS section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_HP_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->hp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read HP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_LP_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->lp_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read LP section data \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_RS_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->rs_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read RS section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_MS_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->mux_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read MS section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_HP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->hp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read HP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_LP_SUSPECT_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->lp_suspect_data;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read LP section suspect \n", __FUNCTION__);
	}

	temp_pm_request.pm_data_section = GET_DATE_AND_TIME_DATA;
	temp_pm_request.pm_databuffer = &ptr_24hrs_data->interval_start_time;
	ret_val = pm_read_interval_data(&temp_pm_request);

	if(FAILURE == ret_val)
	{
		printf("%s : Failed to read Date and Time Info \n", __FUNCTION__);
	}

	return ret_val;
}

PAL_INT_32 pm_interval_data_read_complete(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = 0;
	if(PM_DATA_15_MIN == pm_request->pm_data_type)
	{
		ModAPISTM1Perf15minsData_S  *ptr_15mins_data = pm_request->pm_databuffer;

		ret_val = get_15min_interval_data(ptr_15mins_data);
	}
	else
	{
		ModAPISTM1Perf24hrsData_S  *ptr_24hrs_data = pm_request->pm_databuffer;

		ret_val = get_24hrs_interval_data(ptr_24hrs_data);
	}

	return ret_val;
}

/* Performance Action Task related functions */
PAL_INT_32 pm_read_interval_data(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS;

	switch(pm_request->pm_data_section)
	{
		case GET_RS_DATA:
		{
			ret_val = pm_rs_interval_data_read(pm_request);
			break;
		}
		case GET_MS_DATA:
		{
			ret_val = pm_ms_interval_data_read(pm_request);
			break;
		}
		case GET_HP_DATA:
		case GET_LP_DATA:
		{
			ret_val = pm_hp_lp_interval_data_read(pm_request);
			break;
		}
		case GET_MS_FAR_DATA:
		{
			ret_val = pm_far_ms_interval_data_read(pm_request);
			break;
		}
		case GET_HP_FAR_DATA:
		case GET_LP_FAR_DATA:
		{
			ret_val = pm_far_hp_lp_interval_data_read(pm_request);
			break;
		}
		case GET_RS_SUSPECT_DATA:
		case GET_MS_SUSPECT_DATA:
		case GET_FAR_MS_SUSPECT_DATA:
		{
			ret_val = pm_rs_ms_suspect_interval_data_read(pm_request);
			break;
		}
		case GET_HP_SUSPECT_DATA:
		case GET_LP_SUSPECT_DATA:
		case GET_FAR_HP_SUSPECT_DATA:
		case GET_FAR_LP_SUSPECT_DATA:
		{
			ret_val = pm_hp_lp_suspect_interval_data_read(pm_request);
			break;
		}
		case GET_DATE_AND_TIME_DATA:
		{
			ret_val = pm_interval_date_and_time(pm_request);
			break;
		}
		case GET_ALL_DATA:
		{
			ret_val = pm_interval_data_read_complete(pm_request);
			break;
		}
		default:
		{
			printf(" %s : Invalid PM data selection (%d) request \n", __FUNCTION__, pm_request->pm_data_section);
			ret_val = FAILURE;
			break;
		}
	}
	return ret_val;
}

PAL_INT_32 pm_read_interval_info(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS, core_index = 0;
	//unsigned short  interval_index = 0; //Pramod WARNGS
	unsigned int row_index = 0;

	ModAPISTM1PerfIntervalInfo_S *ptr_interval_info = pm_request->pm_databuffer;

	for(row_index = 0;row_index < ptr_interval_info->num_rows;row_index++)
	{
		core_index = get_pm_buffer_core_index(ptr_interval_info->interval_info[row_index].card_num, \
						ptr_interval_info->interval_info[row_index].core_num);

		if(FAILURE == core_index)
		{
			ptr_interval_info->interval_info[row_index].status = FAILURE;
			printf(" %s :  Invalid card number \n", __FUNCTION__);
		}

		if(PM_DATA_15_MIN == pm_request->pm_data_type)
		{
			ptr_interval_info->interval_info[row_index].invalidIntervals = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_15mins_data.invalidIntervals;
			ptr_interval_info->interval_info[row_index].validIntervals = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_15mins_data.validIntervals;
			ptr_interval_info->interval_info[row_index].intervalCount = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_15mins_data.intervalCount;
		}
		else
		{
			ptr_interval_info->interval_info[row_index].invalidIntervals = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_24hrs_data.invalidIntervals;
			ptr_interval_info->interval_info[row_index].validIntervals = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_24hrs_data.validIntervals;
			ptr_interval_info->interval_info[row_index].intervalCount = \
				sdh_pm_data_buffer.ptr_sdh_pm_data[core_index].pm_24hrs_data.intervalCount;
		}
	}

	return ret_val;
}

PAL_INT_32 pml_sdh_stm1_perf_request(pm_user_request_S *pm_request)
{
	PAL_INT_32 ret_val = SUCCESS;

	switch(pm_request->pm_select_info)
	{
		case STM1_INTERVAL_INFO:
		{
			ret_val = pm_read_interval_info(pm_request);
			break;
		}
		case STM1_PERFORMANCE_DATA:
		{
			ret_val = pm_read_interval_data(pm_request);
			break;
		}
		default:
		{
			printf(" %s : Invalid PM selection info(%d ) request \n", __FUNCTION__, pm_request->pm_select_info);
			ret_val = FAILURE;
			break;
		}
	}
	return ret_val;
}

/* OLD Impleementations for PM */

PAL_INT_32 pml_sdh_stm1_cleanup(PAL_VOID)
{
	if(NULL != sdh_pm_data_buffer.ptr_sdh_card_info)
	{
		free(sdh_pm_data_buffer.ptr_sdh_card_info);
	}

	if(NULL == sdh_pm_data_buffer.ptr_sdh_pm_data)
	{
		free(sdh_pm_data_buffer.ptr_sdh_pm_data);
	}

	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_data_xml_get(Ui_If_Pm_Data_S *pm_request)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_far_end_interval_span_data_get(Ui_If_Pm_Data_S *pm_request)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_far_end_current_data_get(Ui_If_Pm_Data_S *pm_request)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_far_end_data_xml_get(Ui_If_Pm_Data_S *pm_request)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_pm_update_dt_interval_data(LogPerf_dateTime_S *sdh_start_interval_dateTime)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 get_current_perf_notify_status(PAL_CHAR_8 *stm1_perf_interval_status)
{
	/* TBD */
	return SUCCESS;
}

PAL_INT_32 pml_sdh_stm1_adm_pm_config_set(Sdh_Stm1_Pm_Status_E pm_stat, Sdh_Stm1_Pm_Config_Type_E pm_config)
{
	/* TBD */

	return SUCCESS;
}
