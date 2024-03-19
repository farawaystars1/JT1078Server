#ifndef __CODEC__H
#define __CODEC__H
//extern"C"
//{
//#include<libavcodec/avcodec.h>
//}
#include"Adpcm.h"
#include"G711.h"
#include"G726.h"
#include<iostream>
#include<functional>
#include<memory.h>
using namespace std;

enum AudioCodecType{
	ADPCM=26,
	G711_ALAW=6,
	G711_ULAW=7,
	G726,
	Unsupported
};
class Codec {
public:
	Codec(uint8_t* src_data, int src_size, AudioCodecType type);
	~Codec();
public:
	int decode(const uint8_t* src_data, int src_size, short* des_data)const;
	int encode(const short* src_data, int src_size, uint8_t* des_data)const;
private:
	int _g726_init(uint8_t* src_data, int src_size, AudioCodecType type);
	int _g726_decode(const uint8_t* src_data, int src_size, short* des_data)const;//此为g726 （g726le可用ffmpeg解码器AV_CODEC_ID_ADPCM_G726LE解即可）
	int _g726_encode(const short* src_data, int src_size, uint8_t* des_data)const;
	int _g711a_decode(const uint8_t* src_data, int src_size, short* des_data)const;
	int _g711u_decode(const uint8_t* src_data, int src_size, short* des_data)const;
	int _g711a_encode(const short* src_data, int src_size, uint8_t* des_data)const;
	int _g711u_encode(const short* src_data, int src_size, uint8_t* des_data)const;
 	int _adpcm_decode(const uint8_t* src_data, int src_size, short* des_data)const;
	int _adpcm_encode(const short* src_data, int src_size, uint8_t* des_data)const;
	int _init(uint8_t* src_data, size_t src_size, AudioCodecType type);
private:
	function<int(const short*, int, uint8_t*)> enc_;
	function<int(const uint8_t*, int, short*)> dec_;
	g726_state_t* g726_st_;
	AudioCodecType codec_type_;
};
#endif //__CODEC__H
