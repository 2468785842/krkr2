package org.libsdl.app;

final class KrkrTextInput {
    private KrkrTextInput() {}

    static native void commit(String value);
}
