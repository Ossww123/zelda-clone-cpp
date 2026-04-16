#include "pch.h"
#include "ChatPanel.h"
#include "Client.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"
#include "ResourceManager.h"
#include "Sprite.h"

ChatPanel::ChatPanel ( )
{
    SetSize ( { PANEL_W, PANEL_H } );
    SetVisible ( false );
}

ChatPanel::~ChatPanel ( )
{
    if ( _hEdit )
    {
        ::DestroyWindow ( _hEdit );
        _hEdit = nullptr;
    }
}

void ChatPanel::Init ( HWND parentHwnd )
{
    _hEdit = ::CreateWindowEx (
        0 , L"EDIT" , L"" ,
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL ,
        0 , 0 , PANEL_W - 2 , INPUT_H ,
        parentHwnd , nullptr ,
        ::GetModuleHandle ( nullptr ) , nullptr
    );

    HFONT hFont = ::CreateFont (
        16 , 0 , 0 , 0 , FW_NORMAL ,
        FALSE , FALSE , FALSE ,
        HANGUL_CHARSET ,
        OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
        DEFAULT_QUALITY , DEFAULT_PITCH ,
        L"맑은 고딕"
    );
    ::SendMessage ( _hEdit , WM_SETFONT , ( WPARAM ) hFont , TRUE );

    SetPos ( { ( float ) ( GWinSizeX - PANEL_W / 2 - 10 ), ( float ) ( GWinSizeY - PANEL_H / 2 - 10 ) } );
    UpdateEditControlPos ( );
}

void ChatPanel::Tick ( )
{
    if ( !IsVisible ( ) )
        return;

    Super::Tick ( );

    if ( _justOpened )
    {
        _justOpened = false;
        return;
    }

    // Enter 키 → 채팅 전송
    if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_RETURN ) ) )
    {
        if ( _hEdit && ::GetFocus ( ) == _hEdit )
            SendChat ( );
    }
}

void ChatPanel::Render ( HDC hdc )
{
    if ( !IsVisible ( ) )
        return;

    RECT rect = GetRect ( );

    // 배경
    RECT logRect = { rect.left, rect.top, rect.right, rect.bottom - INPUT_H - 2 };
    Sprite* bg = GET_SINGLE ( ResourceManager )->GetSprite ( L"ChatPanel" );
    if ( bg )
    {
        ::TransparentBlt ( hdc,
            rect.left, rect.top,
            PANEL_W, PANEL_H,
            bg->GetDC ( ),
            bg->GetPos ( ).x, bg->GetPos ( ).y,
            bg->GetSize ( ).x, bg->GetSize ( ).y,
            bg->GetTransparent ( ) );
    }
    else
    {
        HBRUSH bgBrush = ::CreateSolidBrush ( RGB ( 20, 20, 20 ) );
        ::FillRect ( hdc, &logRect, bgBrush );
        ::DeleteObject ( bgBrush );
    }

    // 채팅 메시지 출력
    ::SetBkMode ( hdc , TRANSPARENT );
    ::SetTextColor ( hdc , RGB ( 255 , 255 , 255 ) );

    HFONT hFont = ::CreateFont (
        14 , 0 , 0 , 0 , FW_NORMAL ,
        FALSE , FALSE , FALSE ,
        HANGUL_CHARSET ,
        OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
        DEFAULT_QUALITY , DEFAULT_PITCH ,
        L"맑은 고딕"
    );
    HFONT hOldFont = ( HFONT )::SelectObject ( hdc , hFont );

    int32 lineH = 18;
    int32 startY = logRect.bottom - lineH;

    for ( int32 i = ( int32 ) _messages.size ( ) - 1; i >= 0; i-- )
    {
        if ( startY < logRect.top )
            break;

        wstring line = _messages[ i ].sender + L": " + _messages[ i ].msg;
        RECT lineRect = { rect.left + 10, startY, rect.right - 10, startY + lineH };
        ::DrawText ( hdc , line.c_str ( ) , ( int32 ) line.length ( ) , &lineRect , DT_LEFT | DT_SINGLELINE | DT_VCENTER |
DT_END_ELLIPSIS );

        startY -= lineH;
    }

    ::SelectObject ( hdc , hOldFont );
    ::DeleteObject ( hFont );

    // UpdateEditControlPos ( );
}

void ChatPanel::AddMessage ( const wstring& sender , const wstring& msg )
{
    _messages.push_back ( { sender, msg } );
    if ( ( int32 ) _messages.size ( ) > MAX_MESSAGES )
        _messages.pop_front ( );
}

void ChatPanel::SetVisible ( bool visible )
{
    Super::SetVisible ( visible );
    if ( _hEdit )
        ::ShowWindow ( _hEdit , visible ? SW_SHOW : SW_HIDE );
    if ( visible )
        _justOpened = true;
}

void ChatPanel::SendChat ( )
{
    if ( !_hEdit )
        return;

    wchar_t buf[ 256 ] = {};
    ::GetWindowText ( _hEdit , buf , 256 );

    wstring text ( buf );
    if ( text.empty ( ) )
        return;

    // UTF-16 → UTF-8 변환
    int32 size = ::WideCharToMultiByte ( CP_UTF8 , 0 , text.c_str ( ) , -1 , nullptr , 0 , nullptr , nullptr );
    string msg ( size - 1 , 0 );
    ::WideCharToMultiByte ( CP_UTF8 , 0 , text.c_str ( ) , -1 , &msg[ 0 ] , size , nullptr , nullptr );

    Protocol::CHAT_TYPE chatType = Protocol::CHAT_TYPE_GLOBAL;
    string target = "";

    if ( msg.size ( ) > 3 && msg.substr ( 0 , 3 ) == "/p " )
    {
        chatType = Protocol::CHAT_TYPE_PARTY;
        msg = msg.substr ( 3 );
    }
    else if ( msg.size ( ) > 3 && msg.substr ( 0 , 3 ) == "/w " )
    {
        chatType = Protocol::CHAT_TYPE_WHISPER;
        size_t space = msg.find ( ' ' , 3 );
        if ( space == string::npos )
            return;  // "/w 이름" 만 있고 메시지 없는 경우
        target = msg.substr ( 3 , space - 3 );
        msg = msg.substr ( space + 1 );
    }

    Protocol::C_Chat pkt;
    pkt.set_type ( chatType );
    pkt.set_msg ( msg );
    pkt.set_target ( target );

    SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Chat ( pkt );
    GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );

    if ( chatType == Protocol::CHAT_TYPE_WHISPER )
    {
        auto toWstr = []( const string& s ) -> wstring {
            int32 len = ::MultiByteToWideChar ( CP_UTF8, 0, s.c_str(), -1, nullptr, 0 );
            wstring ws( len - 1, 0 );
            ::MultiByteToWideChar ( CP_UTF8, 0, s.c_str(), -1, &ws[0], len );
            return ws;
        };
        wstring label = L"[귓말->" + toWstr ( target ) + L"]";
        AddMessage ( label, toWstr ( msg ) );
    }

    ::SetWindowText ( _hEdit , L"" );
}

void ChatPanel::UpdateEditControlPos ( )
{
    if ( !_hEdit )
        return;

    RECT rect = GetRect ( );
    ::MoveWindow (
        _hEdit ,
        rect.left , rect.bottom - INPUT_H - 2 ,
        rect.right - rect.left , INPUT_H ,
        TRUE
    );

    if ( IsVisible ( ) )
        ::ShowWindow ( _hEdit , SW_SHOW );
    else
        ::ShowWindow ( _hEdit , SW_HIDE );
}
