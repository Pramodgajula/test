/******************************************************************************
 * PROJECT: Sedona SDH Library for Param Database configuration
 *
 * Copyright (C) CarrierComm Inc.
 *
 * FILE: sdh_stm1_dbl.c
 *
 * ABSTRACT:
 *
 * REVISION HISTORY
 *
 * Revision 1.0  2014/02/27
 *  -- First release of module files.
 *
 ******************************************************************************/

/*******************************************************************************
 * Include Header Files
 ******************************************************************************/
/* Standard Includes */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

/* Param Includes */
#include <params.h>
#include <params_types.h>
#include <params_sqlite.h>
#include <params_xml.h>

/* Sedond SDH Modules*/
#include "sdh_stm1_dbl.h"

#define ELEM_TAG "STM1-PortConfiguration"

/* CARD/CHIP TYPE ENUM - ID */
#define SEDONA_IO_INIT(label, value, name) {name,label},
static const struct params_id Sedona_IO_list[] = {SEDONA_IO_TYPE{NULL,-1}};
#undef SEDONA_IO_INIT

/* CARD/CHIP TYPE ENUM - TYPE */
static const struct params_type Sedona_IO_type = PARAMS_ID_TYPE(Sedona_IO_list, sizeof(Sedona_IO_E));



/* STM1 CARD INFO - FIELD */			
static const struct params_field Stm1_Card_Info_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Card_Info, card_type, &Sedona_IO_type),
	PARAMS_STRUCT_FIELD(Stm1_Card_Info, max_stm_port_core, &params_type_uchar),
	PARAMS_STRUCT_FIELD_END
};

/* STM1 CARD INFO - TYPE */			
static const struct params_type Stm1_Card_Info_type = \
						      PARAMS_STRUCT_TYPE(Stm1_Card_Info, &Stm1_Card_Info_fields);

/* STM1 CARD INFO - ARRAY TYPE */		
static const struct params_type Stm1_Card_Info_array_type = \
							    PARAMS_ARRAY_TYPE(&Stm1_Card_Info_type, "Stm1CardInfoBuf", "Stm1CardInfoBuf");


/*
   |*********** START PORT CONFIGURATION ***********|
 */

/* LOOP BACK ENUM - ID */
#define SDH_LB_INIT(label, value, name) {name, label},
static const struct params_id Stm1_Port_Loopback_list[] = \
{SDH_LOOPBACK_SEL{NULL, -1}};
#undef SDH_LB_INIT

/* LOOP BACK ENUM - TYPE */
static const struct params_type Stm1_Port_Loopback_type = \
							  PARAMS_ID_TYPE(Stm1_Port_Loopback_list, sizeof(Stm1_Port_Loopback_E));


/* LINECODING ENUM - ID */
#define STM1_MEDIUM_LINE_CODING_SETTING_INIT(label, value, name) {name,label},
static const struct params_id Stm1_Medium_Line_Coding_list[] = \
{STM1_MEDIUM_LINE_CODING_SETTING{NULL, -1}};
#undef STM1_MEDIUM_LINE_CODING_SETTING_INIT

/* LINECODING ENUM - TYPE */
static const struct params_type Stm1_Medium_Line_Coding_type = \
							       PARAMS_ID_TYPE(Stm1_Medium_Line_Coding_list, sizeof(Stm1_Medium_Line_Coding_E));


/* LINETYPE ENUM - ID */
#define STM1_MEDIUM_LINE_TYPE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Stm1_Medium_Line_Type_list[] = \
{STM1_MEDIUM_LINE_TYPE_SETTING {NULL, -1}};
#undef STM1_MEDIUM_LINE_TYPE_SETTING_INIT

/* LINETYPE ENUM - TYPE */
static const struct params_type Stm1_Medium_Line_Type_type = \
							     PARAMS_ID_TYPE(Stm1_Medium_Line_Type_list, sizeof(Stm1_Medium_Line_Type_E)); 


/* STM1 PORT PARAMS  - FIELD */
static const struct params_field Stm1_Port_Params_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Port_Params, loopback, &Stm1_Port_Loopback_type),
		PARAMS_STRUCT_FIELD(Stm1_Port_Params, line_coding, &Stm1_Medium_Line_Coding_type),
		PARAMS_STRUCT_FIELD(Stm1_Port_Params, line_type, &Stm1_Medium_Line_Type_type),
		PARAMS_STRUCT_FIELD(Stm1_Port_Params, status, &params_type_uchar),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 PORT PARAMS  - TYPE */
static const struct params_type Stm1_Port_Params_type = \
							PARAMS_STRUCT_TYPE(Stm1_Port_Params, &Stm1_Port_Params_fields);

/* STM1 PORT PARAMS  - ARRAY TYPE */
static const struct params_type Stm1_Port_Params_array_type = \
							      PARAMS_ARRAY_TYPE(&Stm1_Port_Params_type, "Stm1PortParams", "Stm1PortParams");

/* STM1 PORT CONFIG  - FIELD */
static const struct params_field Stm1_Port_Config_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Port_Config, version, &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Port_Config, num_cards, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Stm1_Port_Config, sdh_card_info, &Stm1_Card_Info_array_type),
		PARAMS_STRUCT_FIELD(Stm1_Port_Config, port_params, &Stm1_Port_Params_array_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 PORT CONFIG  - TYPE */
static const struct params_type Stm1_Port_Config_type = \
							PARAMS_STRUCT_TYPE(Stm1_Port_Config, &Stm1_Port_Config_fields);
/*
   |*********** End PORT CONFIGURATION ***********|
 */

/*	
	|*********** Start Core CONFIGURATION ***********|
 */

/* ENUM STM1 OP mode - ID */
#define SDH_OPMODE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Opmode_list[] = \
{SDH_OPMODE_SETTING{NULL, -1}};
#undef SDH_OPMODE_SETTING_INIT

/*  ENUM STM1 OP mode - TYPE */
static const struct params_type Sdh_Stm1_Opmode_type = \
						       PARAMS_ID_TYPE(Sdh_Stm1_Opmode_list, sizeof(Sdh_Stm1_Opmode_E));

/* ENUM SONET Medium Type - ID */
#define SONET_MEDIUM_TYPE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sonet_Medium_Type_list[] = \
{SONET_MEDIUM_TYPE_SETTING{NULL, -1}};
#undef SONET_MEDIUM_TYPE_SETTING_INIT

/*  ENUM SONET Medium Type - TYPE */
static const struct params_type Sonet_Medium_Type_type = \
							 PARAMS_ID_TYPE(Sonet_Medium_Type_list, sizeof(Sonet_Medium_Type_E));

/* ENUM STM1 Medium Threshold - ID */
#define SDH_MEDIUM_THRESH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Ses_Thresh_list[] = \
{SDH_MEDIUM_THRESH_SETTING{NULL, -1}};
#undef SDH_MEDIUM_THRESH_SETTING_INIT

/* ENUM STM1 Medium Threshold - TYPE */
static const struct params_type Sdh_Stm1_Ses_Thresh_type = \
							   PARAMS_ID_TYPE(Sdh_Stm1_Ses_Thresh_list, sizeof(Sdh_Stm1_Ses_Thresh_E));

/* MUX-DEMUX Regenerator - RS*/

/*  ENUM STM1 RS RATE - ID */
#define RS_SDH_RATE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Rs_Rate_list[] = \
{RS_SDH_RATE_SETTING{NULL, -1}};
#undef RS_SDH_RATE_SETTING_INIT

/* ENUM STM1 RS RATE - TYPE */
static const struct params_type Sdh_Stm1_Rs_Rate_type = \
							PARAMS_ID_TYPE(Sdh_Stm1_Rs_Rate_list, sizeof(Sdh_Stm1_Rs_Rate_E));

/* STM1 CORE RS Parms - FIELD */
static const struct params_field  Sdh_Rs_Parms_fields[] = \
{
	//PARAMS_STRUCT_FIELD(Sdh_Rs_Parms, rs_descrambler, &params_type_boolean_char),
	PARAMS_STRUCT_FIELD(Sdh_Rs_Parms, rs_descrambler, &params_type_char),
	PARAMS_STRUCT_FIELD(Sdh_Rs_Parms, rs_sdh_rate,    &Sdh_Stm1_Rs_Rate_type),
	PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE RS Parms - TYPE */
static const struct params_type Sdh_Rs_Parms_type = \
						    PARAMS_STRUCT_TYPE(Sdh_Rs_Parms, &Sdh_Rs_Parms_fields);

/* MUX-DEMUX Multiplex - MS*/

/*  ENUM MS-REI-CTRL - ID */
#define MS_REI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Ms_Rei_Ctrl_Type_list[] = \
{MS_REI_SETTING{NULL, -1}};
#undef MS_REI_SETTING_INIT

/*  ENUM MS-REI-CTRL - TYPE */
static const struct params_type Ms_Rei_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Ms_Rei_Ctrl_Type_list, sizeof(Ms_Rei_Ctrl_Type_E));


/*  ENUM MS-RDI-CTRL - ID */
#define MS_RDI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Ms_Rdi_Ctrl_Type_list[] = \
{MS_RDI_SETTING{NULL, -1}};
#undef MS_RDI_SETTING_INIT

/*  ENUM MS-RDI-CTRL - TYPE */
static const struct params_type Ms_Rdi_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Ms_Rdi_Ctrl_Type_list, sizeof(Ms_Rdi_Ctrl_Type_E));


