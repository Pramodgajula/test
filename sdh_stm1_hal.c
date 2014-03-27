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
 #include "ces_common.h"
 #include "ces_sockets.h"

/*module includes*/
 #include "pal_lib.h"
 #include "sdh_stm1_cel.h"
 #include "sdh_stm1_config_int_ext_lib.h"
 #include "../core/include/sdh_stm1_enums.h"
 #include "sdh_stm1_hal.h"
 #include "sdh_stm1_hil.h"
 #include "../core/include/sdh_stm1_pm_data_struct.h"
 #include "sdh_stm1_adm_register_info.h"
 #include "sdh_stm1_hal_liu.h"

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/
static PAL_UCHAR	 stm_table[MAX_STM1_ADM_PORT];
int apb_fd = FAILURE;
int *b_access = NULL;
int access_cnt = 0;

/* create the Stm1 register values */
#define PARM_INIT(id, addr, datasize, access_type, default_val, status) {addr, datasize, access_type, default_val, status},
Stm_Mux_Demux_Info_S	stm_mux_demux_info[] = { STM1_MUX_DEMUX_REG_PARAM_LIST };
#undef PARM_INIT


#define SDH_STM1_LIU_INIT(liu_index, liu1_fn, liu2_fn)   {liu1_fn,liu2_fn},

/* Array to store the STM1 callback functions for LIUs supported */
stm1_liu_fn_Ptr_S sdh_stm1_liu_cfg[] = {SDH_STM1_LIU_FN_LIST};

#undef SDH_STM1_LIU_INIT

/******************************************************************************
 * FUNCTION PROTOTYPE
 *****************************************************************************/

/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/

 /*******************************************************************************
 *	Description:	Open the '/dev/APB' character device
 *	Inputs:		 	Device name '/dev/APB'
 *	Outputs:		Returns failure or file descriptor
 ********************************************************************************/
 PAL_INT_32 open_apb_device(PAL_VOID)
 {
 	PAL_INT_32 fd;

 	#ifndef DEVICE_TEST
 	fd = open(MOUNTAPB, O_RDWR);
 	if (fd < 0)
 	{
 		printf("Error in file open\n");
 		return FAILURE;
 	}
 	else
 	{
 		printf("ABP open successfully\n");
 		return fd;
 	}
 	#else
 		return SUCCESS;
 	#endif
 }

 /*******************************************************************************
 *	Description:	Close the '/dev/APB' character device
 *	Inputs:		 	Device name '/dev/APB'
 *	Outputs:		Returns failure or Success
 ********************************************************************************/
 PAL_INT_32 close_apb_device(PAL_INT_32 fd)
 {
 	#ifndef DEVICE_TEST
 	if (close(fd) < 0)
 	{
 		return FAILURE;
 	}
 	else
 	{
 		printf("APB Close successfully\n");
 		return SUCCESS;
 	}
 	#else
 		return SUCCESS;
 	#endif
 }

 /****************************************************************************
  *	Description:This Function cleamup the memory allocated by this Layer
  *
  *	Inputs:	port number
  *
  *	Outputs:	Returns failure or Success
 ****************************************************************************/
 PAL_INT_32 hal_sdh_stm1_cleanup(PAL_VOID)
 {
 	if(sdh_db.core_set_parms != NULL)
 	{
 		free(sdh_db.core_set_parms);
 		sdh_db.core_set_parms = NULL;
 	}
 	if(NULL != b_access)
 		free(b_access);
 }
 /******************************************************************************
  * Description :This function will be called to access the apb device driver
  *				It will open the apb device is not already opened by the
  *				module and after accessing the apb device it will close the
  *				apb device. If abp device is already opened, then it will only
  *				access the apb device.
  *
  * Inputs      :command No, arguments for abp driver and apb device
  *				status
  *
  * Outputs     :Returns success or failure.
  ******************************************************************************/
 PAL_INT_32 apb_access(PAL_UINT_32 cmd, PAL_ULONG_32 arg,	PAL_INT_32 fd)
 {
 	PAL_INT_32 retval = FAILURE;
 	PAL_INT_32 devicestatus = 0;

 	#ifndef DEVICE_TEST
 	/* if framer device file is already opened then this flag will be one */
 	if(fd == DEVICE_NOT_OPENED)
 	{
 		fd = open_apb_device();
 		if (fd < 0)
 		{
 			return FAILURE;
 		}
 		devicestatus = 1;
 	}
 	retval = ioctl(fd, cmd, arg);
 	if(devicestatus == 1)
 	{
 		if(close_apb_device(fd) < 0)
 		{
 			return FAILURE;
 		}
 	}
 	#else
 		return SUCCESS;
 	#endif
 	return retval;
 }

/******************************************************************************
 * Description : Converts a ST port number to its corresponding KLM string .
 *				A lookup table is used to convert the port numbers for STM.
 *
 *
 * Inputs	  : port_no - port number to be converted
 *			    str -  the converted string
 *				e1t1_typ1- 3 for E1 ;4 for T1
 *				stm_table - lookup table to convert ST port to KLM nos
 *
 *
 * Outputs    :	0 on success
 *				-ve error code on failure
 ******************************************************************************/
