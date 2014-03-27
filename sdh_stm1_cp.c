/* Standard Library Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>




#include "csv_util.h"
#include "../core/include/sdh_stm1_enums.h"
#include "sdh_stm1_cp.h"
#include "pdh_e1t1_cp.h"

/******************************************************************************
 * GLOBAL VARIABLES AND ENUMS
 *****************************************************************************/
#define STM1_CP_INIT(enume,interface,interface_type,interface_offset,interface_locn,port_no_locn,max_ports,max_ports_alp,priority,port_not,port_str)    \
                            {interface,interface_type,interface_locn,port_no_locn,interface_offset,max_ports,max_ports_alp,priority,port_not,port_str},
STM1_CP_Info_S stm1_cp_info[]={STM1_CP_INTERFACE};
#undef STM1_CP_INIT


#define STM1_PRECEDENCE_INIT(enume,precedence_str,repeat_limit,max_no)    \
                            {precedence_str,repeat_limit,max_no},
STM1_Precedence_Info_S stm1_precedence_info[]={STM1_PRECEDENCE_INTERFACE};
#undef STM1_PRECEDENCE_INIT

/******************************************************************************
 * FUNCTION DEFINITIONS
 *****************************************************************************/


/******************************************************************************
 * Description: This function will append the string src to the string dest and
 *              add the separator string to the same.
 *
 * Inputs     : chmap  - structure containing the separator and combination string.
 *
 * Outputs    : None
 ******************************************************************************/
char *strcat_separator(char *dest,char *src,int size_dest,char *precedence)
{
    if((size_dest - strlen(dest)) < strlen(src))
    {
        return NULL;
    }

    strcat(dest,src);
    strcat(dest,precedence);
    return dest;
}

/******************************************************************************
 * Description: This function applies generic rule to extract the individual
 *				components. These sub components are stored in string format with
 *				their precedence value, which is useful for configuring the
 *				specific hardware.
 *
 * Inputs     :
 *              stm1_cp_map 	- This points to structure where the maps are stored.
 *              map_str_cnt 	- Number of mapping entered.
 *              sub_map_cnt		- Number of individual component entered in each line.
 *              precedence_cnt  - Precedence count number.
 *
 * Outputs    : 0 on success
 *              -ve error code on failure
 ******************************************************************************/
int extract_mapping(stm1_cp_map_S *stm1_cp_map, int map_str_cnt, int *sub_map_cnt, int precedence_cnt)
{
	char *token=NULL, *pretoken=NULL;
	int  map_length=0, comb=0;
	char map_string_temp[MAP_STRING_LENGTH_MAX];
	char map_string[MAP_STRING_LENGTH_MAX];
	char precedence_l[2];
	int map_cnt=0;

	map_cnt = map_str_cnt;

	/* Initilize the Array Memory*/
	memset(map_string_temp, 0, sizeof(map_string_temp));
	memset(map_string, 0, sizeof(map_string));
	memset(precedence_l, 0, sizeof(precedence_l));

	//printf("Start stm1_cp_map[%d].map_element[%d] = %s \n", map_cnt, *sub_map_cnt, stm1_cp_map[map_cnt].map_element[*sub_map_cnt]);
	/* Copy the Mapping which need to be extracted */
	strcpy(map_string_temp, stm1_cp_map[map_cnt].map_element[*sub_map_cnt]);
	/* Keep a backup copy */
	strcpy(map_string, map_string_temp);

	/* Check the String Length for Validity */
	if(strlen(map_string)<=0)
		return;

	//printf("Start map_string : %s map_string_temp : %s\n", map_string, map_string_temp);

	/* Take a copy of Local Precedence value */
	precedence_l[0] = stm1_precedence_info[precedence_cnt].precedence[0];
	//printf("precedence_l : %s precedence_cnt : %d\n", precedence_l, precedence_cnt);

	/* Search for the Token */
	token=strtok(map_string_temp, &precedence_l[0]);
	/* Move the address Location next to token */
	token=strtok(NULL,&precedence_l[0]);

	/* If we need to follow the precedence e.g, : can exist only if higher presence , is there
	   then remove the below if condition and its statement */
	if(token == NULL)
	{
		precedence_cnt = MAX_PRECEDENCE;
	}
	/* If token does not matches then verify whether it matches to the
	   any other token from the precedence database */
	while(!token)
	{
		//printf("!token map_string_temp : %s \n", map_string_temp);
		token=strtok(map_string_temp, &precedence_l[0]);
		token=strtok(NULL,&precedence_l[0]);
		if(token || precedence_cnt<1)
			break;
		else
		{
			precedence_cnt--;
			precedence_l[0] =  stm1_precedence_info[precedence_cnt].precedence[0];
			//printf("!token precedence_l : %s precedence_cnt : %d\n", precedence_l, precedence_cnt);
		}

	}
	/* Take a Backup Location of the token to store the previous address */
	pretoken = token;
	if(token != NULL)
	{
		/* Setup Length to copy */
		map_length=token - map_string_temp -1;
		if(map_length>0)
		{
			strncpy(stm1_cp_map[map_cnt].map_element[*sub_map_cnt],map_string_temp,map_length);
			stm1_cp_map[map_cnt].map_element[*sub_map_cnt][map_length]='\0';
			stm1_cp_map[map_cnt].map_status[*sub_map_cnt+1] = precedence_cnt;
			precedence_cnt++;
			if(precedence_cnt<MAX_PRECEDENCE)
			{
				extract_mapping(stm1_cp_map, map_cnt, sub_map_cnt, precedence_cnt);
			}
			while(token)
			{
				token=strtok(NULL,&precedence_l[0]);
				/* End of Token */
				if(token == NULL)
				{
					if(stm1_cp_map[map_cnt].precedence_max[precedence_cnt]< stm1_precedence_info[precedence_cnt].max_no)
					{
						*sub_map_cnt = *sub_map_cnt+1;
						map_length = map_string_temp + strlen(map_string) - pretoken;
						strncpy(stm1_cp_map[map_cnt].map_element[*sub_map_cnt],map_string+strlen(map_string_temp)+1,map_length);
						pretoken = token;
						stm1_cp_map[map_cnt].map_element[*sub_map_cnt][map_length]='\0';
						stm1_cp_map[map_cnt].map_status[*sub_map_cnt] = precedence_cnt;
						stm1_cp_map[map_cnt].precedence_max[precedence_cnt]++;
						extract_mapping(stm1_cp_map, map_cnt, sub_map_cnt, precedence_cnt);
						return;

					}
					else
					{
						printf("Channel Mapping Invalid in Line : %d......\n",map_cnt);
						stm1_cp_map[map_cnt].status=STM1_CP_INVALID;
						break;
					}
				}
				else
				{
					if(stm1_cp_map[map_cnt].precedence_max[precedence_cnt]<stm1_precedence_info[precedence_cnt].max_no)
					{
						*sub_map_cnt = *sub_map_cnt+1;
						map_length = token - pretoken;
						strncpy(stm1_cp_map[map_cnt].map_element[*sub_map_cnt],pretoken,map_length);
						pretoken = token;
						stm1_cp_map[map_cnt].map_element[*sub_map_cnt][map_length]='\0';
						stm1_cp_map[map_cnt].map_status[*sub_map_cnt] = precedence_cnt;
						stm1_cp_map[map_cnt].precedence_max[precedence_cnt]++;
						extract_mapping(stm1_cp_map, map_cnt, sub_map_cnt, precedence_cnt);
					}
					else
					{
						//printf("1.Channel Mapping Invalid in Line : %d......\n",map_cnt);
						stm1_cp_map[map_cnt].status=STM1_CP_INVALID;
						break;
					}
				}
			}

		}
		else
		{
			//printf("2.Channel Mapping Invalid in Line : %d......\n",map_cnt);
			stm1_cp_map[map_cnt].status=STM1_CP_INVALID;
		}
	}
	return 0;
}

