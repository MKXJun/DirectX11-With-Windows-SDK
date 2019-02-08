//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;		// 总游戏时间
	float DeltaTime()const;		// 帧间隔时间

	void Reset(); // 在消息循环之前调用
	void Start(); // 在取消暂停的时候调用
	void Stop();  // 在暂停的时候调用
	void Tick();  // 在每一帧的时候调用

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H