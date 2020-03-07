#ifndef FASTVIDEO_DECODER_H
#define FASTVIDEO_DECODER_H

#include <fastvideo_sdk.h>

#include "common.h"
#include "common_utils.h"

class fastvideo_decoder
{
public:
    fastvideo_decoder();
    ~fastvideo_decoder();

	bool init_decoder(uint32_t width, uint32_t height, fastSurfaceFormat_t fmt);

    bool decode(const uint8_t *input, uint32_t len, PImage &output);
    bool decode(const bytearray& input, PImage &output);

private:
    fastJpegDecoderHandle_t m_handle = nullptr;
    fastDeviceSurfaceBufferHandle_t m_dHandle = nullptr;
    fastSurfaceFormat_t m_surfaceFmt;
    fastExportToHostHandle_t m_DeviceToHost = nullptr;

    bool m_isInit = false;

    bytearray buf;
};

#endif // FASTVIDEO_DECODER_H