/******************************************************************************
 * Description: This function applies generic rule and breaks the Channel Map into
 *				individual components i.e. it breaks down the mapping into the sub
 *				components. These sub components are stored in string format.
 *
 * Inputs     : map_list - this is buffer containing the channel map.
 *              stm1_cp_map - this points to structure where the maps are stored.
 *              chmap - contains the separator and combination strings defined by the caller
 *
 * Outputs    : 0 on success
 *              -ve error code on failure
 ******************************************************************************/
int convert_map_to_token(char *map_list, stm1_cp_map_S *stm1_cp_map, STM1_Chmap_S *chmap)
{
     char map_string_temp[MAP_STRING_LENGTH_MAX];
     int  map_length=0, line_length=0;
     int  map_str_cnt=0;
     char *token=NULL,*pretoken=NULL,*map_type_token=NULL;
     int  no_of_mappings=0;
     int  map_cnt=0;
     char passthru_map = 0;
     unsigned char vc_map_invalid =0;
     char vc_map_string_temp[MAP_STRING_LENGTH_MAX];
     int  sub_map_cnt = 0;
     int precedence_cnt = 0;

     //printf("Entering %s\n",__FUNCTION__);
	 //printf("map_list = %s \n", map_list);
     /*
      * Obtain individual mappings from the channel map. The individual mappings
      * in the channel map are separated by the chmap->separator. This separator
      * is defined by the calling function.
      */
     token=strtok(map_list, SEPERATOR);
     if(token == NULL)
     {
         return -1;
     }

     while(token)
     {
         //printf("Seperator Present \n");
         /*
          * Each mapping is stored as it is in the map_string component of the stm1_cp_map.
          * This is done because, in case of invalid mappings the string can be used as
          * it is.
          */
          if(line_length = strlen(token)<=MAP_STRING_LENGTH_MAX)
          {
			  strcpy(stm1_cp_map[no_of_mappings].map_string,token);
		  }
		  else
		  {
			  strncpy(stm1_cp_map[no_of_mappings].map_string,token, MAP_STRING_LENGTH_MAX);
		  }
		  token=strtok(NULL,"\n");
		  //printf("stm1_cp_map[no_of_mappings].map_string = %s \n", stm1_cp_map[no_of_mappings].map_string);
          no_of_mappings++;
     }

	 //printf("All Lines Copied : %d\n", no_of_mappings);
 	 /*
      *  no_of_mappings provides the number of lines that is present in the channel map.
      *  Each line in the mapping is converted into its invidual component which is the
      *  input and the output component.The input and output componenets are stored in the
      *  string format in the map_element component of the stm1_cp_map_S structure.
      */
     for(map_cnt=0;map_cnt<no_of_mappings;map_cnt++)
     {
        /*<Bugzilla><1581><4-Mar-2010><RP>*/
         passthru_map = 0;
        /*
         * Line contains the Info string "Invalid Mappings:".The status of this line is
         * updated as info. This status ensures that this line is not processed in the
         * next stages.
         */
         if(strcmp(stm1_cp_map[map_cnt].map_string,STM1_INVALID_STR) == 0)
         {
			 //printf("1.Channel Mapping STM1_INVALID_STR in Line : %d......\n",map_cnt);
             stm1_cp_map[map_cnt].status=STM1_CP_INFO;
             continue;
         }

        //eat_white_space(map_string_temp);
        /*
         * Line contains the Info string "Valid Mappings:".The status of this line is
         * updated as info. This status ensures that this line is not processed in the
         * next stages.
         */
         if(strcmp(stm1_cp_map[map_cnt].map_string,STM1_VALID_STR) == 0)
         {
			 //printf("1.Channel Mapping STM1_VALID_STR in Line : %d......\n",map_cnt);
             stm1_cp_map[map_cnt].status=STM1_CP_INFO;
             continue;
         }

        /*
         * Line contains the Info string "Incomplete Mappings:".The status of this line is
         * updated as info. This status ensures that this line is not processed in the
         * next stages.
         */
         if(strcmp(stm1_cp_map[map_cnt].map_string,STM1_INCOMPLETE_STR) == 0)
         {
			 //printf("1.Channel Mapping STM1_INCOMPLETE_STR in Line : %d......\n",map_cnt);
             stm1_cp_map[map_cnt].status=STM1_CP_INFO;
             continue;
         }

        /*
         * Line contains the Info string "Unused Mappings:".The status of this line is
         * updated as info. This status ensures that this line is not processed in the
         * next stages.
         */
         if(strcmp(stm1_cp_map[map_cnt].map_string,STM1_UNUSED_STR) == 0)
         {
			 //printf("1.Channel Mapping STM1_UNUSED_STR in Line : %d......\n",map_cnt);
             stm1_cp_map[map_cnt].status=STM1_CP_INFO;
             continue;
         }

        /*
         * Copy each line of the map is copied into a temporary string. This is done so that the
         * original string remains unaffected.All operations are then performed on the temporary
         * string, this ensures that if the string is invalidated for any reason then the
         * original string can still be provided to calling function as an invalid string.
         */
         memset(map_string_temp,0,sizeof(map_string_temp));
         strncpy(map_string_temp,stm1_cp_map[map_cnt].map_string,MAP_STRING_LENGTH_MAX);

         eat_white_space(map_string_temp);
         //printf("map_string_temp,stm1_cp_map[map_cnt].map_string %s \n", map_string_temp,stm1_cp_map[map_cnt].map_string);
        /*
         * The input and output component in the map is separated by the combination
         * string defined by the calling function.
         */

        /* Extract the Component seperated by tokens from precedence database*/
         strcpy(stm1_cp_map[map_cnt].map_element[sub_map_cnt],stm1_cp_map[map_cnt].map_string);
		 extract_mapping(stm1_cp_map, map_cnt, &sub_map_cnt, 0);
		 stm1_cp_map[map_cnt].max_sub_elements = sub_map_cnt+1;
		 sub_map_cnt = 0;
	 }

    return no_of_mappings;
}

