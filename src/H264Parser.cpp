//============================================================================
// Name        : H264PARSER.cpp
// Author      : ss
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_util.h"
#include "H264Parser.h"



int h264Parser::GetAnnexbNALU (NALU_t *nalu){
	int info2=0, info3=0;
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

	nalu->startcodeprefix_len=3;

	//read_buffer(Buf,1,m_h264_buffer);
	if (3 != read_buffer(Buf,3,m_h264_buffer)){//先读3个字节出来，看文件里面是否有足够的字节
		free(Buf);
		Buf = NULL;
		return 0;
	}
	//int d = get_one_Byte(Buf,m_h264_buffer);
	info2 = FindStartCode2 (Buf);
	if(info2 != 1) {
		if(1 != read_buffer(Buf+3, 1,m_h264_buffer)){
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
		if (0 == m_h264_buffer.length){
			nalu->len = (pos)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
			nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
			free(Buf);
			Buf = NULL;
			return pos;
		}
		if(m_h264_buffer.length > 0)
			Buf[pos++] = get_char (m_h264_buffer);
		info3 = FindStartCode3(&Buf[pos-4]);
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != buffer_seek (rewind,m_h264_buffer)){
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
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
	if(Buf != NULL){
		free(Buf);
		Buf = NULL;
	}

	return (pos+rewind);//返回的是整个NALU的长度（nalu.len+StartCodeSizeLen）
}
/**
 * Analysis H.264 Bitstream
 * @param url    Location of input H.264 bitstream file.
 */

int h264Parser::h264_parser(unsigned char * buffer, unsigned int bufferlen,HEVCParser::stHDRMetadata & h264info){

	NALU_t *n;
	m_start_code_size = 0;
	m_code_core_id = -1;
	m_h264_buffer.data = buffer;
	m_h264_buffer.length = bufferlen;
	m_h264_buffer.endIdx = bufferlen;
	m_isSps = 0;
	FILE *myout=stdout;
	n = (NALU_t*)calloc (1, sizeof (NALU_t));
	if (n == NULL){
		printf("Alloc NALU Error\n");
		//return 0;
	}

	n->max_size=H264_FRAME_MAX_LEN;
	n->buf = (char*)calloc (H264_FRAME_MAX_LEN, sizeof (char));
	if (n->buf == NULL){
		free (n);
		printf ("AllocNALU: n->buf");
		return 0;
	}

	int data_offset=0;//数据偏移量
	int nal_num=0;//帧数
#if SHOW_DETAIL
	printf("-----+-------- NALU Table ------+---------+\n");
	printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
	printf("-----+---------+--------+-------+---------+\n");
#endif
	while(m_h264_buffer.length > 0)
	{
		int data_lenth;
		if(nal_num == 876){
			printf("end");
		}
		data_lenth=GetAnnexbNALU(n);

		char type_str[20]={0};
		switch(n->nal_unit_type){
			case NALU_TYPE_SLICE:sprintf(type_str,"SLICE");break;
			case NALU_TYPE_DPA:sprintf(type_str,"DPA");break;
			case NALU_TYPE_DPB:sprintf(type_str,"DPB");break;
			case NALU_TYPE_DPC:sprintf(type_str,"DPC");break;
			case NALU_TYPE_IDR:sprintf(type_str,"IDR");break;
			case NALU_TYPE_SEI:{
				int ret = h264_parser_sei(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				sprintf(type_str,"SEI_%d",ret);
				break;
			}
			case NALU_TYPE_SPS:{
				sprintf(type_str,"SPS");
				h264_parser_sps(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				m_isSps = 1;
				break;
			}
			case NALU_TYPE_PPS:sprintf(type_str,"PPS");break;
			case NALU_TYPE_AUD:sprintf(type_str,"AUD");break;
			case NALU_TYPE_EOSEQ:sprintf(type_str,"EOSEQ");break;
			case NALU_TYPE_EOSTREAM:sprintf(type_str,"EOSTREAM");break;
			case NALU_TYPE_FILL:sprintf(type_str,"FILL");break;
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
		fprintf(myout,"%5d| %8d| %7s| %6s| %8d|\n",nal_num,data_offset,idc_str,type_str,n->len);
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
int h264Parser::h264_parser_sps(unsigned char * buf, unsigned int nLen){
	int width = 0;
	int height = 0;
	double fps = 0;

	unsigned int StartBit=0;
    fps=0;
    de_emulation_prevention(buf,&nLen);

    int forbidden_zero_bit=u(1,buf,StartBit);
    int nal_ref_idc=u(2,buf,StartBit);
    int nal_unit_type=u(5,buf,StartBit);
    if(nal_unit_type==7)
    {
        int profile_idc=u(8,buf,StartBit);
        int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
        int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
        int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
        int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
        int reserved_zero_4bits=u(4,buf,StartBit);
        int level_idc=u(8,buf,StartBit);

        int seq_parameter_set_id=Ue(buf,nLen,StartBit);

        if( profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144 )
        {
            int chroma_format_idc=Ue(buf,nLen,StartBit);
            if( chroma_format_idc == 3 ){
                int residual_colour_transform_flag=u(1,buf,StartBit);
               }
            int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);
            int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);
            int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
            int seq_scaling_matrix_present_flag=u(1,buf,StartBit);

            int seq_scaling_list_present_flag[8];
            int useDefaultScalingMatrixFlag;
            if( seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < ( ( chroma_format_idc  !=  3 ) ? 8 : 12 ); i++ ) {
                    seq_scaling_list_present_flag[i]=u(1,buf,StartBit);
                    if(seq_scaling_list_present_flag[i]){
                    	if(i < 6){
                    		h264_scaling_list(16,buf,StartBit,nLen);
                    			}
                    	else{
                    		h264_scaling_list(64,buf,StartBit,nLen);
                    			}
                    		}
                	}
            }
        }
        int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);
        int pic_order_cnt_type=Ue(buf,nLen,StartBit);
        if( pic_order_cnt_type == 0 )
            int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);
        else if( pic_order_cnt_type == 1 )
        {
            int delta_pic_order_always_zero_flag=u(1,buf,StartBit);
            int offset_for_non_ref_pic=Se(buf,nLen,StartBit);
            int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

            int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
                offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
            delete [] offset_for_ref_frame;
        }
        int num_ref_frames=Ue(buf,nLen,StartBit);
        int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);
        int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
        int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);

        width=(pic_width_in_mbs_minus1+1)*16;
        height=(pic_height_in_map_units_minus1+1)*16;

        int frame_mbs_only_flag=u(1,buf,StartBit);
        if(!frame_mbs_only_flag)
            int mb_adaptive_frame_field_flag=u(1,buf,StartBit);

        int direct_8x8_inference_flag=u(1,buf,StartBit);
        int frame_cropping_flag=u(1,buf,StartBit);
        if(frame_cropping_flag)
        {
            int frame_crop_left_offset=Ue(buf,nLen,StartBit);
            int frame_crop_right_offset=Ue(buf,nLen,StartBit);
            int frame_crop_top_offset=Ue(buf,nLen,StartBit);
            int frame_crop_bottom_offset=Ue(buf,nLen,StartBit);
        }
        int vui_parameter_present_flag=u(1,buf,StartBit);
        if(vui_parameter_present_flag)
        {
            int aspect_ratio_info_present_flag=u(1,buf,StartBit);
            if(aspect_ratio_info_present_flag)
            {
                int aspect_ratio_idc=u(8,buf,StartBit);
                if(aspect_ratio_idc==255)
                {
                    int sar_width=u(16,buf,StartBit);
                    int sar_height=u(16,buf,StartBit);
                }
            }
            int overscan_info_present_flag=u(1,buf,StartBit);
            if(overscan_info_present_flag)
                int overscan_appropriate_flagu=u(1,buf,StartBit);
            int video_signal_type_present_flag=u(1,buf,StartBit);
            if(video_signal_type_present_flag)
            {
                int video_format=u(3,buf,StartBit);
                int video_full_range_flag=u(1,buf,StartBit);
                int colour_description_present_flag=u(1,buf,StartBit);
                h264info.video_format = video_format;
                h264info.video_full_range_flag = video_full_range_flag;
                if(colour_description_present_flag)
                {
                    int colour_primaries=u(8,buf,StartBit);
                    int transfer_characteristics=u(8,buf,StartBit);
                    int matrix_coefficients=u(8,buf,StartBit);
                    h264info.color_primaries = colour_primaries;
                    h264info.transfer_characteristics = transfer_characteristics;
                    h264info.matrix_coeffs = matrix_coefficients;
                }
            }
            int chroma_loc_info_present_flag=u(1,buf,StartBit);
            if(chroma_loc_info_present_flag)
            {
                int chroma_sample_loc_type_top_field=Ue(buf,nLen,StartBit);
                int chroma_sample_loc_type_bottom_field=Ue(buf,nLen,StartBit);
                h264info.chroma_sample_loc_type_bottom_field = chroma_sample_loc_type_bottom_field;
                h264info.chroma_sample_loc_type_top_field = chroma_sample_loc_type_top_field;
            }
            int timing_info_present_flag=u(1,buf,StartBit);

            if(timing_info_present_flag)
            {
                int num_units_in_tick=u(32,buf,StartBit);
                int time_scale=u(32,buf,StartBit);
                fps=time_scale/num_units_in_tick;
                fps = fps/2;
                int fixed_frame_rate_flag=u(1,buf,StartBit);
                if(fixed_frame_rate_flag)
                {
                    //fps=fps/2;
                }
            }
        }
        return true;
    }
    else
        return false;
}

