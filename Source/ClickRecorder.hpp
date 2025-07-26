#pragma once
#include "pch.h"
#include "FileHandler.hpp"

struct ClickInfo
{
    POINT position;
    DWORD timestamp;
    bool isRightClick;
    bool isShiftPressed;
};

struct Display
{
    UINT64 Width;
    UINT64 Height;
} mainDisplay;

std::vector<ClickInfo> recordedClicks;
std::vector<std::string> playbackFiles;
bool recording = false;
DWORD curRecordStartTime = 0;
HHOOK mouseHook = NULL;


// Function to save clicks to a .txt file
void SaveClicksToFile( const std::vector<ClickInfo>& clicks, const std::string& baseFilename )
{
    std::string filename = GenerateUniqueFilename( baseFilename );
    std::ofstream outFile( filename );

    if( !outFile ) {
        std::cerr << "Failed to open file for writing: " << filename << "\n";
        return;
    }

    for( const auto& click : clicks ) {
        outFile << click.position.x << " "
            << click.position.y << " "
            << click.timestamp << " "
            << click.isRightClick << " "
            << click.isShiftPressed << "\n";
    }

    outFile.close();
    std::cout << "Clicks saved to " << filename << "\n";
}

// Function to load clicks from a .txt file
std::vector<ClickInfo> LoadClicksFromFile( const std::string& filename )
{
    std::vector<ClickInfo> clicks;
    std::ifstream inFile( filename );
    if( !inFile ) {
        std::cerr << "Failed to open file for reading: " << filename << "\n";
        return clicks;
    }

    ClickInfo click;
    while( inFile >> click.position.x >> click.position.y >> click.timestamp >> click.isRightClick >> click.isShiftPressed ) {
        clicks.push_back( click );
    }

    inFile.close();
    std::cout << "Clicks loaded from " << filename << "\n";
    return clicks;
}

LRESULT CALLBACK MouseHookProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    if( nCode >= 0 && ( wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ) && recording ) {

        MSLLHOOKSTRUCT* clickUInfo = (MSLLHOOKSTRUCT*)lParam;
        if( clickUInfo != nullptr ) {
            ClickInfo click;
            click.position = clickUInfo->pt;
            click.timestamp = GetTickCount64() - curRecordStartTime;
            click.isRightClick = ( wParam == WM_RBUTTONDOWN );
            click.isShiftPressed = ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) != 0;
            recordedClicks.push_back( click );
            /*std::cout << "Recorded " << ( click.isRightClick ? "right" : "left" ) << " click at ("
                << click.position.x << ", " << click.position.y << ")"
                << ( click.isShiftPressed ? " with Shift pressed.\n" : ".\n" );*/
        }
    }
    return CallNextHookEx( NULL, nCode, wParam, lParam );
}

void StopRecoding()
{
    recording = false;
    UnhookWindowsHookEx( mouseHook );
    mouseHook = NULL;

    // save the recording to file
    std::string filePath = GetExecutablePath() + "\\" + "ClickRecord.txt";
    SaveClicksToFile( recordedClicks, filePath );
    recordedClicks.clear();

    std::string soundFile = GetExecutablePath() + "\\Sounds\\" + "StopRecording.wav";
    PlaySound( ConvertStringToWString( soundFile ).c_str(), NULL, SND_FILENAME | SND_ASYNC );
}