/******************************************************************************
 * Description: This function will convert the KLM numbering format to E1/T1 port
 *              format.
 * Inputs     :
 *              port_no 	- This have KLM Numbering format.
 *
 * Outputs    : Failure -1 when validation fails
 *				Success  0
 ******************************************************************************/
int convert_from_stm1_klm_no(int *port_no)
{
    int k,l,m;

	k = *port_no/100;
	*port_no = *port_no%100;
	l = *port_no/10;
	m = *port_no%10;

	//printf("K : %d L : %d M : %d \n", k, l, m);
	/*KLM validation*/
	if((k < MIN_KLM_FIELD) || (k > MAX_K_FIELD) || \
			(l < MIN_KLM_FIELD) || (l > MAX_L_FIELD) || \
			(m < MIN_KLM_FIELD) || (m > max_m_field))
	{
		printf("KLM : Invalid\n");
		return -1;
	}

	/* convert to port number for E1/T1 */
	*port_no = ((MAX_L_FIELD*max_m_field)*(k-1)) + (max_m_field*(l-1)) + m;
	return 0;
}

/******************************************************************************
 * Description: This function will convert the port number from string to
 *              numerical format.
 * Inputs     :
 *              map 		- This have string name.
 *              interface 	- This shows interface type
 *
 * Outputs    : None
 ******************************************************************************/
int convert_to_port_no(char *map ,int interface_index)
{
    int port_no=-1;
    char *endptr;
	int retval=-1;
	//printf("Entering : %s \n", __FUNCTION__);
    /*
     * Convert port number from string to integer format.
     */
    /* If map contains only digits */
    //printf("map : %s \n", map);
    //printf("port_no_locn : %d \n", stm1_cp_info[interface_index].port_no_locn);
    if(stm1_cp_info[interface_index].port_not == INTEGER)
    {
		port_no = strtol(&map[stm1_cp_info[interface_index].port_no_locn],&endptr,10);
		printf("port_no : %d \n", port_no);
		if(*endptr != '\0')
		{
			printf("Endptr Not NULL \n");
			return -1;
		}
	}
	else
	{
		 /* If the string has extra chars other than Alphabet'
		 * it is an invalid string */
		 if(map[stm1_cp_info[interface_index].port_no_locn+1])
		 	return -1;

		 port_no = (int)map[stm1_cp_info[interface_index].port_no_locn];
		 if((port_no >= 'A') && (port_no <= 'D'))
		 	port_no = (port_no - 'A')+1;
		 else if((port_no >= 'a') && (port_no <= 'd'))
			port_no = (port_no - 'a')+1;
		 else
		 	return -1;
	 }

    /* If Master or Expansion IO ADM ports */
/*    if(stm1_cp_info[interface_index].interface_type == STM1_ADM_PORT)
    {*/

/*      if((retval=convert_str_to_portno(&port_no))<0)*/
        /*get the individual KLM fields from the number obtained*/
 /*       if((retval=convert_from_stm1_klm_no(&port_no))<0)
        {
			 printf("KLM Port Number Failure: %d\n", port_no);
			 return retval;
		}
		//printf("KLM Port Number: %d\n", port_no);
        return (stm1_cp_info[interface_index].offset + port_no);
    }
    else*/
    {
        /*
         * Convert port number to internal numbering format based on the interface_index.
         */
        if(port_no >=1 && port_no <=stm1_cp_info[interface_index].max_ports)
        {
            //printf("Real port_no : %d \n", stm1_cp_info[interface_index].offset + port_no);
            return (stm1_cp_info[interface_index].offset + port_no);
        }
        else
        {
            return -1;
        }
    }
    return -1;
}

/******************************************************************************
 * Description: This function apply generic rule converts the invidual components
 *              of the map string into its corresponding port number.The conversion
 *              makes use of the stm1_cp_info to determine the Port offset for the
 *              various interfaces. Also, mapping with invalid port numbers are detected
 *
 * Inputs     :
 *              stm1_cp_map - This points to structure where the maps are stored.
 *              chmap 		- contains the separator and combination strings defined by the caller
 *              map_str_cnt - Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int convert_token_to_port(stm1_cp_map_S *stm1_cp_map, STM1_Chmap_S *chmap, int map_str_cnt)
{
    int  map_cnt=0, sub_map_cnt=0;
    char temp_str[MAX_STM1_STR_LENGTH];
    int  port_no=-1;
    char *token=NULL;
    int  convert_protection_path=0, map_length=0;
	int  invalid_cnt=0, str_info=0;

    //printf("Entering %s\n",__FUNCTION__);

    /*
     * map_cnt is incremented only if both the components of a given map have been
     * converted or if an error was detected in the conversion of the first component
     * itself.
     */
    for(map_cnt=0; map_cnt<map_str_cnt; map_cnt++)
    {
        //printf("map_str_cnt : %d \n", map_str_cnt);
        //printf("stm1_cp_map[map_cnt].status : %d \n", stm1_cp_map[map_cnt].status);
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
			{
				port_no=-1;
				/*
				 * Conversion to port numbers is only done for those mappings that have been
				 * declared valid in the previous stage. Also, the mapping elements
				 * that are info strings are also ingnored for this stage of processing
				 */
            	memset(temp_str, 0, sizeof(temp_str));
                strcpy(temp_str,stm1_cp_map[map_cnt].map_element[sub_map_cnt]);
                for(str_info=0; str_info<MAX_STM1_CP; str_info++)
                {
					//printf("port_str : %s \n", stm1_cp_info[str_info].port_str);
					token=strstr(temp_str, stm1_cp_info[str_info].port_str);
					if(token != NULL)
					{
						//printf("temp_str : %s \n", temp_str);
						port_no = convert_to_port_no(temp_str, str_info);
						//printf("port_no : %d \n", port_no);
						break;
					}
					else
					{
						continue;
					}
				}
				if(port_no == -1)
				{
					//printf("port_no Invalid \n");
					invalid_cnt++;
				}
                else
                {
                    stm1_cp_map[map_cnt].map_array[sub_map_cnt] = port_no;
				}
			}

		}
		/*
		 * If any port in the mapping is an invlid port(refer to channel map rules)
		 * then that mapping is treated as an invalid mapping.
		 */
		if(invalid_cnt>0)
		{
			invalid_cnt = 0;
			//printf("Line Invalid due to port invalid\n");
			stm1_cp_map[map_cnt].status=STM1_CP_INVALID;
		}
    }
    return map_cnt;
}

