// MainBrain32_USBTest.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include <SetupAPI.h>
#include <winusb.h>
#include <initguid.h>
#include <Usbiodef.h>
#include <shlwapi.h>
#include <sstream>
#include <Dbt.h>
#include <strsafe.h>
#include <winnt.h>
#include <vector>
#include <array>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

#include "MainBrain32_USBTest.h"
#include "resource.h"


#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "winusb.lib")

//using namespace std;

#define MAX_LOADSTRING 100

//MainBrain
#define IDD_OPEN_FILE               133
#define BackLightSlider             508
#define BackLightSliderText         509
#define BackLightSliderTitleText    510
#define GetADCTimer                 511
#define GetADCCurrentProgressBar    512
#define ProgressBarMAText           513
#define ProgressBarText             514
#define ProgressBarTitleText        515
#define ProgramListTitleText        516
#define IDC_LISTBOX                 517
#define TimeBox                     518
#define TestText                    519
#define IDC_EDIT                    520
#define EditBoxHeader               521
#define InputDlgEditBox             522
#define LED7_CheckBox               523
#define LED6_CheckBox               524
#define LED5_CheckBox               525
#define LED4_CheckBox               526
#define LED3_CheckBox               527
#define LED2_CheckBox               528
#define LED1_CheckBox               529
#define LED0_CheckBox               530
#define LED_Static                  531
#define SRAM_Static                 532
#define SRAM_Edit                   533
#define Memory_Edit                 534
#define Memory_Static               535
#define GetADCVoltageProgressBar    536
#define ProgressBarVText            537
#define ProgressBarVoltsText        538
#define ProgressBarVoltageTitleText 539
#define IDD_DUMPSRAM                140
#define IDD_INPUTBOX                1005
#define IDD_DUMPMEM                 1006
#define ID_UPDATE                   1007
#define ID_FILE_OPEN                32780
#define ID_FILE_SAVE                32778
#define ID_FILE_WRITE               32779

//The Device Programs Listbox stuff
typedef struct
{
    TCHAR progName[MAX_PATH];
} devicePrograms;

devicePrograms ProgramList[] =
{
    { TEXT("Dump BDT")},
    { TEXT("Dump Device Memory") },
    { TEXT("Clock") },
    { TEXT("Set Time") },
    { TEXT("Get Device Info") },
    { TEXT("Read SRAM") },
    { TEXT("Dump SRAM to Display") },
    { TEXT("Write File to SRAM") },
    { TEXT("Read Flash") },
    { TEXT("Write Flash") },
    { TEXT("Reset Device") },
    { TEXT("Set LEDs") },
    { TEXT("Update Firmware") },
    { TEXT("Exit") }
};

SYSTEMTIME localTime;
HDEVINFO DeviceInfoTable = INVALID_HANDLE_VALUE;
DWORD Error = NO_ERROR;
GUID InterfaceClassGuid = { 0x58D07210, 0x27C1, 0x11DD, 0xBD, 0x0B, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 };
PSP_DEVICE_INTERFACE_DATA InterfaceDataStructure = new SP_DEVICE_INTERFACE_DATA;
DWORD InterfaceIndex;
PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure = new SP_DEVICE_INTERFACE_DETAIL_DATA;
HANDLE MyDeviceHandle = INVALID_HANDLE_VALUE;
WINUSB_INTERFACE_HANDLE MyWinUSBInterfaceHandle;
HDC dc;
LPCWSTR c;
WORD hpos = 10;
WORD vpos = 10;
RECT rect;
HBRUSH hBrush;
HFONT hFont;
PAINTSTRUCT ps;
BYTE keyInput = 0;
//TCHAR writeBuffer[8193];
//wchar_t EditBuffer[8193];
wchar_t buf[4];
unsigned int fs;

//File Save Buffer
wchar_t  SaveBuffer[8193] = { 0 };

wchar_t ErrorBuffer[10] = { '\0' };
wchar_t MyBuffer[30];
int pos;
float current;
float volts;
BOOL a = 0;
int prog = 0;
int lbIndex = 0;
BYTE DeviceIndex = -1;
HWND hEdit;
static HBRUSH hbrBkgnd;
unsigned char outBuffer[64] = { 0 };
unsigned long MemoryData = 0;

//Global DialogBox GetSRAM data variable
unsigned char SRAMBlock = 0;

//CheckBox states
BOOL checked_7 = 0;
BOOL checked_6 = 0;
BOOL checked_5 = 0;
BOOL checked_4 = 0;
BOOL checked_3 = 0;
BOOL checked_2 = 0;
BOOL checked_1 = 0;
BOOL checked_0 = 0;

//The state machine to manage the USB connection
enum class STATE_MACHINE
{
    NOT_ATTACHED,
    NOT_CONNECTED,
    ATTACHED,
    CONNECTED
};

STATE_MACHINE deviceState = STATE_MACHINE::NOT_CONNECTED;

//Device Program List
WCHAR DevicePrograms[] = { TEXT("Display BDT")};