/* STM1 CORE MS Parms - FIELD */
static const struct params_field Sdh_Ms_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Ms_Parms,  ms_rei_ctrl_type, &Ms_Rei_Ctrl_Type_type),
		PARAMS_STRUCT_FIELD(Sdh_Ms_Parms,  ms_rdi_ctrl_type, &Ms_Rdi_Ctrl_Type_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE MS Parms - TYPE */
static const struct params_type Sdh_Ms_Parms_type = \
						    PARAMS_STRUCT_TYPE(Sdh_Ms_Parms, &Sdh_Ms_Parms_fields);

/* MUX-DEMUX Higher Ordier Path - HP*/

/*  ENUM HP-REI-CTRL - ID */
#define HP_REI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Hp_Rei_Ctrl_Type_list[] = \
{HP_REI_SETTING{NULL, -1}};
#undef HP_REI_SETTING_INIT

/*  ENUM HP-REI-CTRL - TYPE */
static const struct params_type Hp_Rei_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Hp_Rei_Ctrl_Type_list, sizeof(Hp_Rei_Ctrl_Type_E));


/*  ENUM HP-RDI-CTRL - ID */
#define HP_RDI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Hp_Rdi_Ctrl_Type_list[] = \
{HP_RDI_SETTING{NULL, -1}};
#undef HP_RDI_SETTING_INIT

/*  ENUM HP-RDI-CTRL - TYPE */
static const struct params_type Hp_Rdi_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Hp_Rdi_Ctrl_Type_list, sizeof(Hp_Rdi_Ctrl_Type_E));


/*  ENUM HP-WIDTH - ID */
#define HP_WIDTH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Hp_Width_list[] = \
{HP_WIDTH_SETTING{NULL, -1}};
#undef HP_WIDTH_SETTING_INIT

/*  ENUM HP-WIDTH - TYPE */
static const struct params_type Hp_Width_type = \
						PARAMS_ID_TYPE(Hp_Width_list, sizeof(Hp_Width_E));


/* STM1 CORE HP Parms - FIELD */
static const struct params_field Sdh_Hp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Hp_Parms, hp_rei_ctrl_type, &Hp_Rei_Ctrl_Type_type ),
		PARAMS_STRUCT_FIELD(Sdh_Hp_Parms, hp_rdi_ctrl_type, &Hp_Rdi_Ctrl_Type_type ),
		PARAMS_STRUCT_FIELD(Sdh_Hp_Parms, hp_path_label, 	&params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Hp_Parms, hp_width,		&Hp_Width_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE HP Parms - TYPE */
static const struct params_type Sdh_Hp_Parms_type = \
						    PARAMS_STRUCT_TYPE(Sdh_Hp_Parms, &Sdh_Hp_Parms_fields);

/* MUX-DEMUX Lower Order Path - LP*/

/*  ENUM LP-REI-CTRL - ID */
#define LP_REI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Lp_Rei_Ctrl_Type_list[] = \
{LP_REI_SETTING{NULL, -1}};
#undef LP_REI_SETTING_INIT

/*  ENUM LP-REI-CTRL - TYPE */
static const struct params_type Lp_Rei_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Lp_Rei_Ctrl_Type_list, sizeof(Lp_Rei_Ctrl_Type_E));


/*  ENUM LP-RDI-CTRL - ID */
#define LP_RDI_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Lp_Rdi_Ctrl_Type_list[] = \
{LP_RDI_SETTING{NULL, -1}};
#undef LP_RDI_SETTING_INIT

/*  ENUM LP-RDI-CTRL - TYPE */
static const struct params_type Lp_Rdi_Ctrl_Type_type = \
							PARAMS_ID_TYPE(Lp_Rdi_Ctrl_Type_list, sizeof(Lp_Rdi_Ctrl_Type_E));


/*  ENUM LP-WIDTH - ID */
#define LP_WIDTH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Lp_Width_list[] = \
{LP_WIDTH_SETTING{NULL, -1}};
#undef LP_WIDTH_SETTING_INIT

/*  ENUM LP-WIDTH - TYPE */
static const struct params_type Lp_Width_type = \
						PARAMS_ID_TYPE(Lp_Width_list, sizeof(Lp_Width_E));


/* STM1 CORE LP Parms - FIELD */
static const struct params_field Sdh_Lp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Lp_Parms, lp_rei_ctrl_type, &Lp_Rei_Ctrl_Type_type),
		PARAMS_STRUCT_FIELD(Sdh_Lp_Parms, lp_rdi_ctrl_type, &Lp_Rdi_Ctrl_Type_type),
		PARAMS_STRUCT_FIELD(Sdh_Lp_Parms, lp_path_label, 	&params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Lp_Parms, lp_width, 	&Lp_Width_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE LP Parms - TYPE */
static const struct params_type Sdh_Lp_Parms_type = \
						    PARAMS_STRUCT_TYPE(Sdh_Lp_Parms, &Sdh_Lp_Parms_fields);

/* Type for field 'circuit_id' */
static const struct params_type circuit_id_type = \
						  PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Stm1_Mux_Demux_Cfg *)0)->circuit_id));

/* STM1 CORE Mux-Demux Cfg- FIELD */
static const struct params_field Stm1_Mux_Demux_Cfg_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	op_mode,       	    	&Sdh_Stm1_Opmode_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	circuit_id,             &circuit_id_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	medium_type, 	    	&Sonet_Medium_Type_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	ses_thresh_val, 	&Sdh_Stm1_Ses_Thresh_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	rs_parms, 		&Sdh_Rs_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	ms_parms, 		&Sdh_Ms_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	hp_parms, 		&Sdh_Hp_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	lp_parms, 		&Sdh_Lp_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Mux_Demux_Cfg, 	status,			&params_type_uchar),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE Mux-Demux Cfg- TYPE */
static const struct params_type Stm1_Mux_Demux_Cfg_type = \
							  PARAMS_STRUCT_TYPE(Stm1_Mux_Demux_Cfg, &Stm1_Mux_Demux_Cfg_fields);

/* STM1 CORE Mux-Demux Cfg - ARRAY TYPE */
static const struct params_type Stm1_Mux_Demux_Cfg_array_type = \
								PARAMS_ARRAY_TYPE(&Stm1_Mux_Demux_Cfg_type, "Stm1MuxDemuxCfg", "Stm1MuxDemuxCfg");

/* STM1 CORE Mux-Demux - FIELD */
static const struct params_field Stm1_Core_Mux_Demux_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Core_Mux_Demux, version,       &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Core_Mux_Demux, num_cards,     &params_type_uchar),
		PARAMS_STRUCT_FIELD(Stm1_Core_Mux_Demux, sdh_card_info, &Stm1_Card_Info_array_type),
		PARAMS_STRUCT_FIELD(Stm1_Core_Mux_Demux, ptr_core_cfg,  &Stm1_Mux_Demux_Cfg_array_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE Mux-Demux - TYPE */
static const struct params_type Stm1_Core_Mux_Demux_type = \
							   PARAMS_STRUCT_TYPE(Stm1_Core_Mux_Demux, &Stm1_Core_Mux_Demux_fields);

/*  Trace Msg  */

/* TM Regenerator - RS*/

/* ENUM STM1 TM RS - ID */
#define RS_TM_ALIGN_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Rs_Tm_Align_Type_list[] = \
{RS_TM_ALIGN_SETTING{NULL, -1}};
#undef RS_TM_ALIGN_SETTING_INIT

/* ENUM STM1 TM RS - TYPE */
static const struct params_type Rs_Tm_Align_Type_type = \
							PARAMS_ID_TYPE(Rs_Tm_Align_Type_list, sizeof(Rs_Tm_Align_Type_E));


/* ENUM STM1 TM Length - ID */
#define RS_TM_LENGTH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Rs_Tm_Length_list[] = \
{RS_TM_LENGTH_SETTING{NULL, -1}};
#undef RS_TM_LENGTH_SETTING_INIT

/* ENUM STM1 TM Length - TYPE */
static const struct params_type Rs_Tm_Length_type = \
						    PARAMS_ID_TYPE(Rs_Tm_Length_list, sizeof(Rs_Tm_Length_E));


/* STM1 CORE TM RS - FIELD */
static const struct params_field  Sdh_Tm_Rs_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Tm_Rs_Parms, rs_tm_align_type, &Rs_Tm_Align_Type_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Rs_Parms, rs_tm_length,    &Rs_Tm_Length_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE TM RS - TYPE */
static const struct params_type Sdh_Tm_Rs_Parms_type = \
						       PARAMS_STRUCT_TYPE(Sdh_Tm_Rs_Parms, &Sdh_Tm_Rs_Parms_fields);


/* TM Higher Order Path - HP*/

/*  ENUM HP TM ALIGN - ID */
#define HP_TM_ALIGN_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Hp_Tm_Align_Type_list[] = \
{HP_TM_ALIGN_SETTING{NULL, -1}};
#undef HP_TM_ALIGN_SETTING_INIT

/*  ENUM HP TM ALIGN  - TYPE */
static const struct params_type Hp_Tm_Align_Type_type = \
							PARAMS_ID_TYPE(Hp_Tm_Align_Type_list, sizeof(Hp_Tm_Align_Type_E));


/*  ENUM HP TM LENGTH - ID */
#define HP_TM_LENGTH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Hp_Tm_Length_list[] = \
{HP_TM_LENGTH_SETTING{NULL, -1}};
#undef HP_TM_LENGTH_SETTING_INIT

/*  ENUM HP TM LENGTH - TYPE */
static const struct params_type Hp_Tm_Length_type = \
						    PARAMS_ID_TYPE(Hp_Tm_Length_list, sizeof(Hp_Tm_Length_E));


/* STM1 CORE HP Parms - FIELD */
static const struct params_field Sdh_Tm_Hp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Tm_Hp_Parms, hp_tm_align_type, &Hp_Tm_Align_Type_type ),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Hp_Parms, hp_tm_length, &Hp_Tm_Length_type ),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE HP Parms - TYPE */
static const struct params_type Sdh_Tm_Hp_Parms_type = \
						       PARAMS_STRUCT_TYPE(Sdh_Tm_Hp_Parms, &Sdh_Tm_Hp_Parms_fields);