/******************************************************************************
 * Description: This function creates a table of all valid STM entries in
 *              ascending order. The table acts as a look up to convert the
 *              port numbers of STM1 to string
 *
 * Inputs     : ptr - To store the created STM table
 *              max_m_field - Max 'M'field based on E1/T1.
 *
 * Outputs    : 0 on success
 *              -ve error code on failure
 ******************************************************************************/
void create_stm1_klm_table(void)
{
    int k,l,m,i=0;;
    int stm1_type;

    max_m_field = MAX_M_FIELD_E1; // Default E1

    for(k=1; k<=MAX_K_FIELD; k++) /*for all Ks*/
    {
        for(l=1; l<=MAX_L_FIELD; l++) /*for all Ls*/
        {
            /*for all Ms :max_m_field is 3 for E1 and 4 for T1*/
            for(m=1; m <= max_m_field; m++)
            {
                stm1_table[i] = (k*100)+(l*10)+m;
                i++;
            }
        }
    }
    return;
}

/******************************************************************************
 * Description: This function applies line rule for the ports that have repeated
 *              mapping and invalidates those mappings within the line
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int line_repeat_mappings_check(stm1_cp_map_S *stm1_cp_map,int map_str_cnt)
{
    int line_repeat_map_array[MAX_INTERFACE_NO][MAX_PORTS_INTERFACE];
    int interface_no=0;
    int port_no=0;
    int map_cnt=0, sub_map_cnt=0, sub_map_precedence=0;
    unsigned int enum_precedence, repeat_mapping_found = 0;
    int repeat_mapping_limit[MAX_PRECEDENCE];

    //printf("Entering %s\n",__FUNCTION__);

    /*
     * The line_repeat_map_array two dimensional array, such that the array can be
     * indexed by means of the interface no and port no. By default,
     * the array will be initialised to 0. For, any valid port if indexed array element
     * is non-zero it indicates that the port is already part of another valid mapping.
     *
     * The function parses through valid maps from the stm1_cp_map_S array. For, every valid port
     * the corresponing line_repeat_map_array array element is set to the index of the mapping to
     * which this port belongs. Hence,if line_repeat_map_array element is non-zero, then the array
     * provides the index of the map which has a duplicate port.
     *
     * Also, the status component of the map with the duplicate port is set to STM1_CP_DUPL to
     * identify this as a duplicate mapping. Duplicate mappings are ignored for the next processing
     * stages.
     */

    for(map_cnt=0;map_cnt < map_str_cnt;map_cnt++)
    {
        memset(line_repeat_map_array,0,sizeof(line_repeat_map_array));
		memset(repeat_mapping_limit, 0, sizeof(repeat_mapping_limit));

        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
 			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
 			{

            	/*
            	 * Identify duplicate mappings for the Input port.
             	 */
            	interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
            	port_no   	 = PORT_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
            	if(sub_map_cnt)
            		enum_precedence	= stm1_cp_map[map_cnt].map_status[sub_map_cnt] - 1;
            	else
            		enum_precedence = 0;

				/*
				 * For a duplicate mapping, the line_repeat_map_array should be non-zero, and
				 * if non-zero the index should not correspond to the index of the current
				 * mapping. This is to avoid treating loopback mappings as duplicate mappings.
				 */
				/*
				 * +1 is needed to ensure that we can distinguish it from 0, which is the default value.
				 */
            	if(line_repeat_map_array[interface_no][port_no] != 0 &&\
            						line_repeat_map_array[interface_no][port_no] != sub_map_cnt+1)
            	{
                	repeat_mapping_limit[enum_precedence]++;
                	//printf("map_cnt : %d sub_map_cnt: %d interface_no : %d\n", map_cnt, sub_map_cnt, interface_no);
                	//printf("repeat_mapping_limit[%d] : %d \n", enum_precedence, repeat_mapping_limit[enum_precedence]);
                	//printf("stm1_precedence_info[%d].repeat_limit : %d \n", enum_precedence, stm1_precedence_info[enum_precedence].repeat_limit);
                	if(repeat_mapping_limit[enum_precedence] > stm1_precedence_info[enum_precedence].repeat_limit)
                	{
						//printf("repeat_mapping_found : map_cnt :%d \n", map_cnt);
						repeat_mapping_found++;
					}

					if(repeat_mapping_found)
					{
						repeat_mapping_found = 0;
						//printf("1. %d\n",stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
						//printf("1. %d : %d : %d\n",interface_no, port_no, line_repeat_map_array[interface_no][port_no]);
						stm1_cp_map[map_cnt].status = STM1_CP_DUPL;
						line_repeat_map_array[interface_no][port_no] = sub_map_cnt+1;
					}
            	}
				else
				{
					/*
					 * Update the line_repeat_map_array with the index of the current mapping.
					 */
					line_repeat_map_array[interface_no][port_no] = sub_map_cnt+1;
				}
			}
		}
	}
    return 0;
}

/******************************************************************************
 * Description: This function apply file rule for the ports that have repeated
 *              mapping and invalidates those mappings within the file
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int file_repeat_mappings_check(stm1_cp_map_S *stm1_cp_map,int map_str_cnt)
{
    int file_repeat_map_array[MAX_INTERFACE_NO][MAX_PORTS_INTERFACE];
    int interface_no=0;
    int port_no=0;
    int map_cnt=0, sub_map_cnt=0;

    //printf("Entering %s\n",__FUNCTION__);

    /*
     * The file_repeat_map_array two dimensional array, such that the array can be
     * indexed by means of the interface no and port no. By default,
     * the array will be initialised to 0. For, any valid port if indexed array element
     * is non-zero it indicates that the port is already part of another valid mapping.
     *
     * The function parses through valid maps from the stm1_cp_map_S array. For, every valid port
     * the corresponing file_repeat_map_array array element is set to the index of the mapping to
     * which this port belongs. Hence,if file_repeat_map_array element is non-zero, then the array
     * provides the index of the map which has a duplicate port.
     *
     * Also, the status component of the map with the duplicate port is set to STM1_CP_DUPL to
     * identify this as a duplicate mapping. Duplicate mappings are ignored for the next processing
     * stages.
     */
    memset(file_repeat_map_array,0,sizeof(file_repeat_map_array));

    for(map_cnt=0;map_cnt<map_str_cnt;map_cnt++)
    {
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
 			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
 			{
            	/*
            	 * Identify duplicate mappings for the Input port.
             	 */
            	interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
            	port_no   	 = PORT_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);

				/*
				 * For a duplicate mapping, the file_repeat_map_array should be non-zero, and
				 * if non-zero the index should not correspond to the index of the current
				 * mapping. This is to avoid treating loopback mappings as duplicate mappings.
				 */
				/*
				 * +1 is needed to ensure that we can distinguish it from 0, which is the default value.
				 */
            	if(file_repeat_map_array[interface_no][port_no] != 0 &&\
            						file_repeat_map_array[interface_no][port_no] != map_cnt+1)
            	{
                	//printf("2. %d\n",stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
                	//printf("2. %d : %d : %d\n",interface_no, port_no, file_repeat_map_array[interface_no][port_no]);
                	stm1_cp_map[map_cnt].status = STM1_CP_DUPL;
                	stm1_cp_map[file_repeat_map_array[interface_no][port_no] -1].status = STM1_CP_DUPL;
                	file_repeat_map_array[interface_no][port_no] = map_cnt+1;
            	}
				else
				{
					/*
					 * Update the file_repeat_map_array with the index of the current mapping.
					 */
					file_repeat_map_array[interface_no][port_no] = map_cnt+1;
				}
			}
		}
	}
    return 0;
}