void FindMainBrain32(HWND hWnd);
void SelectDevice(HWND hWnd, BYTE DeviceIndex);
void ReadSRAM(HWND hWnd);
void WriteFile2SRAM(HWND hWnd);
void OpenMyFile(HWND hWnd);
void GetDeviceInfo(HWND hWnd);
void SetDeviceDisplayBackLight(HWND hWnd, int pos);
BYTE GetDeviceDisplayBackLight(HWND hWnd);
void GetADCCurrentSense(HWND hWnd);
void SendNotConnected(HWND hWnd);
void RunProgram(HWND hWnd, int prog);
void ListPrograms(HWND hWnd);
void SliderScroll(HWND hWnd);
void SetDeviceLEDs(HWND hWnd);
void SaveMyFile(HWND hWnd);
void SetTime(HWND hWnd);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FindDevice(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DeviceState(HWND, UINT, WPARAM, LPARAM);

char TextArray[] = { "MainBrain32" };

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAINBRAIN32USBTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAINBRAIN32USBTEST));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINBRAIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MAINBRAIN32USBTEST);
    wcex.lpszClassName  = szWindowClass;
    //wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAINBRAIN32));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX ^ WS_MINIMIZEBOX,
      250, 250, 600, 400, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

    //Register for device notification details
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = InterfaceClassGuid;

    RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

    //Connect to MainBrain
    FindMainBrain32(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
      case WM_DEVICECHANGE:
      {
          //PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
          switch (wParam)
          {
             case DBT_DEVNODES_CHANGED:
                 break;

             case DBT_DEVICEARRIVAL:
                 //Write to Window
                 InvalidateRect(hWnd, NULL, TRUE);
                 UpdateWindow(hWnd);

                 deviceState = STATE_MACHINE::ATTACHED;

                 FindMainBrain32(hWnd);
                 break;

             case DBT_DEVICEREMOVECOMPLETE:
                 InvalidateRect(hWnd, NULL, TRUE);
                 UpdateWindow(hWnd);

                 //Make the ListBox visible
                 ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
                 ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);

                 dc = GetDC(hWnd);
                 SelectObject(dc, hFont);
                 c = L"Device Detached";
                 vpos = 10;
                 hpos = 10;
                 TextOut(dc, hpos, vpos, c, 16);
                 deviceState = STATE_MACHINE::NOT_ATTACHED;
                 break;

             default:
                 InvalidateRect(hWnd, NULL, TRUE);
                 UpdateWindow(hWnd);

                c = L"Unknown Device Change";
                TextOut(dc, hpos, vpos, c, sizeof(c));
                vpos = vpos + 20;
                break;
          }
          break;
      }
      break;

      //SendMessageW(GetDlgItem(hWnd, IDC_LISTBOX), LB_GETCURSEL, 0, NULL);

      //This sets the background of the controls to white with black text
      //Gets rid of the default control shadows
      case WM_CTLCOLORSTATIC:
      {
          HDC hdcStatic = (HDC)wParam;
          SetTextColor(hdcStatic, RGB(0, 0, 0));
          SetBkColor(hdcStatic, RGB(255, 255, 255));

          if (hbrBkgnd == NULL)
          {
              hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
          }
          return (INT_PTR)hbrBkgnd;
      }
      break;
      case WM_KEYDOWN:
      {
              if (deviceState == STATE_MACHINE::NOT_CONNECTED)
              {
                  if (wParam > 0x29 && wParam < 0x39)
                  {
                      //MessageBoxW(NULL, L"Key Pressed!", L"MsgFromTextBox", MB_OK);
                      DeviceIndex = ((BYTE)wParam - 0x30);
                      //SelectedState = TRUE;
                      SelectDevice(hWnd, DeviceIndex);
                  }
                  else
                  {
                      SelectDevice(hWnd, 0);
                  }
              }
       }
      break;

     case WM_COMMAND:
     {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            // Parse the menu selections:
            switch (wmId)
            {
            case ID_FILE_WRITE:
                WriteFile2SRAM(hWnd);
                break;
            case ID_FILE_SAVE:
                SaveMyFile(hWnd);
                break;
            case ID_PROGRAMS:
                ListPrograms(hWnd);
                break;
            case ID_FILE_OPEN:
                OpenMyFile(hWnd);
                break;
            case ID_CONNECT:
                if (deviceState == STATE_MACHINE::CONNECTED)
                {
                    if (wParam > 0x29 && wParam < 0x39)
                    {
                        DeviceIndex = ((BYTE)wParam - 0x30);
                        SelectDevice(hWnd, DeviceIndex);
                    }

                }
                if (deviceState == STATE_MACHINE::NOT_CONNECTED)
                {
                    SelectDevice(hWnd, 0);
                }
                break;
            case IDC_LISTBOX:
                if (wmEvent == LBN_SELCHANGE)
                {
                    lbIndex = (BYTE)SendMessage(GetDlgItem(hWnd, IDC_LISTBOX), LB_GETCURSEL, NULL, NULL);
                }
                if (wmEvent == LBN_DBLCLK)
                {
                    RunProgram(hWnd, lbIndex);
                }
                break;
            case IDM_ABOUT:
                 DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                //Close the USB session if connected
                if (deviceState == STATE_MACHINE::CONNECTED)
                {   
                    SetDeviceDisplayBackLight(hWnd, 3);
                    SendNotConnected(hWnd);
                    WinUsb_Free(MyWinUSBInterfaceHandle);
                    CloseHandle(MyDeviceHandle);
                    //Do this when finished drawing text
                    ReleaseDC(hWnd, dc);
                }
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
     case WM_TIMER:
 
         break;
    case WM_CREATE:
    {
        //clear the file buffer
        //memset(readBuffer, 0, sizeof readBuffer);

        hFont = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));

        CreateWindow(L"STATIC", L"0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F", SS_LEFT | WS_CHILD | WS_VISIBLE, 15, 2, 350, 30, hWnd, (HMENU)EditBoxHeader, NULL, NULL);

        //CreateWindow(L"button", NULL, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 305, 10, 12, hWnd, (HMENU)507, NULL, NULL);
       
        //HFONT hFont;
        //For Edit Control...
        hEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,                   /* Extended p ossibilites for variation */
            L"EDIT",         /* Classname */
            NULL,       /* Title Text */
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, /* default window */
            5,       /* Windows decides the position */
            20,       /* where the window ends up on the screen */
            360,                 /* The programs width */
            290,                 /* and height in pixels */
            hWnd,        /* The window is a child-window to hwnd */
            (HMENU)IDC_EDIT,                /* No menu */
            GetModuleHandle(NULL),       /* Program Instance handler */
            NULL                 /* No Window Creation data */
        );

        //Backlight Slider
        CreateWindowW(TRACKBAR_CLASSW, NULL, WS_CHILD | TBS_VERT | WS_VISIBLE | TBS_AUTOTICKS, 520, 50, 30, 230, hWnd, (HMENU)BackLightSlider, NULL, NULL);
        CreateWindow(L"STATIC", L"Display\r\nBrightness", SS_CENTER | WS_CHILD | WS_VISIBLE, 495, 10, 80, 30, hWnd, (HMENU)BackLightSliderTitleText, NULL, NULL);
        CreateWindowW(L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_CENTER, 515, 280, 30, 15, hWnd, (HMENU)BackLightSliderText, NULL, NULL);

        //Current Progress Bar
        CreateWindowEx(NULL, PROGRESS_CLASS, NULL, PBS_SMOOTH | WS_CHILD | WS_VISIBLE | PBS_VERTICAL, 460, 50, 15, 230, hWnd, (HMENU)GetADCCurrentProgressBar, NULL, NULL);
        CreateWindow(L"STATIC", L"Device\r\nCurrent (mA)", SS_CENTER | WS_CHILD | WS_VISIBLE, 425, 10, 80, 30, hWnd, (HMENU)ProgressBarTitleText, NULL, NULL);
        CreateWindow(L"STATIC", L"mA", SS_CENTER | WS_CHILD | WS_VISIBLE, 475, 280, 20, 15, hWnd, (HMENU)ProgressBarMAText, NULL, NULL);
        CreateWindow(L"STATIC", NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, 435, 280, 40, 15, hWnd, (HMENU)ProgressBarText, NULL, NULL);

        //Voltage Progress bar
        CreateWindowEx(NULL, PROGRESS_CLASS, NULL, PBS_SMOOTH | WS_CHILD | WS_VISIBLE | PBS_VERTICAL, 395, 50, 15, 230, hWnd, (HMENU)GetADCVoltageProgressBar, NULL, NULL);
        CreateWindow(L"STATIC", L"Device\r\nInput", SS_CENTER | WS_CHILD | WS_VISIBLE, 365, 10, 70, 30, hWnd, (HMENU)ProgressBarVoltageTitleText, NULL, NULL);
        CreateWindow(L"STATIC", L"V", SS_CENTER | WS_CHILD | WS_VISIBLE, 410, 280, 20, 15, hWnd, (HMENU)ProgressBarVText, NULL, NULL);
        CreateWindow(L"STATIC", NULL, SS_RIGHT | WS_CHILD | WS_VISIBLE, 370, 280, 40, 30, hWnd, (HMENU)ProgressBarVoltsText, NULL, NULL);


        //Listbox
        CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 20, 50, 300, 255, hWnd, (HMENU)IDC_LISTBOX, NULL, NULL);
        CreateWindow(L"STATIC", L"Command List", SS_CENTER | WS_CHILD | WS_VISIBLE, 100, 20, 100, 15, hWnd, (HMENU)ProgramListTitleText, NULL, NULL);

        //Menu
        EnableMenuItem(GetMenu(hWnd), ID_CONNECT, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), ID_PROGRAMS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), ID_FILE_WRITE, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), ID_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_GRAYED);
        SendMessageW(GetDlgItem(hWnd, BackLightSlider), TBM_SETRANGE, TRUE, MAKELONG(0, 10));
        SendMessageW(GetDlgItem(hWnd, BackLightSlider), TBM_SETPOS, TRUE, 0);

        //Sets the min and max values
        SendMessage(GetDlgItem(hWnd, GetADCCurrentProgressBar), PBM_SETRANGE, 0, MAKELPARAM(0, 1023));
        SendMessage(GetDlgItem(hWnd, GetADCVoltageProgressBar), PBM_SETRANGE, 0, MAKELPARAM(0, 24));

        //Sets the bar color
        SendMessage(GetDlgItem(hWnd, GetADCCurrentProgressBar), PBM_SETBARCOLOR, 0, RGB(34, 212, 165));
        SendMessage(GetDlgItem(hWnd, GetADCVoltageProgressBar), PBM_SETBARCOLOR, 0, RGB(34, 212, 165));

        //Hide these controls until connected
        ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
        EnableWindow(GetDlgItem(hWnd, BackLightSlider), FALSE);
        ShowWindow(GetDlgItem(hWnd, BackLightSliderText), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, ProgressBarText), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, ProgressBarVText), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, ProgressBarMAText), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_HIDE); 
        ShowWindow(GetDlgItem(hWnd, EditBoxHeader), SW_HIDE);

        //This gets rid of the dotted line slider
        SendMessage(hWnd, WM_CHANGEUISTATE, (WPARAM)MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

        for (int i = 0; i < ARRAYSIZE(ProgramList); i++)
        {
            SendMessage(GetDlgItem(hWnd, IDC_LISTBOX), LB_ADDSTRING, NULL, (LPARAM)ProgramList[i].progName);
            // Set the array index of the Listbox as item data.
            // This enables us to retrieve the item from the array
            // even after the items are sorted by the list box.
            SendMessage(GetDlgItem(hWnd, IDC_LISTBOX), LB_SETITEMDATA, NULL, (LPARAM)i);
        }

        pos = 0;
        char buf[4];
        sprintf_s(buf, "%i", pos);
        SetWindowTextA(GetDlgItem(hWnd, BackLightSliderText), buf);
    }
    break;
    case WM_INITDIALOG:
    {

    }
    break;
    case WM_VSCROLL:
        SliderScroll(hWnd);
        break;
    
    case WM_PAINT:
        {
            dc = BeginPaint(hWnd, &ps);

            //Set the font for each control
            SendMessage(GetDlgItem(hWnd, ProgressBarTitleText), WM_SETFONT, (WPARAM)hFont, TRUE); 
            SendMessage(GetDlgItem(hWnd, ProgressBarMAText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, ProgressBarText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, ProgressBarVoltageTitleText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, ProgressBarVoltsText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, ProgressBarVText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, IDC_LISTBOX), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, BackLightSliderTitleText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, BackLightSliderText), WM_SETFONT, (WPARAM)hFont, TRUE); 
            SendMessage(GetDlgItem(hWnd, ProgramListTitleText), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, EditBoxHeader), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, IDC_EDIT), WM_SETFONT, (WPARAM)hFont, TRUE);

            dc = GetDC(hWnd);
            SelectObject(dc, hFont);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_CLOSE:
        if (deviceState == STATE_MACHINE::CONNECTED)
        {
            SetDeviceDisplayBackLight(hWnd, 3);
            SendNotConnected(hWnd);
            WinUsb_Free(MyWinUSBInterfaceHandle);
            CloseHandle(MyDeviceHandle);
            //Do this when finished drawing text
            ReleaseDC(hWnd, dc);
        }
        DestroyWindow(hWnd);

        break;
    case WM_DESTROY:
        DeleteObject(hbrBkgnd);
        KillTimer(hWnd, GetADCTimer);        
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);

    }
    return 0;
}

