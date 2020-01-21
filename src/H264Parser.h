/*
 * H264Parser.h
 *
 *  Created on: 2018年12月11日
 *      Author: root
 */

#ifndef H264PARSER_H_
#define H264PARSER_H_
//#include <stdint.h>
#include <math.h>
#include <vector>
#include "parser_util.h"
#include "HEVCParser.h"
#include "h2645_paramter.h"

#define H264_FRAME_MAX_LEN (1024*1024*1024)
#define NAL_GET_5BIT(p) (*(p) & 0x1F)
#define NAL_GET_4BYTE(p) (*p<<24 | *(p+1)<<16 | *(p+2)<<8 |*(p+3))
#define NAL_GET_3BYTE(p) (*(p)<<16 | *(p+1)<<8 |*(p+2))

typedef enum {
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12,
} NaluType;

typedef enum
{
	H264_SEI_TYPE_BUFFERING_PERIOD = 0,

	H264_SEI_TYPE_PICTURE_TIMING = 1,

	H264_SEI_TYPE_USER_DATA = 5,

	H264_SEI_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME = 137,

}H264_SEI_TYPE;

class h264Parser{

private:
	streamBuffer 	m_h264_buffer;

	int 				m_start_code_size;//裸流起始字节的长度
	int 				m_code_core_id;
	H264ParamSets	m_h264_paramter;

public:
	int h264_parser(unsigned char * buffer, unsigned int bufferlen,HEVCParser::stHDRMetadata & h264info);
	void h264_init();
	void h264_uinit();
	HEVCParser::stHDRMetadata h264info;
	int m_isSps;
private:
	//用于求取poc的参数
	long int pic_order_cnt_lsb;
	long int PrevPicOrderCntLsb;
	long int MaxPicOrderCntLsb;
	long int PicOrderCntMsb;
	long int PrevPicOrderCntMsb;
	long int PrePOC;
	long int CurPOC;
	int preMMCO;
	int frame_offset;
	int pre_frame_offset;
	int idr_pic_id;
	int frame_num;
	int delta_pci_order_cnt[2];
	std::vector<int> vecPoc;


	//SPS
	int h264_parser_sps(unsigned char * buffer, unsigned int bufferlen);

	//Slice_header
	int h264_parser_Slice_Header(unsigned char * buffer, unsigned int bufferlen);
	int h264_parse_ref_count(int sliceType,unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int &list_count,int picture_structure);
	int h264_decode_ref_pic_list_reordering(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int list_count);
	int h264_pred_weight_table(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit);
	int h264_decode_ref_pic_marking(unsigned char * buffer, unsigned int bufferlen,unsigned int &StartBit,int slice_type);
	//PPS
	int h264_parser_pps(unsigned char * buffer, unsigned int bufferlen);




	//SEI
	int h264_parser_sei(unsigned char * buffer, unsigned int bufferlen);
	int h264_sei_message(unsigned char * buf,unsigned int &StartBit);
	int h264_sei_playload(int payload_type,unsigned char * buf,unsigned int &StartBit);
	int h264_mastering_display_colour_volume(unsigned char * buf,unsigned int &StartBit);
	int h264_decode_nal_sei_decoded_picture_hash(unsigned char * buf,unsigned int &StartBit);
	int h264_scaling_list(int sizeOfScalingList,unsigned char * buf,unsigned int &StartBit,int nLen);

	int h264_getPOC1();
	int h264_getPOC();
	void h264_resetPOC();
	void h264_resetPOC1();

private:
	int GetAnnexbNALU (NALU_t *nalu);

//======================parser=================
public:

};
#endif /* H264PARSER_H_ */
