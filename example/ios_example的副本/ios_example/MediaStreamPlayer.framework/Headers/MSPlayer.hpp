//
//  MSPlayer.hpp
//  MediaStreamPlayer
//
//  Created by xiaoming on 2018/11/14.
//  Copyright © 2018 freecoder. All rights reserved.
//

#ifndef MSPlayer_hpp
#define MSPlayer_hpp

#include <string>
#include <queue>
#include <cassert>
#include "MSMacros.h"
#include "MSTimer.hpp"
#include "MSAsynDataReceiver.h"
#include "MSDecoderProtocol.h"
#include "MSEncoderProtocol.h"

namespace MS {
    
    using namespace std;
    
    using namespace chrono;

#pragma mark - MSPlayer<T>(declaration)
    template <typename T>
    class MSPlayer : public MSAsynDataReceiver<T> {
        typedef function<void(const MSMedia<MSDecodeMedia,T> &decodeData)> ThrowDecodeData;
        
        MSSyncDecoderProtocol<T> * MSNullable const _syncDecoder;
        
        MSAsynDecoderProtocol<T> * MSNullable const _asynDecoder;
        
        MSEncoderProtocol<T> * MSNullable const _reEncoder;
        
#ifdef __APPLE__
        MSTimer<MSTimerForApple> * MSNonnull const videoTimer;
        MSTimer<MSTimerForApple> * MSNonnull const audioTimer;
#else
        MSTimer<MSTimerForOther> * MSNonnull const videoTimer;
        MSTimer<MSTimerForOther> * MSNonnull const audioTimer;
#endif
        
        thread videoDecodeThread;
        
        thread audioDecodeThread;
        
        condition_variable videoThreadCondition;
        
        condition_variable audioThreadCondition;
        
        mutex videoConditionMutex;
        
        mutex audioConditionMutex;
        
        mutex videoQueueMutex;
        
        mutex pixelQueueMutex;
        
        mutex audioQueueMutex;
        
        mutex sampleQueueMutex;
        
        queue<const MSMedia<MSEncodeMedia> *> videoQueue;
        
        queue<const MSMedia<MSEncodeMedia> *> audioQueue;
        
        queue<const MSMedia<MSDecodeMedia, T> *> pixelQueue;
        
        queue<const MSMedia<MSDecodeMedia, T> *> sampleQueue;
        
        bool isDecoding = true;
        
        bool isEncoding = false;
        
        const ThrowDecodeData throwDecodeVideo;
        
        const ThrowDecodeData throwDecodeAudio;
        
        void clearAllVideo();
        void clearAllAudio();
        
        thread initSyncDataVideoDecodeThread();
        thread initSyncDataAudioDecodeThread();
        
        thread initAsynDataVideoDecodeThread();
        thread initAsynDataAudioDecodeThread();
        
#ifdef __APPLE__
        MSTimer<MSTimerForApple> * MSNonnull initSyncDataVideoTimer();
        MSTimer<MSTimerForApple> * MSNonnull initSyncDataAudioTimer();
        
        MSTimer<MSTimerForApple> * MSNonnull initAsynDataVideoTimer();
        MSTimer<MSTimerForApple> * MSNonnull initAsynDataAudioTimer();
#else
        MSTimer<MSTimerForOther> * MSNonnull initSyncDataVideoTimer();
        MSTimer<MSTimerForOther> * MSNonnull initSyncDataAudioTimer();
        
        MSTimer<MSTimerForOther> * MSNonnull initAsynDataVideoTimer();
        MSTimer<MSTimerForOther> * MSNonnull initAsynDataAudioTimer();
#endif
        
    public:
        MSPlayer(MSSyncDecoderProtocol<T> * MSNonnull const decoder,
                 MSEncoderProtocol<T> * MSNullable const encoder,
                 const ThrowDecodeData throwDecodeVideo,
                 const ThrowDecodeData throwDecodeAudio);
        
        MSPlayer(MSAsynDecoderProtocol<T> * MSNonnull const decoder,
                 MSEncoderProtocol<T> * MSNullable const encoder,
                 const ThrowDecodeData throwDecodeVideo,
                 const ThrowDecodeData throwDecodeAudio);
        
        ~MSPlayer();
        
        MSSyncDecoderProtocol<T> & syncDecoder();
        
        MSAsynDecoderProtocol<T> & asynDecoder();
        
        MSEncoderProtocol<T> & reEncoder();
        