/******************************************************************************
 * Description: This function will apply a line rule for checking the port with
 *				respect to VC interface..
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int line_vc_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
    int interface_no=0;
    int port_no=0;
    int map_cnt=0,sub_map_cnt=0;

    //printf("Entering %s\n",__FUNCTION__);

    /* 	VC Mapping has following rules
       	1. VC Map Should have only Combination as precedence
 		2. VC Mapping should not mixed with Front Pannel Mapping */

    for(map_cnt=0;map_cnt<map_str_cnt;map_cnt++)
    {
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
 			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
 			{
                interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
                //printf("interface_no %d stm1_cp_map[map_cnt].map_array[sub_map_cnt]:%d\n",interface_no, stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
				if(interface_no == stm1_cp_info[STM1_VC_SP_PORT].interface)
				{
					//printf("VC Mapping Present \n");
					for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
					{
						 /* Check VC Map Mixed with Precedence More than combination */
						interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
						if(stm1_cp_map[map_cnt].map_status[sub_map_cnt] > (COMBINATION_MAPPING+1) &&\
						 								interface_no != stm1_cp_info[STM1_VC_SP_PORT].interface)
						 {
							 //printf("VC Mapping Invalid due to Precedence not supported\n");
							 stm1_cp_map[map_cnt].status=STM1_CP_INVALID;
							 break;
						 }
						  /* Check VC Map Mixed with E1/T1 Front Pannel Ports */
						 else
						 {
			                 //printf("interface_no %d stm1_cp_map[map_cnt].map_array[sub_map_cnt]:%d\n",interface_no, stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
							 if(interface_no == stm1_cp_info[STM1_MA_PORT].interface /*||\
							 			interface_no == stm1_cp_info[STM1_EX_PORT].interface*/)
							 {
								//printf("VC Mapping Invalid due to E1/T1 Front Panel Mixed Mapping\n");
								stm1_cp_map[map_cnt].status = STM1_CP_INVALID;
								break;
							 }
						/*	 if(stm1_cp_map[map_cnt].status == STM1_CP_INVALID)
							 {
								 break;
							 }*/
						 }
					 }
				 }
				 if(stm1_cp_map[map_cnt].status == STM1_CP_INVALID)
				 {
					break;
				 }
			 }
		 }
	 }
	 return 0;
 }

/******************************************************************************
 * Description: This function generates the list of incomplete mappings.
 *              As per channel map rules all active timeslots should be
 *              mapped. If any active timeslot is not part of a active or
 *              unused mapping then it  is an incomplete mapping. The channel
 *              map will not be updated if there is an incomplete mapping.
 *
 * Inputs     : stm1_cp_map 		- This points to the validated map
 *				param_map_list		- The list to the user after parsing
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int file_incomplete_port_check(stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
    int  used_list[MAX_INTERFACE_NO][MAX_STM1_MAPPINGS];
    int  map_cnt=0, sub_map_cnt=0;
    char temp_str[MAP_STRING_LENGTH_MAX];
    int  interface_index=0;
    int  interface_no=0;
    int  port_no=0;

    //printf("Entering %s\n",__FUNCTION__);

    memset(used_list,0,sizeof(used_list));

    /*
     * The used list is a two dimension array that can be indexed using the interface_no
     * index and port index of a given port number.If a given port number is part
     * of an active or unused mapping then the entry corresponding to this port in
     * the used list array is set to 1.
     */
    for(map_cnt=0; map_cnt<map_str_cnt; map_cnt++)
    {
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID ||
            stm1_cp_map[map_cnt].status == STM1_CP_UNUSED )
        {
			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
			{
            	interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
            	port_no   	 = PORT_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);

				used_list[interface_no][port_no] =1;
				param_map_list->interface_cnt[interface_no]++;
			}
		}
	}

    /*
     * buf_incomplete, will contain the incomplete mappings in their string format, with
     * required map separator string.
     */
    /*
     * Update the info string "Incomplete Mappings:" in the buffer.
     */
    strcat_separator(param_map_list->incomplete_map, STM1_INCOMPLETE_STR, MAP_INFO_LENGTH_MAX, SEPERATOR);

	/* Read the Database and Identify the Minimum Mapping that need to be provided for Each MODEM Interface */
	//Test Hard code
	//stm1_cp_limit->current_acm_level =0;
	//stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][STM1_RADIO_MA_PORT] = 1000;
	//stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][STM1_RADIO_MB_PORT] = 1000;
	//stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][STM1_RADIO_MC_PORT] = 1000;
	//stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][STM1_RADIO_MD_PORT] = 1000;

	for(interface_index=0; interface_index<MAX_STM1_CP; interface_index++)
	{
	   	interface_no = stm1_cp_info[interface_index].interface;
        /* Set System Ports on Interface not Greater than Maximum Ports supported for that interface */
        if(stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][interface_index] > stm1_cp_info[interface_index].max_ports)
        {
			stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][interface_index] = stm1_cp_info[interface_index].max_ports;
		}

        if(param_map_list->interface_cnt[interface_no] < stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][interface_index])
        {
            /*
             * Check if all the  MODEM active timeslots are present in the used list.If the used_list
             * entry for this port_no is 0 then this port_no is not present in any of the active or unused
             * mappings. Convert this port_no into its string format and update it in the buffer
             * buf_incomplete.
             */
            for(map_cnt=0; map_cnt<stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][interface_index]; map_cnt++)
            {
                if(stm1_cp_info[interface_index].interface_type == STM1_MAC_FPA_PORT)
                {
                	if(used_list[interface_no][map_cnt] == 0)
                	{
                    	memset(temp_str,0,sizeof(temp_str));
                    	sprintf(temp_str,"%s%d",stm1_cp_info[interface_index].port_str,(map_cnt+1));
                    	//printf("Incomplete string :%s\n",temp_str);
                    	param_map_list->incomplete_map_cnt++;
                     	strcat_separator(param_map_list->incomplete_map,temp_str,param_map_list->incomplete_map_cnt*MAX_STM1_MAP_STR_LEN, SEPERATOR);
					}
                }
            }
        }
	}

    //printf("%s\n%s\n",__FUNCTION__,param_map_list->incomplete_map);
    return 1;
}