/*return ：是sei的的类型ID*/
int h264Parser::h264_parser_sei(unsigned char * buffer, unsigned int bufferlen){

	unsigned int StartBit = 0;
	de_emulation_prevention(buffer,&bufferlen);

	int byte = u(8,buffer,StartBit);//READ_CODE(8,"NO_USE");

	return h264_sei_message(buffer,StartBit);;
}

int h264Parser::h264_sei_message(unsigned char * buf,unsigned int &StartBit){
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

	return h264_sei_playload(payload_type,buf,StartBit);;
}

int h264Parser::h264_sei_playload(int payload_type,unsigned char * buf,unsigned int &StartBit){

	 switch (payload_type) {
	 	 case H264_SEI_TYPE_BUFFERING_PERIOD:break;

	 	 case H264_SEI_TYPE_USER_DATA:{//解析x264coreId
	 		u(128,buf,StartBit);//

	 		//m_code_core_id
	 		 break;
	 	 }

	    case H264_SEI_TYPE_PICTURE_TIMING:
	   {// Mismatched value from HM 8.1
		   h264_decode_nal_sei_decoded_picture_hash(buf,StartBit);break;
	    }
	    case H264_SEI_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
	    {
	    	h264_mastering_display_colour_volume(buf,StartBit);break;
	    }
	  }
	 return payload_type;
}