// Message handler for Input box
INT_PTR CALLBACK Input(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:

        //Static Text
        CreateWindowW(L"STATIC", L"D7 D6 D5 D4 D3 D2 D1 D0", WS_CHILD | WS_VISIBLE, 12, 0, 200, 30, hDlg, (HMENU)LED_Static, NULL, NULL);
        //LED CheckBoxes
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 15, 20, 10, 10, hDlg, (HMENU)LED7_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 35, 20, 10, 10, hDlg, (HMENU)LED6_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 55, 20, 10, 10, hDlg, (HMENU)LED5_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 75, 20, 10, 10, hDlg, (HMENU)LED4_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 95, 20, 10, 10, hDlg, (HMENU)LED3_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 115, 20, 10, 10, hDlg, (HMENU)LED2_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 135, 20, 10, 10, hDlg, (HMENU)LED1_CheckBox, NULL, NULL);
        CreateWindowW(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 155, 20, 10, 10, hDlg, (HMENU)LED0_CheckBox, NULL, NULL);
        return (INT_PTR)TRUE;

    case WM_LBUTTONDOWN:
    {
        //Get CheckBox States
        checked_7 = IsDlgButtonChecked(hDlg, LED7_CheckBox);
        checked_6 = IsDlgButtonChecked(hDlg, LED6_CheckBox);
        checked_5 = IsDlgButtonChecked(hDlg, LED5_CheckBox);
        checked_4 = IsDlgButtonChecked(hDlg, LED4_CheckBox);
        checked_3 = IsDlgButtonChecked(hDlg, LED3_CheckBox);
        checked_2 = IsDlgButtonChecked(hDlg, LED2_CheckBox);
        checked_1 = IsDlgButtonChecked(hDlg, LED1_CheckBox);
        checked_0 = IsDlgButtonChecked(hDlg, LED0_CheckBox);
    }
    break;

    case WM_PAINT:
        //Set the Font
        SendMessage(GetDlgItem(hDlg, LED_Static), WM_SETFONT, (WPARAM)hFont, TRUE);
        dc = GetDC(hDlg);
        SelectObject(dc, hFont);
        EndPaint(hDlg, &ps);

        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == ID_UPDATE)
        {
            //Get CheckBox States
            checked_7 = IsDlgButtonChecked(hDlg, LED7_CheckBox);
            checked_6 = IsDlgButtonChecked(hDlg, LED6_CheckBox);
            checked_5 = IsDlgButtonChecked(hDlg, LED5_CheckBox);
            checked_4 = IsDlgButtonChecked(hDlg, LED4_CheckBox);
            checked_3 = IsDlgButtonChecked(hDlg, LED3_CheckBox);
            checked_2 = IsDlgButtonChecked(hDlg, LED2_CheckBox);
            checked_1 = IsDlgButtonChecked(hDlg, LED1_CheckBox);
            checked_0 = IsDlgButtonChecked(hDlg, LED0_CheckBox);
            HWND parent = GetParent(hDlg);
            SetDeviceLEDs(parent);
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for About DialogBox
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
         return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for Dump SRAM DialogBox
INT_PTR CALLBACK DumpSRAM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:

        //Static Text
        CreateWindowW(L"STATIC", L"Enter SRAM Block:", WS_CHILD | WS_VISIBLE, 15, 20, 150, 20, hDlg, (HMENU)SRAM_Static, NULL, NULL);

        //EditBox
        CreateWindowW(L"EDIT", L"0", WS_CHILD | WS_BORDER | WS_VISIBLE | ES_NUMBER , 140, 18, 50, 20, hDlg, (HMENU)SRAM_Edit, NULL, NULL);

        return (INT_PTR)TRUE;

    case WM_PAINT:
        //Set the Font
        SendMessage(GetDlgItem(hDlg, SRAM_Static), WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(GetDlgItem(hDlg, SRAM_Edit), WM_SETFONT, (WPARAM)hFont, TRUE);
        dc = GetDC(hDlg);
        SelectObject(dc, hFont);
        EndPaint(hDlg, &ps);

        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            SRAMBlock = GetDlgItemInt(hDlg, SRAM_Edit, 0, 0);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DumpMemory(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        //Static Text
        CreateWindowW(L"STATIC", L"Enter Memory Address:", WS_CHILD | WS_VISIBLE, 30, 5, 150, 20, hDlg, (HMENU)Memory_Static, NULL, NULL);

        //EditBox
        CreateWindowW(L"EDIT", L"2684354560", WS_CHILD | WS_BORDER | ES_NUMBER | WS_VISIBLE, 75, 25, 100, 20, hDlg, (HMENU)Memory_Edit, NULL, NULL);

        return (INT_PTR)TRUE;

    case WM_PAINT:
        //Set the Font
        SendMessage(GetDlgItem(hDlg, Memory_Static), WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(GetDlgItem(hDlg, Memory_Edit), WM_SETFONT, (WPARAM)hFont, TRUE);
        dc = GetDC(hDlg);
        SelectObject(dc, hFont);
        EndPaint(hDlg, &ps);

        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            MemoryData = GetDlgItemInt(hDlg, Memory_Edit, 0, 0);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


// Message handler for Error DialogBox
INT_PTR CALLBACK DeviceState(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//Set LEDs
void SetDeviceLEDs(HWND hWnd)
{
    ULONG bytesSent;

    outBuffer[1] = checked_7;
    outBuffer[2] = checked_6;
    outBuffer[3] = checked_5;
    outBuffer[4] = checked_4;
    outBuffer[5] = checked_3;
    outBuffer[6] = checked_2;
    outBuffer[7] = checked_1;
    outBuffer[8] = checked_0;

    //Send the command
    bool Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    //Handle any error
    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"Send Command \r\nRun Device Program", ErrorBuffer, MB_OK);

        //If we can't send data then we can't be connected
        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }
}

void SliderScroll(HWND hWnd)
{
    pos = (int)SendMessageW(GetDlgItem(hWnd, BackLightSlider), TBM_GETPOS, 0, 0);
    pos = 10 - pos;
    wsprintfW(buf, L"%ld", pos);

    SetWindowTextW(GetDlgItem(hWnd, BackLightSliderText), buf);

    //Set the value of the device's backlight
    SetDeviceDisplayBackLight(hWnd, pos);

}

void ListPrograms(HWND hWnd)
{
     if (deviceState != STATE_MACHINE::CONNECTED)
    {
        //DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    //Clear the information area
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    //Hide the edit window until open file
    ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, EditBoxHeader), SW_HIDE);

    //Make the ListBox visible
    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_SHOW);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_SHOW);

    //Select index 0
    SendMessageW(GetDlgItem(hWnd, IDC_LISTBOX), LB_SETCURSEL, 0, NULL);

}