/* TM Lower Order Path - LP*/

/*  ENUM LP TM ALIGN - ID */
#define LP_TM_ALIGN_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Lp_Tm_Align_Type_list[] = \
{LP_TM_ALIGN_SETTING{NULL, -1}};
#undef LP_TM_ALIGN_SETTING_INIT

/*  ENUM LP TM ALIGN  - TYPE */
static const struct params_type Lp_Tm_Align_Type_type = \
							PARAMS_ID_TYPE(Lp_Tm_Align_Type_list, sizeof(Lp_Tm_Align_Type_E));


/*  ENUM LP TM LENGTH - ID */
#define LP_TM_LENGTH_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Lp_Tm_Length_list[] = \
{LP_TM_LENGTH_SETTING{NULL, -1}};
#undef LP_TM_LENGTH_SETTING_INIT

/*  ENUM LP TM LENGTH - TYPE */
static const struct params_type Lp_Tm_Length_type = \
						    PARAMS_ID_TYPE(Lp_Tm_Length_list, sizeof(Lp_Tm_Length_E));


/* STM1 CORE LP Parms - FIELD */
static const struct params_field Sdh_Tm_Lp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Tm_Lp_Parms, lp_tm_align_type, &Lp_Tm_Align_Type_type ),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Lp_Parms, lp_tm_length, &Lp_Tm_Length_type ),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE LP Parms - TYPE */
static const struct params_type Sdh_Tm_Lp_Parms_type = \
						       PARAMS_STRUCT_TYPE(Sdh_Tm_Lp_Parms, &Sdh_Tm_Lp_Parms_fields);


/* Type for field 1D 'tm_data' */
static const struct params_type tm_data_type = \
					       PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->tm_data));

/* Type for field 1D 'cmp_tm_data' */
static const struct params_type cmp_tm_data_type = \
						   PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->cmp_tm_data));

/* Type for field 1D 'vc_crc' */
static const struct params_type vc_crc_type = \
					      PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->vc_crc));

/* Type for field 1D 'cmp_vc_crc' */
static const struct params_type cmp_vc_crc_type = \
						  PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->cmp_vc_crc));

/* Type for field 1D 'tu_crc' */
static const struct params_type tu_crc_type = \
					      PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->tu_crc));

/* Type for field 1D 'cmp_tu_crc' */
static const struct params_type cmp_tu_crc_type = \
						  PARAMS_FIXEDSTRING_TYPE(sizeof(((struct Sdh_Tm_Data *)0)->cmp_tu_crc));


/* Type for field 2D 'vc_data' */
static const struct params_type vc_data_type =   \
						 PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								 sizeof(params_type_char), MAX_VC_DATA, "vc_data");

static const struct params_type vc_data_type2 =  \
						 PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								 MAX_VC_DATA*sizeof(params_type_char),
								 MAX_DATA_WIDTH + 1, "vc_data2");

/* Type for field 2D 'cmp_vc_data' */
static const struct params_type cmp_vc_data_type =   \
						     PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								     sizeof(params_type_char), MAX_VC_DATA, "cmp_vc_data");

static const struct params_type cmp_vc_data_type2 =  \
						     PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								     MAX_VC_DATA*sizeof(params_type_char),
								     MAX_DATA_WIDTH + 1, "cmp_vc_data2");

/* Type for field 2D 'tu_data' */
static const struct params_type tu_data_type =   \
						 PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								 sizeof(params_type_char), MAX_VC_DATA, "tu_data");

static const struct params_type tu_data_type2 =  \
						 PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								 MAX_VC_DATA*sizeof(params_type_char),
								 MAX_DATA_WIDTH + 1, "tu_data2");

/* Type for field 2D 'cmp_tu_data' */
static const struct params_type cmp_tu_data_type =   \
						     PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								     sizeof(params_type_char), MAX_VC_DATA, "cmp_tu_data");

static const struct params_type cmp_tu_data_type2 =  \
						     PARAMS_FIXEDARRAY_TYPE(&params_type_char,
								     MAX_VC_DATA*sizeof(params_type_char),
								     MAX_DATA_WIDTH + 1, "cmp_tu_data2");

/* STM1 TRACE DATA - FIELD */
static const struct params_field Sdh_Tm_Data_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Tm_Data, tm_data, &tm_data_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_tm_data, &cmp_tm_data_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, vc_data, &vc_data_type2),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_vc_data, &cmp_vc_data_type2),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, tu_data, &tu_data_type2),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_tu_data, &cmp_tu_data_type2),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, tm_crc, &params_type_char),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_tm_crc, &params_type_char),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, vc_crc, &vc_crc_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_vc_crc, &cmp_vc_crc_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, tu_crc, &tu_data_type),
		PARAMS_STRUCT_FIELD(Sdh_Tm_Data, cmp_tu_crc, &cmp_tu_data_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 TRACE DATA - TYPE */
static const struct params_type Sdh_Tm_Data_type = \
						   PARAMS_STRUCT_TYPE(Sdh_Tm_Data, &Sdh_Tm_Data_fields);


/* STM1 CORE Trace Msg Cfg- FIELD */
static const struct params_field Stm1_Trace_Msg_Cfg_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Trace_Msg_Cfg, 	rs_parms, 		&Sdh_Tm_Rs_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Trace_Msg_Cfg, 	hp_parms, 		&Sdh_Tm_Hp_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Trace_Msg_Cfg, 	lp_parms, 		&Sdh_Tm_Lp_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Trace_Msg_Cfg, 	trace_data, 		&Sdh_Tm_Data_type),
		PARAMS_STRUCT_FIELD(Stm1_Trace_Msg_Cfg, 	status,			&params_type_uchar),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE Trace Msg Cfg- TYPE */
static const struct params_type Stm1_Trace_Msg_Cfg_type = \
							  PARAMS_STRUCT_TYPE(Stm1_Trace_Msg_Cfg, &Stm1_Trace_Msg_Cfg_fields);

/* STM1 CORE Trace Msg Cfg - ARRAY TYPE */
static const struct params_type Stm1_Trace_Msg_Cfg_array_type = \
								PARAMS_ARRAY_TYPE(&Stm1_Trace_Msg_Cfg_type, "Stm1TraceMsgCfg", "Stm1TraceMsgCfg");


/* STM1 CORE Trace-Msg  - FIELD */
static const struct params_field Stm1_Core_Trace_Msg_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Core_Trace_Msg, version,       &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Core_Trace_Msg, num_cards,     &params_type_uchar),
		PARAMS_STRUCT_FIELD(Stm1_Core_Trace_Msg, sdh_card_info, &Stm1_Card_Info_array_type),
		PARAMS_STRUCT_FIELD(Stm1_Core_Trace_Msg, ptr_trace_cfg,  &Stm1_Trace_Msg_Cfg_array_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE Trace-Msg - TYPE */
static const struct params_type Stm1_Core_Trace_Msg_type = \
							   PARAMS_STRUCT_TYPE(Stm1_Core_Trace_Msg, &Stm1_Core_Trace_Msg_fields);


/* STM1 CORE Core Config - FIELD */
static const struct params_field Stm1_Core_Config_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Core_Config, md_config, &Stm1_Core_Mux_Demux_type),
		PARAMS_STRUCT_FIELD(Stm1_Core_Config, tm_config, &Stm1_Core_Trace_Msg_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CORE Core Config - TYPE */
static const struct params_type Stm1_Core_Config_type = \
							PARAMS_STRUCT_TYPE(Stm1_Core_Config, &Stm1_Core_Config_fields);

/*
   |*********** End Core CONFIGURATION ***********|
 */


/*
   |*********** Start CHIP CONFIGURATION ***********|
 */

/*  ENUM STM1 Protection Algo - ID */
#define SDH_STM1_PROTECTION_ALGO_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Prot_Algo_list[] = \
{SDH_STM1_PROTECTION_ALGO{NULL, -1}};
#undef SDH_STM1_PROTECTION_ALGO_INIT

/*  ENUM STM1 Protection Algo - TYPE */
static const struct params_type Sdh_Stm1_Prot_Algo_type = \
							  PARAMS_ID_TYPE(Sdh_Stm1_Prot_Algo_list, sizeof(Sdh_Stm1_Prot_Algo_E));

/* STM1 CHIP MSP Parms - FIELD */
static const struct params_field Sdh_Msp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Msp_Parms, msp_working_prot, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Msp_Parms, msp_revertive, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Msp_Parms, msp_wait_res_period, &params_type_ushort),
		PARAMS_STRUCT_FIELD(Sdh_Msp_Parms, msp_ex_err_defect, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Msp_Parms, msp_degrade_sig_defect, &params_type_uchar),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CHIP MSP Parms - TYPE */
