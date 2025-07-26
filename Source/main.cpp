#include "pch.h"
#include "ClickRecorder.hpp"

int main()
{
    std::cout << "Controls:\n";
    std::cout << "\n";
    std::cout << "  Ctrl+R : start / stop recording\n";
    std::cout << "\n";
    std::cout << "  Ctrl+I : playback file1\n";
    std::cout << "  Ctrl+O : playback file2\n";
    std::cout << "  Ctrl+P : playback file3\n";
    std::cout << "  Ctrl+P : stop on-going playback\n";
    std::cout << "\n";
    std::cout << "  Ctrl+E : End program\n";
    std::cout << "\n";

    if( !InitClickRecorder() ) {
        std::cerr << "Click Recorder Init Failed\n";
        return 1;
    }

    MSG msg;
    bool forcedEnded = false;
    while( GetMessage( &msg, NULL, 0, 0 ) ) {
        if( msg.message == WM_HOTKEY ) {
            // Ctrl+R
            if( msg.wParam == 1 ) { 
                StartRecording( forcedEnded );
            }
            // Ctrl+I
            if( msg.wParam == 2 ) { 
                PlaybackClicks( 0, forcedEnded );
            }
            // Ctrl+O
            if( msg.wParam == 3 ) {
                PlaybackClicks( 1, forcedEnded );
            }
            // Ctrl+P
            if( msg.wParam == 4 ) {
                PlaybackClicks( 2, forcedEnded );
            }
            // Ctrl+E
            if( msg.wParam == 6 ) {
                if( mouseHook != NULL ) {
                    UnhookWindowsHookEx( mouseHook );
                    mouseHook = NULL;
                }
                std::string soundFile = GetExecutablePath() + "\\Sounds\\" + "Ended.wav";
                PlaySound( ConvertStringToWString( soundFile ).c_str(), NULL, SND_FILENAME );
                break;
            }
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );

        if( forcedEnded ) {
            break;
        }
    }

    if( mouseHook != NULL ) {
        UnhookWindowsHookEx( mouseHook );
        mouseHook = NULL;
    }

    // Unregister hotkeys
    UnregisterHotKey( NULL, 1 );
    UnregisterHotKey( NULL, 2 );

    return 0;
}