        void startPlayVideo();
        
        void pausePlayVideo();
        
        void continuePlayVideo();
        
        void stopPlayVideo();
        
        void startPlayAudio();

        void pausePlayAudio();

        void continuePlayAudio();

        void stopPlayAudio();
        
        void startReEncode();
        
        void pauseReEncode();
        
        void continueReEncode();
        
        void stopReEncode();
        
        void pushVideoStreamData(const MSMedia<MSEncodeMedia> * MSNonnull const streamData);
        
        void pushAudioStreamData(const MSMedia<MSEncodeMedia> * MSNonnull const streamData);
        
    private: // 不允许外部主调, 请通过 Protocol 进行多态调用
        void asynPushVideoFrameData(const MSMedia<MSDecodeMedia,T> * MSNonnull const frameData);
        
        void asynPushAudioFrameData(const MSMedia<MSDecodeMedia,T> * MSNonnull const frameData);
    };
    
#pragma mark - MSPlayer<T>(implementation)
    template <typename T>
    MSPlayer<T>::MSPlayer(MSSyncDecoderProtocol<T> * const MSNonnull decoder,
                          MSEncoderProtocol<T> * const MSNullable encoder,
                          const ThrowDecodeData throwDecodeVideo,
                          const ThrowDecodeData throwDecodeAudio)
    :_syncDecoder(decoder), _reEncoder(encoder),
    _asynDecoder(nullptr),
    throwDecodeVideo(throwDecodeVideo),
    throwDecodeAudio(throwDecodeAudio),
    videoTimer(initSyncDataVideoTimer()),
    audioTimer(initSyncDataAudioTimer()),
    videoDecodeThread(initSyncDataVideoDecodeThread()),
    audioDecodeThread(initSyncDataAudioDecodeThread()) {
        assert(_syncDecoder && throwDecodeVideo && throwDecodeAudio);
    }
    
    template <typename T>
    MSPlayer<T>::MSPlayer(MSAsynDecoderProtocol<T> * const MSNonnull decoder,
                          MSEncoderProtocol<T> * const MSNullable encoder,
                          const ThrowDecodeData throwDecodeVideo,
                          const ThrowDecodeData throwDecodeAudio)
    :_asynDecoder(decoder), _reEncoder(encoder),
    _syncDecoder(nullptr),
    throwDecodeVideo(throwDecodeVideo),
    throwDecodeAudio(throwDecodeAudio),
    videoTimer(initAsynDataVideoTimer()),
    audioTimer(initAsynDataAudioTimer()),
    videoDecodeThread(initAsynDataVideoDecodeThread()),
    audioDecodeThread(initAsynDataAudioDecodeThread()) {
        assert(_asynDecoder && throwDecodeVideo && throwDecodeAudio);
        _asynDecoder->setDataReceiver(this);
    }
    
    template <typename T>
    MSPlayer<T>::~MSPlayer() {
        stopPlayVideo();
        stopPlayAudio();
        isDecoding = false;
        delete videoTimer;
        delete audioTimer;
        videoThreadCondition.notify_one();
        if (videoDecodeThread.joinable()) {
            videoDecodeThread.join();
        }
        audioThreadCondition.notify_one();
        if (audioDecodeThread.joinable()) {
            audioDecodeThread.join();
        }
        if (_syncDecoder) {
            delete _syncDecoder;
        }
        if (_asynDecoder) {
            delete _asynDecoder;
        }
        if (_reEncoder) {
            delete _reEncoder;
        }
    }
    
    template <typename T>
    MSSyncDecoderProtocol<T> &
    MSPlayer<T>::syncDecoder() {
        return *_syncDecoder;
    }
    
    template <typename T>
    MSEncoderProtocol<T> &
    MSPlayer<T>::reEncoder() {
        return *_reEncoder;
    }
    
    template <typename T>
    MSAsynDecoderProtocol<T> &
    MSPlayer<T>::asynDecoder() {
        return *_asynDecoder;
    }
    
    template <typename T>
    void MSPlayer<T>::clearAllVideo() {
        const MSMedia<MSEncodeMedia> *encodeData = nullptr;
        const MSMedia<MSDecodeMedia,T> *decodeData = nullptr;
        while (!videoQueue.empty()) {
            encodeData = videoQueue.front();
            videoQueue.pop();
            delete encodeData;
        }
        while (!pixelQueue.empty()) {
            decodeData = pixelQueue.front();
            pixelQueue.pop();
            delete decodeData;
        }
    }
    
