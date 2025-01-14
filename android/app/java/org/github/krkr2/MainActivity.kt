package org.github.krkr2

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.widget.Toast
import androidx.activity.result.ActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import org.tvp.kirikiri2.KR2Activity

class MainActivity : KR2Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (checkStoragePermissionMissing()) {
                requestStoragePermission()
            }
        }
    }

    private fun checkStoragePermissionMissing(): Boolean {
        // 检查 MANAGE_EXTERNAL_STORAGE 权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return !Environment.isExternalStorageManager()
        }
        return ContextCompat.checkSelfPermission(
            getContext(),
            Manifest.permission.WRITE_EXTERNAL_STORAGE
        ) != PackageManager.PERMISSION_GRANTED
    }

    // 请求用户授予 MANAGE_EXTERNAL_STORAGE 权限
    @RequiresApi(api = Build.VERSION_CODES.R)
    private fun requestStoragePermission() {
        val startForResult = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult(), ::onActivityResult
        )
        try {
            startForResult.launch(
                Intent(
                    Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.fromParts("package", packageName, null)
                )
            )
        } catch (e: Exception) {
            Log.w(javaClass.name, "not support direct jump to package access intent")
            startForResult.launch(
                Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
            )
        }
    }

    fun onActivityResult(result: ActivityResult) {
        // 用户是否授予了权限
        if (result.resultCode == REQUEST_MANAGE_STORAGE_PERMISSION) {
            if (checkStoragePermissionMissing()) {
                Toast.makeText(
                    this,
                    "Permission Denied. Please grant permission to proceed.",
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }

    companion object {
        private const val REQUEST_MANAGE_STORAGE_PERMISSION = 1
    }
}
