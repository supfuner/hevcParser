
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "HEVCParser.h"
#include "H264Parser.h"
#include "h265Parser.h"
#define _HEVC_ 1
#define _H264_ 0


void stHDRMetadata_printf( HEVCParser::stHDRMetadata info){
	printf("video_format:%d\n",info.video_format);
	printf("color_primaries:%d\n",info.color_primaries);
	printf("transfer_characteristics:%d\n",info.transfer_characteristics);
	printf("matrix_coeffs:%d\n",info.matrix_coeffs);
	printf("chroma_sample_loc_type_bottom_field:%d\n",info.chroma_sample_loc_type_bottom_field);
	printf("chroma_sample_loc_type_top_field:%d\n",info.chroma_sample_loc_type_top_field);
	printf("video_full_range_flag:%d\n",info.video_full_range_flag);
	printf("===sei Content light level information SEI message syntax==\n");
	printf("MaxCLL:%d\n",info.MaxCLL);
	printf("MaxFall:%d\n",info.MaxFall);
	printf("===sei Mastering display colour volume SEI message syntax==\n");
	printf("display_primaries_x_r:%d\n",info.displayColorVolume.display_primaries_x_r);
	printf("display_primaries_y_r:%d\n",info.displayColorVolume.display_primaries_y_r);
	printf("display_primaries_x_g:%d\n",info.displayColorVolume.display_primaries_x_g);
	printf("display_primaries_y_g:%d\n",info.displayColorVolume.display_primaries_y_g);
	printf("display_primaries_x_b:%d\n",info.displayColorVolume.display_primaries_x_b);
	printf("display_primaries_y_b:%d\n",info.displayColorVolume.display_primaries_y_b);
	printf("white_point_x:%d\n",info.displayColorVolume.white_point_x);
	printf("white_point_y:%d\n",info.displayColorVolume.white_point_y);

	printf("max_display_mastering_luminance:%d\n",info.displayColorVolume.max_display_mastering_luminance);
	printf("min_display_mastering_luminance:%d\n",info.displayColorVolume.min_display_mastering_luminance);
}


int main(int argc,char** argv){

//	FILE * fp = fopen("/mnt/hgfs/shareDisk/测试分片/manguo/bili.h265","rb");
//0 /mnt/hgfs/shareDisk/测试分片/花屏/1541948570.h264
    int type = atoi(argv[1]);
    std::string filePath = "/mnt/hgfs/shareDisk/测试分片/manguo/bili.h265";//argv[2];

    if(argc != 3){
    	printf("param num Err\n");
    	return 0;
    }
    printf("argc:\t%d\n",argc);
    printf("file:\t%s\n",filePath.c_str());

    FILE * fp = fopen(filePath.c_str(),"rb");
    if(fp == NULL){
    	printf("Can not Open file\n");
    	return 0;
    }
    fseek(fp,0L,SEEK_END);
    long int nfilesize = ftell(fp);
    rewind(fp);
    printf("nfilesize:\t%ld\n",nfilesize);
	 unsigned char* buffer = new unsigned char[nfilesize];
	 unsigned int bufferlen = fread(buffer,1,nfilesize,fp);
	 HEVCParser::stHDRMetadata info;

	 switch(type){
	 	 case 0:{
	 		 h264Parser *p = new h264Parser();
	 		 p->h264_init();
	 		 p->h264_parser(buffer,bufferlen,info);
	 		// stHDRMetadata_printf(p->h264info);
	 		 if(!p->m_isSps){
	 			 printf("Not found sps\n");
	 		 }
	 		 p->h264_uinit();
	 		 delete p;
	 		 break;
	 	 }
	 	 case 1:{
	 		 h265Parser *p = new h265Parser();
	 		 p->h265_parser(buffer,bufferlen,info);
	 		 //stHDRMetadata_printf(p->m_h265info);
	 		 if(!p->m_isSps){
	 			 printf("Not found sps\n");
	 		 }
	 		 delete p;
	 		 break;
	 	 }
	 }
	 if(buffer != NULL)
		 free(buffer);
	 printf("FINISH\n");
	 return 0;
}
