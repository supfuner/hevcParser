/*
 * h265Parser.cpp
 *
 *  Created on: 2018年12月14日
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include "h265Parser.h"
#include "parser_util.h"


int h265Parser::h265_parser(unsigned char * buffer, unsigned int bufferlen,HEVCParser::stHDRMetadata & h264info){
	h265_uinit();
	h265_init(buffer,bufferlen);
	NALU_t *n;
	FILE *myout=stdout;
	n = (NALU_t*)calloc (1, sizeof (NALU_t));
	if (n == NULL){
		printf("Alloc NALU Error\n");
		//return 0;
	}

	n->max_size=H265_FRAME_MAX_LEN;
	n->buf = (char*)calloc (H265_FRAME_MAX_LEN, sizeof (char));
	if (n->buf == NULL){
		free (n);
		printf ("AllocNALU: n->buf");
		return 0;
	}

	int data_offset=0;//数据偏移量
	int nal_num=0;//帧数

#if SHOW_DETAIL
	printf("-----+-------- H265 NALU Table--+---------+\n");
	printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
	printf("-----+---------+--------+-------+---------+\n");
#endif

	while(m_h265_buffer.length > 0)
	{
		int data_lenth = 0;
		data_lenth=h265_GetAnnexbNALU(n);

		char type_str[20]={0};
		switch(n->nal_unit_type){
			case H265_NAL_UNIT_VPS:sprintf(type_str,"VPS");break;
			case H265_NAL_UNIT_CODED_SLICE_TSA_N:sprintf(type_str,"SLICE_TSA_N");break;
			case H265_NAL_UNIT_CODED_SLICE_RASL_R:sprintf(type_str,"SLICE_RASL_R");break;
			case H265_NAL_UNIT_CODED_SLICE_CRA:sprintf(type_str,"SLICE_CRA");break;
			case H265_NAL_UNIT_CODED_SLICE_TRAIL_R:sprintf(type_str,"TRAIL_R");break;
			case H265_NAL_UNIT_CODED_SLICE_IDR_N_LP:sprintf(type_str,"IDR_N_LP");break;
			case H265_NAL_UNIT_PREFIX_SEI:
			case H265_NAL_UNIT_SUFFIX_SEI:{
				sprintf(type_str,"SEI");
				h265_parser_sei(m_h265_buffer.data+(m_h265_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				break;
			}
			case H265_NAL_UNIT_SPS:{
				sprintf(type_str,"SPS");
				h265_parser_sps(m_h265_buffer.data+(m_h265_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				m_isSps = 1;
				break;
			}
			case H265_NAL_UNIT_PPS:sprintf(type_str,"PPS");break;
			case H265_NAL_UNIT_ACCESS_UNIT_DELIMITER:sprintf(type_str,"AUD");break;
			case H265_NAL_UNIT_FILLER_DATA:sprintf(type_str,"FILL");break;
			default: sprintf(type_str,"Reserved");break;
		}
		char idc_str[20]={0};
		switch(n->nal_reference_idc>>5){
			case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
			case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
			case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
			case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
		}
#if SHOW_DETAIL
		fprintf(myout,"%5d| %8d| %6s| %15s| %8d|\n",nal_num,data_offset,idc_str,type_str,n->len);
#endif
		data_offset=data_offset+data_lenth;
		nal_num++;
	}

	//Free
	if (n){
		if (n->buf){
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
	return 0;


}

int h265Parser::h265_GetAnnexbNALU (NALU_t *nalu){
	int info2=0, info3=0;
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

	nalu->startcodeprefix_len=3;

	//read_buffer(Buf,1,m_h264_buffer);
	if (3 != read_buffer(Buf,3,m_h265_buffer)){//先读3个字节出来，看文件里面是否有足够的字节
		free(Buf);
		Buf = NULL;
		return 0;
	}
	//int d = get_one_Byte(Buf,m_h264_buffer);
	info2 = FindStartCode2 (Buf);
	if(info2 != 1) {
		if(1 != read_buffer(Buf+3, 1,m_h265_buffer)){
			free(Buf);
			Buf = NULL;
			return 0;
		}
		info3 = FindStartCode3 (Buf);
		if (info3 != 1){
			free(Buf);
			Buf = NULL;
			return -1;
		}
		else {
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	}
	else{
		nalu->startcodeprefix_len = 3;
		pos = 3;
	}
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;
	m_start_code_size = nalu->startcodeprefix_len;
	while (!StartCodeFound){
		if (0 == m_h265_buffer.length){
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
			nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
			nalu->nal_unit_type = ((nalu->buf[0])& 0x7E)>>1;// 5 bit
			free(Buf);
			Buf = NULL;
			return pos-1;
		}
		if(m_h265_buffer.length > 0)
			Buf[pos++] = get_char (m_h265_buffer);
		info3 = FindStartCode3(&Buf[pos-4]);
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != buffer_seek (rewind,m_h265_buffer)){
		free(Buf);
		Buf = NULL;
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//
	nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]& 0x7E)>>1;// & 0x7E)>>1;
	if(Buf != NULL){
		free(Buf);
		Buf = NULL;
	}

	return (pos+rewind);//返回的是整个NALU的长度（nalu.len+StartCodeSizeLen）
}

void h265Parser::h265_init(unsigned char * buffer, unsigned int bufferlen){
	m_h265_buffer.data = buffer;
	m_h265_buffer.endIdx = bufferlen;
	m_h265_buffer.readIdx = 0;
	m_h265_buffer.length = bufferlen;
	m_isSps = 0;
}

void h265Parser::h265_uinit(){
	m_start_code_size = 0;
	m_h265_buffer.data = NULL;
	m_h265_buffer.endIdx = 0;
	m_h265_buffer.readIdx = 0;
	m_h265_buffer.length = 0;
}

int h265Parser::h265_parser_sps(unsigned char * buffer, unsigned int bufferlen){
	unsigned int StartBit=0;
   de_emulation_prevention(buffer,&bufferlen);

    uint32_t    sps_video_parameter_set_id = 0;
    uint32_t    sps_max_sub_layers_minus1 = 0;
    bool        sps_temporal_id_nesting_flag;
    uint32_t    sps_seq_parameter_set_id = 0;
    uint32_t    chroma_format_idc;
    bool        separate_colour_plane_flag = false;
    uint32_t    pic_width_in_luma_samples;
    uint32_t    pic_height_in_luma_samples;
    bool        conformance_window_flag;
    uint32_t    conf_win_left_offset;
    uint32_t    conf_win_right_offset;
    uint32_t    conf_win_top_offset;
    uint32_t    conf_win_bottom_offset;
    uint32_t    bit_depth_luma_minus8;
    uint32_t    bit_depth_chroma_minus8;
    uint32_t    log2_max_pic_order_cnt_lsb_minus4;
    bool        sps_sub_layer_ordering_info_present_flag;
    bool        rbsp_stop_one_bit;

    u(16,buffer,StartBit);//nal_unit_header
    sps_video_parameter_set_id      = u(4,buffer,StartBit);
    sps_max_sub_layers_minus1       = u(3,buffer,StartBit);
    sps_temporal_id_nesting_flag    = u(1,buffer,StartBit);

    h265_parse_ptl(sps_max_sub_layers_minus1,buffer,StartBit,bufferlen);

    sps_seq_parameter_set_id    = Ue(buffer,bufferlen,StartBit);
    //p_sps = &sps[sps_seq_parameter_set_id];

    chroma_format_idc           = Ue(buffer,bufferlen,StartBit);

    if (3 == chroma_format_idc)
    {
        separate_colour_plane_flag = u(1,buffer,StartBit);
    }

    pic_width_in_luma_samples   = Ue(buffer,bufferlen,StartBit);
    pic_height_in_luma_samples  = Ue(buffer,bufferlen,StartBit);

    conformance_window_flag = u(1,buffer,StartBit);

    if (conformance_window_flag)
    {
        conf_win_left_offset    = Ue(buffer,bufferlen,StartBit);
        conf_win_right_offset   = Ue(buffer,bufferlen,StartBit);
        conf_win_top_offset     = Ue(buffer,bufferlen,StartBit);
        conf_win_bottom_offset  = Ue(buffer,bufferlen,StartBit);
    }

    bit_depth_luma_minus8               = Ue(buffer,bufferlen,StartBit);
    bit_depth_chroma_minus8             = Ue(buffer,bufferlen,StartBit);
    log2_max_pic_order_cnt_lsb_minus4   = Ue(buffer,bufferlen,StartBit);

    sps_sub_layer_ordering_info_present_flag = u(1,buffer,StartBit);

    int i;
    uint32_t *sps_max_dec_pic_buffering_minus1   = new uint32_t[sps_max_sub_layers_minus1 + 1];
    uint32_t *sps_max_num_reorder_pics           = new uint32_t[sps_max_sub_layers_minus1 + 1];
    uint32_t *sps_max_latency_increase_plus1     = new uint32_t[sps_max_sub_layers_minus1 + 1];

    for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); i <= sps_max_sub_layers_minus1; i++ )
    {
        sps_max_dec_pic_buffering_minus1[i] = Ue(buffer,bufferlen,StartBit);
        sps_max_num_reorder_pics[i]         = Ue(buffer,bufferlen,StartBit);
        sps_max_latency_increase_plus1[i]   = Ue(buffer,bufferlen,StartBit);
    }

    uint32_t log2_min_luma_coding_block_size_minus3;
    uint32_t log2_diff_max_min_luma_coding_block_size;
    uint32_t log2_min_transform_block_size_minus2;
    uint32_t log2_diff_max_min_transform_block_size;
    uint32_t max_transform_hierarchy_depth_inter;
    uint32_t max_transform_hierarchy_depth_intra;
    bool     scaling_list_enabled_flag;

    log2_min_luma_coding_block_size_minus3      = Ue(buffer,bufferlen,StartBit);
    log2_diff_max_min_luma_coding_block_size    = Ue(buffer,bufferlen,StartBit);
    log2_min_transform_block_size_minus2        = Ue(buffer,bufferlen,StartBit);
    log2_diff_max_min_transform_block_size      = Ue(buffer,bufferlen,StartBit);
    max_transform_hierarchy_depth_inter         = Ue(buffer,bufferlen,StartBit);
    max_transform_hierarchy_depth_intra         = Ue(buffer,bufferlen,StartBit);
    scaling_list_enabled_flag                   = u(1,buffer,StartBit);

    if (scaling_list_enabled_flag)
    {
        bool sps_scaling_list_data_present_flag;

        sps_scaling_list_data_present_flag = u(1,buffer,StartBit);

        if (sps_scaling_list_data_present_flag)
        {
        	h265_parse_scaling_list(buffer,StartBit,bufferlen);
        }
    }

    bool amp_enabled_flag;
    bool sample_adaptive_offset_enabled_flag;
    bool pcm_enabled_flag;
    uint32_t pcm_sample_bit_depth_luma_minus1;
    uint32_t pcm_sample_bit_depth_chroma_minus1;
    uint32_t log2_min_pcm_luma_coding_block_size_minus3;
    uint32_t log2_diff_max_min_pcm_luma_coding_block_size;
    bool pcm_loop_filter_disabled_flag;

    amp_enabled_flag = u(1,buffer,StartBit);
    sample_adaptive_offset_enabled_flag = u(1,buffer,StartBit);
    pcm_enabled_flag = u(1,buffer,StartBit);

    if (pcm_enabled_flag)
    {
        pcm_sample_bit_depth_luma_minus1    = u(4,buffer,StartBit);
        pcm_sample_bit_depth_chroma_minus1  = u(4,buffer,StartBit);
        log2_min_pcm_luma_coding_block_size_minus3 = Ue(buffer,bufferlen,StartBit);
        log2_diff_max_min_pcm_luma_coding_block_size = Ue(buffer,bufferlen,StartBit);
        pcm_loop_filter_disabled_flag       = u(1,buffer,StartBit);
    }

    uint32_t num_short_term_ref_pic_sets = 0;

    num_short_term_ref_pic_sets = Ue(buffer,bufferlen,StartBit);

//    createRPSList(p_sps, num_short_term_ref_pic_sets);

//     for (i = 0; i < num_short_term_ref_pic_sets; i++)
//     {
//    	 //parse_short_term_ref_pic_set(i)
//    	 int inter_ref_pic_set_prediction_flag = 0;
//    	 int delta_idx_minus1 = 0;
//    	 int delta_rps_sign = 0;
//    	 int abs_delta_rps_minus1 = 0;
//    	 int delta_rps = 0;
//    	 if(i){
//    		 inter_ref_pic_set_prediction_flag = u(1,buffer,StartBit);
//    	 }
//    	 if(inter_ref_pic_set_prediction_flag){
//    		 if(i == num_short_term_ref_pic_sets){
//    			 delta_idx_minus1 = Ue(buffer,bufferlen,StartBit);
//    		 }
//    		 delta_rps_sign = u(1,buffer,StartBit);
//    		 abs_delta_rps_minus1 = Ue(buffer,bufferlen,StartBit);
//    	    delta_rps      = (1 - (delta_rps_sign << 1)) * (abs_delta_rps_minus1+1);
////            for (i = 0; i <= rps_ridx->num_delta_pocs; i++) {
////
////            }
//    	 }
//
//     }

    bool long_term_ref_pics_present_flag = false;

    long_term_ref_pics_present_flag = u(1,buffer,StartBit);

    if (long_term_ref_pics_present_flag)
    {
        uint32_t num_long_term_ref_pics_sps;

        num_long_term_ref_pics_sps = Ue(buffer,bufferlen,StartBit);

        uint32_t *lt_ref_pic_poc_lsb_sps = new uint32_t[num_long_term_ref_pics_sps];
        uint32_t *used_by_curr_pic_lt_sps_flag = new uint32_t[num_long_term_ref_pics_sps];

        for (i = 0; i < num_long_term_ref_pics_sps; i++)
        {
        		int varible = (log2_max_pic_order_cnt_lsb_minus4+4)>16?16:(log2_max_pic_order_cnt_lsb_minus4+4);
            lt_ref_pic_poc_lsb_sps[i]       = u(varible,buffer,StartBit);
            used_by_curr_pic_lt_sps_flag[i] = u(1,buffer,StartBit);
        }
    }

    bool    sps_temporal_mvp_enabled_flag;
    bool    strong_intra_smoothing_enabled_flag;
    bool    vui_parameters_present_flag;

    sps_temporal_mvp_enabled_flag       = u(1,buffer,StartBit);
    strong_intra_smoothing_enabled_flag = u(1,buffer,StartBit);
    vui_parameters_present_flag         = u(1,buffer,StartBit);

    if (vui_parameters_present_flag)
    {
        //parse_vui(sps_max_sub_layers_minus1);
    	h265_parse_vui(buffer,StartBit,bufferlen);
    }

//    bool sps_extension_flag = false;
//
//    sps_extension_flag = READ_FLAG("sps_extension_flag");
//
//    if (sps_extension_flag)
//    {
//        while (MORE_RBSP_DATA())
//        {
//            bool sps_extension_data_flag;
//
//            sps_extension_data_flag = READ_FLAG("sps_extension_data_flag");
//        }
//    }
//
//    rbsp_stop_one_bit = READ_FLAG("rbsp_stop_one_bit");
//
//    p_sps->m_VPSId                           = sps_video_parameter_set_id;
//    p_sps->m_SPSId                           = sps_seq_parameter_set_id;
//    p_sps->m_chromaFormatIdc                 = chroma_format_idc;
//
//    p_sps->m_uiBitsForPOC                    = log2_max_pic_order_cnt_lsb_minus4 + 4;
//
//    p_sps->m_separateColourPlaneFlag         = separate_colour_plane_flag;
//
//    p_sps->m_log2MinCodingBlockSize          = log2_min_luma_coding_block_size_minus3 + 3;
//    p_sps->m_log2DiffMaxMinCodingBlockSize   = log2_diff_max_min_luma_coding_block_size;
//    p_sps->m_uiMaxCUWidth                    = 1 << (p_sps->m_log2MinCodingBlockSize + p_sps->m_log2DiffMaxMinCodingBlockSize);
//    p_sps->m_uiMaxCUHeight                   = 1 << (p_sps->m_log2MinCodingBlockSize + p_sps->m_log2DiffMaxMinCodingBlockSize);
//
//    p_sps->m_bLongTermRefsPresent            = long_term_ref_pics_present_flag;
//    p_sps->m_TMVPFlagsPresent                = sps_temporal_mvp_enabled_flag;
//    p_sps->m_bUseSAO                         = sample_adaptive_offset_enabled_flag;
//    //printf("CtbSizeY=%u\n", p_sps->m_uiMaxCUWidth);
//
//    if(conformance_window_flag){
//    	int sub_width_c  = ((1==chroma_format_idc)||(2 == chroma_format_idc))&&(0==separate_colour_plane_flag)?2:1;
//    	int sub_height_c = (1==chroma_format_idc)&& (0 == separate_colour_plane_flag)?2:1;
//    	g_HevcInfo.Width  -= (sub_width_c*conf_win_right_offset + sub_width_c*conf_win_left_offset);
//    	g_HevcInfo.Height -= (sub_height_c*conf_win_bottom_offset + sub_height_c*conf_win_top_offset);
//    }else{
//    	g_HevcInfo.Width  = pic_width_in_luma_samples;
//		g_HevcInfo.Height = pic_height_in_luma_samples;
//    }
    return 1;
}

/* profile_tier_level */
void h265Parser::h265_parse_ptl(uint32_t max_sub_layers_minus1,unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen)
{
    unsigned char general_profile_space;
    bool    general_tier_flag;
    unsigned char general_profile_idc;
    unsigned char general_profile_compatibility_flag[32];
    bool    general_progressive_source_flag;
    bool    general_interlaced_source_flag;
    bool    general_non_packed_constraint_flag;
    bool    general_frame_only_constraint_flag;
    unsigned long int general_reserved_zero_44bits;
    unsigned char general_level_idc;

    bool    *sub_layer_profile_present_flag = new bool[max_sub_layers_minus1];
    bool    *sub_layer_level_present_flag = new bool[max_sub_layers_minus1];
    unsigned char *sub_layer_profile_space = new unsigned char[max_sub_layers_minus1];
    bool    *sub_layer_tier_flag = new bool[max_sub_layers_minus1];
    unsigned char *sub_layer_profile_idc = new unsigned char[max_sub_layers_minus1];

    uint32_t i;
    uint32_t j;

    general_profile_space   = u(2,buffer,StartBit);
    general_tier_flag       = u(1,buffer,StartBit);
    general_profile_idc     = u(5,buffer,StartBit);

    for (j = 0; j < 32; j++)
    {
        general_profile_compatibility_flag[j] = u(1,buffer,StartBit);
    }

    general_progressive_source_flag     = u(1,buffer,StartBit);
    general_interlaced_source_flag      = u(1,buffer,StartBit);
    general_non_packed_constraint_flag  = u(1,buffer,StartBit);
    general_frame_only_constraint_flag  = u(1,buffer,StartBit);

    u(16,buffer,StartBit);
    u(16,buffer,StartBit);
    u(12,buffer,StartBit);

    general_level_idc = u(8,buffer,StartBit);

    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        sub_layer_profile_present_flag[i]   = u(1,buffer,StartBit);
        sub_layer_level_present_flag[i]     = u(1,buffer,StartBit);
    }

    if (max_sub_layers_minus1 > 0)
    {
        for (i = max_sub_layers_minus1; i < 8; i++)
        {
        	u(2,buffer,StartBit);
        }
    }

    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        if (sub_layer_profile_present_flag[i])
        {
            sub_layer_profile_space[i]  = u(2,buffer,StartBit);
            sub_layer_tier_flag[i]      = u(1,buffer,StartBit);
            sub_layer_profile_idc[i]    = u(5,buffer,StartBit);
        }

        if (sub_layer_level_present_flag[i])
        {
        }
    }
}
/** decode quantization matrix */
static uint32_t ScalingList[4][6][64];
void h265Parser::h265_parse_scaling_list(unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen)
{
    int         sizeId;
    int         matrixId;
    bool        scaling_list_pred_mode_flag[4][6];
    uint32_t    scaling_list_pred_matrix_id_delta[4][6];

    for (sizeId = 0; sizeId < 4; sizeId++)
    {
        for (matrixId = 0; matrixId < ( (sizeId == 3) ? 2 : 6 ); matrixId++)
        {
            scaling_list_pred_mode_flag[sizeId][matrixId] = u(1,buffer,StartBit);

            if (!scaling_list_pred_mode_flag[sizeId][matrixId])
            {
                scaling_list_pred_matrix_id_delta[sizeId][matrixId] = Ue(buffer,bufLen,StartBit);
            }
            else
            {
                uint32_t    nextCoef;
                uint32_t    coefNum;
                int32_t     scaling_list_dc_coef_minus8[4][6];

                nextCoef    = 8;
                coefNum     = parser_min(64, (1 << (4 + (sizeId << 1))));

                if (sizeId > 1)
                {
                    scaling_list_dc_coef_minus8[sizeId - 2][matrixId] = Se(buffer,bufLen,StartBit);

                    nextCoef = scaling_list_dc_coef_minus8[sizeId - 2][matrixId] + 8;
                }

                uint32_t i;
                for (i = 0; i < coefNum; i++)
                {
                    int32_t scaling_list_delta_coef;

                    scaling_list_delta_coef = Se(buffer,bufLen,StartBit);
                    nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
                    ScalingList[sizeId][matrixId][i] = nextCoef;
                }
            }
        }
    }
}

