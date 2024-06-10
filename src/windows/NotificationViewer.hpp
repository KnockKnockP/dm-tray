#pragma once

#include <windows.h>
#include "../discord/Snowflake.hpp"

class MessageList;

class NotificationViewer
{
public:
	static void Show(int x, int y, bool rightJustify = true);
	static bool IsActive();

private:
	static void Initialize(HWND hWnd);
	static void OnClickMessage(Snowflake sf);
	static BOOL CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static POINT m_appearXY;
	static bool m_bRightJustify;
	static bool m_bActive;
	static MessageList* m_pMessageList;
};