static const struct params_type Sdh_Msp_Parms_type = \
						     PARAMS_STRUCT_TYPE(Sdh_Msp_Parms, &Sdh_Msp_Parms_fields);



/* STM1 CHIP SNCP Couplet - FIELD */
static const struct params_field Sdh_sncp_couplet_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_sncp_couplet, working_channel, &params_type_int),
		PARAMS_STRUCT_FIELD(Sdh_sncp_couplet, prot_channel, &params_type_int),
		PARAMS_STRUCT_FIELD(Sdh_sncp_couplet, config_flag, &params_type_char),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CHIP SNCP Couplet - TYPE */
static const struct params_type Sdh_sncp_couplet_type = \
							PARAMS_STRUCT_TYPE(Sdh_sncp_couplet, &Sdh_sncp_couplet_fields);

/* STM1 CHIP SNCP Parms - FIELD */
static const struct params_field Sdh_Sncp_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, sncp_wait_res_period, &params_type_ushort),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, active_path, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, hold_off_time, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, sncp_couplet, &Sdh_sncp_couplet_type),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, enable_protection, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, revertive, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, mode, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, degrade_sig_defect, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Sncp_Parms, ex_err_defect, &params_type_uchar),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CHIP SNCP Parms - TYPE */
static const struct params_type Sdh_Sncp_Parms_type = \
						      PARAMS_STRUCT_TYPE(Sdh_Sncp_Parms, &Sdh_Sncp_Parms_fields);


/* Type for field 1D - sncp_prot_data[MAX_NO_STM_PROTECTION] */
static const struct params_type sncp_prot_data_type = \
						      PARAMS_FIXEDARRAY_TYPE(&Sdh_Sncp_Parms_type,
						             sizeof(Sdh_Sncp_Parms_S), MAX_NO_STM_PROTECTION, "sncp_prot_data");

/* STM1 CHIP SNCP Config - FIELD */
static const struct params_field Sdh_Sncp_Config_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Sncp_Config, sncp_prot_data, &sncp_prot_data_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CHIP SNCP Config - TYPE */
static const struct params_type Sdh_Sncp_Config_type = \
						       PARAMS_STRUCT_TYPE(Sdh_Sncp_Config, &Sdh_Sncp_Config_fields);


/*  ENUM Local Command Request - ID */
#define SDH_STM1_LCR_CMD_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Lcr_Cmd_list[] = \
{SDH_STM1_LCR_CMD{NULL, -1}};
#undef SDH_STM1_LCR_CMD_INIT

/*  ENUM Local Command Request - TYPE */
static const struct params_type Sdh_Stm1_Lcr_Cmd_type = \
							PARAMS_ID_TYPE(Sdh_Stm1_Lcr_Cmd_list, sizeof(Sdh_Stm1_Lcr_Cmd_E));


/* STM1 CHIP LCR Config - FIELD */
static const struct params_field Sdh_Prot_Sw_lcr_fields[] = \
{
	PARAMS_STRUCT_FIELD(Sdh_Prot_Sw_lcr, version, &params_type_float),
		PARAMS_STRUCT_FIELD(Sdh_Prot_Sw_lcr, sncp_lcr_cmd[MAX_NO_STM_PROTECTION], &params_type_uchar),
		PARAMS_STRUCT_FIELD(Sdh_Prot_Sw_lcr, msp_lcr_cmd, &Sdh_Stm1_Lcr_Cmd_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 CHIP LCR Config - TYPE */
static const struct params_type Sdh_Prot_Sw_lcr_type = \
						       PARAMS_STRUCT_TYPE(Sdh_Prot_Sw_lcr, &Sdh_Prot_Sw_lcr_fields);

/* STM1 Protection Parameters - FIELD */
static const struct params_field Stm1_Prot_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, version, &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, prot_algo, &Sdh_Stm1_Prot_Algo_type),
		PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, max_sncp_config, &params_type_uchar),
		PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, msp_parms, &Sdh_Msp_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, sncp_parms, &Sdh_Sncp_Config_type),
		PARAMS_STRUCT_FIELD(Stm1_Prot_Parms, lcr_parms, &Sdh_Prot_Sw_lcr_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 Protection Parameters - TYPE */
static const struct params_type Stm1_Prot_Parms_type = \
						       PARAMS_STRUCT_TYPE(Stm1_Prot_Parms, &Stm1_Prot_Parms_fields);


/* TIME SYNC */


/*  ENUM Sdh_Stm1_Sync_Mode - ID */
#define STM1_SYNC_MODE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Sync_Mode_list[] = \
{STM1_SYNC_MODE_SETTING{NULL, -1}};
#undef STM1_SYNC_MODE_SETTING_INIT 

/*  ENUM Sdh_Stm1_Sync_Mode - TYPE */
static const struct params_type Sdh_Stm1_Sync_Mode_type = \
							  PARAMS_ID_TYPE(Sdh_Stm1_Sync_Mode_list, sizeof(Sdh_Stm1_Sync_Mode_E));
#if 0
/*  ENUM SONET MEDIUM TYPE - ID */
#define SONET_MEDIUM_TYPE_SETTING_INIT(label, value, name) {name, label},
static const struct params_id Sonet_Medium_Type_list[] = \
{SONET_MEDIUM_TYPE_SETTING{NULL, -1}};
#undef SONET_MEDIUM_TYPE_SETTING_INIT

/*  ENUM SONET MEDIUM TYPE- TYPE */
static const struct params_type Sonet_Medium_Type_type = \
							 PARAMS_ID_TYPE(Sonet_Medium_Type_list, sizeof(Sonet_Medium_Type_E));
#endif

/*  ENUM E1 CLock - ID */
#define STATIC_E1_CLOCK_SDH_CODE_INIT(label, value, name) {name, label},
static const struct params_id Sdh_E1_Clock_list[] = \
{STATIC_E1_CLOCK_SDH_CODE{NULL, -1}};
#undef STATIC_E1_CLOCK_SDH_CODE_INIT

/*  ENUM E1 CLock - TYPE */
static const struct params_type Sdh_E1_Clock_type = \
						    PARAMS_ID_TYPE(Sdh_E1_Clock_list, sizeof(Sdh_E1_Clock_E));

/*  ENUM Time Reference - ID */
#define STM1_TIME_REF_INIT(label, value, name) {name, label},
static const struct params_id Sdh_Stm1_Time_Ref_list[] = \
{STM1_TIME_REF{NULL, -1}};
#undef STM1_TIME_REF_INIT

/*  ENUM Time Reference - TYPE */
static const struct params_type Sdh_Stm1_Time_Ref_type = \
							 PARAMS_ID_TYPE(Sdh_Stm1_Time_Ref_list, sizeof(Sdh_Stm1_Time_Ref_E));

/* STM1 Time Sync Parameters - FIELD */
static const struct params_field Stm1_Sync_Parms_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, version, &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, sync_mode, &Sdh_Stm1_Sync_Mode_type),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, medium_type, &Sonet_Medium_Type_type),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, e1_clock_quality, &Sdh_E1_Clock_type),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, primary, &Sdh_Stm1_Time_Ref_type),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, secondary, &Sdh_Stm1_Time_Ref_type),
		PARAMS_STRUCT_FIELD(Stm1_Sync_Parms, tertiary, &Sdh_Stm1_Time_Ref_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 Time Sync Parameters - TYPE */
static const struct params_type Stm1_Sync_Parms_type = \
						       PARAMS_STRUCT_TYPE(Stm1_Sync_Parms, &Stm1_Sync_Parms_fields);

/*  ENUM Crosspoint - ID */
static const struct params_id STM1_Crosspoint_Type_list[] = \
{{"Manual", STM1_CROSSPOINT_MANUAL},{NULL, -1}};

/*  ENUM Crosspoint - TYPE */
static const struct params_type STM1_Crosspoint_Type_type = \
							    PARAMS_ID_TYPE(STM1_Crosspoint_Type_list, sizeof(STM1_Crosspoint_Type_E));


/* STM1 Crosspoint Config - FIELD */
static const struct params_field Stm1_Cp_Config_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Cp_Config, version, &params_type_float),
		PARAMS_STRUCT_FIELD(Stm1_Cp_Config, channelmap_type, &STM1_Crosspoint_Type_type),
		PARAMS_STRUCT_FIELD(Stm1_Cp_Config, num_of_stm1_mapping, &params_type_uint),
		PARAMS_STRUCT_FIELD(Stm1_Cp_Config, chmap_data_size, &params_type_uint),
		PARAMS_STRUCT_FIELD(Stm1_Cp_Config, chmap_data, &params_type_string),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 Crosspoint Config - TYPE */
