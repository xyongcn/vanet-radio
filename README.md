# vanet-radio
radio channel hardware and software for vanet

# Build an Intel Edison Image
1. Follow the section 2 of instruction from http://download.intel.com/support/edison/sb/edisonbsp_ug_331188005.pdf
2. After extract edison-src.tgz (before step 3), do
    * vi edison-src/device-software/meta-edison-distro/recipes-connectivity/wpa_supplicant/wpa-supplicant_2.1.bbappend
    * change the following statement
      * SRC_URI = "${BASE_SRC_URI} \ git://android.googlesource.com/platform/external/wpa_supplicant_8;protocol=https;tag=android-4.4.4_r2.0.1"
    * into
      * SRC_URI = "${BASE_SRC_URI} \ git://github.com/wujingbang/wpa_supplicant_8.git;protocol=https;tag=android-4.4.4_r2.0.1"
3. Continue the instruction.

# Connect si4463 to Arduino kit 
1. Pin definition in the Arduino kit:
    * #define NIRQ            6  //interrpt
    * #define RADIO_SDN_PIN   7  //SDN
    * #define SSpin           10 // Slave select. 
    * #define MOSIpin         11 // MOSI
    * #define MISOpin         12 // MISO
    * #define SCKpin          13 // SCK
2. Power output from Arduino kit.
    * 3.3v or 5v
