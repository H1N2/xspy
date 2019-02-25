/* 
 *
 * This file is part of xspy
 * By lynnux <lynnux@qq.com>
 * Copyright 2013 lynnux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#pragma once
#include "atlmisc.h"
#include "common.h"
#include "utils.h"
#include <set>

// ģ���wtl::CBitmapButtonImpl����Ϊ����ҪSubclassWindow����Attach�ǲ��еģ���Ϊ��BEGIN_MSG_MAP
// ��Ҫ���볭Ϯ��wtl::CZoomScrollImpl��mfcspy
template <class T, class TBase = CStatic, class TWinTraits = ATL::CControlWinTraits>
class ATL_NO_VTABLE ui_capture_t : public ATL::CWindowImpl< T, TBase, TWinTraits>
{
public:
    //// API 
    // ����һ��STATIC�ؼ�Ϊ��׽����
    BOOL SubclassWindow(HWND hWnd);
    // ��ӽ���WM_SPY��Ϣ�Ĵ���
    void AddRecvWnd(HWND hWnd);
    // �Ƴ�����WM_SPY��Ϣ�Ĵ���
    void RemoveRecvWnd(HWND hWnd);
public:
    ui_capture_t();
    ~ui_capture_t();

protected:
    BEGIN_MSG_MAP(ui_capture_t)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
	END_MSG_MAP()

    void Init();
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
    LRESULT OnCaptureChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    VOID FrameWindow(HWND hWnd);
    void DrawFrame(POINT pt);

private:
	HICON m_hIcon;
    bool m_bTracking;
    HWND m_hRecvMsgWnd;
    HWND m_hLastWnd;
    CPen m_pen;
    HCURSOR cursor_sys;
    std::set<HWND> m_recvWnd;
};

#include "ui_capture_impl.h"

class ui_capture : public ui_capture_t<ui_capture>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_BitmapButton"), GetWndClassName())
};