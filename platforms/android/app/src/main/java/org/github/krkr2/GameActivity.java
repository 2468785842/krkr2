package org.github.krkr2;

import android.os.Looper;
import android.os.Process;
import android.view.ContextThemeWrapper;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.concurrent.CountDownLatch;

public final class GameActivity extends SDLActivity {
    static final String EXTRA_ARGUMENTS = "org.github.krkr2.launchArguments";

    private static final int MESSAGE = 0;
    private static final int TEXT = 1;
    private static final int MENU = 2;
    private static final int OPEN = 3;
    private static final int SAVE = 4;

    private DialogSession activeDialog;
    private boolean resumed;

    private static final class DialogSession {
        final CountDownLatch dismissed = new CountDownLatch(1);
        String[] result;
        AlertDialog dialog;
    }

    @Override
    protected String[] getLibraries() {
        // SDL is linked statically into libkrkr2.
        return new String[]{"krkr2"};
    }

    public String[] launchArguments() {
        String[] arguments = getIntent().getStringArrayExtra(EXTRA_ARGUMENTS);
        return arguments == null ? new String[0] : arguments;
    }

    private static native boolean requestGameClose();

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK
                && !event.isFromSource(InputDevice.SOURCE_MOUSE)) {
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) {
                onBackPressed();
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onBackPressed() {
        var decor = getWindow().getDecorView();
        var insets = ViewCompat.getRootWindowInsets(decor);
        if (insets != null && insets.isVisible(WindowInsetsCompat.Type.ime())) {
            new WindowInsetsControllerCompat(getWindow(), decor)
                    .hide(WindowInsetsCompat.Type.ime());
            return;
        }
        if (mBrokenLibraries || !requestGameClose()) {
            super.onBackPressed();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        resumed = true;
    }

    @Override
    protected void onPause() {
        resumed = false;
        if (activeDialog != null) {
            complete(activeDialog, -1, "");
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (activeDialog != null) {
            complete(activeDialog, -1, "");
        }
        super.onDestroy();
        // Never reuse process-wide TJS/runtime state for a second game.
        Process.killProcess(Process.myPid());
    }

    // Called synchronously on SDL's native thread. Only the Android UI thread
    // owns Android views. Pause/destroy always release the waiting engine.
    public String[] showEngineDialog(int mode, String title, String text,
                                     String initial, String[] buttons) {
        if (Looper.getMainLooper().isCurrentThread()) {
            return null;
        }
        DialogSession session = new DialogSession();
        runOnUiThread(() -> {
            if (!resumed || isFinishing() || activeDialog != null) {
                complete(session, -1, "");
                return;
            }
            activeDialog = session;
            try {
                if (mode == OPEN || mode == SAVE) {
                    File named = new File(initial);
                    File folder = named.isAbsolute() ? named.getParentFile() : new File(text);
                    if (folder == null || !folder.isAbsolute() || !folder.isDirectory()) {
                        folder = getFilesDir();
                    }
                    showFiles(session, title, folder, named.getName(), mode == SAVE);
                } else {
                    showPrompt(session, mode, title, text, initial, buttons);
                }
            } catch (RuntimeException failure) {
                android.util.Log.e("KrKr2", "Cannot open engine dialog", failure);
                complete(session, -1, "");
            }
        });
        try {
            session.dismissed.await();
            return session.result;
        } catch (InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            runOnUiThread(() -> complete(session, -1, ""));
            return null;
        }
    }

    private void complete(DialogSession session, int index, String value) {
        if (session.result != null) {
            return;
        }
        session.result = new String[]{Integer.toString(index), value};
        if (activeDialog == session) {
            activeDialog = null;
        }
        dismissForReplacement(session);
        session.dismissed.countDown();
    }

    private void dismissForReplacement(DialogSession session) {
        if (session.dialog != null) {
            session.dialog.setOnDismissListener(null);
            session.dialog.dismiss();
            session.dialog = null;
        }
    }

    private void show(DialogSession session, MaterialAlertDialogBuilder builder) {
        session.dialog = builder.create();
        session.dialog.setOnDismissListener(dialog -> complete(session, -1, ""));
        session.dialog.show();
    }

    private void showPrompt(DialogSession session, int mode, String title, String text,
                            String initial, String[] suppliedButtons) {
        MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this).setTitle(title);
        if (mode == MENU) {
            show(session, builder.setItems(suppliedButtons,
                    (dialog, index) -> complete(session, index, "")));
            return;
        }
        String[] buttons = suppliedButtons.length == 0
                ? new String[]{getString(android.R.string.ok)} : suppliedButtons;
        LinearLayout body = body();
        TextInputEditText edit = mode == TEXT ? addInput(body, 0, initial) : null;
        if (buttons.length <= 3) {
            builder.setMessage(text);
            if (edit != null) {
                builder.setView(body);
            }
            builder.setPositiveButton(buttons[0], (d, w) -> choose(session, 0, edit));
            if (buttons.length >= 2) {
                builder.setNegativeButton(buttons[1], (d, w) -> choose(session, 1, edit));
            }
            if (buttons.length == 3) {
                builder.setNeutralButton(buttons[2], (d, w) -> choose(session, 2, edit));
            }
        } else {
            TextView description = new TextView(body.getContext());
            description.setText(text);
            body.addView(description, 0);
            for (int i = 0; i < buttons.length; ++i) {
                int index = i;
                MaterialButton button = new MaterialButton(body.getContext());
                button.setText(buttons[i]);
                button.setOnClickListener(view -> choose(session, index, edit));
                body.addView(button, new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
            }
            ScrollView scroll = new ScrollView(this);
            scroll.addView(body);
            builder.setView(scroll);
        }
        show(session, builder);
    }

    private void choose(DialogSession session, int index, TextInputEditText text) {
        complete(session, index, text == null ? "" : text.getText().toString());
    }

    private void showFiles(DialogSession session, String title, File folder,
                           String filename, boolean save) {
        ArrayList<File> entries = new ArrayList<>();
        ArrayList<String> labels = new ArrayList<>();
        File parent = folder.getParentFile();
        if (parent != null && parent.canRead()) {
            entries.add(parent);
            labels.add("↑ " + getString(R.string.parent_folder));
        }
        File[] files = folder.listFiles(file -> file.isDirectory() || file.isFile());
        if (files != null) {
            Arrays.sort(files, Comparator.comparing(File::isFile)
                    .thenComparing(File::getName, String.CASE_INSENSITIVE_ORDER));
            for (File file : files) {
                entries.add(file);
                labels.add(file.getName() + (file.isDirectory() ? "/" : ""));
            }
        }

        MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this)
                .setTitle(title.isEmpty() ? getString(R.string.choose_file) : title)
                .setNegativeButton(android.R.string.cancel, null);

        LinearLayout body = body();
        TextView path = new TextView(body.getContext());
        path.setText(folder.getAbsolutePath());
        body.addView(path);
        TextInputEditText name = save ? addInput(body, R.string.file_name, filename) : null;

        // List item callbacks dismiss their original dialog. Detach its dismiss
        // listener before opening the next directory or overwrite prompt.
        builder.setView(body).setItems(labels.toArray(new String[0]), (dialog, index) -> {
            File file = entries.get(index);
            dismissForReplacement(session);
            if (file.isDirectory()) {
                showFiles(session, title, file, filename, save);
            } else if (save) {
                confirmOverwrite(session, file);
            } else {
                complete(session, 0, file.getAbsolutePath());
            }
        });
        if (save) {
            builder.setPositiveButton(R.string.save, null);
        }
        show(session, builder);
        if (save) {
            session.dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(view -> {
                String value = name.getText().toString();
                if (!GameLibrary.validName(value)) {
                    name.setError(getString(R.string.invalid_file_name));
                    return;
                }
                File file = new File(folder, value);
                if (file.isDirectory()) {
                    name.setError(getString(R.string.invalid_file_name));
                } else if (file.exists()) {
                    dismissForReplacement(session);
                    confirmOverwrite(session, file);
                } else {
                    complete(session, 0, file.getAbsolutePath());
                }
            });
        }
    }

    private void confirmOverwrite(DialogSession session, File file) {
        show(session, new MaterialAlertDialogBuilder(this).setTitle(R.string.overwrite_file)
                .setMessage(file.getName())
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(R.string.replace,
                        (dialog, index) -> complete(session, 0, file.getAbsolutePath())));
    }

    private LinearLayout body() {
        var context = new ContextThemeWrapper(this, R.style.Theme_KrKr2);
        LinearLayout layout = new LinearLayout(context);
        layout.setOrientation(LinearLayout.VERTICAL);
        int margin = Math.round(24 * getResources().getDisplayMetrics().density);
        layout.setPadding(margin, margin / 2, margin, 0);
        return layout;
    }

    private TextInputEditText addInput(LinearLayout body, int hint, String initial) {
        TextInputLayout layout = new TextInputLayout(body.getContext());
        TextInputEditText field = new TextInputEditText(layout.getContext());
        field.setSingleLine(true);
        field.setText(initial);
        if (hint != 0) {
            layout.setHint(getString(hint));
        }
        layout.addView(field);
        body.addView(layout, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return field;
    }
}