static const struct params_type Stm1_Cp_Config_type = \
						      PARAMS_STRUCT_TYPE(Stm1_Cp_Config, &Stm1_Cp_Config_fields);

/* STM1 Chip Config - FIELD */
static const struct params_field Stm1_Chip_Config_fields[] = \
{
	PARAMS_STRUCT_FIELD(Stm1_Chip_Config, prot_parms, &Stm1_Prot_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Chip_Config, stm1_sync , &Stm1_Sync_Parms_type),
		PARAMS_STRUCT_FIELD(Stm1_Chip_Config, stm1_cp   , &Stm1_Cp_Config_type),
		PARAMS_STRUCT_FIELD_END
};

/* STM1 Chip Config - TYPE */
static const struct params_type Stm1_Chip_Config_type = \
							PARAMS_STRUCT_TYPE(Stm1_Chip_Config, &Stm1_Chip_Config_fields);

/*
   |*********** End CHIP CONFIGURATION ***********|
 */


/*******************************************************************************
 * Function Definitions
 ******************************************************************************/

/******************************************************************************
 * Description          : This function is used to initialize the database buffers
 *
 * Inputs               : Sdh_Stm1_Features_E - database buffer information to be initialized
 *                        void * 	      - database buffer to be initialized
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 dbl_sdh_stm1_init_config_buffer(Sdh_Stm1_Features_E stm1_feature, void *ptr_stm1_cfg)
{
	PAL_INT_32 ret_val = STM1_SUCCESS;

	const struct params_type *port_type   = &Stm1_Port_Config_type;
	const struct params_type *protection_type = &Stm1_Prot_Parms_type;
	const struct params_type *protection_lcr_type = &Sdh_Prot_Sw_lcr_type;
	const struct params_type *crosspoint_type = &Stm1_Cp_Config_type;
	const struct params_type *muxdemux_type = &Stm1_Core_Mux_Demux_type;
	const struct params_type *tracemsg_type = &Stm1_Trace_Msg_Cfg_type;
	const struct params_type *timesync_type = &Stm1_Sync_Parms_type;

	Stm1_Port_Config_S *ptr_stm1_port_cfg;
	Stm1_Prot_Parms_S *ptr_stm1_prot_cfg;
	Sdh_Prot_Sw_lcr_S *ptr_stm1_prot_lcr_cfg;
	Stm1_Cp_Config_S *ptr_stm1_cp_cfg;
	Stm1_Core_Mux_Demux_S *ptr_stm1_mdm_cfg;
	Stm1_Trace_Msg_Cfg_S *ptr_stm1_tm_cfg;
	Stm1_Sync_Parms_S *ptr_stm1_sync_cfg;

	switch(stm1_feature)
	{
		case STM1_PORT_CONFIG:
			ptr_stm1_port_cfg = (Stm1_Port_Config_S *)ptr_stm1_cfg;	
		
			// Initialize STM1 port cfg params structure
			if (-1 == params_init(port_type, NULL, ptr_stm1_port_cfg))
			{
				printf("%s: params_init() failed to initialize STM1 Port config \n", __func__);
				ret_val = STM1_FAILURE;
			}
			break;

		case STM1_PROTECTION_SW_CONFIG:
			{
				ptr_stm1_prot_cfg = ptr_stm1_cfg;

				// Initialize STM1 Protection Switch cfg params structure
				if (-1 == params_init(protection_type, NULL, ptr_stm1_prot_cfg))
				{
					printf("%s: params_init() failed to initialize STM1 Protection Switch config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_PROTECTION_SW_LCR_CONFIG:
			{ 
				ptr_stm1_prot_lcr_cfg = ptr_stm1_cfg;

				// Initialize STM1 Protection Switch cfg params structure
				if (-1 == params_init(protection_lcr_type, NULL, ptr_stm1_prot_lcr_cfg))
				{
				      printf("%s: params_init() failed to initialize STM1Protection Switch lcr config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_CROSSPOINT_CONFIG:
			{
				ptr_stm1_cp_cfg = ptr_stm1_cfg;

				// Initialize STM1 Crosspoint cfg params structure
				if (-1 == params_init(crosspoint_type, NULL, ptr_stm1_cp_cfg))
				{
					printf("%s: params_init() failed to initialize STM1 Crosspoint config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_MUX_DEMUX_CONFIG:
			{
				ptr_stm1_mdm_cfg = ptr_stm1_cfg;

				// Initialize STM1 Mux Demux cfg params structure
				if (-1 == params_init(muxdemux_type, NULL, ptr_stm1_mdm_cfg))
				{
					printf("%s: params_init() failed to initialize STM1 Mux-Demux config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_TRACE_MSG_CONFIG:
			{
				ptr_stm1_tm_cfg = ptr_stm1_cfg;

				// Initialize the STM1 trace message cfg params structure
				if (-1 == params_init(tracemsg_type, NULL, ptr_stm1_tm_cfg))
				{
					printf("%s: params_init() failed to initialize STM1 Trace Msg config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_SYNC_CONFIG:
			{
				ptr_stm1_sync_cfg = ptr_stm1_cfg;

				// Initialize STM1 Time Sync cfg params structure
				if (-1 == params_init(timesync_type, NULL, ptr_stm1_sync_cfg))
				{
					printf("%s: params_init() failed to initialize STM1 Time Sync config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}

		default:
			printf("Invalid Feature \n");
			break;
	}


     return ret_val;
}

/******************************************************************************
 * Description          : This function is used to de-allocate the memory defined
 *                                                         for STM1 configuration
 *
 * Inputs               : Sdh_Stm1_Features_E - Feature information
 *                        void * 	      - Configuration buffer to be de-allocated
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/

PAL_INT_32 dbl_sdh_stm1_release_config_buffer(Sdh_Stm1_Features_E stm1_feature, void *ptr_stm1_cfg)
{
	PAL_INT_32 ret_val = STM1_SUCCESS;

	const struct params_type *port_type   = &Stm1_Port_Config_type;
	const struct params_type *protection_type = &Stm1_Prot_Parms_type;
	const struct params_type *protection_lcr_type = &Sdh_Prot_Sw_lcr_type;
	const struct params_type *crosspoint_type = &Stm1_Cp_Config_type;
	const struct params_type *muxdemux_type = &Stm1_Core_Mux_Demux_type;
	const struct params_type *tracemsg_type = &Stm1_Trace_Msg_Cfg_type;
	const struct params_type *timesync_type = &Stm1_Sync_Parms_type;
	Stm1_Port_Config_S *ptr_stm1_port_cfg; 
	Stm1_Prot_Parms_S *ptr_stm1_prot_cfg; 
	Sdh_Prot_Sw_lcr_S *ptr_stm1_prot_lcr_cfg;
	Stm1_Cp_Config_S *ptr_stm1_cp_cfg;
	Stm1_Core_Mux_Demux_S *ptr_stm1_mdm_cfg;
	Stm1_Trace_Msg_Cfg_S *ptr_stm1_tm_cfg;
	Stm1_Sync_Parms_S *ptr_stm1_sync_cfg;

	switch(stm1_feature)
	{
		case STM1_PORT_CONFIG:
			ptr_stm1_port_cfg = ptr_stm1_cfg;

			// Release STM1 port cfg params structure
			if (-1 == params_free(port_type, NULL, ptr_stm1_port_cfg))
			{
				printf("%s: params_free() failed to release STM1 Port config \n", __func__);
				ret_val = STM1_FAILURE;
			}
			break;

		case STM1_PROTECTION_SW_CONFIG:
			{
				ptr_stm1_prot_cfg = ptr_stm1_cfg;
				// Release STM1 Protection Switch cfg params structure
				if (-1 == params_free(protection_type, NULL, ptr_stm1_prot_cfg))
				{
					printf("%s: params_free() failed to release STM1 Protection Switch config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_PROTECTION_SW_LCR_CONFIG:
			{ 
				ptr_stm1_prot_lcr_cfg = ptr_stm1_cfg;
				// Release STM1 Protection Switch cfg params structure
				if (-1 == params_free(protection_lcr_type, NULL, ptr_stm1_prot_lcr_cfg))
				{
				      printf("%s: params_free() failed to release STM1Protection Switch lcr config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_CROSSPOINT_CONFIG:
			{
				ptr_stm1_cp_cfg = ptr_stm1_cfg;
				// Release STM1 Crosspoint cfg params structure
				if (-1 == params_init(crosspoint_type, NULL, ptr_stm1_cp_cfg))
				{
					printf("%s: params_free() failed to Release STM1 Crosspoint config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_MUX_DEMUX_CONFIG:
			{
				ptr_stm1_mdm_cfg = ptr_stm1_cfg;
				// Release STM1 Mux Demux cfg params structure
				if (-1 == params_init(muxdemux_type, NULL, ptr_stm1_mdm_cfg))
				{
					printf("%s: params_free() failed to Release STM1 Mux-Demux config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_TRACE_MSG_CONFIG:
			{
				ptr_stm1_tm_cfg = ptr_stm1_cfg;
				// Release STM1 trace message cfg params structure
				if (-1 == params_init(tracemsg_type, NULL, ptr_stm1_tm_cfg))
				{
					printf("%s: params_free() failed to Release STM1 Trace Msg config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}
		case STM1_SYNC_CONFIG:
			{
				ptr_stm1_sync_cfg = ptr_stm1_cfg;

				// Release STM1 Time Sync cfg params structure
				if (-1 == params_init(timesync_type, NULL, ptr_stm1_sync_cfg))
				{
					printf("%s: params_free() failed to Release STM1 Time Sync config \n", __func__);
					ret_val = STM1_FAILURE;
				}
				break;
			}

		default:
			printf("Invalid Feature \n");
			break;
	}

     return ret_val;
}


/******************************************************************************
 * Description          : This function is used to initialize and update the
 *                                        variable length array with the latest card information
 *
 * Inputs               : Stm1_Port_Config_S - buffer to be initialize
 *                        SDH_Hw_Max_Interface_S - card information to be copied
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 dbl_sdh_stm1_card_info_update(void *ptr_stm1_cfg, \
		Stm1_Sdh_Hw_Max_Interface_S stm1_hw_info, Sdh_Stm1_Features_E stm1_feature)
{
	PAL_INT_32 index = 0, max_index = 0, ret_val = STM1_SUCCESS;
	max_index = stm1_hw_info.card_num;

        const struct params_type *port_type   = &Stm1_Port_Config_type;
        const struct params_type *muxdemux_type = &Stm1_Core_Mux_Demux_type;
        const struct params_type *tracemsg_type = &Stm1_Trace_Msg_Cfg_type;

	Stm1_Port_Config_S *ptr_stm1_port_cfg;
	Stm1_Core_Mux_Demux_S *ptr_stm1_md_cfg;
	Stm1_Core_Trace_Msg_S *ptr_stm1_tm_cfg;	

	switch(stm1_feature)
	{
		case STM1_PORT_CONFIG:

			ptr_stm1_port_cfg = (Stm1_Port_Config_S *) ptr_stm1_cfg;

			if(ptr_stm1_port_cfg->sdh_card_info.length > stm1_hw_info.card_num)
			{
				max_index = ptr_stm1_port_cfg->sdh_card_info.length;
			}

			for(index = 0;index < max_index;index++)
			{
				if(index > ptr_stm1_port_cfg->sdh_card_info.length)
				{
					params_array_insert(port_type, "sdh_card_info", index, ptr_stm1_port_cfg);
				}
				else if(index > stm1_hw_info.card_num)
				{
					params_array_delete(port_type, "sdh_card_info", index, ptr_stm1_port_cfg);
					continue;
				}
				ptr_stm1_port_cfg->sdh_card_info.elems[index].card_type = \
										      stm1_hw_info.interface_info[index].card_type;
				ptr_stm1_port_cfg->sdh_card_info.elems[index].max_stm_port_core = \
											      stm1_hw_info.interface_info[index].max_stm1_ports;
			}

			break; 
		case STM1_MUX_DEMUX_CONFIG:

			ptr_stm1_md_cfg = (Stm1_Core_Mux_Demux_S *)ptr_stm1_cfg;

			if(ptr_stm1_md_cfg->sdh_card_info.length > stm1_hw_info.card_num)
			{
				max_index = ptr_stm1_md_cfg->sdh_card_info.length;
			}

			for(index = 0;index < max_index;index++)
			{
				if(index > ptr_stm1_md_cfg->sdh_card_info.length)
				{
					params_array_insert(muxdemux_type, "sdh_card_info", index, ptr_stm1_md_cfg);
				}
				else if(index > stm1_hw_info.card_num)
				{
					params_array_delete(muxdemux_type, "sdh_card_info", index, ptr_stm1_md_cfg);
					continue;
				}
				ptr_stm1_md_cfg->sdh_card_info.elems[index].card_type = \
										    stm1_hw_info.interface_info[index].card_type;
				ptr_stm1_md_cfg->sdh_card_info.elems[index].max_stm_port_core = \
											    stm1_hw_info.interface_info[index].max_stm1_ports;
			}	

			break; 
		case STM1_TRACE_MSG_CONFIG:	

			ptr_stm1_tm_cfg = (Stm1_Core_Trace_Msg_S *)ptr_stm1_cfg;

			if(ptr_stm1_tm_cfg->sdh_card_info.length > stm1_hw_info.card_num)
			{
				max_index = ptr_stm1_tm_cfg->sdh_card_info.length;
			}

			for(index = 0;index < max_index;index++)
			{
				if(index > ptr_stm1_tm_cfg->sdh_card_info.length)
				{
					params_array_insert(tracemsg_type, "sdh_card_info", index, ptr_stm1_tm_cfg);
				}
				else if(index > stm1_hw_info.card_num)
				{
					params_array_delete(tracemsg_type, "sdh_card_info", index, ptr_stm1_tm_cfg);
					continue;
				}
				ptr_stm1_tm_cfg->sdh_card_info.elems[index].card_type = \
										    stm1_hw_info.interface_info[index].card_type;
				ptr_stm1_tm_cfg->sdh_card_info.elems[index].max_stm_port_core = \
											    stm1_hw_info.interface_info[index].max_stm1_ports;
			}

			break; 
		default:
			printf("Invalid Feature \n");


			return ret_val;
	}
}


/******************************************************************************
 * Description          : This function is used to initialize and update the
 *                                        variable length array with the default configuration for
 *                                        STM1 Port configuration
 *
 * Inputs               : Stm1_Port_Config_S - buffer to be initialize
 *                        Stm1_Port_Params_S - buffer with default configuration value
 *                        PAL_INT_32 - provide the info about defaulting the configuration
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 dbl_sdh_stm1_config_info_update(void *ptr_stm1_cfg,\
		void *ptr_stm1_dflt_cfg, PAL_INT_32 default_cfg, Sdh_Stm1_Features_E stm1_feature)
{
        PAL_INT_32 index = 0, max_index = 0, cur_max_stm1_ports = 0, max_stm1_ports = 0;

        const struct params_type *port_type   = &Stm1_Port_Config_type;
        const struct params_type *muxdemux_type = &Stm1_Core_Mux_Demux_type;
        const struct params_type *tracemsg_type = &Stm1_Trace_Msg_Cfg_type;

	// C-struct Instances
	Stm1_Port_Config_S *ptr_stm1_port_cfg;
        Stm1_Core_Trace_Msg_S *ptr_stm1_tm_cfg;
        Stm1_Core_Mux_Demux_S *ptr_stm1_md_cfg;

	// default Instances
	Stm1_Port_Params_S *ptr_stm1_port_dflt;
	Stm1_Mux_Demux_Cfg_S *ptr_stm1_md_dflt;
	Stm1_Trace_Msg_Cfg_S *ptr_stm1_tm_dflt;

	switch(stm1_feature)
        {
                case STM1_PORT_CONFIG:
               
                        ptr_stm1_port_cfg = (Stm1_Port_Config_S *) ptr_stm1_cfg;
                        ptr_stm1_port_dflt = (Stm1_Port_Params_S *) ptr_stm1_dflt_cfg;
                
                        max_index = ptr_stm1_port_cfg->sdh_card_info.length;
        		if(0 == max_index)
        		{	
                		printf(" %s: There is no STM1 Port list wrt card  \n", __func__);
                		return STM1_SUCCESS;
        		}

			cur_max_stm1_ports = 0;
			for(index = 0;index < max_index;index++)
			{
				cur_max_stm1_ports += ptr_stm1_port_cfg->sdh_card_info.elems[index].max_stm_port_core;
			}

			max_stm1_ports = ptr_stm1_port_cfg->port_params.length;

			if(cur_max_stm1_ports > ptr_stm1_port_cfg->port_params.length)
			{
				max_stm1_ports = cur_max_stm1_ports;
			}
			for(index = 0;index < max_stm1_ports;index++)
			{
				if(index > ptr_stm1_port_cfg->port_params.length)
				{
					params_array_insert(port_type, "port_params", index, ptr_stm1_port_cfg);
				}
				else if(index > cur_max_stm1_ports)
				{
					params_array_delete(port_type, "port_params", index, ptr_stm1_port_cfg);
					continue;
				}
				if(1 == default_cfg)
				{
					ptr_stm1_port_cfg->port_params.elems[index] = ptr_stm1_port_dflt[0];
				}
			}
			break;
		 case STM1_MUX_DEMUX_CONFIG:

                        ptr_stm1_md_cfg = (Stm1_Core_Mux_Demux_S *) ptr_stm1_cfg;
                        ptr_stm1_md_dflt = (Stm1_Mux_Demux_Cfg_S *) ptr_stm1_dflt_cfg;
                
                        max_index = ptr_stm1_md_cfg->sdh_card_info.length;
        		if(0 == max_index)
        		{	
                		printf(" %s: There is no STM1 Port list wrt card  \n", __func__);
                		return STM1_SUCCESS;
        		}

			cur_max_stm1_ports = 0;
			for(index = 0;index < max_index;index++)
			{
				cur_max_stm1_ports += ptr_stm1_md_cfg->sdh_card_info.elems[index].max_stm_port_core;
			}

			max_stm1_ports = ptr_stm1_md_cfg->ptr_core_cfg.length;

			if(cur_max_stm1_ports > ptr_stm1_md_cfg->ptr_core_cfg.length)
			{
				max_stm1_ports = cur_max_stm1_ports;
			}
			for(index = 0;index < max_stm1_ports;index++)
			{
				if(index > ptr_stm1_md_cfg->ptr_core_cfg.length)
				{
					params_array_insert(muxdemux_type, "ptr_core_cfg", index, ptr_stm1_md_cfg);
				}
				else if(index > cur_max_stm1_ports)
				{
					params_array_delete(muxdemux_type, "ptr_core_cfg", index, ptr_stm1_md_cfg);
					continue;
				}
				if(1 == default_cfg)
				{
					ptr_stm1_md_cfg->ptr_core_cfg.elems[index] = ptr_stm1_md_dflt[0];
				}
			}
			break;
		 case STM1_TRACE_MSG_CONFIG:

                        ptr_stm1_tm_cfg = (Stm1_Core_Trace_Msg_S *) ptr_stm1_cfg;
                        ptr_stm1_tm_dflt = (Stm1_Trace_Msg_Cfg_S *) ptr_stm1_dflt_cfg;
                
                        max_index = ptr_stm1_tm_cfg->sdh_card_info.length;
        		if(0 == max_index)
        		{	
                		printf(" %s: There is no STM1 Port list wrt card  \n", __func__);
                		return STM1_SUCCESS;
        		}

			cur_max_stm1_ports = 0;
			for(index = 0;index < max_index;index++)
			{
				cur_max_stm1_ports += ptr_stm1_tm_cfg->sdh_card_info.elems[index].max_stm_port_core;
			}

			max_stm1_ports = ptr_stm1_tm_cfg->ptr_trace_cfg.length;

			if(cur_max_stm1_ports > ptr_stm1_tm_cfg->ptr_trace_cfg.length)
			{
				max_stm1_ports = cur_max_stm1_ports;
			}
			for(index = 0;index < max_stm1_ports;index++)
			{
				if(index > ptr_stm1_tm_cfg->ptr_trace_cfg.length)
				{
					params_array_insert(tracemsg_type, "ptr_trace_cfg", index, ptr_stm1_tm_cfg);
				}
				else if(index > cur_max_stm1_ports)
				{
					params_array_delete(tracemsg_type, "ptr_trace_cfg", index, ptr_stm1_tm_cfg);
					continue;
				}
				if(1 == default_cfg)
				{
					ptr_stm1_tm_cfg->ptr_trace_cfg.elems[index] = ptr_stm1_tm_dflt[0];
				}
			}
			break;
		default:
			printf("Invalid Feature \n");
	}
}


/******************************************************************************
 * Description          : This function is used to initialize and update the
 *                                        variable length array with the default configuration for
 *                                        STM1 Port configuration
 *
 * Inputs               : Stm1_Port_Config_S - buffer to be initialize
 *                        Stm1_Sdh_Hw_Max_Interface_S - size of the variable length array
 *                        			    shall be calculated from card information
 *                        PAL_INT_32 - provide the info about defaulting the configuration
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 dbl_sdh_stm1_cfg_update(void *ptr_stm1_cfg,\
		Stm1_Sdh_Hw_Max_Interface_S stm1_hw_info,\
		PAL_INT_32 default_cfg,
		Sdh_Stm1_Features_E stm1_feature)
{

	Stm1_Port_Params_S *ptr_stm1_port_cfg;
	Stm1_Core_Mux_Demux_S *ptr_stm1_md_cfg;
	Stm1_Core_Trace_Msg_S *ptr_stm1_tm_cfg;

	Stm1_Trace_Msg_Cfg_S default_tm_cfg;

	switch(stm1_feature)
	{
		case STM1_PORT_CONFIG:

			ptr_stm1_port_cfg = (Stm1_Port_Params_S *) ptr_stm1_cfg;

			/* Read the STM1 Port configuration default value */
