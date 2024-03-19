#include"JT1078.h"

#include"../RtmpStream/rtmpstream.h"



static uint16_t show16(const uint8_t* data) {
	return ((uint16_t)data[0] << 8) | data[1];
}
static uint32_t show24(const uint8_t* data) {
	return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}
static uint32_t show32(const uint8_t* data) {
	return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
}
static uint64_t show64(const uint8_t* data) {
	return ((uint64_t)data[0] << 56) | ((uint64_t)data[1] << 48) | ((uint64_t)data[2] << 42) | ((uint64_t)data[3] << 36) | ((uint64_t)data[4] << 24) | ((uint64_t)data[5] << 16) | ((uint64_t)data[6] << 8) | data[7];
}

static constexpr size_t JT1078_MAX_PACKAGE_SIZE = 950;

AudioPackage::AudioPackage() { memset(this, 0, sizeof(*this)); }

AudioPackage::~AudioPackage() {
	if (data) { delete[]data; data = NULL; }
}

VideoPackage::VideoPackage() { memset(this, 0, sizeof(*this)); }

VideoPackage::~VideoPackage()
{
	if (data) { delete[]data; data = NULL; }
}

PassThroughPackage::PassThroughPackage() { memset(this, 0, sizeof(*this)); }

PassThroughPackage::~PassThroughPackage() {
	if (data) { delete[]data; data = NULL; }
}

JT1078Package::JT1078Package() { memset(this, 0, sizeof(JT1078Package)); }

JT1078Package::~JT1078Package() {
	if (audio_pkg) { delete audio_pkg; audio_pkg = NULL; }
	if (video_pkg){ delete video_pkg; video_pkg = NULL; }
	if (pt_pkg){ delete pt_pkg; pt_pkg = NULL; }
}

int JT1078Package::parse(const uint8_t* p_, size_t length, size_t& offset)
{
	int ret = _quickCheckForFullPackage(p_, length);//快速检查是否够完整包
	if (ret != 0)return ret;
	frame_mark = show32(p_); p_ += 4; length -= 4;
	v2 = (p_[0] >> 6) & 0x03;
	p1 = (p_[0] >> 5) & 0x01;
	x1 = (p_[0] >> 4) & 0x01;
	cc4 = p_[0] & 0x0f;
	m1 = (p_[1] >> 7) & 0x01;
	pt7 = p_[1] & 0x7f; p_ += 2; length -= 2;
	package_sequence = show16(p_); p_ += 2; length -= 2;
	memcpy(bcd_sim_car_number, p_, sizeof(bcd_sim_car_number)); p_ += 6; length -= 6;
	bt1_logicChannelNumber = p_[0];
	package_type = (p_[1] >> 4) & 0x0f;
	subpackage_handle_mark = p_[1] & 0x0f; p_ += 2; length -= 2;
	offset += 16;
	switch (package_type)
	{
	case 0://I Frame 
	case 1://P Frame
	case 2://B Frame
		return _parse_video(p_, length, offset);
	case 3://Audio
		return _parse_audio(p_, length, offset);
	case 4:// PasThrough
		return _parse_pass_through(p_, length, offset);
	default:
		cerr << "unsupported package_type!" << endl;
		return -1;
	}
}

int JT1078Package::_parse_header(const uint8_t* p_, size_t length, size_t& offset)
{

}

//0:整包 没问题，<0: error >0 数据不够

int JT1078Package::_quickCheckForFullPackage(const uint8_t* p_, size_t length)const	//判断是否有一个JT1078包大小 否则数据不足 返回1
{
	if (length < 16) { return 1; }
	uint8_t package_type = (p_[14] >> 4) & 0x0f;
	p_ += 16; length -= 16;
	switch (package_type)
	{
	case 0://I Frame 
	case 1://P Frame
	case 2://B Frame
		p_ += 14; length -= 14; break;//跳过视频头
	case 3://Audio
		p_ += 10; length -= 10; break;//跳过音频头
	case 4:// PasThrough
		p_ += 2; length -= 2; break;
	default:
		cerr << "unsupported package_type!" << endl;
		return -1;
	}
	size_t data_size_ = show16(p_ - 2);
	if (length < data_size_)return 1;
	return 0;
}

int JT1078Package::_parse_video(const uint8_t* p_, size_t length, size_t& offset)
{
	//if (length < 14)return 1;
	video_pkg = new VideoPackage;
	video_pkg->bt8_timestamp = show64(p_);
	video_pkg->last_IFrame_interval = show16(p_ + 8);
	video_pkg->last_frame_interval = show16(p_ + 10);
	video_pkg->size = show16(p_ + 12);
	p_ += 14; length -= 14;
	//if (length < video_pkg->size) { delete video_pkg; video_pkg = NULL; return 1; }
	video_pkg->data= new uint8_t[video_pkg->size];
	if (video_pkg->data == NULL) { cerr << "error!" << endl; return -1; }
	memcpy(video_pkg->data, p_, video_pkg->size);
	offset += 14 + video_pkg->size;
	return 0;
}

