/*
	Skelton for retropc emulator

	Author : @shikarunochi
	Date   : 2020.06.01-

	[ android main ]
*/

#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/configuration.h>
#include <android/window.h>
#include <android/looper.h>

#include <errno.h>
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>

#include <vector>
#include<string>

#include <unistd.h>
#include "emu.h"
#include "Android/osd.h"
#include "fifo.h"
#include "fileio.h"

#include <iconv.h>
#include <sys/stat.h>
#include "Android/menu/menu.h"
#include "Android/menu/BaseMenu.h"
#include "res/resource.h"

// ストレージ権限付与イベント
#define PERMISSIONS_GRANTED_EVENT 1

////////////////////////////////////////////////////////////////////////////////
// emulation core

EMU *emu;

#define     MAX_FRAME_STATS     200
#define     MAX_PERIOD_MS       1500
#define     LOWORD(l)           l

/* simple stats management */
typedef struct {
    double renderTime;
    double frameTime;
} FrameStats;

typedef struct {
    double firstTime;
    double lastTime;
    double frameTime;

    int firstFrame;
    int numFrames;
    FrameStats frames[MAX_FRAME_STATS];
} Stats;

struct engine {
    struct android_app *app;
    Stats stats;
    int animating;
    bool emu_initialized;
};

static double now_ms(void);
static uint16_t make565(int red, int green, int blue);
static void stats_init(Stats *s);
static void stats_startFrame(Stats *s);
static void stats_endFrame(Stats *s);
DWORD timeGetTime();
static void engine_handle_cmd(struct android_app *app, int32_t cmd);
static void init_tables(void);
void android_main(struct android_app *state);

bool resetFlag;
DeviceInfo deviceInfo;
static int64_t start_ms;

////////////////////////////////////////////////////////////////////////////////
// event

void EventProc(engine* engine, MenuNode menuNode);

////////////////////////////////////////////////////////////////////////////////
// menu

#if _WIN32
bool now_menuloop = false;

void update_toplevel_menu(HWND hWnd, HMENU hMenu);
void update_popup_menu(HWND hWnd, HMENU hMenu);
void show_menu_bar(HWND hWnd);
void hide_menu_bar(HWND hWnd);

// status bar
HWND hStatus = NULL;
bool status_bar_visible = false;
#endif

#ifdef USE_FLOPPY_DISK
uint32_t fd_status = 0x80000000;
uint32_t fd_indicator_color = 0x80000000;
#endif
#ifdef USE_QUICK_DISK
uint32_t qd_status = 0x80000000;
#endif
#ifdef USE_HARD_DISK
uint32_t hd_status = 0x80000000;
#endif
#ifdef USE_COMPACT_DISC
uint32_t cd_status = 0x80000000;
#endif
#ifdef USE_LASER_DISC
uint32_t ld_status = 0x80000000;
#endif
#if defined(USE_TAPE) && !defined(TAPE_BINARY_ONLY)
_TCHAR tape_status[1024] = _T("uninitialized");
int tape_position = 0;
#endif

#ifdef _WIN32
void show_status_bar(HWND hWnd);
void hide_status_bar(HWND hWnd);
int get_status_bar_height();
void update_status_bar(HINSTANCE hInstance, LPDRAWITEMSTRUCT lpDrawItem);
#endif

void switchPCG();
void switchSound();
bool get_status_bar_updated();

////////////////////////////////////////////////////////////////////////////////
// file

void selectMedia(struct android_app *state, int iconIndex);
void selectCart(struct android_app *state, int driveNo);
void selectFloppyDisk(struct android_app *state, int diskNo);
void selectQuickDisk(struct android_app *state, int driveNo);
void selectHardDisk(struct android_app *state, int driveNo);
void selectTape(struct android_app *state);
static void all_eject();

char documentDir[_MAX_PATH];
char emulatorDir[_MAX_PATH];
char applicationDir[_MAX_PATH];
char configPath[_MAX_PATH];

std::vector<std::string> fileList;
FileSelectType fileSelectType;
FileSelectIconData fileSelectIconData[MAX_FILE_SELECT_ICON];
int selectingIconIndex;
bool needSelectDiskBank = false;
int selectDiskDrive = 0;

typedef struct {
    int drive;
    enum FileSelectType fileSelectType;
    uint8_t floppy_type;
    int hdd_sector_size;
    int hdd_sectors;
    int hdd_surfaces;
    int hdd_cylinders;
} MediaInfo;

#ifdef USE_CART
void open_cart_dialog(struct android_app *app, int drv) {}
void open_recent_cart(int drv, int index);
#endif
#ifdef USE_FLOPPY_DISK
void open_floppy_disk_dialog(struct android_app *app, int drive);
void open_blank_floppy_disk_dialog(struct android_app * app, int drv, uint8_t type);
void open_recent_floppy_disk(int drv, int index);
void select_d88_bank(int drv, int index);
#endif
#ifdef USE_QUICK_DISK
void open_quick_disk_dialog(struct android_app *app, int drive);
void open_recent_quick_disk(int drv, int index);
#endif
#ifdef USE_HARD_DISK
void open_hard_disk_dialog(struct android_app *app, int drv);
void open_recent_hard_disk(int drv, int index);
void open_blank_hard_disk_dialog(struct android_app *app, int drv, int sector_size, int sectors, int surfaces, int cylinders);
#endif
#ifdef USE_TAPE
void open_tape_dialog(struct android_app *app, int drive, bool play);
void open_recent_tape(int drv, int index);
#endif
#ifdef USE_COMPACT_DISC
void open_compact_disc_dialog(struct android_app *app, int drv);
void open_recent_compact_disc(int drv, int recent);
#endif
#ifdef USE_LASER_DISC
void open_laser_disc_dialog(struct android_app *app, int drv);
void open_recent_laser_disc(int drv, int index);
#endif
#ifdef USE_BINARY_FILE
void open_binary_dialog(struct android_app *app, int drv, bool flag);
void open_recent_binary(int drv, int recent);
#endif
#ifdef USE_BUBBLE
void open_bubble_casette_dialog(struct android_app *appp, int drv);
void open_recent_bubble_casette(int drv, int recent);
#endif
#if defined(USE_CART) || defined(USE_FLOPPY_DISK) || defined(USE_HARD_DISK) || defined(USE_TAPE) || defined(USE_COMPACT_DISC) || defined(USE_LASER_DISC) || defined(USE_BINARY_FILE) || defined(USE_BUBBLE)
#define SUPPORT_DRAG_DROP
#endif
#ifdef SUPPORT_DRAG_DROP
#if defined(_WIN32)
void open_dropped_file(HDROP hDrop);
void open_any_file(const _TCHAR* path);
#endif
#endif

#ifdef _WIN32
_TCHAR* get_open_file_name(HWND hWnd, const _TCHAR* filter, const _TCHAR* title, const _TCHAR* new_file, _TCHAR* dir, size_t dir_len);
#endif

////////////////////////////////////////////////////////////////////////////////
// screen

static void draw_icon(ANativeWindow_Buffer *buffer);
static void draw_message(struct android_app *state, ANativeWindow_Buffer *buffer, const char *message);
static void engine_draw_message(struct engine *engine, const char *message);
static void load_emulator_screen(ANativeWindow_Buffer *buffer);
static void engine_draw_frame(struct engine *engine);
static void engine_term_display(struct engine *engine);
static void clear_screen(struct engine *engine);
void check_update_screen(engine* engine);

//ScreenSize screenSize = SCREEN_SIZE_JUST;
//ScreenSize preScreenSize = SCREEN_SIZE_JUST;
ScreenSize screenSize = SCREEN_SIZE_MAX;
ScreenSize preScreenSize = SCREEN_SIZE_MAX;

BitmapData systemIconData[16];
BitmapData mediaIconData[16];

#ifdef _WIN32
int desktop_width;
int desktop_height;
int desktop_bpp;
int prev_window_mode = 0;
bool now_fullscreen = false;

#define MAX_WINDOW	10
#define MAX_FULLSCREEN	50

int screen_mode_count;
int screen_mode_width[MAX_FULLSCREEN];
int screen_mode_height[MAX_FULLSCREEN];

void enum_screen_mode();
void set_window(HWND hWnd, int mode);
#endif

////////////////////////////////////////////////////////////////////////////////
// input

static int32_t engine_handle_input(struct android_app *app, AInputEvent *event);

#ifdef USE_AUTO_KEY
void start_auto_key(struct android_app *app);
#endif

////////////////////////////////////////////////////////////////////////////////
// dialog

Menu *menu;

jint showAlert(struct android_app *state, const char *message, const char *filenames, bool model = false, int selectMode = 0, int selectIndex = 0);
void showNewFileDialog(struct android_app *state, const char *message, const char *itemNames, const char *fileExtension, MediaInfo *mediaInfo, const char *addPath);
jint showExtendMenu(struct android_app *state, const char *title, const char *extendMenuString);

void EventProc(engine *engine, MenuNode menuNode);
void extendMenu(struct android_app *app);
void selectDialog(struct android_app *state, const char *message, const char *addPath);
void selectD88Bank(struct android_app *state, int driveNo);
void selectBootMode(struct android_app *state);
void createBlankDisk(struct android_app *state, MediaInfo *mediaInfo);

std::string extendMenuString = "";
auto extendMenuDisplay = false;
auto extendMenuCmtPlay = true;
MenuNode notifyMenuNode = MenuNode::emptyNode();

// thanks Marukun (64bit)
#ifdef _WIN32
#ifdef USE_SOUND_VOLUME
INT_PTR CALLBACK VolumeWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
#endif
#ifdef USE_JOYSTICK
INT_PTR CALLBACK JoyWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK JoyToKeyWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// buttons

#ifdef _WIN32
#ifdef ONE_BOARD_MICRO_COMPUTER
void create_buttons(HWND hWnd);
void draw_button(HDC hDC, UINT index, UINT pressed);
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// misc

static int get_unicode_character(struct android_app *app, int event_type, int key_code, int meta_state);
bool caseInsensitiveCompare(const std::string &a, const std::string &b);
void setFileSelectIcon();

#ifdef _WIN32
bool win8_or_later = false;
#endif

////////////////////////////////////////////////////////////////////////////////
// android

const char *jniGetSdcardDownloadPath(struct android_app *state);
void jniReadIconData(struct android_app *state);
BitmapData jniCreateBitmapFromString(struct android_app *state, const char *text, int fontSize);
static void toggle_soft_keyboard(struct android_app *app);
void checkPermissionsAndInitialize(JNIEnv *env, jobject activity);
std::vector<uint8_t> getClipboardText(struct android_app *app);

#define SOFT_KEYBOARD_KEEP_COUNT  3
int softKeyboardCount = 0;
bool softKeyShift = false;
bool softKeyCtrl = false;

int softKeyCode = 0;
bool softKeyDelayFlag = false;

// グローバル参照を保持するための変数
JavaVM* g_JavaVM = nullptr;
jobject g_ActivityObject = nullptr;
bool grantedStorage = false;
static struct android_app* globalAppState = nullptr;

////////////////////////////////////////////////////////////////////////////////
// Debug

//#define  LOG_TAG    "libplasma"
//#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

/* Set to 1 to enable debug log traces. */
#define DEBUG 0

/* Set to 1 to optimize memory stores when generating plasma. */
#define OPTIMIZE_WRITES  1

/** @param onClickListener MUST be NULL because the model dialog is not implemented. */
typedef void *(OnClickListener)(int id);

// ----------------------------------------------------------------------------
// emulation core
// ----------------------------------------------------------------------------

/* Return current time in milliseconds */
static double now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000. + tv.tv_usec / 1000.;
}

static uint16_t make565(int red, int green, int blue) {
    return (uint16_t) (((red << 8) & 0xf800) |
                       ((green << 3) & 0x07e0) |
                       ((blue >> 3) & 0x001f));
}

static void stats_init(Stats *s) {
    s->lastTime = now_ms();
    s->firstTime = 0.;
    s->firstFrame = 0;
    s->numFrames = 0;
}

static void stats_startFrame(Stats *s) {
    s->frameTime = now_ms();
}

static void stats_endFrame(Stats *s) {
    double now = now_ms();
    double renderTime = now - s->frameTime;
    double frameTime = now - s->lastTime;
    int nn;

    if (now - s->firstTime >= MAX_PERIOD_MS) {
        if (s->numFrames > 0) {
            double minRender, maxRender, avgRender;
            double minFrame, maxFrame, avgFrame;
            int count;

            nn = s->firstFrame;
            minRender = maxRender = avgRender = s->frames[nn].renderTime;
            minFrame = maxFrame = avgFrame = s->frames[nn].frameTime;
            for (count = s->numFrames; count > 0; count--) {
                nn += 1;
                if (nn >= MAX_FRAME_STATS)
                    nn -= MAX_FRAME_STATS;
                double render = s->frames[nn].renderTime;
                if (render < minRender) minRender = render;
                if (render > maxRender) maxRender = render;
                double frame = s->frames[nn].frameTime;
                if (frame < minFrame) minFrame = frame;
                if (frame > maxFrame) maxFrame = frame;
                avgRender += render;
                avgFrame += frame;
            }
            avgRender /= s->numFrames;
            avgFrame /= s->numFrames;

            //LOGI("frame/s (avg,min,max) = (%.1f,%.1f,%.1f) "
            //     "render time ms (avg,min,max) = (%.1f,%.1f,%.1f)\n",
            //     1000./avgFrame, 1000./maxFrame, 1000./minFrame,
            //     avgRender, minRender, maxRender);
        }
        s->numFrames = 0;
        s->firstFrame = 0;
        s->firstTime = now;
    }

    nn = s->firstFrame + s->numFrames;
    if (nn >= MAX_FRAME_STATS)
        nn -= MAX_FRAME_STATS;

    s->frames[nn].renderTime = renderTime;
    s->frames[nn].frameTime = frameTime;

    if (s->numFrames < MAX_FRAME_STATS) {
        s->numFrames += 1;
    } else {
        s->firstFrame += 1;
        if (s->firstFrame >= MAX_FRAME_STATS)
            s->firstFrame -= MAX_FRAME_STATS;
    }

    s->lastTime = now;
}

DWORD timeGetTime() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    //return ts.tv_sec * 1000000L + ts.tv_nsec / 1000;
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000;
}

static void engine_handle_cmd(struct android_app *app, int32_t cmd) {
    static int32_t format = WINDOW_FORMAT_RGB_565;
    struct engine *engine = (struct engine *) app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
        {
            if (engine->app->window != NULL) {
                // fill_plasma() assumes 565 format, get it here
                format = ANativeWindow_getFormat(app->window);
                ANativeWindow_setBuffersGeometry(app->window,
                                                 ANativeWindow_getWidth(app->window),
                                                 ANativeWindow_getHeight(app->window),
                                                 WINDOW_FORMAT_RGB_565);
                engine_draw_frame(engine);
                deviceInfo.width = ANativeWindow_getWidth(app->window);
                deviceInfo.height = ANativeWindow_getHeight(app->window);
                clear_screen(engine);
                engine->animating = 1;
                LOGI("APP_CMD_INIT_WINDOW");
                LOGI("Screen size (%d, %d)", deviceInfo.width, deviceInfo.height);
            }
            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            engine_term_display(engine);
            format = ANativeWindow_getFormat(app->window);
            ANativeWindow_setBuffersGeometry(app->window,
                                             ANativeWindow_getWidth(app->window),
                                             ANativeWindow_getHeight(app->window),
                                             WINDOW_FORMAT_RGB_565);
            engine_draw_frame(engine);
            deviceInfo.width = ANativeWindow_getWidth(app->window);
            deviceInfo.height = ANativeWindow_getHeight(app->window);
            clear_screen(engine);
            engine->animating = 1;
            LOGI("APP_CMD_TERM_WINDOW");
            LOGI("Screen size (%d, %d)", deviceInfo.width, deviceInfo.height);
            break;
        }
        case APP_CMD_LOST_FOCUS:
        {
            //engine->animating = 0;
            engine_draw_frame(engine);
            break;
        }
        case APP_CMD_CONFIG_CHANGED:
        {
            // 設定変更時の処理
            AConfiguration* aconfig = AConfiguration_new();
            AConfiguration_fromAssetManager(aconfig, app->activity->assetManager);
            // 新しい画面のサイズや向きに基づいてUIや描画を更新
            // ここに画面サイズ変更に対する処理を追加
            // 画面の向きが変わったら、バッファサイズを再設定
            if (app->window != NULL) {
                ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGB_565);
            }
            AConfiguration_delete(aconfig);
            break;
        }
        case APP_CMD_DESTROY:
        {
            // 強制排出
            LOGI("APP_CMD_DESTROY");
            all_eject();
            break;
        }
    }
}

static void init_tables(void) {}