void RunProgram(HWND hWnd, int prog)
{
 /*The Program List
    0 Dump BDT
    1 Dump Device Memory
    2 Clock
    3 Set Time
    4 Get Device Info
    5 Read SRAM
    6 Dump SRAM to Display
    7 Write File to SRAM
    8 Read Flash
    9 Write Flash
    10 Restart Device
    11 Set LEDs
    12 Update Firmware
    13 Exit
 */
 
    lbIndex = (BYTE)SendMessageW(GetDlgItem(hWnd, IDC_LISTBOX), LB_GETCURSEL, 0, 0);
    
    //If we are not connected to the device then do nothing
    if (deviceState != STATE_MACHINE::CONNECTED)
    {
        //DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    ULONG bytesSent;
    //ULONG BytesTransferred;
    BOOL Status;
    char cmd = lbIndex + 0x20;
    wchar_t tempBuffer[9] = { 0 };

    //Load the command 
    outBuffer[0] = cmd;

    //Dump Device Memory
    if (cmd == 0x21)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DUMPMEM), hWnd, DumpMemory);
        
        outBuffer[1] = (MemoryData >> 24) & 0x000000ff;
        outBuffer[2] = (MemoryData >> 16) & 0x000000ff;
        outBuffer[3] = (MemoryData >> 8) & 0x000000ff;
        outBuffer[4] = MemoryData & 0x000000ff;

        //show the integer from the EditBox
        //_itow_s(outBuffer[1], tempBuffer, 16);
        //MessageBoxW(NULL, tempBuffer, L"MsgFromTextBox", MB_OK);

    }

    //Set Time
    if (cmd == 0x23)
    {
        SetTime(hWnd);
        return;
    }

    //Get Device Info
    if (cmd == 0x24)
    {
        GetDeviceInfo(hWnd);
        return;
    }

    //Read SRAM
    if (cmd == 0x25)
    {
        ReadSRAM(hWnd);
        return;
    }

    //Dump SRAM to Display
    if (cmd == 0x26)
    {
        //Prompt for which block of memory (2 bytes) to display
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DUMPSRAM), hWnd, DumpSRAM);

        //_itow_s(SRAMBlock, ErrorBuffer, 10);
        //MessageBoxW(NULL, L"SRAM BLOCK", ErrorBuffer, MB_OK);
        if (SRAMBlock > 127)
        {
            SRAMBlock = 127;
        }
        outBuffer[1] = SRAMBlock;

        //return;
    }

    //Write File to SRAM
    if (cmd == 0x27)
    {
        WriteFile2SRAM(hWnd);
        return;
    }

    //Reset the Device
    if (cmd == 0x2a)
    {
        ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        DeviceIndex = -1;
        deviceState = STATE_MACHINE::NOT_CONNECTED;
    }

    //Set LEDs
    if (cmd == 0x2b)
    {
        //Prompt for which LEDs
        DialogBox(hInst, MAKEINTRESOURCE(IDD_INPUTBOX), hWnd, Input);
        return;
    }

    //Update Firmware
    if (cmd == 0x2c)
    {
        //ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
        //ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    }

    //Exit
    if (cmd == 0x2d)
    {
        //ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
        //ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    }


    //Send the command
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    //Handle any error
    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"Send Command \r\nRun Device Program", ErrorBuffer, MB_OK);

        //If we can't send data then we can't be connected
        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }
}

void SetTime(HWND hWnd)
{
    ULONG bytesSent = 0;
    GetLocalTime(&localTime);
    //Get the time from localTime
    outBuffer[0] = 0x23;
    outBuffer[1] = (char)localTime.wHour;
    outBuffer[2] = (char)localTime.wMinute;
    outBuffer[3] = (char)localTime.wSecond;

    //Send the command
    BOOL Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    //Handle any error
    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"Set Time Command", ErrorBuffer, MB_OK);

        //If we can't send data then we can't be connected
        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

}

void WriteFile2SRAM(HWND hWnd)
{
    unsigned char outBuffer[64] = { 0 };
    int blockCount = 1;
    DWORD filePosition = 0;
    int count = 0;
    int blockSize = 64;
    BOOL Status = FALSE;
    ULONG bytesSent;
    //A pointer to a constant of any type
    UCHAR ReadBuffer[8193];
    char newBuffer[17000];
    int Textlength = 0;
    std::vector<WORD> vBuffer[8193];
    wchar_t* pEnd;
    HANDLE MyFile;
    LPDWORD BytesWritten = 0;
    DWORD dwBytesRead = 0;

    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);
 
    //Get the length of text
    Textlength = GetWindowTextLengthA(GetDlgItem(hWnd, IDC_EDIT));

    //get the data from the EditBox
    GetWindowTextA(hWnd, (LPSTR)ReadBuffer, Textlength);


    //int Bytes = WideCharToMultiByte(CP_UTF8, WC_DEFAULTCHAR, ReadBuffer, Textlength, newBuffer, 17000, NULL, NULL);

//    UINT codePage = GetACP();

