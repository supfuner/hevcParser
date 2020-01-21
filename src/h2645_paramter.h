/*
 * h2645_paramter.h
 *
 *  Created on: 2018年12月14日
 *      Author: root
 */

#ifndef HEVCPARSER_H2645_PARAMTER_H_
#define HEVCPARSER_H2645_PARAMTER_H_
#define SHOW_DETAIL 1

#define QP_MAX_NUM (51 + 6*6)           //最大支持的量化参数
#define MAX_SPS_COUNT         256
#define MAX_PPS_COUNT         256
#define MAX_LOG2_MAX_FRAME_NUM    (12 + 4

typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! Nal Unit Buffer size
	int forbidden_bit;            //! should be always FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx
	char *buf;                    //! contains the first byte followed by the EBSP
} NALU_t;

typedef enum
{
   H265_NAL_UNIT_CODED_SLICE_TRAIL_N = 0, // 0
	H265_NAL_UNIT_CODED_SLICE_TRAIL_R,     // 1

	H265_NAL_UNIT_CODED_SLICE_TSA_N,       // 2
	H265_NAL_UNIT_CODED_SLICE_TSA_R,       // 3

	H265_NAL_UNIT_CODED_SLICE_STSA_N,      // 4
	H265_NAL_UNIT_CODED_SLICE_STSA_R,      // 5

	H265_NAL_UNIT_CODED_SLICE_RADL_N,      // 6
	H265_NAL_UNIT_CODED_SLICE_RADL_R,      // 7

	H265_NAL_UNIT_CODED_SLICE_RASL_N,      // 8
	H265_NAL_UNIT_CODED_SLICE_RASL_R,      // 9

	H265_NAL_UNIT_RESERVED_VCL_N10,
	H265_NAL_UNIT_RESERVED_VCL_R11,
	H265_NAL_UNIT_RESERVED_VCL_N12,
	H265_NAL_UNIT_RESERVED_VCL_R13,
	H265_NAL_UNIT_RESERVED_VCL_N14,
	H265_NAL_UNIT_RESERVED_VCL_R15,

	H265_NAL_UNIT_CODED_SLICE_BLA_W_LP,    // 16
	H265_NAL_UNIT_CODED_SLICE_BLA_W_RADL,  // 17
	H265_NAL_UNIT_CODED_SLICE_BLA_N_LP,    // 18
	H265_NAL_UNIT_CODED_SLICE_IDR_W_RADL,  // 19
	H265_NAL_UNIT_CODED_SLICE_IDR_N_LP,    // 20
	H265_NAL_UNIT_CODED_SLICE_CRA,         // 21
	H265_NAL_UNIT_RESERVED_IRAP_VCL22,
	H265_NAL_UNIT_RESERVED_IRAP_VCL23,

	H265_NAL_UNIT_RESERVED_VCL24,
	H265_NAL_UNIT_RESERVED_VCL25,
	H265_NAL_UNIT_RESERVED_VCL26,
	H265_NAL_UNIT_RESERVED_VCL27,
	H265_NAL_UNIT_RESERVED_VCL28,
	H265_NAL_UNIT_RESERVED_VCL29,
	H265_NAL_UNIT_RESERVED_VCL30,
	H265_NAL_UNIT_RESERVED_VCL31,

	H265_NAL_UNIT_VPS,                     // 32
	H265_NAL_UNIT_SPS,                     // 33
	H265_NAL_UNIT_PPS,                     // 34
	H265_NAL_UNIT_ACCESS_UNIT_DELIMITER,   // 35
	H265_NAL_UNIT_EOS,                     // 36
	H265_NAL_UNIT_EOB,                     // 37
	H265_NAL_UNIT_FILLER_DATA,             // 38
	H265_NAL_UNIT_PREFIX_SEI,              // 39
	H265_NAL_UNIT_SUFFIX_SEI,              // 40

	H265_NAL_UNIT_RESERVED_NVCL41,
	H265_NAL_UNIT_RESERVED_NVCL42,
	H265_NAL_UNIT_RESERVED_NVCL43,
	H265_NAL_UNIT_RESERVED_NVCL44,
	H265_NAL_UNIT_RESERVED_NVCL45,
	H265_NAL_UNIT_RESERVED_NVCL46,
	H265_NAL_UNIT_RESERVED_NVCL47,
	H265_NAL_UNIT_UNSPECIFIED_48,
	H265_NAL_UNIT_UNSPECIFIED_49,
	H265_NAL_UNIT_UNSPECIFIED_50,
	H265_NAL_UNIT_UNSPECIFIED_51,
	H265_NAL_UNIT_UNSPECIFIED_52,
	H265_NAL_UNIT_UNSPECIFIED_53,
	H265_NAL_UNIT_UNSPECIFIED_54,
	H265_NAL_UNIT_UNSPECIFIED_55,
	H265_NAL_UNIT_UNSPECIFIED_56,
	H265_NAL_UNIT_UNSPECIFIED_57,
	H265_NAL_UNIT_UNSPECIFIED_58,
	H265_NAL_UNIT_UNSPECIFIED_59,
	H265_NAL_UNIT_UNSPECIFIED_60,
	H265_NAL_UNIT_UNSPECIFIED_61,
	H265_NAL_UNIT_UNSPECIFIED_62,
	H265_NAL_UNIT_UNSPECIFIED_63,
	H265_NAL_UNIT_INVALID,
} H265NalUnitType;