void android_main(struct android_app *state) {

    ANativeActivity_setWindowFlags(state->activity,AWINDOW_FLAG_KEEP_SCREEN_ON , 0);    //Sleepさせない

    static int init;
    struct engine engine;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;
    engine.emu_initialized = false;

    // アプリケーションの状態をグローバル変数に保持
    globalAppState = state;

    {
        JNIEnv* env = nullptr;
        state->activity->vm->AttachCurrentThread(&env, nullptr);

        // 権限チェックと初期化を非同期で行う
        checkPermissionsAndInitialize(env, state->activity->clazz);

        state->activity->vm->DetachCurrentThread();
    }

    const char *documentDirTemp = jniGetSdcardDownloadPath(state);

    sprintf(documentDir, "%s", documentDirTemp);
    sprintf(emulatorDir, "%s/emulator", documentDir);
    sprintf(applicationDir, "%s/%sROM", emulatorDir, CONFIG_NAME);
    sprintf(configPath, "%s/config.ini", applicationDir);
    free((void*)documentDirTemp);
    LOGI("documentDir: %s", documentDir);
    LOGI("emulatorDir: %s", emulatorDir);
    LOGI("applicationDir: %s", applicationDir);

#if defined(_EXTEND_MENU)
    // メニュー初期化
    menu = new Menu();
#endif

    //Read Icon Image
    jniReadIconData(state);
    setFileSelectIcon();

    // エミュレータフォルダの存在チェック、存在しなければフォルダを作製する
    if (access(emulatorDir, F_OK) == -1) {
        // フォルダ作成に失敗した場合はエラーログを出力
        if (mkdir(emulatorDir, 0777) != 0) {
            LOGI("Failed to create emulator directory");
        }
    }
    // アプリケーションフォルダの存在チェック、存在しなければフォルダを作製する
    if (access(applicationDir, F_OK) == -1) {
        // フォルダ作成に失敗した場合はエラーログを出力
        if (mkdir(applicationDir, 0777) != 0) {
            LOGI("Failed to create application directory");
        }
    }
    // コンフィグファイルの存在チェック
    if (access(configPath, F_OK) == -1) {
        // コンフィグファイルが存在しない場合は初期化
        initialize_config();
#if defined(_PC8801MA)
        // 設定上、PC-8801MAの場合は初期値を設定しないと起動しない事がある
        config.boot_mode = 2;
        config.cpu_type = 0;
        config.sound_type = 2;
        config.monitor_type = 0;
        config.scan_line = 0;
        config.scan_line_auto = 0;
#endif
        // 画面下にマージンを設定する
        config.screen_bottom_margin = 60;
        // コンフィグファイルを保存
        save_config(configPath);
        // ファイル名付きでコンフィグ新規作成ログ出力
        LOGI("Created new config file: %s", configPath);
    } else {
        // コンフィグファイルを読み込み
        load_config(configPath);
        // コンフィグファイルを保存
        save_config(configPath);
        // コンフィグ読み込みログ出力
        LOGI("Loaded config file: %s", configPath);
    }

    // 権限が付与されるまで先に進まないループ
    while (1) {
        int events;
        struct android_poll_source* source;

        // プロセスイベント取得
        int ident = ALooper_pollAll(-1, NULL, &events, (void **) &source);
        if (source) {
            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }
        }
        if (grantedStorage) {
            // 権限が許可された後の処理
            break;
        }
    }

    // 権限付与待機ループ処理後のクリーンアップ
    globalAppState = nullptr;

    emu = new EMU(state);
    engine.emu_initialized = true;

    if (!init) {
        init_tables();
        init = 1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    start_ms = (((int64_t) now.tv_sec) * 1000000000LL + now.tv_nsec) / 1000000;

    stats_init(&engine.stats);

    // loop waiting for stuff to do.

    int total_frames = 0, draw_frames = 0, skip_frames = 0;
    DWORD next_time = 0;
    bool prev_skip = false;
    DWORD update_fps_time = 0;
    DWORD update_status_bar_time = 0;
    DWORD disable_screen_saver_time = 0;
    bool needDraw = false;

    resetFlag = false;
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        int fd;
        struct android_poll_source *source;

        if (emu->get_osd()->soundEnable != config.sound_on) {
            emu->get_osd()->reset_sound();
            emu->get_osd()->soundEnable = config.sound_on;
        }

        // 画面サイズ変更チェック
        check_update_screen(&engine);

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        //while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
        //                             (void**)&source)) >= 0) {
        ident = ALooper_pollAll(engine.animating ? 0 : -1, &fd, &events,(void **) &source);

        // Process this event.
        if (source != NULL) {
            source->process(state, source);
        }

        // Check if we are exiting.
        if (state->destroyRequested != 0) {
            LOGI("Engine thread destroy requested! ident=%d / fd=%d / events=%d", ident, fd, events);
            all_eject();
            engine_term_display(&engine);
            return;
        }
        if (resetFlag == true) {
            engine.animating = 0;
#ifdef USE_TAPE
            emu->close_tape(0);
#endif
            emu->reset();
            needDraw = true;
            resetFlag = false;
            engine.animating = 1;
        }

        if (emu) {
            // drive machine
            int run_frames = emu->run();
            total_frames += run_frames;

            // timing controls
            int sleep_period = 0;
            bool now_skip = (config.full_speed || emu->is_frame_skippable()) &&
                            !emu->is_video_recording() && !emu->is_sound_recording();

            if ((prev_skip && !now_skip) || next_time == 0) {
                next_time = timeGetTime();
            }
            if (!now_skip) {
                static int accum = 0;
                accum += emu->get_frame_interval();
                int interval = accum >> 10;
                accum -= interval << 10;
                next_time += interval;
            }
            prev_skip = now_skip;

            if (next_time > timeGetTime()) {
                // update window if enough time
                draw_frames += emu->draw_screen();
                needDraw = true;
                skip_frames = 0;
                // sleep 1 frame priod if need
                DWORD current_time = timeGetTime();
                if ((int) (next_time - current_time) >= 10) {
                    sleep_period = next_time - current_time;
                }
            } else if (++skip_frames > (int) emu->get_frame_rate()) {
                // update window at least once per 1 sec in virtual machine time
                draw_frames += emu->draw_screen();
                needDraw = true;
                skip_frames = 0;
                next_time = timeGetTime();
            }
            usleep(sleep_period);

            // calc frame rate
            DWORD current_time = timeGetTime();
            if (update_fps_time <= current_time) {
                if (update_fps_time != 0) {
                    if (emu->message_count > 0) {
                        //        SetWindowText(hWnd, create_string(_T("%s - %s"), _T(DEVICE_NAME), emu->message));
                        emu->message_count--;
                    } else if (now_skip) {
                        //        int ratio = (int)(100.0 * (double)total_frames / emu->get_frame_rate() + 0.5);
                        //        SetWindowText(hWnd, create_string(_T("%s - Skip Frames (%d %%)"), _T(DEVICE_NAME), ratio));
                    } else {
                        //       int ratio = (int)(100.0 * (double)draw_frames / (double)total_frames + 0.5);
                        //       SetWindowText(hWnd, create_string(_T("%s - %d fps (%d %%)"), _T(DEVICE_NAME), draw_frames, ratio));
                    }
                    update_fps_time += 1000;
                    total_frames = draw_frames = 0;
                }
                update_fps_time = current_time + 1000;
            }

            // update status bar
            if (update_status_bar_time <= current_time) {
                if (get_status_bar_updated()) {
                    //engine_draw_message(&engine, tape_status);
                }
                update_status_bar_time = current_time + 200;
            }

        }

        if(softKeyDelayFlag == true){
            emu->get_osd()->key_down(softKeyCode, false, false);
            softKeyDelayFlag = false;
        }

        if (engine.animating && needDraw) {
            engine_draw_frame(&engine);
            needDraw = false;
            if (softKeyboardCount > 0) {
                softKeyboardCount--;
                if (softKeyboardCount == 0) {
                    if (softKeyCtrl == true) {
                        emu->get_osd()->key_up(113, false);
                    }
                    if (softKeyShift == true) {
                        emu->get_osd()->key_up(59, false);
                    }
                    emu->get_osd()->key_up(softKeyCode, false);
                    softKeyShift = false;
                    softKeyCtrl = false;
                    softKeyCode = 0;
                } else {

#if defined(_MZ700)
                    //機種によってはしばらくの間keyDownを送り続ける必要がある…と思ったけど、やらなくても大丈夫でした。
                    //emu->get_osd()->key_down(softKeyCode,false,false);
#endif
                }
            }
        }
#ifdef USE_FLOPPY_DISK
        if (needSelectDiskBank == true) {
            needSelectDiskBank = false;
            selectD88Bank(state, selectDiskDrive);
        }
#endif
        // 通知ノードが有効か確認する
        if (notifyMenuNode.nodeId != -1) {
            // 通知ノードの ItemType で分岐する
            switch (notifyMenuNode.itemType) {
                case Category:
                {
                    // メニュー文字列を取得する
                    extendMenuString = menu->getExtendMenuString(notifyMenuNode.nodeId);
                    // キャプション文字列を取得する
                    std::string caption = menu->getHierarchyString(notifyMenuNode.getNodeId());
                    // 通知をクリア
                    notifyMenuNode = MenuNode::emptyNode();
                    // NDKからJavaの関数を呼び出す
                    extendMenuDisplay = true;
                    jint nodeId = showExtendMenu(
                            state,
                            caption.c_str(),
                            extendMenuString.c_str());
                    break;
                }
                case Property:
                    EventProc(&engine, notifyMenuNode);
                    // コンフィグファイルを保存
                    save_config(configPath);
                    // ファイル名付きでコンフィグ新規作成ログ出力
                    LOGI("Save new config file: %s", configPath);
                    // 通知をクリア
                    notifyMenuNode = MenuNode::emptyNode();
                    break;
                default:
                    break;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// event
// ----------------------------------------------------------------------------

// 拡張メニュー処理
void EventProc(engine* engine, MenuNode menuNode)
{
    struct android_app *app = engine->app;
    int wParam = menuNode.returnValue;
    int hWnd = 0;
    int iMsg = WM_COMMAND;
    int lParam = 0;

    switch(iMsg) {
        case WM_CREATE:
#if defined(_WIN32)
            if(config.disable_dwm && win8_or_later) {
                disable_dwm();
            }
#ifdef ONE_BOARD_MICRO_COMPUTER
            create_buttons(hWnd);
#endif
#ifdef SUPPORT_DRAG_DROP
            DragAcceptFiles(hWnd, TRUE);
#endif
#ifdef _M_AMD64
            // thanks Marukun (64bit)
    		hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
#else
            hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
#endif
            timeBeginPeriod(1);
#endif
            break;
        case WM_CLOSE:
#if defined(_WIN32)
            #ifdef USE_NOTIFY_POWER_OFF
            // notify power off
    		if(emu && !notified) {
			    emu->notify_power_off();
		    	notified = true;
	    		return 0;
    		}
#endif
            // release window
            if(now_fullscreen) {
                ChangeDisplaySettings(NULL, 0);
                now_fullscreen = false;
            }
            if(hMenu != NULL) {
                if(IsMenu(hMenu)) {
                    DestroyMenu(hMenu);
                }
                hMenu = NULL;
            }
            if(hStatus != NULL) {
                DestroyWindow(hStatus);
                hStatus = NULL;
            }
            DestroyWindow(hWnd);
            timeEndPeriod(1);
#endif
        case WM_ENDSESSION:
            // release emulation core
            if(emu) {
                delete emu;
                emu = NULL;
                save_config(create_local_path(_T("%s.ini"), _T(CONFIG_NAME)));
            }
#if defined(_WIN32)
            return 0;
#else
            break;
#endif
        case WM_DESTROY:
#if defined(_WIN32)
            PostQuitMessage(0);
            return 0;
#else
            break;
#endif
        case WM_QUERYENDSESSION:
#if defined(_WIN32)
            #ifdef USE_NOTIFY_POWER_OFF
            // notify power off and drive machine
    		if(emu && !notified) {
	    		emu->notify_power_off();
		    	emu->run();
			    notified = true;
    		}
#endif
            return TRUE;
#else
            break;
#endif
        case WM_ACTIVATE:
#if defined(_WIN32)
            // thanks PC8801MA改
            if(LOWORD(wParam) != WA_INACTIVE) {
                himcPrev = ImmAssociateContext(hWnd, 0);
            } else {
                ImmAssociateContext(hWnd, himcPrev);
            }
#endif
            break;
        case WM_SIZE:
#if defined(_WIN32)
            if(hStatus != NULL) {
                SendMessage(hStatus, WM_SIZE, wParam, lParam);
            }
#ifdef ONE_BOARD_MICRO_COMPUTER
            if(emu) {
                emu->reload_bitmap();
            }
#endif
#endif
            break;
        case WM_KILLFOCUS:
            if(emu) {
                emu->key_lost_focus();
            }
            break;
        case WM_PAINT:
#if defined(_WIN32)
            if(emu) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
#ifdef ONE_BOARD_MICRO_COMPUTER
                // check if self invalidate or not
                int left = 0, top = 0, right = 0, bottom = 0;
                emu->get_invalidated_rect(&left, &top, &right, &bottom);
                if(ps.rcPaint.left != left || ps.rcPaint.top != top || ps.rcPaint.right != right || ps.rcPaint.bottom != bottom) {
                    emu->reload_bitmap();
                }
#endif
                emu->update_screen(hdc);
                EndPaint(hWnd, &ps);
            }
            return 0;
#else
            break;
#endif
        case WM_DRAWITEM:
#if defined(_WIN32)
            {
                LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
                if(lpDrawItem->CtlID == ID_STATUS) {
                    if(emu) {
                        update_status_bar(hInstance, lpDrawItem);
                    }
                } else {
#ifdef ONE_BOARD_MICRO_COMPUTER
//	    			if(lpDrawItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
                        draw_button(lpDrawItem->hDC, (UINT)wParam - ID_BUTTON, lpDrawItem->itemState & ODS_SELECTED);
//			    	}
#endif
                }
            }
            return TRUE;
#else
            break;
#endif
        case WM_MOVING:
            if(emu) {
                emu->suspend();
            }
            break;
        case WM_KEYDOWN:
#if defined(_WIN32)
            if(emu) {
                bool extended = ((HIWORD(lParam) & 0x100) != 0);
                bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
                int code = LOBYTE(wParam);
                if(code == VK_PROCESSKEY) {
                    code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
                }
                emu->key_down(code, extended, repeat);
            }
#endif
            break;
        case WM_KEYUP:
#if defined(_WIN32)
            if(emu) {
                bool extended = ((HIWORD(lParam) & 0x100) != 0);
                int code = LOBYTE(wParam);
                if(code == VK_PROCESSKEY) {
                    code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
                }
                emu->key_up(code, extended);
            }
#endif
            break;
        case WM_SYSKEYDOWN:
#if defined(_WIN32)
            if(emu) {
                bool extended = ((HIWORD(lParam) & 0x100) != 0);
                bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
                int code = LOBYTE(wParam);
                if(code == VK_PROCESSKEY) {
                    code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
                }
                emu->key_down(code, extended, repeat);
            }
            return 0;	// not activate menu when hit ALT/F10
#else
            break;
#endif
        case WM_SYSKEYUP:
#if defined(_WIN32)
            if(emu) {
                bool extended = ((HIWORD(lParam) & 0x100) != 0);
                int code = LOBYTE(wParam);
                if(code == VK_PROCESSKEY) {
                    code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
                }
                emu->key_up(code, extended);
            }
            return 0;	// not activate menu when hit ALT/F10
#else
            break;
#endif
        case WM_CHAR:
#if defined(_WIN32)
            if(emu) {
                emu->key_char(LOBYTE(wParam));
            }
#endif
            break;
        case WM_SYSCHAR:
#if defined(_WIN32)
            return 0;	// not activate menu when hit ALT/F10
#else
            break;
#endif
        case WM_INITMENUPOPUP:
#if defined(_WIN32)
            if(emu) {
                emu->suspend();
            }
            update_popup_menu(hWnd, (HMENU)wParam);
#endif
            break;
        case WM_ENTERMENULOOP:
#if defined(_WIN32)
            now_menuloop = true;
#endif
            break;
        case WM_EXITMENULOOP:
#if defined(_WIN32)
            if(now_fullscreen && now_menuloop) {
                hide_menu_bar(hWnd);
            }
            now_menuloop = false;
#endif
            break;
        case WM_MOUSEMOVE:
#if defined(_WIN32)
            if(now_fullscreen && !now_menuloop) {
                POINTS p = MAKEPOINTS(lParam);
                if(p.y == 0) {
                    show_menu_bar(hWnd);
                } else if(p.y > 32) {
                    hide_menu_bar(hWnd);
                }
            }
#endif
            break;
        case WM_RESIZE:
#if defined(_WIN32)
            if(emu) {
                if(now_fullscreen) {
                    emu->set_host_window_size(-1, -1, false);
                } else {
                    set_window(hWnd, config.window_mode);
                }
            }
#endif
            break;
#ifdef SUPPORT_DRAG_DROP
        case WM_DROPFILES:
#if defined(_WIN32)
            if(emu) {
                open_dropped_file((HDROP)wParam);
            }
#endif
            break;
#endif
#ifdef USE_SOCKET
            case WM_SOCKET0: case WM_SOCKET1: case WM_SOCKET2: case WM_SOCKET3:
#if defined(_WIN32)
    		if(emu) {
	    		update_socket(iMsg - WM_SOCKET0, wParam, lParam);
		    }
#endif
    		break;
#endif
        case WM_COMMAND:
            switch(wParam) {
                case ID_RESET:
                    if(emu) {
                        emu->reset();
                    }
                    break;
#ifdef USE_SPECIAL_RESET
                case ID_SPECIAL_RESET:
                    if(emu) {
                        emu->special_reset();
                    }
                    break;
#endif
                case ID_CPU_POWER0: case ID_CPU_POWER1: case ID_CPU_POWER2: case ID_CPU_POWER3: case ID_CPU_POWER4:
                    config.cpu_power = LOWORD(wParam) - ID_CPU_POWER0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
                case ID_FULL_SPEED:
                    config.full_speed = !config.full_speed;
                    break;
                case ID_DRIVE_VM_IN_OPECODE:
                    config.drive_vm_in_opecode = !config.drive_vm_in_opecode;
                    break;
#ifdef USE_AUTO_KEY
                case ID_AUTOKEY_START:
                    if(emu) {
                        start_auto_key(app);
                    }
                    break;
                case ID_AUTOKEY_STOP:
                    if(emu) {
                        emu->stop_auto_key();
                    }
                    break;
                case ID_ROMAJI_TO_KANA:
                    if(emu) {
                        if(!config.romaji_to_kana) {
                            emu->set_auto_key_char(1); // start
                            config.romaji_to_kana = true;
                        } else {
                            emu->set_auto_key_char(0); // end
                            config.romaji_to_kana = false;
                        }
                    }
                    break;
#endif
#ifdef USE_DEBUGGER
                case ID_OPEN_DEBUGGER0: case ID_OPEN_DEBUGGER1: case ID_OPEN_DEBUGGER2: case ID_OPEN_DEBUGGER3:
                case ID_OPEN_DEBUGGER4: case ID_OPEN_DEBUGGER5: case ID_OPEN_DEBUGGER6: case ID_OPEN_DEBUGGER7:
                    if(emu) {
                        emu->open_debugger(LOWORD(wParam) - ID_OPEN_DEBUGGER0);
                    }
                    break;
                case ID_CLOSE_DEBUGGER:
                    if(emu) {
                        emu->close_debugger();
                    }
                    break;
#endif
#ifdef USE_STATE
                case ID_SAVE_STATE0: case ID_SAVE_STATE1: case ID_SAVE_STATE2: case ID_SAVE_STATE3: case ID_SAVE_STATE4:
                case ID_SAVE_STATE5: case ID_SAVE_STATE6: case ID_SAVE_STATE7: case ID_SAVE_STATE8: case ID_SAVE_STATE9:
                    if(emu) {
                        emu->save_state(emu->state_file_path(LOWORD(wParam) - ID_SAVE_STATE0));
                    }
                    break;
                case ID_LOAD_STATE0: case ID_LOAD_STATE1: case ID_LOAD_STATE2: case ID_LOAD_STATE3: case ID_LOAD_STATE4:
                case ID_LOAD_STATE5: case ID_LOAD_STATE6: case ID_LOAD_STATE7: case ID_LOAD_STATE8: case ID_LOAD_STATE9:
                    if(emu) {
                        emu->load_state(emu->state_file_path(LOWORD(wParam) - ID_LOAD_STATE0));
                    }
                    break;
#endif
                case ID_EXIT:
                    //SendMessage(hWnd, WM_CLOSE, 0, 0L);
                    // 強制排出
                    LOGI("ID_EXIT");
                    all_eject();
                    exit(0);
                    break;
#ifdef USE_BOOT_MODE
                    case ID_VM_BOOT_MODE0: case ID_VM_BOOT_MODE1: case ID_VM_BOOT_MODE2: case ID_VM_BOOT_MODE3:
                case ID_VM_BOOT_MODE4: case ID_VM_BOOT_MODE5: case ID_VM_BOOT_MODE6: case ID_VM_BOOT_MODE7:
                    config.boot_mode = LOWORD(wParam) - ID_VM_BOOT_MODE0;
                    LOGI("Boot Mode: %d", config.boot_mode);
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_CPU_TYPE
                    case ID_VM_CPU_TYPE0: case ID_VM_CPU_TYPE1: case ID_VM_CPU_TYPE2: case ID_VM_CPU_TYPE3:
                case ID_VM_CPU_TYPE4: case ID_VM_CPU_TYPE5: case ID_VM_CPU_TYPE6: case ID_VM_CPU_TYPE7:
                    config.cpu_type = LOWORD(wParam) - ID_VM_CPU_TYPE0;
                    LOGI("CPU Type: %d", config.cpu_type);
                    // need to recreate vm class instance
//      			if(emu) {
//		        		emu->update_config();
//      			}
                    break;
#endif
#ifdef USE_DIPSWITCH
                    case ID_VM_DIPSWITCH0:  case ID_VM_DIPSWITCH1:  case ID_VM_DIPSWITCH2:  case ID_VM_DIPSWITCH3:
                case ID_VM_DIPSWITCH4:  case ID_VM_DIPSWITCH5:  case ID_VM_DIPSWITCH6:  case ID_VM_DIPSWITCH7:
                case ID_VM_DIPSWITCH8:  case ID_VM_DIPSWITCH9:  case ID_VM_DIPSWITCH10: case ID_VM_DIPSWITCH11:
                case ID_VM_DIPSWITCH12: case ID_VM_DIPSWITCH13: case ID_VM_DIPSWITCH14: case ID_VM_DIPSWITCH15:
                case ID_VM_DIPSWITCH16: case ID_VM_DIPSWITCH17: case ID_VM_DIPSWITCH18: case ID_VM_DIPSWITCH19:
                case ID_VM_DIPSWITCH20: case ID_VM_DIPSWITCH21: case ID_VM_DIPSWITCH22: case ID_VM_DIPSWITCH23:
                case ID_VM_DIPSWITCH24: case ID_VM_DIPSWITCH25: case ID_VM_DIPSWITCH26: case ID_VM_DIPSWITCH27:
                case ID_VM_DIPSWITCH28: case ID_VM_DIPSWITCH29: case ID_VM_DIPSWITCH30: case ID_VM_DIPSWITCH31:
                    config.dipswitch ^= (1 << (LOWORD(wParam) - ID_VM_DIPSWITCH0));
                    // 2進数でディップスイッチ情報をログに出す、2進数は真面目に計算して文字列を生成する
                    char dipswitch[33];
                    for (int i = 0; i < 32; i++) {
                        dipswitch[31 - i] = (config.dipswitch & (1 << i)) ? '1' : '0';
                    }
                    dipswitch[32] = '\0';
                    LOGI("DIP Switch: %s", dipswitch);
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_DEVICE_TYPE
                    case ID_VM_DEVICE_TYPE0: case ID_VM_DEVICE_TYPE1: case ID_VM_DEVICE_TYPE2: case ID_VM_DEVICE_TYPE3:
                case ID_VM_DEVICE_TYPE4: case ID_VM_DEVICE_TYPE5: case ID_VM_DEVICE_TYPE6: case ID_VM_DEVICE_TYPE7:
                    config.device_type = LOWORD(wParam) - ID_VM_DEVICE_TYPE0;
                    break;
#endif
#ifdef USE_DRIVE_TYPE
                case ID_VM_DRIVE_TYPE0: case ID_VM_DRIVE_TYPE1: case ID_VM_DRIVE_TYPE2: case ID_VM_DRIVE_TYPE3:
                case ID_VM_DRIVE_TYPE4: case ID_VM_DRIVE_TYPE5: case ID_VM_DRIVE_TYPE6: case ID_VM_DRIVE_TYPE7:
                    config.drive_type = LOWORD(wParam) - ID_VM_DRIVE_TYPE0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_KEYBOARD_TYPE
                case ID_VM_KEYBOARD_TYPE0: case ID_VM_KEYBOARD_TYPE1: case ID_VM_KEYBOARD_TYPE2: case ID_VM_KEYBOARD_TYPE3:
                case ID_VM_KEYBOARD_TYPE4: case ID_VM_KEYBOARD_TYPE5: case ID_VM_KEYBOARD_TYPE6: case ID_VM_KEYBOARD_TYPE7:
                    config.keyboard_type = LOWORD(wParam) - ID_VM_KEYBOARD_TYPE0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_MOUSE_TYPE
                    case ID_VM_MOUSE_TYPE0: case ID_VM_MOUSE_TYPE1: case ID_VM_MOUSE_TYPE2: case ID_VM_MOUSE_TYPE3:
                case ID_VM_MOUSE_TYPE4: case ID_VM_MOUSE_TYPE5: case ID_VM_MOUSE_TYPE6: case ID_VM_MOUSE_TYPE7:
                    config.mouse_type = LOWORD(wParam) - ID_VM_MOUSE_TYPE0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_JOYSTICK_TYPE
                    case ID_VM_JOYSTICK_TYPE0: case ID_VM_JOYSTICK_TYPE1: case ID_VM_JOYSTICK_TYPE2: case ID_VM_JOYSTICK_TYPE3:
                case ID_VM_JOYSTICK_TYPE4: case ID_VM_JOYSTICK_TYPE5: case ID_VM_JOYSTICK_TYPE6: case ID_VM_JOYSTICK_TYPE7:
                    config.joystick_type = LOWORD(wParam) - ID_VM_JOYSTICK_TYPE0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_SOUND_TYPE
                case ID_VM_SOUND_TYPE0: case ID_VM_SOUND_TYPE1: case ID_VM_SOUND_TYPE2: case ID_VM_SOUND_TYPE3:
                case ID_VM_SOUND_TYPE4: case ID_VM_SOUND_TYPE5: case ID_VM_SOUND_TYPE6: case ID_VM_SOUND_TYPE7:
                    config.sound_type = LOWORD(wParam) - ID_VM_SOUND_TYPE0;
                    //if(emu) {
                    //	emu->update_config();
                    //}
                    break;
#endif
#ifdef USE_FLOPPY_DISK
                case ID_VM_SOUND_NOISE_FDD:
                    config.sound_noise_fdd = !config.sound_noise_fdd;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_TAPE
                case ID_VM_SOUND_NOISE_CMT:
                    config.sound_noise_cmt = !config.sound_noise_cmt;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
                case ID_VM_SOUND_TAPE_SIGNAL:
                    config.sound_tape_signal = !config.sound_tape_signal;
                    break;
                case ID_VM_SOUND_TAPE_VOICE:
                    config.sound_tape_voice = !config.sound_tape_voice;
                    break;
#endif
#ifdef USE_MONITOR_TYPE
                case ID_VM_MONITOR_TYPE0: case ID_VM_MONITOR_TYPE1: case ID_VM_MONITOR_TYPE2: case ID_VM_MONITOR_TYPE3:
                case ID_VM_MONITOR_TYPE4: case ID_VM_MONITOR_TYPE5: case ID_VM_MONITOR_TYPE6: case ID_VM_MONITOR_TYPE7:
                    config.monitor_type = LOWORD(wParam) - ID_VM_MONITOR_TYPE0;
                    if(emu) {
#ifdef ONE_BOARD_MICRO_COMPUTER
                        emu->reload_bitmap();
                        emu->invalidate_screen();
#endif
                        emu->update_config();
                    }
                    break;
#endif
#ifdef USE_SCANLINE
                case ID_VM_MONITOR_SCANLINE:
                    config.scan_line = !config.scan_line;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
                case ID_VM_MONITOR_SCANLINE_AUTO:
                    config.scan_line_auto = !config.scan_line_auto;
                    break;
#endif
                case ID_SCREEN_BOTTOM_MARGIN_0: case ID_SCREEN_BOTTOM_MARGIN_30: case ID_SCREEN_BOTTOM_MARGIN_60: // Medamap
                case ID_SCREEN_BOTTOM_MARGIN_90: case ID_SCREEN_BOTTOM_MARGIN_120: case ID_SCREEN_BOTTOM_MARGIN_150:
                case ID_SCREEN_BOTTOM_MARGIN_180: case ID_SCREEN_BOTTOM_MARGIN_210: case ID_SCREEN_BOTTOM_MARGIN_240:
                case ID_SCREEN_BOTTOM_MARGIN_270:
                    config.screen_bottom_margin = (LOWORD(wParam) - ID_SCREEN_BOTTOM_MARGIN_0) * 30;
                    clear_screen(engine);
                    clear_screen(engine);
                    break;
#ifdef USE_PRINTER_TYPE
                case ID_VM_PRINTER_TYPE0: case ID_VM_PRINTER_TYPE1: case ID_VM_PRINTER_TYPE2: case ID_VM_PRINTER_TYPE3:
                case ID_VM_PRINTER_TYPE4: case ID_VM_PRINTER_TYPE5: case ID_VM_PRINTER_TYPE6: case ID_VM_PRINTER_TYPE7:
                    config.printer_type = LOWORD(wParam) - ID_VM_PRINTER_TYPE0;
                    break;
#endif
#ifdef USE_SERIAL_TYPE
                    case ID_VM_SERIAL_TYPE0: case ID_VM_SERIAL_TYPE1: case ID_VM_SERIAL_TYPE2: case ID_VM_SERIAL_TYPE3:
                case ID_VM_SERIAL_TYPE4: case ID_VM_SERIAL_TYPE5: case ID_VM_SERIAL_TYPE6: case ID_VM_SERIAL_TYPE7:
                    config.serial_type = LOWORD(wParam) - ID_VM_SERIAL_TYPE0;
                    break;
#endif
                case ID_HOST_REC_MOVIE_60FPS: case ID_HOST_REC_MOVIE_30FPS: case ID_HOST_REC_MOVIE_15FPS:
                    if(emu) {
                        static const int fps[3] = {60, 30, 15};
                        emu->start_record_sound();
                        if(!emu->start_record_video(fps[LOWORD(wParam) - ID_HOST_REC_MOVIE_60FPS])) {
                            emu->stop_record_sound();
                        }
                    }
                    break;
                case ID_HOST_REC_SOUND:
                    if(emu) {
                        emu->start_record_sound();
                    }
                    break;
                case ID_HOST_REC_STOP:
                    if(emu) {
                        emu->stop_record_video();
                        emu->stop_record_sound();
                    }
                    break;
                case ID_HOST_CAPTURE_SCREEN:
                    if(emu) {
                        emu->capture_screen();
                    }
                    break;
#ifdef SUPPORT_D2D1
                    case ID_HOST_USE_D2D1:
                    config.use_d2d1 = !config.use_d2d1;
#ifdef SUPPORT_D3D9
                    config.use_d3d9 = 0;
#endif
                    if(emu) {
                        emu->set_host_window_size(-1, -1, !now_fullscreen);
                    }
                    break;
#endif
#ifdef SUPPORT_D3D9
                    case ID_HOST_USE_D3D9:
#ifdef SUPPORT_D2D1
                    config.use_d2d1 = 0;
#endif
                    config.use_d3d9 = !config.use_d3d9;
                    if(emu) {
                        emu->set_host_window_size(-1, -1, !now_fullscreen);
                    }
                    break;
                case ID_HOST_WAIT_VSYNC:
                    config.wait_vsync = !config.wait_vsync;
                    if(emu) {
                        emu->set_host_window_size(-1, -1, !now_fullscreen);
                    }
                    break;
#endif
#if !defined(__ANDROID__)
                    case ID_HOST_USE_DINPUT:
                    config.use_dinput = !config.use_dinput;
                    break;
                case ID_HOST_DISABLE_DWM:
                    config.disable_dwm = !config.disable_dwm;
                    break;
                case ID_HOST_SHOW_STATUS_BAR:
                    config.show_status_bar = !config.show_status_bar;
                    if(emu) {
                        if(!now_fullscreen) {
                            set_window(hWnd, prev_window_mode);
                        }
#ifdef SUPPORT_D2D1
                        emu->set_host_window_size(-1, -1, !now_fullscreen);
#endif
                    }
                    break;
                case ID_SCREEN_WINDOW + 0: case ID_SCREEN_WINDOW + 1: case ID_SCREEN_WINDOW + 2: case ID_SCREEN_WINDOW + 3: case ID_SCREEN_WINDOW + 4:
                case ID_SCREEN_WINDOW + 5: case ID_SCREEN_WINDOW + 6: case ID_SCREEN_WINDOW + 7: case ID_SCREEN_WINDOW + 8: case ID_SCREEN_WINDOW + 9:
                    if(emu) {
                        set_window(hWnd, LOWORD(wParam) - ID_SCREEN_WINDOW);
                    }
                    break;
                case ID_SCREEN_FULLSCREEN +  0: case ID_SCREEN_FULLSCREEN +  1: case ID_SCREEN_FULLSCREEN +  2: case ID_SCREEN_FULLSCREEN +  3: case ID_SCREEN_FULLSCREEN +  4:
                case ID_SCREEN_FULLSCREEN +  5: case ID_SCREEN_FULLSCREEN +  6: case ID_SCREEN_FULLSCREEN +  7: case ID_SCREEN_FULLSCREEN +  8: case ID_SCREEN_FULLSCREEN +  9:
                case ID_SCREEN_FULLSCREEN + 10: case ID_SCREEN_FULLSCREEN + 11: case ID_SCREEN_FULLSCREEN + 12: case ID_SCREEN_FULLSCREEN + 13: case ID_SCREEN_FULLSCREEN + 14:
                case ID_SCREEN_FULLSCREEN + 15: case ID_SCREEN_FULLSCREEN + 16: case ID_SCREEN_FULLSCREEN + 17: case ID_SCREEN_FULLSCREEN + 18: case ID_SCREEN_FULLSCREEN + 19:
                case ID_SCREEN_FULLSCREEN + 20: case ID_SCREEN_FULLSCREEN + 21: case ID_SCREEN_FULLSCREEN + 22: case ID_SCREEN_FULLSCREEN + 23: case ID_SCREEN_FULLSCREEN + 24:
                case ID_SCREEN_FULLSCREEN + 25: case ID_SCREEN_FULLSCREEN + 26: case ID_SCREEN_FULLSCREEN + 27: case ID_SCREEN_FULLSCREEN + 28: case ID_SCREEN_FULLSCREEN + 29:
                case ID_SCREEN_FULLSCREEN + 30: case ID_SCREEN_FULLSCREEN + 31: case ID_SCREEN_FULLSCREEN + 32: case ID_SCREEN_FULLSCREEN + 33: case ID_SCREEN_FULLSCREEN + 34:
                case ID_SCREEN_FULLSCREEN + 35: case ID_SCREEN_FULLSCREEN + 36: case ID_SCREEN_FULLSCREEN + 37: case ID_SCREEN_FULLSCREEN + 38: case ID_SCREEN_FULLSCREEN + 39:
                case ID_SCREEN_FULLSCREEN + 40: case ID_SCREEN_FULLSCREEN + 41: case ID_SCREEN_FULLSCREEN + 42: case ID_SCREEN_FULLSCREEN + 43: case ID_SCREEN_FULLSCREEN + 44:
                case ID_SCREEN_FULLSCREEN + 45: case ID_SCREEN_FULLSCREEN + 46: case ID_SCREEN_FULLSCREEN + 47: case ID_SCREEN_FULLSCREEN + 48: case ID_SCREEN_FULLSCREEN + 49:
                    if(emu && !now_fullscreen) {
                        set_window(hWnd, LOWORD(wParam) - ID_SCREEN_FULLSCREEN + MAX_WINDOW);
                    }
                    break;
                case ID_SCREEN_WINDOW_STRETCH:
                case ID_SCREEN_WINDOW_ASPECT:
                    config.window_stretch_type = LOWORD(wParam) - ID_SCREEN_WINDOW_STRETCH;
                    if(emu) {
                        if(!now_fullscreen) {
                            set_window(hWnd, prev_window_mode);
                        }
                    }
                    break;
                case ID_SCREEN_FULLSCREEN_DOTBYDOT:
                case ID_SCREEN_FULLSCREEN_STRETCH:
                case ID_SCREEN_FULLSCREEN_ASPECT:
                case ID_SCREEN_FULLSCREEN_FILL:
                    config.fullscreen_stretch_type = LOWORD(wParam) - ID_SCREEN_FULLSCREEN_DOTBYDOT;
                    if(emu) {
                        if(now_fullscreen) {
                            emu->set_host_window_size(-1, -1, false);
                        }
                    }
                    break;
//#ifdef USE_SCREEN_ROTATE
                case ID_SCREEN_ROTATE_0: case ID_SCREEN_ROTATE_90: case ID_SCREEN_ROTATE_180: case ID_SCREEN_ROTATE_270:
                    config.rotate_type = LOWORD(wParam) - ID_SCREEN_ROTATE_0;
                    if(emu) {
                        if(now_fullscreen) {
                            emu->set_host_window_size(-1, -1, false);
                        } else {
                            set_window(hWnd, prev_window_mode);
                        }
                    }
                    break;
//#endif
#endif
#ifdef USE_SCREEN_FILTER
                case ID_FILTER_RGB:
                    config.filter_type = SCREEN_FILTER_RGB;
                    break;
                case ID_FILTER_RF:
                    config.filter_type = SCREEN_FILTER_RF;
                    break;
                case ID_FILTER_NONE:
                    config.filter_type = SCREEN_FILTER_NONE;
                    break;
#endif
                case ID_SOUND_ON:
                    emu->get_osd()->reset_sound();
                    emu->get_osd()->soundEnable = !(emu->get_osd()->soundEnable);
                    config.sound_on = emu->get_osd()->soundEnable;
                    break;
                case ID_SOUND_FREQ0: case ID_SOUND_FREQ1: case ID_SOUND_FREQ2: case ID_SOUND_FREQ3:
                case ID_SOUND_FREQ4: case ID_SOUND_FREQ5: case ID_SOUND_FREQ6: case ID_SOUND_FREQ7:
                    config.sound_frequency = LOWORD(wParam) - ID_SOUND_FREQ0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
                case ID_SOUND_LATE0: case ID_SOUND_LATE1: case ID_SOUND_LATE2: case ID_SOUND_LATE3: case ID_SOUND_LATE4:
                    config.sound_latency = LOWORD(wParam) - ID_SOUND_LATE0;
                    if(emu) {
                        emu->update_config();
                    }
                    break;
                case ID_SOUND_STRICT_RENDER: case ID_SOUND_LIGHT_RENDER:
                    config.sound_strict_rendering = (LOWORD(wParam) == ID_SOUND_STRICT_RENDER);
                    if(emu) {
                        emu->update_config();
                    }
                    break;
#if !defined(__ANDROID__)
                    #ifdef USE_SOUND_VOLUME
                case ID_SOUND_VOLUME:
                    // thanks Marukun (64bit)
                    DialogBoxParam((HINSTANCE)GetModuleHandle(0), MAKEINTRESOURCE(IDD_VOLUME), hWnd, reinterpret_cast<DLGPROC>(VolumeWndProc), 0);
                    break;
#endif
#ifdef USE_JOYSTICK
                case ID_INPUT_JOYSTICK0: case ID_INPUT_JOYSTICK1: case ID_INPUT_JOYSTICK2: case ID_INPUT_JOYSTICK3:
                case ID_INPUT_JOYSTICK4: case ID_INPUT_JOYSTICK5: case ID_INPUT_JOYSTICK6: case ID_INPUT_JOYSTICK7:
                    {
                        // thanks Marukun (64bit)
                        LONG index = LOWORD(wParam) - ID_INPUT_JOYSTICK0;
                        DialogBoxParam((HINSTANCE)GetModuleHandle(0), MAKEINTRESOURCE(IDD_JOYSTICK), hWnd, reinterpret_cast<DLGPROC>(JoyWndProc), (LPARAM)&index);
                    }
                    break;
                case ID_INPUT_JOYTOKEY:
                {
                // thanks Marukun (64bit)
                LONG index = 0;
                DialogBoxParam((HINSTANCE)GetModuleHandle(0), MAKEINTRESOURCE(IDD_JOYTOKEY), hWnd, reinterpret_cast<DLGPROC>(JoyToKeyWndProc), (LPARAM)&index);
                }
                break;
#endif
#endif
#ifdef USE_VIDEO_CAPTURE
                    case ID_CAPTURE_FILTER:
                    if(emu) {
                        emu->show_capture_dev_filter();
                    }
                    break;
                case ID_CAPTURE_PIN:
                    if(emu) {
                        emu->show_capture_dev_pin();
                    }
                    break;
                case ID_CAPTURE_SOURCE:
                    if(emu) {
                        emu->show_capture_dev_source();
                    }
                    break;
                case ID_CAPTURE_CLOSE:
                    if(emu) {
                        emu->close_capture_dev();
                    }
                    break;
                case ID_CAPTURE_DEVICE + 0: case ID_CAPTURE_DEVICE + 1: case ID_CAPTURE_DEVICE + 2: case ID_CAPTURE_DEVICE + 3:
                case ID_CAPTURE_DEVICE + 4: case ID_CAPTURE_DEVICE + 5: case ID_CAPTURE_DEVICE + 6: case ID_CAPTURE_DEVICE + 7:
                    if(emu) {
                        emu->open_capture_dev(LOWORD(wParam) - ID_CAPTURE_DEVICE, false);
                    }
                    break;
#endif
#ifdef USE_CART
                    #if USE_CART >= 1
#define CART_MENU_ITEMS(drv, ID_OPEN_CART, ID_CLOSE_CART, ID_RECENT_CART) \
                case ID_OPEN_CART: \
                    if(emu) { \
                        open_cart_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_CART: \
                    if(emu) { \
                        emu->close_cart(drv); \
                    } \
                    break; \
                case ID_RECENT_CART + 0: case ID_RECENT_CART + 1: case ID_RECENT_CART + 2: case ID_RECENT_CART + 3: \
                case ID_RECENT_CART + 4: case ID_RECENT_CART + 5: case ID_RECENT_CART + 6: case ID_RECENT_CART + 7: \
                    if(emu) { \
                        open_recent_cart(drv, LOWORD(wParam) - ID_RECENT_CART); \
                    } \
                    break;
                CART_MENU_ITEMS(0, ID_OPEN_CART1, ID_CLOSE_CART1, ID_RECENT_CART1)
#endif
#if USE_CART >= 2
                CART_MENU_ITEMS(1, ID_OPEN_CART2, ID_CLOSE_CART2, ID_RECENT_CART2)
#endif
#endif
#ifdef USE_FLOPPY_DISK
#if USE_FLOPPY_DISK >= 1
#define FD_MENU_ITEMS(drv, ID_OPEN_FD, ID_CLOSE_FD, ID_OPEN_BLANK_2D_FD, ID_OPEN_BLANK_2DD_FD, ID_OPEN_BLANK_2HD_FD, ID_WRITE_PROTECT_FD, ID_CORRECT_TIMING_FD, ID_IGNORE_CRC_FD, ID_RECENT_FD, ID_SELECT_D88_BANK, ID_EJECT_D88_BANK) \
                case ID_OPEN_FD: \
                    if(emu) { \
                        open_floppy_disk_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_FD: \
                    if(emu) { \
                        emu->close_floppy_disk(drv); \
                    } \
                    break; \
                case ID_OPEN_BLANK_2D_FD: \
                    if(emu) { \
                        open_blank_floppy_disk_dialog(app, drv, 0x00); \
                    } \
                    break; \
                case ID_OPEN_BLANK_2DD_FD: \
                    if(emu) { \
                        open_blank_floppy_disk_dialog(app, drv, 0x10); \
                    } \
                    break; \
                case ID_OPEN_BLANK_2HD_FD: \
                    if(emu) { \
                        open_blank_floppy_disk_dialog(app, drv, 0x20); \
                    } \
                    break; \
                case ID_WRITE_PROTECT_FD: \
                    if(emu) { \
                        emu->is_floppy_disk_protected(drv, !emu->is_floppy_disk_protected(drv)); \
                    } \
                    break; \
                case ID_CORRECT_TIMING_FD: \
                    config.correct_disk_timing[drv] = !config.correct_disk_timing[drv]; \
                    break; \
                case ID_IGNORE_CRC_FD: \
                    config.ignore_disk_crc[drv] = !config.ignore_disk_crc[drv]; \
                    break; \
                case ID_RECENT_FD + 0: case ID_RECENT_FD + 1: case ID_RECENT_FD + 2: case ID_RECENT_FD + 3: \
                case ID_RECENT_FD + 4: case ID_RECENT_FD + 5: case ID_RECENT_FD + 6: case ID_RECENT_FD + 7: \
                    if(emu) { \
                        open_recent_floppy_disk(drv, LOWORD(wParam) - ID_RECENT_FD); \
                    } \
                    break; \
                case ID_SELECT_D88_BANK +  0: case ID_SELECT_D88_BANK +  1: case ID_SELECT_D88_BANK +  2: case ID_SELECT_D88_BANK +  3: \
                case ID_SELECT_D88_BANK +  4: case ID_SELECT_D88_BANK +  5: case ID_SELECT_D88_BANK +  6: case ID_SELECT_D88_BANK +  7: \
                case ID_SELECT_D88_BANK +  8: case ID_SELECT_D88_BANK +  9: case ID_SELECT_D88_BANK + 10: case ID_SELECT_D88_BANK + 11: \
                case ID_SELECT_D88_BANK + 12: case ID_SELECT_D88_BANK + 13: case ID_SELECT_D88_BANK + 14: case ID_SELECT_D88_BANK + 15: \
                case ID_SELECT_D88_BANK + 16: case ID_SELECT_D88_BANK + 17: case ID_SELECT_D88_BANK + 18: case ID_SELECT_D88_BANK + 19: \
                case ID_SELECT_D88_BANK + 20: case ID_SELECT_D88_BANK + 21: case ID_SELECT_D88_BANK + 22: case ID_SELECT_D88_BANK + 23: \
                case ID_SELECT_D88_BANK + 24: case ID_SELECT_D88_BANK + 25: case ID_SELECT_D88_BANK + 26: case ID_SELECT_D88_BANK + 27: \
                case ID_SELECT_D88_BANK + 28: case ID_SELECT_D88_BANK + 29: case ID_SELECT_D88_BANK + 30: case ID_SELECT_D88_BANK + 31: \
                case ID_SELECT_D88_BANK + 32: case ID_SELECT_D88_BANK + 33: case ID_SELECT_D88_BANK + 34: case ID_SELECT_D88_BANK + 35: \
                case ID_SELECT_D88_BANK + 36: case ID_SELECT_D88_BANK + 37: case ID_SELECT_D88_BANK + 38: case ID_SELECT_D88_BANK + 39: \
                case ID_SELECT_D88_BANK + 40: case ID_SELECT_D88_BANK + 41: case ID_SELECT_D88_BANK + 42: case ID_SELECT_D88_BANK + 43: \
                case ID_SELECT_D88_BANK + 44: case ID_SELECT_D88_BANK + 45: case ID_SELECT_D88_BANK + 46: case ID_SELECT_D88_BANK + 47: \
                case ID_SELECT_D88_BANK + 48: case ID_SELECT_D88_BANK + 49: case ID_SELECT_D88_BANK + 50: case ID_SELECT_D88_BANK + 51: \
                case ID_SELECT_D88_BANK + 52: case ID_SELECT_D88_BANK + 53: case ID_SELECT_D88_BANK + 54: case ID_SELECT_D88_BANK + 55: \
                case ID_SELECT_D88_BANK + 56: case ID_SELECT_D88_BANK + 57: case ID_SELECT_D88_BANK + 58: case ID_SELECT_D88_BANK + 59: \
                case ID_SELECT_D88_BANK + 60: case ID_SELECT_D88_BANK + 61: case ID_SELECT_D88_BANK + 62: case ID_SELECT_D88_BANK + 63: \
                    if(emu) { \
                        select_d88_bank(drv, LOWORD(wParam) - ID_SELECT_D88_BANK); \
                    } \
                    break; \
                case ID_EJECT_D88_BANK: \
                    if(emu) { \
                        select_d88_bank(drv, -1); \
                    } \
                    break;
                FD_MENU_ITEMS(0, ID_OPEN_FD1, ID_CLOSE_FD1, ID_OPEN_BLANK_2D_FD1, ID_OPEN_BLANK_2DD_FD1, ID_OPEN_BLANK_2HD_FD1, ID_WRITE_PROTECT_FD1, ID_CORRECT_TIMING_FD1, ID_IGNORE_CRC_FD1, ID_RECENT_FD1, ID_SELECT_D88_BANK1, ID_EJECT_D88_BANK1)
#endif
#if USE_FLOPPY_DISK >= 2
                FD_MENU_ITEMS(1, ID_OPEN_FD2, ID_CLOSE_FD2, ID_OPEN_BLANK_2D_FD2, ID_OPEN_BLANK_2DD_FD2, ID_OPEN_BLANK_2HD_FD2, ID_WRITE_PROTECT_FD2, ID_CORRECT_TIMING_FD2, ID_IGNORE_CRC_FD2, ID_RECENT_FD2, ID_SELECT_D88_BANK2, ID_EJECT_D88_BANK2)
#endif
#if USE_FLOPPY_DISK >= 3
                FD_MENU_ITEMS(2, ID_OPEN_FD3, ID_CLOSE_FD3, ID_OPEN_BLANK_2D_FD3, ID_OPEN_BLANK_2DD_FD3, ID_OPEN_BLANK_2HD_FD3, ID_WRITE_PROTECT_FD3, ID_CORRECT_TIMING_FD3, ID_IGNORE_CRC_FD3, ID_RECENT_FD3, ID_SELECT_D88_BANK3, ID_EJECT_D88_BANK3)
#endif
#if USE_FLOPPY_DISK >= 4
                FD_MENU_ITEMS(3, ID_OPEN_FD4, ID_CLOSE_FD4, ID_OPEN_BLANK_2D_FD4, ID_OPEN_BLANK_2DD_FD4, ID_OPEN_BLANK_2HD_FD4, ID_WRITE_PROTECT_FD4, ID_CORRECT_TIMING_FD4, ID_IGNORE_CRC_FD4, ID_RECENT_FD4, ID_SELECT_D88_BANK4, ID_EJECT_D88_BANK4)
#endif
#if USE_FLOPPY_DISK >= 5
                    FD_MENU_ITEMS(4, ID_OPEN_FD5, ID_CLOSE_FD5, ID_OPEN_BLANK_2D_FD5, ID_OPEN_BLANK_2DD_FD5, ID_OPEN_BLANK_2HD_FD5, ID_WRITE_PROTECT_FD5, ID_CORRECT_TIMING_FD5, ID_IGNORE_CRC_FD5, ID_RECENT_FD5, ID_SELECT_D88_BANK5, ID_EJECT_D88_BANK5)
#endif
#if USE_FLOPPY_DISK >= 6
                    FD_MENU_ITEMS(5, ID_OPEN_FD6, ID_CLOSE_FD6, ID_OPEN_BLANK_2D_FD6, ID_OPEN_BLANK_2DD_FD6, ID_OPEN_BLANK_2HD_FD6, ID_WRITE_PROTECT_FD6, ID_CORRECT_TIMING_FD6, ID_IGNORE_CRC_FD6, ID_RECENT_FD6, ID_SELECT_D88_BANK6, ID_EJECT_D88_BANK6)
#endif
#if USE_FLOPPY_DISK >= 7
                    FD_MENU_ITEMS(6, ID_OPEN_FD7, ID_CLOSE_FD7, ID_OPEN_BLANK_2D_FD7, ID_OPEN_BLANK_2DD_FD7, ID_OPEN_BLANK_2HD_FD7, ID_WRITE_PROTECT_FD7, ID_CORRECT_TIMING_FD7, ID_IGNORE_CRC_FD7, ID_RECENT_FD7, ID_SELECT_D88_BANK7, ID_EJECT_D88_BANK7)
#endif
#if USE_FLOPPY_DISK >= 8
                    FD_MENU_ITEMS(7, ID_OPEN_FD8, ID_CLOSE_FD8, ID_OPEN_BLANK_2D_FD8, ID_OPEN_BLANK_2DD_FD8, ID_OPEN_BLANK_2HD_FD8, ID_WRITE_PROTECT_FD8, ID_CORRECT_TIMING_FD8, ID_IGNORE_CRC_FD8, ID_RECENT_FD8, ID_SELECT_D88_BANK8, ID_EJECT_D88_BANK8)
#endif
#endif
#ifdef USE_QUICK_DISK
                    #if USE_QUICK_DISK >= 1
#define QD_MENU_ITEMS(drv, ID_OPEN_QD, ID_CLOSE_QD, ID_RECENT_QD) \
                case ID_OPEN_QD: \
                    if(emu) { \
                        open_quick_disk_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_QD: \
                    if(emu) { \
                        emu->close_quick_disk(drv); \
                    } \
                    break; \
                case ID_RECENT_QD + 0: case ID_RECENT_QD + 1: case ID_RECENT_QD + 2: case ID_RECENT_QD + 3: \
                case ID_RECENT_QD + 4: case ID_RECENT_QD + 5: case ID_RECENT_QD + 6: case ID_RECENT_QD + 7: \
                    if(emu) { \
                        open_recent_quick_disk(drv, LOWORD(wParam) - ID_RECENT_QD); \
                    } \
                    break;
                QD_MENU_ITEMS(0, ID_OPEN_QD1, ID_CLOSE_QD1, ID_RECENT_QD1)
#endif
#if USE_QUICK_DISK >= 2
                QD_MENU_ITEMS(1, ID_OPEN_QD2, ID_CLOSE_QD2, ID_RECENT_QD2)
#endif
#endif
#ifdef USE_HARD_DISK
#if USE_HARD_DISK >= 1
#define HD_MENU_ITEMS(drv, ID_OPEN_HD, ID_CLOSE_HD, ID_OPEN_BLANK_20MB_HD, ID_OPEN_BLANK_20MB_1024_HD, ID_OPEN_BLANK_40MB_HD, ID_RECENT_HD) \
                case ID_OPEN_HD: \
                    if(emu) { \
                        open_hard_disk_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_HD: \
                    if(emu) { \
                        emu->close_hard_disk(drv); \
                    } \
                    break; \
                case ID_OPEN_BLANK_20MB_HD: \
                    if(emu) { \
                        open_blank_hard_disk_dialog(app, drv, 256, 33, 4, 615); \
                    } \
                    break; \
                case ID_OPEN_BLANK_20MB_1024_HD: \
                    if(emu) { \
                        open_blank_hard_disk_dialog(app, drv, 1024, 8, 4, 615); \
                    } \
                    break; \
                case ID_OPEN_BLANK_40MB_HD: \
                    if(emu) { \
                        open_blank_hard_disk_dialog(app, drv, 256, 33, 8, 615); \
                    } \
                    break; \
                case ID_RECENT_HD + 0: case ID_RECENT_HD + 1: case ID_RECENT_HD + 2: case ID_RECENT_HD + 3: \
                case ID_RECENT_HD + 4: case ID_RECENT_HD + 5: case ID_RECENT_HD + 6: case ID_RECENT_HD + 7: \
                    if(emu) { \
                        open_recent_hard_disk(drv, LOWORD(wParam) - ID_RECENT_HD); \
                    } \
                    break;
                HD_MENU_ITEMS(0, ID_OPEN_HD1, ID_CLOSE_HD1, ID_OPEN_BLANK_20MB_HD1, ID_OPEN_BLANK_20MB_1024_HD1, ID_OPEN_BLANK_40MB_HD1, ID_RECENT_HD1)
#endif
#if USE_HARD_DISK >= 2
                HD_MENU_ITEMS(1, ID_OPEN_HD2, ID_CLOSE_HD2, ID_OPEN_BLANK_20MB_HD2, ID_OPEN_BLANK_20MB_1024_HD2, ID_OPEN_BLANK_40MB_HD2, ID_RECENT_HD2)
#endif
#if USE_HARD_DISK >= 3
                HD_MENU_ITEMS(2, ID_OPEN_HD3, ID_CLOSE_HD3, ID_OPEN_BLANK_20MB_HD3, ID_OPEN_BLANK_20MB_1024_HD3, ID_OPEN_BLANK_40MB_HD3, ID_RECENT_HD3)
#endif
#if USE_HARD_DISK >= 4
                HD_MENU_ITEMS(3, ID_OPEN_HD4, ID_CLOSE_HD4, ID_OPEN_BLANK_20MB_HD4, ID_OPEN_BLANK_20MB_1024_HD4, ID_OPEN_BLANK_40MB_HD4, ID_RECENT_HD4)
#endif
#if USE_HARD_DISK >= 5
                    HD_MENU_ITEMS(4, ID_OPEN_HD5, ID_CLOSE_HD5, ID_OPEN_BLANK_20MB_HD5, ID_OPEN_BLANK_20MB_1024_HD5, ID_OPEN_BLANK_40MB_HD5, ID_RECENT_HD5)
#endif
#if USE_HARD_DISK >= 6
                    HD_MENU_ITEMS(5, ID_OPEN_HD6, ID_CLOSE_HD6, ID_OPEN_BLANK_20MB_HD6, ID_OPEN_BLANK_20MB_1024_HD6, ID_OPEN_BLANK_40MB_HD6, ID_RECENT_HD6)
#endif
#if USE_HARD_DISK >= 7
                    HD_MENU_ITEMS(6, ID_OPEN_HD7, ID_CLOSE_HD7, ID_OPEN_BLANK_20MB_HD7, ID_OPEN_BLANK_20MB_1024_HD7, ID_OPEN_BLANK_40MB_HD7, ID_RECENT_HD7)
#endif
#if USE_HARD_DISK >= 8
                    HD_MENU_ITEMS(7, ID_OPEN_HD8, ID_CLOSE_HD8, ID_OPEN_BLANK_20MB_HD8, ID_OPEN_BLANK_20MB_1024_HD8, ID_OPEN_BLANK_40MB_HD8, ID_RECENT_HD8)
#endif
#endif
#ifdef USE_TAPE
#if USE_TAPE >= 1
#define TAPE_MENU_ITEMS(drv, ID_PLAY_TAPE, ID_REC_TAPE, ID_CLOSE_TAPE, ID_PLAY_BUTTON, ID_STOP_BUTTON, ID_FAST_FORWARD, ID_FAST_REWIND, ID_APSS_FORWARD, ID_APSS_REWIND, ID_USE_WAVE_SHAPER, ID_DIRECT_LOAD_MZT, ID_TAPE_BAUD_LOW, ID_TAPE_BAUD_HIGH, ID_RECENT_TAPE) \
                case ID_PLAY_TAPE: \
                    if(emu) { \
                        open_tape_dialog(app, drv, true); \
                    } \
                    break; \
                case ID_REC_TAPE: \
                    if(emu) { \
                        open_tape_dialog(app, drv, false); \
                    } \
                    break; \
                case ID_CLOSE_TAPE: \
                    if(emu) { \
                        emu->close_tape(drv); \
                    } \
                    break; \
                case ID_PLAY_BUTTON: \
                    if(emu) { \
                        emu->push_play(drv); \
                    } \
                    break; \
                case ID_STOP_BUTTON: \
                    if(emu) { \
                        emu->push_stop(drv); \
                    } \
                    break; \
                case ID_FAST_FORWARD: \
                    if(emu) { \
                        emu->push_fast_forward(drv); \
                    } \
                    break; \
                case ID_FAST_REWIND: \
                    if(emu) { \
                        emu->push_fast_rewind(drv); \
                    } \
                    break; \
                case ID_APSS_FORWARD: \
                    if(emu) { \
                        emu->push_apss_forward(drv); \
                    } \
                    break; \
                case ID_APSS_REWIND: \
                    if(emu) { \
                        emu->push_apss_rewind(drv); \
                    } \
                    break; \
                case ID_USE_WAVE_SHAPER: \
                    config.wave_shaper[drv] = !config.wave_shaper[drv]; \
                    break; \
                case ID_DIRECT_LOAD_MZT: \
                    config.direct_load_mzt[drv] = !config.direct_load_mzt[drv]; \
                    break; \
                case ID_TAPE_BAUD_LOW: \
                    config.baud_high[drv] = false; \
                    break; \
                case ID_TAPE_BAUD_HIGH: \
                    config.baud_high[drv] = true; \
                    break; \
                case ID_RECENT_TAPE + 0: case ID_RECENT_TAPE + 1: case ID_RECENT_TAPE + 2: case ID_RECENT_TAPE + 3: \
                case ID_RECENT_TAPE + 4: case ID_RECENT_TAPE + 5: case ID_RECENT_TAPE + 6: case ID_RECENT_TAPE + 7: \
                    if(emu) { \
                        open_recent_tape(drv, LOWORD(wParam) - ID_RECENT_TAPE); \
                    } \
                    break;
                TAPE_MENU_ITEMS(0, ID_PLAY_TAPE1, ID_REC_TAPE1, ID_CLOSE_TAPE1, ID_PLAY_BUTTON1, ID_STOP_BUTTON1, ID_FAST_FORWARD1, ID_FAST_REWIND1, ID_APSS_FORWARD1, ID_APSS_REWIND1, ID_USE_WAVE_SHAPER1, ID_DIRECT_LOAD_MZT1, ID_TAPE_BAUD_LOW1, ID_TAPE_BAUD_HIGH1, ID_RECENT_TAPE1)
#endif
#if USE_TAPE >= 2
                    TAPE_MENU_ITEMS(1, ID_PLAY_TAPE2, ID_REC_TAPE2, ID_CLOSE_TAPE2, ID_PLAY_BUTTON2, ID_STOP_BUTTON2, ID_FAST_FORWARD2, ID_FAST_REWIND2, ID_APSS_FORWARD2, ID_APSS_REWIND2, ID_USE_WAVE_SHAPER2, ID_DIRECT_LOAD_MZT2, ID_TAPE_BAUD_LOW2, ID_TAPE_BAUD_HIGH2, ID_RECENT_TAPE2)
#endif
#endif
#ifdef USE_COMPACT_DISC
                    #if USE_COMPACT_DISC >= 1
#define COMPACT_DISC_MENU_ITEMS(drv, ID_OPEN_COMPACT_DISC, ID_CLOSE_COMPACT_DISC, ID_RECENT_COMPACT_DISC) \
                case ID_OPEN_COMPACT_DISC: \
                    if(emu) { \
                        open_compact_disc_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_COMPACT_DISC: \
                    if(emu) { \
                        emu->close_compact_disc(drv); \
                    } \
                    break; \
                case ID_RECENT_COMPACT_DISC + 0: case ID_RECENT_COMPACT_DISC + 1: case ID_RECENT_COMPACT_DISC + 2: case ID_RECENT_COMPACT_DISC + 3: \
                case ID_RECENT_COMPACT_DISC + 4: case ID_RECENT_COMPACT_DISC + 5: case ID_RECENT_COMPACT_DISC + 6: case ID_RECENT_COMPACT_DISC + 7: \
                    if(emu) { \
                        open_recent_compact_disc(drv, LOWORD(wParam) - ID_RECENT_COMPACT_DISC); \
                    } \
                    break;
                COMPACT_DISC_MENU_ITEMS(0, ID_OPEN_COMPACT_DISC1, ID_CLOSE_COMPACT_DISC1, ID_RECENT_COMPACT_DISC1)
#endif
#if USE_COMPACT_DISC >= 2
                COMPACT_DISC_MENU_ITEMS(1, ID_OPEN_COMPACT_DISC2, ID_CLOSE_COMPACT_DISC2, ID_RECENT_COMPACT_DISC2)
#endif
#endif
#ifdef USE_LASER_DISC
                    #if USE_LASER_DISC >= 1
#define LASER_DISC_MENU_ITEMS(drv, ID_OPEN_LASER_DISC, ID_CLOSE_LASER_DISC, ID_RECENT_LASER_DISC) \
                case ID_OPEN_LASER_DISC: \
                    if(emu) { \
                        open_laser_disc_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_LASER_DISC: \
                    if(emu) { \
                        emu->close_laser_disc(drv); \
                    } \
                    break; \
                case ID_RECENT_LASER_DISC + 0: case ID_RECENT_LASER_DISC + 1: case ID_RECENT_LASER_DISC + 2: case ID_RECENT_LASER_DISC + 3: \
                case ID_RECENT_LASER_DISC + 4: case ID_RECENT_LASER_DISC + 5: case ID_RECENT_LASER_DISC + 6: case ID_RECENT_LASER_DISC + 7: \
                    if(emu) { \
                        open_recent_laser_disc(drv, LOWORD(wParam) - ID_RECENT_LASER_DISC); \
                    } \
                    break;
                LASER_DISC_MENU_ITEMS(0, ID_OPEN_LASER_DISC1, ID_CLOSE_LASER_DISC1, ID_RECENT_LASER_DISC1)
#endif
#if USE_LASER_DISC >= 2
                LASER_DISC_MENU_ITEMS(1, ID_OPEN_LASER_DISC2, ID_CLOSE_LASER_DISC2, ID_RECENT_LASER_DISC2)
#endif
#endif
#ifdef USE_BINARY_FILE
                    #if USE_BINARY_FILE >= 1
#define BINARY_MENU_ITEMS(drv, ID_LOAD_BINARY, ID_SAVE_BINARY, ID_RECENT_BINARY) \
                case ID_LOAD_BINARY: \
                    if(emu) { \
                        open_binary_dialog(app, drv, true); \
                    } \
                    break; \
                case ID_SAVE_BINARY: \
                    if(emu) { \
                        open_binary_dialog(app, drv, false); \
                    } \
                    break; \
                case ID_RECENT_BINARY + 0: case ID_RECENT_BINARY + 1: case ID_RECENT_BINARY + 2: case ID_RECENT_BINARY + 3: \
                case ID_RECENT_BINARY + 4: case ID_RECENT_BINARY + 5: case ID_RECENT_BINARY + 6: case ID_RECENT_BINARY + 7: \
                    if(emu) { \
                        open_recent_binary(drv, LOWORD(wParam) - ID_RECENT_BINARY); \
                    } \
                    break;
                BINARY_MENU_ITEMS(0, ID_LOAD_BINARY1, ID_SAVE_BINARY1, ID_RECENT_BINARY1)
#endif
#if USE_BINARY_FILE >= 2
                BINARY_MENU_ITEMS(1, ID_LOAD_BINARY2, ID_SAVE_BINARY2, ID_RECENT_BINARY2)
#endif
#endif
#ifdef USE_BUBBLE
                    #if USE_BUBBLE >= 1
#define BUBBLE_CASETTE_MENU_ITEMS(drv, ID_OPEN_BUBBLE, ID_CLOSE_BUBBLE, ID_RECENT_BUBBLE) \
                case ID_OPEN_BUBBLE: \
                    if(emu) { \
                        open_bubble_casette_dialog(app, drv); \
                    } \
                    break; \
                case ID_CLOSE_BUBBLE: \
                    if(emu) { \
                        emu->close_bubble_casette(drv); \
                    } \
                    break; \
                case ID_RECENT_BUBBLE + 0: case ID_RECENT_BUBBLE + 1: case ID_RECENT_BUBBLE + 2: case ID_RECENT_BUBBLE + 3: \
                case ID_RECENT_BUBBLE + 4: case ID_RECENT_BUBBLE + 5: case ID_RECENT_BUBBLE + 6: case ID_RECENT_BUBBLE + 7: \
                    if(emu) { \
                        open_recent_bubble_casette(drv, LOWORD(wParam) - ID_RECENT_BUBBLE); \
                    } \
                    break;
                BUBBLE_CASETTE_MENU_ITEMS(0, ID_OPEN_BUBBLE1, ID_CLOSE_BUBBLE1, ID_RECENT_BUBBLE1)
#endif
#if USE_BUBBLE >= 2
                BUBBLE_CASETTE_MENU_ITEMS(1, ID_OPEN_BUBBLE2, ID_CLOSE_BUBBLE2, ID_RECENT_BUBBLE2)
#endif
#endif
                case ID_ACCEL_SCREEN:
                    if(emu) {
                        emu->suspend();
                        //set_window(hWnd, now_fullscreen ? prev_window_mode : -1);
                    }
                    break;
#ifdef USE_MOUSE
                case ID_ACCEL_MOUSE:
                    if(emu) {
                        emu->toggle_mouse();
                    }
                    break;
#endif
                case ID_ACCEL_SPEED:
                    config.full_speed = !config.full_speed;
                    break;
#ifdef USE_AUTO_KEY
                case ID_ACCEL_ROMAJI:
                    if(emu) {
                        if(!config.romaji_to_kana) {
                            emu->set_auto_key_char(1); // start
                            config.romaji_to_kana = true;
                        } else {
                            emu->set_auto_key_char(0); // end
                            config.romaji_to_kana = false;
                        }
                    }
                    break;
#endif
#ifdef ONE_BOARD_MICRO_COMPUTER
                    case ID_BUTTON +  0: case ID_BUTTON +  1: case ID_BUTTON +  2: case ID_BUTTON +  3: case ID_BUTTON +  4:
                case ID_BUTTON +  5: case ID_BUTTON +  6: case ID_BUTTON +  7: case ID_BUTTON +  8: case ID_BUTTON +  9:
                case ID_BUTTON + 10: case ID_BUTTON + 11: case ID_BUTTON + 12: case ID_BUTTON + 13: case ID_BUTTON + 14:
                case ID_BUTTON + 15: case ID_BUTTON + 16: case ID_BUTTON + 17: case ID_BUTTON + 18: case ID_BUTTON + 19:
                case ID_BUTTON + 20: case ID_BUTTON + 21: case ID_BUTTON + 22: case ID_BUTTON + 23: case ID_BUTTON + 24:
                case ID_BUTTON + 25: case ID_BUTTON + 26: case ID_BUTTON + 27: case ID_BUTTON + 28: case ID_BUTTON + 29:
                case ID_BUTTON + 30: case ID_BUTTON + 31: case ID_BUTTON + 32: case ID_BUTTON + 33: case ID_BUTTON + 34:
                case ID_BUTTON + 35: case ID_BUTTON + 36: case ID_BUTTON + 37: case ID_BUTTON + 38: case ID_BUTTON + 39:
                case ID_BUTTON + 40: case ID_BUTTON + 41: case ID_BUTTON + 42: case ID_BUTTON + 43: case ID_BUTTON + 44:
                case ID_BUTTON + 45: case ID_BUTTON + 46: case ID_BUTTON + 47: case ID_BUTTON + 48: case ID_BUTTON + 49:
                case ID_BUTTON + 50: case ID_BUTTON + 51: case ID_BUTTON + 52: case ID_BUTTON + 53: case ID_BUTTON + 54:
                case ID_BUTTON + 55: case ID_BUTTON + 56: case ID_BUTTON + 57: case ID_BUTTON + 58: case ID_BUTTON + 59:
                case ID_BUTTON + 60: case ID_BUTTON + 61: case ID_BUTTON + 62: case ID_BUTTON + 63: case ID_BUTTON + 64:
                case ID_BUTTON + 65: case ID_BUTTON + 66: case ID_BUTTON + 67: case ID_BUTTON + 68: case ID_BUTTON + 69:
                case ID_BUTTON + 70: case ID_BUTTON + 71: case ID_BUTTON + 72: case ID_BUTTON + 73: case ID_BUTTON + 74:
                case ID_BUTTON + 75: case ID_BUTTON + 76: case ID_BUTTON + 77: case ID_BUTTON + 78: case ID_BUTTON + 79:
                case ID_BUTTON + 80: case ID_BUTTON + 81: case ID_BUTTON + 82: case ID_BUTTON + 83: case ID_BUTTON + 84:
                case ID_BUTTON + 85: case ID_BUTTON + 86: case ID_BUTTON + 87: case ID_BUTTON + 88: case ID_BUTTON + 89:
                case ID_BUTTON + 90: case ID_BUTTON + 91: case ID_BUTTON + 92: case ID_BUTTON + 93: case ID_BUTTON + 94:
                case ID_BUTTON + 95: case ID_BUTTON + 96: case ID_BUTTON + 97: case ID_BUTTON + 98: case ID_BUTTON + 99:
                    if(emu) {
                        emu->press_button(LOWORD(wParam) - ID_BUTTON);
                    }
                    break;
#endif
            }
            break;
    }
    //return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// menu
// ----------------------------------------------------------------------------

void switchPCG() {
#if defined(_MZ80K) || defined(_MZ1200) || defined(_MZ700)
    config.dipswitch = config.dipswitch ^ 1;
#endif
#if defined(SUPPORT_PC88_PCG8100)
    config.dipswitch = config.dipswitch ^ (1 << 3);
#endif
}

void switchSound() {
    emu->get_osd()->reset_sound();
#if defined(__ANDROID__)
    emu->get_osd()->soundEnable = !(emu->get_osd()->soundEnable);
    config.sound_on = emu->get_osd()->soundEnable;
#else
    emu->get_osd()->soundEnable = !(emu->get_osd()->soundEnable);
#endif
}

bool get_status_bar_updated() {
    bool updated = false;

#ifdef USE_FLOPPY_DISK
    uint32_t new_fd_status = emu->is_floppy_disk_accessed();
    if (fd_status != new_fd_status) {
        updated = true;
        fd_status = new_fd_status;
    }
#endif
#ifdef USE_QUICK_DISK
    uint32_t new_qd_status = emu->is_quick_disk_accessed();
    if (qd_status != new_qd_status) {
        updated = true;
        qd_status = new_qd_status;
    }
#endif
#ifdef USE_HARD_DISK
    uint32_t new_hd_status = emu->is_hard_disk_accessed();
    if(hd_status != new_hd_status) {
        updated = true;
        hd_status = new_hd_status;
    }
#endif
#ifdef USE_COMPACT_DISC
    uint32_t new_cd_status = emu->is_compact_disc_accessed();
    if(cd_status != new_cd_status) {
        updated = true;
        cd_status = new_cd_status;
    }
#endif
#ifdef USE_LASER_DISC
    uint32_t new_ld_status = emu->is_laser_disc_accessed();
    if(ld_status != new_ld_status) {
        updated = true;
        ld_status = new_ld_status;
    }
#endif
#if defined(USE_TAPE) && !defined(TAPE_BINARY_ONLY)
    _TCHAR new_tape_status[array_length(tape_status)] = {0};
    for (int drv = 0; drv < USE_TAPE; drv++) {
        tape_position = emu->get_tape_position(drv);
        const _TCHAR *message = emu->get_tape_message(drv);
        if (message != NULL) {
            my_tcscpy_s(new_tape_status, array_length(new_tape_status), message);
            break;
        }
    }
    if (_tcsicmp(tape_status, new_tape_status) != 0) {
        updated = true;
        my_tcscpy_s(tape_status, array_length(tape_status), new_tape_status);
    }
#endif
    return updated;
}

// ----------------------------------------------------------------------------
// file
// ----------------------------------------------------------------------------

#define UPDATE_HISTORY(path, recent) { \
	int index = MAX_HISTORY - 1; \
	for(int i = 0; i < MAX_HISTORY; i++) { \
		if(_tcsicmp(recent[i], path) == 0) { \
			index = i; \
			break; \
		} \
	} \
	for(int i = index; i > 0; i--) { \
		my_tcscpy_s(recent[i], _MAX_PATH, recent[i - 1]); \
	} \
	my_tcscpy_s(recent[0], _MAX_PATH, path); \
}

void selectMedia(struct android_app *state, int iconIndex) {
    selectingIconIndex = iconIndex;

    if (fileSelectIconData[selectingIconIndex].fileSelectType < 0) {
        return;
    }
    switch (fileSelectIconData[selectingIconIndex].fileSelectType) {
#ifdef USE_FLOPPY_DISK
        case FLOPPY_DISK:
            selectFloppyDisk(state, fileSelectIconData[selectingIconIndex].driveNo);
            break;
#endif
#ifdef USE_HARD_DISK
        case HARD_DISK:
            selectHardDisk(state, fileSelectIconData[selectingIconIndex].driveNo);
            break;
#endif
#ifdef USE_TAPE
        case CASETTE_TAPE:
            selectTape(state);
            break;
#endif
#ifdef USE_CART
        case CARTRIDGE:
            selectCart(state, fileSelectIconData[selectingIconIndex].driveNo);
            break;
#endif
#ifdef USE_QUICK_DISK
        case QUICK_DISK:
            selectQuickDisk(state, fileSelectIconData[selectingIconIndex].driveNo);
            break;
#endif
    }

    return;
}

void selectCart(struct android_app *state, int driveNo) {
    char message[32];
    sprintf(message, "Select CARTRIDGE[%d]", driveNo);
    selectDialog(state, message, "CART");
}

void selectFloppyDisk(struct android_app *state, int diskNo) {
    char message[32];
    if (emu->d88_file[diskNo].bank_num > 0 && emu->d88_file[diskNo].cur_bank != -1) {
        sprintf(message, "DISK[%d] %s", diskNo, emu->d88_file[diskNo].disk_name[emu->d88_file[diskNo].cur_bank]);
    } else {
        sprintf(message, "Select DISK[%d]", diskNo);
    }
    selectDialog(state, message, "DISK");
}

void selectHardDisk(struct android_app *state, int driveNo) {
    char message[32];
    sprintf(message, "Select HDD[%d]", driveNo);
    selectDialog(state, message, "HDD");
}

void selectQuickDisk(struct android_app *state, int driveNo) {
    char message[32];
    sprintf(message, "Select QD[%d]", driveNo);
    selectDialog(state, message, "QD");
}

void selectTape(struct android_app *state) {
    char message[32];
    sprintf(message, "Select TAPE and Play");
    selectDialog(state, message, "TAPE");
}

static void all_eject() {
    // 各種メディアの排出処理

    for (int index = 0; index < MAX_FILE_SELECT_ICON; index++) {
        switch (fileSelectIconData[index].fileSelectType) {
#ifdef USE_FLOPPY_DISK
            case FLOPPY_DISK:
                emu->close_floppy_disk(fileSelectIconData[index].driveNo);
                LOGI("Eject Floppy Disk %d", fileSelectIconData[index].driveNo);
                break;
#endif
#ifdef USE_HARD_DISK
        case HARD_DISK:
                emu->close_hard_disk(fileSelectIconData[index].driveNo);
                LOGI("Eject Hard Disk %d", fileSelectIconData[index].driveNo);
                break;
#endif
#ifdef USE_QUICK_DISK
                case QUICK_DISK:
                emu->close_quick_disk(fileSelectIconData[index].driveNo);
                LOGI("Eject Quick Disk %d", fileSelectIconData[index].driveNo);
                break;
#endif
#ifdef USE_TAPE
            case CASETTE_TAPE:
                emu->close_tape(fileSelectIconData[index].driveNo);
                LOGI("Eject Tape %d", fileSelectIconData[index].driveNo);
                break;
#endif
#ifdef USE_CART
                case CARTRIDGE:
                emu->close_cart(fileSelectIconData[selectingIconIndex].driveNo);
                LOGI("Eject Cartridge %d", fileSelectIconData[selectingIconIndex].driveNo);
                break;
#endif
        }
    }
}

#ifdef USE_CART
void open_cart_dialog(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
#if defined(_COLECOVISION)
		_T("Supported Files (*.rom;*.bin;*.hex;*.col)\0*.rom;*.bin;*.hex;*.col\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#elif defined(_GAMEGEAR)
		_T("Supported Files (*.rom;*.bin;*.hex;*.gg;*.col)\0*.rom;*.bin;*.hex;*.gg;*.col\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#elif defined(_MASTERSYSTEM)
		_T("Supported Files (*.rom;*.bin;*.hex;*.sms)\0*.rom;*.bin;*.hex;*.sms\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#elif defined(_PC6001) || defined(_PC6001MK2) || defined(_PC6001MK2SR) || defined(_PC6601) || defined(_PC6601SR)
		_T("Supported Files (*.rom;*.bin;*.hex;*.60)\0*.rom;*.bin;*.hex;*.60\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#elif defined(_PCENGINE) || defined(_X1TWIN)
		_T("Supported Files (*.rom;*.bin;*.hex;*.pce)\0*.rom;*.bin;*.hex;*.pce\0All Files (*.*)\0*.*\0\0"),
		_T("HuCARD"),
#elif defined(_SC3000)
		_T("Supported Files (*.rom;*.bin;*.hex;*.sc;*.sg)\0*.rom;*.bin;*.hex;*.sc;*.sg\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#else
		_T("Supported Files (*.rom;*.bin;*.hex)\0*.rom;*.bin;*.hex\0All Files (*.*)\0*.*\0\0"),
		_T("Game Cartridge"),
#endif
		NULL,
		config.initial_cart_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_cart_path[drv]);
		my_tcscpy_s(config.initial_cart_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_cart(drv, path);
	}
}

void open_recent_cart(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_cart_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_cart_path[drv][i], _MAX_PATH, config.recent_cart_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_cart_path[drv][0], _MAX_PATH, path);
	emu->open_cart(drv, path);
}
#endif

#ifdef USE_FLOPPY_DISK
#if defined(_WIN32)
void open_floppy_disk_dialog(HWND hWnd, int drv)
{
    _TCHAR* path = get_open_file_name(
            hWnd,
            _T("Supported Files (*.d88;*.d8e;*.d77;*.1dd;*.td0;*.imd;*.dsk;*.nfd;*.fdi;*.hdm;*.hd5;*.hd4;*.hdb;*.dd9;*.dd6;*.tfd;*.xdf;*.2d;*.sf7;*.img;*.ima;*.vfd)\0*.d88;*.d8e;*.d77;*.1dd;*.td0;*.imd;*.dsk;*.nfd;*.fdi;*.hdm;*.hd5;*.hd4;*.hdb;*.dd9;*.dd6;*.tfd;*.xdf;*.2d;*.sf7;*.img;*.ima;*.vfd\0All Files (*.*)\0*.*\0\0"),
            _T("Floppy Disk"),
            NULL,
            config.initial_floppy_disk_dir, _MAX_PATH
    );
    if(path) {
        UPDATE_HISTORY(path, config.recent_floppy_disk_path[drv]);
        my_tcscpy_s(config.initial_floppy_disk_dir, _MAX_PATH, get_parent_dir(path));
        emu->open_floppy_disk(drv, path, 0);
#if USE_FLOPPY_DISK >= 2
        if((drv & 1) == 0 && drv + 1 < USE_FLOPPY_DISK && emu->d88_file[drv].bank_num > 1) {
            emu->open_floppy_disk(drv + 1, path, 1);
        }
#endif
    }
}
#else
void open_floppy_disk_dialog(struct android_app *app, int drive) {
    // ドライブ番号（ここではアイコンインデックスにもなる）のタイプがフロッピでなければ何もしない
    if (fileSelectIconData[selectingIconIndex].fileSelectType != FLOPPY_DISK) {
        return;
    }
    selectMedia(app, drive);
}
#endif

#if defined(_WIN32)
void open_blank_floppy_disk_dialog(HWND hWnd, int drv, uint8_t type)
{
    _TCHAR* path = get_open_file_name(
            hWnd,
            _T("Supported Files (*.d88;*.d77)\0*.d88;*.d77\0All Files (*.*)\0*.*\0\0"),
            _T("Floppy Disk"),
            create_date_file_name(_T("d88")),
            config.initial_floppy_disk_dir, _MAX_PATH
    );
    if(path) {
        if(!check_file_extension(path, _T(".d88")) && !check_file_extension(path, _T(".d77"))) {
            my_tcscat_s(path, _MAX_PATH, _T(".d88"));
        }
        if(emu->create_blank_floppy_disk(path, type)) {
            UPDATE_HISTORY(path, config.recent_floppy_disk_path[drv]);
            my_tcscpy_s(config.initial_floppy_disk_dir, _MAX_PATH, get_parent_dir(path));
            emu->open_floppy_disk(drv, path, 0);
        }
    }
}
#else
void open_blank_floppy_disk_dialog(struct android_app * app, int drv, uint8_t type)
{
    MediaInfo mediaInfo = {drv, FLOPPY_DISK, type};
    createBlankDisk(app, &mediaInfo);
}
#endif

void open_recent_floppy_disk(int drv, int index)
{
    _TCHAR path[_MAX_PATH];
    my_tcscpy_s(path, _MAX_PATH, config.recent_floppy_disk_path[drv][index]);
    for(int i = index; i > 0; i--) {
        my_tcscpy_s(config.recent_floppy_disk_path[drv][i], _MAX_PATH, config.recent_floppy_disk_path[drv][i - 1]);
    }
    my_tcscpy_s(config.recent_floppy_disk_path[drv][0], _MAX_PATH, path);
    emu->open_floppy_disk(drv, path, 0);
#if USE_FLOPPY_DISK >= 2
    if((drv & 1) == 0 && drv + 1 < USE_FLOPPY_DISK && emu->d88_file[drv].bank_num > 1) {
        emu->open_floppy_disk(drv + 1, path, 1);
    }
#endif
}

void select_d88_bank(int drv, int index)
{
    if(emu->d88_file[drv].cur_bank != index) {
        emu->open_floppy_disk(drv, emu->d88_file[drv].path, index);
    }
}
#endif

#ifdef USE_QUICK_DISK
#if defined(_WIN32)
void open_quick_disk_dialog(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.mzt;*.q20;*.qdf)\0*.mzt;*.q20;*.qdf\0All Files (*.*)\0*.*\0\0"),
		_T("Quick Disk"),
		NULL,
		config.initial_quick_disk_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_quick_disk_path[drv]);
		my_tcscpy_s(config.initial_quick_disk_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_quick_disk(drv, path);
	}
}
#else
void open_quick_disk_dialog(struct android_app *app, int drive) {
    // ドライブ番号 drive は 0 オリジンになっているが、アイコンインデックスは .fileSelectType が QUICK_DISK になっているため、
    // QUICK_DISK が設定されているインデックスをオフセットとして加算する
    // オフセットの設定方法は、fileSelectIconData[?].fileSelectType を上から順に走査し、最初に QUICK_DISK が設定されているインデックスを探す
    int offset = 0;
    for (int i = 0; i < MAX_FILE_SELECT_ICON; i++) {
        if (fileSelectIconData[i].fileSelectType == QUICK_DISK) {
            offset = i;
            break;
        }
    }
    // ドライブ番号（ここではアイコンインデックスにもなる）のタイプがクイックディスクば何もしない
    if (fileSelectIconData[drive + offset].fileSelectType != QUICK_DISK) {
        return;
    }
    selectMedia(app, drive + offset);
}
#endif

void open_recent_quick_disk(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_quick_disk_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_quick_disk_path[drv][i], _MAX_PATH, config.recent_quick_disk_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_quick_disk_path[drv][0], _MAX_PATH, path);
	emu->open_quick_disk(drv, path);
}
#endif

#ifdef USE_HARD_DISK
#if defined(_WIN32)
void open_hard_disk_dialog(HWND hWnd, int drv)
{
    _TCHAR* path = get_open_file_name(
            hWnd,
            _T("Supported Files (*.thd;*.nhd;*.hdi;*.hdd;*.dat)\0*.thd;*.nhd;*.hdi;*.hdd;*.dat\0All Files (*.*)\0*.*\0\0"),
            _T("Hard Disk"),
            NULL,
            config.initial_hard_disk_dir, _MAX_PATH
    );
    if(path) {
        UPDATE_HISTORY(path, config.recent_hard_disk_path[drv]);
        my_tcscpy_s(config.initial_hard_disk_dir, _MAX_PATH, get_parent_dir(path));
        emu->open_hard_disk(drv, path);
    }
}
#else
void open_hard_disk_dialog(struct android_app *app, int drive) {
    // ドライブ番号 drive は 0 オリジンになっているが、アイコンインデックスは .fileSelectType が QUICK_DISK になっているため、
    // QUICK_DISK が設定されているインデックスをオフセットとして加算する
    // オフセットの設定方法は、fileSelectIconData[?].fileSelectType を上から順に走査し、最初に QUICK_DISK が設定されているインデックスを探す
    int offset = 0;
    for (int i = 0; i < MAX_FILE_SELECT_ICON; i++) {
        if (fileSelectIconData[i].fileSelectType == HARD_DISK) {
            offset = i;
            break;
        }
    }
    // ドライブ番号（ここではアイコンインデックスにもなる）のタイプがクイックディスクば何もしない
    if (fileSelectIconData[drive + offset].fileSelectType != HARD_DISK) {
        return;
    }
    selectMedia(app, drive + offset);
}
#endif

void open_recent_hard_disk(int drv, int index)
{
    _TCHAR path[_MAX_PATH];
    my_tcscpy_s(path, _MAX_PATH, config.recent_hard_disk_path[drv][index]);
    for(int i = index; i > 0; i--) {
        my_tcscpy_s(config.recent_hard_disk_path[drv][i], _MAX_PATH, config.recent_hard_disk_path[drv][i - 1]);
    }
    my_tcscpy_s(config.recent_hard_disk_path[drv][0], _MAX_PATH, path);
    emu->open_hard_disk(drv, path);
}

#if defined(_WIN32)
void open_blank_hard_disk_dialog(HWND hWnd, int drv, int sector_size, int sectors, int surfaces, int cylinders)
{
    _TCHAR* path = get_open_file_name(
            hWnd,
            _T("Supported Files (*.hdi;*.nhd)\0*.hdi;*.nhd\0All Files (*.*)\0*.*\0\0"),
            _T("Hard Disk"),
            create_date_file_name(_T("hdi")),
            config.initial_hard_disk_dir, _MAX_PATH
    );
    if(path) {
        if(!check_file_extension(path, _T(".hdi")) && !check_file_extension(path, _T(".nhd"))) {
            my_tcscat_s(path, _MAX_PATH, _T(".hdi"));
        }
        if(emu->create_blank_hard_disk(path, sector_size, sectors, surfaces, cylinders)) {
            UPDATE_HISTORY(path, config.recent_hard_disk_path[drv]);
            my_tcscpy_s(config.initial_hard_disk_dir, _MAX_PATH, get_parent_dir(path));
            emu->open_hard_disk(drv, path);
        }
    }
}
#else
void open_blank_hard_disk_dialog(struct android_app * app, int drv, int sector_size, int sectors, int surfaces, int cylinders)
{
    MediaInfo mediaInfo = {drv, HARD_DISK, 0, sector_size, sectors, surfaces, cylinders};
    createBlankDisk(app, &mediaInfo);
}
#endif
#endif

#ifdef USE_TAPE
#if defined(_WIN32)
void open_tape_dialog(HWND hWnd, int drv, bool play)
{
    _TCHAR* path = get_open_file_name(
            hWnd,
#if defined(_PC6001) || defined(_PC6001MK2) || defined(_PC6001MK2SR) || defined(_PC6601) || defined(_PC6601SR)
            play ? _T("Supported Files (*.wav;*.cas;*.p6;*.p6t;*.gz)\0*.wav;*.cas;*.p6;*.p6t;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas;*.p6;*.p6t)\0*.wav;*.cas;*.p6;*.p6t\0All Files (*.*)\0*.*\0\0"),
#elif defined(_PC8001) || defined(_PC8001MK2) || defined(_PC8001SR) || defined(_PC8801) || defined(_PC8801MK2) || defined(_PC8801MA) || defined(_PC98DO)
            play ? _T("Supported Files (*.cas;*.cmt;*.n80;*.t88)\0*.cas;*.cmt;*.n80;*.t88\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.cas;*.cmt)\0*.cas;*.cmt\0All Files (*.*)\0*.*\0\0"),
#elif defined(_MZ80A) || defined(_MZ80K) || defined(_MZ1200) || defined(_MZ700) || defined(_MZ800) || defined(_MZ1500)
            play ? _T("Supported Files (*.wav;*.cas;*.mzt;*.mzf;*.m12;*.gz)\0*.wav;*.cas;*.mzt;*.mzf;*.m12;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(_MZ80B) || defined(_MZ2000) || defined(_MZ2200)
            play ? _T("Supported Files (*.wav;*.cas;*.mzt;*.mzf;*.mti;*.mtw;*.dat;*.gz)\0*.wav;*.cas;*.mzt;*.mzf;*.mti;*.mtw;*.dat;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(_MZ2500)
            play ? _T("Supported Files (*.wav;*.cas;*.mzt;*.mzf;*.mti;*.gz)\0*.wav;*.cas;*.mzt;*.mzf;*.mti;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(_X1) || defined(_X1TWIN) || defined(_X1TURBO) || defined(_X1TURBOZ)
            play ? _T("Supported Files (*.wav;*.cas;*.tap;*.gz)\0*.wav;*.cas;*.tap;*.gz\0All Files (*.*)\0*.*\0\0")
                 : _T("Supported Files (*.wav;*.cas;*.tap)\0*.wav;*.cas;*.tap\0All Files (*.*)\0*.*\0\0"),
#elif defined(_FM8) || defined(_FM7) || defined(_FMNEW7) || defined(_FM77_VARIANTS) || defined(_FM77AV_VARIANTS)
            play ? _T("Supported Files (*.wav;*.cas;*.t77;*.gz)\0*.wav;*.cas;*.t77;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas;*.t77)\0*.wav;*.cas;*.t77\0All Files (*.*)\0*.*\0\0"),
#elif defined(_BMJR)
		play ? _T("Supported Files (*.wav;*.cas;*.bin;*.gz)\0*.wav;*.cas;*.bin;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(_TK80BS)
		drv == 1 ? _T("Supported Files (*.cas;*.cmt)\0*.cas;*.cmt\0All Files (*.*)\0*.*\0\0")
		: play   ? _T("Supported Files (*.wav;*.cas;*.gz)\0*.wav;*.cas;*.gz\0All Files (*.*)\0*.*\0\0")
		         : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif !defined(TAPE_BINARY_ONLY)
		play ? _T("Supported Files (*.wav;*.cas;*.gz)\0*.wav;*.cas;*.gz\0All Files (*.*)\0*.*\0\0")
		     : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#else
		_T("Supported Files (*.cas;*.cmt)\0*.cas;*.cmt\0All Files (*.*)\0*.*\0\0"),
#endif
            play ? _T("Data Recorder Tape [Play]") : _T("Data Recorder Tape [Rec]"),
            NULL,
            config.initial_tape_dir, _MAX_PATH
    );
    if(path) {
        UPDATE_HISTORY(path, config.recent_tape_path[drv]);
        my_tcscpy_s(config.initial_tape_dir, _MAX_PATH, get_parent_dir(path));
        if(play) {
            emu->play_tape(drv, path);
        } else {
            emu->rec_tape(drv, path);
        }
    }
}
#else
void open_tape_dialog(struct android_app *app, int drive, bool play) {
    // ドライブ番号 drive は 0 オリジンになっているが、アイコンインデックスは .fileSelectType が CASETTE_TAPE になっているため、
    // CASETTE_TAPE が設定されているインデックスをオフセットとして加算する
    // オフセットの設定方法は、fileSelectIconData[?].fileSelectType を上から順に走査し、最初に CASETTE_TAPE が設定されているインデックスを探す
    int offset = 0;
    for (int i = 0; i < MAX_FILE_SELECT_ICON; i++) {
        if (fileSelectIconData[i].fileSelectType == CASETTE_TAPE) {
            offset = i;
            break;
        }
    }
    // ドライブ番号（ここではアイコンインデックスにもなる）のタイプがカセットテープでなければ何もしない
    if (fileSelectIconData[drive + offset].fileSelectType != CASETTE_TAPE) {
        return;
    }
    extendMenuCmtPlay = play;

    if (play) {
        selectMedia(app, drive + offset);
    } else {
        MediaInfo mediaInfo = {drive, CASETTE_TAPE};
        createBlankDisk(app, &mediaInfo);
    }
}
#endif

void open_recent_tape(int drv, int index)
{
    _TCHAR path[_MAX_PATH];
    my_tcscpy_s(path, _MAX_PATH, config.recent_tape_path[drv][index]);
    for(int i = index; i > 0; i--) {
        my_tcscpy_s(config.recent_tape_path[drv][i], _MAX_PATH, config.recent_tape_path[drv][i - 1]);
    }
    my_tcscpy_s(config.recent_tape_path[drv][0], _MAX_PATH, path);
    emu->play_tape(drv, path);
}
#endif

#ifdef USE_COMPACT_DISC
void open_compact_disc_dialog(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.ccd;*.cue)\0*.ccd;*.cue\0All Files (*.*)\0*.*\0\0"),
		_T("Compact Disc"),
		NULL,
		config.initial_compact_disc_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_compact_disc_path[drv]);
		my_tcscpy_s(config.initial_compact_disc_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_compact_disc(drv, path);
	}
}

void open_recent_compact_disc(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_compact_disc_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_compact_disc_path[drv][i], _MAX_PATH, config.recent_compact_disc_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_compact_disc_path[drv][0], _MAX_PATH, path);
	emu->open_compact_disc(drv, path);
}
#endif

#ifdef USE_LASER_DISC
void open_laser_disc_dialog(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.avi;*.mpg;*.mpeg;*.mp4;*.wmv;*.ogv)\0*.avi;*.mpg;*.mpeg;*.mp4;*.wmv;*.ogv\0All Files (*.*)\0*.*\0\0"),
		_T("Laser Disc"),
		NULL,
		config.initial_laser_disc_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_laser_disc_path[drv]);
		my_tcscpy_s(config.initial_laser_disc_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_laser_disc(drv, path);
	}
}

void open_recent_laser_disc(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_laser_disc_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_laser_disc_path[drv][i], _MAX_PATH, config.recent_laser_disc_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_laser_disc_path[drv][0], _MAX_PATH, path);
	emu->open_laser_disc(drv, path);
}
#endif

#ifdef USE_BINARY_FILE
void open_binary_dialog(HWND hWnd, int drv, bool load)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.ram;*.bin;*.hex)\0*.ram;*.bin;*.hex\0All Files (*.*)\0*.*\0\0"),
#if defined(_PASOPIA) || defined(_PASOPIA7)
		_T("RAM Pack Cartridge"),
#else
		_T("Memory Dump"),
#endif
		NULL,
		config.initial_binary_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_binary_path[drv]);
		my_tcscpy_s(config.initial_binary_dir, _MAX_PATH, get_parent_dir(path));
		if(load) {
			emu->load_binary(drv, path);
		} else {
			emu->save_binary(drv, path);
		}
	}
}

void open_recent_binary(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_binary_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_binary_path[drv][i], _MAX_PATH, config.recent_binary_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_binary_path[drv][0], _MAX_PATH, path);
	emu->load_binary(drv, path);
}
#endif

#ifdef USE_BUBBLE
void open_bubble_casette_dialog(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.b77;*.bbl)\0*.b77;*.bbl\0All Files (*.*)\0*.*\0\0"),
		_T("Bubble Casette"),
		NULL,
		config.initial_bubble_casette_dir, _MAX_PATH
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_bubble_casette_path[drv]);
		my_tcscpy_s(config.initial_bubble_casette_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_bubble_casette(drv, path, 0);
	}
}

