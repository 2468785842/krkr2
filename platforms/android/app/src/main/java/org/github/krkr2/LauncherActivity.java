package org.github.krkr2;

import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.progressindicator.LinearProgressIndicator;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.material.switchmaterial.SwitchMaterial;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class LauncherActivity extends AppCompatActivity {
    private static final String STATE_SETTINGS = "settings";
    private static final String ARGUMENTS = "engineArguments";

    private GameLibrary library;

    private final ActivityResultLauncher<android.net.Uri> folderPicker =
            registerForActivityResult(new ActivityResultContracts.OpenDocumentTree(), uri -> {
                if (uri != null) {
                    library.importGame(uri, true);
                }
            });

    private final ActivityResultLauncher<String[]> archivePicker =
            registerForActivityResult(new ActivityResultContracts.OpenDocument(), uri -> {
                if (uri != null) {
                    library.importGame(uri, false);
                }
            });

    private LinearLayout root;
    private LinearLayout content;
    private boolean inSettings;
    private boolean launching;
    private OnBackPressedCallback backToLibrary;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        library = new ViewModelProvider(this).get(GameLibrary.class);
        backToLibrary = new OnBackPressedCallback(false) {
            @Override
            public void handleOnBackPressed() {
                showLibrary();
            }
        };
        getOnBackPressedDispatcher().addCallback(this, backToLibrary);
        if (state != null && state.getBoolean(STATE_SETTINGS)) {
            showSettings();
        } else {
            showLibrary();
        }
        library.busy().observe(this, busy -> {
            if (!inSettings) {
                showLibrary();
            }
        });
        library.error().observe(this, error -> {
            if (error != null) {
                library.clearError();
                showError(error);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        launching = false;
    }

    @Override
    protected void onSaveInstanceState(Bundle state) {
        state.putBoolean(STATE_SETTINGS, inSettings);
        super.onSaveInstanceState(state);
    }

    private MaterialToolbar page(int title, boolean settings) {
        inSettings = settings;
        backToLibrary.setEnabled(settings);
        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        MaterialToolbar toolbar = new MaterialToolbar(this);
        toolbar.setTitle(title);
        root.addView(toolbar);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(20), dp(8), dp(20), dp(20));
        scroll.addView(content);
        root.addView(scroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        setContentView(root);
        return toolbar;
    }

    private void showLibrary() {
        MaterialToolbar toolbar = page(R.string.games, false);
        toolbar.getMenu().add(R.string.settings).setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        toolbar.setOnMenuItemClickListener(item -> {
            showSettings();
            return true;
        });

        boolean busy = Boolean.TRUE.equals(library.busy().getValue());
        addButton(content, getString(R.string.import_directory), !busy,
                () -> folderPicker.launch(null));
        addButton(content, getString(R.string.import_archive), !busy,
                () -> archivePicker.launch(new String[]{"*/*"}));
        addButton(content, getString(R.string.open_path), !busy, this::openPath);

        if (busy) {
            LinearProgressIndicator progress = new LinearProgressIndicator(this);
            progress.setIndeterminate(true);
            content.addView(progress, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
            content.addView(label(getString(R.string.importing)));
        }
        List<File> games = library.entries();
        if (games.isEmpty() && !busy) {
            content.addView(label(getString(R.string.no_games)));
        }
        for (File game : games) {
            addButton(content, game.getName(), !busy, () -> play(game.getAbsolutePath()));
        }
    }

    private void openPath() {
        TextInputLayout input = field(R.string.local_path, rememberedPath(), false);
        var dialog = new MaterialAlertDialogBuilder(this).setTitle(R.string.open_path)
                .setView(padded(input))
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(R.string.play, null).show();
        dialog.getButton(androidx.appcompat.app.AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(view -> {
                    String path = input.getEditText().getText().toString();
                    if (play(path)) {
                        getPreferences(MODE_PRIVATE).edit().putString("localPath", path).apply();
                        dialog.dismiss();
                    }
                });
    }

    private String rememberedPath() {
        return getPreferences(MODE_PRIVATE).getString("localPath", "");
    }

    private boolean play(String path) {
        if (launching) {
            return false;
        }
        try {
            ArrayList<String> arguments = new ArrayList<>(Arrays.asList("--game", path));
            arguments.addAll(engineArguments(savedArguments()));
            String[] argv = arguments.toArray(new String[0]);
            EngineSettings.checkLaunch(argv);
            launching = true;
            startActivity(new Intent(this, GameActivity.class)
                    .putExtra(GameActivity.EXTRA_ARGUMENTS, argv));
            return true;
        } catch (RuntimeException failure) {
            launching = false;
            showError(failure.getMessage());
            return false;
        }
    }

    private String savedArguments() {
        return getPreferences(MODE_PRIVATE).getString(ARGUMENTS, "");
    }

    private static List<String> engineArguments(String text) {
        ArrayList<String> arguments = new ArrayList<>();
        arguments.add("--");
        for (String line : text.split("\\r?\\n")) {
            if (!line.trim().isEmpty()) {
                arguments.add(line);
            }
        }
        return arguments;
    }

    private void showSettings() {
        MaterialToolbar toolbar = page(R.string.settings, true);
        toolbar.getMenu().add(R.string.back).setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        toolbar.setOnMenuItemClickListener(item -> {
            showLibrary();
            return true;
        });

        try {
            String[] rows = EngineSettings.read(getFilesDir().getAbsolutePath());
            LinearLayout compatibility = new LinearLayout(this);
            compatibility.setOrientation(LinearLayout.VERTICAL);
            compatibility.setVisibility(View.GONE);
            compatibility.addView(label(getString(R.string.compatibility_summary)));
            for (int i = 0; i < rows.length; i += 3) {
                String key = rows[i];
                boolean legacy = key.startsWith("ogl_") || key.startsWith("GL_")
                        || key.equals("menu_handler_opa") || key.equals("outputlog");
                addSetting(legacy ? compatibility : content, key, rows[i + 1], rows[i + 2]);
            }
            addButton(content, getString(R.string.engine_arguments), true, this::editArguments);
            addButton(content, getString(R.string.compatibility_settings), true,
                    () -> compatibility.setVisibility(
                            compatibility.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE));
            content.addView(compatibility);
        } catch (RuntimeException failure) {
            showError(failure.getMessage());
        }
    }

    private void addSetting(LinearLayout group, String key, String allowed, String value) {
        String name = settingLabel(key);
        if (allowed.equals("0|1")) {
            SwitchMaterial toggle = new SwitchMaterial(this);
            toggle.setText(name);
            toggle.setMinHeight(dp(48));
            toggle.setChecked(value.equals("1"));
            toggle.setOnCheckedChangeListener((button, checked) -> {
                if (!saveSetting(key, checked ? "1" : "0")) {
                    showSettings();
                }
            });
            group.addView(toggle, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        } else if (key.equals("renderer")) {
            group.addView(label(name + " · " + getString(R.string.renderer_summary)));
        } else {
            String display = value.isEmpty() ? getString(R.string.empty_value) : value;
            MaterialButton button = addButton(group, name + " · " + display, true, null);
            button.setTag(value);
            button.setOnClickListener(view -> {
                String current = (String) button.getTag();
                if (allowed.contains("|")) {
                    String[] choices = allowed.split("\\|");
                    new MaterialAlertDialogBuilder(this).setTitle(name)
                            .setSingleChoiceItems(choices, Arrays.asList(choices).indexOf(current),
                                    (dialog, index) -> {
                                        if (saveSetting(key, choices[index])) {
                                            button.setTag(choices[index]);
                                            button.setText(name + " · " + choices[index]);
                                        }
                                        dialog.dismiss();
                                    })
                            .setNegativeButton(android.R.string.cancel, null).show();
                } else {
                    editText(name, allowed, current, false, updated -> {
                        if (!saveSetting(key, updated)) {
                            return false;
                        }
                        button.setTag(updated);
                        button.setText(name + " · " + (updated.isEmpty()
                                ? getString(R.string.empty_value) : updated));
                        return true;
                    });
                }
            });
        }
    }

    private boolean saveSetting(String key, String value) {
        try {
            EngineSettings.save(getFilesDir().getAbsolutePath(), new String[]{key + "=" + value});
            Snackbar.make(root, R.string.saved, Snackbar.LENGTH_SHORT).show();
            return true;
        } catch (RuntimeException failure) {
            showError(failure.getMessage());
            return false;
        }
    }

    private void editArguments() {
        editText(getString(R.string.engine_arguments), getString(R.string.engine_arguments_hint),
                savedArguments(), true, value -> {
                    try {
                        EngineSettings.checkLaunch(engineArguments(value).toArray(new String[0]));
                        getPreferences(MODE_PRIVATE).edit().putString(ARGUMENTS, value).apply();
                        return true;
                    } catch (RuntimeException failure) {
                        showError(failure.getMessage());
                        return false;
                    }
                });
    }

    private interface SaveText {
        boolean save(String value);
    }

    private void editText(String title, String description, String value,
                          boolean multiline, SaveText save) {
        TextInputLayout input = field(0, value, multiline);
        var dialog = new MaterialAlertDialogBuilder(this).setTitle(title).setMessage(description)
                .setView(padded(input))
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(R.string.save, null).show();
        dialog.getButton(androidx.appcompat.app.AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(view -> {
                    if (save.save(input.getEditText().getText().toString())) {
                        dialog.dismiss();
                    }
                });
    }

    private TextInputLayout field(int hint, String value, boolean multiline) {
        TextInputLayout layout = new TextInputLayout(this);
        TextInputEditText edit = new TextInputEditText(layout.getContext());
        edit.setInputType(InputType.TYPE_CLASS_TEXT | (multiline
                ? InputType.TYPE_TEXT_FLAG_MULTI_LINE : InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
        if (!multiline) {
            edit.setSingleLine(true);
        }
        edit.setText(value);
        layout.addView(edit);
        if (hint != 0) {
            layout.setHint(getString(hint));
        }
        return layout;
    }

    private View padded(View view) {
        LinearLayout wrapper = new LinearLayout(this);
        wrapper.setPadding(dp(24), dp(12), dp(24), 0);
        wrapper.addView(view, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return wrapper;
    }

    private MaterialButton addButton(LinearLayout group, String text, boolean enabled, Runnable action) {
        MaterialButton button = new MaterialButton(this, null,
                com.google.android.material.R.attr.materialButtonOutlinedStyle);
        button.setText(text);
        button.setEnabled(enabled);
        if (action != null) {
            button.setOnClickListener(view -> action.run());
        }
        group.addView(button, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return button;
    }

    private TextView label(String text) {
        TextView label = new TextView(this);
        label.setText(text);
        label.setPadding(0, dp(12), 0, dp(12));
        return label;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private String settingLabel(String key) {
        int id = getResources().getIdentifier("setting_" + key, "string", getPackageName());
        return id == 0 ? key : getString(id);
    }

    private void showError(String error) {
        new MaterialAlertDialogBuilder(this).setTitle(R.string.error).setMessage(error)
                .setPositiveButton(android.R.string.ok, null).show();
    }
}