void h265Parser::h265_parse_vui(unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen)
{
    bool            aspect_ratio_info_present_flag;
   int  aspect_ratio_idc = 0;
    unsigned short int        sar_width;
    unsigned short int        sar_height;

    aspect_ratio_info_present_flag = u(1,buffer,StartBit);

    if (aspect_ratio_info_present_flag)
    {
        aspect_ratio_idc = u(8,buffer,StartBit);

        if (aspect_ratio_idc == 255)
        {
            sar_width   = u(16,buffer,StartBit);
            sar_height  = u(16,buffer,StartBit);
        }
    }

    bool overscan_info_present_flag;
    bool overscan_appropriate_flag;

    overscan_info_present_flag = u(1,buffer,StartBit);

    if (overscan_info_present_flag)
    {
        overscan_appropriate_flag = u(1,buffer,StartBit);
    }

    bool    video_signal_type_present_flag;
    unsigned char video_format;
    bool    video_full_range_flag;
    bool    colour_description_present_flag;
    unsigned char colour_primaries;
    unsigned char transfer_characteristics;
    unsigned char matrix_coeffs;

    video_signal_type_present_flag = u(1,buffer,StartBit);

    if (video_signal_type_present_flag)
    {
        video_format            = u(3,buffer,StartBit);
        video_full_range_flag   = u(1,buffer,StartBit);
        colour_description_present_flag = u(1,buffer,StartBit);
        m_h265info.video_format = video_format;
        m_h265info.video_full_range_flag = video_full_range_flag;

        if (colour_description_present_flag)
        {
            colour_primaries            = u(8,buffer,StartBit);
            transfer_characteristics    = u(8,buffer,StartBit);
            matrix_coeffs               = u(8,buffer,StartBit);

            m_h265info.color_primaries = colour_primaries;
            m_h265info.transfer_characteristics = transfer_characteristics;
            m_h265info.matrix_coeffs = matrix_coeffs;

        }
    }

    bool        chroma_loc_info_present_flag;
    uint32_t    chroma_sample_loc_type_top_field;
    uint32_t    chroma_sample_loc_type_bottom_field;

    chroma_loc_info_present_flag = u(1,buffer,StartBit);

    if (chroma_loc_info_present_flag)
    {
        chroma_sample_loc_type_top_field    = Ue(buffer,bufLen,StartBit);
        chroma_sample_loc_type_bottom_field = Ue(buffer,bufLen,StartBit);
        m_h265info.chroma_sample_loc_type_bottom_field = chroma_sample_loc_type_bottom_field;
        m_h265info.chroma_sample_loc_type_top_field = chroma_sample_loc_type_top_field;
    }

    bool        neutral_chroma_indication_flag;
    bool        field_seq_flag;
    bool        frame_field_info_present_flag;
    bool        default_display_window_flag;
    uint32_t    def_disp_win_left_offset;
    uint32_t    def_disp_win_right_offset;
    uint32_t    def_disp_win_top_offset;
    uint32_t    def_disp_win_bottom_offset;

    neutral_chroma_indication_flag  = u(1,buffer,StartBit);
    field_seq_flag                  = u(1,buffer,StartBit);
    frame_field_info_present_flag   = u(1,buffer,StartBit);
    default_display_window_flag     = u(1,buffer,StartBit);

    if (default_display_window_flag)
    {
        def_disp_win_left_offset    = Ue(buffer,bufLen,StartBit);
        def_disp_win_right_offset   = Ue(buffer,bufLen,StartBit);
        def_disp_win_top_offset     = Ue(buffer,bufLen,StartBit);
        def_disp_win_bottom_offset  = Ue(buffer,bufLen,StartBit);
    }


    bool        vui_timing_info_present_flag;
    uint32_t    vui_num_units_in_tick;
    uint32_t    vui_time_scale;
    bool        vui_poc_proportional_to_timing_flag;
    uint32_t    vui_num_ticks_poc_diff_one_minus1;
    bool        vui_hrd_parameters_present_flag;

    vui_timing_info_present_flag = u(1,buffer,StartBit);

    if (vui_timing_info_present_flag)
    {
        vui_num_units_in_tick   = u(32,buffer,StartBit);
        vui_time_scale          = u(32,buffer,StartBit);
        vui_poc_proportional_to_timing_flag = u(1,buffer,StartBit);

        double fps = (double)vui_time_scale/(double)vui_num_units_in_tick;

        if (vui_poc_proportional_to_timing_flag)
        {
            vui_num_ticks_poc_diff_one_minus1 = Ue(buffer,bufLen,StartBit);
        }

        vui_hrd_parameters_present_flag = u(1,buffer,StartBit);

        if (vui_hrd_parameters_present_flag)
        {
            //parse_hrd(true, maxNumSubLayersMinus1);
        }
    }

    bool        bitstream_restriction_flag;
    bool        tiles_fixed_structure_flag;
    bool        motion_vectors_over_pic_boundaries_flag;
    bool        restricted_ref_pic_lists_flag;
    uint32_t    min_spatial_segmentation_idc;
    uint32_t    max_bytes_per_pic_denom;
    uint32_t    max_bits_per_min_cu_denom;
    uint32_t    log2_max_mv_length_horizontal;
    uint32_t    log2_max_mv_length_vertical;

//    bitstream_restriction_flag = READ_FLAG("bitstream_restriction_flag");
//
//    if (bitstream_restriction_flag)
//    {
//        tiles_fixed_structure_flag              = READ_FLAG("tiles_fixed_structure_flag");
//        motion_vectors_over_pic_boundaries_flag = READ_FLAG("motion_vectors_over_pic_boundaries_flag");
//        restricted_ref_pic_lists_flag           = READ_FLAG("restricted_ref_pic_lists_flag");
//        min_spatial_segmentation_idc            = READ_UVLC("min_spatial_segmentation_idc");
//        max_bytes_per_pic_denom                 = READ_UVLC("max_bytes_per_pic_denom");
//        max_bits_per_min_cu_denom               = READ_UVLC("max_bits_per_min_cu_denom");
//        log2_max_mv_length_horizontal           = READ_UVLC("log2_max_mv_length_horizontal");
//        log2_max_mv_length_vertical             = READ_UVLC("log2_max_mv_length_vertical");
//    }

}