void open_recent_bubble_casette(int drv, int index)
{
	_TCHAR path[_MAX_PATH];
	my_tcscpy_s(path, _MAX_PATH, config.recent_bubble_casette_path[drv][index]);
	for(int i = index; i > 0; i--) {
		my_tcscpy_s(config.recent_bubble_casette_path[drv][i], _MAX_PATH, config.recent_bubble_casette_path[drv][i - 1]);
	}
	my_tcscpy_s(config.recent_bubble_casette_path[drv][0], _MAX_PATH, path);
	emu->open_bubble_casette(drv, path, 0);
}
#endif

#ifdef SUPPORT_DRAG_DROP
#if defined(_WIN32)
void open_dropped_file(HDROP hDrop)
{
    if(DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0) == 1) {
        _TCHAR path[_MAX_PATH];
        DragQueryFile(hDrop, 0, path, _MAX_PATH);
        open_any_file(path);
    }
    DragFinish(hDrop);
}

void open_any_file(const _TCHAR* path)
{
#if defined(USE_CART)
    if(check_file_extension(path, _T(".rom")) ||
	   check_file_extension(path, _T(".bin")) ||
	   check_file_extension(path, _T(".hex")) ||
	   check_file_extension(path, _T(".sc" )) ||
	   check_file_extension(path, _T(".sg" )) ||
	   check_file_extension(path, _T(".gg" )) ||
	   check_file_extension(path, _T(".col")) ||
	   check_file_extension(path, _T(".sms")) ||
	   check_file_extension(path, _T(".60" )) ||
	   check_file_extension(path, _T(".pce"))) {
		UPDATE_HISTORY(path, config.recent_cart_path[0]);
		my_tcscpy_s(config.initial_cart_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_cart(0, path);
		return;
	}
#endif
#if defined(USE_FLOPPY_DISK)
    if(check_file_extension(path, _T(".d88")) ||
       check_file_extension(path, _T(".d8e")) ||
       check_file_extension(path, _T(".d77")) ||
       check_file_extension(path, _T(".1dd")) ||
       check_file_extension(path, _T(".td0")) ||
       check_file_extension(path, _T(".imd")) ||
       check_file_extension(path, _T(".dsk")) ||
       check_file_extension(path, _T(".nfd")) ||
       check_file_extension(path, _T(".fdi")) ||
       check_file_extension(path, _T(".hdm")) ||
       check_file_extension(path, _T(".hd5")) ||
       check_file_extension(path, _T(".hd4")) ||
       check_file_extension(path, _T(".hdb")) ||
       check_file_extension(path, _T(".dd9")) ||
       check_file_extension(path, _T(".dd6")) ||
       check_file_extension(path, _T(".tfd")) ||
       check_file_extension(path, _T(".xdf")) ||
       check_file_extension(path, _T(".2d" )) ||
       check_file_extension(path, _T(".sf7")) ||
       check_file_extension(path, _T(".img")) ||
       check_file_extension(path, _T(".ima")) ||
       check_file_extension(path, _T(".vfd"))) {
        UPDATE_HISTORY(path, config.recent_floppy_disk_path[0]);
        my_tcscpy_s(config.initial_floppy_disk_dir, _MAX_PATH, get_parent_dir(path));
        emu->open_floppy_disk(0, path, 0);
#if USE_FLOPPY_DISK >= 2
        if(emu->d88_file[0].bank_num > 1) {
            emu->open_floppy_disk(1, path, 1);
        }
#endif
        return;
    }
#endif
#if defined(USE_HARD_DISK)
    if(check_file_extension(path, _T(".thd")) ||
       check_file_extension(path, _T(".nhd")) ||
       check_file_extension(path, _T(".hdi")) ||
       check_file_extension(path, _T(".hdd"))) {
        UPDATE_HISTORY(path, config.recent_hard_disk_path[0]);
        my_tcscpy_s(config.initial_hard_disk_dir, _MAX_PATH, get_parent_dir(path));
        emu->open_hard_disk(0, path);
        return;
    }
#endif
#if defined(USE_TAPE)
    if(check_file_extension(path, _T(".wav")) ||
       check_file_extension(path, _T(".cas")) ||
       check_file_extension(path, _T(".p6" )) ||
       check_file_extension(path, _T(".p6t")) ||
       check_file_extension(path, _T(".cmt")) ||
       check_file_extension(path, _T(".n80")) ||
       check_file_extension(path, _T(".t88")) ||
       check_file_extension(path, _T(".mzt")) ||
       check_file_extension(path, _T(".mzf")) ||
       check_file_extension(path, _T(".m12")) ||
       check_file_extension(path, _T(".mti")) ||
       check_file_extension(path, _T(".mtw")) ||
       check_file_extension(path, _T(".tap")) ||
       check_file_extension(path, _T(".t77"))) {
        UPDATE_HISTORY(path, config.recent_tape_path[0]);
        my_tcscpy_s(config.initial_tape_dir, _MAX_PATH, get_parent_dir(path));
        emu->play_tape(0, path);
        return;
    }
#endif
#if defined(USE_COMPACT_DISC)
    if(check_file_extension(path, _T(".ccd" )) ||
	   check_file_extension(path, _T(".cue" ))) {
		UPDATE_HISTORY(path, config.recent_compact_disc_path[0]);
		my_tcscpy_s(config.initial_compact_disc_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_compact_disc(0, path);
		return;
	}
#endif
#if defined(USE_LASER_DISC)
    if(check_file_extension(path, _T(".avi" )) ||
	   check_file_extension(path, _T(".mpg" )) ||
	   check_file_extension(path, _T(".mpeg")) ||
	   check_file_extension(path, _T(".mp4" )) ||
	   check_file_extension(path, _T(".wmv" )) ||
	   check_file_extension(path, _T(".ogv" ))) {
		UPDATE_HISTORY(path, config.recent_laser_disc_path[0]);
		my_tcscpy_s(config.initial_laser_disc_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_laser_disc(0, path);
		return;
	}
#endif
#if defined(USE_BINARY_FILE)
    if(check_file_extension(path, _T(".ram")) ||
	   check_file_extension(path, _T(".bin")) ||
	   check_file_extension(path, _T(".hex"))) {
		UPDATE_HISTORY(path, config.recent_binary_path[0]);
		my_tcscpy_s(config.initial_binary_dir, _MAX_PATH, get_parent_dir(path));
		emu->load_binary(0, path);
		return;
	}
#endif
#if defined(USE_BUBBLE)
    if(check_file_extension(path, _T(".b77")) ||
	   check_file_extension(path, _T(".bbl"))) {
		UPDATE_HISTORY(path, config.recent_bubble_casette_path[0]);
		my_tcscpy_s(config.initial_bubble_casette_dir, _MAX_PATH, get_parent_dir(path));
		emu->open_bubble_casette(0, path, 0);
		return;
	}
#endif
}
#endif
#endif

#if defined(_WIN32)
_TCHAR* get_open_file_name(HWND hWnd, const _TCHAR* filter, const _TCHAR* title, const _TCHAR* new_file, _TCHAR* dir, size_t dir_len)
{
    static _TCHAR path[_MAX_PATH];
    _TCHAR tmp[_MAX_PATH] = _T("");
    OPENFILENAME OpenFileName;

    if(new_file != NULL) {
        my_tcscpy_s(tmp, _MAX_PATH, new_file);
    }
    memset(&OpenFileName, 0, sizeof(OpenFileName));
    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hWnd;
    OpenFileName.lpstrFilter = filter;
    OpenFileName.lpstrFile = tmp;
    OpenFileName.nMaxFile = _MAX_PATH;
    OpenFileName.lpstrTitle = title;
    if(dir[0]) {
        OpenFileName.lpstrInitialDir = dir;
    } else {
        _TCHAR app[_MAX_PATH];
        GetModuleFileName(NULL, app, _MAX_PATH);
        OpenFileName.lpstrInitialDir = get_parent_dir(app);
    }
    if(GetOpenFileName(&OpenFileName)) {
        get_long_full_path_name(OpenFileName.lpstrFile, path, _MAX_PATH);
        my_tcscpy_s(dir, dir_len, get_parent_dir(path));
        return path;
    }
    return NULL;
}
#endif

// ----------------------------------------------------------------------------
// screen
// ----------------------------------------------------------------------------

static void draw_icon(ANativeWindow_Buffer *buffer) {
    int bufferWidth = buffer->width;
    int bufferHeight = buffer->height;
    int sideOffset = (bufferWidth > bufferHeight) ? 300 : 100; // 横長画面の時は左右オフセットを設定する
    bufferWidth -= sideOffset * 2; // 左右の余白を設定

    uint16_t *pixels = (uint16_t *) (buffer->bits) + sideOffset; // 左のオフセットを加算する

    int unitPixel = bufferWidth / 12;

    int offsetY = 100;
    int iconHeightMax = 0;

    for (int index = 0; index < MAX_FILE_SELECT_ICON; index++) {
        bool accessFlag = false;
        bool setFlag = false;
        int fileSelectType = fileSelectIconData[index].fileSelectType;
        if (fileSelectType >= 0) {
            //アクセス中は、緑色にして表示
#ifdef USE_FLOPPY_DISK
            if (fileSelectType == FLOPPY_DISK) {
                if (emu->is_floppy_disk_inserted(fileSelectIconData[index].driveNo) == true) {
                    setFlag = true;
                }
                int idx = (fd_status >> fileSelectIconData[index].driveNo) & 1;
                if (idx != 0) {
                    //アクセス中
                    accessFlag = true;
                }
            }
#endif
#ifdef USE_HARD_DISK
            if (fileSelectType == HARD_DISK) {
                if (emu->is_hard_disk_inserted(fileSelectIconData[index].driveNo) == true) {
                    setFlag = true;
                }
                int idx = (hd_status >> fileSelectIconData[index].driveNo) & 1;
                if (idx != 0) {
                    //アクセス中
                    accessFlag = true;
                }
            }
#endif
#ifdef USE_QUICK_DISK
            if (fileSelectType == QUICK_DISK) {
                if (emu->is_quick_disk_inserted(fileSelectIconData[index].driveNo) == true) {
                    setFlag = true;
                }
                if (emu->is_quick_disk_accessed() != 0) {
                    //アクセス中
                    accessFlag = true;
                }
            }
#endif
#ifdef USE_TAPE
            if (fileSelectType == CASETTE_TAPE) {
                if (emu->is_tape_inserted(0) == true) {
                    setFlag = true;
                }
                if (emu->is_tape_playing(0) == true) {
                    //アクセス中
                    accessFlag = true;
                }
            }
#endif
#ifdef USE_CART
            if(fileSelectType == CARTRIDGE){
                if(emu->is_cart_inserted(fileSelectIconData[index].driveNo)==true){
                    setFlag = true;
                }
            }
#endif
            if (iconHeightMax < mediaIconData[fileSelectType].height) {
                iconHeightMax = mediaIconData[fileSelectType].height;
            }
            uint16_t *iconPixels = pixels + buffer->stride * offsetY + (index) * unitPixel;


            for (int y = 0; y < mediaIconData[fileSelectType].height; y++) {

                uint16_t *line = (uint16_t *) iconPixels;
                for (int x = 0; x < mediaIconData[fileSelectType].width; x++) {
                    uint16_t dotData = mediaIconData[fileSelectType].bmpImage[x + y * mediaIconData[fileSelectType].width];
                    //データ縮小時に色が付いているので白黒で表示する。
                    if (dotData > 0) {
                        if (setFlag == true) {
                            dotData = 0xFFFF;
                        } else {
                            dotData = 0x8410;
                        }
                    } else {
                        dotData = 0x0000;
                    }
                    if (accessFlag == true) {
                        dotData = dotData & 0b0000011111100000; //緑色のみにする
                    }
                    line[x] = dotData;
                }
                iconPixels = iconPixels + buffer->stride;
            }
        }
    }
    //右端に CONFIG / PCG / SOUND / SCREEN / RESET
    for (int index = 0; index < SYSTEM_ICON_MAX; index++) {
        int dotMask = 0xFFFF;
#if defined(_EXTEND_MENU)
        if (index == SYSTEM_CONFIG) {
        }
#endif
        if (index == SYSTEM_PCG) {
#if defined(_MZ80K) || defined(_MZ1200) || defined(_MZ700)
            if((config.dipswitch & 1)==false){
                dotMask = 0x8410;
            }
#elif defined(SUPPORT_PC88_PCG8100)
            if((config.dipswitch & (1 << 3))==false){
                dotMask = 0x8410;
            }
#else
            continue;
#endif
        }
        if (index == SYSTEM_SOUND) {
            if (emu->get_osd()->soundEnable == false) {
                dotMask = 0x8410;
            }
        }
        if (index == SYSTEM_SCREEN) {
            continue;
        }

        uint16_t *iconPixels = pixels + buffer->stride * offsetY + (11 - index) * unitPixel;
        for (int y = 0; y < systemIconData[index].height; y++) {
            uint16_t *line = (uint16_t *) iconPixels;
            for (int x = 0; x < systemIconData[index].width; x++) {
                line[x] = (systemIconData[index].bmpImage[x + y * systemIconData[index].width]) &
                          dotMask;
            }
            iconPixels = iconPixels + buffer->stride;
        }
    }
    //テープ読み込み中の場合、パーセントに応じた表示する。
#if defined(USE_TAPE) && !defined(TAPE_BINARY_ONLY)
    int tapeY = iconHeightMax + offsetY + 10;
    uint16_t *line = pixels + buffer->stride * tapeY;
    int tapePercentPixel = bufferWidth * tape_position / 100;

    for (int x = 0; x < bufferWidth; x++) {
        if (x >= tapePercentPixel) {
            line[x] = 0;
            line[x + buffer->stride] = 0;
        } else {
            line[x] = 0x6761;
            line[x + buffer->stride] = 0x6761;
        }
    }
#endif
}

static void draw_message(struct android_app *state, ANativeWindow_Buffer *buffer, const char *message) {
    bitmap_t *screenBuffer = emu->get_osd()->getScreenBuffer();
    scrntype_t *lpBmp = screenBuffer->lpBmp;

    //画面のサイズ
    int bufferWidth = buffer->width;
    int bufferHeight = buffer->height;

    //エミュレータ側の画面のサイズ
    int width = emu->get_osd()->get_vm_window_width();
    int height = emu->get_osd()->get_vm_window_height();

    float widthRate = (float) bufferWidth / width;
    float heightRate = (float) bufferHeight / height;

    //縦横小さい側の倍率で拡大
    float screenRate = 0;
    if (widthRate < heightRate) {
        screenRate = widthRate;
    } else {
        screenRate = heightRate;
    }
    int realScreenWidth = (float) width * screenRate;
    int realScreenHeight = (float) height * screenRate;

    int messageY = realScreenHeight + 150 * screenRate;
    //messageY = 2000;
    uint16_t *pixels = (uint16_t *) (buffer->bits);
    pixels = pixels + buffer->stride * messageY;


    BitmapData messageBitmap = jniCreateBitmapFromString(state, message, 15 * screenRate);

    for (int y = 0; y < messageBitmap.height; y++) {
        uint16_t *line = (uint16_t *) pixels;
        for (int x = 0; x < messageBitmap.width; x++) {
            line[x] = messageBitmap.bmpImage[x + y * messageBitmap.width];
        }
        pixels = pixels + buffer->stride;
    }
    delete messageBitmap.bmpImage;
}

static void engine_draw_message(struct engine *engine, const char *message) {
    if (engine->app->window == NULL) {
        // No window.
        return;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
        LOGW("Unable to lock window buffer");
        return;
    }

    stats_startFrame(&engine->stats);
    draw_message(engine->app, &buffer, message);
    ANativeWindow_unlockAndPost(engine->app->window);
    stats_endFrame(&engine->stats);
}

static void load_emulator_screen(ANativeWindow_Buffer *buffer) {
    bitmap_t *screenBuffer = emu->get_osd()->getScreenBuffer();
    scrntype_t *lpBmp = screenBuffer->lpBmp;
    void *pixels = buffer->bits;
    int topOffset = 178;
    pixels = (uint16_t *) pixels + buffer->stride * topOffset;

    //画面のサイズ
    int bufferWidth = buffer->width;
    int bufferHeight = buffer->height - topOffset - config.screen_bottom_margin;
    //LOGI("D4 bufferWidth (%d, %d)", buffer->width, buffer->height);

    //エミュレータ側の画面のサイズ
    int width = emu->get_osd()->get_vm_window_width();
    int height = emu->get_osd()->get_vm_window_height();
    float aspect = (float) width / height;
    //LOGI("D5 width/height = aspect : (%d, %d) = (%f) ", width, height, aspect);

    float widthRate = (float) bufferWidth / width;
    float heightRate = (float) bufferHeight / height;
    //LOGI("D6 widthRate/heightRate = (%f, %f)", widthRate, heightRate);

    //縦横小さい側の倍率で拡大
    float screenRate = 0;
    if (widthRate < heightRate) {
        screenRate = widthRate;
        //LOGI("D7 screenRate = %f", screenRate);
    } else {
        screenRate = heightRate;
        //LOGI("D8 screenRate = %f", screenRate);
    }
    int realScreenWidth = (float) width * screenRate;
    int realScreenHeight = (float) height * screenRate;
    //LOGI("D9 realScreenWidth/Height (%d, %d)", realScreenWidth, realScreenHeight);

    int widthOffset = (bufferWidth - realScreenWidth) / 4;
    //int heightOffset = 50 * screenRate; // 計算がめんどくさくなるので縦オフセットは0にする
    int heightOffset = 0;
    //LOGI("D10 width/heightOffset = (%d, %d)", widthOffset, heightOffset);

    //pixels = (uint16_t *) pixels + buffer->stride * heightOffset + widthOffset;

    if (preScreenSize != screenSize) {
        void *tempPixels = pixels;
        for (int y = 0; y < realScreenHeight; y++) {
            uint16_t *line = (uint16_t *) tempPixels + widthOffset; // 横方向のオフセットを適用
            for (int x = 0; x < realScreenWidth; x++) {
                line[x] = 0;
            }
            tempPixels = (uint16_t *) tempPixels + buffer->stride;
        }
        preScreenSize = screenSize;
    }

    if (screenSize == SCREEN_SIZE_MAX) {
        for (int y = 0; y < realScreenHeight; y++) {
            int realY = y + topOffset; // すでに pixels に topOffset を加えているため、ここでは追加しない
            if (realY >= 0 && realY < buffer->height) {
                uint16_t *line = (uint16_t *) pixels + buffer->stride * y + widthOffset;  // ここで widthOffset を適用
                //LOGI("D11-2 y=%d / realY = %d", y, realY);
                for (int x = 0; x < realScreenWidth; x++) {
                    int realX = x + widthOffset;
                    if (realX >= 0 && realX < buffer->width) {
                        int emuX = x / screenRate;
                        int emuY = y / screenRate;

                        if (emuX >= 0 && emuX < width && emuY >= 0 && emuY < height) {
                            int emuOffset = (height - emuY - 1) * width + emuX;
                            if (emuOffset >= 0 && emuOffset < width * height) {
                                line[realX] = *(lpBmp + emuOffset);
                            } else {
                                line[realX] = 0; // エミュレータのバッファ範囲外
                            }
                        } else {
                            line[realX] = 0; // エミュレータのバッファ範囲外
                        }
                    }
                }
            }
        }
    } else if (screenSize == SCREEN_SIZE_SPECIAL) {
        //真ん中の描画画面
        int xOffset = (realScreenWidth - width) / 2;
        int yOffset = (realScreenHeight - height) / 2;

        int xScreenCount = xOffset / width + 1;
        int emuStartX = -(xOffset - width * xScreenCount);

        int yScreenCount = yOffset / height + 1;
        int emuStartY = -(yOffset - height * yScreenCount);

        int emuX = emuStartX;
        int emuY = emuStartY;

        for (int y = 0; y < realScreenHeight; y++) {
            uint16_t *line = (uint16_t *) pixels;
            for (int x = 0; x < realScreenWidth; x++) {
                line[x] =
                        *(lpBmp + (height - emuY - 1) * width + emuX - 1);
                emuX = emuX + 1;
                if (emuX > width) {
                    emuX = 0;
                }
            }
            emuX = emuStartX;
            emuY = emuY + 1;
            if (emuY > height) {
                emuY = 0;
            }
            pixels = (uint16_t *) pixels + buffer->stride;
        }
    } else {
        float scale = 1.0;
        //MAXより小さく、ドットバイドット等倍で入るギリギリサイズ
        int screenSize1ScaleWidth = realScreenWidth / width;
        int screenSize1ScaleHeight = realScreenHeight / height;
        int screenSize1Scale = 0;
        if (screenSize1ScaleWidth < screenSize1ScaleHeight) {
            screenSize1Scale = screenSize1ScaleWidth;
        } else {
            screenSize1Scale = screenSize1ScaleHeight;
        }
        float screenSize2Scale = screenSize1Scale;
        if (screenSize1Scale > 3) {
            screenSize2Scale = (float) screenSize1Scale / 2;
        } else {
            screenSize2Scale = 1.5;
        }
        switch (screenSize) {
            case SCREEN_SIZE_JUST:
                scale = screenSize1Scale;
                break;
            case SCREEN_SIZE_1:
                scale = screenSize2Scale;
                break;
            case SCREEN_SIZE_2:
                scale = 1.0;
                break;
        }

        int drawScreenWidth = (float) width * scale;
        int drawScreenHeight = (float) height * scale;
        int xOffset = (realScreenWidth - drawScreenWidth) / 2;
        int yOffset = (realScreenHeight - drawScreenHeight) / 2;

//        LOGI("width %d, height %d, screenSize %d: scale %f, drawScreenHeight %d, drawScreenWidth %d, xOffset %d, yOffset %d"
//               ,width, height, screenSize, scale, drawScreenHeight, drawScreenWidth, xOffset, yOffset);
        pixels = (uint16_t *) pixels + xOffset + yOffset * buffer->stride;
        for (int y = 0; y < drawScreenHeight; y++) {
            uint16_t *line = (uint16_t *) pixels;
            for (int x = 0; x < drawScreenWidth; x++) {
                int emuX = x / scale;
                int emuY = y / scale;
                if (0 < emuX && emuX < width && 0 < emuY && emuY < height) {
                    line[x] =
                            *(lpBmp + (height - emuY) * width + emuX - 1);
                } else {
                    line[x] = 0;
                }
            }
            pixels = (uint16_t *) pixels + buffer->stride;
        }
//        LOGI("draW END");
    }


    //if(*lpBmp !=0){
    //    char test[51] = {'\0'};
    //    for(int y = 100;y < 150;y++){
    //        for(int x = 0;x < 50;x++){
    //            if(G_OF_COLOR(*(lpBmp+y*width + x + 100))  > 0){
    //                test[x]='1';
    //            }else{
    //                test[x]='0';
    //            }
    //        }
    //        LOGI("%d: %s", y, test);
    //    }
    //}
}

static void engine_draw_frame(struct engine *engine) {
    if (engine->app->window == NULL) {
        // No window.
        return;
    }
    if (!engine->emu_initialized) {
        return;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
        LOGW("Unable to lock window buffer");
        return;
    }

    stats_startFrame(&engine->stats);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t time_ms = (((int64_t) now.tv_sec) * 1000000000LL + now.tv_nsec) / 1000000;
    time_ms -= start_ms;

    /* Now fill the values with a nice little plasma */
    if (engine->emu_initialized) {
        load_emulator_screen(&buffer);
    }
    draw_icon(&buffer);

    ANativeWindow_unlockAndPost(engine->app->window);

    stats_endFrame(&engine->stats);
}

static void engine_term_display(struct engine *engine) {
    engine->animating = 0;
}

static void clear_screen(struct engine *engine) {
    if (engine->app->window == NULL) {
        // No window.
        return;
    }

    LOGI("ClearScreen");
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
        LOGW("Unable to lock window buffer");
        return;
    }
    int bufferWidth = buffer.width;
    int bufferHeight = buffer.height;
    void *pixels = buffer.bits;
    memset(pixels, 0, bufferWidth * bufferHeight * sizeof(uint16_t));

    ANativeWindow_unlockAndPost(engine->app->window);

    //ダブルバッファになっているようなので２回繰り返しクリア（もっと良い方法があれば…）
    if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0) {
        LOGW("Unable to lock window buffer");
        return;
    }
    bufferWidth = buffer.width;
    bufferHeight = buffer.height;
    pixels = buffer.bits;
    memset(pixels, 0, bufferWidth * bufferHeight * sizeof(uint16_t));

    ANativeWindow_unlockAndPost(engine->app->window);

}

void check_update_screen(engine* engine) {
    if (engine == NULL || engine->app == NULL || engine->app->window == NULL) {
        return;
    }
    int newWidth = ANativeWindow_getWidth(engine->app->window);
    int newHeight = ANativeWindow_getHeight(engine->app->window);
    if (newWidth != deviceInfo.width || newHeight != deviceInfo.height) {
        LOGI("Changed: Screen(%d, %d) -> (%d, %d)", deviceInfo.width, deviceInfo.height, newWidth, newHeight);
        deviceInfo.width = newWidth;
        deviceInfo.height = newHeight;
    }
}

// ----------------------------------------------------------------------------
// input
// ----------------------------------------------------------------------------

static int32_t engine_handle_input(struct android_app *app, AInputEvent *event) {
    struct engine *engine = (struct engine *) app->userData;
    if (!engine->emu_initialized) return 0;
    int action = AKeyEvent_getAction(event);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_UP) {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);

            int sideOffset = (deviceInfo.width > deviceInfo.height) ? 300 : 100;
            x -= sideOffset; // 左側のオフセットを考慮したX座標の補正

            //アイコンチェック
            LOGI("X:%f, Y:%f, Witdh:%d, Height:%d", x, y, deviceInfo.width, deviceInfo.height);
            if (y > 100 && y < 300) {
                int unitPixel = (deviceInfo.width - sideOffset * 2) / 12;
                for (int index = 0; index < MAX_FILE_SELECT_ICON; index++) {
                    if (x > index * unitPixel && x < (index + 1) * unitPixel) {
                        if (fileSelectIconData[index].fileSelectType >= 0) {
                            LOGI("Tap File %d", index);
                            if (fileSelectIconData[index].fileSelectType == CASETTE_TAPE) {
                                extendMenuCmtPlay = true;
                            }
                            selectMedia(app, index);
                            return 1;
                        }
                        break;
                    }
                }
                // 右端のアイコンチェックについても、オフセットを考慮する
                int adjustedWidth = deviceInfo.width - sideOffset * 2;
                if (x > adjustedWidth - unitPixel) {
                    LOGI("Tap Reset");
                    selectBootMode(app);
                    return 1;
                } else if (x > adjustedWidth - unitPixel * 2) {
#if false
                    int newScreenSize = (int) screenSize + 1;
                    if (newScreenSize > SCREEN_SIZE_SPECIAL) {
                        screenSize = SCREEN_SIZE_JUST;
                    } else {
                        screenSize = (ScreenSize) newScreenSize;
                    }
                    clear_screen(engine);
#endif
                } else if (x > adjustedWidth - unitPixel * 3) {
                    LOGI("Tap switch Sound");
                    switchSound();
                    return 1;
                } else if (x > adjustedWidth - unitPixel * 4) {
                    LOGI("Tap switch PCG");
                    switchPCG();
                    return 1;
                } else if (x > adjustedWidth - unitPixel * 5) {
                    LOGI("Tap Config ******");
                    extendMenu(app);
                    return 1;
                }
            }
        }

        if (AMotionEvent_getAction(event) == AMOTION_EVENT_ACTION_DOWN) {
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            // 縦横の長さの関係からキーボード判定境界を決定する
            int keyboardHeight = (deviceInfo.width > deviceInfo.height)
                                 ? deviceInfo.height * 3 / 4 // 横長
                                 : deviceInfo.height / 2;    // 縦長
            int tapBorder = deviceInfo.height - keyboardHeight;

            // 実際の縦座標が、画面の下端からキーボードの高さを引いた位置よりも下ならば、タップ判定とする
            if (y > tapBorder) {
                LOGI("Tap toggle Keyboard (border = %d)", tapBorder);
                toggle_soft_keyboard(app);
                return 1;
            }
        }
        return 0;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        LOGI("Key event: action=%d keyCode=%d metaState=0x%x flags=%d source=%d",
             AKeyEvent_getAction(event),
             AKeyEvent_getKeyCode(event),
             AKeyEvent_getMetaState(event),
             AKeyEvent_getFlags(event),
             AInputEvent_getSource(event)
        );
        int action = AKeyEvent_getAction(event);
        int code = AKeyEvent_getKeyCode(event);

        int flags = AKeyEvent_getFlags(event);
        int source = AInputEvent_getSource(event);

        if (action == AKEY_EVENT_ACTION_DOWN) {
            if (AKEYCODE_BACK == AKeyEvent_getKeyCode(event)) {
                char message[128];
                sprintf(message, "Exit Emulator? [%s/%s]",__DATE__,__TIME__);
                showAlert(app, message, "", true, EXIT_EMULATOR);
                return 1;
            }

            if ((flags & AKEY_EVENT_FLAG_FROM_SYSTEM) == 0) { //ソフトウェアキーボード
                LOGI("SoftwareKeyboard:count:%d", softKeyboardCount);
                if (softKeyboardCount > 0) {
                    return 1;
                } else {
                    //SHIFTキー
                    if (code == 59) {
                        softKeyShift = true;
                    } else if (code == 113) {
                        softKeyCtrl = true;
                    } else {
                        //ここでは shift / ctrl 押下のみ行い、キー入力自体はメインループ内で行っています。
                        //同時処理だと、 shift / ctrl を拾えない機種があったので。
                        if (softKeyCtrl == true) {
                            emu->get_osd()->key_down(113, false, false);
                            softKeyDelayFlag = true;
                            //emu->get_osd()->key_down(softKeyCode, false, false);
                            softKeyboardCount = SOFT_KEYBOARD_KEEP_COUNT;
                        }
                        if (softKeyShift == true) {
                            softKeyCode = usKeytoJISKeyShift[code][0];
                            if (usKeytoJISKeyShift[code][1] == 1) {
                                emu->get_osd()->key_down(59, false, false);
                                softKeyShift = true;
                            } else {
                                softKeyShift = false;
                            }
                            softKeyDelayFlag = true;
                            //emu->get_osd()->key_down(softKeyCode, false, false);
                            softKeyboardCount = SOFT_KEYBOARD_KEEP_COUNT;
                        } else {
                            softKeyCode = usKeytoJISKey[code][0];
                            if (usKeytoJISKey[code][1] == 1) {
                                emu->get_osd()->key_down(59, false, false);
                                softKeyShift = true;
                            } else {
                                softKeyShift = false;
                            }
                            softKeyDelayFlag = true;
                            //emu->get_osd()->key_down(softKeyCode, false, false);
                        }
                        softKeyboardCount = SOFT_KEYBOARD_KEEP_COUNT;
                    }
                }

            } else {
                emu->get_osd()->key_down(code, false, false);
            }
        } else if (action == AKEY_EVENT_ACTION_UP) {
            if ((flags & AKEY_EVENT_FLAG_FROM_SYSTEM) == 0) { //ソフトウェアキーボード

            } else {
                emu->get_osd()->key_up(code, false);
            }
        } else if (action == AKEY_EVENT_ACTION_MULTIPLE) {
            int unicodeCharactor = get_unicode_character(app,
                                                         AInputEvent_getType(event),
                                                         AKeyEvent_getKeyCode(event),
                                                         AKeyEvent_getMetaState(event));
            LOGI("unicodeCharactor:%d", unicodeCharactor);
        }
    }
    return 0;
}

