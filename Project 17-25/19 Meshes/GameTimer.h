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

    void Reset();               // 在消息循环之前调用
    void Start();               // 在取消暂停的时候调用
    void Stop();                // 在暂停的时候调用
    void Tick();                // 在每一帧的时候调用

private:
    double m_SecondsPerCount;
    double m_DeltaTime;

    __int64 m_BaseTime;
    __int64 m_PausedTime;
    __int64 m_StopTime;
    __int64 m_PrevTime;
    __int64 m_CurrTime;

    bool m_Stopped;
};

#endif // GAMETIMER_H