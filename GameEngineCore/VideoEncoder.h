#ifndef GAME_ENGINE_VIDEO_ENCODER_H
#define GAME_ENGINE_VIDEO_ENCODER_H

#include "CoreLib/LibIO.h"

namespace GameEngine
{
    class VideoEncodingOptions
    {
    public:
        int Width = 1920, Height = 1080;
        int Bitrate = 20*1024*1024;
        int FramesPerSecond = 30;
        VideoEncodingOptions() = default;
        VideoEncodingOptions(int w, int h)
        {
            Width = w;
            Height = h;
        }
    };

    class IVideoEncoder : public CoreLib::RefObject
    {
    public:
        virtual void Init(VideoEncodingOptions options, CoreLib::IO::Stream * outputStream) = 0;
        virtual void EncodeFrame(int w, int h, unsigned char * rgbaImage) = 0;
        virtual void Close() = 0;
    };

    // Encode video using H264 codec and wrap into a mp4 file
    IVideoEncoder * CreateH264VideoEncoder();
}

#endif