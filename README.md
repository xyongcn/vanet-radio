# vanet-radio
radio channel hardware and software for vanet

# Build an Intel Edison Image 
1. IMPORTANT: SPI problem exists in the newly version of linux-yocto for Edison, we can only use the version 2.0:https://downloadcenter.intel.com/download/24909
	* spi problem: The spi will fall into an infinite loop when the spi speed exceeds a threshold (about 100KHz)[1-3]
	* [1] https://communities.intel.com/thread/75511
	* [2] https://communities.intel.com/thread/78149?start=15&tstart=0
	* [3] https://communities.intel.com/message/314820#314820
	* In [3], the solution will end up with malfunction of Arduino spi and rf.ko.
	* Untill 20151012, the only usable system is the version 2.0.
1. Follow the section 2 of instruction from http://download.intel.com/support/edison/sb/edisonbsp_ug_331188005.pdf
2. After extract edison-src.tgz (before step 3), do
    * vi edison-src/device-software/meta-edison-distro/recipes-connectivity/wpa_supplicant/wpa-supplicant_2.1.bbappend
    * change the following statement
      * SRC_URI = "${BASE_SRC_URI} \ git://android.googlesource.com/platform/external/wpa_supplicant_8;protocol=https;tag=android-4.4.4_r2.0.1"
    * into
      * SRC_URI = "${BASE_SRC_URI} \ git://github.com/wujingbang/wpa_supplicant_8.git;protocol=https;tag=android-4.4.4_r2.0.1"
3. Continue the instruction.

# Connect si4463 to Arduino kit 
1. Set the jumper in the Arduino: 
    * Set J9 to 3.3V
    * Reference: https://communities.intel.com/docs/DOC-23455
1. Pin definition in the Arduino kit:
    * #define NIRQ            6  //interrpt
    * #define RADIO_SDN_PIN   7  //SDN
    * #define SSpin           10 // Slave select. 
    * #define MOSIpin         11 // MOSI
    * #define MISOpin         12 // MISO
    * #define SCKpin          13 // SCK
2. Power output from Arduino kit.
    * 3.3v or 5v (RF4463F30 needs 5V)
    
# Notices for driving the si4463
1. Cmd id MUST be sent after pulling down the slave select pin. In other words:
	 * Before sending every cmd (including the READ_CMD_BUFF), slave select pin must be pulled low from the High state.
	 * In the READ_CMD_BUFF, if the CTS response byte is not 0xFF, the host MCU should pull NSEL high and repeat the polling procedure.
