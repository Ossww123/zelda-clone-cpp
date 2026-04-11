#include "pch.h"
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "Service.h"
#include "ChatSession.h"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SocketUtils::Init();

    ServerServiceRef service = make_shared<ServerService>(
        NetAddress(L"127.0.0.1", 8888),
        make_shared<IocpCore>(),
        []() { return make_shared<ChatSession>(); },
        10
    );

    assert(service->Start());

    cout << "[ChatServer] Started on port 8888" << endl;

    const int32 workerCount = max(1, (int32)thread::hardware_concurrency());

    for (int32 i = 0; i < workerCount; ++i)
    {
        GThreadManager->Launch([service]()
            {
                while (true)
                {
                    service->GetIocpCore()->Dispatch(10);
                }
            });
    }

    GThreadManager->Join();

    SocketUtils::Clear();
}