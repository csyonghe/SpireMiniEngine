#include "VideoEncoder.h"
#include "H264Encoder/src/codec_api.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
    const uint32_t unitsPerSecond = 1200000;
    uint32_t ToBigEndian32(uint32_t val)
    {
        auto b0 = val & 255u;
        auto b1 = (val >> 8) & 255u;
        auto b2 = (val >> 16) & 255u;
        auto b3 = (val >> 24) & 255u;
        return b3 + (b2 << 8) + (b1 << 16) + (b0 << 24);
    }
    uint16_t ToBigEndian16(uint16_t val)
    {
        auto b0 = val & 255u;
        auto b1 = (val >> 8) & 255u;
        return (uint16_t)((b1) + (b0 << 8));
    }

#pragma pack(push, 1)
    struct decimal32
    {
        uint32_t val;
        decimal32() = default;
        decimal32(uint16_t i, uint16_t frac)
        {
            set(i, frac);
        }
        void set(uint16_t i, uint16_t frac)
        {
            val = ToBigEndian16(i) + (ToBigEndian16(frac) << 16);
        }
        void set(float v)
        {
            set((uint16_t)v, (uint16_t)Math::Clamp((v - (int)v) * 65535.0f, 0.0f, 65535.0f));
        }
    };
    struct decimal16
    {
        uint16_t val;
        decimal16() = default;
        decimal16(unsigned char i, unsigned char frac)
        {
            set(i, frac);
        }
        void set(unsigned char i, unsigned char frac)
        {
            val = i + (frac << 8);
        }
        void set(float v)
        {
            set((unsigned char)v, (unsigned char)Math::Clamp((v - (int)v) * 255.0f, 0.0f, 255.0f));
        }
    };
    struct moovBox
    {
        uint32_t size = ToBigEndian32(sizeof(moovBox));
        char id[4] = { 'm', 'o', 'o', 'v' };
        void updateSize()
        {
            auto rs = 8 + ToBigEndian32(mvhd.size) + track.updateSize();
            size = ToBigEndian32(rs);
        }
        void Write(Stream * s)
        {
            updateSize();
            s->Write(this, 8);
            this->mvhd.Write(s);
            this->track.Write(s);
        }
        struct mvhdBox
        {
            uint32_t size = ToBigEndian32(sizeof(mvhdBox));
            char id[4] = { 'm', 'v', 'h', 'd' };
            uint32_t version_flags = 0;
            uint32_t createDate = 0, modifiedDate = 0;
            uint32_t timeScale = ToBigEndian32(1000); // time units per second
            uint32_t duration = 0;  // set this!
            decimal32 speed = decimal32(1, 0);
            decimal16 volume = decimal16(1, 0);
            char reserved[10] = { 0,0,0,0,0,0,0,0,0,0 };
            decimal32 A = decimal32(1, 0);
            decimal32 B = decimal32(0, 0);
            decimal32 U = decimal32(0, 0);
            decimal32 C = decimal32(0, 0);
            decimal32 D = decimal32(1, 0);
            decimal32 V = decimal32(0, 0);
            decimal32 X = decimal32(0, 0);
            decimal32 Y = decimal32(0, 0);
            decimal32 W = decimal32(0x4000, 0);
            uint32_t quicktime_Preview[2] = { 0, 0 };
            uint32_t quicktime_1 = 0;
            uint32_t quicktime_2[2] = { 0, 0 };
            uint32_t quicktime_3 = 0;
            uint32_t nextTrack = ToBigEndian32(2);
            void Write(Stream * s)
            {
                s->Write(this, sizeof(mvhdBox));
            }
        };
        mvhdBox mvhd;
        struct trackBox
        {
            uint32_t size = ToBigEndian32(sizeof(trackBox));
            char id[4] = { 't', 'r', 'a', 'k' };
            uint32_t updateSize()
            {
                auto rs = 8 + ToBigEndian32(tkhd.size) + ToBigEndian32(edts.size) + mdia.updateSize();
                size = ToBigEndian32(rs);
                return rs;
            }
            void Write(Stream * s)
            {
                s->Write(this, 8);
                this->tkhd.Write(s);
                this->edts.Write(s);
                this->mdia.Write(s);
            }
            struct tkhdBox
            {
                uint32_t size = ToBigEndian32(sizeof(tkhdBox));
                char id[4] = { 't', 'k', 'h', 'd' };
                unsigned char version = 0;
                unsigned char flags[2] = { 0 , 0 };
                unsigned char flag1 = 3;
                uint32_t createDate = 0, modifiedDate = 0;
                uint32_t trakId = ToBigEndian32(1);
                uint32_t reserved = 0;
                uint32_t duration = 0;  // set this!
                uint32_t reserved2[2] = { 0,0 };
                uint16_t videoLayer = 0;
                uint16_t quicktime_alternate = 0;
                decimal16 volume = decimal16(1, 0);
                uint16_t reserved3 = 0;
                decimal32 A = decimal32(1, 0);
                decimal32 B = decimal32(0, 0);
                decimal32 U = decimal32(0, 0);
                decimal32 C = decimal32(0, 0);
                decimal32 D = decimal32(1, 0);
                decimal32 V = decimal32(0, 0);
                decimal32 X = decimal32(0, 0);
                decimal32 Y = decimal32(0, 0);
                decimal32 W = decimal32(1, 0);
                decimal32 videoWidth, videoHeight; // set this!
                void Write(Stream * s)
                {
                    s->Write(this, sizeof(tkhdBox));
                }
            };
            tkhdBox tkhd;
            struct edtsBox
            {
                uint32_t size = ToBigEndian32(sizeof(edtsBox));
                char id[4] = { 'e', 'd', 't', 's' };
                struct elstBox
                {
                    uint32_t size = ToBigEndian32(sizeof(elstBox));
                    char id[4] = { 'e', 'l', 's', 't' };
                    uint32_t version_flag = 0;
                    uint32_t numEntries = ToBigEndian32(1);
                    uint32_t trackDuration = 0; // set this!
                    uint32_t trackStartTime = 0;
                    decimal32 speed = decimal32(1, 0);
                };
                elstBox elst;
                void Write(Stream * s)
                {
                    s->Write(this, sizeof(edtsBox));
                }
            };
            edtsBox edts;
            struct mdiaBox
            {
                uint32_t size = ToBigEndian32(sizeof(mdiaBox));
                char id[4] = { 'm', 'd', 'i', 'a' };
                uint32_t updateSize()
                {
                    auto rs = 8u + ToBigEndian32(mdhd.size) + ToBigEndian32(hdlr.size) + minf.updateSize();
                    size = ToBigEndian32(rs);
                    return rs;
                }
                void Write(Stream * s)
                {
                    s->Write(this, 8);
                    this->mdhd.Write(s);
                    this->hdlr.Write(s);
                    this->minf.Write(s);
                }
                struct mdhdBox
                {
                    uint32_t size = ToBigEndian32(sizeof(mdhdBox));
                    char id[4] = { 'm', 'd', 'h', 'd' };
                    uint32_t version_flag = 0;
                    uint32_t createDate = 0, modifiedDate = 0;
                    uint32_t timeScale = ToBigEndian32(unitsPerSecond);
                    uint32_t duration = 0; // set this! in mdiaBox::timeScale!
                    uint16_t language = 0;
                    uint16_t quality = 0;
                    void Write(Stream * s)
                    {
                        s->Write(this, sizeof(mdhdBox));
                    }
                };
                mdhdBox mdhd;
                struct hdlrBox
                {
                    uint32_t size = ToBigEndian32(sizeof(hdlrBox));
                    char id[4] = { 'h', 'd', 'l', 'r' };
                    uint32_t version_flag = 0;
                    char componentType[4] = { 'm', 'h', 'l', 'r' };
                    char mediaType[4] = { 'v', 'i', 'd', 'e' };
                    uint32_t reserved = 0;
                    uint32_t reserved1 = 0, reserved2 = 0;
                    char str[5] = { 'H', 'D', 'L', 'R', 0 };
                    void Write(Stream * s)
                    {
                        s->Write(this, sizeof(*this));
                    }
                };
                hdlrBox hdlr;

                struct minfBox
                {
                    uint32_t size = ToBigEndian32(sizeof(minfBox));
                    char id[4] = { 'm', 'i', 'n', 'f' };
                    uint32_t updateSize()
                    {
                        auto rs = 8 + ToBigEndian32(vmhd.size) + stbl.updateSize();
                        size = ToBigEndian32(rs);
                        return rs;
                    }
                    void Write(Stream * s)
                    {
                        s->Write(this, 8);
                        this->vmhd.Write(s);
                        this->stbl.Write(s);
                    }
                    struct vmhdBox
                    {
                        uint32_t size = ToBigEndian32(sizeof(vmhdBox));
                        char id[4] = { 'v', 'm', 'h', 'd' };
                        uint32_t version_flag = ToBigEndian32(sizeof(1));
                        uint32_t mode[2] = { 0, 0 };
                        void Write(Stream * s)
                        {
                            s->Write(this, sizeof(*this));
                        }
                    };
                    vmhdBox vmhd;
                    struct stblBox
                    {
                        uint32_t size = ToBigEndian32(sizeof(stblBox));
                        char id[4] = { 's', 't', 'b', 'l' };
                        uint32_t updateSize()
                        {
                            auto rs = 8 + stsd.updateSize() + ToBigEndian32(stsc.size) + ToBigEndian32(stts.size) + ToBigEndian32(stco.size) + stsz.updateSize() + stss.updateSize();
                            size = ToBigEndian32(rs);
                            return rs;
                        }
                        void Write(Stream * s)
                        {
                            s->Write(this, 8);
                            this->stsd.Write(s);
                            this->stsc.Write(s);
                            this->stts.Write(s);
                            this->stco.Write(s);
                            this->stsz.Write(s);
                            this->stss.Write(s);
                        }
                        struct stsdBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(stsdBox));
                            char id[4] = { 's', 't', 's', 'd' };
                            int32_t version_flag = 0;
                            uint32_t numDescriptions = ToBigEndian32(1);
                            struct descriptor
                            {
                                uint32_t length = ToBigEndian32(sizeof(descriptor));
                                char videoFormat[4] = { 'a', 'v', 'c', '1' };
                                char reserved[6] = { 0 ,0 ,0 ,0 ,0 ,0 };
                                uint16_t refIndex = ToBigEndian16(1);
                                uint16_t videoEncodingVersion = 0;
                                uint16_t videoEncodingRevision = 0;
                                uint32_t videoEncodingVendor = 0;
                                uint32_t quality1 = 0, quality2 = 0;
                                uint16_t videoWidth = 0, videoHeight = 0; // set this!
                                decimal32 pixelDensityx = decimal32(72, 0);
                                decimal32 pixelDensityy = decimal32(72, 0);
                                uint32_t videoDataSize = 0;
                                uint16_t videoFrameCount = ToBigEndian16(1);
                                char nameStringLen = 0;
                                char name[31] = { 0 };
                                uint16_t pixelColors = ToBigEndian16(24);
                                uint16_t colorTableId = 0xFFFF;
                                struct avcCBox
                                {
                                    uint32_t size = ToBigEndian32(sizeof(avcCBox));
                                    char id[4] = { 'a', 'v', 'c', 'C' };
                                    char version = 1;
                                    uint8_t header[5] = { 0x64, 0x0C, 0x34, 0xFF, 0xE1 };
                                    uint16_t SPSSize = ToBigEndian16(0x11); // set this!
                                    List<uint8_t> SPS; // set this!
                                    uint8_t numPPS = 1;
                                    uint16_t ppsLen = ToBigEndian16(4);
                                    List<uint8_t> PPS;
                                    void Write(Stream * s)
                                    {
                                        s->Write(this, 16);
                                        s->Write(SPS.Buffer(), SPS.Count());
                                        s->Write(&numPPS, 1);
                                        s->Write(&ppsLen, 2);
                                        s->Write(PPS.Buffer(), PPS.Count());
                                    }
                                    uint32_t updateSize()
                                    {
                                        uint32_t rs = 16 + SPS.Count() + 1 + 2 + PPS.Count();
                                        size = ToBigEndian32(rs);
                                        return rs;
                                    }
                                };
                                avcCBox avcC;
                                void Write(Stream * s)
                                {
                                    s->Write(this, 86);
                                    avcC.Write(s);
                                }
                                uint32_t updateSize()
                                {
                                    auto rs = 86 + avcC.updateSize();
                                    this->length = ToBigEndian32(rs);
                                    return rs;
                                }
                            };
                            descriptor desc;
                            uint32_t updateSize()
                            {
                                auto rs = 16 + desc.updateSize();
                                this->size = ToBigEndian32(rs);
                                return rs;
                            }
                            void Write(Stream * s)
                            {
                                s->Write(this, 16);
                                this->desc.Write(s);
                            }
                        };
                        stsdBox stsd;
                        struct sttsBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(sttsBox));
                            char id[4] = { 's', 't', 't', 's' };
                            uint32_t flag = 0;
                            uint32_t numTimes = ToBigEndian32(1);
                            uint32_t frameCount; // set this!
                            uint32_t perSampleDuration; // set this! in milliseconds.
                            void Write(Stream * s)
                            {
                                s->Write(this, sizeof(sttsBox));
                            }
                        };
                        sttsBox stts;
                        struct stscBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(stscBox));
                            char id[4] = { 's', 't', 's', 'c' };
                            uint32_t flag = 0;
                            uint32_t numBlocks = ToBigEndian32(1);
                            uint32_t block0_firstFrame = ToBigEndian32(1);
                            uint32_t block0_framesCount = 0; // set this!
                            uint32_t descId = ToBigEndian32(1);
                            void Write(Stream * s)
                            {
                                s->Write(this, sizeof(stscBox));
                            }
                        };
                        stscBox stsc;
                        struct stssBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(stssBox));
                            char id[4] = { 's', 't', 's', 's' };
                            uint32_t flag = 0;
                            uint32_t numEntries = 0;
                            List<uint32_t> entries;
                            void Write(Stream * s)
                            {
                                s->Write(this, 16);
                                s->Write(entries.Buffer(), entries.Count() * sizeof(uint32_t));
                            }
                            uint32_t updateSize()
                            {
                                auto s = (uint32_t)(16 + entries.Count() * sizeof(uint32_t));
                                size = ToBigEndian32(s);
                                return s;
                            }
                        };
                        stssBox stss;
                        struct stcoBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(stcoBox));
                            char id[4] = { 's', 't', 'c', 'o' };
                            uint32_t flag = 0;
                            uint32_t num = ToBigEndian32(1);
                            uint32_t offset = ToBigEndian32(0x30u);
                            void Write(Stream * s)
                            {
                                s->Write(this, sizeof(stcoBox));
                            }
                        };
                        stcoBox stco;
                        struct stszBox
                        {
                            uint32_t size = ToBigEndian32(sizeof(stszBox));
                            char id[4] = { 's', 't', 's', 'z' };
                            uint32_t flag = 0;
                            uint32_t sizeAll = 0;
                            uint32_t numSizes = 0; // set this!
                            List<uint32_t> sizes;
                            uint32_t updateSize()
                            {
                                auto s = (uint32_t)(20 + sizes.Count() * sizeof(uint32_t));
                                size = ToBigEndian32(s);
                                return s;
                            }
                            void Write(Stream * s)
                            {
                                s->Write(this, 20);
                                s->Write(sizes.Buffer(), sizes.Count() * sizeof(uint32_t));
                            }
                        };
                        stszBox stsz;
                    };
                    stblBox stbl;
                };
                minfBox minf;
            };
            mdiaBox mdia;
        };
        trackBox track;
    };