    template <typename T>
    void MSPlayer<T>::clearAllAudio() {
        const MSMedia<MSEncodeMedia> *encodeData = nullptr;
        const MSMedia<MSDecodeMedia,T> *decodeData = nullptr;
        while (!audioQueue.empty()) {
            encodeData = audioQueue.front();
            audioQueue.pop();
            delete encodeData;
        }
        while (!sampleQueue.empty()) {
            decodeData = sampleQueue.front();
            sampleQueue.pop();
            delete decodeData;
        }
    }
    
    template <typename T>
    void MSPlayer<T>::startPlayVideo() {
        assert(videoTimer->isActivity() == false);
        microseconds timeInterval = audioTimer->getTimeInterval();
        if (timeInterval != intervale(1)) { // 同步音视频播放时间
            videoTimer->updateDelayTime((audioQueue.size() + sampleQueue.size()) * timeInterval);
        }
        videoTimer->start();
    }
    
    template <typename T>
    void MSPlayer<T>::pausePlayVideo() {
        videoTimer->pause();
    }
    
    template <typename T>
    void MSPlayer<T>::continuePlayVideo() {
        videoTimer->rePlay();
    }
    
    template <typename T>
    void MSPlayer<T>::stopPlayVideo() {
        videoTimer->stop();
        isEncoding = false;
        clearAllVideo();
    }
    
    template <typename T>
    void MSPlayer<T>::startPlayAudio() {
        assert(audioTimer->isActivity() == false);
        microseconds timeInterval = videoTimer->getTimeInterval();
        if (timeInterval != intervale(1)) { // 同步音视频播放时间
            audioTimer->updateDelayTime((videoQueue.size() + pixelQueue.size()) * timeInterval);
        }
        audioTimer->start();
    }
    
    template <typename T>
    void MSPlayer<T>::pausePlayAudio() {
        audioTimer->pause();
    }
    
    template <typename T>
    void MSPlayer<T>::continuePlayAudio() {
        audioTimer->rePlay();
    }
    
    template <typename T>
    void MSPlayer<T>::stopPlayAudio() {
        audioTimer->stop();
        isEncoding = false;
        clearAllAudio();
    }
    
    template <typename T>
    void MSPlayer<T>::startReEncode() {
        assert(_reEncoder->isEncoding() == false);
        _reEncoder->beginEncode();
        isEncoding = true;
    }
    
    template <typename T>
    void MSPlayer<T>::pauseReEncode() {
        isEncoding = false;
    }
    
    template <typename T>
    void MSPlayer<T>::continueReEncode() {
        isEncoding = true;
    }
    
    template <typename T>
    void MSPlayer<T>::stopReEncode() {
        isEncoding = false;
        _reEncoder->endEncode();
    }
    
    template <typename T>
    void MSPlayer<T>::pushVideoStreamData(const MSMedia<MSEncodeMedia> * MSNonnull const streamData) {
        if (videoTimer->isActivity()) {
            while (!videoQueueMutex.try_lock());
            videoQueue.push(streamData);
            videoQueueMutex.unlock();
        } else {
            delete streamData;
        }
    }
    
    template <typename T>
    void MSPlayer<T>::pushAudioStreamData(const MSMedia<MSEncodeMedia> * MSNonnull const streamData) {
        if (audioTimer->isActivity()) {
            while (!audioQueueMutex.try_lock());
            audioQueue.push(streamData);
            audioQueueMutex.unlock();
        } else {
            delete streamData;
        }
    }
    
    template <typename T>
    thread MSPlayer<T>::initSyncDataVideoDecodeThread() {
        return thread([this](){
            const MSMedia<MSEncodeMedia> *sourceData = nullptr;
            const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
            while (isDecoding) {
                while (videoQueue.empty() || pixelQueue.size() > MaxPixelBufferSize) {
                    unique_lock<mutex> lock(videoConditionMutex);
                    videoThreadCondition.wait(lock);
                    if (!isDecoding) break;
                }
                if (!isDecoding) break;
                sourceData = videoQueue.front();
                while (!videoQueueMutex.try_lock());
                videoQueue.pop();
                videoQueueMutex.unlock();
                frameData = _syncDecoder->decodeVideo(sourceData);
                if (frameData) {
                    while (!pixelQueueMutex.try_lock());
                    pixelQueue.push(frameData);
                    pixelQueueMutex.unlock();
                }
            }
        });
    }
    
