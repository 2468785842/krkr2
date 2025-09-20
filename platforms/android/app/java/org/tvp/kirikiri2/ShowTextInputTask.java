package org.tvp.kirikiri2;

import static org.tvp.kirikiri2.KR2Activity.mTextEdit;

import android.content.Context;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;

import org.cocos2dx.lib.Cocos2dxActivity;

public class ShowTextInputTask implements Runnable {
    /*
     * This is used to regulate the pan&scan method to have some offset from
     * the bottom edge of the input region and the top edge of an input
     * method (soft keyboard)
     */
    static final int HEIGHT_PADDING = 15;
    public int x, y, w, h;

    public ShowTextInputTask(int x, int y, int w, int h) {
        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
    }

    @Override
    public void run() {
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(w, h + HEIGHT_PADDING);
        params.leftMargin = x;
        params.topMargin = y;

        if (mTextEdit == null) {
            mTextEdit = new DummyEdit(Cocos2dxActivity.getContext());

            KR2Activity.sInstance.mFrameLayout.addView(mTextEdit, params);
        } else {
            mTextEdit.setLayoutParams(params);
        }

        mTextEdit.setVisibility(View.VISIBLE);
        mTextEdit.requestFocus();

        InputMethodManager imm = (InputMethodManager) Cocos2dxActivity.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(mTextEdit, 0);
    }
}