package org.tvp.kirikiri2;

public final class NativeText {
    static { System.loadLibrary("android_text_contracts"); }
    public static native byte[] read(String text);
    public static native void stage(String text);
    public static native byte[] take();
}
