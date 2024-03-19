#ifndef __RTMPSTREAM__H
#define __RTMPSTREAM__H
#include"../JT1078/JT1078.h"
#include"../Codec/codec.h"


#define rtmp_url "/home/xingxing/video/test.flv"  //rtmp://localhost:1935/live/stream
extern "C"
{
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<libswresample/swresample.h>
#include <libavutil/opt.h>
}

class  AudioDeal {
public:
	AudioDeal(AVFormatContext* output_fmt_ctx_);
	~AudioDeal();
	int dealJt1078AudioPacket(JT1078Package* pkg);
	void encode(AVCodecContext* ctx, AVFrame* frame_, AVPacket* pkt_,
		AVFormatContext* output);
private:
	int __init(JT1078Package* pkg);
	int __init_aac_encoder_and_add_aac_stream();
private:
	Codec* codec_;//��Ƶ�������
	SwrContext* swr_ctx;//s16->fltp
	AVCodecContext* enc_ctx;//pcm->aac

	AVPacket* pkt_;
	AVFrame* frame_;
	int stream_index_;

	AVFormatContext* output_fmt_ctx_;
	uint8_t** p_;//������Ƶ��������Ƶ���� �������ز���

	bool has_audio_ = false;
	size_t data_size_ = 0;//p_[0]�����������
	int64_t pts = 0;

};
class RtmpStream {
public:
	RtmpStream();
	~RtmpStream();
	int dealJT1078Packet(JT1078Package* pkg);
	int _dealJt1078H264Packet(JT1078Package* pkg);
	int _init_add_h264_stream();
private:
	AudioDeal* audio_deal_;//����aac
	AVPacket* pkt_;//����h264ʹ��
	AVFrame* frame_;//����h264ʹ��
	int stream_index_;//����h264ʹ��
	uint64_t first_pts_;//����h264ʹ��
	AVFormatContext* output_fmt_ctx_;

	bool get_first_pts_ = false;
};
#endif //__RTMPSTREAM__H