#pragma pack(pop)
    class H264VideoEncoder : public IVideoEncoder
    {
    private:
        ISVCEncoder * encoder = nullptr;
        Stream * stream = nullptr;
        int width, height;
        List<unsigned char> yuv;
        List<uint32_t> frameSizes;
        int fps = 30;
        moovBox moov;
    public:
        struct Color
        {
            unsigned char r, g, b, a;
        };
        void RGB2YUV(int w, int h, unsigned char * rgbaImage)
        {
            yuv.SetSize(width * height * 3 / 2);
            auto yPlane = yuv.begin();
            auto uPlane = yPlane + width * height;
            auto vPlane = uPlane + width * height / 4;
            auto hWidth = width >> 1;
            auto hHeight = height >> 1;
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    Color color;
                    if (i < h && j < w)
                        color = ((Color*)rgbaImage)[i * width + j];
                    else
                        color = Color{ 0,0,0,0 };
                    int lum = 19595 * color.r + 38470 * color.g + 7471 * color.b;
                    auto y = (unsigned char)Math::Clamp(((lum) >> 16), 0, 255);
                    auto Cb = (unsigned char)Math::Clamp(((36962 * (color.b - y)) >> 16) + 128, 0, 255);
                    auto Cr = (unsigned char)Math::Clamp(((46727 * (color.r - y)) >> 16) + 128, 0, 255);
                    yPlane[(height - i - 1)*width + j] = y;
                    if ((i & 1) == 0 && (j & 1) == 0)
                    {
                        uPlane[(hHeight - (i >> 1) - 1) * hWidth + (j >> 1)] = Cb;
                        vPlane[(hHeight - (i >> 1) - 1) * hWidth + (j >> 1)] = Cr;
                    }
                }
            }
        }
       
        List<int> leadingWordPos;
        void WriteFrame(SFrameBSInfo & info)
        {
            const unsigned int leading = 0x01000000;
            uint32_t totalLen = 0;
            for (int i = 0; i < info.iLayerNum; i++)
            {
                auto srcBuf = info.sLayerInfo[i].pBsBuf;
                for (int j = 0; j < info.sLayerInfo[i].iNalCount; j++)
                {
                    int len = info.sLayerInfo[i].pNalLengthInByte[j];
                    totalLen += (uint32_t)len;
                    auto b = ToBigEndian32((unsigned int)len - 4);
                    stream->Write(&b, sizeof(b));
                    stream->Write(srcBuf + 4, len - 4);
                    
                    srcBuf += len;
                }
            }

            if (frameSizes.Count() == 0 && info.iLayerNum > 1)
            {
                auto srcBuf = info.sLayerInfo[0].pBsBuf;
                for (int j = 0; j < info.sLayerInfo[0].iNalCount; j++)
                {
                    // if this is the first frame, OpenH264 will produce two layers.
                    // the first layer contains two NALs representing SPS and PPS descriptors.
                    // and we need to write these descriptors to the moov header as well.
                    int len = info.sLayerInfo[0].pNalLengthInByte[j];
                    if (j == 0)
                    {
                        moov.track.mdia.minf.stbl.stsd.desc.avcC.SPSSize = ToBigEndian16((uint16_t)(len - 4));
                        moov.track.mdia.minf.stbl.stsd.desc.avcC.SPS.AddRange(srcBuf + 4, len - 4);
                    }
                    else
                    {
                        moov.track.mdia.minf.stbl.stsd.desc.avcC.ppsLen = ToBigEndian16((uint16_t)(len - 4));
                        moov.track.mdia.minf.stbl.stsd.desc.avcC.PPS.AddRange(srcBuf + 4, len - 4);
                    }
                }
            }
            frameSizes.Add(totalLen);

            // if this is an IDR frame, add a reference to synchronization table
            if (info.eFrameType == videoFrameTypeIDR ||
                info.eFrameType == videoFrameTypeI)
            {
                moov.track.mdia.minf.stbl.stss.entries.Add(ToBigEndian32(frameSizes.Count()));
                moov.track.mdia.minf.stbl.stss.numEntries = ToBigEndian32((uint32_t)moov.track.mdia.minf.stbl.stss.entries.Count());
            }
        }
        void BigEndianAdd(uint32_t & a, uint32_t b)
        {
            a = ToBigEndian32(ToBigEndian32(a) + b);
        }
        int frameCount = 0;
        virtual void Init(VideoEncodingOptions options, Stream * outputStream) override
        {
            width = options.Width;
            height = options.Height;
            stream = outputStream;
            fps = options.FramesPerSecond;
            WelsCreateSVCEncoder(&encoder);
            SEncParamExt params;
            encoder->GetDefaultParams(&params);
            params.iPicWidth = options.Width;
            params.iPicHeight = options.Height;
            params.iTargetBitrate = options.Bitrate * 10;
            params.iUsageType = CAMERA_VIDEO_REAL_TIME;
            params.iComplexityMode = HIGH_COMPLEXITY;
            params.fMaxFrameRate = (float)options.FramesPerSecond;
            params.iSpatialLayerNum = 1;
            params.iMaxQp = 1;
            params.iMinQp = 0;
            params.iRCMode = RC_QUALITY_MODE;
            params.iMultipleThreadIdc = 8;
            for (int i = 0; i < params.iSpatialLayerNum; i++) {
                params.sSpatialLayers[i].iVideoWidth = width >> (params.iSpatialLayerNum - 1 - i);
                params.sSpatialLayers[i].iVideoHeight = height >> (params.iSpatialLayerNum - 1 - i);
                params.sSpatialLayers[i].fFrameRate = (float)options.FramesPerSecond;
                params.sSpatialLayers[i].iSpatialBitrate = params.iTargetBitrate;
                params.sSpatialLayers[i].uiProfileIdc = PRO_HIGH;
                params.sSpatialLayers[i].uiLevelIdc = LEVEL_5_2;
                params.sSpatialLayers[i].uiVideoFormat = 2;
                params.sSpatialLayers[i].iDLayerQp = 0;
                params.sSpatialLayers[i].iMaxSpatialBitrate = UNSPECIFIED_BIT_RATE;
                params.sSpatialLayers[i].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
                params.sSpatialLayers[i].sSliceArgument.uiSliceNum = 8;

            }
            params.iTargetBitrate *= params.iSpatialLayerNum;

            encoder->InitializeExt(&params);
            int videoFormat = videoFormatI420;
            encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &videoFormat);

            const unsigned char mp4header[] = {
                0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70,   0x69, 0x73, 0x6F, 0x6D, 0x00, 0x00, 0x02, 0x00,
                0x69, 0x73, 0x6F, 0x6D, 0x69, 0x73, 0x6F, 0x32,   0x61, 0x76, 0x63, 0x31, 0x6D, 0x70, 0x34, 0x31,
                0x00, 0x00, 0x00, 0x08, 0x66, 0x72, 0x65, 0x65,   0x00, 0x00, 0x00, 0x00, 0x6D, 0x64, 0x61, 0x74
            };
            stream->Write(mp4header, sizeof(mp4header));
            frameSizes.Clear();
        }
        virtual void EncodeFrame(int w, int h, unsigned char * rgbaImage) override
        {
             SFrameBSInfo info;
             memset(&info, 0, sizeof(SFrameBSInfo));
             SSourcePicture pic;
             memset(&pic, 0, sizeof(SSourcePicture));
             RGB2YUV(w, h, rgbaImage);
             pic.iPicWidth = width;
             pic.iPicHeight = height;
             pic.iColorFormat = videoFormatI420;
             pic.iStride[0] = pic.iPicWidth;
             pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
             pic.pData[0] = yuv.Buffer();
             pic.pData[1] = pic.pData[0] + width * height;
             pic.pData[2] = pic.pData[1] + (width * height >> 2);
             pic.uiTimeStamp = frameCount * 1000 / fps;
             //prepare input data
             encoder->ForceIntraFrame(frameCount % (fps * 2) == 0);
             encoder->EncodeFrame(&pic, &info);
             if (info.eFrameType != videoFrameTypeSkip) 
             {
                 //output bitstream
                 WriteFrame(info);
             }
             frameCount++;
        }
        virtual void Close() override
        {
            auto pos = stream->GetPosition();
            // write tailer
            moov.mvhd.duration = ToBigEndian32((uint32_t)(frameSizes.Count() * 1000 / fps));
            moov.track.edts.elst.trackDuration = moov.mvhd.duration;
            moov.track.tkhd.duration = moov.mvhd.duration;
            moov.track.tkhd.videoHeight.set((uint16_t)height, 0u);
            moov.track.tkhd.videoWidth.set((uint16_t)width, 0u);
            moov.track.mdia.mdhd.duration = ToBigEndian32((uint32_t)((uint64_t)frameSizes.Count() * unitsPerSecond / fps));
            moov.track.mdia.minf.stbl.stts.perSampleDuration = ToBigEndian32((uint32_t)(unitsPerSecond / fps));
            moov.track.mdia.minf.stbl.stts.frameCount = ToBigEndian32(frameSizes.Count());
            moov.track.mdia.minf.stbl.stsd.desc.videoWidth = ToBigEndian16((uint16_t)width);
            moov.track.mdia.minf.stbl.stsd.desc.videoHeight = ToBigEndian16((uint16_t)height);
            moov.track.mdia.minf.stbl.stsc.block0_framesCount = ToBigEndian32(frameSizes.Count());
            moov.track.mdia.minf.stbl.stsz.numSizes = ToBigEndian32(frameSizes.Count());
            for (auto size : frameSizes)
            {
                auto e = ToBigEndian32((uint32_t)size);
                moov.track.mdia.minf.stbl.stsz.sizes.Add(e);
            }
            moov.Write(stream);

            stream->Seek(SeekOrigin::Start, 40);
            uint32_t mdatLen = ToBigEndian32((uint32_t)(pos - 40));
            stream->Write(&mdatLen, sizeof(uint32_t));
            WelsDestroySVCEncoder(encoder);
        }
    };
    IVideoEncoder * CreateH264VideoEncoder()
    {
        return new H264VideoEncoder();
    }

}