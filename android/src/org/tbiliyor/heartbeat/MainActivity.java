package org.tbiliyor.heartbeat;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import org.qtproject.qt.android.QtNative;

public class MainActivity extends org.qtproject.qt.android.bindings.QtActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    public static boolean checkPermission(String permission) {
        Activity activity = QtNative.activity();
        if (activity != null) {
            return activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
        }
        return false;
    }

    public static void requestPermission(String permission) {
        Activity activity = QtNative.activity();
        if (activity != null) {
            activity.requestPermissions(new String[]{permission}, 1);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for (int i = 0; i < permissions.length; i++) {
            if (permissions[i].equals("android.permission.CAMERA")) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    notifyCameraPermissionGranted();
                } else {
                    // Permission denied, handle accordingly                    
                }
                break;
            }
        }
    }

    private native void notifyCameraPermissionGranted();
}
