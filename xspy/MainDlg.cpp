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

// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include "atlmisc.h"
#include "utils.h"
#include <string>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <atlstr.h>
#include "../xspydll/xspydll.h"

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();
    DlgResize_Init();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);
    // UIAddChildWindowContainer(m_hWnd); // for update

	// init
	ui_capture_.SubclassWindow(GetDlgItem(IDC_STATIC1));

    {
        GetDlgItem(IDC_EDIT2).EnableWindow(FALSE);
        CButton b(GetDlgItem(IDC_CHECK1));
        b.SetCheck(BST_UNCHECKED);
    }

    ui_capture_.AddRecvWnd(m_hWnd); // ��������ͳһ����

    {
        CButton b(GetDlgItem(IDC_CHECK2));
        b.SetCheck(BST_CHECKED);
        hideXspyWhenCapture_ = true;
    }

#ifdef _M_X64
    ATL::CString Title;
    GetWindowText(Title);
    Title += TEXT(" (x64 version)");
    SetWindowText(Title);
#endif

    LPCWSTR lpStrCmdLine=GetCommandLineW();
    int NumArgs=0;
    LPWSTR* szArglist=CommandLineToArgvW(lpStrCmdLine, &NumArgs);
    if (szArglist)
    {
        if (NumArgs>1)
        {
            HWND hWnd;
#ifdef _WIN64 // ʹ��StrToInt64Ex����Ҫ#define _WIN32_IE	0x0600
            StrToInt64ExW(szArglist[1], STIF_SUPPORT_HEX, (LONGLONG *)&hWnd);
#else
            StrToIntExW(szArglist[1], STIF_SUPPORT_HEX, (int*)&hWnd);
#endif
            if (::IsWindow(hWnd))
            {
                PostMessage(WM_SPY,(WPARAM)hWnd,NULL);
            }
        }
        LocalFree(szArglist);
    }
	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);
    return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ATL::CString text;
    GetDlgItem(IDC_EDIT1).GetWindowText(text);
    if (!text.IsEmpty())
    {
        if (text.GetLength() >= 2)
        {
            if (!(text.GetAt(0) == _T('0')
                && text.GetAt(1) == _T('x')))
            {
                text.Insert(0, _T("0x"));
            }
        }
        else
            text.Insert(0, _T("0x"));
        HWND hWnd;
#ifdef _WIN64 // ʹ��StrToInt64Ex����Ҫ#define _WIN32_IE	0x0600
        StrToInt64Ex(text, STIF_SUPPORT_HEX, (LONGLONG *)&hWnd);
#else
        StrToIntEx(text, STIF_SUPPORT_HEX, (int*)&hWnd);
#endif
        BOOL b;
        OnSpy(0, (WPARAM)hWnd, 0, b);
    }
    return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CloseDialog(wID);
	return 0;
}


