#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <gif_lib.h>
#include <string>
#include <vector>
#include <cstdint>

class GIFEncoder {
private:
    GifFileType* gifFile;
    int width, height;
    ColorMapObject* colorMap;
    std::vector<uint32_t> frameBufferRGBA;  // Store frames as ARGB32
    std::vector<uint8_t> frameBufferIndexed;  // Indexed version for writing
    bool isRecording;

    void initColorMap();
    void quantizeFrame(const uint32_t* pixelData);
    uint8_t findNearestColor(uint8_t r, uint8_t g, uint8_t b);

public:
    GIFEncoder(int width, int height);
    ~GIFEncoder();

    bool startRecording(const std::string& filename);
    void addFrame(uint32_t* pixelData, int delayMs = 16);
    void finishRecording();
    bool recording() const { return isRecording; }
};

#endif

