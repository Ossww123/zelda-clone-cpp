#pragma once

class InventoryPanel;
class PartyPanel;
class PartyInvitePanel;
class ChatPanel;
class RankingPanel;

class UIManager
{
public:
	void Init ( HWND hWnd );
	void Tick ( );
	void Render ( HDC hdc );

	bool IsInputConsumed ( ) const { return _inputConsumed; }

	void AddChatMessage ( const wstring& sender , const wstring& msg );
	void SetRankingData ( const vector<pair<wstring, int32>>& entries );

private:
	void TickPanels ( );
	void TickUIInput ( );
	void RenderHUD ( HDC hdc );

private:
	InventoryPanel* _inventoryPanel = nullptr;
	PartyPanel* _partyPanel = nullptr;
	PartyInvitePanel* _partyInvitePanel = nullptr;
	ChatPanel* _chatPanel = nullptr;
	RankingPanel* _rankingPanel = nullptr;

	bool _inputConsumed = false;
};
