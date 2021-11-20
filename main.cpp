
class MyFFmpeg
{
    public:
    AVFrame *m_pFrameRGB, *m_pFrameYUV;
    uint8_t *m_rgbBuffer, *m_yuvBuffer;
    struct SwsContext *m_img_convert_ctx;

    void init（） //Assign two frames, two buffs, and one conversion context
    {
        　 //Allocate memory for each frame of image
            m_pFrameYUV = av_frame_alloc();
        m_pFrameRGB = av_frame_alloc(); //  width and heigt are the size of the incoming resolution, when the resolution changes, the maximum standard can be applied
        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, nwidth, nheight);
        m_rgbBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
        int yuvSize = nwidth * nheight * 3 / 2;
        m_yuvBuffer = (uint8_t *)av_malloc(yuvSize);
        //Pay special attention to the memory leak of sws_getContext,
        //Note that sws_getContext can only be called once, and it can be called during initialization. After the call, sws_freeContext is used in the destructor to release its memory.
        //Set image conversion context
        m_img_convert_ctx = sws_getContext(nwidth, nheight, AV_PIX_FMT_YUV420P, \
　　　　　　　　　　　　　　　　　　　　　　　 nwidth, nheight, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    }

    void play（char *pbuff_in, int nwidth, int nheight）
    {
        avpicture_fill((AVPicture *)m_pFrameRGB, m_rgbBuffer, AV_PIX_FMT_RGB32, nwidth, nheight);
        avpicture_fill((AVPicture *)m_pFrameYUV, (uint8_t *)pbuff_in, AV_PIX_FMT_YUV420P, nwidth, nheight);
        //Convert image format, convert the decompressed YUV420P image to RGB image
        sws_scale(m_img_convert_ctx,
                  (uint8_t const *const *)m_pFrameYUV->data,
                  m_pFrameYUV->linesize, 0, nheight, m_pFrameRGB->data,
                  m_pFrameRGB->linesize);
        //Load this RGB data with QImage
        QImage tmpImg((uchar *)m_rgbBuffer, nwidth, nheight, QImage::Format_RGB32);
        //Copy the image and pass it to the interface display
        m_mapImage[nWindowIndex] = tmpImg.copy();
    }

    void release()
    {
        av_frame_free(&m_pFrameYUV);
        av_frame_free(&m_pFrameRGB);
        av_free(m_rgbBuffer);
        av_free(m_yuvBuffer);
        sws_freeContext(m_img_convert_ctx);
    }

    bool YV12ToBGR24_FFmpeg(unsigned char *pYUV, unsigned char *pBGR24, int width, int height)
    {
        if (width < 1 || height < 1 || pYUV == NULL || pBGR24 == NULL)
            return false;
        //int srcNumBytes,dstNumBytes;
        //uint8_t *pSrc,*pDst;
        AVPicture pFrameYUV, pFrameBGR;

        //pFrameYUV = avpicture_alloc();
        //srcNumBytes = avpicture_get_size(PIX_FMT_YUV420P,width,height);
        //pSrc = (uint8_t *)malloc(sizeof(uint8_t) * srcNumBytes);
        avpicture_fill(&pFrameYUV, pYUV, PIX_FMT_YUV420P, width, height);

        //U, V interchange
        uint8_t *ptmp = pFrameYUV.data[1];
        pFrameYUV.data[1] = pFrameYUV.data[2];
        pFrameYUV.data[2] = ptmp;

        //pFrameBGR = avcodec_alloc_frame();
        //dstNumBytes = avpicture_get_size(PIX_FMT_BGR24,width,height);
        //pDst = (uint8_t *)malloc(sizeof(uint8_t) * dstNumBytes);
        avpicture_fill(&pFrameBGR, pBGR24, PIX_FMT_BGR24, width, height);

        struct SwsContext *imgCtx = NULL;
        imgCtx = sws_getContext(width, height, PIX_FMT_YUV420P, width, height, PIX_FMT_BGR24, SWS_BILINEAR, 0, 0, 0);

        if (imgCtx != NULL)
        {
            sws_scale(imgCtx, pFrameYUV.data, pFrameYUV.linesize, 0, height, pFrameBGR.data, pFrameBGR.linesize);
            if (imgCtx)
            {
                sws_freeContext(imgCtx);
                imgCtx = NULL;
            }
            return true;
        }
        else
        {
            sws_freeContext(imgCtx);
            imgCtx = NULL;
            return false;
        }
    }
};