int h265Parser::h265_parser_sei(unsigned char * buffer, unsigned int bufferlen){

	unsigned int StartBit = 0;
	de_emulation_prevention(buffer,&bufferlen);

	int byte = u(16,buffer,StartBit);//READ_CODE(8,"NO_USE");

	return h265_sei_message(buffer,StartBit);
}

int h265Parser::h265_sei_message(unsigned char * buf,unsigned int &StartBit){
	  int payload_type = 0;
	  int payload_size = 0;
	  int byte = 0xFF;
	  //byte = u(8,buf,StartBit);//READ_CODE(8,"NO_USE");

	  while (next_bits(8,buf,StartBit) == 0xFF) {
	      byte          = u(8,buf,StartBit);
	      payload_type += 255;
	    }
	  int last_payload_type_byte = u(8,buf,StartBit);//READ_CODE(8,"last_payload_type_byte");
	  payload_type +=last_payload_type_byte;
	  while (next_bits(8,buf,StartBit) == 0xFF) {
	      byte          = u(8,buf,StartBit);
	      payload_size += 255;
	    }
	  int last_playload_size_byte = u(8,buf,StartBit);;//READ_CODE(8,"last_playload_size_byte");
	  payload_size += last_playload_size_byte;
	  h265_sei_playload(payload_type,buf,StartBit);
	return 1;
}

