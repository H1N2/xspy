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

#include "stdafx.h"
#include <boost/format.hpp>
#include "common.h"
#include "mfc.h"
#include "mfc_class.h"
#include "mfc_msg.h"

extern char g_dllname[MAX_PATH];

static bool g_Isdbg  = false;
static bool g_IsStatic = false; // mfc����ͬʱ���Ӷ�̬�������Ӿ�̬
unsigned long g_mfcver = 0;
static bool g_IsUnicode = false;

// ֻ�о�̬����release�汾��û��AssertValid��Dump�����麯��
bool IsStaticRelease()
{
    return g_IsStatic && !g_Isdbg;
}

//kmp�����㷨
void kmp_init(const unsigned char *patn, int len, int *next)
{
    int i, j;

    /*	assert(patn != NULL && len > 0 && next != NULL);
    */
    next[0] = 0;
    for (i = 1, j = 0; i < len; i++) {
        while (j > 0 && patn[j] != patn[i])
            j = next[j - 1];
        if (patn[j] == patn[i])
            j ++;
        next[i] = j;
    }
}

int kmp_find(const unsigned char *text, int text_len, const unsigned char *patn,
             int patn_len, int *next)
{
    int i, j;

    /*	assert(text != NULL && text_len > 0 && patn != NULL && patn_len > 0
    && next != NULL);
    */
    for (i = 0, j = 0; i < text_len; i ++ ) {
        while (j > 0 && text[i] != patn[j])
            j = next[j - 1];
        if (text[i] == patn[j])
            j ++;
        if (j == patn_len)
            return (i + 1 - patn_len);
    }

    return -1;//û���ҵ�
}

// ��AfxWndProc�����������ȷ��CWnd::FromHandlePermanent��ַ
// �Ƽ�ʹ��x64dbg����mfc dll�ķ�����Ϣ���۲�
#ifndef _WIN64
// cmp dword ptr [ebp+0Ch],360h
const unsigned char magic_code[] = {
    0x81, 0x7D, 0x0C, 0x60, 0x03, 0x00, 0x00
};

// mfc42.dll����������ֹEDI ? EAX, ECX, EBX... �ּ�����cmp dword ptr [ebp+0Ch],360h���ε�mfc42.dll��mfc42u.dll
// MOV EDI,DWORD PTR SS:[EBP+0xC]
// CMP EDI,0x360
const unsigned char magic_code2[] = {
    0x8B, 0x7D, 0x0C, 0x81, 0xFF, 0x60, 0x03, 0x00, 0x00
};
#else
// release cmp     edx, 360h
const unsigned char magic_code[] = {
    0x81, 0xFA, 0x60, 0x03, 0x00, 0x00
};
// debug cmp     [rsp+48h+nMsg], 360h
const unsigned char magic_code2[] = {
    0x81, 0x7C, 0x24, 0x58, 0x60, 0x03, 0x00, 0x00
};
#endif

// size_t��64λ�Ͼͱ��64λ��!

extern "C" size_t __stdcall LDE(PVOID  Address, ULONG  x64);

#include <boost/shared_array.hpp>
LPVOID find_FromHandlePermanent(LPVOID start_addr, size_t start_len)
{
    int ret = -1;
    size_t mg_len;
    {
        mg_len = sizeof(magic_code);
        boost::shared_array<int> next(new int[mg_len]);
        kmp_init(magic_code, mg_len, next.get());
        ret = kmp_find((const unsigned char*)start_addr, (int)start_len, magic_code, mg_len, next.get());
    }
    
    if (42 == g_mfcver)
    {
        if (ret!=-1)
        {
            unsigned char* pCode = ((unsigned char*)start_addr + ret + mg_len);
            int CodeLen=start_len-(ret + mg_len);
            boost::shared_array<int> next_second(new int[mg_len]);
            kmp_init(magic_code, mg_len, next_second.get());
            int ret_second=kmp_find(pCode, CodeLen, magic_code, mg_len, next_second.get());
            if (ret_second!=-1)
            {
                //������ڵڶ�����˵����һ����AfxWndProcBase�е�AfxGetModuleThreadState���ã����������������ͱ�����
                ret=(ret + mg_len)+ret_second;
            }
        }
    }

    // �ٳ���
    if (ret == -1)
    {
        mg_len = sizeof(magic_code2);
        boost::shared_array<int> next(new int[mg_len]);
        kmp_init(magic_code2, mg_len, next.get());
        ret = kmp_find((const unsigned char*)start_addr, (int)start_len, magic_code2, mg_len, next.get());
    }

    if (ret == -1)
    {
        return 0;
    }
    
    unsigned char* pStart = ((unsigned char*)start_addr + ret + mg_len);

    for (int i = 0; i< 20; ++i)
    {
        if (*pStart == 0xE8) // call
        {
            //INT_PTR reloc= *(INT_PTR*)(pStart + 1);
            //return (LPVOID)((INT_PTR)pStart + 5 + reloc);
            // 64λ��Ҳ��32λƫ��
            int reloc = *(int*)(pStart + 1);
            return (LPVOID)((INT_PTR)pStart + 5 + reloc);
        }
#ifndef _WIN64
        size_t Length = LDE(pStart, 0);
#else
        size_t Length = LDE(pStart, 64);
#endif
        pStart += Length;
    }

    return 0;
}

