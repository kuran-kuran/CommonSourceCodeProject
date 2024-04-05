//
// Created by medamap on 2024/04/03.
//

#include "menu.h"
#include "BaseMenu.h"
#include "../../res/resource.h"

// BaseMenu を継承して Menu クラスを作成する
Menu::Menu() {
    // Root メニューを作成
    int rootId = addNode(0, "Root", Category, -1);
    int controlId = addNode(rootId, "Control", Category, -1);
    addNode(controlId, "Reset", Property, ID_RESET);
    addNode(controlId, "CPU x1", Property, ID_CPU_POWER0);
    addNode(controlId, "CPU x2", Property, ID_CPU_POWER1);
    addNode(controlId, "CPU x4", Property, ID_CPU_POWER2);
    addNode(controlId, "CPU x8", Property, ID_CPU_POWER3);
    addNode(controlId, "CPU x16", Property, ID_CPU_POWER4);
    addNode(controlId, "Full Speed", Property, ID_FULL_SPEED);
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

    int fd1Id = addNode(rootId, "FD1", Category, -1);
    addNode(fd1Id, "Insert", Property, ID_OPEN_FD1);
    addNode(fd1Id, "Eject", Property, ID_CLOSE_FD1);
    addNode(fd1Id, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD1);
    addNode(fd1Id, "Write Protected", Property, ID_WRITE_PROTECT_FD1);
    addNode(fd1Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD1);
    addNode(fd1Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD1);
    addNode(fd1Id, "Recent", Property, ID_RECENT_FD1);

    int fd2Id = addNode(rootId, "FD2", Category, -1);
    addNode(fd2Id, "Insert", Property, ID_OPEN_FD2);
    addNode(fd2Id, "Eject", Property, ID_CLOSE_FD2);
    addNode(fd2Id, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD2);
    addNode(fd2Id, "Write Protected", Property, ID_WRITE_PROTECT_FD2);
    addNode(fd2Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD2);
    addNode(fd2Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD2);
    addNode(fd2Id, "Recent", Property, ID_RECENT_FD2);

    int kb640fd1Id = addNode(rootId, "640KB-FD1", Category, -1);
    addNode(kb640fd1Id, "Insert", Property, ID_OPEN_FD3);
    addNode(kb640fd1Id, "Eject", Property, ID_CLOSE_FD3);
    addNode(kb640fd1Id, "Insert Blank 2DD Disk", Property, ID_OPEN_BLANK_2DD_FD3);
    addNode(kb640fd1Id, "Write Protected", Property, ID_WRITE_PROTECT_FD3);
    addNode(kb640fd1Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD3);
    addNode(kb640fd1Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD3);
    addNode(kb640fd1Id, "Recent", Property, ID_RECENT_FD3);

    int kb640fd2Id = addNode(rootId, "640KB-FD2", Category, -1);
    addNode(kb640fd2Id, "Insert", Property, ID_OPEN_FD4);
    addNode(kb640fd2Id, "Eject", Property, ID_CLOSE_FD4);
    addNode(kb640fd2Id, "Insert Blank 2DD Disk", Property, ID_OPEN_BLANK_2DD_FD4);
    addNode(kb640fd2Id, "Write Protected", Property, ID_WRITE_PROTECT_FD4);
    addNode(kb640fd2Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD4);
    addNode(kb640fd2Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD4);
    addNode(kb640fd2Id, "Recent", Property, ID_RECENT_FD4);

    int kb320fd1Id = addNode(rootId, "320KB-FD1", Category, -1);
    addNode(kb320fd1Id, "Insert", Property, ID_OPEN_FD5);
    addNode(kb320fd1Id, "Eject", Property, ID_CLOSE_FD5);
    addNode(kb320fd1Id, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD5);
    addNode(kb320fd1Id, "Write Protected", Property, ID_WRITE_PROTECT_FD5);
    addNode(kb320fd1Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD5);
    addNode(kb320fd1Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD5);
    addNode(kb320fd1Id, "Recent", Property, ID_RECENT_FD5);

    int kb320fd2Id = addNode(rootId, "320KB-FD2", Category, -1);
    addNode(kb320fd2Id, "Insert", Property, ID_OPEN_FD6);
    addNode(kb320fd2Id, "Eject", Property, ID_CLOSE_FD6);
    addNode(kb320fd2Id, "Insert Blank 2D Disk", Property, ID_OPEN_BLANK_2D_FD6);
    addNode(kb320fd2Id, "Write Protected", Property, ID_WRITE_PROTECT_FD6);
    addNode(kb320fd2Id, "Correct Timing", Property, ID_CORRECT_TIMING_FD6);
    addNode(kb320fd2Id, "Ignore CRC Errors", Property, ID_IGNORE_CRC_FD6);
    addNode(kb320fd2Id, "Recent", Property, ID_RECENT_FD6);

    int cmtId = addNode(rootId, "CMT", Category, -1);
    addNode(cmtId, "Play", Property, ID_PLAY_TAPE1);
    addNode(cmtId, "Rec", Property, ID_REC_TAPE1);
    addNode(cmtId, "Eject", Property, ID_CLOSE_TAPE1);
    addNode(cmtId, "Recent", Property, ID_RECENT_TAPE1);

    int device = addNode(rootId, "Device", Category, -1);
    int dipSwitchId = addNode(device, "DIP Switch", Category, -1);
    addNode(dipSwitchId, "SW 2-1", Property, ID_VM_DIPSWITCH16);
    addNode(dipSwitchId, "SW 2-2 (Terminal)", Property, ID_VM_DIPSWITCH17);
    addNode(dipSwitchId, "SW 2-3 (80 Columns)", Property, ID_VM_DIPSWITCH18);
    addNode(dipSwitchId, "SW 2-4 (25 Lines)", Property, ID_VM_DIPSWITCH19);
    addNode(dipSwitchId, "SW 2-5 (Hold Memory Switch)", Property, ID_VM_DIPSWITCH20);
    addNode(dipSwitchId, "SW 2-6", Property, ID_VM_DIPSWITCH21);
    addNode(dipSwitchId, "SW 2-7", Property, ID_VM_DIPSWITCH22);
    addNode(dipSwitchId, "SW 2-8", Property, ID_VM_DIPSWITCH23);

    int floppyDriveId = addNode(device, "Floppy Drive", Category, -1);
    addNode(floppyDriveId, "1MB-FD", Property, ID_VM_DIPSWITCH0);
    addNode(floppyDriveId, "640KB-FD", Property, ID_VM_DIPSWITCH1);
    addNode(floppyDriveId, "320KB-FD", Property, ID_VM_DIPSWITCH2);

    int soundId = addNode(device, "Sound", Category, -1);
    addNode(soundId, "PC-9801-26 (BIOS Enabled)", Property, ID_VM_SOUND_TYPE0);
    addNode(soundId, "PC-9801-26 (BIOS Disabled)", Property, ID_VM_SOUND_TYPE1);
    addNode(soundId, "PC-9801-14 (BIOS Enabled)", Property, ID_VM_SOUND_TYPE2);
    addNode(soundId, "PC-9801-14 (BIOS Disabled)", Property, ID_VM_SOUND_TYPE3);
    addNode(soundId, "None", Property, ID_VM_SOUND_TYPE4);
    addNode(soundId, "Play FDD Noise", Property, ID_VM_SOUND_NOISE_FDD);
    addNode(soundId, "Play CMT Noise", Property, ID_VM_SOUND_NOISE_CMT);
    addNode(soundId, "Play CMT Signal", Property, ID_VM_SOUND_TAPE_SIGNAL);
    addNode(soundId, "Play CMT Voice", Property, ID_VM_SOUND_TAPE_VOICE);

    int displayId = addNode(device, "Display", Category, -1);
    addNode(displayId, "High Resolution", Property, ID_VM_MONITOR_TYPE0);
    addNode(displayId, "Standard", Property, ID_VM_MONITOR_TYPE1);
    addNode(displayId, "Scanline", Property, ID_VM_MONITOR_SCANLINE);

    int printerId = addNode(device, "Printer", Category, -1);
    addNode(printerId, "Write Printer to File", Property, ID_VM_PRINTER_TYPE0);
    addNode(printerId, "PC-PR201", Property, ID_VM_PRINTER_TYPE1);
    addNode(printerId, "None", Property, ID_VM_PRINTER_TYPE2);

    int serialId = addNode(device, "Serial", Category, -1);
    addNode(serialId, "Physical Comm Port", Property, ID_VM_SERIAL_TYPE0);
    addNode(serialId, "Named Pipe", Property, ID_VM_SERIAL_TYPE1);
    addNode(serialId, "MIDI Device", Property, ID_VM_SERIAL_TYPE2);
    addNode(serialId, "None", Property, ID_VM_SERIAL_TYPE3);

    int hostId = addNode(rootId, "Host", Category, -1);
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
    addNode(soundId2, "55467Hz", Property, ID_SOUND_FREQ6);
    addNode(soundId2, "96000Hz", Property, ID_SOUND_FREQ7);
    addNode(soundId2, "50msec", Property, ID_SOUND_LATE0);
    addNode(soundId2, "100msec", Property, ID_SOUND_LATE1);
    addNode(soundId2, "200msec", Property, ID_SOUND_LATE2);
    addNode(soundId2, "300msec", Property, ID_SOUND_LATE3);
    addNode(soundId2, "400msec", Property, ID_SOUND_LATE4);
    addNode(soundId2, "Realtime Mix", Property, ID_SOUND_STRICT_RENDER);
    addNode(soundId2, "Light Weight Mix", Property, ID_SOUND_LIGHT_RENDER);
    addNode(soundId2, "Volume", Property, ID_SOUND_VOLUME);

    int inputId = addNode(hostId, "Input", Category, -1);
    addNode(inputId, "Joystick #1", Property, ID_INPUT_JOYSTICK0);
    addNode(inputId, "Joystick #2", Property, ID_INPUT_JOYSTICK1);
    addNode(inputId, "Joystick To Keyboard", Property, ID_INPUT_JOYTOKEY);

    addNode(hostId, "Use Direct2D1", Property, ID_HOST_USE_D2D1);
    addNode(hostId, "Use Direct3D9", Property, ID_HOST_USE_D3D9);
    addNode(hostId, "Wait Vsync", Property, ID_HOST_WAIT_VSYNC);
    addNode(hostId, "Use DirectInput", Property, ID_HOST_USE_DINPUT);
    addNode(hostId, "Disable Windows 8 DWM", Property, ID_HOST_DISABLE_DWM);
    addNode(hostId, "Show Status Bar", Property, ID_HOST_SHOW_STATUS_BAR);
}
