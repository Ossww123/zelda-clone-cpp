#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <map>
#include <queue>
using namespace std;
#include "ThreadManager.h"

void TestThread()
{
    cout << " i am thread " << LThreadId << endl;

    while (true)
    {

    }
}

int main()
{
    for (int i = 0; i < 10; i++)
        GThreadManager->Launch(TestThread);

    GThreadManager->Join();
}