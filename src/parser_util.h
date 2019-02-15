/*
 * parser_util.h
 *
 *  Created on: 2018年12月14日
 *      Author: root
 */

#ifndef PARSER_UTIL_H_
#define PARSER_UTIL_H_
#include <string.h>
#include <math.h>

#define parser_min(a, b) (((a) < (b)) ? (a) : (b))
#define parser_max(a, b) (((a) > (b)) ? (a) : (b))

//=============buffer struct==========
struct streamBuffer
{
	unsigned char * data;
	unsigned int length;
	unsigned int readIdx;
	unsigned int endIdx;
	streamBuffer(){
		data = NULL;
		length = 0;
		readIdx = 0;
		endIdx = 0;
	}
};

//=================parser function====
unsigned int static Ue(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit);

int static Se(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit);

unsigned int static u(unsigned int BitCount,unsigned char * buf,unsigned int &nStartBit);

unsigned int static next_bits(unsigned int BitCount,unsigned char * buf,unsigned int &nStartBit);

//NAL起始码防竞争机制
void static de_emulation_prevention(unsigned char* buf,unsigned int* buf_size);

//=================NALU function=====
/*func:00 00 01 起始字节
 *成功则返回：1*/
static int FindStartCode2 (unsigned char *Buf);

/*func:00 00 00 01 起始字节
 *成功则返回：1*/
static int FindStartCode3 (unsigned char *Buf);

/*func：从src中读取n个字节到dec中，指针向后移动n个字节
 *return：返回实际读取的字节数*/
static int read_buffer(unsigned char *dec,unsigned int n,streamBuffer &src);

/*func:从src中读取1个字节并返回，指针向后移动1个字节*/
static unsigned char get_char(streamBuffer &src);

/*func:移动src指针，与fseek()功能类似*/
static int buffer_seek(unsigned int n,streamBuffer &src);


unsigned int static Ue(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
	    //计算0bit的个数
		unsigned int nZeroNum = 0;
	    while (nStartBit < nLen * 8)
	    {
	        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
	        {
	            break;
	        }
	        nZeroNum++;
	        nStartBit++;
	    }
	    nStartBit ++;


	    //计算结果
	    unsigned int dwRet = 0;
	    for (unsigned int i=0; i<nZeroNum; i++)
	    {
	        dwRet <<= 1;
	        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
	        {
	            dwRet += 1;
	        }
	        nStartBit++;
	    }
	    return (1 << nZeroNum) - 1 + dwRet;
}


int static Se(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
	    int UeVal=Ue(pBuff,nLen,nStartBit);
	    double k=UeVal;
	    int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	    if (UeVal % 2==0)
	        nValue=-nValue;
	    return nValue;
}


unsigned int static u(unsigned int BitCount,unsigned char * buf,unsigned int &nStartBit)
{
		unsigned int dwRet = 0;
	    for (unsigned int i=0; i<BitCount; i++)
	    {
	        dwRet <<= 1;
	        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
	        {
	            dwRet += 1;
	        }
	        nStartBit++;
	    }
	    return dwRet;
}

unsigned int static next_bits(unsigned int BitCount,unsigned char * buf,unsigned int &nStartBit)
{
		unsigned int dwRet = 0;
		unsigned int startBit = nStartBit;
	   for (unsigned int i=0; i<BitCount; i++){
		   dwRet <<= 1;
			if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8))){
				dwRet += 1;
			}
			    nStartBit++;
	    }
	   /*nStartBit回退到之前的位置，比特流的指针不移动*/
	   nStartBit = startBit;
	   return dwRet;
}

	/**
	 * H264的NAL起始码防竞争机制
	 *
	 * @param buf SPS数据内容
	 *
	 * @无返回值
	 */
void static de_emulation_prevention(unsigned char* buf,unsigned int* buf_size)
{
	    int i=0,j=0;
	    unsigned char* tmp_ptr=NULL;
	    unsigned int tmp_buf_size=0;
	    int val=0;

	    tmp_ptr=buf;
	    tmp_buf_size=*buf_size;
	    for(i=0;i<(tmp_buf_size-2);i++)
	    {
	        //check for 0x000003
	        val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);
	        if(val==0)
	        {
	            //kick out 0x03
	            for(j=i+2;j<tmp_buf_size-1;j++)
	                tmp_ptr[j]=tmp_ptr[j+1];

	            //and so we should devrease bufsize
	            (*buf_size)--;
	        }
	    }
}

static int FindStartCode2 (unsigned char *Buf){
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //0x000001?
	else return 1;
}

static int FindStartCode3 (unsigned char *Buf){
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//0x00000001?
	else return 1;
}

static int read_buffer(unsigned char *dec,unsigned int n,streamBuffer &src){
	int ret = 0;
	if(src.length < n){
		ret = n-src.length;
	}
	else{
		ret = n;
	}
	memcpy(dec,src.data+src.readIdx,ret);
	src.length = src.length - ret;
	src.readIdx = src.readIdx + ret;
	return ret;
}

static unsigned char get_char(streamBuffer &src){
	unsigned char ret = *(src.data+src.readIdx);
	src.length = src.length - 1;
	src.readIdx = src.readIdx + 1;
	return ret;
}

static int buffer_seek(unsigned int n,streamBuffer &src){
	int ret = 0;
	int m = -n;
	if(src.length < m){
		ret = m-src.length;
		return 1;
	}
	else{
		src.length = src.length + m;
		src.readIdx = src.readIdx - m;
		return 0;
	}

}


#endif /* PARSER_UTIL_H_ */
