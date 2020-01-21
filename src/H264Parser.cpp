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
#include <algorithm>
#include "parser_util.h"
#include "H264Parser.h"

void h264Parser::h264_init(){
	h264_resetPOC();
}
void h264Parser::h264_uinit(){
	h264_resetPOC();
}


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
		//printf("nal_num=%d,read=%d\n",nal_num,m_h264_buffer.readIdx);
		data_lenth=GetAnnexbNALU(n);

		char type_str[20]={0};
		switch(n->nal_unit_type){
			case NALU_TYPE_AUD:sprintf(type_str,"AUD");break;
			case NALU_TYPE_SPS:{
				printf(type_str,"SPS");
				h264_parser_sps(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				m_isSps = 1;
				break;
			}
			case NALU_TYPE_PPS:{
				sprintf(type_str,"PPS");
				h264_parser_pps(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				break;
			}
			case NALU_TYPE_SEI:{
				int ret = h264_parser_sei(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				sprintf(type_str,"SEI_%d",ret);
				break;
			}
			case NALU_TYPE_IDR:{
				sprintf(type_str,"IDR");
				h264_resetPOC();
				//printf("IDR\n");
				h264_parser_Slice_Header(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				break;
			}
			case NALU_TYPE_SLICE:{
				sprintf(type_str,"SLICE");
				h264_parser_Slice_Header(m_h264_buffer.data+(m_h264_buffer.readIdx-data_lenth+m_start_code_size),(data_lenth-m_start_code_size));
				break;
			}

			case NALU_TYPE_DPA:sprintf(type_str,"DPA");break;
			case NALU_TYPE_DPB:sprintf(type_str,"DPB");break;
			case NALU_TYPE_DPC:sprintf(type_str,"DPC");break;
			case NALU_TYPE_EOSEQ:sprintf(type_str,"EOSEQ");break;
			case NALU_TYPE_EOSTREAM:sprintf(type_str,"EOSTREAM");break;
			case NALU_TYPE_FILL:sprintf(type_str,"FILL");break;
			default: sprintf(type_str,"Reserved");break;
		}

#if SHOW_DETAIL
		char idc_str[20]={0};
		switch(n->nal_reference_idc>>5){
			case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
			case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
			case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
			case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
		}

		FILE *myout=stdout;
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

int h264Parser::h264_parser_Slice_Header(unsigned char * buffer, unsigned int bufferlen){
	unsigned int StartBit=0;
	de_emulation_prevention(buffer,&bufferlen);
   int forbidden_zero_bit=u(1,buffer,StartBit);
   int nal_ref_idc=u(2,buffer,StartBit);
   int nal_unit_type=u(5,buffer,StartBit);
   int field_pic_flag;
   int picture_structure;
   int ret;
   int list_count;

   int first_mb_in_slice = Ue(buffer,bufferlen,StartBit);
   int slice_type = Ue(buffer,bufferlen,StartBit);//1:I Slice,0
   int pic_parameter_set_id = Ue(buffer,bufferlen,StartBit);

   frame_num = u(m_h264_paramter.sps[0].log2_max_frame_num,buffer,StartBit);

   if(m_h264_paramter.sps[0].frame_mbs_only_flag){
	   picture_structure = 3;/*ffmpeg中PICT_FRAME*/
   }
   else{
	   field_pic_flag = u(1,buffer,StartBit);
	   if(field_pic_flag){
		   int bottom_field_flag = u(1,buffer,StartBit);
		   picture_structure = 1 + bottom_field_flag;
	   }else{
		   picture_structure = 3;
	   }
   }

   if(nal_unit_type == 5){
	   idr_pic_id = Ue(buffer,bufferlen,StartBit);/*idr_pic_id*/
   }

   if(m_h264_paramter.sps[m_h264_paramter.spslen-1].pic_order_cnt_type == 0){//第一种POC的求取方法
	   //log2_max_pic_order_cnt_lsb_minus4+4
	   pic_order_cnt_lsb = u((m_h264_paramter.sps[m_h264_paramter.spslen-1].log2_max_pic_order_cnt_lsb_minus),buffer,StartBit);
	   h264_getPOC();
	   if(m_h264_paramter.pps[m_h264_paramter.ppslen-1].pic_order_present_flag == 1 && picture_structure == 3){
		   /*delta_poc_bottom = */Se(buffer,bufferlen,StartBit);
	   }
   	 }
   else if((m_h264_paramter.sps[m_h264_paramter.spslen-1].pic_order_cnt_type == 1)
		   &&(!m_h264_paramter.sps[m_h264_paramter.spslen-1].delta_pic_order_always_zero_flag)){//第二种POC求取方法
	   delta_pci_order_cnt[0] = Se(buffer,bufferlen,StartBit);
	   if((m_h264_paramter.pps[m_h264_paramter.ppslen-1].pic_order_present_flag)&&(picture_structure == 3)){
		   delta_pci_order_cnt[1] = Se(buffer,bufferlen,StartBit);
	   }
	   //
		//int h264_getPOC2();


   }

   if((m_h264_paramter.pps[m_h264_paramter.ppslen-1].redundant_pic_cnt_present_flag)){
	   /*redundant_pic_cnt  冗余片的id号 */Ue(buffer,bufferlen,StartBit);
   }

   if((slice_type == 3)){//B
	   /*direct_spatial_mv_pred_flag  B图像在直接预测模式下，1:空间预测，0:时间预测 */
	   u(1,buffer,StartBit);
   }
   h264_parse_ref_count(slice_type,buffer,bufferlen,StartBit,list_count,picture_structure);

   if(slice_type != 1){
	   //ret = ff_h264_decode_ref_pic_list_reordering()
	   h264_decode_ref_pic_list_reordering(buffer,bufferlen,StartBit,list_count);
   }

   if(((m_h264_paramter.pps[m_h264_paramter.ppslen-1].weighted_pred_flag)&&((slice_type == 2)))||
		   ((m_h264_paramter.pps[m_h264_paramter.ppslen-1].weighted_bipred_idc == 1)&&((slice_type == 3)))){
		  //ff_h264_pred_weight_table();
	   h264_pred_weight_table(buffer,bufferlen,StartBit);
   }

   if(nal_ref_idc){
	   //ff_h264_decode_ref_pic_marking();
	   preMMCO = h264_decode_ref_pic_marking(buffer,bufferlen,StartBit,slice_type);
   }
   //TODO:to be continue...



   return 0;
}

int h264Parser::h264_getPOC(){

	 if(pic_order_cnt_lsb  <  PrevPicOrderCntLsb  &&
	      (PrevPicOrderCntLsb - pic_order_cnt_lsb )  >=  ( MaxPicOrderCntLsb / 2 ) ){
	      PicOrderCntMsb = PrevPicOrderCntMsb + MaxPicOrderCntLsb;
	 }
	 else if ( pic_order_cnt_lsb  >  PrevPicOrderCntLsb  &&
	      ( pic_order_cnt_lsb - PrevPicOrderCntLsb )  >  ( MaxPicOrderCntLsb / 2 ) ){
	      PicOrderCntMsb = PrevPicOrderCntMsb - MaxPicOrderCntLsb;
	 }
	 else{
	      PicOrderCntMsb = PrevPicOrderCntMsb;
	 }
	 CurPOC = (PicOrderCntMsb+pic_order_cnt_lsb);
	 printf("%d\n",CurPOC);
	 vecPoc.push_back(CurPOC);
	 PrePOC = CurPOC;
	 PrevPicOrderCntLsb = pic_order_cnt_lsb;
	 PrevPicOrderCntMsb = PicOrderCntMsb;
	 return PicOrderCntMsb+pic_order_cnt_lsb;
}

int h264Parser::h264_getPOC1(){
    int abs_frame_num, expected_delta_per_poc_cycle, expectedpoc;
    int i;
    frame_offset = pre_frame_offset;
    if(frame_num < pre_frame_offset){
    	frame_offset +=MaxPicOrderCntLsb;
    }
    if(m_h264_paramter.sps[m_h264_paramter.spslen-1].num_ref_frames_in_pic_order_cnt_cycle != 0){
    	abs_frame_num = frame_offset+frame_num;
    }else{
    	abs_frame_num = 0;
    }
    if(idr_pic_id == 0 && abs_frame_num > 0){
    	abs_frame_num--;
    }
    expected_delta_per_poc_cycle = 0;

    for(int i = 0;i < m_h264_paramter.sps[m_h264_paramter.spslen-1].num_ref_frames_in_pic_order_cnt_cycle;i++){
    	expected_delta_per_poc_cycle += m_h264_paramter.sps[m_h264_paramter.spslen-1].offset_for_ref_frame[i];
    }
    if(abs_frame_num > 0){
    	int poc_cycle_cnt = (abs_frame_num - 1)/m_h264_paramter.sps[m_h264_paramter.spslen-1].num_ref_frames_in_pic_order_cnt_cycle;
    	int frame_num_int_poc_cycle = (abs_frame_num - 1)%m_h264_paramter.sps[m_h264_paramter.spslen-1].num_ref_frames_in_pic_order_cnt_cycle;
    	expectedpoc = poc_cycle_cnt * expected_delta_per_poc_cycle;
    	for(i = 0;i < frame_num_int_poc_cycle;i++){
    		expectedpoc = expectedpoc + m_h264_paramter.sps[m_h264_paramter.spslen-1].offset_for_ref_frame[i];
    	}
    }else{
    	expectedpoc = 0;
    }
    if(idr_pic_id == 0){
    	expectedpoc = expectedpoc + m_h264_paramter.sps[m_h264_paramter.spslen-1].offset_for_non_ref_pic;
    }
    int poc = expectedpoc+delta_pci_order_cnt[0];
    vecPoc.push_back(poc);
}

void h264Parser::h264_resetPOC1(){
	//在reset之前进行检查是否丢帧
	sort(vecPoc.begin(),vecPoc.end());
	for(int i = 0;(i < vecPoc.size()-1)&&(!vecPoc.empty());i++){
		if(((vecPoc[i]+2) != vecPoc[i+1])){
			printf("miss pic :last poc=%d,next poc = %d\n",vecPoc[i],vecPoc[i+1]);
		}
	}
	pre_frame_offset = 0;
	vecPoc.clear();
}

void h264Parser::h264_resetPOC(){
	//在reset之前进行检查是否丢帧
	sort(vecPoc.begin(),vecPoc.end());
	for(int i = 0;(i < vecPoc.size()-1)&&(!vecPoc.empty());i++){
		if(((vecPoc[i]+2) != vecPoc[i+1])){
			printf("miss pic :last poc=%d,next poc = %d\n",vecPoc[i],vecPoc[i+1]);
		}
	}
	vecPoc.clear();

	PrevPicOrderCntLsb = 0;
	PrevPicOrderCntMsb = 0;
	PrePOC             = 0;
}

int h264Parser::h264_parser_pps(unsigned char * buffer, unsigned int bufferlen){
	unsigned int StartBit=0;
   de_emulation_prevention(buffer,&bufferlen);
   //========NAL Header=========
    /*int forbidden_zero_bit=*/u(1,buffer,StartBit);
    int nal_ref_idc = u(2,buffer,StartBit);
    int nal_unit_type=u(5,buffer,StartBit);

    //========Parser pps=========
    if(nal_unit_type == 8){
    	m_h264_paramter.pps[m_h264_paramter.ppslen].pic_parameter_set_id = Ue(buffer,bufferlen,StartBit);// 1 ue(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].seq_parameter_set_id = Ue(buffer,bufferlen,StartBit);// 1 ue(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].entropy_coding_mode_flag = u(1,buffer,StartBit);// 1 u(1)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].pic_order_present_flag = u(1,buffer,StartBit);// 1 u(1)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].num_slice_groups_minus = Ue(buffer,bufferlen,StartBit) + 1;// 1 ue(v)
    	if((m_h264_paramter.pps[m_h264_paramter.ppslen].num_slice_groups_minus-1) > 0){
    		m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type = Ue(buffer,bufferlen,StartBit);
    		if(m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 0){
    			for(int iGroup = 0;iGroup < (m_h264_paramter.pps[m_h264_paramter.ppslen].num_slice_groups_minus-1);iGroup++){
    				m_h264_paramter.pps[m_h264_paramter.ppslen].run_length_minus1 = Ue(buffer,bufferlen,StartBit);
    			}
    		}else if(m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 2){
    			for(int iGroup = 0;iGroup < (m_h264_paramter.pps[m_h264_paramter.ppslen].num_slice_groups_minus-1);iGroup++){
    				m_h264_paramter.pps[m_h264_paramter.ppslen].top_left = Ue(buffer,bufferlen,StartBit);
    				m_h264_paramter.pps[m_h264_paramter.ppslen].bottom_right = Ue(buffer,bufferlen,StartBit);
    			}
    		}else if((m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 3)||
    					(m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 4)||
						(m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 5)){
    			m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_change_direction_flag = u(1,buffer,StartBit);
    			m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_change_rate_minus1 = Ue(buffer,bufferlen,StartBit);
    		}else if(m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_map_type == 6){
    			m_h264_paramter.pps[m_h264_paramter.ppslen].pic_size_in_map_units_minus1 = Ue(buffer,bufferlen,StartBit);
    			for(int i = 0;i < m_h264_paramter.pps[m_h264_paramter.ppslen].pic_size_in_map_units_minus1;i++){
    				m_h264_paramter.pps[m_h264_paramter.ppslen].slice_group_id = Ue(buffer,bufferlen,StartBit);
    			}
    		}
    	}

    	m_h264_paramter.pps[m_h264_paramter.ppslen].num_ref_idx_l0_default_active_minus1 = Ue(buffer,bufferlen,StartBit);// 1 ue(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].num_ref_idx_l1_default_active_minus1 = Ue(buffer,bufferlen,StartBit);// 1 ue(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].weighted_pred_flag = u(1,buffer,StartBit);// 1 u(1)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].weighted_bipred_idc = u(2,buffer,StartBit);// 1 u(2)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].pic_init_qp_minus26 = Se(buffer,bufferlen,StartBit);// /* relative to 26 */ 1 se(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].pic_init_qs_minus26 = Se(buffer,bufferlen,StartBit);// /* relative to 26 */ 1 se(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].chroma_qp_index_offset = Se(buffer,bufferlen,StartBit);// 1 se(v)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].deblocking_filter_control_present_flag = u(1,buffer,StartBit);// 1 u(1)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].constrained_intra_pred_flag = u(1,buffer,StartBit);// 1 u(1)
    	m_h264_paramter.pps[m_h264_paramter.ppslen].redundant_pic_cnt_present_flag = u(1,buffer,StartBit);// 1 u(1)

    	m_h264_paramter.ppslen++;
    }else{
    	return -1;
    }
    return 1;
}


int h264Parser::h264_parser_sps(unsigned char * buf, unsigned int nLen){
	unsigned int StartBit=0;
   de_emulation_prevention(buf,&nLen);

   //========NAL Header=========
    /*int forbidden_zero_bit=*/u(1,buf,StartBit);
    /*int nal_ref_idc=*/u(2,buf,StartBit);
    int nal_unit_type=u(5,buf,StartBit);

    //========Parser sps=========
    if(nal_unit_type==7)
    {
    		m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc = u(8,buf,StartBit);
    		u(8,buf,StartBit);//固定8字节
    		m_h264_paramter.sps[m_h264_paramter.spslen].level_idc = u(8,buf,StartBit);
    		//表示当前的序列参数集的id。通过该id值，图像参数集pps可以引用其代表的sps中的参数。
    		m_h264_paramter.sps[m_h264_paramter.spslen].seq_parameter_set_id=Ue(buf,nLen,StartBit);

    		if( m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 100
    			|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 110
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 122
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 244
    			|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 44
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 83
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 86
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 118
				|| m_h264_paramter.sps[m_h264_paramter.spslen].profile_idc == 128)
        {
    			m_h264_paramter.sps[m_h264_paramter.spslen].chroma_format_idc = Ue(buf,nLen,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].chroma_format_idc == 3 ){
            	m_h264_paramter.sps[m_h264_paramter.spslen].separate_colour_plane_flag = u(1,buf,StartBit);
               }
            m_h264_paramter.sps[m_h264_paramter.spslen].bit_depth_luma = Ue(buf,nLen,StartBit)+8;
            m_h264_paramter.sps[m_h264_paramter.spslen].bit_depth_chroma = Ue(buf,nLen,StartBit)+8;
            m_h264_paramter.sps[m_h264_paramter.spslen].qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
            m_h264_paramter.sps[m_h264_paramter.spslen].seq_scaling_matrix_present_flag = u(1,buf,StartBit);

            int seq_scaling_list_present_flag[8];
            if(m_h264_paramter.sps[m_h264_paramter.spslen].seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < ( ( m_h264_paramter.sps[m_h264_paramter.spslen].chroma_format_idc  !=  3 ) ? 8 : 12 ); i++ ) {
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

    		m_h264_paramter.sps[m_h264_paramter.spslen].log2_max_frame_num=Ue(buf,nLen,StartBit)+4;
    		m_h264_paramter.sps[m_h264_paramter.spslen].pic_order_cnt_type=Ue(buf,nLen,StartBit);

        if(m_h264_paramter.sps[m_h264_paramter.spslen].pic_order_cnt_type == 0){

        	m_h264_paramter.sps[m_h264_paramter.spslen].log2_max_pic_order_cnt_lsb_minus = (Ue(buf,nLen,StartBit)+4);
        	int c = (m_h264_paramter.sps[m_h264_paramter.spslen].log2_max_pic_order_cnt_lsb_minus);
        	MaxPicOrderCntLsb = 1<<c;
        }
        else if(m_h264_paramter.sps[m_h264_paramter.spslen].pic_order_cnt_type == 1 )
        {
        	m_h264_paramter.sps[m_h264_paramter.spslen].delta_pic_order_always_zero_flag=u(1,buf,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].offset_for_non_ref_pic = Se(buf,nLen,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].offset_for_top_to_bottom_field = Se(buf,nLen,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].num_ref_frames_in_pic_order_cnt_cycle = Ue(buf,nLen,StartBit);
         for( int i = 0; i < m_h264_paramter.sps[m_h264_paramter.spslen].num_ref_frames_in_pic_order_cnt_cycle; i++ )
        	  m_h264_paramter.sps[m_h264_paramter.spslen].offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
        }
        m_h264_paramter.sps[m_h264_paramter.spslen].max_num_ref_frames = Ue(buf,nLen,StartBit);
        m_h264_paramter.sps[m_h264_paramter.spslen].gaps_in_frame_num_allowed_flag = u(1,buf,StartBit);
        m_h264_paramter.sps[m_h264_paramter.spslen].pic_width_in_mbs_minus1 = Ue(buf,nLen,StartBit);
        m_h264_paramter.sps[m_h264_paramter.spslen].pic_height_in_map_units_minus1 = Ue(buf,nLen,StartBit);
        m_h264_paramter.sps[m_h264_paramter.spslen].frame_mbs_only_flag = u(1,buf,StartBit);

        if(!m_h264_paramter.sps[m_h264_paramter.spslen].frame_mbs_only_flag){
        	m_h264_paramter.sps[m_h264_paramter.spslen].mb_adaptive_frame_field_flag = u(1,buf,StartBit);
        }

        m_h264_paramter.sps[m_h264_paramter.spslen].direct_8x8_inference_flag = u(1,buf,StartBit);
        m_h264_paramter.sps[m_h264_paramter.spslen].frame_cropping_flag = u(1,buf,StartBit);
        if(m_h264_paramter.sps[m_h264_paramter.spslen].frame_cropping_flag)
        {
        	m_h264_paramter.sps[m_h264_paramter.spslen].crop_left = Ue(buf,nLen,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].crop_right = Ue(buf,nLen,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].crop_top = Ue(buf,nLen,StartBit);
        	m_h264_paramter.sps[m_h264_paramter.spslen].crop_bottom = Ue(buf,nLen,StartBit);
        }

        //--------------------------------------vui--------------------------------
        m_h264_paramter.sps[m_h264_paramter.spslen].vui_parameters_present_flag = u(1,buf,StartBit);
        if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_parameters_present_flag)
        {
        	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.aspect_ratio_info_present_flag = u(1,buf,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.aspect_ratio_info_present_flag)
            {
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.aspect_ratio_idc=u(8,buf,StartBit);
                if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.aspect_ratio_idc==255)
                {
                	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.sar_width = u(16,buf,StartBit);
                	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.sar_height = u(16,buf,StartBit);
                }
            }
            m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.overscan_info_present_flag = u(1,buf,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.overscan_info_present_flag){
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.overscan_appropriate_flag = u(1,buf,StartBit);
            }
            m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.video_signal_type_present_flag = u(1,buf,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.video_signal_type_present_flag)
            {
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.video_format = u(3,buf,StartBit);
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.video_full_range_flag = u(1,buf,StartBit);
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.colour_description_present_flag = u(1,buf,StartBit);
                if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.colour_description_present_flag)
                {
                    m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.colour_primaries = u(8,buf,StartBit);
                    m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.transfer_characteristic = u(8,buf,StartBit);
                    m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.matrix_coeffs = u(8,buf,StartBit);
                }
            }
            m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.chroma_loc_info_present_flag = u(1,buf,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.chroma_loc_info_present_flag)
            {
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.chroma_sample_loc_type_top_field = Ue(buf,nLen,StartBit);
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.chroma_sample_loc_type_bottom_field = Ue(buf,nLen,StartBit);
            }

            m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.timing_info_present_flag = u(1,buf,StartBit);
            if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.timing_info_present_flag)
            {
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.num_units_in_tick = u(32,buf,StartBit);
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.time_scale = u(32,buf,StartBit);
            	m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.fix_frame_rate_flag = u(1,buf,StartBit);
                if(m_h264_paramter.sps[m_h264_paramter.spslen].vui_param.fix_frame_rate_flag)
                {
                    //to be continue...
                }
            }
        }
        m_h264_paramter.spslen++;
        return true;
    }
    else
        return false;
}

/*return ：是sei的的类型ID*/
int h264Parser::h264_parser_sei(unsigned char * buffer, unsigned int bufferlen){

	unsigned int StartBit = 0;
	de_emulation_prevention(buffer,&bufferlen);

	/*int byte = */u(8,buffer,StartBit);//READ_CODE(8,"NO_USE");

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
	unsigned int  useDefaultScalingMatrixFlag = -1;
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

int h264Parser::h264_parse_ref_count(int sliceType,unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int &list_count,int picture_structure){

	int num_ref_idx_active_override_flag;
	int max[2];
	int ref_count_0 = m_h264_paramter.pps[m_h264_paramter.ppslen-1].num_ref_idx_l0_default_active_minus1+1;
	int ref_count_1 = m_h264_paramter.pps[m_h264_paramter.ppslen-1].num_ref_idx_l1_default_active_minus1+1;
	if(sliceType != 1){
		max[0] = max[1] = picture_structure == 1 ? 15 : 31;
		num_ref_idx_active_override_flag = u(1,buffer,StartBit);
		if(num_ref_idx_active_override_flag){
			ref_count_0 = Ue(buffer,bufferlen,StartBit) + 1;
			if(sliceType == 3){
				ref_count_1 = Ue(buffer,bufferlen,StartBit) + 1;
			}else{
				ref_count_1 = 1;
			}
		}
		if(ref_count_0 -1 > max[0] || ref_count_1 - 1 > max[1]){
			ref_count_0 = ref_count_1 = 0;
			list_count = 0;
			goto fail;
		}
		if(sliceType == 3){
			list_count = 2;
		}
		else{
			list_count = 1;
		}
	}else{
		list_count = 0;
		ref_count_0 = ref_count_1 = 0;
	}
	return 0;
fail:
list_count = 0;
ref_count_0 = 0;
ref_count_1 = 0;
	return 1;
}

int h264Parser::h264_decode_ref_pic_list_reordering(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int list_count){

    int list, index;
    for (list = 0; list < list_count; list++) {
        if (!u(1,buffer,StartBit))    // ref_pic_list_modification_flag_l[01]
            continue;

        for (index = 0; ; index++) {
            unsigned int op = Ue(buffer,bufferlen,StartBit);

            if (op == 3)
                break;

            if (op > 2) {
                return 0;
            }
            Ue(buffer,bufferlen,StartBit);


        }
    }
	return 1;
}

int h264Parser::h264_pred_weight_table(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit){
    int luma_def, chroma_def;

    int luma_log2_weight_denom = Ue(buffer,bufferlen,StartBit);
    if (luma_log2_weight_denom > 7U) {
        luma_log2_weight_denom = 0;
    }

    if (m_h264_paramter.sps[m_h264_paramter.spslen-1].chroma_format_idc) {
    	Ue(buffer,bufferlen,StartBit);
    }
    return 0;
}

int h264Parser::h264_decode_ref_pic_marking(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int slice_type){
int mmco;
int opcode;
	if(slice_type == 5){
		u(1,buffer,StartBit);
		if(u(1,buffer,StartBit)){
			mmco = 1;
		}
		else{
			int i = 0;
			if(u(1,buffer,StartBit)){
				for(i = 0;i < 66;i++){
					opcode = Ue(buffer,bufferlen,StartBit);
					if(opcode == 1 || opcode == 3){
						Ue(buffer,bufferlen,StartBit);
					}
					if(opcode == 2 || opcode == 3||opcode == 4 || opcode == 6){
						Ue(buffer,bufferlen,StartBit);
					}
					if(opcode == 0){
						break;
					}
				}
				mmco = i;
			}

		}
	}

	return mmco;
}



