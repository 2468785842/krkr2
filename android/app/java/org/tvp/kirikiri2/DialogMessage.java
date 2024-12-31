package org.tvp.kirikiri2;

import static org.cocos2dx.lib.Cocos2dxActivity.getContext;
import static org.tvp.kirikiri2.KR2Activity.onMessageBoxOK;
import static org.tvp.kirikiri2.KR2Activity.onMessageBoxText;
import static org.tvp.kirikiri2.KR2Activity.sInstance;

import android.app.AlertDialog;
import android.content.Context;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;

public class DialogMessage
{
    public String Title;
    public String Text;
    public String[] Buttons;
    public EditText TextEditor = null;

    public void Init(final String title, final String text, final String[] buttons)
    {
        this.Title = title;
        this.Text = text;
        this.Buttons = buttons;
    }

    void onButtonClick(int n) {
        if(TextEditor != null) {
            onMessageBoxText(TextEditor.getText().toString());
        }
        onMessageBoxOK(n);
    }

    public AlertDialog.Builder CreateBuilder() {
		/*	TextView showText = new TextView(sInstance);
			showText.setText(Text);
			if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.HONEYCOMB)
				showText.setTextIsSelectable(true);*/
        AlertDialog.Builder builder = new AlertDialog.Builder(sInstance).
                setTitle(Title).
                setMessage(Text).
                //setView(showText).
                        setCancelable(false);
        if(Buttons.length >= 1) {
            builder = builder.setPositiveButton(Buttons[0], (dialog, which) -> onButtonClick(0));
        }
        if(Buttons.length >= 2) {
            builder = builder.setNeutralButton(Buttons[1], (dialog, which) -> onButtonClick(1));
        }
        if(Buttons.length >= 3) {
            builder = builder.setNegativeButton(Buttons[2], (dialog, which) -> onButtonClick(2));
        }
        return builder;
    }

    public void ShowMessageBox()
    {
        CreateBuilder().create().show();
    }

    public void ShowInputBox(final String text) {
        AlertDialog.Builder builder = CreateBuilder();
        TextEditor = new EditText(sInstance);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT);
        TextEditor.setLayoutParams(lp);
        TextEditor.setText(text);
        builder.setView(TextEditor);
        AlertDialog ad = builder.create();
        ad.show();
        TextEditor.requestFocus();
        InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(TextEditor, 0);
    }
}