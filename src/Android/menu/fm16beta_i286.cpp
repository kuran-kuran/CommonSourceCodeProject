//
// Created by medamap on 2024/04/10.
//

#include "menu.h"
#include "BaseMenu.h"
#include "../../res/resource.h"

// BaseMenu を継承して Menu クラスを作成する
Menu::Menu() {

    // Root メニューを作成
    int rootId = addNode(0, "Root", Category, -1);
    int controlId = addNode(rootId, "Control", Category, -1);
    int fd1mbId1 = addNode(rootId, "1MB-FD1", Category, -1);
    int fd1mbId2 = addNode(rootId, "FD2", Category, -1);
    int fd320kbId1 = addNode(rootId, "320KB-FD1", Category, -1);
    int fd320kbId2 = addNode(rootId, "FD2", Category, -1);
    int deviceId = addNode(rootId, "Device", Category, -1);
    int hostId = addNode(rootId, "Host", Category, -1);

    // Control メニューを作成
    addNode(controlId, "Reset", Property, ID_RESET);
    addNode(controlId, "NMI", Property, ID_SPECIAL_RESET);
    addNode(controlId, "CPU x1", Property, ID_CPU_POWER0);
    addNode(controlId, "CPU x2", Property, ID_CPU_POWER1);
    addNode(controlId, "CPU x4", Property, ID_CPU_POWER2);
    addNode(controlId, "CPU x8", Property, ID_CPU_POWER3);
    addNode(controlId, "CPU x16", Property, ID_CPU_POWER4);
    addNode(controlId, "Full Speed", Property, ID_FULL_SPEED);
    addNode(controlId, "Drive VM in M1/R/W Cycle", Property, ID_DRIVE_VM_IN_OPECODE);
    addNode(controlId, "Paste", Property, ID_AUTOKEY_START);
    addNode(controlId, "Stop", Property, ID_AUTOKEY_STOP);
    addNode(controlId, "Romaji to Kana", Property, ID_ROMAJI_TO_KANA);
    int saveStateId = addNode(controlId, "Save State", Category, -1);
    for (int i = 0; i < 10; i++) {
        addNode(saveStateId, "State " + std::to_string(i), Property, ID_SAVE_STATE0 + i);
    }
    int loadStateId = addNode(controlId, "Load State", Category, -1);
    for (int i = 0; i < 10; i++) {
        addNode(loadStateId, "State " + std::to_string(i), Property, ID_LOAD_STATE0 + i);
    }
    addNode(controlId, "Debug Main CPU", Property, ID_OPEN_DEBUGGER0);
    addNode(controlId, "Debug Sub CPU", Property, ID_OPEN_DEBUGGER1);
    addNode(controlId, "Close Debugger", Property, ID_CLOSE_DEBUGGER);
    addNode(controlId, "Exit", Property, ID_EXIT);

    // 1MB-FD1 メニューを作成
    addNode(fd1mbId1, "Insert", Property, ID_OPEN_FD1);
    addNode(fd1mbId1, "Eject", Property, ID_CLOSE_FD1);
    addNode(fd1mbId1, "Insert Blank 2HD Disk", Property, ID_OPEN_BLANK_2HD_FD1);
    addNode(fd1mbId1, "Write Protected", Property, ID_WRITE_PROTECT_FD1);
    addNode(fd1mbId1, "Correct Timing", Property, ID_CORRECT_TIMING_FD1);
    addNode(fd1mbId1, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD1);
    addNode(fd1mbId1, "Recent", Property, ID_RECENT_FD1);

    // FD2 メニューを作成
    addNode(fd1mbId2, "Insert", Property, ID_OPEN_FD2);
    addNode(fd1mbId2, "Eject", Property, ID_CLOSE_FD2);
    addNode(fd1mbId2, "Insert Blank 2HD Disk", Property, ID_OPEN_BLANK_2HD_FD2);
    addNode(fd1mbId2, "Write Protected", Property, ID_WRITE_PROTECT_FD2);
    addNode(fd1mbId2, "Correct Timing", Property, ID_CORRECT_TIMING_FD2);
    addNode(fd1mbId2, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD2);
    addNode(fd1mbId2, "Recent", Property, ID_RECENT_FD2);

    // 320KB-FD1 メニューを作成
    addNode(fd320kbId1, "Insert", Property, ID_OPEN_FD3);
    addNode(fd320kbId1, "Eject", Property, ID_CLOSE_FD3);
    addNode(fd320kbId1, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD3);
    addNode(fd320kbId1, "Write Protected", Property, ID_WRITE_PROTECT_FD3);
    addNode(fd320kbId1, "Correct Timing", Property, ID_CORRECT_TIMING_FD3);
    addNode(fd320kbId1, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD3);
    addNode(fd320kbId1, "Recent", Property, ID_RECENT_FD3);

    // FD2 メニューを作成
    addNode(fd320kbId2, "Insert", Property, ID_OPEN_FD4);
    addNode(fd320kbId2, "Eject", Property, ID_CLOSE_FD4);
    addNode(fd320kbId2, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD4);
    addNode(fd320kbId2, "Write Protected", Property, ID_WRITE_PROTECT_FD4);
    addNode(fd320kbId2, "Correct Timing", Property, ID_CORRECT_TIMING_FD4);
    addNode(fd320kbId2, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD4);
    addNode(fd320kbId2, "Recent", Property, ID_RECENT_FD4);

    // Device メニューを作成
    int soundId = addNode(deviceId, "Sound", Category, -1);

    addNode(soundId, "Play FDD Noise", Property, ID_VM_SOUND_NOISE_FDD);
    addNode(soundId, "Play CMT Noise", Property, ID_VM_SOUND_NOISE_CMT);
    addNode(soundId, "Play CMT Signal", Property, ID_VM_SOUND_TAPE_SIGNAL);
    addNode(soundId, "Play CMT Voice", Property, ID_VM_SOUND_TAPE_VOICE);

    // Host メニューを作成
    addNode(hostId, "Rec Movie 60fps", Property, ID_HOST_REC_MOVIE_60FPS);
    addNode(hostId, "Rec Movie 30fps", Property, ID_HOST_REC_MOVIE_30FPS);
    addNode(hostId, "Rec Movie 15fps", Property, ID_HOST_REC_MOVIE_15FPS);
    addNode(hostId, "Rec Sound", Property, ID_HOST_REC_SOUND);
    addNode(hostId, "Stop", Property, ID_HOST_REC_STOP);
    addNode(hostId, "Capture Screen", Property, ID_HOST_CAPTURE_SCREEN);

    int screenId = addNode(hostId, "Screen", Category, -1);
    addNode(screenId, "Window x1", Property, ID_SCREEN_WINDOW);
    addNode(screenId, "Fullscreen 640x400", Property, ID_SCREEN_FULLSCREEN);
    addNode(screenId, "Window Stretch 1", Property, ID_SCREEN_WINDOW_STRETCH);
    addNode(screenId, "Window Stretch 2", Property, ID_SCREEN_WINDOW_ASPECT);
    addNode(screenId, "Fullscreen Stretch 1", Property, ID_SCREEN_FULLSCREEN_DOTBYDOT);
    addNode(screenId, "Fullscreen Stretch 2", Property, ID_SCREEN_FULLSCREEN_STRETCH);
    addNode(screenId, "Fullscreen Stretch 3", Property, ID_SCREEN_FULLSCREEN_ASPECT);
    addNode(screenId, "Fullscreen Stretch 4", Property, ID_SCREEN_FULLSCREEN_FILL);
    addNode(screenId, "Rotate 0deg", Property, ID_SCREEN_ROTATE_0);
    addNode(screenId, "Rotate +90deg", Property, ID_SCREEN_ROTATE_90);
    addNode(screenId, "Rotate 180deg", Property, ID_SCREEN_ROTATE_180);
    addNode(screenId, "Rotate -90deg", Property, ID_SCREEN_ROTATE_270);

    int marginId = addNode(screenId, "Screen Bottom Margin", Category, -1);
    addNode(marginId, "0", Property, ID_SCREEN_BOTTOM_MARGIN_0);
    addNode(marginId, "30", Property, ID_SCREEN_BOTTOM_MARGIN_30);
    addNode(marginId, "60", Property, ID_SCREEN_BOTTOM_MARGIN_60);
    addNode(marginId, "90", Property, ID_SCREEN_BOTTOM_MARGIN_90);
    addNode(marginId, "120", Property, ID_SCREEN_BOTTOM_MARGIN_120);
    addNode(marginId, "150", Property, ID_SCREEN_BOTTOM_MARGIN_150);
    addNode(marginId, "180", Property, ID_SCREEN_BOTTOM_MARGIN_180);
    addNode(marginId, "210", Property, ID_SCREEN_BOTTOM_MARGIN_210);
    addNode(marginId, "240", Property, ID_SCREEN_BOTTOM_MARGIN_240);
    addNode(marginId, "270", Property, ID_SCREEN_BOTTOM_MARGIN_270);

    int filterId = addNode(hostId, "Filter", Category, -1);
    addNode(filterId, "RGB Filter", Property, ID_FILTER_RGB);
    addNode(filterId, "None", Property, ID_FILTER_NONE);

    int soundId2 = addNode(hostId, "Sound", Category, -1);
    addNode(soundId2, "Switch On / Off", Property, ID_SOUND_ON);
    addNode(soundId2, "2000Hz", Property, ID_SOUND_FREQ0);
    addNode(soundId2, "4000Hz", Property, ID_SOUND_FREQ1);
    addNode(soundId2, "8000Hz", Property, ID_SOUND_FREQ2);
    addNode(soundId2, "11025Hz", Property, ID_SOUND_FREQ3);
    addNode(soundId2, "22050Hz", Property, ID_SOUND_FREQ4);
    addNode(soundId2, "44100Hz", Property, ID_SOUND_FREQ5);
    addNode(soundId2, "62500Hz", Property, ID_SOUND_FREQ6);
    addNode(soundId2, "96000Hz", Property, ID_SOUND_FREQ7);
    addNode(soundId2, "50msec", Property, ID_SOUND_LATE0);
    addNode(soundId2, "100msec", Property, ID_SOUND_LATE1);
    addNode(soundId2, "200msec", Property, ID_SOUND_LATE2);
    addNode(soundId2, "300msec", Property, ID_SOUND_LATE3);
    addNode(soundId2, "400msec", Property, ID_SOUND_LATE4);
    addNode(soundId2, "Realtime Mix", Property, ID_SOUND_STRICT_RENDER);
    addNode(soundId2, "Light Weight Mix", Property, ID_SOUND_LIGHT_RENDER);
    addNode(soundId2, "Volume", Property, ID_SOUND_VOLUME);

    addNode(hostId, "Use Direct2D1", Property, ID_HOST_USE_D2D1);
    addNode(hostId, "Use Direct3D9", Property, ID_HOST_USE_D3D9);
    addNode(hostId, "Wait Vsync", Property, ID_HOST_WAIT_VSYNC);
    addNode(hostId, "Use DirectInput", Property, ID_HOST_USE_DINPUT);
    addNode(hostId, "Disable Windows 8 DWM", Property, ID_HOST_DISABLE_DWM);
    addNode(hostId, "Show Status Bar", Property, ID_HOST_SHOW_STATUS_BAR);
}

