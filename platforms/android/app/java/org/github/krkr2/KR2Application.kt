package org.github.krkr2;

import android.annotation.SuppressLint
import android.app.Application
import android.content.Context

class KR2Application : Application() {
    override fun onCreate() {
        super.onCreate()
        context = applicationContext
    }

    companion object {
        @SuppressLint("StaticFieldLeak")
        @JvmStatic
        lateinit var context: Context
    }
}
