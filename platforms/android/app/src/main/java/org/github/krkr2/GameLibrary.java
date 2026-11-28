package org.github.krkr2;

import android.app.Application;
import android.content.Context;
import android.net.Uri;

import androidx.documentfile.provider.DocumentFile;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class GameLibrary extends AndroidViewModel {
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final MutableLiveData<Boolean> busy = new MutableLiveData<>(false);
    private final MutableLiveData<String> error = new MutableLiveData<>();

    public GameLibrary(Application app) {
        super(app);
    }

    LiveData<Boolean> busy() { return busy; }
    LiveData<String> error() { return error; }
    void clearError() { error.setValue(null); }

    private File root() {
        return new File(getApplication().getFilesDir(), "games");
    }

    List<File> entries() {
        File[] entries = root().listFiles(file -> !file.getName().startsWith(".import-"));
        if (entries == null) {
            return Collections.emptyList();
        }
        Arrays.sort(entries, Comparator.comparing(File::getName, String.CASE_INSENSITIVE_ORDER));
        return Arrays.asList(entries);
    }

    void importGame(Uri uri, boolean directory) {
        if (Boolean.TRUE.equals(busy.getValue())) {
            return;
        }
        busy.setValue(true);
        worker.execute(() -> {
            Path staging = null;
            try {
                Context context = getApplication();
                DocumentFile source = directory ? DocumentFile.fromTreeUri(context, uri)
                        : DocumentFile.fromSingleUri(context, uri);
                if (source == null || !source.canRead()) {
                    throw new IOException("Cannot read the selected game.");
                }
                String name = source.getName();
                if (!validName(name)) {
                    throw new IOException("Invalid document name.");
                }
                Path games = Files.createDirectories(root().toPath());
                staging = Files.createTempDirectory(games, ".import-");
                Path target = staging.resolve("game");
                if (directory) {
                    copy(context, source, target);
                } else {
                    Files.createDirectory(target);
                    copy(context, source, target.resolve("data.xp3"));
                }
                EngineSettings.checkLaunch(new String[]{"--game", target.toString()});
                Path published = games.resolve(name);
                if (Files.exists(published)) {
                    String suffix = UUID.randomUUID().toString().substring(0, 8);
                    published = games.resolve(name + "-" + suffix);
                }
                Files.move(target, published, StandardCopyOption.ATOMIC_MOVE);
            } catch (IOException | RuntimeException failure) {
                String message = failure.getMessage();
                error.postValue(message == null ? failure.toString() : message);
            } finally {
                if (staging != null) {
                    clearStaging(staging.toFile());
                }
                busy.postValue(false);
            }
        });
    }

    static boolean validName(String name) {
        return name != null && !name.isEmpty() && !name.equals(".") && !name.equals("..")
                && name.indexOf('/') < 0 && name.indexOf('\\') < 0 && name.indexOf('\0') < 0;
    }

    private static void copy(Context context, DocumentFile source, Path target)
            throws IOException {
        if (Thread.currentThread().isInterrupted()) {
            throw new IOException("Import cancelled.");
        }
        if (source.isDirectory()) {
            Files.createDirectory(target);
            for (DocumentFile child : source.listFiles()) {
                String name = child.getName();
                if (!validName(name)) {
                    throw new IOException("Invalid document name in " + source.getName());
                }
                copy(context, child, target.resolve(name));
            }
            return;
        }
        try (InputStream input = context.getContentResolver().openInputStream(source.getUri())) {
            if (input == null) {
                throw new IOException("Cannot open " + source.getName());
            }
            try (OutputStream output = Files.newOutputStream(target,
                    java.nio.file.StandardOpenOption.CREATE_NEW,
                    java.nio.file.StandardOpenOption.WRITE)) {
                byte[] buffer = new byte[128 * 1024];
                for (int length; (length = input.read(buffer)) != -1; ) {
                    if (Thread.currentThread().isInterrupted()) {
                        throw new IOException("Import cancelled.");
                    }
                    output.write(buffer, 0, length);
                }
            }
        }
    }

    private static void clearStaging(File file) {
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) {
                clearStaging(child);
            }
        }
        file.delete();
    }

    @Override
    protected void onCleared() {
        worker.shutdownNow();
    }
}
