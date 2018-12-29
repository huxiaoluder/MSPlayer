//
//  APCodecContext.cpp
//  MediaStreamPlayer
//
//  Created by 胡校明 on 2018/12/26.
//  Copyright © 2018 freecoder. All rights reserved.
//

#include "APCodecContext.hpp"

using namespace MS;
using namespace MS::APhard;

APCodecContext::APCodecContext(const APCodecType codecType,
                               const MSCodecID codecID,
                               MSPlayer<__CVBuffer> &player)
:codecType(codecType),
codecID(codecID),
audioConvert(nullptr),
videoDecodeSession(nullptr),
videoEncodeSession(nullptr),
player(player) {
    
}

APCodecContext::APCodecContext(const APCodecType codecType,
                               const MSCodecID codecID,
                               const MSBinaryData &spsData,
                               const MSBinaryData &ppsData,
                               MSPlayer<__CVBuffer> &player)
:codecType(codecType),
codecID(codecID),
audioConvert(initAudioConvert()),
videoDecodeSession(initVideoDecodeSession(spsData,ppsData)),
videoEncodeSession(initVideoEncodeSession()),
player(player) {
    
}

APCodecContext::~APCodecContext() {
    
}

AudioConverterRef
APCodecContext::initAudioConvert() {
    return nullptr;
}

VTCompressionSessionRef
APCodecContext::initVideoEncodeSession() {
    return nullptr;
}

VTDecompressionSessionRef
APCodecContext::initVideoDecodeSession(const MSBinaryData &spsData, const MSBinaryData &ppsData) {
    APCodecInfo codecInfo = getAPCodecInfo(codecID);
//    CMVideoCodecType codec_type = get<0>(codecInfo);
    IsVideoCodec isVideoCodec   = get<1>(codecInfo);
    
    CMFormatDescriptionRef      formatDescription   = nullptr;
    VTDecompressionSessionRef   videoDecodeSession  = nullptr;
    
    if (codecType == APCodecDecoder && isVideoCodec) {
        OSStatus status;
        
        uint8_t *datas[2] = {spsData.bytes, ppsData.bytes};
        size_t lengths[2] = {spsData.size,  ppsData.size};
        
        if (codecID == MSCodecID_H264) {
            status = CMVideoFormatDescriptionCreateFromH264ParameterSets(nullptr, sizeof(datas), datas, lengths, 1, &formatDescription);
        } else {
            if (__builtin_available(iOS 11.0, *)) {
                status = CMVideoFormatDescriptionCreateFromHEVCParameterSets(nullptr, sizeof(datas), datas, lengths, 1, nullptr, &formatDescription);
            } else {
                return nullptr;
            }
        }
    
        /*
         CFDictionaryRef    videoDecoderSpecification,
         CFDictionaryRef    destinationImageBufferAttributes,
         */
        VTDecompressionOutputCallbackRecord outputCallback;
        outputCallback.decompressionOutputCallback = &decompressionOutputCallback;
        outputCallback.decompressionOutputRefCon = this;
        
        status = VTDecompressionSessionCreate(nullptr,
                                              formatDescription,
                                              nullptr,
                                              nullptr,
                                              &outputCallback,
                                              &videoDecodeSession);
    }
    return videoDecodeSession;
}

APCodecInfo
APCodecContext::getAPCodecInfo(const MSCodecID codecID) {
    APCodecID codec_id;
    switch (codecID) {
        case MSCodecID_H264:    codec_id = kCMVideoCodecType_H264;  break;
        case MSCodecID_H265:    codec_id = kCMVideoCodecType_HEVC;  break;
        case MSCodecID_AAC:     codec_id = kAudioFormatMPEG4AAC;    break;
        case MSCodecID_OPUS:    codec_id = kAudioFormatOpus;        break;
        case MSCodecID_ALAW:    codec_id = kAudioFormatALaw;        break;
    }
    return APCodecInfo(codec_id, codecID <= MSCodecID_H265);
}

void
APCodecContext::decompressionOutputCallback(void * _Nullable decompressionOutputRefCon,
                                            void * _Nullable sourceFrameRefCon,
                                            OSStatus status,
                                            VTDecodeInfoFlags infoFlags,
                                            CVImageBufferRef _Nullable imageBuffer,
                                            CMTime presentationTimeStamp,
                                            CMTime presentationDuration) {
    APCodecContext &codecContext = *(APCodecContext *)decompressionOutputRefCon;
    codecContext.player.asynPushAudioFrameData(nullptr);
};