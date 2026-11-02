package com.project.los_santos;

final class NativeBridge {
    static final boolean LIBRARY_LOADED;
    static final String LOAD_ERROR;

    static {
        boolean loaded = false;
        String error = "";
        try {
            System.loadLibrary("native_lib");
            loaded = true;
        } catch (UnsatisfiedLinkError exception) {
            error = exception.toString();
        }
        LIBRARY_LOADED = loaded;
        LOAD_ERROR = error;
    }

    private NativeBridge() {}

    static native String nativeInitialize();
    static native String nativeRunDiagnostic();
    static native void nativeShutdown();
}
