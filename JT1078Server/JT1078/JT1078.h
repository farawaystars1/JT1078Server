#ifndef __JT1078__H
#define __JT1078__H
#include<vector>
#include<iostream>
#include<fstream>
#include<memory.h>
#include"../Codec/codec.h"

using namespace std;

class RtmpStream;

struct AudioPackage {
	uint64_t bt8_timestamp;
	uint8_t* data;
	uint16_t size;
	AudioPackage();
	~AudioPackage();
};
struct VideoPackage {
	uint64_t bt8_timestamp;
	uint16_t last_IFrame_interval;
	uint16_t last_frame_interval;
	uint8_t* data;
	size_t size;
	VideoPackage();
	~VideoPackage();
};
struct PassThroughPackage {
	uint8_t* data;
	uint16_t size;
	PassThroughPackage();;
	~PassThroughPackage();
};
class JT1078Package {
public:
	JT1078Package();
	~JT1078Package();
	int parse(const uint8_t* p_, size_t length, size_t& offset);
	int _parse_header(const uint8_t* p_, size_t length, size_t& offset);
	//0:整包 没问题，<0: error >0 数据不够
	int  _quickCheckForFullPackage(const uint8_t* p_, size_t length)const;
	int _parse_video(const uint8_t* p_, size_t length, size_t& offset);
	int _parse_audio(const uint8_t* p_, size_t length, size_t& offset);
	int _parse_pass_through(const uint8_t* p_, size_t length, size_t& offset);
	void dump()const;
public:
	uint32_t frame_mark;
	uint8_t v2 : 2;
	uint8_t p1 : 1;
	uint8_t x1 : 1;
	uint8_t cc4 : 4;
	uint8_t m1 : 1;
	uint8_t pt7 : 7;//6: g711a 7: g711u 26:adpcm 98: h264
	uint16_t package_sequence;
	uint8_t bcd_sim_car_number[6];
	uint8_t bt1_logicChannelNumber;
	uint8_t package_type : 4;//0：I 1：P 2：B 3：audio 4：passthrough other：不支持
	uint8_t subpackage_handle_mark : 4;

	AudioPackage* audio_pkg;
	VideoPackage* video_pkg;
	PassThroughPackage* pt_pkg;

};
class JT1078
{
public:
	JT1078();
	~JT1078();
	int parse_file(const char* file_name);
	int parse(const uint8_t* p_, size_t length, size_t& much);
	void dump()const;
private:
	vector<JT1078Package*>pkgs_;
	RtmpStream* rtmp_stream_;
	vector<pair<uint8_t*, size_t>> temp_data_;//分包临时数据

};


#endif //__JT1078__H