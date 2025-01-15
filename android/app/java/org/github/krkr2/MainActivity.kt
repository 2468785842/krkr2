package org.github.krkr2

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.tvp.kirikiri2.KR2Activity

class MainActivity : KR2Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!checkStoragePermission()) {
                requestStoragePermission()
            }
        }
    }

    private fun checkStoragePermission(): Boolean {
        // 检查 MANAGE_EXTERNAL_STORAGE 权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager()
        }
        return ContextCompat.checkSelfPermission(
            this, Manifest.permission.WRITE_EXTERNAL_STORAGE
        ) == PackageManager.PERMISSION_GRANTED
    }

    // 请求用户授予 MANAGE_EXTERNAL_STORAGE 权限
    @RequiresApi(api = Build.VERSION_CODES.R)
    private fun requestStoragePermission(): Boolean {
        var r = false

        val startForResult = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            // 用户是否授予了权限
            r = result.resultCode == 1 && checkStoragePermission()
        }

        MaterialAlertDialogBuilder(this)
            .setTitle(getString(R.string.request_storage_permission_title))
            .setMessage(getString(R.string.request_storage_permission))
            .setPositiveButton(getString(R.string.ok)) { _, _ ->
                startForResult.launch(
                    Intent(
                        Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.fromParts("package", packageName, null)
                    )
                )
            }
            .setNegativeButton(getString(R.string.cancel), null)
            .show()

        return r
    }

}