#ifdef USE_AUTO_KEY
#if defined(_WIN32)
void start_auto_key()
{
    if(OpenClipboard(NULL)) {
        HANDLE hClip = GetClipboardData(CF_TEXT);
        if(hClip) {
            char* buf = (char*)GlobalLock(hClip);
            int size = (int)strlen(buf);

            if(size > 0) {
                emu->stop_auto_key();
                emu->set_auto_key_list(buf, size);
                emu->start_auto_key();
            }
            GlobalUnlock(hClip);
        }
        CloseClipboard();
    }
}
#else
void start_auto_key(struct android_app *app)
{
    std::vector<uint8_t> clipboardData = getClipboardText(app);

    if (!clipboardData.empty()) {
        // vectorのデータを直接ポインタとして使用
        uint8_t* buf = clipboardData.data();
        int size = static_cast<int>(clipboardData.size());

        LOGI("clipboardData size: %d", size);

        if(size > 0) {
            emu->stop_auto_key();
            // set_auto_key_list が char* を受け取る場合、キャストが必要です
            emu->set_auto_key_list(reinterpret_cast<char*>(buf), size);
            emu->start_auto_key();
        }
    }
}
#endif
#endif

// ----------------------------------------------------------------------------
// dialog
// ----------------------------------------------------------------------------