//   _itow_s(codePage, ErrorBuffer, 10);
//   MessageBoxW(NULL, L"Converted", ErrorBuffer, MB_OK);

   //Open REN70V05.sram
   MyFile = CreateFileA("REN70V05.sram", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (MyFile == INVALID_HANDLE_VALUE)
   {
       Error = GetLastError();
       _itow_s(Error, ErrorBuffer, 10);
       MessageBoxW(NULL, L"File Create Handle\r\nInvalid Handle", ErrorBuffer, MB_OK);

       return;
   }

   //Read File
   if (ReadFile(MyFile, ReadBuffer, fs, &dwBytesRead, NULL) == 0)
   {
       Error = GetLastError();
       _itow_s(Error, ErrorBuffer, 10);
       MessageBoxW(NULL, L"File Read error", ErrorBuffer, MB_OK);

       return;
   }


   if (WriteFile(MyFile, newBuffer, fs, BytesWritten, NULL) == 0)
   {
       Error = GetLastError();
       _itow_s(Error, ErrorBuffer, 10);
       MessageBoxW(NULL, L"File Write Error", ErrorBuffer, MB_OK);

       return;
   }


   return;





   
 

    //Now send the data to the device
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, outBuffer, blockSize, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Read Command 0x60\r\nRead File Error", ErrorBuffer, MB_OK);

        return;
    }
 

    vpos = 315;
    hpos = 10;


    c = L"File Size: ";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 75;

    wchar_t ByteBuffer[16] = { 0 };
    _itow_s(Textlength, ByteBuffer, 10);
    TextOut(dc, hpos, vpos, ByteBuffer, (int)_tcslen(ByteBuffer));
    hpos = hpos + 50;

    c = L"Bytes Sent: ";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 80;

    _itow_s(filePosition, ByteBuffer, 10);
    TextOut(dc, hpos, vpos, ByteBuffer, (int)_tcslen(ByteBuffer));
    hpos = hpos + 50;

     c = L"Block Size: ";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 80;

    _itow_s(blockSize, ByteBuffer, 10);
    TextOut(dc, hpos, vpos, ByteBuffer, (int)_tcslen(ByteBuffer));

}

void SaveMyFile(HWND hWnd)
{
    HANDLE MyFile;
    DWORD fs = 0;
    LPDWORD BytesWritten = 0;
    int BytesRead = 0;
    char FileBuffer[8193] = { 0 };
    char ReadBuffer[8193];
    int Textlength = 0;
    wchar_t _Buffer = { 0 };

    //Get the length of text
    Textlength = GetWindowTextLengthA(GetDlgItem(hWnd, IDC_EDIT));

    //get the data from the EditBox
    BytesRead = GetWindowTextA((GetDlgItem(hWnd, IDC_EDIT)), ReadBuffer, Textlength);

    int i;
    int n = 0;
    //ReadBuffer now contains ASCII characters
    //so convert to unsigned char
    for(i=0;i<Textlength;i=i+2)
    { 
        if (ReadBuffer[i] == 32)
        {
            i++;
        }

        unsigned char a = atoi(&ReadBuffer[i]);

        //add the char to the file buffer
        FileBuffer[n] = a;
        n++;
    }

//    _itow_s(a, ErrorBuffer, 10);
 //   MessageBoxW(NULL, L"Data", ErrorBuffer, MB_OK);

    //Open REN70V05.sram
    MyFile = CreateFileA("REN70V05_NEW.sram", GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (MyFile == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"File Open Error", ErrorBuffer, MB_OK);

        return;
    }

    fs = GetFileSize(MyFile, NULL);


    WriteFile(MyFile, FileBuffer, n, BytesWritten, NULL);


    CloseHandle(MyFile);
}

void OpenMyFile(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    //Show the edit window
    ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_SHOW);
    ShowWindow(GetDlgItem(hWnd, EditBoxHeader), SW_SHOW);
    EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
    //Hide the ListBox
    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);

    //clear the EditBox
    SetWindowText(GetDlgItem(hWnd, IDC_EDIT), L"");

    HANDLE MyFile;
    DWORD  dwBytesRead = 0;
    unsigned char ReadBuffer[25000] = { 0 };
    //File and EditBox buffers
    char ByteBuffer[25000] = { '\0' };
    unsigned int i = 0;
    char midBuffer[3] = { 0 };

    //This is the size of MainBrain's SRAM and cannot be exceeded
    DWORD MAXBUFFERSIZE = 8193;
    
    //REN70V05.sram
    MyFile = CreateFileA("REN70V05.sram", GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (MyFile == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"File Create Handle\r\nInvalid Handle", ErrorBuffer, MB_OK);

        return;
    }


    //Get the file size
    fs = GetFileSize(MyFile, NULL);
 
 
    BOOL rf = ReadFile(MyFile, ReadBuffer, fs, &dwBytesRead, NULL);

    if(!rf)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"File Read Data\r\nUnable to read file", ErrorBuffer, MB_OK);

        CloseHandle(MyFile);

        return;
    }

    if (fs > MAXBUFFERSIZE)
    {
        fs = MAXBUFFERSIZE;
    }

    //Write file data to the EditBox
    for (i = 0; i < fs; i++)
    {
        
       //convert the binary file byte to a string
        _itoa_s(ReadBuffer[i], midBuffer, 16);

        if (ReadBuffer[i] < 16)
        {
            //insert a zero if single digit
            strcat_s(ByteBuffer, "0");

        }

        // append the newText to the string buffer
        strcat_s(ByteBuffer, midBuffer);
 
        //add a space between characters
        strcat_s(ByteBuffer, " ");
    }
    

    //Dump the file data to the EditBox
    SetWindowTextA(hEdit, ByteBuffer);

    vpos = 315;
    hpos = 10;

    LPCWSTR c = L"REN70V05.sram: ";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 100;

    wchar_t buf[30] = { 0 };
    _itow_s(fs, buf, 10);
    TextOut(dc, hpos, vpos, buf, (int)_tcslen(buf));
   
    hpos = hpos + 30;

    c = L"Bytes";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 100;

    EnableMenuItem(GetMenu(hWnd), ID_FILE_WRITE, MF_BYCOMMAND | MF_ENABLED);

    CloseHandle(MyFile);
}

void GetADCCurrentSense(HWND hWnd)
{
    if (deviceState != STATE_MACHINE::CONNECTED)
    {
        //DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    char outBuffer[64] = { 0 };
    unsigned char inBuffer[64] = { 0 };
    ULONG bytesSent;
    ULONG BytesTransferred;
    int new_pos = 0;
    int new_pos_v = 0;
    BOOL Status;

    //We must set the beginning address and length of the SRAM data to read
    outBuffer[0] = 0x73;

    //Send the command to enable ADC read
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Enable Command 0x73\r\nGet Board Current Error", ErrorBuffer, MB_OK);
 
        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

        //place a command byte (97 - Read Data) at the beginning of the buffer
        outBuffer[0] = (char)0x81;

        //Send the command to read SRAM data
        Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

        if (Status == TRUE)
        {
            WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &inBuffer[0], 64, &BytesTransferred, NULL);
        }
        else
        {
            Error = GetLastError();
            _itow_s(Error, ErrorBuffer, 10);
            MessageBoxW(NULL, L"USB Read Command 0x81\r\nGet Board Current Error", ErrorBuffer, MB_OK);

            deviceState = STATE_MACHINE::NOT_CONNECTED;

            return;
        }

    //Get the data
    new_pos = inBuffer[1];
    new_pos = new_pos << 8;
    new_pos = new_pos + inBuffer[0];
    current = new_pos * 1;
    
    //voltage input
    new_pos_v = inBuffer[3];
    new_pos_v = new_pos_v << 8;
    new_pos_v = new_pos_v + inBuffer[2];
    if (new_pos_v < 460)
    {
        volts = new_pos_v * .0263;
    }
    else
    {
        volts = new_pos_v * .0255;
    }
    //Sets the current positions of the bars
    SendMessage(GetDlgItem(hWnd, GetADCCurrentProgressBar), PBM_SETPOS, current, 0);
    SendMessage(GetDlgItem(hWnd, GetADCVoltageProgressBar), PBM_SETPOS, volts, 0);

    char cuf[16];
    sprintf_s(cuf, "%.0f", current);

    SetWindowTextA(GetDlgItem(hWnd, ProgressBarText), cuf);

    char cuf_v[16];
    sprintf_s(cuf_v, "%.1f", volts);

    SetWindowTextA(GetDlgItem(hWnd, ProgressBarVoltsText), cuf_v);

}