#define STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandStriing)   default_value,
			Stm1_Port_Params_S def_port_cfg_inst = {STM1_PORT_CFG_INST_LIST};
#undef STM1_PARAM_INIT

			/* Update the STM1 card information with stm1_hw_info variable */
			dbl_sdh_stm1_card_info_update(ptr_stm1_port_cfg, stm1_hw_info, stm1_feature);

			/* Update the STM1 Port configuration default values */
			dbl_sdh_stm1_config_info_update(ptr_stm1_port_cfg,
					&def_port_cfg_inst, default_cfg, STM1_PORT_CONFIG);
			break;

		case STM1_MUX_DEMUX_CONFIG:

			ptr_stm1_md_cfg  = (Stm1_Core_Mux_Demux_S *)ptr_stm1_cfg;
			
			/* Read the STM1 Mux Demux configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
                        Stm1_Mux_Demux_Cfg_S default_muxdemux_cfg = {SDH_STM1_MUX_CFG_LIST};
#undef SDH_STM1_PARAM_INIT
			
                        /* Read the STM1 MD-RS configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
                        Sdh_Rs_Parms_S RS_Parms = {SDH_STM1_MUX_RS_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

                        /* Read the STM1 MD-MS configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
                        Sdh_Ms_Parms_S MS_Parms = {SDH_STM1_MUX_MS_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

                        /* Read the STM1 MD-HP configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
                        Sdh_Hp_Parms_S HP_Parms = {SDH_STM1_MUX_HP_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

                        /* Read the STM1 MD-LP configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
                        Sdh_Lp_Parms_S LP_Parms = {SDH_STM1_MUX_LP_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

			memcpy(&default_muxdemux_cfg.rs_parms, &RS_Parms, sizeof(Sdh_Rs_Parms_S));
			memcpy(&default_muxdemux_cfg.ms_parms, &MS_Parms, sizeof(Sdh_Ms_Parms_S));
			memcpy(&default_muxdemux_cfg.hp_parms, &HP_Parms, sizeof(Sdh_Hp_Parms_S));
			memcpy(&default_muxdemux_cfg.lp_parms, &LP_Parms, sizeof(Sdh_Lp_Parms_S));
	
                        /* Update the STM1 card information with stm1_hw_info variable */
                        dbl_sdh_stm1_card_info_update(ptr_stm1_md_cfg, stm1_hw_info, STM1_MUX_DEMUX_CONFIG);

                        /* Update the STM1 MuxDemux - RS configuration default values */
                        dbl_sdh_stm1_config_info_update(ptr_stm1_md_cfg,
                                        &default_muxdemux_cfg, default_cfg, STM1_MUX_DEMUX_CONFIG);
			break;

		case STM1_TRACE_MSG_CONFIG:
			
			ptr_stm1_tm_cfg  = ptr_stm1_cfg;
			

			/* Read the STM1 TM-RS configuration default value */
			default_tm_cfg.status = DISABLE;
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
			Sdh_Tm_Rs_Parms_S RS_parms = {SDH_STM1_MUX_TM_RS_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

			/* Read the STM1 TM-HP configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
			Sdh_Tm_Hp_Parms_S HP_parms = {SDH_STM1_MUX_TM_HP_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

			/* Read the STM1 TM-LP configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
			Sdh_Tm_Lp_Parms_S LP_parms = {SDH_STM1_MUX_TM_LP_CFG_LIST};
#undef SDH_STM1_PARAM_INIT

			/* Read the STM1 TM-Data configuration default value */
#define SDH_STM1_PARAM_INIT(index, datatype, var_name, size, default_value,  NameString, CommandString)   default_value,
			Sdh_Tm_Data_S TRACE_data = {SDH_STM1_MUX_TM_MSG_DATA};
#undef SDH_STM1_PARAM_INIT

			//Update Default structure
                        memcpy(&default_tm_cfg.rs_parms, &RS_parms, sizeof(Sdh_Tm_Rs_Parms_S)); 
                        memcpy(&default_tm_cfg.hp_parms, &HP_parms, sizeof(Sdh_Tm_Hp_Parms_S)); 
                        memcpy(&default_tm_cfg.lp_parms, &LP_parms, sizeof(Sdh_Tm_Lp_Parms_S)); 
                        memcpy(&default_tm_cfg.trace_data, &TRACE_data, sizeof(Sdh_Tm_Data_S)); 
			
			/* Update the STM1 card information with stm1_hw_info variable */
			dbl_sdh_stm1_card_info_update(ptr_stm1_port_cfg, stm1_hw_info, STM1_TRACE_MSG_CONFIG);

			/* Update the STM1 TraceMsg - RS configuration default values */
			dbl_sdh_stm1_config_info_update(ptr_stm1_tm_cfg,
					&default_tm_cfg, default_cfg, STM1_TRACE_MSG_CONFIG);
			break;
 		default:
			printf("Invalid Feature option \n");

                return STM1_SUCCESS;
	}
}