int h264Parser::h264_mastering_display_colour_volume(unsigned char * buf,unsigned int &StartBit){
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

	h264info.displayColorVolume.display_primaries_x_b = display_primaries_x_b;
	h264info.displayColorVolume.display_primaries_x_g = display_primaries_x_g;
	h264info.displayColorVolume.display_primaries_x_r = display_primaries_x_r;
	h264info.displayColorVolume.display_primaries_y_b = display_primaries_y_b;
	h264info.displayColorVolume.display_primaries_y_g = display_primaries_y_g;
	h264info.displayColorVolume.display_primaries_y_r = display_primaries_y_r;

	h264info.displayColorVolume.white_point_x = white_point_x;
	h264info.displayColorVolume.white_point_y = white_point_y;

	h264info.displayColorVolume.max_display_mastering_luminance = max_display_mastering_luminance;
	h264info.displayColorVolume.min_display_mastering_luminance = min_display_mastering_luminance;

	return 1;
}
int h264Parser::h264_decode_nal_sei_decoded_picture_hash(unsigned char * buf,unsigned int &StartBit){

	return 1;
}
int h264Parser::h264_scaling_list(int sizeOfScalingList,unsigned char * buf,unsigned int &StartBit,int len){
	unsigned int lastScale = 8;
	unsigned int nextScale = 8;
	unsigned int  useDefaultScalingMatrixFlag;
	for(int j = 0; j < sizeOfScalingList; j++ ) {
		if( nextScale != 0 ) {
			unsigned int delta_scale = Se(buf,len,StartBit); // 0 | 1  se(v)
			nextScale = ( lastScale + delta_scale + 256 ) % 256;
			useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
		}
		lastScale = ( nextScale == 0 ) ? lastScale : nextScale;
	}
	return 0;
}