BYTE GetDeviceDisplayBackLight(HWND hWnd)
{
    if (deviceState != STATE_MACHINE::CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return 0;
    }

    char outBuffer[64] = { 0 };
    unsigned char inBuffer[64] = { 0 };
    ULONG bytesSent;
    ULONG BytesTransferred;

    //We must set the beginning address and length of the SRAM data to read
    outBuffer[0] = 0x72;
    outBuffer[1] = pos;

    //Send the command to set SRAM address7
    BOOL Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == TRUE)
    {
        WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &inBuffer[0], 64, &BytesTransferred, NULL);
    }
    else
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Write Command 0x72\r\nGet Board Backlight Error", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return 0;
    }

    pos = inBuffer[0];
    return pos;
}


void SetDeviceDisplayBackLight(HWND hWnd, int pos)
{
    if (deviceState == STATE_MACHINE::NOT_CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    char outBuffer[64] = { 0 };
    ULONG bytesSent;

    //We must set the beginning address and length of the SRAM data to read
    outBuffer[0] = 0x61;
    outBuffer[1] = pos; 

    //Send the command to set SRAM address7
    BOOL Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Write Command 0x64\r\nSet Board Current Error", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

}

void ReadSRAM(HWND hWnd)
{
    
    unsigned char inBuffer[64] = { 0 };
    char outBuffer[64] = { 0 };
    int count;
    int blockCount = 1;
    wchar_t countBuffer[64] = { 0 };
    int blockSize = 64;
    int SRAMReadLength;
    ULONG bytesSent;
    ULONG BytesTransferred;
    int adc = 0;
    //File and EditBox buffers
    wchar_t ByteBuffer[8193] = { '\0' };
    wchar_t midBuffer[8193] = { 0 };
    wchar_t SRAMBuffer[24577] = { 0 };

    BOOL Status;

    if (deviceState == STATE_MACHINE::NOT_CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
 
        return;
    }

    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    //Show the edit window
    ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_SHOW);
    ShowWindow(GetDlgItem(hWnd, EditBoxHeader), SW_SHOW);
    EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
    //Hide the ListBox
    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);
    SelectObject(dc, hFont);

    vpos = 10;
    hpos = 15;

    char value = 1;
    value = (value - 1) * 64;

    SRAMReadLength = 64; // 64;

    //We must set the beginning address and length of the SRAM data to read
    outBuffer[0] = 0x70;

    //Send the command to set SRAM address
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Write Command 0x70\r\nSet SRAM Range Error", ErrorBuffer, MB_OK);

        return;
    }
 
    
    //MainBrain has the SRAM offset now so send the data
    while (blockCount <= 128)
    {
        //place a command byte at the beginning of the buffer
        outBuffer[0] = (char)0x81;

        //Send the command to read SRAM data
        Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, blockSize, &bytesSent, NULL);

        if (Status == TRUE)
        {
            WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, inBuffer, 64, &BytesTransferred, NULL);
        }
        else
        {
            Error = GetLastError();
            _itow_s(Error, ErrorBuffer, 10);
            MessageBoxW(NULL, L"USB Read Command 0x81\r\nGet SRAM Data Error", ErrorBuffer, MB_OK);

            return;
        }

        blockCount++;

        for (count = 0; count < 64; count++)
        {
            //convert the binary file byte to a 16-bit wchar_t
            _itow_s(inBuffer[count], midBuffer, 16);

            if (inBuffer[count] < 16)
            {
                //insert a zero if single digit
                _tcscat_s(SRAMBuffer, L"0");
            }

            // append the newText to the string buffer
            _tcscat_s(SRAMBuffer, midBuffer);

            //add a space between characters
            _tcscat_s(SRAMBuffer, L" ");
        }

    }

    //LPCWSTR ByteBuffer = L"test";
    //Dump the file data to the EditBox
    SetWindowTextW(hEdit, SRAMBuffer);

    //the command to end the read
    outBuffer[0] = (char)0x74;

    //Send the command to set SRAM address
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Write Command 0x70\r\nSet SRAM Range Error", ErrorBuffer, MB_OK);

        return;
    }

}

void FindMainBrain32(HWND hWnd)
{
    //If we are already connected then return
    if (deviceState == STATE_MACHINE::CONNECTED)
    {
        MessageBoxW(NULL, L"Already Connected!", L"Device Connect", MB_OK | MB_ICONSTOP);

        return;
    }

    dc = GetDC(hWnd);

    //SetupDiGetClassDevs - Get a list of present interface class devices
    DeviceInfoTable = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (DeviceInfoTable == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB SetupDiGetClassDevs\r\nInvalid Handle", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    //Set the font
    SelectObject(dc, hFont);

    //Display GUID since we passed SetupDiGetClassDevs OK
 /*   LPCWSTR c = L"Our GUID:";
    wchar_t buffer[256];
    int rt = StringFromGUID2(InterfaceClassGuid, buffer, _countof(buffer));

    if (rt > 0)
    {
        //TextOut(dc, hpos, vpos, c, _tcslen(c));
        //vpos = vpos + 20;
        //TextOut(dc, hpos, vpos, (LPCWSTR)buffer, _tcslen(buffer));
        //vpos = vpos + 40;
    }
    */

    InterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    c = L"Devices:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20;

    //buffer for the index conversion to string
    wchar_t index_buffer[20] = { 0 };

    InterfaceIndex = 0;

    while (SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL, &InterfaceClassGuid, InterfaceIndex, InterfaceDataStructure))
    {
        wchar_t buffer[256];
        int rt = StringFromGUID2(InterfaceDataStructure->InterfaceClassGuid, buffer, _countof(buffer));
        
        //convert index to string
        _itow_s(InterfaceIndex, index_buffer, 10);

        if (rt)
        {
            c = L"Index";
            TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
            hpos = hpos + 40;

            TextOut(dc, hpos, vpos, index_buffer, (int)_tcslen(index_buffer));
            hpos = hpos + 10;

            TextOut(dc, hpos, vpos, (LPCWSTR)buffer, (int)_tcslen(buffer));
            vpos = vpos + 20;
            hpos = hpos - 50;
        }

        InterfaceIndex++;
    }

    vpos = vpos + 20;
    Error = GetLastError();
    //Check to see if the list has been completely read
    if (Error == ERROR_NO_MORE_ITEMS)
    {
        //but if index is 0 that means no items were in the list
        if (!InterfaceIndex)
        {
            LPCWSTR c = L"No matching interfaces found!";
            TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
            vpos = vpos + 20;
            SetupDiDestroyDeviceInfoList(DeviceInfoTable);

            return;
        }
    }
    //Else some other kind of unknown error ocurred...
    else
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"SetupDiEnumDeviceInterfaces\r\nUnknown error", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    //adjust for last increment
    InterfaceIndex = InterfaceIndex - 1;

    if (InterfaceIndex > 0)
    {
        c = L"Enter the device Index to connect ...";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        vpos = vpos + 20;
        DeviceIndex = -1;
    }
    else
    {
        c = L"Press any key to connect";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        vpos = vpos + 20;
    }

    deviceState = STATE_MACHINE::NOT_CONNECTED;

}

