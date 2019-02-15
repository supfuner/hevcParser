/*
 * h265Parser.h
 *
 *  Created on: 2018年12月14日
 *      Author: root
 */

#ifndef H265PARSER_H_
#define H265PARSER_H_
#include "parser_util.h"
#include "h2645_paramter.h"
#include "HEVCParser.h"

#define H265_FRAME_MAX_LEN (1024*1024*2)

typedef enum
{
	H265_SEI_TYPE_PICTURE_TIMING = 1,

	H265_SEI_TYPE_ACTIVE_PARAMTER_SETS = 129,

	H265_SEI_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME = 137,

	H265_SEI_TYPE_CONTENT_LIGHT_LEVEL_INFO = 144,

}H265_SEI_TYPE;

class h265Parser{
private:
	streamBuffer m_h265_buffer;

	int m_start_code_size;
public:
	int h265_parser(unsigned char * buffer, unsigned int bufferlen,HEVCParser::stHDRMetadata & h264info);
	void h265_init(unsigned char * buffer, unsigned int bufferlen);
	void h265_uinit();
	HEVCParser::stHDRMetadata m_h265info;
	int m_isSps;
private:
	int h265_parser_sps(unsigned char * buffer, unsigned int bufferlen);
	void h265_parse_ptl(uint32_t max_sub_layers_minus1,unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen);
	void h265_parse_scaling_list(unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen);
	void h265_parse_vui(unsigned char * buffer,unsigned int &StartBit,unsigned int bufLen);

	int h265_parser_sei(unsigned char * buffer, unsigned int bufferlen);
	int h265_sei_message(unsigned char * buf,unsigned int &StartBit);
	int h265_sei_playload(int payload_type,unsigned char * buf,unsigned int &StartBit);
	int h265_mastering_display_colour_volume(unsigned char * buf,unsigned int &StartBit);
	int h265_content_light_level_info(unsigned char * buf,unsigned int &StartBit);
	int h265_active_parameter_sets(unsigned char * buf,unsigned int &StartBit);
	int h265_decode_nal_sei_decoded_picture_hash(unsigned char * buf,unsigned int &StartBit);
	int h265_GetAnnexbNALU (NALU_t *nalu);
};


#endif /* H265PARSER_H_ */