    template <typename T>
    thread MSPlayer<T>::initSyncDataAudioDecodeThread() {
        return thread([this](){
            const MSMedia<MSEncodeMedia> *sourceData = nullptr;
            const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
            while (isDecoding) {
                while (audioQueue.empty() || sampleQueue.size() > MaxSampleBufferSize) {
                    unique_lock<mutex> lock(audioConditionMutex);
                    audioThreadCondition.wait(lock);
                    if (!isDecoding) break;
                }
                if (!isDecoding) break;
                sourceData = audioQueue.front();
                while (!audioQueueMutex.try_lock());
                audioQueue.pop();
                audioQueueMutex.unlock();
                frameData = _syncDecoder->decodeAudio(sourceData);
                if (frameData) {
                    while (!sampleQueueMutex.try_lock());
                    sampleQueue.push(frameData);
                    sampleQueueMutex.unlock();
                }
            }
        });
    }
    
    template <typename T>
    thread MSPlayer<T>::initAsynDataVideoDecodeThread() {
        return thread([this](){
            const MSMedia<MSEncodeMedia> *sourceData = nullptr;
            while (isDecoding) {
                while (videoQueue.empty() || pixelQueue.size() > MaxPixelBufferSize) {
                    unique_lock<mutex> lock(videoConditionMutex);
                    videoThreadCondition.wait(lock);
                    if (!isDecoding) break;
                }
                if (!isDecoding) break;
                sourceData = videoQueue.front();
                while (!videoQueueMutex.try_lock());
                videoQueue.pop();
                videoQueueMutex.unlock();
                _asynDecoder->decodeVideo(sourceData);
            }
        });
    }
    
