/*
 * AudioFDK.h
 *
 *  Created on: Mar 24, 2015
 *      Author: Mark
 */

#ifndef AUDIOFDK_H_
#define AUDIOFDK_H_

#include <jni.h>
#include <androidVideoShim.h>
#include <semaphore.h>
#include <RefCounted.h>
#include <AudioPlayer.h>
#include <aacdecoder_lib.h>
#include <list>

class AudioFDK: public AudioPlayer {
public:
	AudioFDK(JavaVM* jvm);
	virtual ~AudioFDK();

	bool Init();
	void Close();

	virtual void unload(); // from RefCounted

	bool Start();
	void Play();
	void Pause();
	void Flush();
	bool Stop(bool seeking = false);

	bool Set(android_video_shim::sp<android_video_shim::MediaSource> audioSource, bool alreadyStarted = false);
	bool Set23(android_video_shim::sp<android_video_shim::MediaSource23> audioSource, bool alreadyStarted = false);
	void ClearAudioSource();

	int Update();

	int64_t GetTimeStamp();

	void forceTimeStampUpdate();

	int getBufferSize();

	bool UpdateFormatInfo();

	bool ReadUntilTime(double timeSecs);

	int GetState();

private:

	void SetState(int state, const char* func = "");


	/*
	 * TargetState, and it's associated methods (pushTargetSTate, targetStateCount, and popTargetState),
	 * are used to queue up actions that will be handled in the Update method when the audio thread
	 * makes the next call to it. This is used to synchronize these actions so that they don't interrupt
	 * the decoding of frames and the interaction with the java audio track.
	 *
	 */
	struct TargetState
	{
		int state; // The state you want to go to
		int data; // Associated data (if you need to encode some flags or other information)
	};
	std::list<TargetState> mTargetStates;

	void pushTargetState(int state, int data); // Push a target state onto the state queue
	int targetStateCount(); // Get the number of states on the queue
	TargetState popTargetState(); // Retrieve the first state on the queue



	bool doStop(int data);

	void SetTimeStampOffset(double offsetSecs);

	bool InitJavaTrack();

	HANDLE_AACDECODER mAACDecoder;

	uint32_t mESDSType;
	const void* mESDSData;
	size_t mESDSSize;

	jclass mCAudioTrack;
	jmethodID mAudioTrack;
	jmethodID mGetMinBufferSize;
	jmethodID mPlay;
	jmethodID mPause;
	jmethodID mStop;
	jmethodID mRelease;
	jmethodID mGetTimestamp;
	jmethodID mWrite;
	jmethodID mFlush;
	jmethodID mSetPositionNotificationPeriod;
	jmethodID mGetPlaybackHeadPosition;

	jobject mTrack;
	jarray buffer;

	JavaVM* mJvm;

	android_video_shim::sp<android_video_shim::MediaSource> mAudioSource;
	android_video_shim::sp<android_video_shim::MediaSource23> mAudioSource23;

	int mSampleRate;
	int mNumChannels;
	int mChannelMask;
	int mBufferSizeInBytes;

	int mPlayState;
	bool mWaiting;
	bool mPlayingSilence;

	double mTimeStampOffset;
	bool mNeedsTimeStampOffset;

	long long samplesWritten;

	sem_t semPause;
	pthread_mutex_t updateMutex;
	pthread_mutex_t lock;
	pthread_mutex_t stateQueueLock;

};

#endif /* AUDIOFDK_H_ */
