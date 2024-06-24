package jp.matrix.shikarunochi.emulator;

import static android.content.ContentValues.TAG;

import android.Manifest;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.NativeActivity;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.util.Log;
import android.util.TypedValue;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class EmulatorActivity extends NativeActivity {

    private static final int PERMISSIONS_REQUEST_CODE = 1;
    private static final int REQUEST_CODE_FILE_PICKER = 123;
    private static final int REQUEST_ENABLE_BT = 1;
    private volatile boolean permissionsGranted = false;

    private static final String[] PERMISSIONS_REQUIRED = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.RECEIVE_BOOT_COMPLETED
    };

    // APIレベル31以上で必要な追加の権限
    private static final String[] ADDITIONAL_PERMISSIONS_API_31 = {
            "android.permission.BLUETOOTH_CONNECT",
            "android.permission.BLUETOOTH_SCAN",
            "android.permission.MIDI"
    };


    private DeviceManager deviceManager;
    private MidiManagerActivity midiManagerActivity;

    private native void sendMouseClickEvent(int action, float x, float y, int pointerCount, int buttonState);
    private native void sendMouseMovementEvent(float x, float y);

    private native void sendJoypadInputToNative(int deviceId, int index, float axisX, float axisY, int keyCode, int action);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            List<String> ungrantedPermissions = getUngrantedPermissions();
            if (!ungrantedPermissions.isEmpty()) {
                requestPermissions(ungrantedPermissions.toArray(new String[0]), PERMISSIONS_REQUEST_CODE);
            } else {
                initializeApp();
            }
        } else {
            initializeApp();
        }

        // MidiManagerActivityを初期化
        midiManagerActivity = MidiManagerActivity.getInstance(this);

        // ジョイパッドを管理するデバイスマネージャを初期化
        deviceManager = new DeviceManager(this);
        deviceManager.registerInputDeviceListener();

        // Bluetoothを有効にするIntentを発行
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter != null && !bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        setContentView(R.layout.activity_main);
        View mouseView = findViewById(R.id.mouseView);
        mouseView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                int action = event.getActionMasked();
                int buttonState = event.getButtonState();
                int pointerCount = event.getPointerCount();
                float x = event.getX();
                float y = event.getY();
                sendMouseClickEvent(action, x, y, pointerCount, buttonState);
                return true;
            }
        });

        System.loadLibrary("native-activity");
        enableImmersiveMode();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            sendMouseMovementEvent(event.getX(), event.getY());
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (event.isFromSource(InputDevice.SOURCE_MOUSE)) {
            float deltaX = event.getAxisValue(MotionEvent.AXIS_X);
            float deltaY = event.getAxisValue(MotionEvent.AXIS_Y);
            // マウスカーソルの移動を検知してNDKに送信
            sendMouseMovementEvent(deltaX, deltaY);
        }
        else if (event.isFromSource(InputDevice.SOURCE_JOYSTICK) ||
                event.isFromSource(InputDevice.SOURCE_GAMEPAD)) {
            // ジョイパッドからの入力を処理
            int deviceId = event.getDeviceId();
            float axisX = event.getAxisValue(MotionEvent.AXIS_X);
            float axisY = event.getAxisValue(MotionEvent.AXIS_Y);
            // ここでジョイパッドの入力をログまたは処理
            handleJoystickInput(deviceId, axisX, axisY, event);
        }
        return super.onGenericMotionEvent(event);
    }

    private void handleJoystickInput(int deviceId, float axisX, float axisY, MotionEvent event) {
        sendJoypadInputToNative(deviceId, 1, axisX, axisY, 0, 0);

        // 他のボタンやアナログスティックの状態もここでチェック
        float axisZ = event.getAxisValue(MotionEvent.AXIS_Z);
        float axisRZ = event.getAxisValue(MotionEvent.AXIS_RZ);
        // その他の必要な軸のデータを追加
        sendJoypadInputToNative(deviceId, 2, axisZ, axisRZ, 0, 0);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (event.isFromSource(InputDevice.SOURCE_JOYSTICK) || event.isFromSource(InputDevice.SOURCE_GAMEPAD)) {
            sendJoypadInputToNative(event.getDeviceId(), 0, 0, 0, keyCode, 1);
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (event.isFromSource(InputDevice.SOURCE_JOYSTICK) || event.isFromSource(InputDevice.SOURCE_GAMEPAD)) {
            sendJoypadInputToNative(event.getDeviceId(), 0, 0, 0, keyCode, 0);
        }
        return super.onKeyUp(keyCode, event);
    }

    public void enableImmersiveMode() {
        final View decorView = getWindow().getDecorView();
        decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                    // システムUIが表示されたことを検出
                    decorView.setSystemUiVisibility(
                            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    );
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        enableImmersiveMode(); // アプリが前面に戻るたびにイマーシブモードを再適用
    }

    // NDKから呼び出されるメソッド
    public void doFinish() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                finishAffinity(); // API level 16+
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    finishAndRemoveTask(); // API level 21+
                }
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.i(TAG, "onRequestPermissionsResult called");
        if (requestCode == PERMISSIONS_REQUEST_CODE) {
            List<String> ungrantedPermissions = getUngrantedPermissions();
            if (ungrantedPermissions.isEmpty()) {
                Log.i(TAG, "All permissions granted");
                onPermissionsGranted();
            } else {
                Log.i(TAG, "Some permissions denied: " + ungrantedPermissions.toString());
                onPermissionsDenied();
            }
        }
    }

    // ネイティブメソッドの宣言
    private native void nativeOnPermissionsGranted();
    private native void nativeOnPermissionsDenied();

    private List<String> getUngrantedPermissions() {
        List<String> ungrantedPermissions = new ArrayList<>();

        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.READ_EXTERNAL_STORAGE);
        }
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }
        if (checkSelfPermission(Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.BLUETOOTH);
        }
        if (checkSelfPermission(Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.BLUETOOTH_ADMIN);
        }
        if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }
        if (checkSelfPermission(Manifest.permission.RECEIVE_BOOT_COMPLETED) != PackageManager.PERMISSION_GRANTED) {
            ungrantedPermissions.add(Manifest.permission.RECEIVE_BOOT_COMPLETED);
        }

        if (Build.VERSION.SDK_INT >= 31) {
            if (checkSelfPermission("android.permission.BLUETOOTH_CONNECT") != PackageManager.PERMISSION_GRANTED) {
                ungrantedPermissions.add("android.permission.BLUETOOTH_CONNECT");
            }
            if (checkSelfPermission("android.permission.BLUETOOTH_SCAN") != PackageManager.PERMISSION_GRANTED) {
                ungrantedPermissions.add("android.permission.BLUETOOTH_SCAN");
            }
            if (checkSelfPermission("android.permission.BLUETOOTH_ADVERTISE") != PackageManager.PERMISSION_GRANTED) {
                ungrantedPermissions.add("android.permission.BLUETOOTH_ADVERTISE");
            }
        }

        return ungrantedPermissions;
    }

    private void onPermissionsGranted() {
        Log.i(TAG, "Permissions granted");
        // JNIに権限付与成功を通知
        nativeOnPermissionsGranted();
    }

    private void onPermissionsDenied() {
        // JNIに権限拒否を通知
        nativeOnPermissionsDenied();
    }

    public void checkPermissionsAsync() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "checkPermissionsAsync called");
                List<String> ungrantedPermissions = getUngrantedPermissions();
                if (ungrantedPermissions.isEmpty()) {
                    Log.i(TAG, "Permissions already granted");
                    onPermissionsGranted();
                } else {
                    requestPermissions(ungrantedPermissions.toArray(new String[0]), PERMISSIONS_REQUEST_CODE);
                }
            }
        });
    }

    public String getMidiDeviceInfo(int index) {
        return MidiManagerActivity.getInstance(this).getMidiDeviceInfo(index);
    }

    public void sendMidiMessage(byte[] message) {
        MidiManagerActivity.getInstance(this).sendMidiMessage(message);
    }

    public void openFilePickerForImages() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.setType("*/*");  // この設定は後の EXTRA_MIME_TYPES により上書きされます。
        String[] mimeTypes = {"image/jpeg", "image/png"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        startActivityForResult(intent, REQUEST_CODE_FILE_PICKER);
    }

    public void updateMidiDevice() {
        Log.i(TAG, "updateMidiDevice called");

        // アプリケーションの初期化時にUSBデバイスをチェック
        Log.i(TAG, "checkForUsbDevices called");
        midiManagerActivity.checkForUsbDevices();

        // アプリケーションの初期化時にBLEスキャンを開始
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
            Log.e(TAG, "Bluetooth is not enabled or not available");
            return;
        }
        Log.i(TAG, "startBleScan called");
        midiManagerActivity.startBleScan();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                // ユーザーが Bluetooth を有効にした場合の処理
                midiManagerActivity.checkForUsbDevices();
                midiManagerActivity.startBleScan();
            } else {
                // ユーザーが Bluetooth を有効にしなかった場合の処理
                Log.e(TAG, "Bluetooth is not enabled");
            }
        }
        if (requestCode == REQUEST_CODE_FILE_PICKER && resultCode == Activity.RESULT_OK) {
            if (data != null) {
                Uri uri = data.getData();
                if (uri != null) {
                    // 選択されたファイルに対する処理を行う
                    handleSelectedFile(uri);
                }
            }
        }
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                // ユーザーが Bluetooth を有効にした場合の処理
                // 例えば、Bluetooth デバイスのスキャンを開始するなど
            } else {
                // ユーザーが Bluetooth を有効にしなかった場合の処理
                // 例えば、エラーメッセージを表示するなど
            }
        }
    }

    private Bitmap getBitmapFromUri(Uri uri) throws IOException {
        ParcelFileDescriptor parcelFileDescriptor = getContentResolver().openFileDescriptor(uri, "r");
        FileDescriptor fileDescriptor = parcelFileDescriptor.getFileDescriptor();
        Bitmap image = BitmapFactory.decodeFileDescriptor(fileDescriptor);
        parcelFileDescriptor.close();
        return image;
    }

    private void handleSelectedFile(Uri fileUri) {
        try {
            Bitmap bitmap = getBitmapFromUri(fileUri);
            if (bitmap != null) {
                int width = bitmap.getWidth();
                int height = bitmap.getHeight();
                int[] pixels = new int[width * height];
                bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
                // NDKにピクセルデータを送る
                sendImageToNative(pixels, width, height);
            }
        } catch (IOException e) {
            Log.e(TAG, "File select error", e);
        }
    }

    private native void sendImageToNative(int[] pixels, int width, int height);

    // URIから実際のファイルパスを取得するメソッド
    private String getRealPathFromUri(Uri uri) {
        String filePath = "";
        if (DocumentsContract.isDocumentUri(this, uri)) {
            String documentId = DocumentsContract.getDocumentId(uri);
            if ("com.android.externalstorage.documents".equals(uri.getAuthority())) {
                String[] split = documentId.split(":");
                if (split.length >= 2) {
                    String type = split[0];
                    if ("primary".equalsIgnoreCase(type)) {
                        filePath = getExternalFilesDir(null) + "/" + split[1];
                    }
                }
            } else if ("com.android.providers.downloads.documents".equals(uri.getAuthority())) {
                Uri contentUri = ContentUris.withAppendedId(
                        Uri.parse("content://downloads/public_downloads"), Long.valueOf(documentId));
                filePath = getDataColumn(this, contentUri, null, null);
            }
        } else if ("content".equalsIgnoreCase(uri.getScheme())) {
            filePath = getDataColumn(this, uri, null, null);
        }
        return filePath;
    }

    // URIからデータ列を取得するメソッド
    private String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs) {
        String column = "_data";
        String[] projection = {column};
        try (Cursor cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int columnIndex = cursor.getColumnIndexOrThrow(column);
                return cursor.getString(columnIndex);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    // C++ 側からアクセス可能なメソッド
    public boolean arePermissionsGranted() {
        return permissionsGranted;
    }

    private void initializeApp() {
        // 権限がある場合のアプリの初期化処理
        System.loadLibrary("native-activity");
        // 権限がある場合のアプリの初期化処理
        midiManagerActivity = MidiManagerActivity.getInstance(this);
    }

    @Override
    protected void onDestroy(){
        super.onDestroy();
        // MidiManagerActivityのクローズ処理を呼び出す
        if (midiManagerActivity != null) {
            midiManagerActivity.close();
        }
        deviceManager.unregisterInputDeviceListener();
    }

    // ネイティブメソッドの宣言
    public native void newFileCallback(String filename, String mediaInfo, String addPath);

    // コールバック用のインターフェースを定義
    interface NewFileCallback {
        void onNewFileSelected(String filename, String mediaInfo, String addPath);
    }

    // 接続されたジョイパッドの軸情報及びボタン情報を取得し返す
    public float[] getJoyPadInformation() {
        int[] deviceIds = InputDevice.getDeviceIds();
        // ジョイパッドごとに32個の情報を格納するために配列のサイズを調整
        float[] values = new float[deviceIds.length * 32];
        // values を 0 で埋める
        Arrays.fill(values, 0);
        int deviceCount = 0;

        for (int deviceId : deviceIds) {
            InputDevice device = InputDevice.getDevice(deviceId);
            if ((device.getSources() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE ||
               ((device.getSources() & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD && device.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC) ||
                (device.getSources() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                continue;
            }
            if ((device.getSources() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
                    (device.getSources() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {
                // デバイス情報の取得
                String deviceName = device.getName();
                String manufacturer = device.getDescriptor();
                int productId = device.getProductId();
                int vendorId = device.getVendorId();

                // 軸情報とボタン情報の取得
                List<InputDevice.MotionRange> ranges = device.getMotionRanges();
                int buttonCount = 0;
                int rangeIndex = 0;
                for (InputDevice.MotionRange range : ranges) {
                    if ((range.getSource() & InputDevice.SOURCE_CLASS_BUTTON) == InputDevice.SOURCE_CLASS_BUTTON) {
                        buttonCount++;
                    } else if ((range.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
                            (range.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {
                        if (rangeIndex < 10) { // 最大10軸までの情報を保持
                            values[deviceCount * 32 + rangeIndex * 3] = range.getAxis();
                            values[deviceCount * 32 + rangeIndex * 3 + 1] = range.getMin();
                            values[deviceCount * 32 + rangeIndex * 3 + 2] = range.getMax();
                            rangeIndex++;
                        }
                    }
                }
                // デバイスごとの最後の要素にボタンの数とデバイスIDを格納
                values[deviceCount * 31 + 30] = buttonCount;
                values[deviceCount * 31 + 31] = deviceId;

                // デバイス情報のログ出力
                Log.i("DeviceInfo", "Device Name: " + deviceName);
                Log.i("DeviceInfo", "Device Id: " + deviceId);
                Log.i("DeviceInfo", "Manufacturer: " + manufacturer);
                Log.i("DeviceInfo", "Product ID: " + productId);
                Log.i("DeviceInfo", "Vendor ID: " + vendorId);
                Log.i("DeviceInfo", "Button Count: " + buttonCount);
                Log.i("DeviceInfo", "Source:" +
                        ((device.getSources() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ? " Joystick" : "") +
                        ((device.getSources() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ? " Gamepad" : "") +
                        ((device.getSources() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD ? " D-Pad" : "") +
                        ((device.getSources() & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD ? " Keyboard" : "") +
                        ((device.getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN ? " Touchscreen" : "") +
                        ((device.getSources() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE ? " Mouse" : "") +
                        ((device.getSources() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD ? " TouchPad" : ""));
                Log.i("DeviceInfo", "KeyboardType:" +
                        ((device.getKeyboardType() & InputDevice.KEYBOARD_TYPE_ALPHABETIC) == InputDevice.KEYBOARD_TYPE_ALPHABETIC ? " Alphabetic" : "") +
                        ((device.getKeyboardType() & InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC) == InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC ? " NonAlphabetic" : ""));

                deviceCount++;
            }
        }
        return values;
    }

    // NDK から呼び出せるクリップボード取得メソッド
    public String getClipboardText() {
        final String[] result = new String[1];

        // UI スレッドで実行するための Handler
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                ClipboardManager clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                if (clipboardManager.hasPrimaryClip() && clipboardManager.getPrimaryClip().getItemCount() > 0) {
                    CharSequence charSequence = clipboardManager.getPrimaryClip().getItemAt(0).getText();
                    result[0] = charSequence == null ? "" : charSequence.toString();
                } else {
                    result[0] = "";
                }
            }
        });

        // 非同期処理完了を待機
        try {
            Thread.sleep(100);  // 実際のアプリケーションでは、適切な同期処理を実装する必要がある
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return result[0];
    }

    public boolean isSoftKeyboardShown() {
        Rect r = new Rect();
        View rootview = this.getWindow().getDecorView(); // root view
        rootview.getWindowVisibleDisplayFrame(r);
        int screenHeight = rootview.getRootView().getHeight();

        // ステータスバーの高さを取得
        int statusBarHeight = 0;
        int resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android");
        if (resourceId > 0) {
            statusBarHeight = getResources().getDimensionPixelSize(resourceId);
        }

        // ナビゲーションバーの高さを取得
        int navigationBarHeight = 0;
        resourceId = getResources().getIdentifier("navigation_bar_height", "dimen", "android");
        if (resourceId > 0) {
            navigationBarHeight = getResources().getDimensionPixelSize(resourceId);
        }

        int keypadHeight = screenHeight - r.bottom - statusBarHeight - navigationBarHeight;
        return keypadHeight > screenHeight * 0.15; // キーボードが画面の15%以上を占めていれば表示中と判断
    }

    public byte[] getClipboardTextEncoded() {
        final byte[][] result = new byte[1][];

        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                ClipboardManager clipboardManager = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                if (clipboardManager.hasPrimaryClip() && clipboardManager.getPrimaryClip().getItemCount() > 0) {
                    CharSequence charSequence = clipboardManager.getPrimaryClip().getItemAt(0).getText();
                    if (charSequence != null) {
                        String text = charSequence.toString();
                        // 改行コードを CR に統一
                        text = text.replaceAll("\r\n|\n", "\r");
                        try {
                            result[0] = text.getBytes("Shift_JIS");
                        } catch (UnsupportedEncodingException e) {
                            try {
                                // Shift_JIS に変換できない場合は UTF-8 でエンコードする
                                result[0] = text.getBytes("UTF-8");
                            } catch (UnsupportedEncodingException ex) {
                                // UTF-8 での変換にも失敗した場合の処理
                                result[0] = null;
                            }
                        }
                    }
                }
            }
        });

        try {
            Thread.sleep(100); // 非同期処理完了を待機
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return result[0];
    }

    // NDKから呼び出すためのメソッド
    public void showNewFileDialog(String message, String itemList, String extension, String mediaInfo, String addPath) {
        showNewFileDialogExecute(message, itemList, extension, mediaInfo, addPath, new NewFileCallback() {
            @Override
            public void onNewFileSelected(String filename, String mediaInfo, String addPath) {
                newFileCallback(filename, mediaInfo, addPath);
            }
        });
    }

    // ファイル選択ダイアログを表示する
    public void showNewFileDialogExecute(final String message, final String itemList, final String extension, final String mediaInfo, final String addPath, final NewFileCallback callback) {
        final Activity activity = this;
        activity.runOnUiThread(new Runnable() {
            final EditText input = new EditText(activity);
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                builder.setTitle(message);

                input.setHint("ファイル名を入力してください（" + extension + "ファイル）");
                input.setSingleLine(true);
                builder.setView(input);

                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        validateAndProcessInput((AlertDialog) dialog);
                    }
                });

                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                        if (callback != null) {
                            callback.onNewFileSelected("", "", "");
                        }
                    }
                });

                final AlertDialog dialog = builder.create();

                input.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                        if (actionId == EditorInfo.IME_ACTION_DONE ||
                                (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                            validateAndProcessInput(dialog);
                            return true;
                        }
                        return false;
                    }
                });

                dialog.show();
            }

            private void validateAndProcessInput(AlertDialog dialog) {
                String enteredText = input.getText().toString().trim();
                String fullFilename = enteredText.endsWith(extension) ? enteredText : enteredText + extension;

                List<String> existingFiles = Arrays.asList(itemList.toLowerCase().split(";"));
                if (!enteredText.isEmpty()) {
                    if (existingFiles.contains(fullFilename.toLowerCase())) {
                        Toast.makeText(activity, "ファイル名が重複しています。", Toast.LENGTH_SHORT).show();
                    } else {
                        dialog.dismiss();
                        if (callback != null) {
                            callback.onNewFileSelected(fullFilename, mediaInfo, addPath);
                        }
                    }
                }
            }
        });
    }

    //https://stackoverflow.com/questions/11730001/create-a-message-dialog-in-android-via-ndk-callback
    public int showAlert(final String message, final String itemList, boolean model, final int selectMode) {
        final AtomicInteger buttonId = new AtomicInteger();
        buttonId.set(-1);
        final long[] lastClickTime = {0}; // Shared last click time for all buttons
        final AlertDialog[] finalDialog = new AlertDialog[1]; // 配列のサイズを1に設定

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(EmulatorActivity.this);
                View dialogView = getLayoutInflater().inflate(R.layout.dialog_layout, null);
                builder.setView(dialogView);

                TextView title = dialogView.findViewById(R.id.dialog_title);
                title.setText(message);

                LinearLayout buttonLayout = dialogView.findViewById(R.id.button_layout);
                if (!itemList.isEmpty()) {
                    String[] items = itemList.split(";");
                    for (int i = 0; i < items.length; i++) {
                        Button button = new Button(EmulatorActivity.this);
                        button.setText(items[i]);
                        button.setBackgroundColor(Color.argb(255, 200, 255, 255)); // ARGBで白に近い青
                        final int index = i;
                        button.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                long clickTime = System.currentTimeMillis();
                                if (clickTime - lastClickTime[0] < 500) { // Double click
                                    buttonId.set(index);
                                    fileSelectCallback(index, selectMode);
                                    finalDialog[0].dismiss();
                                }
                                lastClickTime[0] = clickTime;
                            }
                        });
                        // マージンを設定
                        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                                LinearLayout.LayoutParams.MATCH_PARENT,
                                LinearLayout.LayoutParams.WRAP_CONTENT
                        );
                        params.setMargins(1, 3, 1, 3); // left, top, right, bottom
                        button.setLayoutParams(params);
                        buttonLayout.addView(button);
                    }
                }

                Button cancelButton = dialogView.findViewById(R.id.cancel_button);
                cancelButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        fileSelectCallback(-1, selectMode);
                        finalDialog[0].dismiss();
                    }
                });

                AlertDialog dialog = builder.create();
                finalDialog[0] = dialog;
                dialog.show();
            }
        });
        return buttonId.get(); // 選択されたボタンのIDを返す
    }

    private void fileSelectCallback(int id, int selectMode) {
        switch (selectMode) {
            case 0: fileSelectCallback(id); break;
            case 1: bankSelectCallback(id); break;
            case 2: bootSelectCallback(id); break;
            case 3: exitSelectCallback(id); break;
        }
    }

    public int showExtendMenu(final String title, final String extendMenu) {
        final AtomicInteger buttonId = new AtomicInteger();
        final String[] nodes;
        buttonId.set(-1);
        if (!extendMenu.isEmpty()) {
            nodes = extendMenu.split(",");
        } else {
            return buttonId.get();
        }

        this.runOnUiThread(new Runnable() {
            public void run() {
                final Dialog dialog = new Dialog(EmulatorActivity.this);
                dialog.setTitle(title);
                dialog.setContentView(R.layout.custom_dialog_layout); // 事前に定義したカスタムレイアウトを使用

                LinearLayout layout = dialog.findViewById(R.id.custom_dialog_layout);
                for (int i = 0; i < nodes.length; i++) {
                    String[] node = nodes[i].split(";");
                    if (node.length < 3) {
                        continue;
                    }
                    Button button = new Button(EmulatorActivity.this);
                    button.setText(node[1]);
                    // nodes[2] が "0" の時はフォルダ、"1" の時はファイル
                    //  フォルダの時はボタンの色を薄い黄色、ファイルの時は薄い青色にする
                    // 白に近い黄色の背景色を設定
                    // node 配列の数が足りない場合、可能なら配列の中の情報を logcat ダンプして continue する
                    if (node.length >= 8 && node[7] != null && !node[7].isEmpty() && new File(node[7]).exists()) {
                        setButtonBackground(button, dialog, node[7]); // Load and set the image background if it exists
                        button.setTextColor(Color.WHITE);  // テキストの色を白に設定
                        button.setShadowLayer(12, 4, 4, Color.BLACK);  // 黒い影をテキストに追加
                        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
                    } else {
                        if (node[2].equals("0")) {
                            button.setBackgroundColor(Color.argb(255, 255, 255, 200)); // ARGBで白に近い黄色
                        } else {
                            if (node[5].equals("1")) {
                                button.setBackgroundColor(Color.argb(255, 150, 150, 255)); // ARGBで青
                            } else {
                                button.setBackgroundColor(Color.argb(255, 200, 255, 255)); // ARGBで白に近い青
                            }
                        }
                        // ボタン押下許可
                        button.setEnabled(node[6].equals("1"));
                        if (node[6].equals("0")) {
                            button.setBackgroundColor(Color.argb(255, 200, 200, 200)); // 灰色
                        }
                        // テキスト色を設定（ここでは黒を例としています）
                        button.setTextColor(Color.BLACK);
                    }
                    final int index = i;
                    button.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            buttonId.set(index);
                            Log.i("EmulatorActivity", "button clicked: " + nodes[index]);
                            extendMenuCallback(nodes[buttonId.get()]);
                            dialog.dismiss();
                        }
                    });
                    layout.addView(button);
                    // ボタンの下に少しマージンを空ける
                    View margin = new View(EmulatorActivity.this);
                    margin.setMinimumHeight(10);
                    layout.addView(margin);
                }

                Button cancelButton = dialog.findViewById(R.id.cancelButton);
                cancelButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Log.i("EmulatorActivity", "cancelButton clicked");
                        extendMenuCallback("");
                        dialog.dismiss();
                    }
                });

                Button backButton = dialog.findViewById(R.id.backButton);
                backButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (nodes.length > 0) {
                            String[] node = nodes[0].split(";");
                            Log.i("EmulatorActivity", "backButton clicked: " + node[4]);
                            extendMenuCallback(node[4]); // 親IDを渡す
                        } else {
                            Log.i("EmulatorActivity", "backButton clicked: empty");
                            extendMenuCallback("");
                        }
                        dialog.dismiss();
                    }
                });

                setupNavigationButtons(dialog, nodes); // Handles back and cancel buttons
                dialog.setCancelable(false);
                dialog.show();
            }
        });

        Log.i("EmulatorActivity", "showExtendMenu end");
        return buttonId.get();
    }

    private Bitmap decodeCustomImageFile(String imagePath) {
        File file = new File(imagePath);
        if (!file.exists()) return null;

        try (FileInputStream fis = new FileInputStream(file)) {
            // ヘッダから画像サイズを読み取る
            byte[] header = new byte[4];
            if (fis.read(header) != header.length) {
                return null; // ヘッダが完全に読み込めなかった場合
            }

            // リトルエンディアン形式での読み取り
            int width = ((header[1] & 0xFF) << 8) | (header[0] & 0xFF);
            int height = ((header[3] & 0xFF) << 8) | (header[2] & 0xFF);

            // 画像データを読み込む
            byte[] pixelData = new byte[width * height * 2];
            if (fis.read(pixelData) != pixelData.length) {
                return null; // ピクセルデータが完全に読み込めなかった場合
            }

            int[] colors = new int[width * height];
            // RGB565からARGB8888へ変換し、同時に画像を上下反転
            for (int y = 0; y < height; y++) {
                int invY = height - 1 - y;  // 上下反転のためのY座標
                for (int x = 0; x < width; x++) {
                    int pixelIndex = 2 * (x + invY * width);
                    int rgb565 = ((pixelData[pixelIndex + 1] & 0xFF) << 8) | (pixelData[pixelIndex] & 0xFF);
                    int red = ((rgb565 >> 11) & 0x1F) << 3;
                    int green = ((rgb565 >> 5) & 0x3F) << 2;
                    int blue = (rgb565 & 0x1F) << 3;
                    colors[x + y * width] = 0xFF000000 | (red << 16) | (green << 8) | blue;
                }
            }

            // Bitmapを生成
            return Bitmap.createBitmap(colors, width, height, Bitmap.Config.ARGB_8888);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    // setButtonBackground メソッドを更新して、Dialog を受け取り、ボタンのサイズを測定
    private void setButtonBackground(final Button button, final Dialog dialog, final String imagePath) {
        button.post(new Runnable() {
            @Override
            public void run() {
                int buttonWidth = button.getWidth();  // ボタンの実際の幅を取得
                Bitmap bitmap = decodeCustomImageFile(imagePath);
                if (bitmap != null && buttonWidth > 0) {
                    Bitmap scaledBitmap = scaleBitmapToWidth(bitmap, buttonWidth); // ボタンの幅に画像をスケール
                    Drawable drawable = new BitmapDrawable(getResources(), scaledBitmap);
                    button.setBackground(drawable);
                }
            }
        });
    }

    private Bitmap scaleBitmapToWidth(Bitmap original, int width) {
        int originalWidth = original.getWidth();
        int originalHeight = original.getHeight();
        float scale = width / (float) originalWidth;
        int newHeight = (int) (originalHeight * scale);
        return Bitmap.createScaledBitmap(original, width, newHeight, true);
    }

    private void setupNavigationButtons(final Dialog dialog, final String[] nodes) {
        Button cancelButton = dialog.findViewById(R.id.cancelButton);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i("EmulatorActivity", "cancelButton clicked");
                extendMenuCallback("");
                dialog.dismiss();
            }
        });

        Button backButton = dialog.findViewById(R.id.backButton);
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (nodes.length > 0) {
                    String[] node = nodes[0].split(";");
                    Log.i("EmulatorActivity", "backButton clicked: " + node[4]);
                    extendMenuCallback(node[4]); // Pass parent ID
                } else {
                    Log.i("EmulatorActivity", "backButton clicked: empty");
                    extendMenuCallback("");
                }
                dialog.dismiss();
            }
        });
    }

    public native void extendMenuCallback(String extendMenu);
    public native void fileSelectCallback(int id);
    public native void bankSelectCallback(int id);
    public native void bootSelectCallback(int id);
    public native void exitSelectCallback(int id);

    // NDKから呼び出される画像保存メソッド
    public void savePngImage(String imageName, byte[] imageData, int width, int height) {
        ImageSaver.saveImage(this, imageName, imageData, width, height);
        showTemporaryDialog("Save " + imageName + ".png");
    }

    public void showTemporaryDialog(final String dialogString) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // ダイアログの作成
                final Dialog dialog = new Dialog(EmulatorActivity.this);
                dialog.requestWindowFeature(Window.FEATURE_NO_TITLE); // タイトル非表示
                dialog.setContentView(R.layout.temp_save_dialog); // ダイアログで使用するレイアウト

                // ダイアログに表示するテキストビューを設定
                TextView textView = dialog.findViewById(R.id.text_view);
                textView.setText(dialogString);

                // ダイアログを表示
                dialog.show();

                // 1.5秒後にダイアログを閉じる
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        dialog.dismiss();
                    }
                }, 1500); // 1500ミリ秒 = 1.5秒
            }
        });
    }

    private int iconResouceId(int iconType, int iconId){
        switch(iconType){
            case 0://systemIcon
                switch(iconId) {
                    case 0:
                        return R.drawable.exit;
                    case 1:
                        return R.drawable.reset;
                    case 2:
                        return R.drawable.sound;
                    case 3:
                        return R.drawable.pcg;
                    case 4:
                        return R.drawable.config;
                    case 5:
                        return R.drawable.keyboard;
                    case 6:
                        return R.drawable.mouse;
                    case 7:
                        return R.drawable.wallpaper;
                    case 8:
                        return R.drawable.joystick;
                    case 9:
                        return R.drawable.screenshot;
                    case 10:
                        return R.drawable.midi;
                    case 11:
                        return R.drawable.blindness;
                }
            case 1://mediaIcon
                switch(iconId) {
                    case 0:
                        return R.drawable.floppy;
                    case 1:
                        return R.drawable.tape;
                    case 2:
                        return R.drawable.cart;
                    case 3:
                        return R.drawable.qd;
                    case 4:
                        return R.drawable.hdd;
                    case 5:
                        return R.drawable.cd;
                    case 6:
                        return R.drawable.bubble;
                    case 7:
                        return R.drawable.binary;
                }
        }
        return R.drawable.floppy;
    }

    //http://blog.livedoor.jp/itahidamito/archives/51661332.html
    public int[] loadBitmap(int iconType, int iconId, int size){
        //画像を取得する
        Resources r = getResources();
        int id  = iconResouceId(iconType, iconId);

        //ビットマップ読み込みオプションの設定
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = true;    //端末スケールに応じたサイズで取得
        options.inPreferredConfig = Bitmap.Config.RGB_565;
        Bitmap bitmap = BitmapFactory.decodeResource(r, id, options);

        //リサイズ
        //int size = 30;//端末スケールで取得すると、サイズ大きすぎるので、30%に縮小する。
        Bitmap afterResizeBitmap = Bitmap.createScaledBitmap(bitmap,
                (int) (bitmap.getWidth() * size / 100),
                (int) (bitmap.getHeight() * size / 100),
                false);

        Bitmap b = afterResizeBitmap.copy(Bitmap.Config.RGB_565, true);

        //Bitmapをint型の配列取得する
        int width = b.getWidth();
        int height = b.getHeight();
        int pixels[] = new int[width * height];
        b.getPixels(pixels, 0, width, 0, 0, width, height);

        //縦横サイズを最初に入れる
        int returnData[] = new int[width * height + 2];
        returnData[0] = width;
        returnData[1] = height;
        System.arraycopy(pixels, 0, returnData, 2, pixels.length);

        return returnData;
    }

    //http://android-note.open-memo.net/sub/image__draw_string_to_canvas.html
    public int[] createBitmapFromString(final String text, int textSize) {
        Paint objPaint = new Paint();
        Bitmap objBitmap;
        Canvas objCanvas;
        int textWidth = textSize * text.length(), textHeight = textSize;

        objPaint.setAntiAlias(true);
        objPaint.setColor(Color.WHITE);
        objPaint.setTextSize(textSize);
        Paint.FontMetrics fm = objPaint.getFontMetrics();
        objPaint.getTextBounds(text, 0, text.length(), new Rect(0, 0, textWidth, textHeight));
        //テキストの表示範囲を設定

        textWidth = (int) objPaint.measureText(text);
        textHeight = (int) (Math.abs(fm.top) + fm.bottom);
        objBitmap = Bitmap.createBitmap(textWidth, textHeight, Bitmap.Config.RGB_565);
        //キャンバスからビットマップを取得
        objCanvas = new Canvas(objBitmap);
        objCanvas.drawText(text, 0, Math.abs(fm.top), objPaint);

        Bitmap b = objBitmap.copy(Bitmap.Config.RGB_565, true);

        //Bitmapをint型の配列取得する
        int width = b.getWidth();
        int height = b.getHeight();
        int pixels[] = new int[width * height];
        b.getPixels(pixels, 0, width, 0, 0, width, height);

        //縦横サイズを最初に入れる
        int returnData[] = new int[width * height + 2];
        returnData[0] = width;
        returnData[1] = height;
        System.arraycopy(pixels, 0, returnData, 2, pixels.length);

        return returnData;
    }
}
