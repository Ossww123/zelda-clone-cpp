#pragma once
#include "Panel.h"

class ChatPanel : public Panel
{
    using Super = Panel;

public:
    ChatPanel ( );
    virtual ~ChatPanel ( ) override;

    void Init ( HWND parentHwnd );

    virtual void Tick ( ) override;
    virtual void Render ( HDC hdc ) override;

    HWND GetEditHandle ( ) const { return _hEdit; }

    void AddMessage ( const wstring& sender , const wstring& msg );

    virtual void SetVisible ( bool visible ) override;

private:
    void SendChat ( );
    void UpdateEditControlPos ( );

private:
    HWND _hEdit = nullptr;

    struct ChatMessage
    {
        wstring sender;
        wstring msg;
    };

    static const int32 MAX_MESSAGES = 10;
    static const int32 PANEL_W = 300;
    static const int32 PANEL_H = 200;
    static const int32 INPUT_H = 24;

    deque<ChatMessage> _messages;

    bool _justOpened = false;
};