PAL_INT_32 hal_convert_port_to_klm(PAL_INT_32 port_no ,PAL_UCHAR *str)
{

	if(str == NULL )
		return -1;


	if((port_no >= 0 ) && (port_no < MAX_STM1_ADM_PORT )) /*port validity check*/
	{
		*str = stm_table[port_no];

	}
	else
		return -1;


	return 0;
}

 /*******************************************************************************
 * Description:This function will generate the STM1 Table.
 *
 * Inputs:     Stm1_Card_Manager_S
 * Outputs:    N/A.
 ********************************************************************************/
 PAL_INT_32 hal_sdh_stm1_generate_stm_table(Stm1_Card_Manager_S sdh_stm1_hw_info)
 {
 	unsigned char  k,l,m;
 	int i=0;
 	int type=0, no_lower_order_paths=0;

 	if(sdh_stm1_hw_info.max_m_field == 4)
 	{
 		type=3;
 		/* PD 24/Jan/2006 - No. of paths in T1 mode is 84 */
 		no_lower_order_paths = MAX_LOW_ORDER_PATHS_T1;
 	}
 	else
 	{
 		type=2;
 		/* PD 24/Jan/2006 - No. of paths in T1 mode is 64 */
 		no_lower_order_paths = MAX_LOW_ORDER_PATHS_E1;
 	}

 	memset(stm_table,0,sizeof(unsigned char) *MAX_STM1_ADM_PORT);

 	for(k = 0; k <= MAX_K_FIELD ; k++) /*for all Ks*/

 		for(l = 0; l <= MAX_L_FIELD ; l++) /*for all Ls*/

 			for(m = 0; m <= type ; m++)	/*for all Ms :m max is 2 for E1 and 3 for T1*/
 			{
 				/* bits 6-5 :k
 					bits 4-2 : l
 					bits 1-0 : m
 				*/
 				  stm_table[i] = 0;
 				  stm_table[i] = (k << 5 );
 				  stm_table[i] |= (l << 2);
 				  stm_table[i] |= m;

 				i++;
 			}
 	return i;
 }
 /*******************************************************************************
 * Description:This function will initialize the parameters on bootup.
 *
 * Inputs:     N/A
 * Outputs:    APB Device descriptor, Memory pointer for writing and reading data.
 ********************************************************************************/
 PAL_INT_32 hal_sdh_stm1_access_init(Stm1_Card_Manager_S	sdh_stm1_hw_info)
 {
 	if((b_access = malloc(sizeof(PAL_INT_32) *(512+1)))<0)
 	{
 		return FAILURE;
 	}
 	if((apb_fd	= open_apb_device())<0)
 	{
 		return FAILURE;
 	}

 	hal_sdh_stm1_liu_init();

 	hal_sdh_stm1_generate_stm_table(sdh_stm1_hw_info);

 	return SUCCESS;
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
PAL_INT_32 hal_sdh_stm1_alloc_mem(Stm1_Sdh_Hw_Max_Interface_S block_info, PAL_INT_32 config)
{
	printf(" Entered %s \n", __FUNCTION__);
	switch(config)
	{
		case PORT_CONFIG:
		{
			int max_stm1_ports = 0, index = 0;

			for(index = 0;index < block_info.card_num;index++)
			{
				max_stm1_ports +=  block_info.interface_info[index].max_stm1_ports;
			}
			hal_sdh_stm1_liu_mem_alloc(max_stm1_ports);
		}
		break;
		case CORE_CONFIG:
		{
			int max_stm1_cores = 0, index = 0;

			for(index = 0;index < block_info.card_num;index++)
			{
				max_stm1_cores +=  block_info.interface_info[index].max_sdh_cores;
			}
			if(sdh_db.core_set_parms != NULL)
			{
				free(sdh_db.core_set_parms);
			}

			sdh_db.core_set_parms = \
				malloc(sizeof(Stm1_Core_Set_S)*max_stm1_cores);
		}
		break;
		default:
		break;
	}

	printf(" *** Leaving %s \n", __FUNCTION__);
	return SUCCESS;
}
 /*******************************************************************************
 * Description:This function will read the register from hardware through ABP
 *			  interface.
 *
 * Inputs:     read address, read data, access value
 * Outputs:    N/A
 ********************************************************************************/
 static PAL_INT_32 hw_apb_bulk_read(PAL_INT_32 i_addr, PAL_INT_32* i_data, PAL_INT_32 size)
 {
 	PAL_INT_32 cnt = 0,num_of_2k_chunk=0,last_size=0;
 	PAL_INT_32 retval = FAILURE, index=0;

 	/* Convert number of bytes to number of integers to read b/c the driver will be reading
 	 * the data in the chunk of inegers(4 bytes at a time).
 	 */

 	if(size>0)
 	{
 		num_of_2k_chunk = (size >= 2048)? (size/2048) : 0;
 		last_size = size % 2048;

 		do{
 			if(index == num_of_2k_chunk  )
 				access_cnt = last_size/4;
 			else
 			/* 512 means 2048 bytes b/c 'access_cnt' expects number of integers to read (not number of bytes, so 2048/4 = 512). */
 				access_cnt = 512;

 			b_access[0] = access_cnt+1;
 			b_access[2]	= i_addr;
 			if((retval  = apb_access(APB_BULK_READ, (unsigned long)b_access, apb_fd))<0)
 			{
 				return retval;
 			}
 			else
 			{
 				/* Copy the pm data to buffer */
 				memcpy(&i_data[512*index],&b_access[3],sizeof(int)*access_cnt);
 				return SUCCESS;
 			}
 			index++;
 		}while(num_of_2k_chunk < index );
 	}
 	return SUCCESS;
 }

 /*******************************************************************************
 * Description:This function will read the register from hardware through ABP
 *			  interface after writing specfic register.
 *
 * Inputs:     write address, write data, read address, access value
 * Outputs:    read data
 ********************************************************************************/
 static PAL_INT_32 hw_apb_read_after_write(PAL_INT_32 i_addr_w, PAL_INT_32 i_data_w, PAL_INT_32 i_addr_r, PAL_INT_32 *i_data_r, PAL_INT_32 *access)
 {
 	PAL_INT_32 cnt = 0;
 	PAL_INT_32 retval = FAILURE;

 	#ifdef INDIVIDUAL_ACCESS
 		b_access[0] = access_cnt+1;
 		b_access[4]	= i_addr_w;
 		b_access[5]	= i_data_w;
 		b_access[6]	= i_addr_r;
 		if((retval = apb_access(APB_READ_AFTER_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			return retval;
 		}
 		else
 		{
 			i_data_r[0] = b_access[7];
 			return SUCCESS;
 		}
 	#else
 	if((access_cnt>=128) || (*access == 1))
 	{
 		if((retval = apb_access(APB_READ_AFTER_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			access_cnt = 0;
 			*access = access_cnt;
 			return retval;
 		}
 		else
 		{
 			for(cnt = 0; cnt<access_cnt; access_cnt++)
 			{
 				i_data_r[cnt] = b_access[((cnt+1)*4)+3];
 			}

 			if(*access != 1)
 			{
 				*access = access_cnt;
 				access_cnt = 0;
 				b_access[0] = access_cnt+1;
 				b_access[(b_access[0]*4)]	= i_addr_w;
 				b_access[(b_access[0]*4)+1]	= i_data_w;
 				b_access[(b_access[0]*4)+2]	= i_addr_r;
 				access_cnt++;
 			}
 			else
 			{
 				access_cnt = 0;
 			}
 			return SUCCESS;
 		}
 	}
 	else
 	{
 		b_access[0] = access_cnt+1;
 		b_access[(b_access[0]*4)]	= i_addr_w;
 		b_access[(b_access[0]*4)+1]	= i_data_w;
 		b_access[(b_access[0]*4)+2]	= i_addr_r;
 		access_cnt++;
 	}
 	#endif

 	return 1;
 }

 /*******************************************************************************
 * Description:This function will write the register from hardware through ABP
 *			  interface after reading specfic register value and modifying it.
 *
 * Inputs:     write address, write data, read address, read data. access value
 *
 * Outputs:    N/A
 ********************************************************************************/
 static PAL_INT_32 hw_apb_read_modify_write(PAL_INT_32 i_addr_w, PAL_INT_32 i_data_w, PAL_INT_32 i_addr_r, \
 													PAL_INT_32 i_data_r, PAL_UCHAR opt_rmw, PAL_INT_32 *access)
 {
 	PAL_INT_32 cnt = 0;
 	PAL_INT_32 retval = FAILURE;

 	#ifdef INDIVIDUAL_ACCESS
 		b_access[0] = access_cnt+1;
 		b_access[4] = opt_rmw;
 		b_access[5]	= i_addr_r;
 		b_access[6]	= i_addr_w;
 		b_access[7]	= i_data_w;
 		if((retval = apb_access(APB_READ_MODIFY_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			return retval;
 		}
 	#else
 	if(access_cnt>=128 || *access == 1)
 	{
 		if((retval = apb_access(APB_READ_MODIFY_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			access_cnt = 0;
 			*access = access_cnt;
 			return retval;
 		}
 		else
 		{
 			if(*access != 1)
 			{
 				*access = access_cnt;
 				access_cnt = 0;
 				b_access[0] = access_cnt+1;
 				b_access[(b_access[0]*4)] 	= opt_rmw;
 				b_access[(b_access[0]*4)+1]	= i_addr_r;
 				b_access[(b_access[0]*4)+2]	= i_addr_w;
 				b_access[(b_access[0]*4)+3]	= i_data_w;
 				access_cnt++;
 			}
 			else
 			{
 				access_cnt = 0;
 			}
 			return SUCCESS;
 		}
 	}
 	else
 	{
 		b_access[0] = access_cnt+1;
		b_access[(b_access[0]*4)] 	= opt_rmw;
		b_access[(b_access[0]*4)+1]	= i_addr_r;
		b_access[(b_access[0]*4)+2]	= i_addr_w;
		b_access[(b_access[0]*4)+3]	= i_data_w;
 		access_cnt++;
 	}
 	#endif

 	return 1;
 }

 /*******************************************************************************
 * Description:This function will write the register to hardware through ABP
 *			  interface.
 *
 * Inputs:     write address, write data, access value
 * Outputs:    N/A
 ********************************************************************************/
 static PAL_INT_32 hw_apb_write(PAL_INT_32 i_addr, PAL_INT_32 i_data, PAL_INT_32 *access)
 {
 	PAL_INT_32 cnt = 0;
 	PAL_INT_32 retval = FAILURE;

 	printf(" Entered %s \n", __FUNCTION__);
 	#ifdef INDIVIDUAL_ACCESS
 		b_access[0] = access_cnt+1;
 		b_access[2]	= i_addr;
 		b_access[3]	= i_data;
 		printf(" Before the retval section \n");
 		if((retval = apb_access(APB_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			printf(" Entered the retval section \n");
 			return retval;
 		}
 	#else
 	if(access_cnt>=128 || *access == 1)
 	{
 		printf(" %s : apb_access \n", __FUNCTION__);
 		if((retval = apb_access(APB_WRITE, (unsigned long)b_access, apb_fd))<0)
 		{
 			printf(" %s : apb_access failure \n", __FUNCTION__);
 			access_cnt = 0;
 			return retval;
 		}
 		if(*access != 1)
 		{
 			*access = access_cnt;
 			access_cnt = 0;
 			b_access[0] = access_cnt+1;
 			b_access[(b_access[0]*2)]	= i_addr;
 			b_access[(b_access[0]*2)+1]	= i_data;
 			access_cnt++;
 		}
 		else
 		{
 			access_cnt = 0;
 		}
 		return SUCCESS;
 	}
 	else
 	{
 		printf(" %s : b_access  \n", __FUNCTION__);
 		b_access[0] = access_cnt+1;
 		b_access[(b_access[0]*2)]	= i_addr;
 		b_access[(b_access[0]*2)+1]	= i_data;
 		access_cnt++;
 	}
 	#endif
 	printf(" Leaving %s \n", __FUNCTION__);
 	return 1;
 }

 /*******************************************************************************
 * Description:This function will read the register from hardware through ABP
 *			  interface.
 *
 * Inputs:     read address, read data, access value
 * Outputs:    N/A
 ********************************************************************************/
 static PAL_INT_32 hw_apb_read(PAL_INT_32 i_addr, PAL_INT_32 *i_data, PAL_INT_32 *access)
 {
 	PAL_INT_32 cnt = 0;
 	PAL_INT_32 retval = FAILURE;

 	#ifdef INDIVIDUAL_ACCESS
 		b_access[0] = access_cnt+1;
 		b_access[2]	= i_addr;
 		if((retval = apb_access(APB_READ, (unsigned long)b_access, apb_fd))<0)
 		{
 			return retval;
 		}
 		else
 		{
 			i_data[0] = b_access[3];
 			return SUCCESS;
 		}
 	#else
 	if(access_cnt>=128 || *access == 1)
 	{
 		if((retval = apb_access(APB_READ, (unsigned long)b_access, apb_fd))<0)
 		{
 			access_cnt = 0;
 			*access = access_cnt;
 			return retval;
 		}
 		else
 		{
 			for(cnt = 0; cnt<access_cnt; access_cnt++)
 			{
 				i_data[cnt] = b_access[((cnt+1)*2)+1];
 			}

 			if(*access != 1)
 			{
 				*access = access_cnt;
 				access_cnt = 0;
 				b_access[0] = access_cnt+1;
 				b_access[(b_access[0]*2)]	= i_addr;
 				access_cnt++;
 			}
 			else
 			{
 				access_cnt = 0;
 			}
 			return SUCCESS;
 		}
 	}
 	else
 	{
 		b_access[0] = access_cnt+1;
 		b_access[(b_access[0]*2)]	= i_addr;
 		access_cnt++;
 	}
 	#endif

 	return 1;
 }

 static PAL_INT_32 hw_apb_asic_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {

 	PAL_INT_32 retval 		= SUCCESS;

 	PAL_INT_32 apb_addr1 	= sdh_stm1_asic_access->addr1;
 	PAL_INT_32 apb_addr2 	= sdh_stm1_asic_access->addr2;
 	PAL_INT_32 *reg_data1 	= sdh_stm1_asic_access->data1;
 	PAL_INT_32 *reg_data2 	= sdh_stm1_asic_access->data2;
 	Rmw_Opt_E  opt_rmw 		= sdh_stm1_asic_access->opt_rmw;
 	PAL_INT_32 access_info	= sdh_stm1_asic_access->init;

 	switch(sdh_stm1_asic_access->abp_access)
 	{
 		case APB_READ:
 		{
 			retval = hw_apb_read(apb_addr1, reg_data1, &access_info);
 			break;
 		}
 		case APB_WRITE:
 		{
 			retval = hw_apb_write(apb_addr1, *reg_data1, &access_info);
 			break;
 		}
 		case APB_READ_MODIFY_WRITE:
 		{
 			retval = hw_apb_read_modify_write(apb_addr1, *reg_data1, apb_addr1, *reg_data2, opt_rmw, &access_info);
 			break;
 		}
 		case APB_READ_AFTER_WRITE:
 		{
 			retval = hw_apb_read_after_write(apb_addr1, *reg_data1, apb_addr1, reg_data2, &access_info);
 			break;
 		}
 		case APB_BULK_READ:
 		{
 			retval = hw_apb_bulk_read(apb_addr1, reg_data1, access_info);
 			break;
 		}
 		/*case APB_BULK_WRITE:
 		{
 			retval = hw_apb_bulk_write(apb_addr1,reg_data1,access_info);
 			break;
 		}*/
 		default:
 		{
 			break;
 		}
 	}

 	return retval;
 }

 /****************************************************************************
  *	Description:Get/Set the register present in Layer 7 or 8
  *
  *	Inputs:		Ptr to sdh_stm1_asic_access_S
  *
  *	Outputs:	Returns failure or Success
  ****************************************************************************/
 static PAL_INT_32 hal_sdh_asic_cl8_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {
 	PAL_INT_32 retval = FAILURE;
 	PAL_UINT_32 apb_base_addr  	= STM1_ADM_B_OFFSET;
 	PAL_UINT_32 apb_error_info 	= 0xDEADBE08;

 	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
 	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

 	retval = hw_apb_asic_access(sdh_stm1_asic_access);

 	if((retval == apb_error_info)<0)
 		return retval;

 	return retval;
 }

 /****************************************************************************
  *	Description:Get/Set the register present in Layer 7
  *	Inputs:		core,operation,register address, data, access information
  *	Outputs:	Returns failure or Success
  ****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl7_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
 	PAL_INT_32 retval = FAILURE;
 	PAL_UINT_32 apb_base_addr  	= STM1_ADM_A_OFFSET;
 	PAL_UINT_32 apb_error_info 	= 0xDEADBE07;

 	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
 	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

 	retval = hw_apb_asic_access(sdh_stm1_asic_access);

 	if((retval == apb_error_info)<0)
 		return retval;

 	return retval;
}
/****************************************************************************
*	Description:Get/Set the register present in Layer 6
*
*	Inputs:		Ptr to sdh_stm1_asic_access_S
*
*	Outputs:	Returns failure or Success
****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl6_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
   	printf(" Entered %s \n", __FUNCTION__);
   	PAL_INT_32  retval = FAILURE;
   	PAL_UINT_32 apb_base_addr  = 0x60000;
   	PAL_UINT_32 apb_error_info = 0xDEADBE06;

   	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
   	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

   	retval = hw_apb_asic_access(sdh_stm1_asic_access);

   	if(retval == apb_error_info)
   			retval = FAILURE;

   	printf(" Leaving %s \n", __FUNCTION__);
   	return retval;
}

/****************************************************************************
*	Description:Get/Set the register present in Layer 5
*
*	Inputs:		Ptr to sdh_stm1_asic_access_S
*
*	Outputs:	Returns failure or Success
****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl5_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
   	printf(" Entered %s \n", __FUNCTION__);
   	PAL_INT_32  retval = FAILURE;
   	PAL_UINT_32 apb_base_addr  = 0x50000;
   	PAL_UINT_32 apb_error_info = 0xDEADBE05;

   	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
   	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

   	retval = hw_apb_asic_access(sdh_stm1_asic_access);

   	if(retval == apb_error_info)
   			retval = FAILURE;

   	printf(" Leaving %s \n", __FUNCTION__);
   	return retval;
}

/****************************************************************************
*	Description:Get/Set the register present in Layer 4
*
*	Inputs:		Ptr to sdh_stm1_asic_access_S
*
*	Outputs:	Returns failure or Success
****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl4_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
   	printf(" Entered %s \n", __FUNCTION__);
   	PAL_INT_32  retval = FAILURE;
   	PAL_UINT_32 apb_base_addr  = 0x40000;
   	PAL_UINT_32 apb_error_info = 0xDEADBE04;

   	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
   	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

   	retval = hw_apb_asic_access(sdh_stm1_asic_access);

   	if(retval == apb_error_info)
   			retval = FAILURE;

   	printf(" Leaving %s \n", __FUNCTION__);
   	return retval;
}

/****************************************************************************
*	Description:Get/Set the register present in Layer 3
*
*	Inputs:		Ptr to sdh_stm1_asic_access_S
*
*	Outputs:	Returns failure or Success
****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl3_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
  	printf(" Entered %s \n", __FUNCTION__);
  	PAL_INT_32  retval = FAILURE;
  	PAL_UINT_32 apb_base_addr  = 0x30000;
  	PAL_UINT_32 apb_error_info = 0xDEADBE03;

  	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
  	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

  	retval = hw_apb_asic_access(sdh_stm1_asic_access);

  	if(retval == apb_error_info)
  			retval = FAILURE;

  	printf(" Leaving %s \n", __FUNCTION__);
  	return retval;
}

/****************************************************************************
*	Description:Get/Set the register present in Layer 2
*
*	Inputs:		Ptr to sdh_stm1_asic_access_S
*
*	Outputs:	Returns failure or Success
****************************************************************************/
static PAL_INT_32 hal_sdh_asic_cl2_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
{
  	printf(" Entered %s \n", __FUNCTION__);
  	PAL_INT_32  retval = FAILURE;
  	PAL_UINT_32 apb_base_addr  = 0x20000;
  	PAL_UINT_32 apb_error_info = 0xDEADBE02;

  	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
  	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

  	retval = hw_apb_asic_access(sdh_stm1_asic_access);

  	if(retval == apb_error_info)
  			retval = FAILURE;

  	printf(" Leaving %s \n", __FUNCTION__);
  	return retval;
}

 /****************************************************************************
  *	Description:Get/Set the register present in Layer 1
  *
  *	Inputs:		Ptr to sdh_stm1_asic_access_S
  *
  *	Outputs:	Returns failure or Success
  ****************************************************************************/

 static PAL_INT_32 hal_sdh_asic_cl1_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {
 	printf(" Entered %s \n", __FUNCTION__);
 	PAL_INT_32  retval = FAILURE;
 	PAL_UINT_32 apb_base_addr  = 0x10000;
 	PAL_UINT_32 apb_error_info = 0xDEADBE01;

 	sdh_stm1_asic_access->addr1	= apb_base_addr + sdh_stm1_asic_access->addr1;
 	sdh_stm1_asic_access->addr2	= apb_base_addr + sdh_stm1_asic_access->addr2;

 	retval = hw_apb_asic_access(sdh_stm1_asic_access);

 	if(retval == apb_error_info)
 			retval = FAILURE;

 	printf(" Leaving %s \n", __FUNCTION__);
 	return retval;
 }

 /****************************************************************************
  *	Description:Get/Set the register present in Layer 1
  *
  *	Inputs:		Ptr to sdh_stm1_asic_access_S
  *
  *	Outputs:	Returns failure or Success
  ****************************************************************************/
 static PAL_INT_32 hal_sdh_stm1_ind_asic_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {
 	int access = 0;
 	int retval = FAILURE;

 	if(sdh_stm1_asic_access->init)
 	{
 		access = sdh_stm1_asic_access->init;
 		sdh_stm1_asic_access->init = 0;
 	}

 	switch(sdh_stm1_asic_access->asic_sec)
 	{
 		case ASIC_CL1:
 			if((retval = hal_sdh_asic_cl1_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL2:
 			if((retval = hal_sdh_asic_cl2_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL3:
 			if((retval = hal_sdh_asic_cl3_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL4:
 			if((retval = hal_sdh_asic_cl4_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL5:
 			if((retval = hal_sdh_asic_cl5_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL6:
 			if((retval = hal_sdh_asic_cl6_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL7:
 			if((retval = hal_sdh_asic_cl7_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		case ASIC_CL8:
 			if((retval = hal_sdh_asic_cl8_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
			#ifdef BULK_ACCESS
 			if(access)
 			{
 				sdh_stm1_asic_access->init = access;
 				if((retval = hw_apb_asic_access(sdh_stm1_asic_access))<0)
 				{
 					return retval;
 				}
 			}
 			#endif
 			break;

 		default:
 			break;
 	}
 	return retval;
 }

 static PAL_INT_32 hal_sdh_stm1_bulk_asic_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {
 	int retval = FAILURE;

 	switch(sdh_stm1_asic_access->asic_sec)
 	{
 		case ASIC_CL1:
 			if((retval = hal_sdh_asic_cl1_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL2:
 			if((retval = hal_sdh_asic_cl2_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL3:
 			if((retval = hal_sdh_asic_cl3_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL4:
 			if((retval = hal_sdh_asic_cl4_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL5:
 			if((retval = hal_sdh_asic_cl5_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL6:
 			if((retval = hal_sdh_asic_cl6_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL7:
 			if((retval = hal_sdh_asic_cl7_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;

 		case ASIC_CL8:
 			if((retval = hal_sdh_asic_cl8_access(sdh_stm1_asic_access))<0)
 			{
 				return retval;
 			}
 			break;
 	}
 	return retval;
 }

 static PAL_INT_32 hal_sdh_stm1_asic_access(sdh_stm1_asic_access_S *sdh_stm1_asic_access)
 {
 	int retval = FAILURE;
 	/* Find Bulk access been ready to configure */
 	switch(sdh_stm1_asic_access->abp_access)
 	{
 		case APB_READ:
 		case APB_WRITE:
 		case APB_READ_MODIFY_WRITE:
 		case APB_READ_AFTER_WRITE:
 			retval = hal_sdh_stm1_ind_asic_access(sdh_stm1_asic_access);
 			break;

 		case APB_BULK_READ:
 		case APB_BULK_WRITE:
 			retval = hal_sdh_stm1_bulk_asic_access(sdh_stm1_asic_access);
 			break;
 	}
 	return retval;
}

/****************************************************************************
 *	Description:Convert port number to KLM number
 *
 *	Inputs:	port number
 *
 *	Outputs:	Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_convert_port_to_klm(PAL_SHORT_INT_16 port_no ,PAL_UCHAR *str)
{

	if(str == NULL )
		return FAILURE;


	if((port_no >= 0 ) && (port_no < MAX_STM1_ADM_PORT )) /*port validity check*/
	{
		*str = stm_table[port_no];

	}
	else
	{
		return FAILURE;
	}
	return SUCCESS;
}

/******************************************************************************
 *                      STM1 TRACE MESSAGE CONFIGURATION
******************************************************************************/

/*******************************************************************************
*	Description:This function is used to set the Rs section Mesg Length
*	configuration for a	given ADM Core.
*
*	Inputs:		Address, Data, ADM Core index, Accessinfo
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hal_sdh_stm1_tm_config_set(PAL_INT_32 addr, PAL_INT_32 data, Sdh_ADM_Core_E adm_core_index, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	if(STM1_ADM_MA_A == adm_core_index)
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	}
	else
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL8;
	}
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access);
	if(FAILURE == retval)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed \n", __FUNCTION__);
	}

	return retval;
}

/******************************************************************************
 *                      STM1 TIMING SYNC CONFIGURATION
 ******************************************************************************/

/*******************************************************************************
*	Description:This function is used to set the STM1 timing configurations.
*
*	Inputs:		Address, Data, Accessinfo
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hal_sdh_stm1_sync_config_set(PAL_INT_32 addr, PAL_INT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	printf(" Entered %s \n", __FUNCTION__);

	/* Asked CC this configuration will not be in TDM Block in ASIC, so we need to confirm this */
	sdh_stm1_asic_access.asic_sec 	= ASIC_CL7; /*TBD */

	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access);
	if(FAILURE == retval)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed \n", __FUNCTION__);
	}

	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

/******************************************************************************
 *                      STM1 MUX DEMUX CONFIGURATION
 ******************************************************************************/

/*******************************************************************************
*	Description:This function is used to set the mux-demux configurations.
*
*	Inputs:		Address, Data, ADM Core index, Accessinfo
*
*	Outputs:	Returns failure or Success
********************************************************************************/

PAL_INT_32 hal_sdh_stm1_md_config_set(PAL_INT_32 addr, PAL_INT_32 data, Sdh_ADM_Core_E adm_core_index, PAL_INT_32 access)
{
	PAL_INT_32 				retval = FAILURE;
	sdh_stm1_asic_access_S 	sdh_stm1_asic_access;

	printf(" Entered %s \n", __FUNCTION__);

	if(STM1_ADM_MA_A == adm_core_index)
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	}
	else
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL8;
	}

	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access);
	if(FAILURE == retval)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed \n",__FUNCTION__);
	}

	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

/*******************************************************************************
*	Description:This function is used to get the mux-demux configurations.
*
*	Inputs:		Address, Data Ptr, ADM Core index, Accessinfo
*
*	Outputs:	Returns failure or Success
********************************************************************************/
PAL_INT_32 hal_sdh_stm1_md_config_get(PAL_INT_32 addr, PAL_INT_32 *data, Sdh_ADM_Core_E adm_core_index, PAL_INT_32 access)
{
	PAL_INT_32 				retval = FAILURE;
	sdh_stm1_asic_access_S 	sdh_stm1_asic_access;

	printf(" Entered %s \n", __FUNCTION__);

	if(STM1_ADM_MA_A == adm_core_index)
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	}
	else
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL8;
	}

	sdh_stm1_asic_access.abp_access = APB_READ;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access);
	if(FAILURE == retval)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed \n",__FUNCTION__);
	}

	printf(" Leaving %s \n", __FUNCTION__);
	return retval;
}

/****************************************************************************
 *	Description	: Configures the registers for receiving interrupt on ADM
 *				  alarms
 *
 *	Inputs		: Address, Data, Accessinfo
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_intr_config_set(PAL_UINT_32 addr, PAL_UINT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 val = 0;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", stm_mux_demux_info[INTR_MASK_1A].addr, val);
		return FAILURE;
	}

	return retval;
}

/****************************************************************************
 *	Description	: Configures the registers for receiving interrupt on ADM
 *				  alarms
 *
 *	Inputs		: Address, Data, Accessinfo
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_intr_status_get(PAL_UINT_32 addr, PAL_UINT_32 *data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	PAL_INT_32 val = 0;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	sdh_stm1_asic_access.abp_access = APB_READ;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", stm_mux_demux_info[INTR_MASK_1A].addr, val);
		return FAILURE;
	}

	return retval;
}
 /******************************************************************************
  *                      STM1 Protection CONFIGURATION
 ******************************************************************************/

/****************************************************************************
 *	Description	: Get the current PM status
 *
 *	Inputs		: Address, Data, ADM Core index, Accessinfo
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_protection_config_set(PAL_INT_32 addr, PAL_INT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;


	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", addr, data);
		return FAILURE;
	}

	return retval;
}

PAL_INT_32 hal_sdh_stm1_protection_config_get(PAL_INT_32 addr, PAL_INT_32 *data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_READ;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;


	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", addr, *data);
		return FAILURE;
	}

	return retval;
}

PAL_INT_32 hal_sdh_stm1_protection_config_get_set(PAL_INT_32 addr1, PAL_INT_32 addr2, PAL_INT_32 data, Rmw_Opt_E rmw_opt, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_READ_MODIFY_WRITE;
	sdh_stm1_asic_access.addr1		= addr1;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= addr2;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.opt_rmw	= rmw_opt;
	sdh_stm1_asic_access.init		= access;


	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", addr1, data);
		return FAILURE;
	}

	return retval;
}

/****************************************************************************
 *				CROSSPOINT CONFIGURATION
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_cp_config_set(PAL_INT_32 addr, PAL_INT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL6;
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;


	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf("hal_sdh_asic_cl1_operation : Failed to Updated Reg : %x with Value: %x \n", addr, data);
		return FAILURE;
	}

	return retval;
}

/****************************************************************************
 *				PERFORMANCE MONITORING
 ****************************************************************************/
/****************************************************************************
 *	Description	: Set the Performance data from the address requested for the
 *				  given size
 *
 *	Inputs		: Address, Data Ptr, Access Info/Count
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_pm_data_set(PAL_INT_32 addr, PAL_INT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= 1;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf(" %s hal_sdh_stm1_asic_access Failed ",__FUNCTION__);
	}

	return retval;
}
/****************************************************************************
 *	Description	: Get the Performance data from the address requested for the
 *				  given size
 *
 *	Inputs		: Address, Data Ptr, ADM Core Number, Access Info/Count
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_pm_data_get(PAL_INT_32 addr, PAL_INT_32 *data, Sdh_ADM_Core_E adm_core_index, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	if(STM1_ADM_MA_A == adm_core_index)
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL7;
	}
	else
	{
		sdh_stm1_asic_access.asic_sec 	= ASIC_CL8;
	}

	sdh_stm1_asic_access.abp_access = APB_BULK_READ;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= access;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed \n",__FUNCTION__);
		return FAILURE;
	}

	return retval;
}

/****************************************************************************
 *	Description	: Get the current PM status
 *
 *	Inputs		: Address, Data Ptr, Access Info
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_pm_status_get(PAL_INT_32 addr, PAL_INT_32 *data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_READ;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= 1;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf(" %s : hal_sdh_stm1_asic_access failed ",__FUNCTION__);
		return FAILURE;
	}

	return retval;
}

/****************************************************************************
 *	Description:Set the PM status
 *
 *	Inputs		: Address, Data, Access Info
 *
 *	Outputs		: Returns failure or Success
 ****************************************************************************/
PAL_INT_32 hal_sdh_stm1_pm_status_set(PAL_INT_32 addr, PAL_INT_32 data, PAL_INT_32 access)
{
	PAL_INT_32 retval = FAILURE;
	sdh_stm1_asic_access_S sdh_stm1_asic_access;

	sdh_stm1_asic_access.asic_sec 	= ASIC_CL1;
	sdh_stm1_asic_access.abp_access = APB_WRITE;
	sdh_stm1_asic_access.addr1		= addr;
	sdh_stm1_asic_access.data1		= &data;
	sdh_stm1_asic_access.addr2		= 0;
	sdh_stm1_asic_access.data2		= NULL;
	sdh_stm1_asic_access.init		= 1;

	if((retval = hal_sdh_stm1_asic_access(&sdh_stm1_asic_access))<0)
	{
		printf(" %s hal_sdh_stm1_asic_access Failed ",__FUNCTION__);
	}

	return retval;
}

int hal_sdh_stm1_cfg_liu(Stm1LIU_Cfg_Id_E liu_cfg_info,unsigned char liu_type, unsigned char liu_access,
				unsigned char liuPortIndex, unsigned char liuOperation, void *data_val)
{
	hal_liu_cfg_info_S hal_cfg_update;
	int ret_val = 0, retry =0,max_retries = 3;

	/* Configure the STM1 Port  */
	hal_cfg_update.liu_access = liu_access;
	hal_cfg_update.liuPortIndex = liuPortIndex;
	hal_cfg_update.liuOperation = liuOperation;
	hal_cfg_update.data_ptr = data_val;

	/* Callback function call with respect to LIU and STM1 Configuration */
	if(NULL == sdh_stm1_liu_cfg[liu_cfg_info].hal_liu_sdh_stm1_cfg[liu_type])
	{
		return LIU_NO_SUPPORT;
	}
	for(retry = 0; retry < max_retries;retry++)
	{
		ret_val = (sdh_stm1_liu_cfg[liu_cfg_info].hal_liu_sdh_stm1_cfg[liu_type])(&hal_cfg_update);
		if(SUCCESS == ret_val)
			break;
	}
	if(max_retries == retry)
		ret_val = FAILURE;

	return ret_val;
}


PAL_INT_32 hal_sdh_stm1_md_lp_status_register_get(PAL_INT_32 lp_index, PAL_UINT_32 *lp_alm_status, PAL_INT_32 access_type, Sdh_ADM_Core_E  adm_core_index)
{
                /* TBD */
	return 0;
}

PAL_INT_32 form_packet_mux_demux(Sdh_Stm1_Op_S *stm_mux_demux, PAL_INT_32 access_type)
{
                /* TBD */
	return 0;
}

PAL_INT_32 form_packet_access_stm_framer(PAL_UCHAR *addr, PAL_UCHAR *data_buff, Sdh_Stm1_Op_S *sdh_stm1_liu_op)
{
                /* TBD */
	return 0;
}