typedef enum {
	NALU_PRIORITY_DISPOSABLE = 0,
	NALU_PRIRITY_LOW         = 1,
	NALU_PRIORITY_HIGH       = 2,
	NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;

typedef struct AVRational{
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;

//==========================================================h264===

typedef struct VUI {
	 int aspect_ratio_info_present_flag;
	 int aspect_ratio_idc;
	 int sar_width;
	 int sar_height;
   int overscan_info_present_flag;
   int overscan_appropriate_flag;
   int video_signal_type_present_flag;
   int video_format;
   int video_full_range_flag;
   int colour_description_present_flag;
   unsigned char colour_primaries;
   unsigned char transfer_characteristic;
   unsigned char matrix_coeffs;
   int chroma_loc_info_present_flag;
   int chroma_sample_loc_type_top_field;
   int chroma_sample_loc_type_bottom_field;
   int timing_info_present_flag;
   unsigned int num_units_in_tick;
   unsigned int time_scale;
   int fix_frame_rate_flag;
   int nal_hrd_parameters_present_flag;
   int vlc_hrd_parameters_present_flag;
   //TODO:to be continue...

} VUI;

/**
 * Sequence parameter set 参数祥解https://blog.csdn.net/shaqoneal/article/details/52771030
 */
typedef struct H264_SPS {

    int profile_idc;
    int constraint_set_flag_reserved_zero_2bits; //constraint_set_flag(0-5)+reserved_zero_2bits
    int level_idc;
    unsigned int seq_parameter_set_id;
    int chroma_format_idc;
    int separate_colour_plane_flag;
    int bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
    int qpprime_y_zero_transform_bypass_flag;              ///< qpprime_y_zero_transform_bypass_flag
    int seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag[8];
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int pic_order_cnt_type;                      ///< pic_order_cnt_type
    int log2_max_pic_order_cnt_lsb_minus;              ///< log2_max_pic_order_cnt_lsb_minus4+4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;              ///< num_ref_frames_in_pic_order_cnt_cycle
    short offset_for_ref_frame[256]; // FIXME dyn aloc?
    int max_num_ref_frames;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int pic_width_in_mbs_minus1;                      ///< pic_width_in_mbs_minus1 + 1
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int mb_adaptive_frame_field_flag;                        ///< mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int frame_cropping_flag;                          ///< frame_cropping_flag
    /* those 4 are already in luma samples */
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    VUI  vui_param;                    //vui里面的参数

//    unsigned char scaling_matrix4[6][16];
//    unsigned char scaling_matrix8[6][64];
    unsigned char data[4096];
    int data_size;
} H264_SPS;

/**
 * Picture parameter set
 */
typedef struct H264_PPS {
	int pic_parameter_set_id;// 1 ue(v)
	int seq_parameter_set_id;// 1 ue(v)
	int entropy_coding_mode_flag;// 1 u(1)
	int pic_order_present_flag;// 1 u(1)
	int num_slice_groups_minus;// 1 ue(v)num_slice_groups_minus1+1
	int slice_group_map_type;
	int run_length_minus1;//run_length_minus1[ iGroup ]
	int top_left ;//top_left[ iGroup ]
	int bottom_right;//bottom_right[ iGroup ]
	int slice_group_change_direction_flag;
	int slice_group_change_rate_minus1;
	int pic_size_in_map_units_minus1;
	int slice_group_id;//slice_group_id[ i ]

	int num_ref_idx_l0_default_active_minus1;// 1 ue(v)
	int num_ref_idx_l1_default_active_minus1;// 1 ue(v)
	int weighted_pred_flag;// 1 u(1)
	int weighted_bipred_idc;// 1 u(2)
	int pic_init_qp_minus26;// /* relative to 26 */ 1 se(v)
	int pic_init_qs_minus26;// /* relative to 26 */ 1 se(v)
	int chroma_qp_index_offset;// 1 se(v)
	int deblocking_filter_control_present_flag;// 1 u(1)
	int constrained_intra_pred_flag;// 1 u(1)
	int redundant_pic_cnt_present_flag;// 1 u(1)
	//TODO:to be continue....
} H264_PPS;

typedef struct H264ParamSets {
//	streamBuffer *sps_data[MAX_SPS_COUNT];
//	streamBuffer *pps_data[MAX_PPS_COUNT];
	int      spslen;
	int      ppslen;

	H264_SPS sps[MAX_SPS_COUNT];
	H264_PPS pps[MAX_PPS_COUNT];

} H264ParamSets;
//==========================================================h264===

#endif /* HEVCPARSER_H2645_PARAMTER_H_ */
