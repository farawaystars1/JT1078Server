#include"codec.h"

Codec::Codec(uint8_t* src_data, int src_size, AudioCodecType type) :g726_st_(NULL), codec_type_(type) {
	_init(src_data, src_size, type);
}

Codec::~Codec() { if (g726_st_)delete g726_st_; }
//@return pcm样本数 n*2个字节
int Codec::decode(const uint8_t* src_data, int src_size, short* des_data) const {
	return dec_(src_data, src_size, des_data);
}

int Codec::encode(const short* src_data, int src_size, uint8_t* des_data) const
{
	return enc_(src_data, src_size, des_data);
}

int Codec::_g726_init(uint8_t* src_data, int src_size, AudioCodecType type)
{
	g726_st_ = new g726_state_t;
	switch (src_size)
	{
	case 320 * 2 / 8:g726_init(g726_st_, 2 * 8000); break;
	case 320 * 3 / 8:g726_init(g726_st_, 3 * 8000); break;
	case 320 * 4 / 8:g726_init(g726_st_, 4 * 8000); break;
	case 320 * 5 / 8:g726_init(g726_st_, 5 * 8000); break;
	default:
		delete g726_st_;
		g726_st_ = NULL;
		cerr << "unsupported size,g726 init failed" << endl;
		return -1;
	}
	return 0;
}

int Codec::_g726_decode(const uint8_t* src_data, int src_size, short* des_data) const
{
	return g726_decode(g726_st_, des_data, src_data, src_size);
}

int Codec::_g726_encode(const short* src_data, int src_size, uint8_t* des_data) const
{
	return g726_encode(g726_st_, des_data, src_data, src_size);
}

int Codec::_g711a_decode(const uint8_t* src_data, int src_size, short* des_data) const
{
	return g711a_decode(des_data, src_data, src_size);
}

int Codec::_g711u_decode(const uint8_t* src_data, int src_size, short* des_data) const
{
	return g711u_decode(des_data, src_data, src_size);
}

int Codec::_g711a_encode(const short* src_data, int src_size, uint8_t* des_data) const
{
	return g711a_encode(des_data, src_data, src_size);
}

int Codec::_g711u_encode(const short* src_data, int src_size, uint8_t* des_data) const
{
	return g711u_encode(des_data, src_data, src_size);
}

int Codec::_adpcm_decode(const uint8_t* src_data, int src_size, short* des_data) const
{
	if (src_size < 8)return -1;
	src_data += 4;
	src_size -= 4;//跳过海斯头
	static adpcm_state_t state = { 0 };
	state.valprev = *reinterpret_cast<const short*>(src_data); 
		state.index = src_data[2];
	src_data += 4;
	src_size -= 4;//跳过初始化头
	adpcm_decoder((char*)src_data, des_data, src_size*2, &state);
	return src_size * 2;
}

int Codec::_adpcm_encode(const short* src_data, int src_size, uint8_t* des_data) const
{
	//未使用海斯头
	int ret = 0;
	static adpcm_state_t state = { 0 };
	state.valprev = *src_data;
	state.index = 0;
	memcpy(des_data, &state, sizeof(adpcm_state_t));
	des_data += 4; ret += 4;
	adpcm_coder(const_cast<short*>(src_data), reinterpret_cast<char*>(des_data), src_size/2, &state);
	return ret + src_size / 2;
}

int Codec::_init(uint8_t* src_data, size_t src_size, AudioCodecType type) {

	switch (type)
	{
	case ADPCM:
		enc_ = bind(&Codec::_adpcm_encode,this,placeholders::_1,placeholders::_2,placeholders::_3); dec_ = bind(&Codec::_adpcm_decode, this, placeholders::_1, placeholders::_2, placeholders::_3) ; break;
	case G711_ALAW:
		enc_=bind(&Codec::_g711a_encode, this, placeholders::_1, placeholders::_2, placeholders::_3); dec_ = bind(&Codec::_g711a_decode, this, placeholders::_1, placeholders::_2, placeholders::_3); break;
	case G711_ULAW:
		enc_=bind(&Codec::_g711u_encode, this, placeholders::_1, placeholders::_2, placeholders::_3); dec_ = bind(&Codec::_g711u_decode, this, placeholders::_1, placeholders::_2, placeholders::_3); break;
	case G726:if (_g726_init(src_data, src_size, type) < 0)return -1; 
		enc_=bind(&Codec::_g726_encode, this, placeholders::_1, placeholders::_2, placeholders::_3); dec_ = bind(&Codec::_g726_decode, this, placeholders::_1, placeholders::_2, placeholders::_3); break;
	default:
		cerr << "unsupported audio codec type,audio codec init failed!" << endl;
		return -1;
	}
	return 0;
}