#if defined(_WIN32)
#ifdef USE_SOUND_VOLUME
INT_PTR CALLBACK VolumeWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
switch(iMsg) {
case WM_CLOSE:
EndDialog(hDlg, IDCANCEL);
break;
case WM_INITDIALOG:
for(int i = 0; i < USE_SOUND_VOLUME; i++) {
SetDlgItemText(hDlg, IDC_VOLUME_CAPTION0 + i, sound_device_caption[i]);
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_L0 + i, TBM_SETTICFREQ, 5, 0);
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_R0 + i, TBM_SETTICFREQ, 5, 0);
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_L0 + i, TBM_SETRANGE, TRUE, MAKELPARAM(-40, 0));
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_R0 + i, TBM_SETRANGE, TRUE, MAKELPARAM(-40, 0));
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_L0 + i, TBM_SETPOS, TRUE, max(-40, min(0, config.sound_volume_l[i])));
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_R0 + i, TBM_SETPOS, TRUE, max(-40, min(0, config.sound_volume_r[i])));
//			EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_CAPTION0 + i), TRUE);
//			EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_PARAM_L0 + i), TRUE);
//			EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_PARAM_R0 + i), TRUE);
}
break;
case WM_COMMAND:
switch(LOWORD(wParam)) {
case IDOK:
for(int i = 0; i < USE_SOUND_VOLUME; i++) {
config.sound_volume_l[i] = (int)SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_L0 + i, TBM_GETPOS, 0, 0);
config.sound_volume_r[i] = (int)SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_R0 + i, TBM_GETPOS, 0, 0);
emu->set_sound_device_volume(i, config.sound_volume_l[i], config.sound_volume_r[i]);
}
EndDialog(hDlg, IDOK);
break;
case IDC_VOLUME_RESET:
for(int i = 0; i < USE_SOUND_VOLUME; i++) {
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_L0 + i, TBM_SETPOS, TRUE, 0);
SendDlgItemMessage(hDlg, IDC_VOLUME_PARAM_R0 + i, TBM_SETPOS, TRUE, 0);
}
break;
default:
return FALSE;
}
break;
default:
return FALSE;
}
return TRUE;
}
#endif

