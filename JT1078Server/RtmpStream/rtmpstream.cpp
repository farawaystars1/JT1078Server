#include"rtmpstream.h"

AudioDeal::AudioDeal(AVFormatContext* output_fmt_ctx_) :codec_(NULL), swr_ctx(NULL), enc_ctx(NULL), p_(NULL), output_fmt_ctx_(output_fmt_ctx_) {
	pkt_ = av_packet_alloc();
	frame_ = av_frame_alloc();
	stream_index_ = -1;
	int linesize;
	int ret = av_samples_alloc_array_and_samples((uint8_t***)&p_, &linesize, 1, 1024+1024, AV_SAMPLE_FMT_S16, 0);
	if (ret < 0) { cerr << "sample alloc array error!"; }
	//初始化 enc_ctx 对output_fmt_ctx添加音频流
	__init_aac_encoder_and_add_aac_stream();
}

AudioDeal::~AudioDeal()
{
	encode(enc_ctx, NULL, pkt_, output_fmt_ctx_);//清空残留的音频帧
	if (codec_) { delete codec_; }
	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	if (enc_ctx)
	{
		avcodec_free_context(&enc_ctx);
	}
	if (pkt_)av_packet_free(&pkt_);
	if (frame_)av_frame_free(&frame_);
	if (p_[0]) {
		av_freep(&p_[0]);
	}
	if (p_) { av_freep(&p_); }

}

int AudioDeal::dealJt1078AudioPacket(JT1078Package* pkg)
{
	if (has_audio_ == false)
	{
		if (__init(pkg) < 0)return -1;
		has_audio_ = true;
	}
	//音频解码
	int num = codec_->decode(pkg->audio_pkg->data, pkg->audio_pkg->size, reinterpret_cast<short*>(p_[0] + data_size_));
	data_size_ += num * 2;
	if (data_size_ < 2048) return 1;
	//音频样本格式转换pcm->fltp
	memcpy(p_[0], p_[0], 2048);
	memcpy(p_[0], p_[0] + 2048, data_size_ - 2048);
	data_size_ -= 2048;

	int num__ = frame_->nb_samples * frame_->ch_layout.nb_channels * av_get_bytes_per_sample((AVSampleFormat)frame_->format);//4*1024
	int ret = swr_convert(swr_ctx, frame_->data, num__/*frame->nb_samples * frame->ch_layout.nb_channels * av_get_bytes_per_sample((AVSampleFormat)frame->format)*/, (const uint8_t**)p_, 1024);

	//encode
	encode(enc_ctx, frame_, pkt_, output_fmt_ctx_);
}

void AudioDeal::encode(AVCodecContext* ctx, AVFrame* frame_, AVPacket* pkt_, AVFormatContext* output)
{
	int ret;

	/* send the frame for encoding */
	ret = avcodec_send_frame(ctx, frame_);
	if (ret < 0) {
		fprintf(stderr, "Error sending the frame to the encoder\n");
		exit(1);
	}

	/* read all the available output packets (in general there may be any
	* number of them */
	while (ret >= 0) {
		ret = avcodec_receive_packet(ctx, pkt_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding audio frame\n");
			exit(1);
		}

		pkt_->pts = pts;
		av_packet_rescale_ts(pkt_, ctx->time_base, output->streams[pkt_->stream_index]->time_base);
		cout << "audio_pts: "<<pts <<endl;
		pkt_->stream_index = this->stream_index_;
		ret = av_interleaved_write_frame(output, pkt_); if (ret < 0)cout << "write frame error!" << endl;
		pts += 1024;
		av_packet_unref(pkt_);
	}
}