    template <typename T>
    thread MSPlayer<T>::initAsynDataAudioDecodeThread() {
        return thread([this](){
            const MSMedia<MSEncodeMedia> *sourceData = nullptr;
            while (isDecoding) {
                while (audioQueue.empty() || sampleQueue.size() > MaxSampleBufferSize) {
                    unique_lock<mutex> lock(audioConditionMutex);
                    audioThreadCondition.wait(lock);
                    if (!isDecoding) break;
                }
                if (!isDecoding) break;
                sourceData = audioQueue.front();
                while (!audioQueueMutex.try_lock());
                audioQueue.pop();
                audioQueueMutex.unlock();
                _asynDecoder->decodeAudio(sourceData);
            }
        });
    }
    
#ifdef __APPLE__
    template <typename T>
    MSTimer<MSTimerForApple> * MSNonnull
    MSPlayer<T>::initSyncDataVideoTimer() {
        return new MSTimer<MSTimerForApple>(microseconds(0),intervale(1),[this](){
            if (!pixelQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = pixelQueue.front();
                while (!pixelQueueMutex.try_lock());
                pixelQueue.pop();
                pixelQueueMutex.unlock();
                if (pixelQueue.size() < 5) {
                    videoThreadCondition.notify_one();
                }
                videoTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeVideo(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeVideo(*frameData);
                }
                delete frameData;
            } else {
                videoThreadCondition.notify_one();
                this->throwDecodeVideo(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForApple> * MSNonnull
    MSPlayer<T>::initSyncDataAudioTimer() {
        return new MSTimer<MSTimerForApple>(microseconds(0),intervale(1),[this](){
            if (!sampleQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = sampleQueue.front();
                while (!sampleQueueMutex.try_lock());
                sampleQueue.pop();
                sampleQueueMutex.unlock();
                if (sampleQueue.size() < 5) {
                    audioThreadCondition.notify_one();
                }
                audioTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeAudio(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeAudio(*frameData);
                }
                delete frameData;
            } else {
                audioThreadCondition.notify_one();
                this->throwDecodeAudio(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForApple> * MSNonnull
    MSPlayer<T>::initAsynDataVideoTimer() {
        return new MSTimer<MSTimerForApple>(microseconds(0),intervale(1),[this](){
            if (!pixelQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = pixelQueue.front();
                while (!pixelQueueMutex.try_lock());
                pixelQueue.pop();
                pixelQueueMutex.unlock();
                if (pixelQueue.size() < 5) {
                    videoThreadCondition.notify_one();
                }
                videoTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeVideo(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeVideo(*frameData);
                }
                delete frameData;
            } else {
                videoThreadCondition.notify_one();
                this->throwDecodeVideo(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForApple> * MSNonnull
    MSPlayer<T>::initAsynDataAudioTimer() {
        return new MSTimer<MSTimerForApple>(microseconds(0),intervale(1),[this](){
            if (!sampleQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = sampleQueue.front();
                while (!sampleQueueMutex.try_lock());
                sampleQueue.pop();
                sampleQueueMutex.unlock();
                if (sampleQueue.size() < 5) {
                    audioThreadCondition.notify_one();
                }
                audioTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeAudio(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeAudio(*frameData);
                }
                delete frameData;
            } else {
                audioThreadCondition.notify_one();
                this->throwDecodeAudio(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }

#else
    template <typename T>
    MSTimer<MSTimerForOther> * MSNonnull
    MSPlayer<T>::initSyncDataVideoTimer() {
        return new MSTimer<MSTimerForOther>(microseconds(0),intervale(1),[this](){
            if (!pixelQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = pixelQueue.front();
                while (!pixelQueueMutex.try_lock());
                pixelQueue.pop();
                pixelQueueMutex.unlock();
                if (pixelQueue.size() < 5) {
                    videoThreadCondition.notify_one();
                }
                videoTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeVideo(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeVideo(*frameData);
                }
                delete frameData;
            } else {
                videoThreadCondition.notify_one();
                this->throwDecodeVideo(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForOther> * MSNonnull
    MSPlayer<T>::initSyncDataAudioTimer() {
        return new MSTimer<MSTimerForOther>(microseconds(0),intervale(1),[this](){
            if (!sampleQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = sampleQueue.front();
                while (!sampleQueueMutex.try_lock());
                sampleQueue.pop();
                sampleQueueMutex.unlock();
                if (sampleQueue.size() < 5) {
                    audioThreadCondition.notify_one();
                }
                audioTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeAudio(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeAudio(*frameData);
                }
                delete frameData;
            } else {
                audioThreadCondition.notify_one();
                this->throwDecodeAudio(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForOther> * MSNonnull
    MSPlayer<T>::initAsynDataVideoTimer() {
        return new MSTimer<MSTimerForOther>(microseconds(0),intervale(1),[this](){
            if (!pixelQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = pixelQueue.front();
                while (!pixelQueueMutex.try_lock());
                pixelQueue.pop();
                pixelQueueMutex.unlock();
                if (pixelQueue.size() < 5) {
                    videoThreadCondition.notify_one();
                }
                videoTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeVideo(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeVideo(*frameData);
                }
                delete frameData;
            } else {
                videoThreadCondition.notify_one();
                this->throwDecodeVideo(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
    
    template <typename T>
    MSTimer<MSTimerForOther> * MSNonnull
    MSPlayer<T>::initAsynDataAudioTimer() {
        return new MSTimer<MSTimerForOther>(microseconds(0),intervale(1),[this](){
            if (!sampleQueue.empty()) {
                const MSMedia<MSDecodeMedia,T> *frameData = nullptr;
                frameData = sampleQueue.front();
                while (!sampleQueueMutex.try_lock());
                sampleQueue.pop();
                sampleQueueMutex.unlock();
                if (sampleQueue.size() < 5) {
                    audioThreadCondition.notify_one();
                }
                audioTimer->updateTimeInterval(frameData->timeInterval);
                this->throwDecodeAudio(*frameData);
                if (isEncoding) {
                    _reEncoder->encodeAudio(*frameData);
                }
                delete frameData;
            } else {
                audioThreadCondition.notify_one();
                this->throwDecodeAudio(MSMedia<MSDecodeMedia,T>::defaultNullMedia);
            }
        });
    }
#endif
    
    
    template <typename T>
    void MSPlayer<T>::asynPushVideoFrameData(const MSMedia<MSDecodeMedia, T> * MSNonnull const frameData) {
        while (!pixelQueueMutex.try_lock());
        pixelQueue.push(frameData);
        pixelQueueMutex.unlock();
    }
    
    template <typename T>
    void MSPlayer<T>::asynPushAudioFrameData(const MSMedia<MSDecodeMedia, T> * MSNonnull const frameData) {
        while (!sampleQueueMutex.try_lock());
        sampleQueue.push(frameData);
        sampleQueueMutex.unlock();
    }
    
}

#endif /* MSPlayer_hpp */
