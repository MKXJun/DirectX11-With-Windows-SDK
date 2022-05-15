//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: m_SecondsPerCount(0.0), m_DeltaTime(-1.0), m_BaseTime(0), m_StopTime(0),
  m_PausedTime(0), m_PrevTime(0), m_CurrTime(0), m_Stopped(false)
{
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    m_SecondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
float GameTimer::TotalTime()const
{
    // If we are stopped, do not count the time that has passed since we stopped.
    // Moreover, if we previously already had a pause, the distance 
    // m_StopTime - m_BaseTime includes paused time, which we do not want to count.
    // To correct this, we can subtract the paused time from m_StopTime:  
    //
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------------*------> time
    //  m_BaseTime       m_StopTime        startTime     m_StopTime    m_CurrTime

    if( m_Stopped )
    {
        return (float)(((m_StopTime - m_PausedTime)-m_BaseTime)*m_SecondsPerCount);
    }

    // The distance m_CurrTime - m_BaseTime includes paused time,
    // which we do not want to count.  To correct this, we can subtract 
    // the paused time from m_CurrTime:  
    //
    //  (m_CurrTime - m_PausedTime) - m_BaseTime 
    //
    //                     |<--paused time-->|
    // ----*---------------*-----------------*------------*------> time
    //  m_BaseTime       m_StopTime        startTime     m_CurrTime
    
    else
    {
        return (float)(((m_CurrTime-m_PausedTime)-m_BaseTime)*m_SecondsPerCount);
    }
}

float GameTimer::DeltaTime()const
{
    return (float)m_DeltaTime;
}

void GameTimer::Reset()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    m_BaseTime = currTime;
    m_PrevTime = currTime;
    m_StopTime = 0;
    m_PausedTime = 0;	// 涉及到多次Reset的话需要将其归0
    m_Stopped  = false;
}

void GameTimer::Start()
{
    __int64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


    // Accumulate the time elapsed between stop and start pairs.
    //
    //                     |<-------d------->|
    // ----*---------------*-----------------*------------> time
    //  m_BaseTime       m_StopTime        startTime     

    if( m_Stopped )
    {
        m_PausedTime += (startTime - m_StopTime);	

        m_PrevTime = startTime;
        m_StopTime = 0;
        m_Stopped  = false;
    }
}

void GameTimer::Stop()
{
    if( !m_Stopped )
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        m_StopTime = currTime;
        m_Stopped  = true;
    }
}

void GameTimer::Tick()
{
    if( m_Stopped )
    {
        m_DeltaTime = 0.0;
        return;
    }

    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    m_CurrTime = currTime;

    // Time difference between this frame and the previous.
    m_DeltaTime = (m_CurrTime - m_PrevTime)*m_SecondsPerCount;

    // Prepare for next frame.
    m_PrevTime = m_CurrTime;

    if(m_DeltaTime < 0.0)
    {
        m_DeltaTime = 0.0;
    }
}

