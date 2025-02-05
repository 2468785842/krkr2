package org.tvp.kirikiri2;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.storage.StorageManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.cocos2dx.lib.Cocos2dxGLSurfaceView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

public class KR2Activity extends Cocos2dxActivity implements ActivityCompat.OnRequestPermissionsResultCallback {

    static ActivityManager mActivityManager = null;
    static ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
    static Debug.MemoryInfo mDbgMemoryInfo = new Debug.MemoryInfo();

    /**
     * @noinspection unused
     */
    public static void updateMemoryInfo() {
        if (mActivityManager == null) {
            mActivityManager = (ActivityManager) sInstance.getSystemService(Activity.ACTIVITY_SERVICE);
        }
        mActivityManager.getMemoryInfo(memoryInfo);
        Debug.getMemoryInfo(mDbgMemoryInfo);
    }


    /**
     * @noinspection unused
     */
    static public String GetVersion() {
        String verstr = null;
        try {
            verstr = sInstance.getPackageManager()
                    .getPackageInfo(sInstance.getPackageName(), 0)
                    .versionName;
        } catch (PackageManager.NameNotFoundException ignored) {
        }
        return verstr;
    }

    /**
     * @noinspection unused
     */
    public static long getAvailMemory() {
        return memoryInfo.availMem;
    }

    /**
     * @noinspection unused
     */
    public static long getUsedMemory() {
        return mDbgMemoryInfo.getTotalPss(); // in kB
    }

    /**
     * @noinspection unused
     */
    public int getDeviceId() {
        return -1;
    }

    @SuppressLint("StaticFieldLeak")
    static public KR2Activity sInstance;

    static public KR2Activity GetInstance() {
        return sInstance;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sInstance = this;
        initDump(this.getFilesDir().getAbsolutePath() + "/dump");
    }

    @Override
    public Cocos2dxGLSurfaceView onCreateView() {
        Cocos2dxGLSurfaceView glSurfaceView = new KR2GLSurfaceView(this);
        hideSystemUI();

        Cocos2dxEGLConfigChooser chooser = new Cocos2dxEGLConfigChooser(this.mGLContextAttrs);
        glSurfaceView.setEGLConfigChooser(chooser);

        return glSurfaceView;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        System.exit(0);
    }

    @Override
    public void onLowMemory() {
        nativeOnLowMemory();
    }

    /**
     * @noinspection unused
     */
    static native void onMessageBoxOK(int nButton);

    /**
     * @noinspection unused
     */
    static native void onMessageBoxText(String text);

    static private native void initDump(String path);

    static private native void nativeOnLowMemory();

    @SuppressLint("StaticFieldLeak")
    public static View mTextEdit = null;

    public static boolean handleMessage(Message msg) {
        return true;
    }

    static Handler msgHandler = new Handler(KR2Activity::handleMessage);

    /**
     * @noinspection unused
     */
    static public void showTextInput(int x, int y, int w, int h) {
        msgHandler.post(new ShowTextInputTask(x, y, w, h));
    }

    static public void hideTextInput() {
        msgHandler.post(() -> {
            if (mTextEdit != null) {
                mTextEdit.setVisibility(View.GONE);
                InputMethodManager imm = (InputMethodManager) sInstance.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(mTextEdit.getWindowToken(), 0);
            }
        });
    }

    private static native void nativeTouchesBegin(final int id, final float x, final float y);

    private static native void nativeTouchesEnd(final int id, final float x, final float y);

    private static native void nativeTouchesMove(final int[] ids, final float[] xs, final float[] ys);

    private static native void nativeTouchesCancel(final int[] ids, final float[] xs, final float[] ys);

    public static native boolean nativeKeyAction(final int keyCode, final boolean isPress);

    public static native void nativeCharInput(final int keyCode);

    public static native void nativeCommitText(String text, int newCursorPosition);

    private static native void nativeInsertText(final String text);