/******************************************************************************
 * Description: This function will apply the file rule for checking the port
 *				priority with respect to individual interface.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int file_priority_port_check(stm1_cp_map_S *stm1_cp_map, int map_str_cnt)
{
	/* When there is interconnection limitation we need to proritize the
	   the interconnection mapping, depanding on Port priotity and restrict the
	   Mapping */
}

/******************************************************************************
 * Description: This function will apply the file rule for checking the port with
 *				respect to individual interface limitation on hardware.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int	file_hw_limit_active_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_info, int map_str_cnt)
{
	/* Read the Hardware Limitations from the Database */
	/*
	   1. Number of Interconnect supported
	*/

	/* Create a filter from the Hardware Limitation */

	/* Use the Filter to Remove the Mapping that are not supported*/

	/* The Mapping not supported in this section will be moved to Unused section */
}

/******************************************************************************
 * Description: This function will apply the file rule for checking the port with
 *				respect to individual interface limitation on hardware and software.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int file_active_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* Active Mapping Depands on Various Harware and software configurations */
	/* file Rule: 2.2.1  Hardware Port Rule */
	file_hw_limit_active_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
	/* file Rule: 2.2.2  Software Port Rule */
	//file_sw_limit_active_port_check(stm1_cp_map, map_str_cnt);
}

/******************************************************************************
 * Description: This function will apply the file rule for checking the port with
 *				respect to individual interface and precedence.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *				param_map_list		- The list to the user after parsing
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int file_port_check(stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* Check the File Port Rule */
	/* File Rule: 2.1 Priority Rule */
	file_priority_port_check(stm1_cp_map, map_str_cnt);
	/* File Rule: 2.2  Active Port Map Rule */
	file_active_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
	/* File Rule: 2.3  Active Port Map Rule */
	file_incomplete_port_check(stm1_cp_map, param_map_list, stm1_cp_limit, map_str_cnt);

}

/******************************************************************************
 * Description: This function will apply the file rule for parsing the
 *				cross-connect mappings format.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *				param_map_list		- The list to the user after parsing
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int stm1_parse_file_rule(stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* File Rule: 1 Check Repeat Mappings */
	file_repeat_mappings_check(stm1_cp_map, map_str_cnt);
	/* File Rule: 2 Port Rule */
	file_port_check(stm1_cp_map, param_map_list, stm1_cp_limit, map_str_cnt);
}

/******************************************************************************
 * Description: This function will apply the line rule for checking the port with
 *				respect to individual interface limitation on software.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int	line_sw_limit_active_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
    int interface_no=0;
    int port_no=0;
    int map_cnt=0,sub_map_cnt=0;

	/* Read the Software Limitations from the Database */
	/* 1. ODU/IDU Modes of Operation
	   2. ACM Level Check
	   3. Precedence Check
	*/

    //printf("Entering %s\n",__FUNCTION__);
	/* Create a filter from the Software Limitation */

	/* Use the Filter to Remove the Mapping that are not supported*/

	/* The Mapping not supported in this section will be moved to Unused section */
    for(map_cnt=0;map_cnt<map_str_cnt;map_cnt++)
    {
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
 			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
 			{
				interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
				port_no   	 = PORT_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);

				/* ACM Level check */
				/* TBD */
				if(interface_no>0)
				{
					/* This Can be done by creating Filter with Max Port supported in each interface */
					if(port_no>stm1_cp_limit->no_active_interface[stm1_cp_limit->current_acm_level][interface_no])
					{
						stm1_cp_map[map_cnt].status = STM1_CP_UNUSED;
					}
				}
				/* Filteration on Precedence that are not supported */
				/* Curently STM Crosspoint doest not support any precedence other than combination */
				if(stm1_cp_map[map_cnt].map_status[sub_map_cnt] > (COMBINATION_MAPPING+1))
				{
					stm1_cp_map[map_cnt].status = STM1_CP_INVALID;
				}
			}
		}
	}
	return SUCCESS;
}

/******************************************************************************
 * Description: This function will apply the line rule for checking the port with
 *				respect to individual interface limitation on hardware.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int	line_hw_limit_active_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
    int interface_no=0;
    int port_no=0;
    int map_cnt=0,sub_map_cnt=0;
	/* Read the Hardware Limitations from the Database */
	/*
	   1. Number of STM1 supported
	   2. Authorization Limit
	   3. Vendor Limitation
	   4. Protection Mapping Support
	   5. VC Mapping Support
	*/

	/* Create a filter from the Hardware Limitation */
    //printf("Entering %s\n",__FUNCTION__);
	/* Use the Filter to Remove the Mapping that are not supported*/
	/* The Mapping not supported in this section will be moved to Unused section */
    for(map_cnt=0;map_cnt<map_str_cnt;map_cnt++)
    {
        if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
 			for(sub_map_cnt=0; sub_map_cnt<stm1_cp_map[map_cnt].max_sub_elements; sub_map_cnt++)
 			{
				interface_no = INTERFACE_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);
				port_no   	 = PORT_NO(stm1_cp_map[map_cnt].map_array[sub_map_cnt]);

				/* Filteration on Max Port supported in hardware */
				if(interface_no>0)
				{
					/* This Can be done by creating Filter with Max Port supported in each interface */
					if(port_no>stm1_cp_limit->no_active_interface[0][interface_no])
					{
						stm1_cp_map[map_cnt].status = STM1_CP_UNUSED;
					}
				}

			}
		}
	}
	return SUCCESS;
}

/******************************************************************************
 * Description: This function will apply the line rule for checking the port with
 *				respect to individual interface limitation on hardware and software.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int line_active_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* Active Mapping Depands on Various Harware and software configurations */
	/* Line Rule: 2.2.1  Hardware Port Rule */
	line_hw_limit_active_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
	/* Line Rule: 2.2.2  Software Port Rule */
	line_sw_limit_active_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
}

/******************************************************************************
 * Description: This function will apply the line rule for checking the port with
 *				respect to individual interface and precedence.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int line_port_check(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit,int map_str_cnt)
{
	/* Check the Individual Port Rule */
	/* Line Rule: 2.1 VC Port Map Rule */
	line_vc_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
	/* Line Rule: 2.2  Active Port Map Rule */
	line_active_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
}

