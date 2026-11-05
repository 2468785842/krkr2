package org.tvp.kirikiri2;

import android.content.Context;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class InputContractTest {
    private final List<String> commits = new ArrayList<>();
    private final List<String> keys = new ArrayList<>();
    private SDLInputConnection connection;

    @Before
    public void createConnection() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        connection = new SDLInputConnection(new View(context), true, new SDLInputConnection.Sink() {
            @Override
            public void commitText(String text, int cursor) {
                // Exercise the production JNI reader, with a real jstring and
                // Android ART, on every InputConnection text submission.
                byte[] actual = NativeText.read(text);
                assertArrayEquals(text.getBytes(StandardCharsets.UTF_8), actual);
                commits.add(new String(actual, StandardCharsets.UTF_8));
            }
            @Override
            public boolean keyAction(int code, boolean down) {
                keys.add(code + ":" + down);
                return true;
            }
        });
    }

    private KeyEvent key(int action, int code, int meta) {
        return new KeyEvent(0, 0, action, code, 0, meta,
                KeyCharacterMap.VIRTUAL_KEYBOARD, 0, 0, InputDevice.SOURCE_KEYBOARD);
    }

    @Test @UiThreadTest
    public void committedTextReachesJniOnceAndPreservesEditable() {
        String text = "A中あée\u0301\ud83d\ude42\u0000Z";
        assertTrue(connection.commitText(text, 1));
        assertEquals(Arrays.asList(text), commits);
        assertEquals(text, connection.getEditable().toString());
        assertTrue(keys.isEmpty());
    }

    @Test @UiThreadTest
    public void printingKeyDownUpCommitsUnicodeOnce() {
        KeyEvent down = key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A, 0);
        assertEquals('a', down.getUnicodeChar());
        assertTrue(connection.sendKeyEvent(down));
        assertTrue(connection.sendKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_A, 0)));
        assertEquals(Arrays.asList("a"), commits);
        assertTrue(keys.isEmpty());
    }

    @Test @UiThreadTest
    public void hardwarePathAndSpaceUseTheSameHandler() {
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A, KeyEvent.META_SHIFT_ON)));
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_A, KeyEvent.META_SHIFT_ON)));
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_SPACE, 0)));
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_SPACE, 0)));
        assertEquals(Arrays.asList("A", " "), commits);
    }

    @Test @UiThreadTest
    public void multipleCharactersAreNotTruncatedToChar() {
        String text = "中\ud83d\ude42\u0000Z";
        assertTrue(connection.sendKeyEvent(new KeyEvent(0, text, KeyCharacterMap.VIRTUAL_KEYBOARD, 0)));
        assertEquals(Arrays.asList(text), commits);
    }

    @Test @UiThreadTest
    public void composingTextWaitsForCommit() {
        assertTrue(connection.setComposingText("preedit", 1));
        assertTrue(commits.isEmpty());
        assertTrue(connection.commitText("完成\ud83d\ude42", 1));
        assertEquals(Arrays.asList("完成\ud83d\ude42"), commits);
        assertEquals("完成\ud83d\ude42", connection.getEditable().toString());
    }

    @Test @UiThreadTest
    public void deleteSurroundingTextSendsOneNativeBackspacePair() {
        connection.commitText("X", 1);
        assertTrue(connection.deleteSurroundingText(1, 0));
        assertEquals(Arrays.asList(KeyEvent.KEYCODE_DEL + ":true", KeyEvent.KEYCODE_DEL + ":false"), keys);
        assertEquals(Arrays.asList("X"), commits);
        assertEquals("", connection.getEditable().toString());
    }

    @Test @UiThreadTest
    public void codePointDeletionDoesNotSplitTheEditableSurrogatePair() {
        connection.commitText("\ud83d\ude42", 1);
        assertTrue(connection.deleteSurroundingTextInCodePoints(1, 0));
        assertEquals(Arrays.asList(KeyEvent.KEYCODE_DEL + ":true", KeyEvent.KEYCODE_DEL + ":false"), keys);
        assertEquals("", connection.getEditable().toString());
        assertEquals(Arrays.asList("\ud83d\ude42"), commits);
    }

    @Test @UiThreadTest
    public void hardwareAndImeBackspaceDoNotSubmitText() {
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL, 0)));
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL, 0)));
        assertTrue(connection.sendKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL, 0)));
        assertTrue(connection.sendKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL, 0)));
        assertEquals(4, keys.size());
        assertTrue(commits.isEmpty());
    }

    @Test @UiThreadTest
    public void controlAndMetaShortcutsAreNotCommittedAsLetters() {
        assertFalse(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A, KeyEvent.META_CTRL_ON)));
        assertFalse(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A, KeyEvent.META_META_ON)));
        assertTrue(commits.isEmpty());
    }

    @Test @UiThreadTest
    public void deadKeyIsCommittedOnlyAfterTheBaseCharacter() {
        KeyEvent accent = key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_E, KeyEvent.META_ALT_ON);
        assertTrue((accent.getUnicodeChar() & KeyCharacterMap.COMBINING_ACCENT) != 0);
        assertTrue(connection.handleKeyEvent(accent));
        assertTrue(commits.isEmpty());
        assertTrue(connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_E, 0)));
        assertEquals(Arrays.asList("é"), commits);
    }

    @Test @UiThreadTest
    public void repeatedDeadKeyCommitsTheAccentOnce() {
        KeyEvent accent = key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_E, KeyEvent.META_ALT_ON);
        connection.handleKeyEvent(accent);
        connection.handleKeyEvent(accent);
        assertEquals(Arrays.asList("\u00b4"), commits);
    }

    @Test @UiThreadTest
    public void backspaceCancelsAnUncommittedAccentWithoutDeletingGameText() {
        connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_E, KeyEvent.META_ALT_ON));
        connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL, 0));
        connection.handleKeyEvent(key(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL, 0));
        assertTrue(keys.isEmpty());
        assertTrue(commits.isEmpty());
        connection.handleKeyEvent(key(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_E, 0));
        assertEquals(Arrays.asList("e"), commits);
    }

    @Test @UiThreadTest
    public void jniRejectsInvalidUtf16AndRetainsNullAndEmptyDistinctions() {
        for (String text : Arrays.asList("\ud800", "prefix\udc00", "\ud800A", "\udc00\ud800"))
            assertNull(NativeText.read(text));
        assertNull(NativeText.read(null));
        assertArrayEquals(new byte[0], NativeText.read(""));
        String text = "\u0000\ud83d\ude42Z\u0000";
        assertArrayEquals(text.getBytes(StandardCharsets.UTF_8), NativeText.read(text));
    }

    @Test @UiThreadTest
    public void queuedBytesOwnTheirStorageAfterTheJniCallReturns() {
        NativeText.stage(new String(new char[]{'A', '\u0000', '\ud83d', '\ude42', 'Z'}));
        System.gc();
        assertArrayEquals("A\u0000\ud83d\ude42Z".getBytes(StandardCharsets.UTF_8), NativeText.take());
        assertNull(NativeText.take());
        NativeText.stage("valid");
        NativeText.stage("\ud800");
        assertNull(NativeText.take());
    }
}
