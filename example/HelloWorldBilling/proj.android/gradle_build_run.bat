call RMDIR "build/outputs/apk" /S /Q
call ndk-build NDK_MODULE_PATH=../../../../ %*
call gradle assembleRelease

rem app-dev-release-unsigned.apk
call zipalign -v -p 4 build/outputs/apk/proj.android-release-unsigned.apk build/outputs/apk/proj.android-release-unsigned-aligned.apk
call apksigner sign --ks android.jks --out  build/outputs/apk/proj.android-release.apk build/outputs/apk/proj.android-release-unsigned-aligned.apk

call adb install -r build/outputs/apk/proj.android-release.apk
call adb shell am start -n org.oxygine.HelloWorldBilling/org.oxygine.HelloWorldBilling.MainActivity