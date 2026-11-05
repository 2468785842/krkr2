package org.tvp.kirikiri2;

import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;

public class SDLInputConnection extends BaseInputConnection {
    public interface Sink {
        void commitText(String text, int newCursorPosition);
        boolean keyAction(int keyCode, boolean pressed);
    }

    private final Sink sink;
    private int pendingAccent;

    public SDLInputConnection(View targetView, boolean fullEditor, Sink sink) {
        super(targetView, fullEditor);
        this.sink = sink;
    }

    // Shared by the invisible editor's hardware listener and IME key events.
    // Android key codes are never text: only getUnicodeChar()/getCharacters()
    // can enter the committed-text path, once on key down.
    public boolean handleKeyEvent(KeyEvent event) {
        int action = event.getAction();
        if (action == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN
                && event.getCharacters() != null) {
            commitText(event.getCharacters(), 1);
            return true;
        }
        if (action != KeyEvent.ACTION_DOWN && action != KeyEvent.ACTION_UP) return false;
        if (event.getKeyCode() == KeyEvent.KEYCODE_DEL) {
            pendingAccent = 0;
            return sink.keyAction(KeyEvent.KEYCODE_DEL, action == KeyEvent.ACTION_DOWN);
        }
        if (event.isMetaPressed() || (event.isCtrlPressed() && !event.isAltPressed())) {
            pendingAccent = 0;
            return false;
        }
        if (!event.isPrintingKey() && event.getKeyCode() != KeyEvent.KEYCODE_SPACE) return false;
        if (action == KeyEvent.ACTION_UP) return true;
        int codePoint = event.getUnicodeChar();
        if ((codePoint & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            pendingAccent = codePoint & KeyCharacterMap.COMBINING_ACCENT_MASK;
            return true;
        }
        if (codePoint <= 0 || !Character.isValidCodePoint(codePoint)
                || (codePoint >= 0xd800 && codePoint <= 0xdfff)) return true;
        String text = new String(Character.toChars(codePoint));
        if (pendingAccent != 0) {
            int combined = KeyCharacterMap.getDeadChar(pendingAccent, codePoint);
            text = combined != 0 ? new String(Character.toChars(combined))
                    : new String(Character.toChars(pendingAccent)) + text;
            pendingAccent = 0;
        }
        commitText(text, 1);
        return true;
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        return handleKeyEvent(event) || super.sendKeyEvent(event);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        pendingAccent = 0;
        sink.commitText(text.toString(), newCursorPosition);
        // Keep Android's editable/composing state in sync. fullEditor=true
        // avoids BaseInputConnection's dummy-mode synthetic key replay.
        return super.commitText(text, newCursorPosition);
    }

    private boolean backspace() {
        return sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL))
                && sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL));
    }

    @Override
    public boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength) {
        if (beforeLength == 1 && afterLength == 0) {
            super.deleteSurroundingTextInCodePoints(beforeLength, afterLength);
            return backspace();
        }
        return super.deleteSurroundingTextInCodePoints(beforeLength, afterLength);
    }

    @Override
    public boolean deleteSurroundingText(int beforeLength, int afterLength) {
        if (beforeLength == 1 && afterLength == 0) {
            super.deleteSurroundingText(beforeLength, afterLength);
            // Calling super.sendKeyEvent sends to ViewRoot rather than our
            // native sink. Route the pair through this connection exactly once.
            return backspace();
        }
        return super.deleteSurroundingText(beforeLength, afterLength);
    }
}