struct SECTION_T
{
    LPVOID VirtualAddr;
    DWORD VirtualSize;
};
static std::vector<SECTION_T> GetCodeSection(HMODULE hMod)
{
    std::vector<SECTION_T> ret;

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)hMod;
    IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS*)((BYTE *)hMod + pDosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pNtHeader);
    WORD nSections = pNtHeader->FileHeader.NumberOfSections; // ������section����Ϊ�����ж�
    for (WORD i =  0; pish && (i < nSections); ++i, ++pish)
    {
        if ((pish->Characteristics & (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE) ) ==
            (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE))
        {
            SECTION_T c;
            c.VirtualAddr = (char*)hMod + pish->VirtualAddress;
            c.VirtualSize = pish->Misc.VirtualSize;
            ret.push_back(c);
        }
    }
    return ret;
}

// Ϊ�жϾ�̬��debug����release����
static std::vector<SECTION_T> GetDataSection(HMODULE hMod)
{
    std::vector<SECTION_T> ret;

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)hMod;
    IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS*)((BYTE *)hMod + pDosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pNtHeader);
    WORD nSections = pNtHeader->FileHeader.NumberOfSections;
    for (WORD i =  0; pish && (i < nSections); ++i, ++pish)
    {
        // �ų�.rsrc��
        if (0 == _strnicmp((char*)pish->Name, ".rsrc", 5))
        {
            continue;
        }
        // һ����.rdata��
        if ((pish->Characteristics & (IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ) ) ==
            (IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ))
        {
            SECTION_T c;
            c.VirtualAddr = (char*)hMod + pish->VirtualAddress;
            c.VirtualSize = pish->Misc.VirtualSize;
            ret.push_back(c);
        }
    }
    return ret;
}

#include <Psapi.h>

static CRuntimeClass * GetParentRTC(CRuntimeClass * pr)
{
    //#ifdef _AFXDLL
    if (!g_IsStatic)
    {
        return pr->m_pfnGetBaseClass ? pr->m_pfnGetBaseClass() : 0;
    }
    else
    {
        return (CRuntimeClass *)(pr->m_pfnGetBaseClass);
    }
    //#else
    //    using CODEUTIL::memcmp_withmask;
    //    PBYTE punk = (PBYTE)pr->m_pBaseClass;
    //    if(! IsBadReadPtr(punk,10))
    //    {
    //        if(memcmp_withmask(punk,(PBYTE)"\x55\x8b\xec\xb8\x00\x00\x00\x00\x5d\xc3",10,(PBYTE)"1111000011") == 0 ||
    //            memcmp_withmask(punk,(PBYTE)"\xB8\x00\x00\x00\x00\xC3",6,(PBYTE)"100001") == 0 ||
    //            memcmp_withmask(punk,(PBYTE)"\x55\x8b\xec\x33\xC0\x5d\xc3",7,0) == 0 ||
    //            memcmp_withmask(punk,(PBYTE)"\x33\xC0\xC3",3,0) == 0 )
    //        {
    //            CRuntimeClass* (*pget)();
    //            reinterpret_cast<PBYTE&>(pget) = punk;
    //            return pget();
    //        }
    //    }
    //    return (CRuntimeClass*)punk;
    //#endif
}


static PVOID getfn(const AFX_MSGMAP_ENTRY * pentry)
{
    return *(PVOID**) &(pentry->pfn);
}

