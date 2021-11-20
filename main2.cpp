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
class BGRA_to_yuv422pyuv422p {
public:
	~BGRA_to_yuv422pyuv422p()
	{
		sws_freeContext(swsCtx);
	}
	bool init(int width,int height,bool is8bits) {
		av_register_all();
		m_width = width;
		m_height = height;
		m_is8bits = is8bits;
		swsCtx = sws_getCachedContext(swsCtx,
			width, height, AV_PIX_FMT_BGRA64,
			width, height, m_is8bits ?AV_PIX_FMT_YUVA422P: AV_PIX_FMT_YUVA422P16LE, //AV_PIX_FMT_YUVA444P16LE
			SWS_BICUBIC,
			NULL, NULL, NULL
		);
		rgbFrame = av_frame_alloc();
		yuvFrame = av_frame_alloc();
		return true;
	}

	void doConvert(uint8_t* pBGRA, uint8_t* pDestBuffer) {
		av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, pDestBuffer, m_is8bits ? AV_PIX_FMT_YUVA422P : AV_PIX_FMT_YUVA422P16LE, m_width, m_height, 1);		
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
		if(m_is8bits)
			lambdaC((uint8_t*)yuvFrame->data[3]);
		else
			lambdaC((uint16_t*)yuvFrame->data[3]);
		

	}
private:
	bool m_is8bits = false;
	AVFrame* yuvFrame;
	AVFrame* rgbFrame;
	SwsContext* swsCtx = NULL;
	int m_width;
	int m_height;
};
int main()
{
	char infile[] = R"(D:\tmp\1920x1080.vCFrame_sourceTgaAAAA.RGBA)";
	char outfile16[] = R"(d:\tmp\1920x1080-out-16bit.yuv)";
	char outfile8[] = R"(d:\tmp\1920x1080-out-8bit.yuv)";
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

	FILE* fpout8 = fopen(outfile8, "wb");
	FILE* fpout16 = fopen(outfile16, "wb");
	if (!fpout8)
	{
		cout << infile << "open outfile failed!" << endl;
		getchar();
		return -1;
	}
	// Create RGB buffer and allocate memory at the same time
	unsigned char* rgbBuf = new unsigned char[width * height * 4*2];


	BGRA_to_yuv422pyuv422p m_convert8;
	m_convert8.init(width, height, true);
	BGRA_to_yuv422pyuv422p m_convert16;
	m_convert16.init(width, height, false);


	//3 Create YUV video frame and configure

	/*yuvFrame->format = AV_PIX_FMT_YUVA444P16LE;
	yuvFrame->width = width;
	yuvFrame->height = height;
	av_frame_get_buffer(yuvFrame, 3 * 16);*/
	uint8_t* pDestBuffer = new uint8_t[width * height * 10 * 2];
	

	// Write video files in loop
	for (;;)
	{
		//4 Read one frame of RGB data to rgbBuf at a time, and exit after reading
		int len = fread(rgbBuf, width * height * 4*2, 1, fpin);
		if (len <= 0)
		{
			break;
		}

		//5 Create RGB video frame and bind RGB buffer (avpicture_fill initializes some fields for rgbFrame, and will automatically fill in data and linesize)
		m_convert8.doConvert(rgbBuf, pDestBuffer);

		fwrite(pDestBuffer, width * height * 4 , 1, fpout8);
		m_convert16.doConvert(rgbBuf, pDestBuffer);
		fwrite(pDestBuffer, width * height * 4 * 2, 1, fpout16);
		//7 Write the data of each component of YUV to the file
		/*fwrite(yuvFrame->data[0], width * height*2, 1, fpout);
		fwrite(yuvFrame->data[1], width * height , 1, fpout);
		fwrite(yuvFrame->data[2], width * height , 1, fpout);*/

		cout << ".";
	}

	cout << "\n======================end=========================" << endl;

	// Close RGB and YUV files
	fclose(fpin);
	fclose(fpout8);
	fclose(fpout16);

	// Release the RGB buffer
	delete rgbBuf;

	//Clean up the video resampling context
	

	

	return 0;
}