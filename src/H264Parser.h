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
#include "parser_util.h"
#include "HEVCParser.h"
#include "h2645_paramter.h"

#define H264_FRAME_MAX_LEN (1024*1024)
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
	streamBuffer m_h264_buffer;

	int m_start_code_size;//裸流起始字节的长度
	int m_code_core_id;

public:
	int h264_parser(unsigned char * buffer, unsigned int bufferlen,HEVCParser::stHDRMetadata & h264info);
	HEVCParser::stHDRMetadata h264info;
	int m_isSps;
private:
	int h264_parser_sps(unsigned char * buffer, unsigned int bufferlen);
	int h264_parser_sei(unsigned char * buffer, unsigned int bufferlen);
	int h264_sei_message(unsigned char * buf,unsigned int &StartBit);
	int h264_sei_playload(int payload_type,unsigned char * buf,unsigned int &StartBit);
	int h264_mastering_display_colour_volume(unsigned char * buf,unsigned int &StartBit);
	int h264_decode_nal_sei_decoded_picture_hash(unsigned char * buf,unsigned int &StartBit);
	int h264_scaling_list(int sizeOfScalingList,unsigned char * buf,unsigned int &StartBit,int nLen);

private:
	int GetAnnexbNALU (NALU_t *nalu);

//======================parser=================
public:

};
#endif /* H264PARSER_H_ */