#pragma warning(push)
#pragma warning(disable:4996)
#include <boost/xpressive/xpressive.hpp> // ��������������ļ�������ٰ���
bool parse_AfxFrameOrView(const char* str)
{
    using namespace boost::xpressive;
    // ��׼������ʽ^AfxFrameOrView\d+su?d?$
    cregex re = cregex::compile("^AfxFrameOrView(?P<version>\\d+)su?(?P<debug>d?)$");
    cmatch what;
    if( regex_match(str, what, re))
    {
        char* pEnd;
        std::string s = what["version"];
        g_mfcver = strtoul(s.c_str(), &pEnd, 10);
        s = what["debug"];
        g_Isdbg = (s == "d");
        return true;
    }
    return false;
}

bool parse_AfxFrameOrView_u(const wchar_t* str)
{
    using namespace boost::xpressive;
    wcregex re = wcregex::compile(L"^AfxFrameOrView(?P<version>\\d+)su?(?P<debug>d?)$");
    wcmatch what;
    if( regex_match(str, what, re ) )
    {
        wchar_t* pEnd;
        std::wstring s = what[L"version"];
        g_mfcver = wcstoul(s.c_str(), &pEnd, 10);
        s = what[L"debug"];
        g_Isdbg = (s == L"d");
        return true;
    }
    return false;
}

bool parse_mfcdll(const char* str)
{
    using namespace boost::xpressive;
    // ��׼������ʽ^mfc\d+u?d?\.dll$
    cregex re = cregex::compile("^mfc(?P<version>\\d+)u?(?P<debug>d?)\\.dll$", regex_constants::icase);
    cmatch what;
    if( regex_match(str, what, re))
    {
        char* pEnd;
        std::string s = what["version"];
        unsigned long ver = strtoul(s.c_str(), &pEnd, 10);
        if (ver > 40 && ver < 200)
        {
            g_mfcver = ver;
            s = what["debug"];
            g_Isdbg = (s == "d");
            return true;
        }
    }
    return false;
}

