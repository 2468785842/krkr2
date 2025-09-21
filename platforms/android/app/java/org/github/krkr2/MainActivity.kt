package org.github.krkr2

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.view.WindowManager
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.libsdl.app.SDLAudioManager
import org.tvp.kirikiri2.KR2Activity


class MainActivity : KR2Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.setEnableVirtualButton(false)
        super.onCreate(savedInstanceState)

        // Workaround in https://stackoverflow.com/questions/16283079/re-launch-of-activity-on-home-button-but-only-the-first-time/16447508
        if (!isTaskRoot) {
            // Android launched another instance of the root activity into an existing task
            //  so just quietly finish and go away, dropping the user back into the activity
            //  at the top of the stack (ie: the last state of this task)
            // Don't need to finish it again since it's finished in super.onCreate .
            return
        }

        // Make sure we're running on Pie or higher to change cutout mode
        // Enable rendering into the cutout area
        val lp = window.attributes
        lp.layoutInDisplayCutoutMode =
            WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        window.attributes = lp

        if (!checkStoragePermission()) {
            requestStoragePermission()
        }

        SDLAudioManager.nativeSetupJNI()
        SDLAudioManager.initialize()
        SDLAudioManager.setContext(getContext())

    }

    override fun onDestroy()
    {
        super.onDestroy()
        SDLAudioManager.release(this)
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
    private fun requestStoragePermission(): Boolean {
        var r = false

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            registerForActivityResult(
                ActivityResultContracts.RequestPermission()
            ) { result -> r = result }
                .launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
            return r
        }

        val startForResult = registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
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