/******************************************************************************
 * Description: This function will apply the line rule for parsing the
 *				cross-connect mappings format.
 *
 * Inputs     :
 *				stm1_cp_map 		- This points to the validated map
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int stm1_parse_line_rule(stm1_cp_map_S *stm1_cp_map, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* Line Rule: 1 Check Repeat Mappings */
	line_repeat_mappings_check(stm1_cp_map, map_str_cnt);
	/* Line Rule: 2 Port Rule */
	line_port_check(stm1_cp_map, stm1_cp_limit, map_str_cnt);
}

/******************************************************************************
 * Description: This function will apply the generic rule for parsing the
 *				cross-connect mappings format.
 *
 * Inputs     :
 *              map_list			- List entered by the user
 *				stm1_cp_map 		- This points to the validated map
 *              chmap  				- Structure containing the separator and combination string.
 *              stm1_cp_info 		- System Information
 *
 * Outputs    : None
 ******************************************************************************/
int stm1_parse_generic_rule(char *map_list,stm1_cp_map_S *stm1_cp_map,STM1_Chmap_S *chmap, stm1_cp_limit_S *stm1_cp_limit)
{
	int map_cnt;
	/* Generic Rule: 1 Extract Individual Mapping */
	map_cnt = convert_map_to_token(map_list, stm1_cp_map, chmap);
	/* Generic Rule: 2 Converting Valid String to Port Number */
	convert_token_to_port(stm1_cp_map, chmap, map_cnt);

	return map_cnt;
}

/******************************************************************************
 * Description: This function will create the stm1 channel map in parsed
 *              mappings format.
 *
 * Inputs     :
 *              stm1_cp_map 		- This points to the validated map
 *				param_map_list		- The list to the user after parsing
 *              stm1_cp_info 		- System Information
 *              map_str_cnt  		- Number of mapping entered.
 *
 * Outputs    : None
 ******************************************************************************/
