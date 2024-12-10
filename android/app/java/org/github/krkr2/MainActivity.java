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
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import org.tvp.kirikiri2.KR2Activity;

public class MainActivity extends KR2Activity {


    private static final int REQUEST_MANAGE_STORAGE_PERMISSION = 1;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 如果是 Android 13 或更高版本，检查 MANAGE_EXTERNAL_STORAGE 权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (!isStoragePermissionGranted()) {
                requestStoragePermission();
            } else {
                // 已经拥有权限，可以执行相关操作
                Toast.makeText(this, "Storage Permission Granted!", Toast.LENGTH_SHORT).show();
            }
        }
    }

    // 检查 MANAGE_EXTERNAL_STORAGE 权限
    private boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
    }

    // 请求用户授予 MANAGE_EXTERNAL_STORAGE 权限
    @RequiresApi(api = Build.VERSION_CODES.R)
    private void requestStoragePermission() {
        Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + getApplication().getPackageName()));
        try {
            startActivityForResult(intent, REQUEST_MANAGE_STORAGE_PERMISSION);
        } catch (Exception e) {
            Log.w(getClass().getName(), "not support direct jump to package access intent");
            intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
            startActivityForResult(intent, REQUEST_MANAGE_STORAGE_PERMISSION);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        // 用户是否授予了权限
        if (requestCode == REQUEST_MANAGE_STORAGE_PERMISSION) {
            if (isStoragePermissionGranted()) {
                Toast.makeText(this, "Permission Granted!", Toast.LENGTH_SHORT).show();
                // 继续执行需要权限的操作
            } else {
                Toast.makeText(this, "Permission Denied. Please grant permission to proceed.", Toast.LENGTH_LONG).show();
            }
        }
    }

    @Override
    public int get_res_sd_operate_step() {
        return R.drawable.sd_operate_step;
    }
}
