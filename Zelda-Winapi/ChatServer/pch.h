#pragma once
#include "CorePch.h"                                  t

using ChatSessionRef = shared_ptr<class ChatSession>;

#include "Enum.pb.h"
#include "Protocol.pb.h"
#include "Struct.pb.h"

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#include "ChatPacketHandler.h"
#include "ChatServerLogger.h"