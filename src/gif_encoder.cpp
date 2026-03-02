#include "gif_encoder.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

GIFEncoder::GIFEncoder(int w, int h)
    : width(w), height(h), gifFile(nullptr),
      isRecording(false), colorMap(nullptr) {
    frameBufferRGBA.resize(width * height);
    frameBufferIndexed.resize(width * height);
}

GIFEncoder::~GIFEncoder() {
    if(isRecording) {
        finishRecording();
    }
    if(colorMap) {
        GifFreeMapObject(colorMap);
    }
}

// Create a standard 256-color palette (6x6x6 RGB cube + grays)
void GIFEncoder::initColorMap() {
    if(colorMap) {
        GifFreeMapObject(colorMap);
    }
    colorMap = GifMakeMapObject(256, NULL);

    int idx = 0;
    // 6x6x6 RGB cube: 216 colors
    for(int r = 0; r < 6; r++) {
        for(int g = 0; g < 6; g++) {
            for(int b = 0; b < 6; b++) {
                colorMap->Colors[idx].Red = (r * 255) / 5;
                colorMap->Colors[idx].Green = (g * 255) / 5;
                colorMap->Colors[idx].Blue = (b * 255) / 5;
                idx++;
            }
        }
    }

    // Fill remaining 40 colors with grayscale gradient
    for(int i = 0; i < 40 && idx < 256; i++) {
        uint8_t gray = (i * 255) / 39;
        colorMap->Colors[idx].Red = gray;
        colorMap->Colors[idx].Green = gray;
        colorMap->Colors[idx].Blue = gray;
        idx++;
    }
}

// Find nearest color in the palette using Euclidean distance
uint8_t GIFEncoder::findNearestColor(uint8_t r, uint8_t g, uint8_t b) {
    int minDist = INT_MAX;
    uint8_t bestIdx = 0;

    for(int i = 0; i < 256; i++) {
        int dr = (int)r - (int)colorMap->Colors[i].Red;
        int dg = (int)g - (int)colorMap->Colors[i].Green;
        int db = (int)b - (int)colorMap->Colors[i].Blue;
        int dist = dr*dr + dg*dg + db*db;

        if(dist < minDist) {
            minDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void GIFEncoder::quantizeFrame(const uint32_t* pixelData) {
    // Quantize ARGB pixels to indexed color
    for(int i = 0; i < width * height; i++) {
        uint32_t argb = pixelData[i];
        uint8_t r = (argb >> 16) & 0xFF;
        uint8_t g = (argb >> 8) & 0xFF;
        uint8_t b = argb & 0xFF;
        frameBufferIndexed[i] = findNearestColor(r, g, b);
    }
}

bool GIFEncoder::startRecording(const std::string& filename) {
    int error = 0;
    gifFile = EGifOpenFileName(filename.c_str(), false, &error);
    if(!gifFile) {
        fprintf(stderr, "Failed to open GIF file: %d\n", error);
        return false;
    }

    // Initialize the standard color palette
    initColorMap();

    // Set GIF version and screen descriptor
    EGifSetGifVersion(gifFile, true);
    if(EGifPutScreenDesc(gifFile, width, height, 8, 0, colorMap) != GIF_OK) {
        int err = 0;
        EGifCloseFile(gifFile, &err);
        return false;
    }

    isRecording = true;
    return true;
}

void GIFEncoder::addFrame(uint32_t* pixelData, int delayMs) {
    if(!isRecording || !gifFile) return;

    // Quantize the frame to indexed color
    quantizeFrame(pixelData);

    // Add graphics control extension for delay
    GraphicsControlBlock gcb;
    gcb.DisposalMode = DISPOSE_DO_NOT;
    gcb.UserInputFlag = false;
    gcb.TransparentColor = NO_TRANSPARENT_COLOR;
    // convert ms -> cs with rounding
    int delayCs = (delayMs + 5) / 10;
    if(delayCs < 2) delayCs = 2;  // minimum 20ms to avoid viewer bugs
    gcb.DelayTime = delayCs;

    if(EGifPutExtension(gifFile, GRAPHICS_EXT_FUNC_CODE, 4, &gcb) != GIF_OK) {
        return;
    }

    // Add image descriptor
    if(EGifPutImageDesc(gifFile, 0, 0, width, height, false, NULL) != GIF_OK) {
        return;
    }

    // Write scanlines
    for(int y = 0; y < height; y++) {
        if(EGifPutLine(gifFile, &frameBufferIndexed[y * width], width) != GIF_OK) {
            return;
        }
    }
}

void GIFEncoder::finishRecording() {
    if(!gifFile) return;

    int error = 0;
    EGifCloseFile(gifFile, &error);
    gifFile = nullptr;
    isRecording = false;
}