int h265Parser::h265_sei_playload(int payload_type,unsigned char * buf,unsigned int &StartBit){

	 switch (payload_type) {
	 	 case H265_SEI_TYPE_PICTURE_TIMING:break;

	    case H265_SEI_TYPE_ACTIVE_PARAMTER_SETS:
	   {
		   h265_active_parameter_sets(buf,StartBit);break;
	    }
	    case H265_SEI_TYPE_CONTENT_LIGHT_LEVEL_INFO:
	    {
	    	h265_content_light_level_info(buf,StartBit);
	    	break;
	    }
	    case H265_SEI_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
	    {
	    	h265_mastering_display_colour_volume(buf,StartBit);break;
	    }
	  }
	 return payload_type;
}

int h265Parser::h265_mastering_display_colour_volume(unsigned char * buf,unsigned int &StartBit){
	int  display_primaries_x_r = -1;//2
	int  display_primaries_y_r = -1;
	int  display_primaries_x_g = -1;//0
	int  display_primaries_y_g = -1;
	int  display_primaries_x_b = -1;//1
	int  display_primaries_y_b = -1;

	int  white_point_x = -1;
	int  white_point_y = -1;

	int  max_display_mastering_luminance = -1;
	int  min_display_mastering_luminance = -1;
	display_primaries_x_g = u(16,buf,StartBit);
	display_primaries_y_g = u(16,buf,StartBit);
	display_primaries_x_b = u(16,buf,StartBit);
	display_primaries_y_b = u(16,buf,StartBit);
	display_primaries_x_r = u(16,buf,StartBit);
	display_primaries_y_r = u(16,buf,StartBit);

	white_point_x = u(16,buf,StartBit);
	white_point_y = u(16,buf,StartBit);

	max_display_mastering_luminance = u(32,buf,StartBit);
	min_display_mastering_luminance = u(32,buf,StartBit);

	m_h265info.displayColorVolume.display_primaries_x_b = display_primaries_x_b;
	m_h265info.displayColorVolume.display_primaries_x_g = display_primaries_x_g;
	m_h265info.displayColorVolume.display_primaries_x_r = display_primaries_x_r;
	m_h265info.displayColorVolume.display_primaries_y_b = display_primaries_y_b;
	m_h265info.displayColorVolume.display_primaries_y_g = display_primaries_y_g;
	m_h265info.displayColorVolume.display_primaries_y_r = display_primaries_y_r;

	m_h265info.displayColorVolume.white_point_x = white_point_x;
	m_h265info.displayColorVolume.white_point_y = white_point_y;

	m_h265info.displayColorVolume.max_display_mastering_luminance = max_display_mastering_luminance;
	m_h265info.displayColorVolume.min_display_mastering_luminance = min_display_mastering_luminance;

	return 1;
}

int h265Parser::h265_content_light_level_info(unsigned char * buf,unsigned int &StartBit) {
	int max_content_light_level = -1;
	int max_pic_average_light_level = -1;

	max_content_light_level = u(16,buf,StartBit);
	max_pic_average_light_level = u(16,buf,StartBit);

	m_h265info.MaxCLL = max_content_light_level;
	m_h265info.MaxFall = max_pic_average_light_level;
	return 1;
}

int h265Parser::h265_active_parameter_sets(unsigned char * buf,unsigned int &StartBit){
	int active_video_parameter_set_id = u(4,buf,StartBit);
	int self_contained_cvs_flag = u(1,buf,StartBit);
	int no_parameter_set_update_flag = u(1,buf,StartBit);
	return 1;
}


