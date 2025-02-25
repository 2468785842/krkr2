package org.github.krkr2;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.WindowManager;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import androidx.core.content.ContextCompat;

import org.tvp.kirikiri2.KR2Activity;

public class MainActivity extends KR2Activity {

    private ActivityResultLauncher<Intent> storagePermissionLauncher;
    private ActivityResultLauncher<String> writePermissionLauncher;
    private boolean permissionResult;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Workaround for task root check
        if (!isTaskRoot()) {
            return;
        }

        // Configure window attributes for cutout display
        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        getWindow().setAttributes(lp);

        // Initialize Activity Result Launchers
        storagePermissionLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> permissionResult = checkStoragePermission()
        );

        writePermissionLauncher = registerForActivityResult(
                new ActivityResultContracts.RequestPermission(),
                result -> permissionResult = result
        );

        if (!checkStoragePermission()) {
            requestStoragePermission();
            if (!permissionResult) {
                Log.e(this.getClass().getName(),
                        "get storage permission failed!");
            }
        }

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    private boolean checkStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void requestStoragePermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            writePermissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE);
            return;
        }

        new AlertDialog.Builder(this)
                .setTitle(R.string.request_storage_permission_title)
                .setMessage(R.string.request_storage_permission)
                .setPositiveButton(R.string.ok, (dialog, which) -> {
                    Intent intent = new Intent(
                            Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                            Uri.parse("package:" + getPackageName())
                    );
                    storagePermissionLauncher.launch(intent);
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
    }

}