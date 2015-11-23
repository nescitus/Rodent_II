#pragma once

enum eTimeData { W_TIME, B_TIME, W_INC, B_INC, TIME, INC, MOVES_TO_GO, MOVE_TIME, 
               MAX_NODES, MAX_DEPTH, FLAG_INFINITE, SIZE_OF_DATA };

struct sTimer {
private:
    int data[SIZE_OF_DATA]; // various data used to set actual time per move (see eTimeData)
    int start_time;         // when we have begun searching
    int iteration_time;     // when we are allowed to start new iteration
    int allocated_time;     // basic time allocated for a move
  int BulletCorrection(int time);
public:
  int nps_limit;
  int slow_play;
    void Clear(void);
    void SetStartTime();
    void SetMoveTiming(void);
    void SetIterationTiming(void);
    int FinishIteration(void);
    int GetMS(void);
    int GetElapsedTime(void);
    int IsInfiniteMode(void);
    int TimeHasElapsed(void);
  void Init(void);
  void WasteTime(int miliseconds);
    int GetData(int slot);
    void SetData(int slot, int val);
    void SetSideData(int side);
};

extern sTimer Timer;