/*
IDR_MENU1 MENU DISCARDABLE
BEGIN
    POPUP "Host"
    BEGIN
        MENUITEM "Rec Movie 60fps",             ID_HOST_REC_MOVIE_60FPS
        MENUITEM "Rec Movie 30fps",             ID_HOST_REC_MOVIE_30FPS
        MENUITEM "Rec Movie 15fps",             ID_HOST_REC_MOVIE_15FPS
        MENUITEM "Rec Sound",                   ID_HOST_REC_SOUND
        MENUITEM "Stop",                        ID_HOST_REC_STOP
        MENUITEM "Capture Screen",              ID_HOST_CAPTURE_SCREEN
        MENUITEM SEPARATOR
        POPUP "Screen"
        BEGIN
            MENUITEM "Window x1",               ID_SCREEN_WINDOW
            MENUITEM "Fullscreen 640x400",      ID_SCREEN_FULLSCREEN
            MENUITEM SEPARATOR
            MENUITEM "Fullscreen Stretch 1",    ID_SCREEN_FULLSCREEN_DOTBYDOT
            MENUITEM "Fullscreen Stretch 2",    ID_SCREEN_FULLSCREEN_STRETCH
            MENUITEM "Fullscreen Stretch 4",    ID_SCREEN_FULLSCREEN_FILL
            MENUITEM SEPARATOR
            MENUITEM "Rotate 0deg",             ID_SCREEN_ROTATE_0
            MENUITEM "Rotate +90deg",           ID_SCREEN_ROTATE_90
            MENUITEM "Rotate 180deg",           ID_SCREEN_ROTATE_180
            MENUITEM "Rotate -90deg",           ID_SCREEN_ROTATE_270
        END
        POPUP "Sound"
        BEGIN
            MENUITEM "2000Hz",                  ID_SOUND_FREQ0
            MENUITEM "4000Hz",                  ID_SOUND_FREQ1
            MENUITEM "8000Hz",                  ID_SOUND_FREQ2
            MENUITEM "11025Hz",                 ID_SOUND_FREQ3
            MENUITEM "22050Hz",                 ID_SOUND_FREQ4
            MENUITEM "44100Hz",                 ID_SOUND_FREQ5
            MENUITEM "48000Hz",                 ID_SOUND_FREQ6
            MENUITEM "96000Hz",                 ID_SOUND_FREQ7
            MENUITEM SEPARATOR
            MENUITEM "50msec",                  ID_SOUND_LATE0
            MENUITEM "100msec",                 ID_SOUND_LATE1
            MENUITEM "200msec",                 ID_SOUND_LATE2
            MENUITEM "300msec",                 ID_SOUND_LATE3
            MENUITEM "400msec",                 ID_SOUND_LATE4
            MENUITEM SEPARATOR
            MENUITEM "Realtime Mix",            ID_SOUND_STRICT_RENDER
            MENUITEM "Light Weight Mix",        ID_SOUND_LIGHT_RENDER
            MENUITEM SEPARATOR
            MENUITEM "Volume",                  ID_SOUND_VOLUME
        END
        MENUITEM SEPARATOR
        MENUITEM "Use Direct2D1",               ID_HOST_USE_D2D1
        MENUITEM "Use Direct3D9",               ID_HOST_USE_D3D9
        MENUITEM "Wait Vsync",                  ID_HOST_WAIT_VSYNC
        MENUITEM "Use DirectInput",             ID_HOST_USE_DINPUT
        MENUITEM "Disable Windows 8 DWM",       ID_HOST_DISABLE_DWM
        MENUITEM "Show Status Bar",             ID_HOST_SHOW_STATUS_BAR
    END
END

 */