/******************************************************************************
 * Description          : This function is used to copy the STM1 Port configuration
 *                                        between the structures
 *                      
 * Inputs               : Stm1_Port_Config_S * - source and destination                         
 *                                              buffers to be copied
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 dbl_sdh_stm1_copy_cfg(void *ptr_dst_cfg, void *ptr_src_cfg, Sdh_Stm1_Features_E stm1_feature) 
{               
        PAL_INT_32 ret_val = STM1_SUCCESS;
	const struct params_type *port_cfg_type = &Stm1_Port_Config_type;
	const struct params_type *md_cfg_type = &Stm1_Core_Mux_Demux_type;
	const struct params_type *tm_cfg_type = &Stm1_Core_Trace_Msg_type;
	const struct params_type *prot_cfg_type = &Stm1_Prot_Parms_type;
	const struct params_type *prot_lcr_cfg_type = &Sdh_Prot_Sw_lcr_type;
	const struct params_type *sync_cfg_type = &Stm1_Sync_Parms_type;
	const struct params_type *cp_cfg_type = &Stm1_Cp_Config_type;

        Stm1_Port_Config_S * ptr_src_port_cfg;
	Stm1_Port_Config_S * ptr_dst_port_cfg;
	Stm1_Core_Mux_Demux_S* ptr_src_md_cfg;
	Stm1_Core_Mux_Demux_S* ptr_dst_md_cfg;
	Stm1_Core_Trace_Msg_S* ptr_src_tm_cfg;
	Stm1_Core_Trace_Msg_S* ptr_dst_tm_cfg;
	Stm1_Prot_Parms_S* ptr_src_prot_cfg;
	Stm1_Prot_Parms_S* ptr_dst_prot_cfg;
	Sdh_Prot_Sw_lcr_S * ptr_src_prot_lcr_cfg;
	Sdh_Prot_Sw_lcr_S * ptr_dst_prot_lcr_cfg;
	Stm1_Sync_Parms_S* ptr_src_sync_cfg;
	Stm1_Sync_Parms_S* ptr_dst_sync_cfg;
	Stm1_Cp_Config_S* ptr_src_cp_cfg;
	Stm1_Cp_Config_S* ptr_dst_cp_cfg;

	switch(stm1_feature)
	{

		case STM1_PORT_CONFIG:
			ptr_src_port_cfg = (Stm1_Port_Config_S *)ptr_src_cfg;
			ptr_dst_port_cfg = (Stm1_Port_Config_S *)ptr_dst_cfg;

			if(params_set(port_cfg_type, ptr_src_port_cfg, NULL,\
						ptr_dst_port_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_MUX_DEMUX_CONFIG:
			ptr_src_md_cfg = (Stm1_Core_Mux_Demux_S *)ptr_src_cfg;
			ptr_dst_md_cfg = (Stm1_Core_Mux_Demux_S *)ptr_dst_cfg;

			if(params_set(md_cfg_type, ptr_src_md_cfg, NULL,\
						ptr_dst_md_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_TRACE_MSG_CONFIG:
			ptr_src_tm_cfg = (Stm1_Core_Trace_Msg_S *)ptr_src_cfg;
			ptr_dst_tm_cfg = (Stm1_Core_Trace_Msg_S *)ptr_dst_cfg;

			if(params_set(tm_cfg_type, ptr_src_port_cfg, NULL,\
						ptr_dst_port_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_PROTECTION_SW_CONFIG:
			ptr_src_prot_cfg = (Stm1_Prot_Parms_S *)ptr_src_cfg;
			ptr_dst_prot_cfg = (Stm1_Prot_Parms_S *)ptr_dst_cfg;

			if(params_set(prot_cfg_type, ptr_src_prot_cfg, NULL,\
						ptr_dst_prot_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_PROTECTION_SW_LCR_CONFIG:
			ptr_src_prot_lcr_cfg = (Sdh_Prot_Sw_lcr_S *)ptr_src_cfg;
			ptr_dst_prot_lcr_cfg = (Sdh_Prot_Sw_lcr_S *)ptr_dst_cfg;

			if(params_set(prot_lcr_cfg_type, ptr_src_prot_lcr_cfg, NULL,\
						ptr_dst_prot_lcr_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_SYNC_CONFIG:
			ptr_src_sync_cfg = (Stm1_Sync_Parms_S *)ptr_src_cfg;
			ptr_dst_sync_cfg = (Stm1_Sync_Parms_S *)ptr_dst_cfg;

			if(params_set(sync_cfg_type, ptr_src_sync_cfg, NULL,\
						ptr_dst_sync_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;

		case STM1_CROSSPOINT_CONFIG:
			ptr_src_cp_cfg = (Stm1_Cp_Config_S*)ptr_src_cfg;
			ptr_dst_cp_cfg = (Stm1_Cp_Config_S*)ptr_dst_cfg;

			if(params_set(cp_cfg_type, ptr_src_cp_cfg, NULL,\
						ptr_dst_cp_cfg) == -1)
			{
				ret_val = STM1_FAILURE; 
			}
			break;
                
		default:
                        {
                                printf(" Entering the default case \n");
                        }
	}/* end of the switch(sdh_feature) */

       return ret_val;
}