int JT1078Package::_parse_audio(const uint8_t* p_, size_t length, size_t& offset)
{
	//if (length < 10)return 1;
	audio_pkg = new AudioPackage;
	audio_pkg->bt8_timestamp = show64(p_);
	audio_pkg->size = show16(p_ + 8);
	p_ += 10; length -= 10;
	//if (length < audio_pkg->size) { delete audio_pkg; audio_pkg = NULL; return 1; }
	audio_pkg->data = new uint8_t[audio_pkg->size]{0};
	memcpy(audio_pkg->data, p_, audio_pkg->size);
	offset += 10 + audio_pkg->size;
	return 0;
}

int JT1078Package::_parse_pass_through(const uint8_t* p_, size_t length, size_t& offset)
{
	//if (length < 2)return 1;
	pt_pkg->size = show16(p_);
	p_ += 2; length -= 2;
	//if (length < pt_pkg->size) { delete pt_pkg; return 1; }
	pt_pkg->data = new uint8_t[pt_pkg->size];
	memcpy(pt_pkg->data, p_, pt_pkg->size);
	offset += 2 + pt_pkg->size;
	return 0;
}


void JT1078Package::dump()const
{
	cout << "帧标记: " << frame_mark << ",包序列号： " << (int)package_sequence << ",包类型: " << (int)package_type << "分包标记：" << (int)subpackage_handle_mark;
	if (audio_pkg) { cout << ",音频包大小" << (int)audio_pkg->size; }
	else if (video_pkg) { cout << ",视频包大小" << (int)video_pkg->size; }
	else if (pt_pkg)cout << "，透传包大小" << (int)pt_pkg->size;
	cout << endl;
}

JT1078::JT1078() { rtmp_stream_ = new RtmpStream; }

JT1078::~JT1078() {
	for (JT1078Package* pkg : pkgs_)
	{
		delete pkg; pkg = NULL;
	}
	if (rtmp_stream_) { delete rtmp_stream_; rtmp_stream_ = NULL; }

	for (const auto& v : temp_data_)
	{
		delete[]v.first;
	}
}

int JT1078::parse_file(const char* file_name)
{
	ifstream in(file_name, ios::binary);
	char buf[1024];
	string str;
	size_t offset = 0;
	while (in.good())
	{
		in.read(buf, sizeof(buf));
		str.append(buf, in.gcount());
	}
	in.close();
	const  uint8_t* p_ = reinterpret_cast<const uint8_t*>(str.data());
	while (true)
	{
		JT1078Package* pkg = new JT1078Package;
		int ret = pkg->parse(p_ + offset, str.size() - offset, offset);
		if (ret != 0) { delete pkg; pkg = NULL; return ret; }
		if (pkg->subpackage_handle_mark == 0 || pkg->subpackage_handle_mark == 2)
			pkgs_.push_back(pkg);

	}
	return 0;
}

int JT1078::parse(const uint8_t* p_, size_t length, size_t& much)
{
	much = 0;
	int ret = 0;
	JT1078Package* pkg=NULL;
	while (true)
	{
		pkg = new JT1078Package;
		int ret = pkg->parse(p_ + much, length - much, much);
		if (ret != 0) { delete pkg; pkg = NULL; return ret; }
		//合并分包
		if (pkg->subpackage_handle_mark == 1 || pkg->subpackage_handle_mark == 3|| pkg->subpackage_handle_mark == 2)//第一个分包 或中间分包
		{
			if (pkg->video_pkg)
			{
				temp_data_.push_back(make_pair(pkg->video_pkg->data, pkg->video_pkg->size));
				pkg->video_pkg->data = NULL;
				pkg->video_pkg->size = 0;
				if (pkg->subpackage_handle_mark == 2)
				{
					for (const auto& v : temp_data_)//计算完整包长度
					{
						//data: v.first
						//size v.second;
						pkg->video_pkg->size += v.second;
					}
					pkg->video_pkg->data = new (std::nothrow)uint8_t[pkg->video_pkg->size];
					if (pkg->video_pkg->data == NULL) { cerr << "alloc memory error!"; delete pkg; pkg = NULL; return -1; }
					int offset = 0;
					for (const auto& v : temp_data_)
					{
						memcpy(pkg->video_pkg->data + offset, v.first, v.second);
						offset += v.second;
						delete[]v.first;
					}
					temp_data_.clear();
				}
				else {
					delete pkg;
					pkg = NULL;
					continue;
				}
			}
		}

		if (pkg->subpackage_handle_mark == 0 || pkg->subpackage_handle_mark == 2)//原子包 或最后一个分包(此时分包已合并）
		{
			//pkgs_.push_back(pkg);
			int ret = rtmp_stream_->dealJT1078Packet(pkg); 
			delete pkg;
			if (ret < 0)break;
		}
	}
	return ret;
}

void JT1078::dump() const
{

}
