#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}
//
//#pragma comment(lib,"avformat.lib")
//#pragma comment(lib,"avcodec.lib")
//#pragma comment(lib,"avutil.lib")
//#pragma comment(lib,"swscale.lib")

using namespace std;

class FFmpeg_toV210 {
public:
	int init(int width, int height) {
		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_V210);
		if (!codec) {
			return -3;
		}
		m_yuv422p10le_v210_ctx = avcodec_alloc_context3(codec);
		if (!m_yuv422p10le_v210_ctx) {
			return -4;
		}

		/* resolution must be a multiple of two */
		m_yuv422p10le_v210_ctx->width = width;
		m_yuv422p10le_v210_ctx->height = height;
		/* frames per second */
		m_yuv422p10le_v210_ctx->time_base.num = 25;// Config->getFrameRateNum();
		m_yuv422p10le_v210_ctx->time_base.den = 1;// Config->getFrameRateDen();

		m_yuv422p10le_v210_ctx->framerate.num = 25;// Config->getFrameRateDen();
		m_yuv422p10le_v210_ctx->framerate.den = 1;// Config->getFrameRateNum();

		m_yuv422p10le_v210_ctx->pix_fmt = AV_PIX_FMT_YUV422P10LE;
		int ret = avcodec_open2(m_yuv422p10le_v210_ctx, codec, nullptr);
		if (ret < 0) {
			return -5;
		}
		m_yuv422p10le_v210_pkt = av_packet_alloc();
		return 0;
	}
	int convertToV210( AVFrame* frame, uint8_t* pFrameInfo)
	{
		frame->format = AV_PIX_FMT_YUV422P10LE;
		return convertToV210(m_yuv422p10le_v210_ctx, frame, pFrameInfo);
		//return convertToV210(m_yuv422p10le_v210_ctx, nullptr, pFrameInfo);
	}
private:
	int convertToV210(AVCodecContext* enc_ctx, AVFrame* frame, uint8_t* pFrameInfo)
	{
		int ret = avcodec_send_frame(enc_ctx, frame);
		if (ret < 0)
		{
			return ret;
		}
		//while (ret >= 0)
		{
			ret = avcodec_receive_packet(enc_ctx, m_yuv422p10le_v210_pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				return 0;
			if (ret < 0)
			{
				return ret;
			}
			memcpy(pFrameInfo, m_yuv422p10le_v210_pkt->data, m_yuv422p10le_v210_pkt->size);
			av_packet_unref(m_yuv422p10le_v210_pkt);
		}
		return 0;
	}
private:

	AVCodecContext* m_yuv422p10le_v210_ctx = nullptr;
	AVPacket* m_yuv422p10le_v210_pkt = nullptr;
};
class RGBA_to_JPEG
{
public:
	int init(int width, int height) {
		m_width = width;
		m_height = height;
		prepareBackGround(width, height);

		return 0;
	}
	void doConvert(uint8_t*pRGBA) {
		
		uint64_t* pRGBAEnd = (uint64_t*)(pRGBA + m_width * m_height * 4*2);
		uint64_t* pPosA = (uint64_t*)pRGBA;
		uint64_t* pDestBuffer = new  uint64_t[m_width * m_height];
		uint64_t* pDest = pDestBuffer;
		uint64_t* pBG = (uint64_t*)pBackGround;
		
		//while(pPos !=pRGBAEnd)
		for(int h=0;h<m_height;h++)
		{
			
			for (int w = 0; w < m_width; w+=2)
			{
				uint64_t* pPos = pPosA + h * m_width+w;

				int a = (*pPos >> 48) & 0xFFf0;
				*pDestBuffer = (a == 0xFFF0) ? *pPos : *pBG;
			
				pBG++;
				pDestBuffer++;
			}
		}

		{
			FILE* fp = fopen("d:\\tmp\\FFmpegrgb2222_background_1920x1080_.rgb", "wb");

			fwrite(pDest, m_width * m_height * 4 * 2, 1, fp);
			fclose(fp);
		}

	}
	void prepareBackGround(int width, int height) {
		int dstWidth = width;
		int dstHeight = height;
		int gridWidth = 16;
		int gridHeight = 16;

		int gridLine = 0;
		pBackGround = new uint8_t[width * height * 4*2];
		
		gridLine = 0;
		//FFFFFF ,D9D9D9
		memset(pBackGround, 0xFF, width * height * 4*2);
		for (int h = 0; h < height - gridHeight; h += gridHeight)
		{			
			for (int w = 0; w < dstWidth; w += gridWidth * 2)
			{
				for (int gridH = 0; gridH < gridHeight; gridH++)
				{
					unsigned char* y = pBackGround + (dstWidth * (h + gridH) + w + ((gridLine % 2 == 0) ? 0 : gridWidth)) * 4*2;
					memset(y, 0xd9, gridWidth * 4*2);
				}
			}
			++gridLine;
		}
		for (int w = 0; w < dstWidth; w += gridWidth * 2)
		{
			for (int gridH = 0; gridH < (dstHeight - (gridHeight * (dstHeight / gridHeight))); gridH++)
			{
				unsigned char* y = pBackGround + (dstWidth * (gridLine * gridHeight + gridH) + w + ((gridLine % 2 == 0) ? 0 : gridWidth)) * 4*2;
				memset(y, 0xd9, gridWidth * 4*2);
			}
		}
		{
			FILE* fp = fopen("d:\\tmp\\FFmpegrgb_background_1920x1080_.rgb", "wb");

			fwrite(pBackGround, width * height * 4*2, 1, fp);
			fclose(fp);

		}
	}

private:
	uint8_t* pBackGround = nullptr;
	int m_width;
	int m_height;
private:
};
class BGRA_to_yuv422pyuv422p {
public:
	~BGRA_to_yuv422pyuv422p()
	{
		sws_freeContext(swsCtx);
	}
	bool init(int width,int height,AVPixelFormat format) {
		av_register_all();
		m_width = width;
		m_height = height;
		m_destFormat = format;
		swsCtx = sws_getCachedContext(swsCtx,
			width, height, AV_PIX_FMT_BGRA64,
			width, height, format, //AV_PIX_FMT_YUVA444P16LE
			SWS_BICUBIC,
			NULL, NULL, NULL
		);
		rgbFrame = av_frame_alloc();
		yuvFrame = av_frame_alloc();
		yuvFrame->width = width;
		yuvFrame->height = height;
		yuvFrame->format = format;		
		return true;
	}