/******************************************************************************
 * Description          : This function is used to read/write the database storage
 *                                        file with the latest configuration data
 *
 * Inputs               : Sdh_Stm1_Features_E - STM1 Feature information to read/write
 *                        void * - Buffer to read/write the E1T1 configuration
 *                        Sdh_Stm1_Ext_Db_Op_E - Database operation to be performed
 * Output               : None
 * Return Value         : SUCCESS on success
 ******************************************************************************/
PAL_INT_32 sdh_stm1_configuration_dB(Sdh_Stm1_Features_E stm1_feature, void *ptr_stm1_cfg,\
                                                                                                Sdh_Stm1_Ext_Db_Op_E dB_operation)
{
        const struct params_type *ptr_cfg_type; 

        PAL_INT_32 ret_val = STM1_SUCCESS;
        FILE *fp = NULL;
        char *file_path;

        switch(stm1_feature)
        {
                case STM1_PORT_CONFIG:
                {
                        ptr_cfg_type = &Stm1_Port_Config_type;
                        file_path = "/tmp/pdh_port_cfg.xml";
                        break;
                }
                case STM1_MUX_DEMUX_CONFIG:
		{
                        ptr_cfg_type = &Stm1_Core_Mux_Demux_type;
                        file_path = "/tmp/pdh_md_cfg.xml";
                        break;
		}
                case STM1_TRACE_MSG_CONFIG:
		{
                        ptr_cfg_type = &Stm1_Core_Trace_Msg_type;
                        file_path = "/tmp/pdh_tm_cfg.xml";
                        break;
		}
                case STM1_PROTECTION_SW_CONFIG:
		{
                        ptr_cfg_type = &Stm1_Prot_Parms_type;
                        file_path = "/tmp/pdh_prot_cfg.xml";
                        break;
		}
                case STM1_PROTECTION_SW_LCR_CONFIG:
                {
                        ptr_cfg_type = &Sdh_Prot_Sw_lcr_type;
                        file_path = "/tmp/pdh_prot_lcr_cfg.xml";
                        break;
                }
                case STM1_SYNC_CONFIG:
		{
                        ptr_cfg_type = &Stm1_Sync_Parms_type;
                        file_path = "/tmp/pdh_sync_cfg.xml";
                        break;
		}
                case STM1_CROSSPOINT_CONFIG:
                {
			ptr_cfg_type = &Stm1_Cp_Config_type;
                        file_path = "/tmp/pdh_cp_cfg.xml";
                        break;
                }
	}
        /* Open the configuration file to read/write */
        fp = fopen(file_path,"w+");
        switch(dB_operation)
        {
                case STM1_GET:
                {
                        if (params_xml_input(ptr_cfg_type, ELEM_TAG, NULL,
                          NULL, fp, ptr_stm1_cfg, PARAMS_XML_UNINIT,
                          PARAMS_LOGGER_STDERR) == -1)
                        {
                                printf(" %s: Failed to read STM1 cfg from %s \n", \
                                                                                __func__,file_path);
                                ret_val = STM1_FAILURE;
                        }
                        break;
                }
                case STM1_SET:
                {
                        if (params_xml_output(ptr_cfg_type,
                          ELEM_TAG, NULL, ptr_stm1_cfg, fp, NULL, PARAMS_XML_FULL) == -1)
                        {
                                printf(" %s: Failed to write STM1 cfg from %s \n",\
                                                                                __func__,file_path);
                                ret_val = STM1_FAILURE;
                        }
                        break;
                }
        }

        /* Close the configuration file */
        fclose(fp);
        return ret_val;
}