LRESULT CMainDlg::OnSpy( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
    ShowWindow(SW_SHOW);

    HWND hWnd = (HWND)wParam;
    if(hWnd==NULL || !::IsWindow(hWnd)){MessageBeep(0);return 0;}

    if (hWnd == GetDlgItem(IDC_STATIC1).m_hWnd)
        return 0;


    ATL::CString str;
    str.Format(_T("%08X"), hWnd);
    TCHAR cn[1024];
    if(GetClassName(hWnd, cn, sizeof(cn)))
    {
        str += "(";
        str += cn;
        str += ")";
    }
    if( ::GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CHILD )
    {
        ATL::CString strTemp;
        HMENU menu = ::GetMenu(hWnd);
        strTemp.Format(_T(",id=%04x"), menu);
        str += strTemp;
    }
    
    GetDlgItem(IDC_EDIT1).SetWindowText(str);
    GetDlgItem(IDC_EDIT_MSG).SetWindowText(_T(""));

    // spy
    arg_struct arg;
    arg.hWnd = hWnd;
    arg.mfc_dll_name[0] = 0;
    CButton b(GetDlgItem(IDC_CHECK1));
    if(BST_CHECKED == b.GetCheck())
    {
        ATL::CString str;
        GetDlgItem(IDC_EDIT2).GetWindowText(str);
        if (!str.IsEmpty())
        {
#ifdef _UNICODE
            std::string str1 = ws2s((LPCTSTR)str);
            strncpy_s(arg.mfc_dll_name, MAX_PATH, str1.c_str(), sizeof(arg.mfc_dll_name) - 1);
#else
            strncpy_s(arg.mfc_dll_name, MAX_PATH, str, sizeof(arg.mfc_dll_name) - 1);
#endif
        }
    }
    boost::shared_ptr<result_struct> result;
    result.reset(xspydll_Spy(&arg), xspydll_SpyFree);
    if (result)
    {
        std::tstring strResult;
        strResult = _T("Tip: Double click to select address��right mouse immediately copy it\r\n");

#ifdef _UNICODE
        strResult += s2ws(result->retMsg);
#else
        strResult += result->retMsg;
#endif
        GetDlgItem(IDC_EDIT_MSG).SetWindowText(strResult.c_str());

    }
    else
    {
        typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
        static LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
            GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

        if(NULL != fnIsWow64Process)
        {
            DWORD Pid;
            GetWindowThreadProcessId(hWnd, &Pid);
            boost::shared_ptr<void> Process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid)
                , CloseHandle);
            if (Process != NULL)
            {
                BOOL bIsWow64;
                TCHAR szExePath[MAX_PATH];
                GetModuleFileName(NULL, szExePath, MAX_PATH);
                TCHAR* p = StrRChr(szExePath, 0, TEXT('\\'));
                *p = 0;
                
#ifdef _M_IX86
                // ���32�汾�Ƿ�������64λϵͳ��
                if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
                {
                    if (bIsWow64)
                    {
                        if (fnIsWow64Process(Process.get(),&bIsWow64))
                        {
                            // 64λ������64λϵͳ��������FALSE
                            if (!bIsWow64)
                            {
                                if(IDYES==MessageBox(TEXT("Target process is 64-bit, do you want to switch to 64-bit version of xspy to detect it?")
                                    , TEXT("Note")
                                    , MB_YESNO | MB_ICONINFORMATION))
                                {
                                    ShowWindow(SW_HIDE);
                                    TCHAR StrhWnd[64];
                                    _itot_s((int)hWnd, StrhWnd, 10);
                                    lstrcat(szExePath, _T("\\xspy-x64.exe"));
                                    ShellExecute(NULL, _T("open"), szExePath, StrhWnd, NULL, SW_SHOW);

                                    PostQuitMessage(0);
                                }
                                return 0;
                            }
                        }
                    }
                }
#endif

#ifdef _M_X64
                if (fnIsWow64Process(Process.get(),&bIsWow64))
                {
                    if (bIsWow64)
                    {
                        if(IDYES==MessageBox(TEXT("Target process is 32 bit, do you want to switch to 32-bit version of xspy to detect it?")
                            , TEXT("Note")
                            , MB_YESNO | MB_ICONINFORMATION))
                        {
                            ShowWindow(SW_HIDE);
                            TCHAR StrhWnd[64];
                            _i64tot((__int64)hWnd, StrhWnd, 10);
                            lstrcat(szExePath, _T("\\xspy.exe"));
                            ShellExecute(NULL, _T("open"), szExePath, StrhWnd, NULL, SW_SHOW);
                            PostQuitMessage(0);
                        }
                        return 0;
                    }
                }
#endif

            }
        }
    }

    return 0;
}

BOOL CMainDlg::PreTranslateMessage( MSG* pMsg )
{
    if(pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_RETURN)
        {
            HWND hWnd= ::GetFocus();
            if (IDC_EDIT1 == ::GetDlgCtrlID(hWnd))
            {
                BOOL b;
                OnOK(0, 0, 0, b);
                return FALSE;
            }
        }
    }

    // ʵ��edit����windbg�Ĺ���        
    if (pMsg->message == WM_RBUTTONDOWN)
    {
        TCHAR szClassName[MAX_PATH];
        HWND hFocus = GetFocus();
        GetClassName(hFocus, szClassName, MAX_PATH);
        if (0 == lstrcmpi(szClassName, TEXT("Edit")))
        {
            DWORD dwP, dwL;
            ::SendMessage(hFocus, EM_GETSEL, (WPARAM)&dwP, (LPARAM)&dwL);
            // _cprintf("p:%d, l:%d", dwP, dwL);

            DWORD dwSelected = dwL - dwP;
            // ����
            if (dwSelected)
            {
                ::SendMessage(hFocus, WM_COPY, 0, 0);
                // ���ƺ�ȥ��ѡ��״̬����λ���׸����Ƶ�
                ::SendMessage(hFocus, EM_SETSEL, (WPARAM)dwP, (LPARAM)dwP); 
            }
            // ճ��
            else
            {
                ::SendMessage(hFocus, WM_PASTE, 0, 0);
            }
            return TRUE; // ���ٵ����Ҽ��˵�
        }
    }

    return CWindow::IsDialogMessage(pMsg);
}

void CMainDlg::CloseDialog( int nVal )
{
    DestroyWindow();
    ::PostQuitMessage(nVal);
}

BOOL CMainDlg::OnIdle()
{
    return FALSE;
}

LRESULT CMainDlg::OnBnClickedCheck1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CButton b;
    b.Attach(GetDlgItem(IDC_CHECK1));
    if(BST_CHECKED == b.GetCheck())
    {
        GetDlgItem(IDC_EDIT2).EnableWindow(TRUE);
    }
    else
    {
        GetDlgItem(IDC_EDIT2).EnableWindow(FALSE);
    }
    return 0;
}

LRESULT CMainDlg::OnSpyStart( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
    {
        CButton b(GetDlgItem(IDC_CHECK2));
        hideXspyWhenCapture_ = (BST_CHECKED == b.GetCheck());
    }

    if(hideXspyWhenCapture_)
        ShowWindow(SW_HIDE);
    return 0;
}