	void doConvert(uint8_t* pBGRA, uint8_t* pDestBuffer) {
		av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, pDestBuffer, 
			m_destFormat, m_width, m_height, 1);
		avpicture_fill((AVPicture*)rgbFrame, pBGRA,  AV_PIX_FMT_BGRA64, m_width, m_height);
		//6 Pixel format conversion, the converted YUV data is stored in yuvFrame
		int outSliceH = sws_scale(swsCtx, rgbFrame->data, rgbFrame->linesize, 0, m_height,
			yuvFrame->data, yuvFrame->linesize);

		auto lambdaC = [&]<typename T>(T  a) {
			std::cout << typeid(a).name() << std::endl;
			T pYAlpha = (T)a;
			T pUAlpha = pYAlpha + m_width * m_height;
			T pVAlpha = pUAlpha + m_width * m_height / 2;
			for (int h = 0; h < m_height; h++)
			{
				for (int w = 0; w < m_width; w += 2)
				{
					*pVAlpha++ = *pUAlpha++ = ((*pYAlpha++) + (*pYAlpha++)) / 2;
				}
			}
		};
		if(m_destFormat== AV_PIX_FMT_YUVA422P)
			lambdaC((uint8_t*)yuvFrame->data[3]);
		else		
			lambdaC((uint16_t*)yuvFrame->data[3]);
	}
private:
	
	AVFrame* yuvFrame;
	AVFrame* rgbFrame;
	SwsContext* swsCtx = NULL;
	int m_width;
	int m_height;
	AVPixelFormat m_destFormat;
};
#include "nameof.hpp"
#include "fmt/format.h"
#include <map>
class Worker {
public:Worker() {
	gmap[AV_PIX_FMT_YUVA422P] = "AV_PIX_FMT_YUVA422P";
	gmap[AV_PIX_FMT_YUVA422P16LE] = "AV_PIX_FMT_YUVA422P16LE";
	gmap[AV_PIX_FMT_YUVA422P10LE] = "AV_PIX_FMT_YUVA422P10LE";
}
	void doWork(int width, int height, AVPixelFormat format, uint8_t* rgbBuf) {
		
		auto outfile = fmt::format(R"(d:\tmp\aa_{}_{}_1920x1080_out-8bit.yuv)",(format), gmap[format]);
		fpout = fopen(outfile.data(), "wb");
		uint8_t* pDestBuffer = new uint8_t[width * height * 10 * 2];		
		m_worker.init(width,height,format);
		m_worker.doConvert(rgbBuf, pDestBuffer);
		fwrite(pDestBuffer, width * height * 4 * 2, 1, fpout);
	}
private:
	std::map<int, std::string> gmap;
	BGRA_to_yuv422pyuv422p m_worker;
	FILE* fpout10, * fpout;
};
int main()
{
	char infile[] = R"(..\..\..\1920x1080.vCFrame_sourceTgaAAAA.RGBA)";
	
	//AV_PIX_FMT_BGRA64
	// Source image parameters (the parameters must be set correctly, otherwise the conversion will be abnormal)
	int width = 1920;
	int height = 1080;

	//1 Open RGB and YUV files
	FILE* fpin = fopen(infile, "rb");
	if (!fpin)
	{
		cout << infile << "open infile failed!" << endl;
		getchar();
		return -1;
	}

	// Create RGB buffer and allocate memory at the same time
	unsigned char* rgbBuf = new unsigned char[width * height * 4*2];
	RGBA_to_JPEG m_RGBA_to_JPEG;
	m_RGBA_to_JPEG.init(width, height);
	Worker m_worker[3];
	// Write video files in loop
	for (;;)
	{
		//4 Read one frame of RGB data to rgbBuf at a time, and exit after reading
		int len = fread(rgbBuf, width * height * 4*2, 1, fpin);
		if (len <= 0)
		{
			break;
		}
		m_RGBA_to_JPEG.doConvert(rgbBuf);
		m_worker[0].doWork(width, height, AV_PIX_FMT_YUVA422P, rgbBuf);
		m_worker[1].doWork(width, height, AV_PIX_FMT_YUVA422P16LE, rgbBuf);
		m_worker[2].doWork(width, height, AV_PIX_FMT_YUVA422P10LE, rgbBuf);
		
		cout << ".";
	}

	cout << "\n======================end=========================" << endl;

	// Close RGB and YUV files
	fclose(fpin);

	// Release the RGB buffer
	delete rgbBuf;

	//Clean up the video resampling context
	

	

	return 0;
}