package org.github.krkr2;

final class EngineSettings {
    static {
        System.loadLibrary("krkr2_settings");
    }

    private EngineSettings() {}

    // Ordered triples from the native CLI schema: key, accepted values, value.
    static native String[] read(String filesDirectory);
    static native void save(String filesDirectory, String[] assignments);
    static native void checkLaunch(String[] arguments);
}
