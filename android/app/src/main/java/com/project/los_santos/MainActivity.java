package com.project.los_santos;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Process;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public final class MainActivity extends Activity {
    private static final String TAG = "ProjectLosSantos";
    private static final String VERSION = "0.1.0";
    private final List<String> logEntries = new ArrayList<>();

    private JSONObject nativeReport;
    private boolean nativeInitialized;
    private String nativeError = "";
    private TextView statusView;
    private TextView diagnosticsView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        log("Activity created");
        initializeNative();
        setContentView(createMainScreen());
        refreshDiagnostics();
    }

    @Override
    protected void onResume() {
        super.onResume();
        log("Activity resumed");
        if (diagnosticsView != null) {
            refreshDiagnostics();
        }
    }

    @Override
    protected void onPause() {
        log("Activity paused");
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        log("Activity destroyed");
        if (NativeBridge.LIBRARY_LOADED && nativeInitialized) {
            try {
                NativeBridge.nativeShutdown();
            } catch (RuntimeException | UnsatisfiedLinkError exception) {
                Log.e(TAG, "Native shutdown failed", exception);
            }
        }
        nativeInitialized = false;
        super.onDestroy();
    }

    private void initializeNative() {
        if (!NativeBridge.LIBRARY_LOADED) {
            nativeError = NativeBridge.LOAD_ERROR;
            log("Native library load failed: " + nativeError);
            return;
        }
        try {
            nativeReport = new JSONObject(NativeBridge.nativeInitialize());
            nativeInitialized = nativeReport.optBoolean("initialized", false) &&
                    nativeReport.optBoolean("runtimeCoreInitialized", false);
            if (!nativeInitialized) {
                nativeError = nativeReport.optString("error", "Native initialization returned false");
            }
            log(nativeInitialized ? "Native initialization succeeded" :
                    "Native initialization reported failure: " + nativeError);
        } catch (RuntimeException | UnsatisfiedLinkError | JSONException exception) {
            nativeError = exception.toString();
            log("Native initialization failed: " + nativeError);
        }
    }

    private View createMainScreen() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(20), dp(18), dp(20), dp(24));
        content.setBackgroundColor(Color.rgb(15, 23, 42));

        TextView logo = new TextView(this);
        logo.setText("PROJECT LOGO");
        logo.setTextColor(Color.rgb(148, 163, 184));
        logo.setTextSize(12);
        logo.setGravity(Gravity.CENTER);
        logo.setBackgroundColor(Color.rgb(30, 41, 59));
        content.addView(logo, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(72)));

        TextView title = new TextView(this);
        title.setText("Project_Los Santos");
        title.setTextColor(Color.WHITE);
        title.setTextSize(27);
        title.setPadding(0, dp(16), 0, dp(2));
        content.addView(title);

        TextView version = new TextView(this);
        version.setText("Development build " + VERSION + "  |  Android ARM64 bring-up");
        version.setTextColor(Color.rgb(148, 163, 184));
        version.setTextSize(13);
        content.addView(version);

        statusView = new TextView(this);
        statusView.setTextColor(Color.WHITE);
        statusView.setTextSize(14);
        statusView.setPadding(dp(14), dp(12), dp(14), dp(12));
        statusView.setBackgroundColor(Color.rgb(30, 41, 59));
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        statusParams.topMargin = dp(18);
        content.addView(statusView, statusParams);

        TextView gameData = new TextView(this);
        gameData.setText("GAME DATA\nDisc 1 — Not supplied\nDisc 2 — Not supplied\n" +
                "Game executable — Not available\n\nThese are expected until user-provided data is inspected.");
        gameData.setTextColor(Color.rgb(203, 213, 225));
        gameData.setTextSize(14);
        gameData.setPadding(dp(14), dp(12), dp(14), dp(12));
        gameData.setBackgroundColor(Color.rgb(30, 41, 59));
        LinearLayout.LayoutParams gameParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        gameParams.topMargin = dp(12);
        content.addView(gameData, gameParams);

        addButton(content, "Run diagnostics", view -> refreshDiagnostics());
        addButton(content, "Run native C++ check", this::runNativeDiagnostic);
        addButton(content, "View application logs", this::showLogs);
        addButton(content, "System information", this::showSystemInformation);

        TextView diagnosticsHeading = new TextView(this);
        diagnosticsHeading.setText("DEVICE DIAGNOSTICS");
        diagnosticsHeading.setTextColor(Color.rgb(96, 165, 250));
        diagnosticsHeading.setTextSize(13);
        diagnosticsHeading.setPadding(0, dp(18), 0, dp(6));
        content.addView(diagnosticsHeading);

        diagnosticsView = new TextView(this);
        diagnosticsView.setTextColor(Color.rgb(226, 232, 240));
        diagnosticsView.setTextSize(13);
        diagnosticsView.setTypeface(android.graphics.Typeface.MONOSPACE);
        diagnosticsView.setPadding(dp(12), dp(12), dp(12), dp(12));
        diagnosticsView.setBackgroundColor(Color.rgb(30, 41, 59));
        content.addView(diagnosticsView, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.addView(content);
        return scroll;
    }

    private void addButton(LinearLayout parent, String label, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(label);
        button.setAllCaps(false);
        button.setOnClickListener(listener);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = dp(6);
        parent.addView(button, params);
    }

    private void refreshDiagnostics() {
        if (statusView != null) {
            String vulkanStatus = vulkanSummary();
            statusView.setText("PLATFORM STATUS\n" +
                    "Native library: " + (NativeBridge.LIBRARY_LOADED ? "Loaded" : "Failed to load") + "\n" +
                    "Native initialization: " + (nativeInitialized ? "Initialized" : "Unavailable") + "\n" +
                    "Android platform: Initialized (API " + Build.VERSION.SDK_INT + ")\n" +
                    "Vulkan: " + vulkanStatus + "\n" +
                    "Game data: Not supplied (expected)");
        }
        if (diagnosticsView != null) {
            diagnosticsView.setText(buildSystemInformation());
        }
        log("Diagnostics refreshed");
    }

    private String vulkanSummary() {
        if (nativeReport == null) {
            return "Not checked";
        }
        JSONObject vulkan = nativeReport.optJSONObject("vulkan");
        if (vulkan == null) {
            return "No Vulkan report";
        }
        if (!vulkan.optBoolean("available", false)) {
            return "Unavailable — " + vulkan.optString("error", "no device details");
        }
        JSONArray devices = vulkan.optJSONArray("physicalDevices");
        if (devices == null || devices.length() == 0) {
            return "Available; no physical device listed";
        }
        JSONObject first = devices.optJSONObject(0);
        return "Available — " + (first == null ? "device detected" :
                first.optString("name", "unnamed device")) + " (Vulkan " +
                vulkan.optString("apiVersion", "unknown") + ")";
    }

    private String buildSystemInformation() {
        StringBuilder text = new StringBuilder();
        text.append("Android: ").append(Build.VERSION.RELEASE)
                .append(" (API ").append(Build.VERSION.SDK_INT).append(")\n");
        text.append("Manufacturer: ").append(Build.MANUFACTURER).append('\n');
        text.append("Model: ").append(Build.MODEL).append('\n');
        text.append("ABIs: ");
        String[] abis = Build.SUPPORTED_ABIS;
        for (int i = 0; i < abis.length; i++) {
            if (i > 0) text.append(", ");
            text.append(abis[i]);
        }
        text.append("\nProcessors: ").append(Runtime.getRuntime().availableProcessors());
        text.append("\nNative library: ").append(NativeBridge.LIBRARY_LOADED ? "loaded" : "load failed");
        if (!nativeError.isEmpty()) {
            text.append("\nNative error: ").append(nativeError);
        }

        ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
        ActivityManager manager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        if (manager != null) {
            manager.getMemoryInfo(memoryInfo);
            text.append("\nAvailable system RAM: ").append(formatBytes(memoryInfo.availMem));
            text.append("\nTotal system RAM: ").append(formatBytes(memoryInfo.totalMem));
            text.append("\nLow-memory state: ").append(memoryInfo.lowMemory ? "yes" : "no");
        }
        Runtime javaRuntime = Runtime.getRuntime();
        text.append("\nApp Java heap used: ")
                .append(formatBytes(javaRuntime.totalMemory() - javaRuntime.freeMemory()));
        text.append("\nApp native heap: ").append(formatBytes(Debug.getNativeHeapAllocatedSize()));
        text.append("\nProcess ID: ").append(Process.myPid());
        text.append("\nVulkan: ").append(vulkanDetails());
        return text.toString();
    }

    private String vulkanDetails() {
        if (nativeReport == null) return "not checked";
        JSONObject vulkan = nativeReport.optJSONObject("vulkan");
        if (vulkan == null) return "no report";
        if (!vulkan.optBoolean("available", false)) {
            return "unavailable (" + vulkan.optString("error", "unknown reason") + ")";
        }
        JSONArray devices = vulkan.optJSONArray("physicalDevices");
        if (devices == null || devices.length() == 0) return "instance available; no devices reported";

        StringBuilder result = new StringBuilder("API ").append(vulkan.optString("apiVersion"));
        for (int i = 0; i < devices.length(); i++) {
            JSONObject device = devices.optJSONObject(i);
            if (device == null) continue;
            result.append("\nGPU ").append(i + 1).append(": ")
                    .append(device.optString("name"))
                    .append(" (device Vulkan ").append(device.optString("apiVersion"))
                    .append(", ").append(device.optInt("extensionCount"))
                    .append(" device extensions)");
            JSONArray extensions = device.optJSONArray("extensions");
            if (extensions != null) {
                for (int extensionIndex = 0; extensionIndex < extensions.length(); extensionIndex++) {
                    result.append("\n  ").append(extensions.optString(extensionIndex));
                }
            }
        }
        result.append("\nRenderer status: not implemented (Vulkan probe only)");
        return result.toString();
    }

    private void runNativeDiagnostic(View ignored) {
        if (!NativeBridge.LIBRARY_LOADED) {
            showMessage("Native library unavailable", nativeError);
            return;
        }
        try {
            String result = NativeBridge.nativeRunDiagnostic();
            log(result);
            showMessage("Native diagnostic", result);
        } catch (RuntimeException | UnsatisfiedLinkError exception) {
            log("Native diagnostic failed: " + exception);
            showMessage("Native diagnostic failed", exception.toString());
        }
    }

    private void showLogs(View ignored) {
        StringBuilder text = new StringBuilder();
        for (String entry : logEntries) {
            text.append(entry).append('\n');
        }
        showMessage("Application logs", text.length() == 0 ? "No log entries" : text.toString());
    }

    private void showSystemInformation(View ignored) {
        showMessage("System information", buildSystemInformation());
    }

    private void showMessage(String title, String message) {
        new AlertDialog.Builder(this)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton(android.R.string.ok, null)
                .show();
    }

    private void log(String message) {
        String entry = System.currentTimeMillis() + " " + message;
        logEntries.add(entry);
        Log.i(TAG, message);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static String formatBytes(long bytes) {
        if (bytes >= 1024L * 1024L * 1024L) {
            return String.format(java.util.Locale.US, "%.2f GiB", bytes / (1024.0 * 1024.0 * 1024.0));
        }
        return String.format(java.util.Locale.US, "%.1f MiB", bytes / (1024.0 * 1024.0));
    }
}