void StartRecording( bool & forcedEnded )
{
    recordedClicks.clear();

    // Set the mouse hook to record mouse clicking
    if( mouseHook == NULL ) {
        mouseHook = SetWindowsHookEx( WH_MOUSE_LL, MouseHookProc, NULL, 0 );
        if( mouseHook == NULL ) {
            std::cerr << "Failed to install mouse hook! Error: " << GetLastError() << "\n";
            return;
        }
    }

    std::string soundFile = GetExecutablePath() + "\\Sounds\\" + "StartRecording.wav";
    PlaySound( ConvertStringToWString( soundFile ).c_str(), NULL, SND_FILENAME | SND_ASYNC );

    curRecordStartTime = GetTickCount64();
    recording = true;

    // Loop until recording stops
    MSG msg;
    while( GetMessage( &msg, NULL, 0, 0 ) ) {
        // Stop recording at another ctrl+r on-press
        if( msg.message == WM_HOTKEY && msg.wParam == 1) {
            StopRecoding();
            break;
        }
        // end process at ctrl+e
        else if( msg.message == WM_HOTKEY && msg.wParam == 6 ) { // ctrl+e
            forcedEnded = true;
            break;
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

void SendClickInfo( const ClickInfo & click )
{
    // Calculate the absolute pos
    int absoluteX = click.position.x * 65535 / mainDisplay.Width;
    int absoluteY = click.position.y * 65535 / mainDisplay.Height;

    // Simulate Shift key press if needed
    if( click.isShiftPressed ) {
        INPUT shiftInput = { 0 };
        shiftInput.type = INPUT_KEYBOARD;
        shiftInput.ki.wVk = VK_SHIFT;
        shiftInput.ki.dwFlags = 0; // Key down
        SendInput( 1, &shiftInput, sizeof( INPUT ) );
    }

    // Simulate mouse click
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = absoluteX;
    input.mi.dy = absoluteY;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    SendInput( 1, &input, sizeof( INPUT ) );

    input.mi.dwFlags = click.isRightClick ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;
    SendInput( 1, &input, sizeof( INPUT ) );

    input.mi.dwFlags = click.isRightClick ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;
    SendInput( 1, &input, sizeof( INPUT ) );

    // Simulate Shift key release if needed
    if( click.isShiftPressed ) {
        INPUT shiftInput = { 0 };
        shiftInput.type = INPUT_KEYBOARD;
        shiftInput.ki.wVk = VK_SHIFT;
        shiftInput.ki.dwFlags = KEYEVENTF_KEYUP; // Key up
        SendInput( 1, &shiftInput, sizeof( INPUT ) );
    }

}

void PlaybackClicks( UINT16 fileID, bool& forcedEnded )
{
    if( mouseHook != NULL ) {
        UnhookWindowsHookEx( mouseHook );
        mouseHook = NULL;
    }

    if( fileID >= playbackFiles.size() ) {
        // no playback file for this hotkey
        return;
    }
    std::vector<ClickInfo> clicks = LoadClicksFromFile( playbackFiles[ fileID ]);

    if( clicks.empty() ) {
        std::cout << "No clicks recorded.\n";
        return;
    }

    std::string soundFile = GetExecutablePath() + "\\Sounds\\" + "PlaybackStarted.wav";
    PlaySound( ConvertStringToWString( soundFile ).c_str(), NULL, SND_FILENAME | SND_ASYNC );

    size_t curClickIndex = 0;
    DWORD playbackStartTime = GetTickCount64();
    bool stopPlayback = false;

    // Loop over all clickInfo in the list
    while( curClickIndex < clicks.size() && !stopPlayback) {

        ClickInfo click = clicks[ curClickIndex ];

        // Only play the click when it has passed the same amount of time as the recorded timestamp
        if( ( GetTickCount64() - playbackStartTime ) >= click.timestamp ) {
            SendClickInfo( click );
            curClickIndex++;
        }

        // allow to stop the current playback
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY && msg.wParam == 5) { // ctrl+s
                stopPlayback = true;
            }
            if( msg.message == WM_HOTKEY && msg.wParam == 6 ) { // ctrl+e
                stopPlayback = true;
                forcedEnded = true;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    soundFile = GetExecutablePath() + "\\Sounds\\" + "PlaybackStopped.wav";
    PlaySound( ConvertStringToWString( soundFile ).c_str(), NULL, SND_FILENAME | SND_ASYNC );
}

BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData )
{
    Display* mainDisp = (Display*)dwData;
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof( monitorInfo );
    // Get size of main display
    if( GetMonitorInfo( hMonitor, &monitorInfo ) ) {
        if( monitorInfo.dwFlags & MONITORINFOF_PRIMARY ) {
            mainDisp->Width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            mainDisp->Height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        }
    }
    return TRUE;
}

bool InitClickRecorder()
{
    // Need to set the DPI awareness so that it can handle the case where the monitor's display is not 100%
    HRESULT hr = SetProcessDpiAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
    if( FAILED( hr ) ) {
        std::cerr << "Failed to set DPI awareness!\n";
        return false;
    }

    // Set the mouse hook to record mouse clicking
    mouseHook = SetWindowsHookEx( WH_MOUSE_LL, MouseHookProc, NULL, 0 );
    if( mouseHook == NULL ) {
        std::cerr << "Failed to install mouse hook! Error: " << GetLastError() << "\n";
        return false;
    }

    // Get the dimension of the main display
    EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, (LPARAM)&mainDisplay );

    // Register hotkeys
    if( !RegisterHotKey( NULL, 1, MOD_CONTROL, 'R' ) ) {
        std::cerr << "Failed to register Ctrl+R hotkey!\n";
        return false;
    }
    if( !RegisterHotKey( NULL, 2, MOD_CONTROL, 'I' ) ) {
        std::cerr << "Failed to register Ctrl+I hotkey!\n";
        return false;
    }
    if( !RegisterHotKey( NULL, 3, MOD_CONTROL, 'O' ) ) {
        std::cerr << "Failed to register Ctrl+O hotkey!\n";
        return false;
    }
    if( !RegisterHotKey( NULL, 4, MOD_CONTROL, 'P' ) ) {
        std::cerr << "Failed to register Ctrl+P hotkey!\n";
        return false;
    }
    if( !RegisterHotKey( NULL, 5, MOD_CONTROL, 'S' ) ) {
        std::cerr << "Failed to register Ctrl+S hotkey!\n";
        return false;
    }
    if( !RegisterHotKey( NULL, 6, MOD_CONTROL, 'E' ) ) {
        std::cerr << "Failed to register Ctrl+E hotkey!\n";
        return false;
    }

    // read in config.txt
    std::string config = GetExecutablePath() + "\\config.txt";
    playbackFiles = ParseConfigFile( config );
    for( auto f : playbackFiles ) {
        std::cout << "User defined playback file: " << f << std::endl;
    }

    return true;
}