#ifdef USE_JOYSTICK
// from http://homepage3.nifty.com/ic/help/rmfunc/vkey.htm
static const _TCHAR *vk_names[] = {
        _T("VK_$00"),			_T("VK_LBUTTON"),		_T("VK_RBUTTON"),		_T("VK_CANCEL"),
        _T("VK_MBUTTON"),		_T("VK_XBUTTON1"),		_T("VK_XBUTTON2"),		_T("VK_$07"),
        _T("VK_BACK"),			_T("VK_TAB"),			_T("VK_$0A"),			_T("VK_$0B"),
        _T("VK_CLEAR"),			_T("VK_RETURN"),		_T("VK_$0E"),			_T("VK_$0F"),
        _T("VK_SHIFT"),			_T("VK_CONTROL"),		_T("VK_MENU"),			_T("VK_PAUSE"),
        _T("VK_CAPITAL"),		_T("VK_KANA"),			_T("VK_$16"),			_T("VK_JUNJA"),
        _T("VK_FINAL"),			_T("VK_KANJI"),			_T("VK_$1A"),			_T("VK_ESCAPE"),
        _T("VK_CONVERT"),		_T("VK_NONCONVERT"),		_T("VK_ACCEPT"),		_T("VK_MODECHANGE"),
        _T("VK_SPACE"),			_T("VK_PRIOR"),			_T("VK_NEXT"),			_T("VK_END"),
        _T("VK_HOME"),			_T("VK_LEFT"),			_T("VK_UP"),			_T("VK_RIGHT"),
        _T("VK_DOWN"),			_T("VK_SELECT"),		_T("VK_PRINT"),			_T("VK_EXECUTE"),
        _T("VK_SNAPSHOT"),		_T("VK_INSERT"),		_T("VK_DELETE"),		_T("VK_HELP"),
        _T("VK_0"),			_T("VK_1"),			_T("VK_2"),			_T("VK_3"),
        _T("VK_4"),			_T("VK_5"),			_T("VK_6"),			_T("VK_7"),
        _T("VK_8"),			_T("VK_9"),			_T("VK_$3A"),			_T("VK_$3B"),
        _T("VK_$3C"),			_T("VK_$3D"),			_T("VK_$3E"),			_T("VK_$3F"),
        _T("VK_$40"),			_T("VK_A"),			_T("VK_B"),			_T("VK_C"),
        _T("VK_D"),			_T("VK_E"),			_T("VK_F"),			_T("VK_G"),
        _T("VK_H"),			_T("VK_I"),			_T("VK_J"),			_T("VK_K"),
        _T("VK_L"),			_T("VK_M"),			_T("VK_N"),			_T("VK_O"),
        _T("VK_P"),			_T("VK_Q"),			_T("VK_R"),			_T("VK_S"),
        _T("VK_T"),			_T("VK_U"),			_T("VK_V"),			_T("VK_W"),
        _T("VK_X"),			_T("VK_Y"),			_T("VK_Z"),			_T("VK_LWIN"),
        _T("VK_RWIN"),			_T("VK_APPS"),			_T("VK_$5E"),			_T("VK_SLEEP"),
        _T("VK_NUMPAD0"),		_T("VK_NUMPAD1"),		_T("VK_NUMPAD2"),		_T("VK_NUMPAD3"),
        _T("VK_NUMPAD4"),		_T("VK_NUMPAD5"),		_T("VK_NUMPAD6"),		_T("VK_NUMPAD7"),
        _T("VK_NUMPAD8"),		_T("VK_NUMPAD9"),		_T("VK_MULTIPLY"),		_T("VK_ADD"),
        _T("VK_SEPARATOR"),		_T("VK_SUBTRACT"),		_T("VK_DECIMAL"),		_T("VK_DIVIDE"),
        _T("VK_F1"),			_T("VK_F2"),			_T("VK_F3"),			_T("VK_F4"),
        _T("VK_F5"),			_T("VK_F6"),			_T("VK_F7"),			_T("VK_F8"),
        _T("VK_F9"),			_T("VK_F10"),			_T("VK_F11"),			_T("VK_F12"),
        _T("VK_F13"),			_T("VK_F14"),			_T("VK_F15"),			_T("VK_F16"),
        _T("VK_F17"),			_T("VK_F18"),			_T("VK_F19"),			_T("VK_F20"),
        _T("VK_F21"),			_T("VK_F22"),			_T("VK_F23"),			_T("VK_F24"),
        _T("VK_$88"),			_T("VK_$89"),			_T("VK_$8A"),			_T("VK_$8B"),
        _T("VK_$8C"),			_T("VK_$8D"),			_T("VK_$8E"),			_T("VK_$8F"),
        _T("VK_NUMLOCK"),		_T("VK_SCROLL"),		_T("VK_$92"),			_T("VK_$93"),
        _T("VK_$94"),			_T("VK_$95"),			_T("VK_$96"),			_T("VK_$97"),
        _T("VK_$98"),			_T("VK_$99"),			_T("VK_$9A"),			_T("VK_$9B"),
        _T("VK_$9C"),			_T("VK_$9D"),			_T("VK_$9E"),			_T("VK_$9F"),
        _T("VK_LSHIFT"),		_T("VK_RSHIFT"),		_T("VK_LCONTROL"),		_T("VK_RCONTROL"),
        _T("VK_LMENU"),			_T("VK_RMENU"),			_T("VK_BROWSER_BACK"),		_T("VK_BROWSER_FORWARD"),
        _T("VK_BROWSER_REFRESH"),	_T("VK_BROWSER_STOP"),		_T("VK_BROWSER_SEARCH"),	_T("VK_BROWSER_FAVORITES"),
        _T("VK_BROWSER_HOME"),		_T("VK_VOLUME_MUTE"),		_T("VK_VOLUME_DOWN"),		_T("VK_VOLUME_UP"),
        _T("VK_MEDIA_NEXT_TRACK"),	_T("VK_MEDIA_PREV_TRACK"),	_T("VK_MEDIA_STOP"),		_T("VK_MEDIA_PLAY_PAUSE"),
        _T("VK_LAUNCH_MAIL"),		_T("VK_LAUNCH_MEDIA_SELECT"),	_T("VK_LAUNCH_APP1"),		_T("VK_LAUNCH_APP2"),
        _T("VK_$B8"),			_T("VK_$B9"),			_T("VK_OEM_1"),			_T("VK_OEM_PLUS"),
        _T("VK_OEM_COMMA"),		_T("VK_OEM_MINUS"),		_T("VK_OEM_PERIOD"),		_T("VK_OEM_2"),
        _T("VK_OEM_3"),			_T("VK_$C1"),			_T("VK_$C2"),			_T("VK_$C3"),
        _T("VK_$C4"),			_T("VK_$C5"),			_T("VK_$C6"),			_T("VK_$C7"),
        _T("VK_$C8"),			_T("VK_$C9"),			_T("VK_$CA"),			_T("VK_$CB"),
        _T("VK_$CC"),			_T("VK_$CD"),			_T("VK_$CE"),			_T("VK_$CF"),
        _T("VK_$D0"),			_T("VK_$D1"),			_T("VK_$D2"),			_T("VK_$D3"),
        _T("VK_$D4"),			_T("VK_$D5"),			_T("VK_$D6"),			_T("VK_$D7"),
        _T("VK_$D8"),			_T("VK_$D9"),			_T("VK_$DA"),			_T("VK_OEM_4"),
        _T("VK_OEM_5"),			_T("VK_OEM_6"),			_T("VK_OEM_7"),			_T("VK_OEM_8"),
        _T("VK_$E0"),			_T("VK_OEM_AX"),		_T("VK_OEM_102"),		_T("VK_ICO_HELP"),
        _T("VK_ICO_00"),		_T("VK_PROCESSKEY"),		_T("VK_ICO_CLEAR"),		_T("VK_PACKET"),
        _T("VK_$E8"),			_T("VK_OEM_RESET"),		_T("VK_OEM_JUMP"),		_T("VK_OEM_PA1"),
        _T("VK_OEM_PA2"),		_T("VK_OEM_PA3"),		_T("VK_OEM_WSCTRL"),		_T("VK_OEM_CUSEL"),
        _T("VK_OEM_ATTN"),		_T("VK_OEM_FINISH"),		_T("VK_OEM_COPY"),		_T("VK_OEM_AUTO"),
        _T("VK_OEM_ENLW"),		_T("VK_OEM_BACKTAB"),		_T("VK_ATTN"),			_T("VK_CRSEL"),
        _T("VK_EXSEL"),			_T("VK_EREOF"),			_T("VK_PLAY"),			_T("VK_ZOOM"),
        _T("VK_NONAME"),		_T("VK_PA1"),			_T("VK_OEM_CLEAR"),		_T("VK_$FF"),
};