int AudioDeal::__init(JT1078Package* pkg)
{
	int ret = 0;

	//初始化音频解码器
	if (codec_ == NULL)
	{
		codec_ = new Codec(pkg->audio_pkg->data, pkg->audio_pkg->size, (AudioCodecType)pkg->pt7);
	}
	//初始化swr_ctx
	swr_ctx = swr_alloc();
	av_opt_set_chlayout(swr_ctx, "in_chlayout", &enc_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", enc_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);

	av_opt_set_chlayout(swr_ctx, "out_chlayout", &enc_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", enc_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);
	if (swr_init(swr_ctx) < 0)
		return -1;
	//初始化frame
	av_channel_layout_copy(&frame_->ch_layout, &enc_ctx->ch_layout);
	frame_->format = AV_SAMPLE_FMT_FLTP;
	frame_->nb_samples = 1024;
	av_frame_get_buffer(frame_, 0);



	return 0;
}

int AudioDeal::__init_aac_encoder_and_add_aac_stream()
{
	//初始化  enc_ctx
	const AVCodec* av_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (av_codec == NULL)
		return -1;
	enc_ctx = avcodec_alloc_context3(av_codec);
	av_channel_layout_default(&enc_ctx->ch_layout, 1);
	enc_ctx->sample_rate = 8000;
	enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	enc_ctx->bit_rate = 32000;
	if (enc_ctx == NULL)return -1;
	if (avcodec_open2(enc_ctx, av_codec, NULL) < 0)
		return -1;

	//向output_fmt_context中添加音频流
	AVStream* stream = avformat_new_stream(output_fmt_ctx_, NULL);
	if (stream == NULL) { cerr << "alloc new stream error!"; return -1; }
	int ret = avcodec_parameters_from_context(stream->codecpar, enc_ctx); if (ret < 0)return -1;
	this->stream_index_ = stream->index;

	if (!(output_fmt_ctx_->flags & AVFMT_NOFILE)) {
		ret = avio_open(&output_fmt_ctx_->pb, rtmp_url, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", rtmp_url);
			return ret;
		}
	}
	ret = avformat_write_header(output_fmt_ctx_, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		return ret;
	}
}

RtmpStream::RtmpStream() {
	pkt_ = av_packet_alloc();
	frame_ = av_frame_alloc();
	if (avformat_alloc_output_context2(&output_fmt_ctx_, NULL, "flv", rtmp_url) < 0) { cerr << "alloc output context2 error!"; }
	_init_add_h264_stream();
	audio_deal_ = new AudioDeal(output_fmt_ctx_);

}

RtmpStream::~RtmpStream() {
	av_write_trailer(output_fmt_ctx_);
	if (audio_deal_)delete audio_deal_;
	if (output_fmt_ctx_ && !(output_fmt_ctx_->flags & AVFMT_NOFILE))
		avio_closep(&output_fmt_ctx_->pb);
	if (output_fmt_ctx_)avformat_free_context(output_fmt_ctx_);
	if (pkt_)av_packet_free(&pkt_);
	if (frame_)av_frame_free(&frame_);
}

int RtmpStream::dealJT1078Packet(JT1078Package* pkg)
{
	switch (pkg->package_type)
	{
	case 0:
	case 1:
	case 2:_dealJt1078H264Packet(pkg);
		break;
	case 3:audio_deal_->dealJt1078AudioPacket(pkg);
		break;
	case 4://透传
		//delete pkg;
		break;
	default:
		//delete pkg;
		cerr << "unknown pkg!" << endl;
		return -1;
	}
	return 0;
}

int RtmpStream::_dealJt1078H264Packet(JT1078Package* pkg)
{
	int ret = 0;
	if (get_first_pts_ == false)
	{
		first_pts_ = pkg->video_pkg->bt8_timestamp;
		get_first_pts_ = true;
	}
	pkt_->data = pkg->video_pkg->data;
	pkt_->size = pkg->video_pkg->size;
	pkt_->pts = pkg->video_pkg->bt8_timestamp - first_pts_;
	cout << "video pts: " << pkt_->pts<<endl;
	pkt_->stream_index = this->stream_index_;
	ret = av_interleaved_write_frame(output_fmt_ctx_, pkt_);
	if (ret < 0) { cerr << "write frame error!"; /*delete pkg;*/ return ret; }

	//delete pkg;
	return 0;
}

int RtmpStream::_init_add_h264_stream() {
	int ret = 0;
	//添加h264视频流
	AVStream* stream = avformat_new_stream(output_fmt_ctx_, NULL);
	if (stream == NULL) { cerr << "alloc new stream error!"; return -1; }
	stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	stream->codecpar->codec_id = AV_CODEC_ID_H264;
	stream->codecpar->width = 800;
	stream->codecpar->height = 800;
	stream_index_ = stream->index;


	if (!(output_fmt_ctx_->flags & AVFMT_NOFILE)) {
		ret = avio_open(&output_fmt_ctx_->pb, rtmp_url, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", rtmp_url);
			return ret;
		}
	}
	return ret;
}
