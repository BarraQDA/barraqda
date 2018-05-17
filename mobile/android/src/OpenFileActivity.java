package org.kde.something;

import android.content.ContentResolver;
import android.content.Intent;
import android.util.Log;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.net.Uri;
import android.app.Activity;

import org.qtproject.qt5.android.bindings.QtActivity;

class FileClass
{
    public static native void openUri(String uri);
}

public class OpenFileActivity extends QtActivity
{
    private void displayUri(Uri uri)
    {
        if (uri == null)
            return;

        if (!uri.getScheme().equals("file")) {
            try {
                ContentResolver resolver = getBaseContext().getContentResolver();
                ParcelFileDescriptor fdObject = resolver.openFileDescriptor(uri, "r");
                uri = Uri.parse("fd:///" + fdObject.detachFd());
            } catch (Exception e) {
                e.printStackTrace();

                //TODO: emit warning that couldn't be opened
                Log.e("Okular", "failed to open");
                return;
            }
        }

        Log.e("Okular", "opening url: " + uri.toString());
        FileClass.openUri(uri.toString());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent bundleIntent = getIntent();
        if (bundleIntent == null)
            return;

        final String action = bundleIntent.getAction();
        Log.v("Okular", "Starting action: " + action);
        if (action == "android.intent.action.VIEW") {
            displayUri(bundleIntent.getData());
        }
    }

    private static int OpenDocumentRequest = 42;

    public static void openFile(Activity context, String title, String mimes)
    {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_GET_CONTENT);
        intent.setType("application/pdf");
        Log.v("Okular", "opening: " + mimes);
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimes.split(";"));

        context.startActivityForResult(intent, OpenDocumentRequest);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        Log.v("Okular", "Activity Result: " + String.valueOf(requestCode) + " with code: " + String.valueOf(resultCode));
        if (requestCode == OpenDocumentRequest) {
            Uri uri = intent.getData();
            Log.v("Okular", "Opening document: " + uri.toString());
            displayUri(uri);
        }
    }
}
