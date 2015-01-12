#
# usage example a2l.sh libnative.so 0x00005ef4
~/sdk/android-ndk/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-addr2line -f -e obj/local/armeabi-v7a/$1 $2

#HOW TO USE a2l.sh
#---------------------------------------------------------------------------------
#
#*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
#Build fingerprint: 'NOOK/swift/bn_swift:4.2.2/JDQ39/1.0.0:eng/test-keys'
#Revision: '19'
#pid: 8245, tid: 8245, name: ip.playground  >>> ip.playground <<<
#signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr beca8130
#r0 beca812c  r1 6dfd3e38  r2 beca7efc  r3 00000000
#r4 00000140  r5 00000000  r6 00000230  r7 beca7508
#r8 00000079  r9 beca7efc  sl 00000a00  fp 0000011f
#ip 0004b000  sp bec498e8  lr fffb5230  pc 6dea1ef4  cpsr 600b0030
#d0  75386574616c6964  d1  737365636f72506e
#d2  676e697373656367  d3  386574616c69643b
#d4  3cfab37f9d6f1130  d5  0000000400000004
#d6  000000c8000000c8  d7  fffffffc000000c8
#d8  0000000000000000  d9  0000000000000000
#d10 0000000000000000  d11 0000000000000000
#d12 0000000000000000  d13 0000000000000000
#d14 0000000000000000  d15 0000000000000000
#d16 0000000000002035  d17 0000203500000000
#d18 0000000000261aa0  d19 ff000000ff000000
#d20 ff000000ff000000  d21 ff000000ff000000
#d22 ff000000ff000000  d23 ff000000ff000000
#d24 0000000000000000  d25 0000000000000000
#d26 0000000000000000  d27 0000000000000000
#d28 0000000000000000  d29 0000000000000000
#d30 0000000000000000  d31 0000000000000000
#scr 20000091
#backtrace:
##00  pc 00005ef4  /data/app-lib/ip.playground-2/libnative.so (ip_dilate8unz+211)
##01  pc 0000022c  <unknown>
#
#$ a2l.sh libnative.so 0x00005ef4
#
#$ ip_dilate8unz
#$ android_image_processing_playground/jni/libnative/image_processing.c:203
#
#---------------------------------------------------------------------------------