void SendIsConnected(HWND hWnd)
{
    //Send data to device indicating we are connected
    char outBuffer[64] = { 0 };
    ULONG bytesSent;;
    BOOL Status;

    if (deviceState == STATE_MACHINE::NOT_CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    SelectObject(dc, hFont);

    vpos = 10;
    hpos = 15;

    //setup header

    SetTextColor(dc, RGB(0, 0, 255));

    //set the command
    outBuffer[0] = 0x62;

    //Send the command to get register data
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Send Command 0x62\r\nNotify Connected is True", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }
}

void SendNotConnected(HWND hWnd)
{
    //Send data to device indicating we are connected
    char outBuffer[64] = { 0 };
    ULONG bytesSent;;
    BOOL Status;

    if (deviceState == STATE_MACHINE::NOT_CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    SelectObject(dc, hFont);

    vpos = 10;
    hpos = 15;

    //setup header

    SetTextColor(dc, RGB(0, 0, 255));

    //set the command
    outBuffer[0] = 0x63;

    //Send the command to get register data
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Send Command 0x63\r\nNotify Connected is False", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }
}

void GetDeviceInfo(HWND hWnd)
{

    unsigned char inBuffer[64] = { 0 };
    char outBuffer[64] = { 0 };
    LPCWSTR c;
    ULONG bytesSent;;
    ULONG BytesTransferred;

    BOOL Status;

    if (deviceState == STATE_MACHINE::NOT_CONNECTED)
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DeviceState);
        //MessageBoxW(NULL, (LPWSTR)buffer, L"MsgFromTextBox", MB_OK);

        return;
    }

    ShowWindow(GetDlgItem(hWnd, IDC_LISTBOX), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, ProgramListTitleText), SW_HIDE);
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);
    SelectObject(dc, hFont);

    vpos = 10;
    hpos = 15;

    //setup header

    SetTextColor(dc, RGB(0, 0, 255));

    //We must set the beginning address and length of the SRAM data to read
    outBuffer[0] = 0x71;

    //Send the command to get register data
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == FALSE)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Send Command 0x71\r\nGet Device Information", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    outBuffer[0] = (char)0x81;

    //Send the command to read device data
    Status = WinUsb_WritePipe(MyWinUSBInterfaceHandle, 0x01, (PUCHAR)outBuffer, 64, &bytesSent, NULL);

    if (Status == TRUE)
    {
        WinUsb_ReadPipe(MyWinUSBInterfaceHandle, 0x81, &inBuffer[0], 64, &BytesTransferred, NULL);
    }
    else
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Read Command 0x81\r\nGet Device Information", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    int i;
    //Device Name
    for (i = 0; i < 10; i++)
    {
        MyBuffer[i] = inBuffer[i];
    }

    hpos = 10;
    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));

    //Device Revision
    for (i = 0; i < 9; i++)
    {
        MyBuffer[i] = inBuffer[i + 10];
    }

    hpos = hpos + 70;

    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));

    hpos = hpos + 80;

    //Device Processor
    for (i = 0; i < 16; i++)
    {
        MyBuffer[i] = inBuffer[i + 19];
    }

    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));

    hpos = hpos + 140;

    //Processor Revision
    c = L"Rev. ";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));

    hpos = hpos + 30;

    _itow_s(inBuffer[35], MyBuffer, 10, 16);
    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));

    vpos = vpos + 30; 
    hpos = 10;


    //OSCCON Data
    c = L"OSCCON:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 55;

    SetTextColor(dc, RGB(245, 0, 0));

    c = L"0x";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 15;

    if (inBuffer[41] < 10)
    {
        c = L"0";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        hpos = hpos + 8;
        _itow_s(inBuffer[41], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 8;
    }
    else
    {
        _itow_s(inBuffer[41], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 15;
    }

    if (inBuffer[40] < 10)
    {
        c = L"0";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        hpos = hpos + 8;
        _itow_s(inBuffer[40], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 8;
    }
    else
    {
        _itow_s(inBuffer[40], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 15;
    }

    if (inBuffer[39] < 10)
    {
        c = L"0";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        hpos = hpos + 8;
        _itow_s(inBuffer[39], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 8;
    }
    else
    {
        _itow_s(inBuffer[39], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 15;
    }

    if (inBuffer[38] < 10)
    {
        c = L"0";
        TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
        hpos = hpos + 8;
        _itow_s(inBuffer[38], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 8;
    }
    else
    {
        _itow_s(inBuffer[38], MyBuffer, 10, 16);
        TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
        hpos = hpos + 15;
    }

    SetTextColor(dc, RGB(0, 0, 255));

    //OSC1 input frequency
    hpos = 170;
    //vpos = vpos + 20;
    c = L"OSC1 FREQ:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 75;

    SetTextColor(dc, RGB(255, 0, 0));

    _itow_s(inBuffer[42], MyBuffer, 10, 10);
    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
    hpos = hpos + 15;

    c = L"MHz";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));

    hpos = 10;
    vpos = vpos + 20;
    SetTextColor(dc, RGB(0, 0, 255));

    c = L"Pri OSC FREQ:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));

    hpos = hpos + 95;

    SetTextColor(dc, RGB(255, 0, 0));
    
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int POSC_Frequency;

    //determine the PLL in divisor value
    if (inBuffer[45] == 0)
    {
        t3 = 1;
    }
    if (inBuffer[45] == 1)
    {
        t3 = 2;
    }
    if (inBuffer[45] == 2)
    {
        t3 = 3;
    }
    if (inBuffer[45] == 3)
    {
        t3 = 4;
    }
    if (inBuffer[45] == 4)
    {
        t3 = 5;
    }
    if (inBuffer[45] == 5)
    {
        t3 = 6;
    }
    if (inBuffer[45] == 6)
    {
        t3 = 10;
    }
    if (inBuffer[45] == 7)
    {
        t3 = 12;
    }

    //Determine the PLL multiplier
    if (inBuffer[44] == 0)
    {
        t2 = 15;
    }
    if (inBuffer[44] == 1)
    {
        t2 = 16;
    }
    if (inBuffer[44] == 2)
    {
        t2 = 17;
    }
    if (inBuffer[44] == 3)
    {
        t2 = 18;
    }
    if (inBuffer[44] == 4)
    {
        t2 = 19;
    }
    if (inBuffer[44] == 5)
    {
        t2 = 20;
    }
    if (inBuffer[44] == 6)
    {
        t2 = 21;
    }
    if (inBuffer[44] == 7)
    {
        t2 = 24;
    }

    //Determine the PLL out divisor
    if (inBuffer[43] == 0)
    {
        t1 = 1;
    }
    if (inBuffer[43] == 1)
    {
        t1 = 2;
    }
    if (inBuffer[43] == 2)
    {
        t1 = 4;
    }
    if (inBuffer[43] == 3)
    {
        t1 = 8;
    }
    if (inBuffer[43] == 4)
    {
        t1 = 16;
    }
    if (inBuffer[43] == 5)
    {
        t1 = 32;
    }
    if (inBuffer[43] == 6)
    {
        t1 = 64;
    }
    if (inBuffer[43] == 7)
    {
        t1 = 256;
    }

    POSC_Frequency = ((inBuffer[42] / t3) * t2) / t1;

    _itow_s(POSC_Frequency, MyBuffer, 10, 10);
    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
    hpos = hpos + 15;

    c = L"MHz";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));

    SetTextColor(dc, RGB(0, 0, 255));

    hpos = 10;
    vpos = vpos + 20;

   //Primary Oscillator Mode
    c = L"Pri OSC Mode:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));

    hpos = hpos + 100;

    if (inBuffer[36] == 0)
    {
        c = L"EC";
    }
    if (inBuffer[36] == 1)
    {
        c = L"XT";
    }
    if (inBuffer[36] == 2)
    {
        c = L"HS";
    }
    if (inBuffer[36] == 3)
    {
        c = L"Disabled";
    }

    SetTextColor(dc, RGB(255, 0, 0));

    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 15;

    SetTextColor(dc, RGB(0, 0, 255));
    
    vpos = vpos - 20;
    hpos = 170;
    //Primary Oscillator Source
    c = L"Pri OSC Source:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c)); 

    hpos = hpos + 115;

    if (inBuffer[37] == 0)
    {
        c = L"FRC";
    }
    if (inBuffer[37] == 1)
    {
        c = L"FRC DIV + PLL";
    }
    if (inBuffer[37] == 2)
    {
        c = L"PRI OSC";
    }
    if (inBuffer[37] == 3)
    {
        c = L"PRI OSC + PLL";
    }
    if (inBuffer[37] == 4)
    {
        c = L"SEC OSC";
    }
    if (inBuffer[37] == 5)
    {
        c = L"LPRC";
    }
    if (inBuffer[37] == 6)
    {
        c = L"FRCDIV16";
    }
    if (inBuffer[37] == 7)
    {
        c = L"FRCDIV";
    }
 
    SetTextColor(dc, RGB(255, 0, 0));

    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 15;

    SetTextColor(dc, RGB(0, 0, 255));
}


void SelectDevice(HWND hWnd, BYTE DeviceIndex)
{
    
    //dc = GetDC(hWnd);
    //SelectObject(dc, hFont);

    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);
   

    vpos = 10;
    LPCWSTR c = L"MainBrain 32";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20;

    //Now retrieve the hardware ID from the registry.  The hardware ID contains the VID and PID, which we will then 
    //check to see if it is the correct device or not.
        //Initialize an appropriate SP_DEVINFO_DATA structure.  We need this structure for SetupDiGetDeviceRegistryProperty().
    SP_DEVINFO_DATA DevInfoData;

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(DeviceInfoTable, DeviceIndex, &DevInfoData))
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"SetupDiEnumDeviceInfo\r\nSelect Listed Device", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        SetupDiDestroyDeviceInfoList(DeviceInfoTable);
        return;
    }

    //Lets look at the Device Instance Handle
    c = L"Device Node:";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    hpos = hpos + 100;

    _itow_s(DevInfoData.DevInst, MyBuffer, 10);
    TextOut(dc, hpos, vpos, MyBuffer, (int)_tcslen(MyBuffer));
    vpos = vpos + 20;
    hpos = hpos - 100;


    //First query for the size of the hardware ID, so we can know how big a buffer to allocate for the data.
    //if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_CONFIGFLAGS, &RegDataType, (PBYTE)&ConfigFlags, sizeof(ConfigFlags), NULL)
    DWORD requested_size;
    DWORD dwRegType;

    SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, NULL, 0, &requested_size);

    //Allocate a buffer for the hardware ID.
    PBYTE PropertyValueBuffer;

    PropertyValueBuffer = (BYTE*)malloc(requested_size);

    //if null, error, couldn't allocate enough memory
    if (PropertyValueBuffer == NULL)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"SetupDiGetDeviceRegistryProperty\r\nCouldn't Allocate Enough Memory", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    //Retrieve the hardware IDs for the current device we are looking at.  PropertyValueBuffer gets filled with a 
    //REG_MULTI_SZ (array of null terminated strings).  To find a device, we only care about the very first string in the
    //buffer, which will be the "device ID".  The device ID is a string which contains the VID and PID, in the example 
    //format "Vid_04d8&Pid_003f".
    SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, PropertyValueBuffer, requested_size, NULL);

    //Hardware ID
    c = (LPCWSTR)PropertyValueBuffer;
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20;

    SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, &dwRegType, PropertyValueBuffer, requested_size, NULL);

    //Device Object Name
    c = (LPCWSTR)PropertyValueBuffer;
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20;

    std::free(PropertyValueBuffer);

    //Open WinUSB interface handle now.
    //In order to do this, we will need the actual device path first.
    //We can get the path by calling SetupDiGetDeviceInterfaceDetail(), however, we have to call this function twice:  The first
    //time to get the size of the required structure/buffer to hold the detailed interface data, then a second time to actually 
    //get the structure (after we have allocated enough memory for the structure.)
    DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    DWORD StructureSize = 0;

    //First call populates "StructureSize" with the correct value
    SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, NULL, NULL, &StructureSize, NULL);

    //Allocate enough memory
    DetailedInterfaceDataStructure = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(malloc(StructureSize));

    //if null, error, couldn't allocate enough memory
    if (DetailedInterfaceDataStructure == NULL)
    {
        //Can't really recover from this situation, just exit instead.
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"DetailedInterfaceDataStructure\r\nCouldn't Allocate Enough Memory", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    //Specify the Index/Node to connect to
    SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL, &InterfaceClassGuid, DeviceIndex, InterfaceDataStructure);


    //Now call SetupDiGetDeviceInterfaceDetail() again to get the data
    SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, DetailedInterfaceDataStructure, StructureSize, NULL, NULL);

    //Device Path
    c = (LPCWSTR)DetailedInterfaceDataStructure->DevicePath;
    //TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20;

    //We now have the proper device path, and we can finally open a device handle to the device.
    //WinUSB requires the device handle to be opened with the FILE_FLAG_OVERLAPPED attribute.
    SetupDiDestroyDeviceInfoList(DeviceInfoTable);

    //WinUSB requires the device handle to be opened with the FILE_FLAG_OVERLAPPED attribute.
    MyDeviceHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

    Error = GetLastError();
    if (Error > 0)
    {
        Error = GetLastError();
        _itow_s(Error, ErrorBuffer, 10);
        MessageBoxW(NULL, L"USB Create Device Handle\r\nInvalid Handle", ErrorBuffer, MB_OK);

        deviceState = STATE_MACHINE::NOT_CONNECTED;

        return;
    }

    //Now get the WinUSB interface handle by calling WinUsb_Initialize() and providing the device handle.
    WinUsb_Initialize(MyDeviceHandle, &MyWinUSBInterfaceHandle);

    //We are now ready to exchange data with our USB device

    //Set the state machine to show we are connected
    deviceState = STATE_MACHINE::CONNECTED;
    EnableMenuItem(GetMenu(hWnd), ID_CONNECT, MF_BYCOMMAND | MF_GRAYED);
    
    //Write to screen
    c = L"Device Connected";
    TextOut(dc, hpos, vpos, c, (int)_tcslen(c));
    vpos = vpos + 20; 

    //Show these controls now that we are connected
    //ShowWindow(GetDlgItem(hWnd, GetADCCurrentProgressBar), SW_SHOW);
    SetTimer(hWnd, GetADCTimer, 1000, (TIMERPROC)GetADCCurrentSense);
    ShowWindow(GetDlgItem(hWnd, BackLightSliderText), SW_SHOW);
    ShowWindow(GetDlgItem(hWnd, ProgressBarVText), SW_SHOW);
    ShowWindow(GetDlgItem(hWnd, ProgressBarMAText), SW_SHOW); 
    ShowWindow(GetDlgItem(hWnd, ProgressBarText), SW_SHOW);
    EnableMenuItem(GetMenu(hWnd), ID_PROGRAMS, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(GetMenu(hWnd), ID_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), ID_FILE_WRITE, MF_BYCOMMAND | MF_GRAYED);
    SendIsConnected(hWnd);
    SetDeviceDisplayBackLight(hWnd, 5);
    SendMessageW(GetDlgItem(hWnd, BackLightSlider), TBM_SETPOS, TRUE, 5);

    wchar_t buf[4];
    wsprintfW(buf, L"%ld", 5);
    SetWindowTextW(GetDlgItem(hWnd, BackLightSliderText), buf);
    EnableWindow(GetDlgItem(hWnd, BackLightSlider), TRUE);

    //update the time
    SetTime(hWnd);

    return;
}