static const _TCHAR *joy_button_names[32] = {
        _T("Up"),
        _T("Down"),
        _T("Left"),
        _T("Right"),
        _T("Button #1"),
        _T("Button #2"),
        _T("Button #3"),
        _T("Button #4"),
        _T("Button #5"),
        _T("Button #6"),
        _T("Button #7"),
        _T("Button #8"),
        _T("Button #9"),
        _T("Button #10"),
        _T("Button #11"),
        _T("Button #12"),
        _T("Button #13"),
        _T("Button #14"),
        _T("Button #15"),
        _T("Button #16"),
        _T("Z-Axis Low"),
        _T("Z-Axis High"),
        _T("R-Axis Low"),
        _T("R-Axis High"),
        _T("U-Axis Low"),
        _T("U-Axis High"),
        _T("V-Axis Low"),
        _T("V-Axis High"),
        _T("POV 0deg"),
        _T("POV 90deg"),
        _T("POV 180deg"),
        _T("POV 270deg"),
};

HWND hJoyDlg;
HWND hJoyEdit[16];
WNDPROC JoyOldProc[16];
int joy_stick_index;
int joy_button_index;
int joy_button_params[16];
uint32_t joy_status[4];

#define get_joy_range(min_value, max_value, lo_value, hi_value) \
{ \
	uint64_t center = ((uint64_t)min_value + (uint64_t)max_value) / 2; \
	lo_value = (DWORD)((center + (uint64_t)min_value) / 2); \
	hi_value = (DWORD)((center + (uint64_t)max_value) / 2); \
}

uint32_t get_joy_status(int index)
{
    JOYCAPS joycaps;
    JOYINFOEX joyinfo;
    uint32_t status = 0;

    if(joyGetDevCaps(index, &joycaps, sizeof(JOYCAPS)) == JOYERR_NOERROR) {
        joyinfo.dwSize = sizeof(JOYINFOEX);
        joyinfo.dwFlags = JOY_RETURNALL;
        if(joyGetPosEx(index, &joyinfo) == JOYERR_NOERROR) {
            if(joycaps.wNumAxes >= 2) {
                DWORD dwYposLo, dwYposHi;
                get_joy_range(joycaps.wYmin, joycaps.wYmax, dwYposLo, dwYposHi);
                if(joyinfo.dwYpos < dwYposLo) status |= 0x00000001;	// up
                if(joyinfo.dwYpos > dwYposHi) status |= 0x00000002;	// down
            }
            if(joycaps.wNumAxes >= 1) {
                DWORD dwXposLo, dwXposHi;
                get_joy_range(joycaps.wXmin, joycaps.wXmax, dwXposLo, dwXposHi);
                if(joyinfo.dwXpos < dwXposLo) status |= 0x00000004;	// left
                if(joyinfo.dwXpos > dwXposHi) status |= 0x00000008;	// right
            }
            if(joycaps.wNumAxes >= 3) {
                DWORD dwZposLo, dwZposHi;
                get_joy_range(joycaps.wZmin, joycaps.wZmax, dwZposLo, dwZposHi);
                if(joyinfo.dwZpos < dwZposLo) status |= 0x00100000;
                if(joyinfo.dwZpos > dwZposHi) status |= 0x00200000;
            }
            if(joycaps.wNumAxes >= 4) {
                DWORD dwRposLo, dwRposHi;
                get_joy_range(joycaps.wRmin, joycaps.wRmax, dwRposLo, dwRposHi);
                if(joyinfo.dwRpos < dwRposLo) status |= 0x00400000;
                if(joyinfo.dwRpos > dwRposHi) status |= 0x00800000;
            }
            if(joycaps.wNumAxes >= 5) {
                DWORD dwUposLo, dwUposHi;
                get_joy_range(joycaps.wUmin, joycaps.wUmax, dwUposLo, dwUposHi);
                if(joyinfo.dwUpos < dwUposLo) status |= 0x01000000;
                if(joyinfo.dwUpos > dwUposHi) status |= 0x02000000;
            }
            if(joycaps.wNumAxes >= 6) {
                DWORD dwVposLo, dwVposHi;
                get_joy_range(joycaps.wVmin, joycaps.wVmax, dwVposLo, dwVposHi);
                if(joyinfo.dwVpos < dwVposLo) status |= 0x04000000;
                if(joyinfo.dwVpos > dwVposHi) status |= 0x08000000;
            }
            if(joyinfo.dwPOV != 0xffff) {
                static const uint32_t dir[8] = {
                        0x10000000 + 0x00000000,
                        0x10000000 + 0x20000000,
                        0x00000000 + 0x20000000,
                        0x40000000 + 0x20000000,
                        0x40000000 + 0x00000000,
                        0x40000000 + 0x80000000,
                        0x00000000 + 0x80000000,
                        0x10000000 + 0x80000000,
                };
                for(int i = 0; i < 9; i++) {
                    if(joyinfo.dwPOV < (DWORD)(2250 + 4500 * i)) {
                        status |= dir[i & 7];
                        break;
                    }
                }
            }
            DWORD dwButtonsMask = (1 << min(16, joycaps.wNumButtons)) - 1;
            status |= ((joyinfo.dwButtons & dwButtonsMask) << 4);
        }
    }
    return status;
}

void set_joy_button_text(int index)
{
    if(joy_button_params[index] < 0) {
        SetDlgItemText(hJoyDlg, IDC_JOYSTICK_PARAM0 + index, vk_names[-joy_button_params[index]]);
    } else if(joy_stick_index == -1) {
        SetDlgItemText(hJoyDlg, IDC_JOYSTICK_PARAM0 + index, _T("(None)"));
    } else {
        SetDlgItemText(hJoyDlg, IDC_JOYSTICK_PARAM0 + index, create_string(_T("Joystick #%d - %s"), (joy_button_params[index] >> 5) + 1, joy_button_names[joy_button_params[index] & 0x1f]));
    }
}

LRESULT CALLBACK JoySubProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
int index = -1;
for(int i = 0; i < 16; i++) {
if(hWnd == hJoyEdit[i]) {
index = i;
break;
}
}
if(index == -1) {
return 0L;
}
switch(iMsg) {
case WM_CHAR:
return 0L;
case WM_KEYDOWN:
case WM_SYSKEYDOWN:
if(joy_stick_index == -1 && LOBYTE(wParam) == VK_BACK) {
joy_button_params[index] = 0;
} else {
joy_button_params[index] = -(int)LOBYTE(wParam);
}
set_joy_button_text(index);
if(hJoyEdit[++index] == NULL) {
index = 0;
}
SetFocus(hJoyEdit[index]);
return 0L;
case WM_SETFOCUS:
joy_button_index = index;
break;
default:
break;
}
return CallWindowProc(JoyOldProc[index], hWnd, iMsg, wParam, lParam);
}

INT_PTR CALLBACK JoyWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
switch(iMsg) {
case WM_CLOSE:
EndDialog(hDlg, IDCANCEL);
break;
case WM_INITDIALOG:
hJoyDlg = hDlg;
joy_stick_index = (int)(*(LONG*)lParam);
SetWindowText(hDlg, create_string(_T("Joystick #%d"), joy_stick_index + 1));
for(int i = 0; i < 16; i++) {
joy_button_params[i] = config.joy_buttons[joy_stick_index][i];
if((hJoyEdit[i] = GetDlgItem(hDlg, IDC_JOYSTICK_PARAM0 + i)) != NULL) {
#ifdef USE_JOY_BUTTON_CAPTIONS
if(i < array_length(joy_button_captions)) {
					SetDlgItemText(hDlg, IDC_JOYSTICK_CAPTION0 + i, joy_button_captions[i]);
				} else
#endif
SetDlgItemText(hDlg, IDC_JOYSTICK_CAPTION0 + i, joy_button_names[i]);
set_joy_button_text(i);
#ifdef _M_AMD64
// thanks Marukun (64bit)
				JoyOldProc[i] = (WNDPROC)GetWindowLongPtr(hJoyEdit[i], GWLP_WNDPROC);
				SetWindowLongPtr(hJoyEdit[i], GWLP_WNDPROC, (LONG_PTR)JoySubProc);
#else
JoyOldProc[i] = (WNDPROC)GetWindowLong(hJoyEdit[i], GWL_WNDPROC);
SetWindowLong(hJoyEdit[i], GWL_WNDPROC, (LONG)JoySubProc);
#endif
}
}
memset(joy_status, 0, sizeof(joy_status));
SetTimer(hDlg, 1, 100, NULL);
break;
case WM_COMMAND:
switch(LOWORD(wParam)) {
case IDOK:
for(int i = 0; i < 16; i++) {
config.joy_buttons[joy_stick_index][i] = joy_button_params[i];
}
EndDialog(hDlg, IDOK);
break;
case IDC_JOYSTICK_RESET:
for(int i = 0; i < 16; i++) {
joy_button_params[i] = (joy_stick_index << 5) | i;
set_joy_button_text(i);
}
break;
default:
return FALSE;
}
break;
case WM_TIMER:
for(int i = 0; i < 4; i++) {
uint32_t status = get_joy_status(i);
for(int j = 0; j < 32; j++) {
uint32_t bit = 1 << j;
if((joy_status[i] & bit) && !(status & bit)) {
joy_button_params[joy_button_index] = (i << 5) | j;
set_joy_button_text(joy_button_index);
if(hJoyEdit[++joy_button_index] == NULL) {
joy_button_index = 0;
}
SetFocus(hJoyEdit[joy_button_index]);
break;
}
}
joy_status[i] = status;
}
break;
default:
return (INT_PTR)FALSE;
}
return (INT_PTR)TRUE;
}

INT_PTR CALLBACK JoyToKeyWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
switch(iMsg) {
case WM_CLOSE:
EndDialog(hDlg, IDCANCEL);
break;
case WM_INITDIALOG:
hJoyDlg = hDlg;
joy_stick_index = -1;//(int)(*(LONG*)lParam);
//		SetWindowText(hDlg, create_string(_T("Joystick To Keyboard #%d"), joy_stick_index + 1));
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_CHECK0), BM_SETCHECK, (WPARAM)config.use_joy_to_key, 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO0), BM_SETCHECK, (WPARAM)(config.joy_to_key_type == 0), 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO1), BM_SETCHECK, (WPARAM)(config.joy_to_key_type == 1), 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO2), BM_SETCHECK, (WPARAM)(config.joy_to_key_type == 2), 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_CHECK1), BM_SETCHECK, (WPARAM)config.joy_to_key_numpad5, 0L);
for(int i = 0; i < 16; i++) {
joy_button_params[i] = config.joy_to_key_buttons[i];
if((hJoyEdit[i] = GetDlgItem(hDlg, IDC_JOYSTICK_PARAM0 + i)) != NULL) {
set_joy_button_text(i);
#ifdef _M_AMD64
// thanks Marukun (64bit)
				JoyOldProc[i] = (WNDPROC)GetWindowLongPtr(hJoyEdit[i], GWLP_WNDPROC);
				SetWindowLongPtr(hJoyEdit[i], GWLP_WNDPROC, (LONG_PTR)JoySubProc);
#else
JoyOldProc[i] = (WNDPROC)GetWindowLong(hJoyEdit[i], GWL_WNDPROC);
SetWindowLong(hJoyEdit[i], GWL_WNDPROC, (LONG)JoySubProc);
#endif
}
}
memset(joy_status, 0, sizeof(joy_status));
SetTimer(hDlg, 1, 100, NULL);
break;
case WM_COMMAND:
switch(LOWORD(wParam)) {
case IDOK:
config.use_joy_to_key = (IsDlgButtonChecked(hDlg, IDC_JOYTOKEY_CHECK0) == BST_CHECKED);
if(IsDlgButtonChecked(hDlg, IDC_JOYTOKEY_RADIO0) == BST_CHECKED) {
config.joy_to_key_type = 0;
} else if(IsDlgButtonChecked(hDlg, IDC_JOYTOKEY_RADIO1) == BST_CHECKED) {
config.joy_to_key_type = 1;
} else {
config.joy_to_key_type = 2;
}
config.joy_to_key_numpad5 = (IsDlgButtonChecked(hDlg, IDC_JOYTOKEY_CHECK1) == BST_CHECKED);
for(int i = 0; i < 16; i++) {
config.joy_to_key_buttons[i] = joy_button_params[i];
}
EndDialog(hDlg, IDOK);
break;
case IDC_JOYSTICK_RESET:
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_CHECK0), BM_SETCHECK, (WPARAM)false, 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO0), BM_SETCHECK, (WPARAM)false, 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO1), BM_SETCHECK, (WPARAM)false, 0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_RADIO2), BM_SETCHECK, (WPARAM)true,  0L);
SendMessage(GetDlgItem(hDlg, IDC_JOYTOKEY_CHECK1), BM_SETCHECK, (WPARAM)false, 0L);
for(int i = 0; i < 16; i++) {
joy_button_params[i] = (i == 0) ? -('Z') : (i == 1) ? -('X') : 0;
set_joy_button_text(i);
}
break;
default:
return (INT_PTR)FALSE;
}
break;
case WM_TIMER:
for(int i = 0; i < 1; i++) {
uint32_t status = get_joy_status(i);
for(int j = 0; j < 16; j++) {
uint32_t bit = 1 << (j + 4);
if((joy_status[i] & bit) && !(status & bit)) {
if(hJoyEdit[j] != NULL) {
joy_button_index = j;
SetFocus(hJoyEdit[joy_button_index]);
}
break;
}
}
joy_status[i] = status;
}
break;
default:
return (INT_PTR)FALSE;
}
return (INT_PTR)TRUE;
}
#endif
#endif

/** @return the id of the button clicked if model is true, or 0 */
jint showAlert(struct android_app *state, const char *message, const char *itemNames,
               bool model /* = false */, int selectMode, int selectIndex) {
    JNIEnv *jni = NULL;
    state->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass clazz = jni->GetObjectClass(state->activity->clazz);

    // Get the ID of the method we want to call
    // This must match the name and signature from the Java side Signature has to match java
    // implementation (second string hints a java string parameter)
    jmethodID methodID = jni->GetMethodID(clazz, "showAlert",
                                          "(Ljava/lang/String;Ljava/lang/String;ZI)I");

    // Strings passed to the function need to be converted to a java string object
    jstring jmessage = jni->NewStringUTF(message);
    jobjectArray day = 0;
    jstring jfilenames = jni->NewStringUTF(itemNames);

    jint result = jni->CallIntMethod(state->activity->clazz, methodID, jmessage, jfilenames, model,
                                     selectMode);

    // Remember to clean up passed values
    jni->DeleteLocalRef(jmessage);

    state->activity->vm->DetachCurrentThread();

    return result;
}

void showNewFileDialog(
        struct android_app *state,
        const char *message,
        const char *itemNames,
        const char *fileExtension,
        MediaInfo *mediaInfo,
        const char *addPath
) {
    JNIEnv* env = nullptr;
    state->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass clazz = env->GetObjectClass(state->activity->clazz);
    jmethodID mid = env->GetMethodID(clazz, "showNewFileDialog", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

    if (mid == nullptr) {
        // メソッドIDが取得できなかった場合のエラー処理
        env->DeleteLocalRef(clazz);
        state->activity->vm->DetachCurrentThread();
        return;
    }

    jstring jMessage = env->NewStringUTF(message);
    jstring jItemList = env->NewStringUTF(itemNames);
    jstring jExtension = env->NewStringUTF(fileExtension);

    std::string strMediaInfo =
            std::to_string(mediaInfo->drive) + "," +
            std::to_string(static_cast<int>(mediaInfo->fileSelectType)) + "," +
            std::to_string(mediaInfo->floppy_type) + "," +
            std::to_string(mediaInfo->hdd_sector_size) + "," +
            std::to_string(mediaInfo->hdd_sectors) + "," +
            std::to_string(mediaInfo->hdd_surfaces) + "," +
            std::to_string(mediaInfo->hdd_cylinders);
    jstring jMediaInfo = env->NewStringUTF(strMediaInfo.c_str());

    jstring jAddPath = env->NewStringUTF(addPath);

    env->CallVoidMethod(state->activity->clazz, mid, jMessage, jItemList, jExtension, jMediaInfo, jAddPath);

    // ローカル参照の解放
    env->DeleteLocalRef(jMessage);
    env->DeleteLocalRef(jItemList);
    env->DeleteLocalRef(jExtension);
    env->DeleteLocalRef(jMediaInfo);
    env->DeleteLocalRef(jAddPath);
    env->DeleteLocalRef(clazz);

    state->activity->vm->DetachCurrentThread();
}

jint showExtendMenu(struct android_app *state, const char *title, const char *extendMenuString) {
    JNIEnv *jni = NULL;
    state->activity->vm->AttachCurrentThread(&jni, NULL);
    jclass clazz = jni->GetObjectClass(state->activity->clazz);
    jmethodID methodID = jni->GetMethodID(clazz, "showExtendMenu", "(Ljava/lang/String;Ljava/lang/String;)I");
    jstring jtitle = jni->NewStringUTF(title);
    jstring jextendMenu = jni->NewStringUTF(extendMenuString);
    jint result = jni->CallIntMethod(state->activity->clazz, methodID, jtitle, jextendMenu);
    jni->DeleteLocalRef(jextendMenu);
    jni->DeleteLocalRef(jtitle);
    state->activity->vm->DetachCurrentThread();
    return result;
}

// 拡張メニュー生成と表示
void extendMenu(struct android_app *app)
{
    if (extendMenuDisplay) return;
    // メニュー文字列を取得する
    extendMenuString = menu->getExtendMenuString(1);
    // NDKからJavaの関数を呼び出す
    extendMenuDisplay = true;
    jint nodeId = showExtendMenu(app, menu->getCaption(1).c_str(), extendMenuString.c_str());
}

void selectDialog(struct android_app *state, const char *message, const char *addPath) {
    char long_message[_MAX_PATH+33];

    fileList.clear();

    const char *aplicationPath = get_application_path();
    char dirPath[_MAX_PATH];
    sprintf(dirPath, "%s%s", aplicationPath, addPath);
    DIR *dir;
    struct dirent *dp;
    dir = opendir(dirPath);

    std::vector<std::string> tempFileList;
    std::string filenameList = "[EJECT]";
    fileList.push_back("");

    if (dir == NULL) {
        char errorMessage[32];
        sprintf(errorMessage, "Directory does not exist: %s", addPath);
        sprintf(long_message, "%s\n%s", errorMessage, dirPath);
        showAlert(state, long_message, filenameList.c_str(), true, MEDIA_SELECT);
    } else {
        while ((dp = readdir(dir)) != NULL) {
            std::string filename = dp->d_name;
            if (filename.find_first_of(".") != 0) { // Skip hidden files and directories
                tempFileList.push_back(filename);
            }
        }
        closedir(dir);

        // Sort the file list in ascending order
        std::sort(tempFileList.begin(), tempFileList.end(), caseInsensitiveCompare);

        for (const auto& filename : tempFileList) {
            if (!filenameList.empty()) {
                filenameList += ";";
            }
            filenameList += filename;
            std::string filePath = std::string(dirPath) + "/" + filename;
            fileList.push_back(filePath);
        }
        showAlert(state, message, filenameList.c_str(), true, MEDIA_SELECT, 0);
    }
}

#ifdef USE_FLOPPY_DISK

void selectD88Bank(struct android_app *state, int driveNo) {
    if (emu->d88_file[driveNo].bank_num <= 1) {
        return;
    }
    char message[32];
    sprintf(message, "Select DISK %d BANK", driveNo);
    std::string disknameList;
    for (int index = 0; index < emu->d88_file[driveNo].bank_num; index++) {
        std::string diskname = emu->d88_file[driveNo].disk_name[index];
        if (disknameList.length() > 0) {
            disknameList = disknameList + ";" + diskname;
        } else {
            disknameList = diskname;
        }
    }
    showAlert(state, message, disknameList.c_str(), true, DISK_BANK_SELECT, 0);
}

#endif

void selectBootMode(struct android_app *state) {

    char message[128];
    sprintf(message, "Reset Emulator?");
    std::string itemList;
#ifdef _PC8801MA
    itemList="N88-V1(S) mode;N88-V1(H) mode;N88-V2 mode;N mode;N88-V2(CD) mode";
#endif
#ifdef _X1TURBO_FEATURE
    itemList="High mode;Standard mode";
#endif

    showAlert(state, message, itemList.c_str(), true, BOOT_MODE_SELECT, 0);
}

#if defined(USE_FLOPPY_DISK) || defined(USE_QUICK_DISK) || defined(USE_TAPE)

void createBlankDisk(struct android_app *state, MediaInfo *mediaInfo) {

    // addPath が "DISK" か "QD" か "TAPE" かで拡張子と保存パスを設定する
    std::string fileExtension;
    std::string addPath = "DISK";
    std::string message = "Create Blank Disk";
    switch(mediaInfo->fileSelectType)
    {
        case FLOPPY_DISK:
            //_T("Supported Files (*.d88;*.d8e;*.d77;*.1dd;*.td0;*.imd;*.dsk;*.nfd;*.fdi;*.hdm;*.hd5;*.hd4;*.hdb;
            // *.dd9;*.dd6;*.tfd;*.xdf;*.2d;*.sf7;*.img;*.ima;*.vfd)\0*.d88;*.d8e;*.d77;*.1dd;*.td0;*.imd;*.dsk;*.nfd;
            // *.fdi;*.hdm;*.hd5;*.hd4;*.hdb;*.dd9;*.dd6;*.tfd;*.xdf;*.2d;*.sf7;*.img;*.ima;*.vfd\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".d88";
            addPath = "DISK";
            message = "New Floppy";
            break;
        case QUICK_DISK:
            //_T("Supported Files (*.mzt;*.q20;*.qdf)\0*.mzt;*.q20;*.qdf\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".qdf";
            addPath = "QD";
            message = "New Quick Disk";
            break;
        case HARD_DISK:
            //_T("Supported Files (*.hdi;*.nhd)\0*.hdi;*.nhd\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".hdi";
            addPath = "HDD";
            message = "New Hard Disk";
            break;
        case CASETTE_TAPE:
#if defined(_PC6001) || defined(_PC6001MK2) || defined(_PC6001MK2SR) || defined(_PC6601) || defined(_PC6601SR)
            // : _T("Supported Files (*.wav;*.cas;*.p6;*.p6t)\0*.wav;*.cas;*.p6;*.p6t\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_PC8001) || defined(_PC8001MK2) || defined(_PC8001SR) || defined(_PC8801) || defined(_PC8801MK2) || defined(_PC8801MA) || defined(_PC98DO)
            // : _T("Supported Files (*.cas;*.cmt)\0*.cas;*.cmt\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cmt";
#elif defined(_MZ80A) || defined(_MZ80K) || defined(_MZ1200) || defined(_MZ700) || defined(_MZ800) || defined(_MZ1500)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_MZ80B) || defined(_MZ2000) || defined(_MZ2200)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_MZ2500)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_X1) || defined(_X1TWIN) || defined(_X1TURBO) || defined(_X1TURBOZ)
            // : _T("Supported Files (*.wav;*.cas;*.tap)\0*.wav;*.cas;*.tap\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".tap";
#elif defined(_FM8) || defined(_FM7) || defined(_FMNEW7) || defined(_FM77_VARIANTS) || defined(_FM77AV_VARIANTS)
            // : _T("Supported Files (*.wav;*.cas;*.t77)\0*.wav;*.cas;*.t77\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_BMJR)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif defined(_TK80BS)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#elif !defined(TAPE_BINARY_ONLY)
            // : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#else
    		//_T("Supported Files (*.cas;*.cmt)\0*.cas;*.cmt\0All Files (*.*)\0*.*\0\0"),
            fileExtension = ".cas";
#endif
            message = "New Tape";
            addPath = "TAPE";
            break;
        default:
            break;
    }
    const char *aplicationPath = get_application_path();
    char dirPath[_MAX_PATH];
    sprintf(dirPath, "%s%s", aplicationPath, addPath.c_str());

    DIR *dir;
    struct dirent *dp;
    dir = opendir(dirPath);

    std::vector<std::string> tempFileList;
    std::string filenameList = "";

    if (dir != NULL) {
        while ((dp = readdir(dir)) != NULL) {
            std::string filename = dp->d_name;
            if (filename.find_first_of(".") != 0) { // Skip hidden files and directories
                tempFileList.push_back(filename);
            }
        }
        closedir(dir);
        // Sort the file list in ascending order
        std::sort(tempFileList.begin(), tempFileList.end(), caseInsensitiveCompare);
        for (const auto& filename : tempFileList) {
            if (!filenameList.empty()) {
                filenameList += ";";
            }
            filenameList += filename;
            std::string filePath = std::string(dirPath) + "/" + filename;
        }
    }
    showNewFileDialog(
            state,
            message.c_str(),
            filenameList.c_str(),
            fileExtension.c_str(),
            mediaInfo,
            addPath.c_str());
}

#endif

// ----------------------------------------------------------------------------
// button
// ----------------------------------------------------------------------------

#if defined(_WIN32)
#ifdef ONE_BOARD_MICRO_COMPUTER
#define MAX_FONT_SIZE 32
HWND hButton[MAX_BUTTONS];
WNDPROC ButtonOldProc[MAX_BUTTONS];

LRESULT CALLBACK ButtonSubProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	for(int i = 0; i < MAX_BUTTONS; i++) {
		if(hWnd == hButton[i]) {
			switch(iMsg) {
			case WM_KEYDOWN:
				if(emu) {
					bool extended = ((HIWORD(lParam) & 0x100) != 0);
					bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
					int code = LOBYTE(wParam);
					if(code == VK_PROCESSKEY) {
						code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
					}
					emu->key_down(code, extended, repeat);
				}
				break;
			case WM_SYSKEYDOWN:
				if(emu) {
					bool extended = ((HIWORD(lParam) & 0x100) != 0);
					bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
					int code = LOBYTE(wParam);
					if(code == VK_PROCESSKEY) {
						code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
					}
					emu->key_down(code, extended, repeat);
				}
				return 0;	// not activate menu when hit ALT/F10
			case WM_KEYUP:
				if(emu) {
					bool extended = ((HIWORD(lParam) & 0x100) != 0);
					int code = LOBYTE(wParam);
					if(code == VK_PROCESSKEY) {
						code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
					}
					emu->key_up(code, extended);
				}
				break;
			case WM_SYSKEYUP:
				if(emu) {
					bool extended = ((HIWORD(lParam) & 0x100) != 0);
					int code = LOBYTE(wParam);
					if(code == VK_PROCESSKEY) {
						code = MapVirtualKey(HIWORD(lParam) & 0xff, 3);
					}
					emu->key_up(code, extended);
				}
				return 0;	// not activate menu when hit ALT/F10
			case WM_CHAR:
				if(emu) {
					emu->key_char(LOBYTE(wParam));
				}
				break;
			}
			return CallWindowProc(ButtonOldProc[i], hWnd, iMsg, wParam, lParam);
		}
	}
	return 0;
}