    public static native void nativeDeleteBackward();

    private static native void nativeHoverMoved(final float x, final float y);

    private static native void nativeMouseScrolled(final float scroll);

    static class KR2GLSurfaceView extends Cocos2dxGLSurfaceView {

        public KR2GLSurfaceView(final Context context) {
            super(context);
        }

        public KR2GLSurfaceView(final Context context, final AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        public void insertText(final String pText) {
            nativeInsertText(pText);
        }

        @Override
        public void deleteBackward() {
            nativeDeleteBackward();
        }

        @Override
        public boolean onKeyDown(final int pKeyCode, final KeyEvent pKeyEvent) {
            return switch (pKeyCode) {
                case KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_DPAD_LEFT,
                     KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_UP,
                     KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_ENTER,
                     KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, KeyEvent.KEYCODE_DPAD_CENTER -> {
                    nativeKeyAction(pKeyCode, true);
                    yield true;
                }
                default -> super.onKeyDown(pKeyCode, pKeyEvent);
            };
        }

        @Override
        public boolean onKeyUp(final int pKeyCode, final KeyEvent pKeyEvent) {
            return switch (pKeyCode) {
                case KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_MENU, KeyEvent.KEYCODE_DPAD_LEFT,
                     KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_DPAD_UP,
                     KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_ENTER,
                     KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, KeyEvent.KEYCODE_DPAD_CENTER -> {
                    nativeKeyAction(pKeyCode, false);
                    yield true;
                }
                default -> super.onKeyUp(pKeyCode, pKeyEvent);
            };
        }

        @Override
        public boolean onHoverEvent(final MotionEvent pMotionEvent) {
            final int pointerNumber = pMotionEvent.getPointerCount();
            final float[] xs = new float[pointerNumber];
            final float[] ys = new float[pointerNumber];
            for (int i = 0; i < pointerNumber; i++) {
                xs[i] = pMotionEvent.getX(i);
                ys[i] = pMotionEvent.getY(i);
            }

            if (pMotionEvent.getActionMasked() == MotionEvent.ACTION_HOVER_MOVE) {
                nativeHoverMoved(xs[0], ys[0]);
            }
            return true;
        }

        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouchEvent(final MotionEvent pMotionEvent) {

            // these data are used in ACTION_MOVE and ACTION_CANCEL
            final int pointerNumber = pMotionEvent.getPointerCount();
            final int[] ids = new int[pointerNumber];
            final float[] xs = new float[pointerNumber];
            final float[] ys = new float[pointerNumber];

            for (int i = 0; i < pointerNumber; i++) {
                ids[i] = pMotionEvent.getPointerId(i);
                xs[i] = pMotionEvent.getX(i);
                ys[i] = pMotionEvent.getY(i);
            }

            switch (pMotionEvent.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_POINTER_DOWN:
                    final int indexPointerDown = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    final int idPointerDown = pMotionEvent.getPointerId(indexPointerDown);
                    final float xPointerDown = pMotionEvent.getX(indexPointerDown);
                    final float yPointerDown = pMotionEvent.getY(indexPointerDown);
                    nativeTouchesBegin(idPointerDown, xPointerDown, yPointerDown);
                    break;

                case MotionEvent.ACTION_DOWN:
                    // there are only one finger on the screen
                    final int idDown = pMotionEvent.getPointerId(0);
                    final float xDown = xs[0];
                    final float yDown = ys[0];
                    nativeTouchesBegin(idDown, xDown, yDown);
                    break;

                case MotionEvent.ACTION_MOVE:
                    nativeTouchesMove(ids, xs, ys);
                    break;

                case MotionEvent.ACTION_POINTER_UP:
                    final int indexPointUp = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    final int idPointerUp = pMotionEvent.getPointerId(indexPointUp);
                    final float xPointerUp = pMotionEvent.getX(indexPointUp);
                    final float yPointerUp = pMotionEvent.getY(indexPointUp);
                    nativeTouchesEnd(idPointerUp, xPointerUp, yPointerUp);
                    break;

                case MotionEvent.ACTION_UP:
                    // there are only one finger on the screen
                    final int idUp = pMotionEvent.getPointerId(0);
                    final float xUp = xs[0];
                    final float yUp = ys[0];
                    nativeTouchesEnd(idUp, xUp, yUp);
                    break;

                case MotionEvent.ACTION_CANCEL:
                    nativeTouchesCancel(ids, xs, ys);
                    break;
            }

            return true;
        }

        @Override
        public boolean onGenericMotionEvent(MotionEvent event) {
            if (event.getActionMasked() == MotionEvent.ACTION_SCROLL) {
                float v = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
                nativeMouseScrolled(-v);
                return true;
            }
            return super.onGenericMotionEvent(event);
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        //SDLActivity.mHasFocus = hasFocus;
        if (hasFocus) {
            hideSystemUI();
        }
    }

    void doSetSystemUiVisibility() {
        int uiOpts = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        getWindow().getDecorView().setSystemUiVisibility(uiOpts);
    }

    private static native boolean nativeGetHideSystemButton();

    void hideSystemUI() {
        if (nativeGetHideSystemButton()) {
            doSetSystemUiVisibility();
        }
    }

    static public void exit() {
        System.exit(0);
    }

    static final int ORIENT_VERTICAL = 1;
    static final int ORIENT_HORIZONTAL = 2;

    static public void setOrientation(int orient) {
        if (orient == ORIENT_VERTICAL) {
            sInstance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        } else if (orient == ORIENT_HORIZONTAL) {
            sInstance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }
    }

    @SuppressLint("StaticFieldLeak")
    static DialogMessage mDialogMessage = new DialogMessage();

    /**
     * @noinspection unused
     */
    static public void ShowMessageBox(final String title, final String text, final String[] Buttons) {
        mDialogMessage.Init(title, text, Buttons);
        msgHandler.post(() -> mDialogMessage.ShowMessageBox());
    }

    /**
     * @noinspection unused
     */
    static public void ShowInputBox(final String title, final String prompt, final String text, final String[] Buttons) {
        mDialogMessage.Init(title, prompt, Buttons);
        msgHandler.post(() -> mDialogMessage.ShowInputBox(text));
    }


    /**
     * @noinspection unused
     */
    static public void MessageController(int what, int arg1, int arg2) {
        Message msg = msgHandler.obtainMessage();
        msg.what = what;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
        msgHandler.sendMessage(msg);
    }


    /**
     * @noinspection unused
     */
    static public String getLocaleName() {
        Locale defloc = Locale.getDefault();
        String lang = defloc.getLanguage();
        String country = defloc.getCountry();
        if (!country.isEmpty()) {
            lang += "_";
            lang += country.toLowerCase();
        }
        return lang;
    }

//    StorageManager mStorageManager = null;

    public String[] getStoragePath() {
//        List<String> storagePaths = new ArrayList<>();
//        if (mStorageManager != null) {
//            for (StorageVolume volume : mStorageManager.getStorageVolumes()) {
//                String volumeState = volume.getState();
//                if (!Environment.MEDIA_MOUNTED.equals(volumeState) && !Environment.MEDIA_MOUNTED_READ_ONLY.equals(volumeState)) {
//                    break;
//                }
//                File volumeFile = null;
//                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) { // API 30+
//                    volumeFile = volume.getDirectory();
//                } else if (volume.isPrimary()) {
//                    volumeFile = Environment.getExternalStorageDirectory();
//                }
//                if (volumeFile != null) {
//                    storagePaths.add(volumeFile.getAbsolutePath());
//                }
//            }
//        }
//
//        return storagePaths.toArray(new String[0]);
        return new String[]{Environment.getExternalStorageDirectory().getAbsolutePath()};
    }


    private static String[] getExtSdCardPaths(Context context) {
        List<String> paths = new ArrayList<>();
        for (File file : context.getExternalFilesDirs("external")) {
            if (file != null && !file.equals(context.getExternalFilesDir("external"))) {
                int index = file.getAbsolutePath().lastIndexOf("/Android/data");
                if (index < 0) {
                    Log.w("FileUtils", "Unexpected external file dir: " + file.getAbsolutePath());
                } else {
                    String path = file.getAbsolutePath().substring(0, index);
                    try {
                        path = new File(path).getCanonicalPath();
                    } catch (IOException e) {
                        // Keep non-canonical path.
                    }
                    paths.add(path);
                }
            }
        }
        //if(paths.isEmpty())paths.add("/storage/sdcard1");
        return paths.toArray(new String[0]);
    }

    static String[] _extSdPaths;

    public static String getExtSdCardFolder(final File file, Context context) {
        if (_extSdPaths == null)
            _extSdPaths = getExtSdCardPaths(context);
        try {
            for (String extSdPath : _extSdPaths) {
                if (file.getCanonicalPath().startsWith(extSdPath)) {
                    return extSdPath;
                }
            }
        } catch (IOException e) {
            return null;
        }
        return null;
    }

    /**
     * @noinspection unused
     */
    public static boolean isOnExtSdCard(final File file, Context c) {
        return getExtSdCardFolder(file, c) != null;
    }

    /**
     * @noinspection unused
     */
    static public boolean RenameFile(String from, String to) {
        File file = new File(from);
        File target = new File(to);
        if (!file.exists())
            return false;
        if (target.exists()) {
            if (!DeleteFile(target.getAbsolutePath())) return false;
        }

        File parent = target.getParentFile();
        assert parent != null;
        if (!parent.exists()) {
            if (!CreateFolders(parent.getAbsolutePath())) return false;
        }
        // Try the normal way
        return file.renameTo(target);
    }

    /**
     * @noinspection unused
     */
    public static boolean deleteFilesInFolder(final File folder, Context context) {
        boolean totalSuccess = true;
        if (folder == null)
            return false;
        if (folder.isDirectory()) {
            for (File child : Objects.requireNonNull(folder.listFiles())) {
                deleteFilesInFolder(child, context);
            }

        }
        if (!folder.delete())
            totalSuccess = false;
        return totalSuccess;
    }

    static public boolean DeleteFile(String path) {
        File file = new File(path);
        // First try the normal deletion.
        boolean fileDelete = deleteFilesInFolder(file, sInstance);
        if (file.delete() || fileDelete)
            return true;

        return !file.exists();
    }

    /**
     * @noinspection unused
     */
    public static OutputStream getOutputStream(@NonNull final File target, Context context, long s) throws Exception {
        OutputStream outStream = null;
        try {
            // First try the normal way
            if (Files.isWritable(target.toPath()))
                outStream = new FileOutputStream(target);
        } catch (Exception e) {
            Log.e("FileUtils",
                    "Error when copying file from " + target.getAbsolutePath(), e);
        }
        return outStream;
    }

    /**
     * @noinspection unused
     */
    static public boolean WriteFile(String path, byte[] data) {
        File target = new File(path);
        if (target.exists()) {
            DeleteFile(target.getAbsolutePath()); // to avoid number suffix name
        } else {
            File parent = target.getParentFile();
            assert parent != null;
            if (!parent.exists())
                CreateFolders(parent.getAbsolutePath());
        }

        return true;
    }

    /**
     * @noinspection unused
     */
    static public boolean CreateFolders(String path) {
        File file = new File(path);

        // Try the normal way
        return file.mkdirs();
    }

    /**
     * @noinspection unused
     */
    static boolean isWritableNormalOrSaf(final String path) {
        Context c = sInstance;
        File folder = new File(path);
        return folder.exists() && folder.isDirectory();
    }
}
