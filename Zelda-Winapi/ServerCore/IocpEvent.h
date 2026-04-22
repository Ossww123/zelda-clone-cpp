#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

/*--------------
	IocpEvent
---------------*/

struct IocpEvent : public OVERLAPPED
{
	// virtual 금지 : 시작 주소에 vptr이 끼어들기 때문

	IocpEvent(EventType type);

	void		Init();

	EventType		type;
	IocpObjectRef	owner = nullptr;
	SessionRef		session = nullptr; // Accept Only

	vector<SendBufferRef> sendBuffers; // shared_ptr 참조 카운트 유지
};