void create_buttons(HWND hWnd)
{
	for(int i = 0; i < MAX_BUTTONS; i++) {
		hButton[i] = CreateWindow(_T("BUTTON"), NULL/*vm_buttons[i].caption*/,
		                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW/*(_tcsstr(vm_buttons[i].caption, _T("\n")) ? BS_MULTILINE : 0)*/,
		                          vm_buttons[i].x, vm_buttons[i].y,
		                          vm_buttons[i].width, vm_buttons[i].height,
		                          hWnd, (HMENU)(ID_BUTTON + i), (HINSTANCE)GetModuleHandle(0), NULL);
#ifdef _M_AMD64
		ButtonOldProc[i] = (WNDPROC)GetWindowLongPtr(hButton[i], GWLP_WNDPROC);
		SetWindowLongPtr(hButton[i], GWLP_WNDPROC, (LONG_PTR)ButtonSubProc);
#else
		ButtonOldProc[i] = (WNDPROC)(LONG_PTR)GetWindowLong(hButton[i], GWL_WNDPROC);
		SetWindowLong(hButton[i], GWL_WNDPROC, (LONG)(LONG_PTR)ButtonSubProc);
#endif
	}
}

void draw_button(HDC hDC, UINT index, UINT pressed)
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
	_TCHAR name[24];

	_stprintf(name, _T("IDI_BITMAP_BUTTON%02d"), index);
#if 1
	// load png from resource
	HRSRC hResource = FindResource(hInstance, name, _T("IMAGE"));
	if(hResource != NULL) {
		const void* pResourceData = LockResource(LoadResource(hInstance, hResource));
		if(pResourceData != NULL) {
			DWORD dwResourceSize = SizeofResource(hInstance, hResource);
			HGLOBAL hResourceBuffer = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
			if(hResourceBuffer != NULL) {
				void* pResourceBuffer = GlobalLock(hResourceBuffer);
				if(pResourceBuffer != NULL) {
					CopyMemory(pResourceBuffer, pResourceData, dwResourceSize);
					IStream* pIStream = NULL;
					if(CreateStreamOnHGlobal(hResourceBuffer, FALSE, &pIStream) == S_OK) {
						Gdiplus::Bitmap *pBitmap = Gdiplus::Bitmap::FromStream(pIStream);
						if(pBitmap != NULL) {
							Gdiplus::Graphics graphics(hDC);
							if(pressed) {
								graphics.DrawImage(pBitmap, 2,  2, pBitmap->GetWidth() - 2, pBitmap->GetHeight() - 2);
							} else {
								graphics.DrawImage(pBitmap, 1, 1);
							}
							delete pBitmap;
						}
					}
				}
				GlobalUnlock(hResourceBuffer);
			}
			GlobalFree(hResourceBuffer);
		}
	}
#else
	// load bitmap from resource
	HDC hmdc = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = LoadBitmap(hInstance, name);
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);
	int w = (int)bmp.bmWidth;
	int h = (int)bmp.bmHeight;
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hmdc, hBitmap);
	if(pressed) {
		StretchBlt(hDC, 2, 2, w - 2, h - 2, hmdc, 0, 0, w, h, SRCCOPY);
	} else {
		BitBlt(hDC, 1, 1, w, h, hmdc, 0, 0, SRCCOPY);
	}
	SelectObject(hmdc, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hmdc);
#endif
}
#endif
#endif

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

static int get_unicode_character(struct android_app *app, int event_type, int key_code, int meta_state) {
    JNIEnv *jni;
    app->activity->vm->AttachCurrentThread(&jni, NULL);

    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    args.name = "NativeThread";
    args.group = NULL;

    jclass KeyEvent = jni->FindClass("android/view/KeyEvent");
    jmethodID constructor = jni->GetMethodID(KeyEvent, "<init>", "(II)V");
    jmethodID getUnicodeChar = jni->GetMethodID(KeyEvent, "getUnicodeChar", "(I)I");

    jobject key_event = jni->NewObject(KeyEvent, constructor, event_type, key_code);
    jint unicode_key = jni->CallIntMethod(key_event, getUnicodeChar, meta_state);

    app->activity->vm->DetachCurrentThread();
    return unicode_key;
}

// 大文字小文字を区別しない比較関数
bool caseInsensitiveCompare(const std::string &a, const std::string &b) {
    std::string lowerA = a;
    std::string lowerB = b;
    std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lowerA < lowerB;
}

void setFileSelectIcon() {
    for(int iconIndex = 0;iconIndex < MAX_FILE_SELECT_ICON;iconIndex++){
        fileSelectIconData[iconIndex].fileSelectType = FILE_SELECT_NONE;
    }
    int index = 0;
    //FD
#ifdef USE_FLOPPY_DISK
    fileSelectIconData[index].fileSelectType = FLOPPY_DISK;
    fileSelectIconData[index].driveNo = 0;
    index++;
    if (USE_FLOPPY_DISK >= 2) {
        fileSelectIconData[index].fileSelectType = FLOPPY_DISK;
        fileSelectIconData[index].driveNo = 1;
        index++;
    }
#endif

#ifdef USE_HARD_DISK
    fileSelectIconData[index].fileSelectType = HARD_DISK;
    fileSelectIconData[index].driveNo = 0;
    index++;
    if (USE_HARD_DISK >= 2) {
        fileSelectIconData[index].fileSelectType = HARD_DISK;
        fileSelectIconData[index].driveNo = 1;
        index++;
    }
#endif

#ifdef USE_QUICK_DISK
    fileSelectIconData[index].fileSelectType = QUICK_DISK;
    fileSelectIconData[index].driveNo = 0;
    index++;
#endif

#ifdef USE_TAPE
    fileSelectIconData[index].fileSelectType = CASETTE_TAPE;
    fileSelectIconData[index].driveNo = 0;
    index++;
#endif

#ifdef USE_CART
    fileSelectIconData[index].fileSelectType = CARTRIDGE;
    fileSelectIconData[index].driveNo=0;
    index++;
    if(USE_CART >=2){
        fileSelectIconData[index].fileSelectType = CARTRIDGE;
        fileSelectIconData[index].driveNo=1;
        index++;
    }
#endif
}

// ----------------------------------------------------------------------------
// android
// ----------------------------------------------------------------------------

const char *jniGetSdcardDownloadPath(struct android_app *state) {
    JNIEnv *jni = NULL;
    state->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass environmentClass = jni->FindClass("android/os/Environment");
    jmethodID getExternalStorageDirectoryMethod = jni->GetStaticMethodID(environmentClass, "getExternalStorageDirectory", "()Ljava/io/File;");
    jobject externalStorageFileObject = jni->CallStaticObjectMethod(environmentClass, getExternalStorageDirectoryMethod);

    jclass fileClass = jni->FindClass("java/io/File");
    jmethodID getPathMethod = jni->GetMethodID(fileClass, "getPath", "()Ljava/lang/String;");
    jstring externalStoragePath = (jstring) jni->CallObjectMethod(externalStorageFileObject, getPathMethod);

    const char *externalPath = jni->GetStringUTFChars(externalStoragePath, NULL);
    const char *downloadPathSuffix = "/Download";
    char *downloadPath = (char *)malloc(strlen(externalPath) + strlen(downloadPathSuffix) + 1); // +1 for the null terminator
    strcpy(downloadPath, externalPath);
    strcat(downloadPath, downloadPathSuffix);

    jni->ReleaseStringUTFChars(externalStoragePath, externalPath);
    state->activity->vm->DetachCurrentThread();

    return downloadPath; // 呼び出し側で free() する必要がある
}

void jniReadIconData(struct android_app *state) {
    JNIEnv *jni = NULL;
    state->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass clazz = jni->GetObjectClass(state->activity->clazz);
    jmethodID jIDLoadBitmap = jni->GetMethodID(clazz, "loadBitmap", "(II)[I");
    int width, height;

    for (int iconType = 0; iconType < 2; iconType++) {
        int iconNumber = 0;
        if (iconType == 0) {
            iconNumber = SYSTEM_ICON_MAX;
        } else {
            iconNumber = FILE_SELECT_TYPE_MAX;
        }

        for (int iconIndex = 0; iconIndex < iconNumber; iconIndex++) {
            LOGI("Load Icon:%d", iconIndex);
            jintArray ja = (jintArray) (jni->CallObjectMethod(state->activity->clazz, jIDLoadBitmap, iconType, iconIndex));
            int jasize = jni->GetArrayLength(ja);

            jint *arr1;
            arr1 = jni->GetIntArrayElements(ja, 0);
            int cnt;
            // 画像の幅を取得
            width = arr1[0];
            // 画像の縦サイズを取得
            height = arr1[1];
            if (iconType == 0) {
                systemIconData[iconIndex].height = height;
                systemIconData[iconIndex].width = width;

                systemIconData[iconIndex].bmpImage = new uint16_t[height * width];
                for (int index = 0; index < jasize - 2; index++) {
                    systemIconData[iconIndex].bmpImage[index] = arr1[index + 2];
                }
            } else {
                mediaIconData[iconIndex].height = height;
                mediaIconData[iconIndex].width = width;

                mediaIconData[iconIndex].bmpImage = new uint16_t[height * width];
                for (int index = 0; index < jasize - 2; index++) {
                    mediaIconData[iconIndex].bmpImage[index] = arr1[index + 2];
                }
            }
            jni->ReleaseIntArrayElements(ja, arr1, 0);
        }
    }
    state->activity->vm->DetachCurrentThread();
}

BitmapData jniCreateBitmapFromString(struct android_app *state, const char *text, int fontSize) {
    JNIEnv *jni = NULL;
    state->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass clazz = jni->GetObjectClass(state->activity->clazz);
    jmethodID jCreateBitmapFromString = jni->GetMethodID(clazz, "createBitmapFromString",
                                                         "(Ljava/lang/String;I)[I");

    int width, height;
    jstring jtext = jni->NewStringUTF(text);
    jintArray ja = (jintArray) (jni->CallObjectMethod(state->activity->clazz,
                                                      jCreateBitmapFromString, jtext,
                                                      fontSize));
    int jasize = jni->GetArrayLength(ja);
    jint *arr1;
    arr1 = jni->GetIntArrayElements(ja, 0);
    // 画像の幅を取得
    width = arr1[0];
    // 画像の縦サイズを取得
    height = arr1[1];

    BitmapData returnBitmapData;
    returnBitmapData.width = width;
    returnBitmapData.height = height;
    returnBitmapData.bmpImage = new uint16_t[width * height];
    for (int index = 0; index < width * height; index++) {
        returnBitmapData.bmpImage[index] = arr1[index + 2];
    }
    jni->ReleaseIntArrayElements(ja, arr1, 0);

    state->activity->vm->DetachCurrentThread();
    return returnBitmapData;
}

static void toggle_soft_keyboard(struct android_app *app) {
    JNIEnv *jni;
    app->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass cls = jni->GetObjectClass(app->activity->clazz);
    jmethodID methodID = jni->GetMethodID(cls,
                                          "getSystemService",
                                          "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring service_name = jni->NewStringUTF("input_method");
    jobject input_service = jni->CallObjectMethod(app->activity->clazz, methodID, service_name);

    jclass input_service_cls = jni->GetObjectClass(input_service);
    methodID = jni->GetMethodID(input_service_cls, "toggleSoftInput", "(II)V");
    jni->CallVoidMethod(input_service, methodID, 0, 0);

    jni->DeleteLocalRef(service_name);

    app->activity->vm->DetachCurrentThread();
}

void checkPermissionsAndInitialize(JNIEnv *env, jobject activity) {
    jclass clazz = env->GetObjectClass(activity);
    jmethodID methodID = env->GetMethodID(clazz, "checkPermissionsAsync", "()V");
    env->CallVoidMethod(activity, methodID);
}

std::vector<uint8_t> getClipboardText(struct android_app *app) {
    JNIEnv *jni;
    app->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass activityClass = jni->GetObjectClass(app->activity->clazz);
    jmethodID getClipboardTextMethodId = jni->GetMethodID(activityClass, "getClipboardTextEncoded", "()[B");

    if (getClipboardTextMethodId == NULL) {
        jni->ExceptionDescribe();
        jni->ExceptionClear();
        app->activity->vm->DetachCurrentThread();
        return std::vector<uint8_t>(); // メソッドIDが見つからない場合は空のベクターを返す
    }

    jbyteArray byteArray = (jbyteArray)jni->CallObjectMethod(app->activity->clazz, getClipboardTextMethodId);
    if (jni->ExceptionCheck()) {
        jni->ExceptionDescribe();
        jni->ExceptionClear();
        jni->DeleteLocalRef(activityClass);
        app->activity->vm->DetachCurrentThread();
        return std::vector<uint8_t>(); // 例外が発生した場合は空のベクターを返す
    }

    // バイト配列を std::vector<uint8_t> に変換
    std::vector<uint8_t> clipboardData;
    if (byteArray != NULL) {
        jsize length = jni->GetArrayLength(byteArray);
        jbyte *bytes = jni->GetByteArrayElements(byteArray, NULL);

        clipboardData.assign(bytes, bytes + length);

        jni->ReleaseByteArrayElements(byteArray, bytes, JNI_ABORT);
        jni->DeleteLocalRef(byteArray);
    }

    jni->DeleteLocalRef(activityClass);
    app->activity->vm->DetachCurrentThread();

    return clipboardData;
}

// ----------------------------------------------------------------------------
// jni export
// ----------------------------------------------------------------------------

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_JavaVM = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_nativeOnPermissionsGranted(JNIEnv *env, jobject obj) {
    // 権限が付与された後の処理をここに実装
    grantedStorage = true;

    // カスタムイベントを送信してメインスレッドに通知
    if (globalAppState != nullptr) {
        ALooper_wake(ALooper_forThread());
    }
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_nativeOnPermissionsDenied(JNIEnv *env, jobject obj) {
    // 権限が拒否された場合の処理をここに実装
}

std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream tokenStream(s);
    std::string token;
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

#include <sys/stat.h>
#include <cstring>

// const char *path の親フォルダを取得し、const char * で返す
const char *get_parent_path(const char *path) {
    size_t len = strlen(path);
    char *local_path = new char[len + 1];
    strcpy(local_path, path);

    for (char *p = local_path + len - 1; p >= local_path; p--) {
        if (*p == '/') {
            *p = '\0';
            break;
        }
    }

    return local_path;
}

// フォルダ存在チェック関数（実装は環境による）
bool check_dir_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return false;  // パスが存在しない
    }
    return (info.st_mode & S_IFDIR);  // ディレクトリかどうかをチェック
}

bool create_dir(const char *path) {
    size_t len = strlen(path);
    char *local_path = new char[len + 1];
    strcpy(local_path, path);

    bool result = true;
    for (char *p = local_path + 1; *p; p++) {  // ルートディレクトリをスキップ
        if (*p == '/') {
            *p = '\0';  // 一時的に null 文字に置き換える
            if (!check_dir_exists(local_path) && mkdir(local_path, 0777) != 0) {
                result = false;  // ディレクトリ作成に失敗
                break;
            }
            *p = '/';  // null 文字を元に戻す
        }
    }

    if (result && !check_dir_exists(local_path) && mkdir(local_path, 0777) != 0) {
        result = false;  // 最終ディレクトリの作成
    }

    delete[] local_path;
    return result;
}


JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_newFileCallback(JNIEnv *env, jobject obj, jstring filename, jstring mediaInfo, jstring addPath) {
    // GetStringUTFCharsを呼び出す前にnullチェックを行う
    if (filename == nullptr || addPath == nullptr) {
        LOGE("Filename or addPath is null");
        return;
    }
    MediaInfo mediaInfoData;
    const char *fileStr = env->GetStringUTFChars(filename, nullptr);
    const char *addPathStr = env->GetStringUTFChars(addPath, nullptr);
    // mediaInfo をカンマ区切りで分割し、各変数に格納、1番目 int drv, 2番目 FileSlectType fileSelectType, 3番目 int floppyType, 4番目 int hd1, 5番目 int hd2, 6番目 int hd3, 7番目 int hd4 値が存在しなければ 0
    int drv = 0;
    FileSelectType fileSelectType = FILE_SELECT_NONE;
    int floppyType = 0;
    int hd1 = 0;
    int hd2 = 0;
    int hd3 = 0;
    int hd4 = 0;
    if (mediaInfo != nullptr) {
        const char *mediaInfoStr = env->GetStringUTFChars(mediaInfo, nullptr);
        std::vector<std::string> mediaInfoList = split(mediaInfoStr, ',');
        if (mediaInfoList.size() >= 2) {
            drv = std::stoi(mediaInfoList[0]);
            fileSelectType = static_cast<FileSelectType>(std::stoi(mediaInfoList[1]));
            if (mediaInfoList.size() >= 3) {
                floppyType = std::stoi(mediaInfoList[2]);
            }
            if (mediaInfoList.size() >= 4) {
                hd1 = std::stoi(mediaInfoList[3]);
            }
            if (mediaInfoList.size() >= 5) {
                hd2 = std::stoi(mediaInfoList[4]);
            }
            if (mediaInfoList.size() >= 6) {
                hd3 = std::stoi(mediaInfoList[5]);
            }
            if (mediaInfoList.size() >= 7) {
                hd4 = std::stoi(mediaInfoList[6]);
            }
        }
        env->ReleaseStringUTFChars(mediaInfo, mediaInfoStr);
    }
    // ファイル名を使った処理を行う
    // 例: ログ出力
    const char *applicationPath = get_application_path();
    char path[_MAX_PATH];
    sprintf(path, "%s%s/%s", applicationPath, addPathStr, fileStr);
    LOGI("Selected file: %s", path);

    if (path[0] != '\0') {
        // pathはファイルの場所なので、path の親フォルダを取得する処理を行う
        const char *parentPath = get_parent_path(path);
        // pathのフォルダの存在をc++のファイル関数ででチェックし、無ければフォルダをc++の関数で作成する
        if (!check_dir_exists(parentPath)) {
            create_dir(parentPath);
        }
#ifdef USE_FLOPPY_DISK
        // ファイル拡張子が大文字小文字を区別せず ".d88" または ".d77" の時か確認する
        if (check_file_extension(path, ".d88") || check_file_extension(path, ".d77")) {
            if (emu->create_blank_floppy_disk(path, floppyType)) {
                // my_tcscpy_sは標準のC++関数ではないため、strcpyを使用
                strcpy(config.initial_floppy_disk_dir, get_parent_dir(path));
                emu->open_floppy_disk(drv, path, 0);
            }
        }
#endif
#ifdef USE_HARD_DISK
        // ファイル拡張子が大文字小文字を区別せず ".hdi" の時か確認する
        if (check_file_extension(path, ".hdi")) {
            if (emu->create_blank_floppy_disk(path, fileSelectType)) {
                // my_tcscpy_sは標準のC++関数ではないため、strcpyを使用
                strcpy(config.initial_floppy_disk_dir, get_parent_dir(path));
                emu->open_floppy_disk(drv, path, 0);
            }
            if(emu->create_blank_hard_disk(path, hd1, hd2, hd3, hd4)) {
                // UPDATE_HISTORY(path, config.recent_hard_disk_path[drv]);
                // strcpy で右のコードを置きかえる my_tcscpy_s(config.initial_hard_disk_dir, _MAX_PATH, get_parent_dir(path));
                strcpy(config.initial_hard_disk_dir, get_parent_dir(path));
                emu->open_hard_disk(drv, path);
            }
        }
#endif
#ifdef USE_TAPE
        // ファイル拡張子が大文字小文字を区別せず ".cas" または ".cmt" または ".mzt" の時か確認する
        if (check_file_extension(path, ".cas") || check_file_extension(path, ".cmt") || check_file_extension(path, ".mzt")) {
            LOGI("Selected file: %s", path);
            LOGI("Drive: %d", drv);
            if (emu->is_tape_inserted(drv)) {
                emu->close_tape(drv);
            }
            if (extendMenuCmtPlay) {
                LOGI("Play tape: %s", path);
                emu->play_tape(drv, path);
            } else {
                LOGI("Rec tape: %s", path);
                // 指定パスにファイルサイズ0のファイルを作成する
                FILE *fp = fopen(path, "wb");
                if (fp != nullptr) {
                    fclose(fp);
                }
                emu->rec_tape(drv, path);
            }
        }
#endif
    }
    // メモリ解放
    env->ReleaseStringUTFChars(filename, fileStr);
    env->ReleaseStringUTFChars(addPath, addPathStr);
}

// 階層メニュー選択後に、階層メニュー定義情報の文字列が送られる
// 1: ノードID, 2: キャプション, 3: ノードタイプ, 4: 戻り値, 5: 親ID の順で文字列をセミコロンで区切った情報が送られる
// 前階層に戻る場合は親階層のIDのみが送られる
// 5: 親ID のみが送られる場合は、前階層に戻る
JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_extendMenuCallback(JNIEnv *env, jobject thiz, jstring extendMenu)
{
    const char *extendMenuString = env->GetStringUTFChars(extendMenu, 0);

    extendMenuDisplay = false;
    // LOG 出力する
    LOGI("extendMenuCallback %s", extendMenuString);
    // 先ずは空白文字（null又は空文字）なら何もしない
    if (extendMenuString == nullptr || strlen(extendMenuString) == 0) {
        // 何もしない
        notifyMenuNode = MenuNode::emptyNode();
        LOGI("Empty extendMenuString");
    }
        // else if で、親IDのみか判断する、親IDとは1トークンで、数字のみの文字列
    else if (std::all_of(extendMenuString, extendMenuString + strlen(extendMenuString), ::isdigit)) {
        // 親IDのみの場合は、int に変換する、変換に失敗した場合は何もしない
        int parentId = 0;
        try {
            parentId = std::stoi(extendMenuString);
            // そこから更に親IDを取得し、通知メニューIDにセットして終了する
            notifyMenuNode = menu->getNode(menu->getParentId(parentId));
            LOGI("Parent Node: %s", menu->getNodeString(notifyMenuNode).c_str());
        } catch (std::invalid_argument &e) {
            // 何もしない
            notifyMenuNode = MenuNode::emptyNode();
            LOGI("Invalid extendMenuString: %s", extendMenuString);
        }
    }
        // else if で、ノード情報か判断する
    else if (menu->isValidExtendMenuString(extendMenuString)) {
        // ノード情報を取得する
        notifyMenuNode = menu->getNodeFromExtendMenuString(extendMenuString);
        LOGI("Notify node: %s", menu->getNodeString(notifyMenuNode).c_str());
    }
        // else 何もないので何もしない
    else {
        notifyMenuNode = MenuNode::emptyNode();
        LOGI("Nothing extendMenuString: %s", extendMenuString);
    }
    // メモリ解放
    env->ReleaseStringUTFChars(extendMenu, extendMenuString);
    LOGI("extendMenuCallback end");
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_fileSelectCallback(JNIEnv *env, jobject thiz, jint id) {
    LOGI("fileSelectCallback %d", id);
    if (fileList.size() < id) {
        return;
    }
    if (id == 0) { //EJECT
        switch (fileSelectIconData[selectingIconIndex].fileSelectType) {
#ifdef USE_FLOPPY_DISK
            case FLOPPY_DISK:
                emu->close_floppy_disk(fileSelectIconData[selectingIconIndex].driveNo);
                break;
#endif
#ifdef USE_HARD_DISK
            case HARD_DISK:
                emu->close_hard_disk(fileSelectIconData[selectingIconIndex].driveNo);
                break;
#endif
#ifdef USE_TAPE
            case CASETTE_TAPE:
                emu->close_tape(0);
                break;
#endif
#ifdef USE_CART
                case CARTRIDGE:
                    emu->close_cart(fileSelectIconData[selectingIconIndex].driveNo);
                    break;
#endif
        }
    } else {

        std::string filePath = fileList.at(id);

        switch (fileSelectIconData[selectingIconIndex].fileSelectType) {
#ifdef USE_FLOPPY_DISK
            case FLOPPY_DISK:
                emu->open_floppy_disk(
                        fileSelectIconData[selectingIconIndex].driveNo,
                        filePath.c_str(),
                        0);
                LOGI("D88 bankNo:%d",
                     emu->d88_file[fileSelectIconData[selectingIconIndex].driveNo].bank_num);
                if (emu->d88_file[fileSelectIconData[selectingIconIndex].driveNo].bank_num > 1) {
                    needSelectDiskBank = true;
                    selectDiskDrive = fileSelectIconData[selectingIconIndex].driveNo;
                    //selectD88Bank(selectDiskDrive); //メインループから呼び出す
                }
                break;
#endif
#ifdef USE_HARD_DISK
            case HARD_DISK:
                emu->open_hard_disk(
                        fileSelectIconData[selectingIconIndex].driveNo,
                        filePath.c_str());
                break;
#endif
#ifdef USE_TAPE
            case CASETTE_TAPE:
                if (emu->is_tape_inserted(0)) {
                    emu->close_tape(0);
                }
#if defined(_EXTEND_MENU)
                if (extendMenuCmtPlay) {
                    emu->play_tape(0, filePath.c_str());
                } else {
                    emu->rec_tape(0, filePath.c_str());
                }
#else
                emu->play_tape(0, filePath.c_str());
#endif
                break;
#endif
#ifdef USE_QUICK_DISK
                case QUICK_DISK:
                if (emu->is_quick_disk_inserted(0)) {
                    emu->close_quick_disk(0);
                }
                emu->open_quick_disk(0, filePath.c_str());
                break;
#endif
#ifdef USE_CART
                case CARTRIDGE:
                emu->open_cart(fileSelectIconData[selectingIconIndex].driveNo , filePath.c_str());
                break;
#endif
        }
    }
    fileList.clear();
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_bankSelectCallback(JNIEnv *env, jobject thiz, jint id) {

    LOGI("bankSelectCallback %d", id);
    if (id < 0) {
        return;
    }
#ifdef USE_FLOPPY_DISK
    if (emu->d88_file[fileSelectIconData[selectingIconIndex].driveNo].cur_bank != id) {
        emu->open_floppy_disk(fileSelectIconData[selectingIconIndex].driveNo,
                              emu->d88_file[fileSelectIconData[selectingIconIndex].driveNo].path,
                              id);
        emu->d88_file[fileSelectIconData[selectingIconIndex].driveNo].cur_bank = id;
    }
#endif
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_bootSelectCallback(JNIEnv *env, jobject thiz, jint id) {
    if (id < 0) {
        return;
    }
#ifdef _PC8801MA
    config.boot_mode = id;
    if(emu) {
        emu->update_config();
    }
#endif
#ifdef _X1TURBO_FEATURE
    config.monitor_type = id;
    if(emu) {
        emu->update_config();
    }
#endif

    resetFlag = true;
}

JNIEXPORT void JNICALL
Java_jp_matrix_shikarunochi_emulator_EmulatorActivity_exitSelectCallback(JNIEnv *env, jobject thiz, jint id) {
    if (id < 0) {
        return;
    }
    // 強制排出
    LOGI("exitSelectCallback");
    all_eject();
    //TODO:free?
    exit(0);
}

} //extern"C"

// ----------------------------------------------------------------------------
// end of file
// ----------------------------------------------------------------------------