int stm1_create_parse_list(stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
    int  valid_map_cnt=0;
    int  unused_map_cnt=0;
    int  invalid_map_cnt=0;
    int  total_map_cnt=0;
    int  map_cnt=0;

    //printf("Entering %s\n",__FUNCTION__);

	strcat_separator(param_map_list->valid_map,STM1_VALID_STR,MAP_INFO_LENGTH_MAX,SEPERATOR);
	strcat_separator(param_map_list->unused_map,STM1_UNUSED_STR,MAP_INFO_LENGTH_MAX,SEPERATOR);
	strcat_separator(param_map_list->invalid_map,STM1_INVALID_STR,MAP_INFO_LENGTH_MAX,SEPERATOR);
    for(map_cnt=0;map_cnt<map_str_cnt;map_cnt++)
    {
        /*
         * Convert the active mappings to the string format.
         */
		 if(stm1_cp_map[map_cnt].status == STM1_CP_VALID)
        {
			valid_map_cnt++;
			strcat_separator(param_map_list->valid_map,stm1_cp_map[map_cnt].map_string,\
													valid_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
            //printf("Valid Map [%d]: %s\n", valid_map_cnt, param_map_list->valid_map);
		}
        /*
         * Convert the unused mappings to the string format.
         */
        else if(stm1_cp_map[map_cnt].status == STM1_CP_UNUSED)
        {
			unused_map_cnt++;
			strcat_separator(param_map_list->unused_map,stm1_cp_map[map_cnt].map_string,\
													unused_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
            //printf("Unused Map [%d]: %s\n", unused_map_cnt, param_map_list->unused_map);
		}
        /*
         * Convert the invalid and duplicated mappings to the string format.
         */
        else if(stm1_cp_map[map_cnt].status == STM1_CP_INVALID ||
            stm1_cp_map[map_cnt].status == STM1_CP_DUPL )
        {
			invalid_map_cnt++;
			strcat_separator(param_map_list->invalid_map,stm1_cp_map[map_cnt].map_string,\
													invalid_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
			//printf("Invalid Map [%d]: %s\n", invalid_map_cnt, param_map_list->invalid_map);
        }
    }

    if(param_map_list->incomplete_map_cnt)
    {
        total_map_cnt += param_map_list->incomplete_map_cnt;
        strcat_separator(param_map_list->map_list, param_map_list->incomplete_map,\
													total_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
    }

    if(valid_map_cnt)
    {
        //printf("Valid_map_cnt : %d Valid Map : %s\n", valid_map_cnt, param_map_list->valid_map);
        total_map_cnt += valid_map_cnt;
        strcat_separator(param_map_list->map_list, param_map_list->valid_map,\
													total_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
		param_map_list->valid_map_cnt = valid_map_cnt;
    }

    if(unused_map_cnt)
    {
        //printf("Unused_map_cnt : %d Unused Map : %s\n", unused_map_cnt, param_map_list->unused_map);
        total_map_cnt += unused_map_cnt;
        strcat_separator(param_map_list->map_list, param_map_list->unused_map,\
													total_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
		param_map_list->unused_map_cnt = unused_map_cnt;
    }

    if(invalid_map_cnt)
    {
        //printf("Invalid_map_cnt : %d Invalid Map : %s\n", invalid_map_cnt, param_map_list->invalid_map);
		total_map_cnt += invalid_map_cnt;
        strcat_separator(param_map_list->map_list, param_map_list->invalid_map,\
													total_map_cnt*MAP_STRING_LENGTH_MAX,SEPERATOR);
		param_map_list->invalid_map_cnt = invalid_map_cnt;
    }

    return 1;
}

/*int get_system_current_info(stm1_cp_limit_S *stm1_cp_limit)
{

}*/

int stm1_parse_cross_connect_update(stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, \
															stm1_cp_limit_S *stm1_cp_limit, int map_str_cnt)
{
	/* Update the Process Regarding Cross Connect only If there is no
	   Invalid Mapping,Duplicate or Incomplete Mapping */
	if(param_map_list->invalid_map_cnt == 0 || param_map_list->incomplete_map_cnt == 0)
	{
		/* Update the Process with User configured Cross Connect Mappings */
	}
}

#if 0
/******************************************************************************
 * Description: This function will read the channel map from the channel map
 *              file and convert the channel map into the string format.
 *              A mapping is said to have occurred only if two field's value is
 *              not a "NA".There will be 3 mappings in the file.
 *              The first column is the source and the destination column will
 *              be either 2nd ,3rd or 4th column based on the mapping number.
 *
 * Outputs    : None
 ******************************************************************************/
int read_all_mappings_from_file(int cmd, FILE *csv_fp, char ptr[][MAX_STM1_MAP_STR_LEN], \
                                    STM1_Chmap_S *chmap)
{
    /* temporary buffer to store lines read from file */
    char    line[MAX_LINE_SIZE];
    char    fields[MAX_CSV_COLS][MAX_CSV_COL_SIZE];
    /* Maximum columns in channel map file */
    int     num_fields = MAXIMUM_FIELDS_IN_FILE;
    int     line_num = 3, i;
    char    *p;
    char    *token = NULL;

    DEBUG3("Entering Function: %s\n", __FUNCTION__);

    if(chmap==NULL)
    {
        return -1;
    }

    /* Jump the first two rows */
    if(read_next_line(csv_fp, line, sizeof(line))==NULL)
    {
        return -E_BAD_FILE;
    }

    memset(chmap->mappings_type,0,sizeof(chmap->mappings_type));

    if(read_csv_next_line(csv_fp, fields, &num_fields, line_num) >=0)
    {
        for(i=1;i<num_fields;i++)
        {
            if(strlen(fields[i]) == 0)
                return -E_BAD_FILE;

            strncpy(chmap->mappings_type[i-1],fields[i],MAX_TYPE_FIELD_SIZE);
            /* When the string len exceeds MAX_TYPE_FIELD_SIZE the string will be
             * truncated with a '/0' */
            chmap->mappings_type[i-1][MAX_TYPE_FIELD_SIZE-1]='\0';
        }
    }
    else
    {
        return -E_BAD_FILE;
    }

    chmap->no_of_mappings_type = num_fields -1;

    /* If user wants only mappings type list, we return only
     * map list, else we will return all the mappings */
    if(cmd == GET_MAP_LIST)
    {
        return 1;
    }

    if(ptr==NULL)
    {
        return -1;
    }

    /* Max no. of columns to read */
    num_fields = MAXIMUM_FIELDS_IN_FILE;
    while(read_csv_next_line(csv_fp, fields, &num_fields, line_num) >=0)
    {
        /* if some mappings are missing */
        if(chmap->no_of_mappings_type != (num_fields-1)) /*including the base col*/
        {
            return -E_BAD_DATA_IN_CFG;
        }

        for(i=1;i<num_fields;i++)
        {
            if(ptr[i-1]==NULL)
                continue;

             p=ptr[i-1];

            if((strlen(fields[i]) == 0) || (strlen(fields[0]) == 0))
            {
                return -E_BAD_DATA_IN_CFG;
            }

            if((strcmp(fields[0],"NA")!=0) && (strcmp(fields[i],"NA")!=0))
            {
                p+=strlen(ptr[i-1]);

                sprintf(p, "%s%s%s%s", fields[0], chmap->combination, \
                                fields[i], chmap->separator);

                token = strstr(p,chmap->combination);
                if(strncmp((token+1),chmap->r_prot_separator, 1)==0)
                {
                    sprintf(p, "%s%s%s", fields[0],  \
                                    fields[i], chmap->separator);
                }
            }
        } /*end of for*/

        chmap->no_of_mappings_type = num_fields-1;

        num_fields = MAXIMUM_FIELDS_IN_FILE;

    } /*end of while*/

    return 1;
}

int get_stm1_cross_connect_map_from_file(int cmd, char *ptr, STM1_Chmap_S *chmap)
{
	FILE *csv_fp = (FILE *)NULL;

	/* Open the E1 T1 Channel map CSV file */
	if((csv_fp = fopen(path,"r")) == (FILE *)NULL)
	{
		return -E_FOPEN_FAILED;
	}

	/* Read all the mappings from the channel map file */
	retval = read_all_mappings_from_file(cmd,csv_fp,ptr,chmap);
	if(retval < 0)
	{
		DEBUG3("Error %d returned from read_mappings_from_file\n", ret_val);
		/* Close the channel map file */
		fclose(csv_fp);
		return -1;
	}

	/* Close the channel map file */
	fclose(csv_fp);
}
#endif

int cil_sdh_stm1_parse_crosspoint(char *ptr, stm1_cp_map_S *stm1_cp_map, stm1_param_map_list_S *param_map_list, int line_cnt)
{
	int i = 0, retval = 0;
	STM1_Chmap_S chmap;
	stm1_cp_limit_S	stm1_cp_limit;
	memset(stm1_cp_map, 0, sizeof(stm1_cp_map_S)*line_cnt);
	memset(param_map_list, 0, sizeof(param_map_list));
	memset(&stm1_cp_limit, 0, sizeof(stm1_cp_limit));
	int map_cnt;
	int sub_map_cnt;

	printf("\n------------Input:--------------\n%s \n", ptr);

	eat_white_space(ptr);


	/* Read the Database and Identify the Minimum Mapping that need to be
	   provided for Each MODEM Interface */
	/* Sedona - YTBM
	cil_sdh_stm1_get_cp_current_info(&stm1_cp_limit);*/

	cil_sdh_stm1_get_cp_system_info(&stm1_cp_limit);

	create_stm1_klm_table();

	/* File Support for Cross Point Data */
	//int cmd;
	//get_stm1_cross_connect_map_from_file(cmd,ptr,&chmap);

	/* General Rule */
	map_cnt = stm1_parse_generic_rule(ptr, stm1_cp_map, &chmap, &stm1_cp_limit);

	/* Line Rule */
	stm1_parse_line_rule(stm1_cp_map, &stm1_cp_limit, map_cnt);

	/* File Rule */
	stm1_parse_file_rule(stm1_cp_map, param_map_list, &stm1_cp_limit, map_cnt);

	/* Parsed List */
	stm1_create_parse_list(stm1_cp_map, param_map_list, &stm1_cp_limit, map_cnt);

	/* Notify Process for Configuration */
	stm1_parse_cross_connect_update(stm1_cp_map, param_map_list, &stm1_cp_limit, map_cnt);


	/* Test */
/*
	for(i=0; i<map_cnt; i++)
	{
		//printf("stm1_cp_map[%d].max_sub_elements : %d \n", i, stm1_cp_map[i].max_sub_elements);
		for(sub_map_cnt = 0; sub_map_cnt<stm1_cp_map[i].max_sub_elements; sub_map_cnt++)
		{
			printf("stm1_cp_map[%d].map_element[%d] = %s \n", i, sub_map_cnt, stm1_cp_map[i].map_element[sub_map_cnt]);
			printf("stm1_cp_map[%d].map_status[%d]  = %d \n", i, sub_map_cnt, stm1_cp_map[i].map_status[sub_map_cnt]);
			printf("stm1_cp_map[%d].map_array[%d]  	= %d \n", i, sub_map_cnt,stm1_cp_map[i].map_array[sub_map_cnt]);
		}
		printf("stm1_cp_map[%d].status  = %d \n", i, stm1_cp_map[i].status);
	}
	printf("\n------------Output:--------------\n %s \n", param_map_list->map_list);*/
	return 0;
}