#pragma warning(pop)
// ���ر�ʾ�н���������ټ�������
bool check_static(LPVOID start_addr, size_t start_len)
{

    // ����"AfxFrameOrView%_MFC_FILENAME_VER%[s][u][d]"��������Ȼ��kmp�㷨
    char aStr[] = "AfxFrameOrView";
    wchar_t uStr[] = L"AfxFrameOrView";

    // ��unicode������
    int ret = -1;
    size_t mg_len;
    {
        mg_len = sizeof(uStr) - sizeof(wchar_t);
        boost::shared_array<int> next(new int[mg_len]);
        kmp_init((unsigned char*)uStr, mg_len, next.get());
        ret = kmp_find((const unsigned char*)start_addr, (int)start_len, (unsigned char*)uStr, mg_len, next.get());

        if (ret != -1)
        {
            wchar_t *pCheck = (wchar_t*)((unsigned char*)(start_addr) + ret);
            size_t t = wcslen(pCheck);
            if (t < 25)
            {
                if(parse_AfxFrameOrView_u(pCheck))
                {
                    return true;
                }
            }
        }
    }

    {
        mg_len = sizeof(aStr) - sizeof(char);
        boost::shared_array<int> next(new int[mg_len]);
        kmp_init((unsigned char*)aStr, mg_len, next.get());
        ret = kmp_find((const unsigned char*)start_addr, (int)start_len, (unsigned char*)aStr, mg_len, next.get());

        if (ret != -1)
        {
            char* pCheck = (char*)(start_addr) + ret;
            size_t t = strlen(pCheck);
            if (t < 25)
            {
                if(parse_AfxFrameOrView(pCheck))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void SpyMfc(HWND hWnd, std::string& result)
{
    result += "\r\n----------Information about MFC-------------\r\n";

    BOOL bIsMfc = ::SendMessage(hWnd,WM_QUERYAFXWNDPROC,0,0);
    if (!bIsMfc)
    {
        result += "It not a MFC window\r\n";
        return ;
    }

    // exe
    HMODULE hMod = NULL, hModSearch = NULL;
    if (0 == g_dllname[0])
    {
        hMod = ::GetModuleHandle(NULL);
    }
    else
        hMod = ::GetModuleHandleA(g_dllname);

    //const char* pBegin = (const char*)::GetModuleHandle(NULL);
    //DWORD dwPE = *((DWORD*)(pBegin + 0x3C)); // ָ��PEǩ�� 
    // dwPE + 4�Ϳ������Ժ��Ƿ���64λ�����ﲻ��

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)hMod;
    IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS*)((BYTE *)hMod + pDosHeader->e_lfanew);
    IMAGE_OPTIONAL_HEADER *pOptHeader = &pNtHeader->OptionalHeader;
    IMAGE_IMPORT_DESCRIPTOR *pImportDesc = (IMAGE_IMPORT_DESCRIPTOR *) ((BYTE *)hMod + 
        pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    g_IsStatic = true;
    while(pImportDesc->FirstThunk)  
    {  
        const char *pszDllName = (const char *)((BYTE *)hMod + pImportDesc->Name);
        if (parse_mfcdll(pszDllName))
        {
            g_IsStatic = false;
            hModSearch = GetModuleHandleA(pszDllName);
            break;
        }
        pImportDesc++;  
    }

    if (g_IsStatic)
    {
        hModSearch = hMod;

        // �ж���debug����release������
        std::vector<SECTION_T> ds = GetDataSection(hModSearch);
        std::vector<SECTION_T>::const_iterator ci = ds.begin();
        for (; ci != ds.end(); ++ci)
        {
            if (check_static((*ci).VirtualAddr, (*ci).VirtualSize))
            {
                break;
            }
        }
    }

    if (!hModSearch)
    {
        return;
    }

    // ��ӡmfc������Ϣ
    result += boost::str(boost::format("MFC version: %d, static linked: %s, debug: %s\r\n") % g_mfcver
        % (g_IsStatic ? "true" : "false") 
        % (g_Isdbg ? "true" : "false") );

    std::vector<SECTION_T> search_area = GetCodeSection(hModSearch);

    std::vector<SECTION_T>::const_iterator vi = search_area.begin();
    LPVOID p = 0;
    for (; vi != search_area.end(); ++vi)
    {
        p = find_FromHandlePermanent((*vi).VirtualAddr, (*vi).VirtualSize);
        if (p)
        {
            break;
        }
 
    }
    if (0 == p)
    {
        result += "Failed to get address of CWnd::FromHandlePermanent!\r\n";
        return;
    }
    result += boost::str(boost::format("Adress of CWnd::FromHandlePermanent: 0x%p\r\n") % p);

    typedef PVOID  (__stdcall *FROMHANDLEPERMANENT)(HWND hWnd);
    FROMHANDLEPERMANENT FromHandlePermanent = (FROMHANDLEPERMANENT)p;
    p = FromHandlePermanent(hWnd);

    if(0 == p)
    {
        result += "Failed to get object of CWnd\r\n";
        return;
    }

    result += boost::str(boost::format("CWnd object: 0x%p\r\n") % p);

    const AFX_MSGMAP* msgmap = NULL;
    CRuntimeClass* pr = NULL;
    int m_nObjectSize = 0;

    if (IsStaticRelease()) // ֻ�о�̬���Ӳ�����release����CWnd
    {
        CWnd* pCWnd = (CWnd*)p;
        pr = pCWnd->GetRuntimeClass();
        msgmap = pCWnd->GetMessageMap();
    }
    else
    {
        CWndd* pCWnd = (CWndd*)p;
        pr = pCWnd->GetRuntimeClass();
        msgmap = pCWnd->GetMessageMap();
    }

    result += boost::str(boost::format("HWND: 0x%p\r\nObject: %p (class: %s, size: %#x)\r\n") % hWnd % p % pr->m_lpszClassName % pr->m_nObjectSize);

    BOOL bIsDialog = FALSE;
    result += "Inheritance: ";
    while(pr)
    {
        if(bIsDialog == FALSE && strcmp(pr->m_lpszClassName,"CDialog") == 0)
        {
            bIsDialog = TRUE;
            //if(pr->m_nObjectSize != sizeof(CDialog))
            //{
            //    CString aa;
            //    aa.Format("Warning: sizeof(CDialog) is %X,expect %x,\r\n"
            //        "dumping of member is *incorrect*!dumping of vtbl may be *incorrect*!\r\n",
            //        pr->m_nObjectSize,sizeof(CDialog));
            //    str += aa;
            //}
        }
        //if(strcmp(pr->m_lpszClassName,"CWnd") == 0 && pr->m_nObjectSize != sizeof(CWnd))
        //{
        //    CString aa;
        //    aa.Format("Warning: sizeof(CWnd) is %X,expect %x,\r\n"
        //        "dumping of member is *incorrect*!dumping of vtbl may be *incorrect*!\r\n",
        //        pr->m_nObjectSize,sizeof(CWnd));
        //    str += aa;
        //}
        result += pr->m_lpszClassName;
        pr = GetParentRTC(pr);
        if(pr)
            result += ":";
    }

    result += "\r\n\r\n";

    // ��ȡ�麯���б�
    DWORD dwIndex;
    PVFN pVtbl = *((PVFN*)p);


#define CASE_MFC(version, debug) \
    { \
        if (IsStaticRelease()) \
        { \
            if (bIsDialog) \
            { \
                CDialog##version* pD = (CDialog##version*)p; \
                pD->get_vfn_string(pVtbl, dwIndex, result); \
            } \
            else \
            { \
                CWnd##version* pCWnd = (CWnd##version*)p; \
                pCWnd->get_vfn_string(pVtbl, dwIndex, result); \
            } \
        } \
        else \
        { \
            if (bIsDialog) \
            { \
                CDialog##version##debug* pD = (CDialog##version##debug*)p; \
                pD->get_vfn_string(pVtbl, dwIndex, result); \
            } \
            else \
            { \
                CWnd##version##debug* pCWnd = (CWnd##version##debug*)p; \
                pCWnd->get_vfn_string(pVtbl, dwIndex, result); \
            } \
        } \
    } \

    switch(g_mfcver)
    {
    case 120:
        CASE_MFC(120, d);
        break;
    case 110:
        CASE_MFC(110, d);
        break;
    case 100:
        CASE_MFC(100, d);
        break;
    case 90:
        CASE_MFC(90, d);
        break;
    case 42:
        CASE_MFC(42, d);
        break;
    default:
        CASE_MFC(00, d);
        break;
    }

#undef CASE_MFC

    if (!msgmap)
    {
        result += "Failed to get message map!\r\n";
        return;
    }
    else
    {
        // ��ӡmessage map
        result += boost::str(boost::format("\r\nMessage map: %s\r\nMessage map entries: %s\r\n")
            % GetMods(msgmap) % GetMods(msgmap->lpEntries));
        if(msgmap->lpEntries)
        {
            static UINT_PTR AfxSig_end = 0;
            std::string s1, sTemp;
            std::string s2;
            const AFX_MSGMAP_ENTRY * pentries = msgmap->lpEntries;

            while(pentries->nSig != AfxSig_end)
            {
                if(pentries->nID == pentries->nLastID || pentries->nLastID == 0)
                    s1 = boost::str(boost::format("%04x") % pentries->nID);
                else
                    s1 = boost::str(boost::format("%04x to %04x") % pentries->nID % pentries->nLastID);
                s2 = boost::str(boost::format("%04X") % pentries->nMessage);
                for(int i=0;i<sizeof(wmmsgs)/sizeof(wmmsgs[0]); ++i)
                {
                    if(wmmsgs[i].msg == pentries->nMessage)
                    {
                        s2 = boost::str(boost::format("%s(%04x)") % wmmsgs[i].smsg % wmmsgs[i].msg);
                    }
                }

                if(pentries->nMessage == -1 || (pentries->nMessage == WM_COMMAND && pentries->nCode==-1))
                    sTemp = boost::str(boost::format("UpdateCmdUI: id=%s,func= %s\r\n") % s1
                    % GetCodes( getfn(pentries) ) );
                else if(pentries->nMessage == WM_COMMAND)
                    sTemp = boost::str(boost::format("OnCommand: notifycode=%04x id=%s,func= %s\r\n")
                    % pentries->nCode % s1 % GetCodes(getfn(pentries)) );
                else if(pentries->nMessage == WM_NOTIFY)
                    sTemp = boost::str(boost::format("OnNotify: notifycode=%04x id=%s,func= %s\r\n")
                    % pentries->nCode % s1 % GetCodes(getfn(pentries)) );
                else if(pentries->nMessage == 0xC000)
                {
                    char name[512];
                    UINT * pMessage = (UINT*)pentries->nSig; 
                    name[0] = 0;
                    GetClipboardFormatNameA(*pMessage,name,sizeof(name));
                    //GlobalGetAtomName(*pMessage,name,sizeof(name));
                    sTemp = boost::str(boost::format("ReggedMsg:*(0x%p)=%04x(%s),func= %s\r\n") % pMessage % *pMessage % name
                        % GetCodes(getfn(pentries)));
                }
                else if(pentries->nCode==0 && pentries->nLastID==0 && pentries->nID==0)
                    sTemp = boost::str(boost::format("OnMsg:%s,func= %s\r\n") % s2 % GetCodes(getfn(pentries)));
                else
                    sTemp = boost::str(boost::format("msg:%s notifycode=%04x,id=%s,func= %s\r\n") % s2 % pentries->nCode
                    %s1 % GetCodes(getfn(pentries)) );
                result += sTemp;
                ++ pentries;
            }
        }
    }

}