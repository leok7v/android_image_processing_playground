#make sure ANDROID_ABI=armeabi-v7a with NEON
read -p "Make sure ANDROID_ABI=armeabi-v7a with NEON in platforms/build_android_arm/CMakeCache.txt" -n 1 -r
echo    # (optional) move to a new line
vi platforms/build_android_arm/CMakeCache.txt
grep ANDROID_ABI platforms/build_android_arm/CMakeCache.txt
export ANDROID_NDK=~/sdk/android-ndk
pushd platforms
sh ./scripts/cmake_android_arm.sh
pushd build_android_arm
make -j8
popd
popd
