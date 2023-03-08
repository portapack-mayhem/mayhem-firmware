*In compliance with the [GPL-2.0](https://opensource.org/licenses/GPL-2.0) license: I declare that this version of the program contains my modifications, which can be seen through the usual "git" mechanism.*  


2022-12  
Contributor(s):  
Brumi-2021  
gullradriel  
lujji  
jLynx  
>Merge pull request #744 from strijar/spectrum_colorNew spectrum color scheme Look at table (LUT) for the Waterfall FFT .  
>implement ook scan  
>Merge pull request #754 from lujji/ook-scanImplement OOK scan  
>Merge pull request #769 from eried/next1.6.0 Stable release  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-11  
Contributor(s):  
Jimi Sanchez  
Erwin Ried  
Brumi-2021  
Belousov Oleg  
>Final opt. AM-9K, applied to Audio_RX and Mic App.  
>New spectrum color scheme  
>Contributors gridview  
>Merge pull request #747 from Brumi-2021/“Additional_AM_BW_type_9Khz_on_top_current_6Khz”“additional am bw type 9 khz on top current 6 khz”  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-10  
Contributor(s):  
Brumi-2021  
GullCode  
>Merge pull request #711 from GullCode/recon-appRecon app  
>Finalised all Mic Boost(WM) and Mic ALC(AK)  
>Merge pull request #717 from Brumi-2021/Recovered_ALC_AK4953_Mic_App_feature_solving_WM8731_side_effectsAdding Mic Control gain in both platforms : mic Auto volume Level Control  (AK4951) and mic Boost control (WM8731)  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-09  
Contributor(s):  
Jimi Sanchez  
Erwin Ried  
Brumi-2021  
GullCode  
>Merge pull request #535 from jimilinuxguy/jimi-merging-conflictsPlaylist replay application  
>Merge pull request #3 from eried/masterMaster  
>Ak4951-ALC_base adding WM8731-boost OFF  options  
>Merge pull request #4 from jimilinuxguy/replayappReplayapp  
>Added Recon app persistent settings  
>Added Recon app under 'Receive' menu  
>Added ButtonWithEncoder  
>Update README.md  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-08  
Contributor(s):  
Arne Luehrs  
>Merge pull request #3 from eried/nextSync with upstream  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-06  
Contributor(s):  
jLynx  
>Fix for new H2+ devices (#666)* WIP Fix for new h2+ devices* reset* Updated sub module  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-05  
Contributor(s):  
Jared Boone  
ArjanOnwezen  
GullCode  
Brumi-2021  
jLynx  
>Fixing branch, moving block, adding quote  
>Recovered lost ctcss/roger beep/correct mic gain in mic app from 1.5.1 without ALC (Auto mic Limit Control-AK) (#633)* Update spectrum_collector.cpplower case correction* Update spectrum_collector.cppDescription changed , better explanation.* Revert "Update spectrum_collector.cpp"This reverts commit 4a6fc35384dd3007a4cc91c319b68d2b17232a8c.* Revert "Update spectrum_collector.cpp"This reverts commit 35cece1cb0b611aec2dcee474168798883da2819.* Revert "Solving Compile error on gcc10 . Keeping same safety protection about the size of the array ,but with slightly different sintax."This reverts commit f4db4e2b535cd722ce0f5ee6cd8094d4ba717c4f.* Recovered CTCSS-Roger_beep-MIC-GAIN from 1.5.1* Temporary removing ALC-( for AK4951 platorm)  
>Merge pull request #619 from ArjanOnwezen/save-app-settingsSave individual app settings.  
>Recovered_ALC_Mic_Feature_AK4953_OK_WM8731  
>Sound/white noise Clock fix (#625)  
>Merge pull request #623 from GullCode/touch_return_optionAdded a ui_config flag to manage gui return icon status  
>Added a ui_config flag to manage gui return icon status  
>Persistent memory check value verification, defaulting when fails. (#662)* Make default constructor for touch calibration* Add persistent memory check value and access abstraction.* Add persistent data_t default constructor with reasonable defaults.* serial_format_t default constructor.* Tidy up backlight timeout type.* Add persistent data struct version/checking.* Make range_t functions constexpr.* Move ui_config and functions into class.* Add backlight_config_t struct, separate enable and time settings.  
>Merge pull request #653 from eried/nextv1.5.3  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-04  
Contributor(s):  
Jimi Sanchez  
Žiga Deisinger  
ArjanOnwezen  
Erwin Ried  
jLynx  
Arjan Onwezen  
>Merge pull request #610 from zigad/fix-title-inconsistancyFix #494 - Change App Titles  
>Enabled auto detection again  
>Save individual app settings. Currently only for apps in Receive section and basic settings like, LNA, VGA, Rx Amp and Frequency.  
>Fixed no audio issue v2 (#567)* Fixed no audio issue v2 (#567)  
>Updated comments  
>Merge pull request #536 from jLynx/gen1Fixed R1_20150901 vs R2_20170522 boot & detection issues. Read up about it here https://github.com/eried/portapack-mayhem/wiki/Won't-boot  
>Initial commit of playlist app for replaying capture files sequentially  
>Added the most random fix  
>SD card backup storage (#565)* Got SD card mounting during boot* Cleaned up comments* Now loads settings from SD as backup* Now loads settings from SD as backup* linting* refactoring  
>Stores in memory  
>Adding back wipe sd  
>removed comments  
>2021/12 QFP100 fix  
>Merge pull request #1 from eried/nextMerging changes  
>Merge pull request #547 from jLynx/FM_fixFixed broken radio offset  
>Merge pull request #550 from jLynx/QFP_fixQFP100 Fix pt. 2  
>What did I change?  
>Added support for H2+  
>Added persistent memory of save state  
>Removing wipe sd  
>Merge pull request #554 from ArjanOnwezen/bigger-qr-codeOption to set bigger qr code (used in RadioSonde app)  
>Fixing spelling of Wav Viewer  
>Merge pull request #2 from jimilinuxguy/jimi-merging-conflictsJimi merging conflicts  
>Fix #494 - Change App TitlesI choose what I think are the best Titles based on existing titles/class names and so on. There were also inconsistencies between TX and Transmit and RX and receive. I renamed them to shorter version TX and RX also added it as suffix where possible to make it clearer in what mode you are in. If you have any other title suggestions or changes please use Add comment on Files Changed Screen so I can change it.  
>2021/12 QFP100 Fix2021/12 QFP100 fix  
>Merge pull request #482 from zigad/feature/disableTouchEnable / Disable Touchscreen  
>Merge pull request #529 from jLynx/version_automationVersion automation/injection  
>Merge pull request #38 from eried/next[pull] next from eried:next  
>Fixed R1_20150901 vs R2_20170522 boot & detection issues  
>Adding playlist to CMakeLists  
>WIP  
>Update README.md  
>Fixed broken radio offset  
>Merge pull request #546 from eried/nextV1.5.0  
>Added button on boot detection  
>Added support for LARGE qr code.  
>removed commented lines  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-03  
Contributor(s):  
eried  
Erwin Ried  
Brumi-2021  
Arjan Onwezen  
>Added consecutive id's in message.hpp, as mentioned in #484.  
>Merge pull request #506 from ArjanOnwezen/#484-add-consecutive-ids-in-message.hppAdded consecutive id's in message.hpp, as mentioned in #484.  
>Merge pull request #497 from Brumi-2021/Solving_mic_saturation_and_spectrum_armonics_Mic_AppSolving_mic_saturation_and_spectrum_harmonics_Mic_App  
>Solving_mic_saturation_and_spectrum_armonics_Mic_App  
>Version bump  
>Merge pull request #488 from ArjanOnwezen/improve-screen-navigationImprovements to screen navigation  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-02  
Contributor(s):  
Arne Luehrs  
>Merge pull request #1 from eried/nextSync with upstream  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-01  
Contributor(s):  
eried  
Arjan Onwezen  
Erwin Ried  
Žiga Deisinger  
>Update README.mdLink to a cheaper item with more accesory options  
>Enable / Disable Touchscreen #481This commit will reverse the logic of touch screen as suggested by @ArjanOnwezen  
>some improvements to screen navigation  
>Merge pull request #461 from notpike/touchtunesTouchtunes EW Mode Feature  
>Version bump  
>Add code for feature "Disable Touch"  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-11  
Contributor(s):  
eried  
Erwin Ried  
heurist1  
ArjanOnwezen  
>Merge pull request #424 from intoxsick/custom-splash-screen-from-sdCustom splash screen from sd  
>Merge pull request #432 from ArjanOnwezen/license-correctionLicense correction  
>Merge pull request #422 from heurist1/adsb-improve-decode-and-guiAdsb improve decode and gui  
>Merge pull request #429 from heurist1/add_2_button_press_back_aka_topleftAdded a quick way of moving the cursor to the top left (back) location  
>Update README.md  
>Changed amp to integer  
>Version bump  
>Create LICENSEadded GPL3 license  
>Merge pull request #426 from heurist1/update_pocsag_decoderUpdate pocsag decoder  
>Improve output  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-10  
Contributor(s):  
heurist1  
>Added backAdded the ability to use the Up and Left buttons simultaneously to cause the cursor to move to the top left of the screen  
>Removed all traces of the parameters on the POGSAG config messageLeft in the message for the moment, because there are likely to be parameters needed at some point.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-09  
Contributor(s):  
Erwin Ried  
Arjan Onwezen  
Brumi-2021  
>Merge pull request #412 from ArjanOnwezen/fix-backlight-time-outFixed #398: Changed datatype to be able to store > 255 second for backlight timer  
>changed datatype to be able to store > 255 second for backlight timer  
>Merge pull request #413 from Brumi-2021/New_Feature_Remove_Initial_short_Ant_DC_bias_pulse_when_cold_resetUpdate main.cpp  
>Update main.cppRemove initial short wrong Ant-DC_BIAS pulse. (250 msegs aprox)  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-06  
Contributor(s):  
Erwin Ried  
teixeluis  
>Merge remote-tracking branch 'portapack-mayhem/next' into feature-radiosondes-beep-371  
>Merge pull request #361 from ArjanOnwezen/iso-date-timeISO datetime for Clock, FileManager, Frequency manager, added clock UI options  
>Merge pull request #367 from teixeluis/hotfix-rx-hang-issue-365Added fix in the scope of issue #365  
>Fixed the mixing of aircraft coordinates in the details view, bychecking if the ICAO address of the frame and the current itemin the details view match. Slight refactor by placing the decimalto string conversion function into the string_format module.Added fix in the scope of issue #365FrequencyStepView field in TransmitterView classFrequencyStepView field in TransmitterView classUpdate ui_transmitter.hppUpdate creditsFixed left padding of the decimal part of the numbers.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-05  
Contributor(s):  
Arjan Onwezen  
teixeluis  
>ISO datetime for Clock, FileManager, Frequency managerAdded option to show/hide clock (with/without date)  
>Added fix in the scope of issue #365  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-04  
Contributor(s):  
Erwin Ried  
eried  
zhang00963  
>Merge pull request #333 from zhang00963/nextAutomatic recognition of audio chip patch  
>Merge pull request #339 from eried/recognition-of-audio-chipRecognition of audio chip  
>Version bump  
>Removing the warningDoes not applies anymore with the latest release  
>Merge pull request #329 from aldude999/nextAM/SSB/DSB Microphone Functionality  
>Realize the automatic recognition of audio chip, including ak4951en/wm8731/wm8731s,Try to fix the max2837 temperature problem  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-03  
Contributor(s):  
DESKTOP-R56EVJP\Alex  
East2West  
>Add APRS Receiving App  
>Rebased code from new eried repo commits. Changed to to reflect strijar implementation. Fixed previous issue with old ssb-am-tx ui_mictx code.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-02  
Contributor(s):  
Erwin Ried  
>Update ui_navigation.cpp  
>Merge pull request #276 from GullCode/chibios_warning_fixFix warning: cast between incompatible function types  
>Merge pull request #288 from GullCode/ui_navigation_warning_fixAdded missing brace  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-01  
Contributor(s):  
Erwin Ried  
euquiq  
GullCode  
eried  
>Merge pull request #273 from GullCode/spi_image_warning_fixmissing contructor  
>missing contructor  
>Cosmetics + Jammer now in green + JitterI think the Jammer deserves a green icon, since it actually does it job pretty well.Then there is a Jitter parameter. It allows to introduce a jitter from 1/60th of a second up to 60/60th of a second (a full one). It will delay / move forward either the TX or the cooldown period for a maximum of a half of the time you choose as jitter.Meaning: If I choose 60/60th, a full second of jitter, it will produce a random number from 1 to 60.Then it will calculate jitter = 30 - randomnumberTHen it will "add" that (positive or negative) time to the  time counter for the next jitter change of state.  
>Merge pull request #263 from euquiq/JAMMER-TX_COOLDOWN_TIMERS_IN_SECONDSJAMMER NOW INCLUDES TWO TIMERS  
>Merge pull request #257 from strijar/waterfall-barWaterfall filter bar  
>Added missing brace  
>Update README.md  
>Version bump  
>Fix warning: cast between incompatible function types from 'void (*)(void *)' to 'msg_t (*)(void *)'  
>Merge pull request #275 from GullCode/channel_stat_collector_warning_fixFix __SIMD32 warning  
>Fix __SIMD32 warning  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-12  
Contributor(s):  
Белоусов Олег  
>Implemented correct display of the filter indicator on the waterfall  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-11  
Contributor(s):  
Erwin Ried  
euquiq  
>Version bump  
>Merge pull request #222 from euquiq/simplify-persistent-memory-ui-configjust making Persistent Memory easier to read  
>just making Persistent Memory easier to readSome internal code re-writing in order to simplify a bit, and hopefully making it easier to understand what's going on inside there.  
>New icon and shortcut  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-10  
Contributor(s):  
Erwin Ried  
dqs105  
euquiq  
>Added options for tuning CLKOUT freq.- Now we have variable CLKOUT.- CLKOUT can be set between 10kHz and 60MHz.(The output signal will become mostly sine shape when reaching 50MHz.)- Click on freq setting field to change tuning step.  
>Merge pull request #215 from euquiq/looking_glass_full_bw_cascade_scannerNew "looking Glass" app  
>Merge pull request #214 from dqs105/clkout_enableAdd options for enabling CLKOUT.  
>Merge pull request #213 from euquiq/Fix-APRS-TX-appFix aprs tx app  
>Removed trailing spaces.  
>APRS TX app is working fine now!Several old bugs squashed.On the APRS side, most notably, SSID numbers where shifted left twice, instead of once, and bits 5,6 where not properly set.On AX.25 side, the bit stuffing part of the encoder was not placing the zero bit on the right place.Finally, I changed APRS icon from ORANGE to GREEN, since even this may be a simple app, now it's doing its work as intended.  
>New "looking Glass" appCapable of showing a cascade with full bandwidth scan. You can select Min and Max Mhz for the cascade.You can move a marker so to (aproximately) know  a particular frequency on the cascade. If you press the select button, the app will jump into the RX -> AUDIO app, already tuned into the just "marked" frequency.This first version SURELY has space for lots of optimizations and improvement in general.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-09  
Contributor(s):  
Erwin Ried  
dqs105  
>Fix for the freezing when no on_touch_release function  
>Merge pull request #172 from dqs105/new_splashNew custom splash.  
>Merge pull request #171 from euquiq/mic-tx-rx-into-main-menuMenu changes  
>Merge pull request #161 from dqs105/mic_tx_rfgainAdded TX Gain control & code simplification  
>Merge pull request #147 from euquiq/add-sensitivity-config-on-touch-calibrationAdd sensitivity config on touch calibration  
>Update ui_navigation.cpp  
>Version bump  
>combine clkout_config => ui_config  
>Added options for enabling CLKOUT.- CLKOUT can be enabled in Radio settings and status bar.- Fixed a typo(I believe) in ui_navigation.  
>Microphone app button rename  
>Default null for on touch release and pressFixes https://github.com/eried/portapack-mayhem/issues/194  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-08  
Contributor(s):  
klockee  
BuildTools  
dqs105  
euquiq  
Erwin Ried  
Белоусов Олег  
eried  
>More cleanup  
>fix-portapack-backlight-timer-bugThe selected time for backlight off on Options-> Interface was not working ok for most of the selectable time options.  
>Optimized-better-variable-namesReduced the code, into a compact more readable version, with better variable naming criteria  
>Added new custom splash.- Custom splash now can be loaded from SD card.  
>Cleaned up and tweaked  
>MIC TX Now includes RX with Volume and SquelchYou can enable RX and adjust VOLUME  and SQUELCH into your liking.Sadly enough, you will NOT be able to use VOICE ACTIVATION when RX is enabled (to ensure there will be NO audio feedback defeating the VA sensing)A "bug" that won over me, but perhaps and hopefully other coder can easily fix:  The Vumeter will momentarily "dissappear" when enabling RX. But it will reappear as soon as you start TX. Or when you turn off RX.I enabled the PEAK LEVEL MARK on the Vumeter, so you can easily see in which level your input voice / signal is peaking and regulate the MIC gain accordingly in an easier / more robust way.Side enhancement: Took off the dark green, yellow and red coloring from the vumeter when no signal is present, and replaced it with dark_grey. I know that some coloring is "eye-candy" but the vu-meter is more readable with this new contrast.  
>Added new toolbar in main menu  
>Restored radio.cpp  
>Margin for footer elements  
>In some scenarios, the new line does not work if is not called thru the write function  
>Merge pull request #157 from eried/gcc9.3-assert-redefinitionGcc9.3 assert redefinition  
>Audio FIR filter for CW  
>Merge pull request #119 from euquiq/MIC_TX_RX_with_volume_and_squelchMIC TX Now includes RX with Volume and Squelch  
>Merge pull request #1 from eried/masterUpdate merge  
>Added BMP file support.- Now can load splash.bmp!  
>Hide the seconds at the beginning  
>ACARS is not implemented  
>Tx led fix & UI tweak & Rx frequency  
>Menu changesMoved the MIC TX RX into the main menu.Changed scanner color from yellow to green, since it is kind of rounded up into an acceptable functionality.Also, did a bit of cleanup on the code spacing inside the menues.  
>Merge Button and TxButton & Minor bug fix- TxButton and Button now merged into one widget(with compatibility).- Fixed Tx stuck when pressing the button with key at the first time.- Other small bug fix.  
>Fix title in splash screen  
>modify file extension from data to bin.  
>Fixed typo.  
>Merge pull request #112 from klockee/interactive_titlebarAdded interactive titlebar  
>Merge pull request #145 from strijar/audio-cwAudio FIR filter for CW  
>Merge pull request #167 from euquiq/fix-backlight-timer-bugfix-portapack-backlight-timer-bug  
>Update README.md  
>Merge pull request #139 from eried/v1.2V1.2  
>Added interactive titlebar!  
>Merged TxButton and Button & minor bug fix  
>Merge pull request #135 from euquiq/radiosonde-vaisala-rs41-decodingRadiosonde-app-Vaisala-rs41-decoding  
>Removed drawRAW.- BMP files can be loaded directly now, so it can be removed.  
>Merge dqs105/mic_tx_rx_dev_uiUI tweak & new PTT button  
>Merge pull request #122 from klockee/new-main-menu-footerNew main menu footer  
>Super simple about  
>Skip splash when pressing titlebar, if enabled  
>UI tweak & new PTT button  
>Version bump  
>Fixed RGB565 mode & clean up  
>Rename m4txevent::assert and m0apptxevent::assert (adding "_event")Copied from https://github.com/sharebrained/portapack-hackrf/commit/f6cdf6a7229638be5c14ae5879ba4ec0dc0bf29e#  
>Update ui_navigation.cppNow the Sonde App icon is in green: All sondes in the original proposal are working!  
>Merge dqs105/mic_tx_rx_devVarious enhancements- Separate TX and RX frequency control- Fixed the TX led bug(What does those code do?)- UI tweaks- New PTT button instead of "Right Button" (allowing right button to be always available)  
>Touchscreen sensitivity configurationNow the user can choose between STANDARD, ENHANCED and EXTREME touchscreen sensitivity, upon entering the Calibration App.This will help performing the calibration  on those clone H1 portapacks carrying a "less than optimal" sensitivty touchscreen.After calibrating the touchscreen, the user will be presented -as before- with an option for saving the calibration data. Now, also, the sensitivity index will be saved too, so the touchscreen will be easier to use in every normal operation.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-07  
Contributor(s):  
Erwin Ried  
Joel Wetzell  
>Merge pull request #90 from jwetzell/fix-draw-bearingFix draw bearing  
>Adjust polar to point and bearing drawing  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-06  
Contributor(s):  
Erwin Ried  
euquiq  
eried  
Gregory Fenton  
>New Antenna length CalculatorIt reads the antennas definition from a txt file:WHIPCALC/ANTENNAS.TXTInside the textfile you place each antenna you own with the following sintaxis:<antenna label> <elements length in mm, separated by a space>For example:ANT500 185 315 450 586 724 862Input the required frequency, adjust the wave type (full / half / quarter, etc.) and the calculator will return the antenna length (metric and imperial) while also calculating how much you need to expand the fitting antennas you got defined on the txt.It may return up to 8 matching antennas, which is more than enough (normally you will have 2, perhaps 3 telescopic antennas around for your portapack)If by any chance your antennas txt got more than 8 antennas, and more than 8 matches the length of the freq / wave you want, it will only show the first 8 matching antennas and will warn you at the bottom that there are even more results (hidden).All calculations now are rounded into the best integer, considering first decimal, so precision is double than the original antenna calculator app.  
>Merge pull request #5 from eried/masterUpstream merge  
>Remove clunky text when changing to HackRF mode  
>Mute and unmute audio  
>Speaker option for the H1  
>Icons and colors changes  
>Adding badge with latest release to Readme  
>Clear with buffer clear  
>No space after question mark  
>Adding nightly builds link  
>Update README.md  
>Merge pull request #64 from gregoryfenton/masterRemove clunky text when changing to HackRF mode  
>Persistent setting for speaker icon  
>Merge pull request #85 from euquiq/new-improved-antenna-calculatorNew Antenna length Calculator  
>Merge pull request #66 from gregoryfenton/patch-1Update README.md  
>Merge pull request #83 from euquiq/add-speaker-headphones-mute-syncSpeaker icon sync with headphones  
>Adding badges  
>Link to the internals too from the Readme  
>Version bump  
>Speaker icon sync with headphonesSpeaker icon now behaves naturally: When the speaker amp is off (muted speaker), the headphones amp is on, and viceversa.Fom now on, whenever a future app needs to turn audio output ON, it will depend on the speaker mode.For example:Before, you just did audio::output::unmute(); to activate audio output.Now, you do: portapack::set_speaker_mode(portapack::speaker_mode);Which will take into account (if present) the speaker mode, and either unmute the headphones, or the speaker.If the user has no Speaker Icon active on the Status-bar (as may be the case with H2 users), it will always unmute the headphones.This new approach has the added benefit of not turning on two amps at the same time (the headphones AND speaker) inside the AK4951 audio codec IC.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-05  
Contributor(s):  
Erwin Ried  
Gregory Fenton  
>More small tweaks to the icons  
>Remote is also not implemented  
>Pocsag improvements (#20)* Update analog_audio_app.cpp (#353)* Adding phase field (extracted from @jamesshao8 repo)  
>Merge pull request #2 from eried/masterUpstream merge  
>Some new icons and colors  
>Removing playdead calls, non-implemented entries in main menu, and small UI tweaks  
>Adding phase field (extracted from @jamesshao8 repo) (#357)  
>Fork rename  
>Version bump  
>This color scheme looks solid, more small tweaks  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-04  
Contributor(s):  
Erwin Ried  
Furrtek  
>Analog tv app (#334)* Analog TV app (PAL)* Icon on main menu* Analog TV should be yellowWorks for PAL only know, it would be nice to add NTSC in the future, or some customizable sync  
>GPS Sim  
>Nrf24l01 demodulation (#338)* NRF demodulation* Update ui_navigation.cpp  
>Moving to transmitters menu  
>Removing old doc files  
>Update README.md  
>Adding Debug app back  
>Ble receiver (#337)* BLE app* Update ui_navigation.cppCo-authored-by: Furrtek <furrtek@gmail.com>  
>Merge pull request #2 from furrtek/masterUpdate from origin  
>Merge pull request #4 from jamesshao8/gps-simGps sim  
>Main menu icon for GPS sim  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-12  
Contributor(s):  
h4waii  
Furrtek  
>Added helpful note  
>Update README.md (#295)Changed some wordings.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-10  
Contributor(s):  
Ziggy  
>UI Redesign for Portapack-Havoc (#268)* Power: Turn off additional peripheral clock branches.* Update schematic with new symbol table and KiCad standard symbols.Fix up wires.* Schematic: Update power net labels.* Schematic: Update footprint names to match library changes.* Schematic: Update header vendor and part numbers.* Schematic: Specify (arbitrary) value for PDN# net.* Schematic: Remove fourth fiducial. Not standard practice, and was taking up valuable board space.* Schematic: Add reference oscillator -- options for clipped sine or HCMOS output.* Schematic: Update copyright year.* Schematic: Remove CLKOUT to CPLD. It was a half-baked idea.* Schematic: Add (experimental) GPS circuit.Add note about charging circuit.Update date and revision to match PCB.* PCB: Update from schematic change: now revision 20180819.Diff was extensive due to net renumbering...* PCB: Fix GPS courtyard to accommodate crazy solder paste recommendation in integration manual.PCB: Address DRC clearance violation between via and oscillator pad.* PCB: Update copyright on drawing.* Update schematic and PCB date and revision.* gitignore: Sublime Text editor project/workspace files* Power: Power up or power down peripheral clock at appropriate times, so firmware doesn't freeze...* Clocking: Fix incorrect shift for CGU IDIVx_CTRL.PD field.* LPC43xx: Add CGU IDIVx struct/union type.* Power: Switch off unused IDIV dividers. Make note of active IDIVs and their use.* HackRF Mode: Upgrade firmware to 2018.01.1 (API 1.02)* MAX V CPLD: Refactor class to look more like Xilinx CoolRunner II CPLD class.* MAX V CPLD: Add BYPASS, SAMPLE support.Rename enter_isp -> enable, exit_isp -> disable.Use SAMPLE at start of flash process, which somehow addresses the problem where CFM wouldn't load into SRAM (and become the active bitstream) after flashing.* MAX V CPLD: Reverse verify data checking logic to make it a little faster.* CPLD: After reprogramming flash, immediately clamp I/O signals, load to SRAM, and "execute" the new bitstream.* Si5351: Refactor code, make one of the registers more type-safe.Clock Manager: Track selected reference clock source for later use in user interface.* Clock Manager: Add note about PPM only affecting Si5351C PLLA, which always runs from the HackRF 25MHz crystal.It is assumed an external clock does not need adjustment, though I am open to being convinced otherwise...* PPM UI: Show "EXT" when showing PPM adjustment and reference clock is external.* CPLD: Add pins and logic for new PortaPack hardware feature(s).* CPLD: Bitstream to support new hardware features.* Clock Generator: Add a couple more setter methods for ClockControl registers.* Clock Manager: Use shared MCU CLKIN clock control configuration constant.* Clock Manager: Reduce MCU CLKIN driver current. 2mA should be plenty.* Clock Manager: Remove redundant clock generator output enable.* Bootstrap: Remove unnecessary ldscript hack to locate SPIFI mode change code in RAM.* Bootstrap: Get CPU operating at max frequency as soon as possible.Update SPIFI speed comment.Make some more LPC43xx types into unions with uint32_t.* Bootstrap: Explicitly configure IDIVB for SPIFI, despite LPC43xx bootloader setting it.* Clock Manager: Init peripherals before CPLD reconfig. Do the clock generator setup after, so we can check presence of PortaPack reference clock with the help of the latest CPLD bitstream.* Clock Manager: Reverse sense of conditional that determines crystal or non-crystal reference source. This is for an expected upcoming change where multiple external options can be differentiated.* Bootstrap: Consolidate clock configuration, update SPIFI rate comment.* Clock Manager: Use IDIVA for clock source for all peripherals, instead of PLL1. Should make switching easier going forward.Don't use IRC as clock during initial clock manager configuration. Until we switch to GP_CLKIN, we should go flat out...* ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution.* PortaPack IO: Expose method to set reference oscillator enable pin.* Pin configuration: Do SPIFI pin config with other pins, in preparation for eliminating separate bootloader.* Pin configuration: Disable input buffers on pins that are never read.* Revert "ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution."This reverts commit c0e2bb6cc4cc656769323bdbb8ee5a16d2d5bb03.* PCB: Change PCB stackup, Tg, clarify solder mask color, use more metric.* PCB: Move HackRF header P9 to B.CrtYd layer.* PCB: Change a Tg reference I missed.* PCB: Update footprints for parts with mismatched CAD->tape rotation.Adjust a few layer choice and line thickness bits.* PCB: Got cold feet, switched back to rectangular pads.* PCB: Add Eco layers to be visible and Gerber output.* PCB: Use aux origin for plotting, for tidier coordinates.* PCB: Output Gerber job file, because why not?* Schematic: Correct footprints for two reference-related components.* Schematic: Remove manfuacturer and part number for DNP component.* Schematic: Specify resistor value, manufacturer, part number for reference oscillator series termination.* PCB: Update netlist and footprints from schematic.* Netlist: Updated component values, footprints.* PCB: Nudge some components and traces to address DRC clearance violations.* PCB: Allow KiCad to update zone timestamps (again?!).* PCB: Generate *all* Gerber layers.* Schematic, PCB: Update revision to 20181025.* PCB: Adjust fab layer annotations orientation and font size.* PCB: Hide mounting hole reference designators on silk layer.* PCB: Shrink U1, U3 pads to get 0.2mm space between pads.* PCB: Set pad-to-mask clearance to zero, leave up to fab. Set minimum mask web to 0.2mm for non-black options.* PCB: Revise U1 pad shape, mask, paste, thermal drills.Clearance is improved at corner pads.* PCB: Tweak U3 for better thermal pad/drill/mask/paste design.* PCB: Change solder mask color to blue.* Schematic, PCB: Update revision to 20181029.* PCB: Bump minimum mask web down a tiny bit because KiCad is having trouble with math.* Update schematic* Remove unused board files.* Add LPC43xx functions.* chibios: Replace code with per-peripheral structs defining clocks, interrupts, and reset bits.* LPC43xx: Add MCPWM peripheral struct.* clock generator: Use recommended PLL reset register value.Datasheet recommends a value. AN619 is quiet on the topic, claims the low nibble is default 0b0000.* GPIO: Tweak masking of SCU function.I don't remember why I thought this was necessary...* HAL: Explicitly turn on timer peripheral clocks used as systicks, during init.* SCU: Add struct to hold pin configuration.* PAL: Add functions to address The Glitch.https://greatscottgadgets.com/2018/02-28-we-fixed-the-glitch/* PAL/board: New IO initialization codeDeclare initial state for SCU pin config, GPIOs. Apply initial state during PAL init. Perform VAA slow turn-on to address The Glitch.* Merge M0 and M4 to eliminate need for bootstrap firmwareDuring _early_init, detect if we're running on the M4 or M0.If M4: do M4-specific core initialization, reset peripherals, speed up SPIFI clock, start M0, go to sleep.If M0: do all the other things.* Pins: Miscellaneous SCU configuration tweaks.* Little code clarity improvement.* bootstrap: Remove, not necessary.* Clock Manager: Large re-working to support external references.* Clock Manager: Actually store chosen clock referenceSimilarly-named local was covering a member and discarding the value.* Clock Manager: Reference type which contains source, frequency.* Setup: Display reference source, frequency in frequency correction screen.* LPC43xx API: Add extern "C" for use from C++.* Use LPC43xx API for SGPIO, GPDMA, I2S initialization.* I2S: Add BASE_AUDIO_CLK management.* Add MOTOCON_PWM clock/reset structure.* Serial: Fix dumb typos.* Serial: Remove extra reference operator.* Serial: Cut-and-paste error in structure type name.* Move SCU structure from PAL to LPC43xx API.It'd be nice if I gave some thought to where code should live before I commit it.* VAA power: Move code to HackRF board fileIt doesn't belong in PAL.* MAX5 CPLD: Add SAMPLE and EXTEST methods.* Flash image: Change packing scheme to use flash more efficiently.Application is now a single image for both M4 bootstrap and M0.Baseband images come immediately after application binary. No need to align to large blocks (and waste lots of flash).* Clock Manager: Remove PLL1 power down function.* Move and rename peripherals reset function to board module.* Remove unused peripheral/clock management.* Clock Manager: Extract switch to IRC into separate function.* Clock Manager: More explicit shutdown of clocks, clock generator.* Move initialization to board module.* ChibiOS: Rename "application" board, add "baseband" board.There are now two ChibiOS "boards", one which runs the application and does the hardware setup. The other board, "baseband", does very little setup.* Clock Manager: Remove unused crystal enable/disable code.* Clock Manager: Restore clock configuration to SPIFI bootloader state before app shutdown.* Reset peripherals on app shutdown.Be careful not to reset M0APP (the core we're running on) or GPIO (which is holding the hardware in a stable state).* M4/baseband hal_lld_init: use IDIVA, which is configured earlier by M0.This was causing problems during restart into HackRF mode. Baseband hal_lld_init changed M4 clock from IDIVA (set by M0) to PLL1, which was unceremoniously turned off during shutdown.* Audio app: Stop audio PLL on shutdown.* M4 HAL: Make LPC43XX_M4_CLK_SRC optional.This was changing the BASE_M4_CLK when a baseband was run.* LPC43xx C++ layer: Fix IDIVx constructor IDIV narrow field width.* Application board: hide the peripherals_reset function, as it isn't useful except during hardware init.* Consolidate hardware init code to some degree.ClockManager is super-overloaded and murky in its purpose.Migrate audio from IDIVC to IDIVD, to more closely resemble initial clock scheme, so it's simpler to get back to it during shutdown.* Migrate some startup code to application board.* Si5351: Use correct methods for reset().update_output_enable_control() doesn't reset the enabled outputs to the reset state, unless the object is freshly initialized, which it isn't when performing firmware shutdown.For similar reasons, use set_clock_control() instead of setting internal state and then using the update function.* GPIO: Set SPIFI CS pin to match input buffer state coming out of bootloader.* Change application board.c to .cpp, with required dependent changes* Board: Clean up SCU configuration code/data.* I2S: Add shutdown code and use it.* LPC43xx: Consolidate a bunch of structures that had been scattered all over....because I'm an undisciplined coder.* I2S: Fix ordering of branch and base clock disable.Core was hanging, presumably because the register interface on the branch/peripheral was unresponsive after the base clock was disabled.* Controls: Save and expose raw navigation wheel switch stateI need to do some work on debouncing and ignoring simultaneous key presses.* Controls: Add debug view for switches state.* Controls: Ignore all key presses until all keys are released.This should address some mechanical quirks of the navigation wheel used on the PortaPack.* Clock Manager: Wait for only the necessary PLL to lock.Wasn't working on PortaPacks without a built-in clock reference, as that uses the other PLL.TODO: Switching PLLs may be kind of pointless now...* CMake: Pull HackRF project from GitHub and build.* CMake: Remove commented code.* CMake: Clone HackRF via HTTPS, not SSH.* CMake: Extra pause for slow post-DFU firmware boot-up.* CMake: TODO to fix SVF/XSVF file source.* CMake: Ask HackRF hackrf_usb to make DFU binary.* Travis-CI: Add dfu-util, now that HackRF firmware is being built for inclusion.* Travis-CI: Update build environment to Ubuntu xenialPreviously Trusty.* Travis-CI: Incorrectly structured my request for dfu-util package.I'm soooo talented.* ldscript: Mark flash, ram with correct R/W/X flags.* ldscript: Enlarge M0 flash region to 1Mbyte, the size of the HackRF SPI flash.* Receiver: Hide PPM adjustment if clock source is not HackRF crystal.* Documentation: Update product photos and README.* Documentation: Add TCXO feature to README description.* Application: Rearrange files to match HAVOC directory structure.* Map view in AIS (#213)* Added GeoMapView to AISRecentEntryDetailView* Added autoupdate in AIS map* Revert "Map view in AIS (#213)"This reverts commit 262c030224b9ea3e56ff1c8a66246e7ecf30e41f.This commit will be cherry-picked onto a clean branch, then re-committed after a troublesome pull request is reverted.* Revert "Upstream merge to make new revision of PortaPack work (#206)"This reverts commit 920b98f7c9a30371b643c42949066fb7d2441daf.This pull request was missing some changes and was preventing firmware from functioning on older PortaPacks.* CPLD: Pull bitstream from HackRF project.* SGPIO: Identify pins on CPLD by their new functions. Pull down HOST_SYNC_EN.* CPLD: Don't load HackRF CPLD bitstream into RAM.Trying to converge CPLD implementations, so this shouldn't be necesssary. HOWEVER, it would be good to *check* the CPLD contents and provide a way to update, if necessary.* CPLD: Tweak clock generator config to match CPLD timing changes in HackRF.* PinConfig: Drive CPLD pins correctly.* CMake: Use jboone/hackrf master branch, now that CPLD fixes are there.* CMake: Fix HackRF CPLD SVF dependency.Build would break on the first pass, but work if you restarted make.* CMake: Fix my misuse of the HackRF CMake configuration -- was building from too deep in the directory tree* CMake: Work-around for CMake 3.5 not supporting ExternalProject_Add SOURCE_SUBDIR.* CMake: Choose a CMP0005 policy to quiet CMake warnings.* Settings: Show active clock reference. Only show PPM adjustment for HackRF source.* Setup: Format clock reference frequency in MHz, not Hz.* Radio Settings: Change reference clock text color.Make consistent color with other un-editable text.TODO: This is a bit of a hack to get ui::Text objects to support custom colors, like the Label structures used elsewhere.* Pin config: VREGMODE=1, add other pins for completeness, comment detail* Pin setup: More useful comments.* Pin setup: Change some defaults, only set up PortaPack pins if detected.* Pin setup: Disable LPC pull-ups on PP CPLD data bus, as CPLD is pulling up.* Baseband: Allow larger HackRF firmware image.* HackRF: Remove USER_INTERFACE CMake variable.* CPLD: Make use of HackRF CPLD tool to generate code.* Release: Add generation of MD5SUMS, SHA256SUMS during "make release"* Clock generator: Match clock output currents to HackRF firmware.Someday, we will share a code base again...* CMake: Make "firmware" target part of the "all" target.So now an unqualified "make" will make the firmware binary.* CMake: Change how HackRF firmware is incorporated into binary.Use the separate HackRF "RAM" binary. Get rid of the strip-dfu utility, since there's no longer a need to extract the binary from the DFU.* CMake: Renamed GIT_REVISION* -> GIT_VERSION* to match HackRF build env.* CMake: Bring git version handling closer to HackRF for code reuse.* Travis-CI: Rework CI release artifact output.* Travis-CI: Don't assign PROJECT_NAME within deploy-nightly.sh* Travis-CI: Oops, don't include distro package for compiler......when also installing it from a third-party PPA.* Travis-CI: Update GCC package, old one seems "retired"?* Travis-CI: OK, the gcc-arm-none-eabi package is NOT current. Undoing...* Travis-CI: Path oopsies.* Travis-CI: More path confusion. I think this will do it. *touch wood** Travis-CI: Update build message sent to FreeNode #portapack IRC.* Travis-CI: Break out BUILD_DATE from BUILD_NAME.* Travis-CI: Introduce build directories, include MD5 and SHA256 hashes.* Travis-CI: Fix MD5SUMS/SHA256SUMS paths.* Travis-CI: Fix typo generating name for binary links.* Power: Keep 1V8 off until after VAA is brought up.* Power: Bring up VAA in several steps to keep voltage swing small.* About: Show longer commit/tag version string.* Versioning: Report non-CI builds with "local-" version prefix.* Travis-CI: Report new nightly build site in IRC notification.* Change use of GIT_VERSION to VERSION_STRINGRequired by prior merge.* Git: add "hackrf" submodule.* CMake: Use hackrf submodule for build, stop pulling during build.* Travis: Fix build paths due to CMake submodule changes.* Travis: Explicitly update submodules recursively* Revert "Travis: Explicitly update submodules recursively"This reverts commit b246438d805f431e727e01b7407540e932e89ee1.* Travis: Try to sort out hackrf submodule output paths...* Travis: I don't know what I'm doing.* CMake: "make firmware" problem due to target vs. path used for dependency.* HackRF: Incorporate YAML security fix.* CMake: Fix more places where targets should be used......instead of paths to outputs.* CMake: Add DFU file to "make firmware" outputs* HackRF: Update submodule for CMake m0_bin.s path fix.* added encoder support to alphanum* added encoder support to freq-keypad* UI Redesign -added BtnGrid & NewButton widgets and created a new button-basedlayout, with both encoder and touchscreen are supported.* Scanner changes:- using SCANNER.TXT for frequencies, ranges also supported. fileformat is the same as any other frequency file, thus can be editedvia the Frequency Manager.- add nfm bw selector & time-to-wait to the UI- add SCANNER.TXT to sdcard dirorignal idea & scanner file adopted from user 'bicurico'* small changes to scanner* remember last category on frequency manager* fix: cast int16_t instead of uint16_t (although i doubt we willhave more than 32767 buttons in the array...)* added a missing last_category_id on freq manager  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-08  
Contributor(s):  
Furrtek  
>Update README.md  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-05  
Contributor(s):  
furrtek  
>Added RFM69 helperLGE tool: new framesText entry string length bugfix  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-03  
Contributor(s):  
Jared Boone  
>Sync up recent portapack-hackrf changes. (#229)* Power: Turn off additional peripheral clock branches.* Update schematic with new symbol table and KiCad standard symbols.Fix up wires.* Schematic: Update power net labels.* Schematic: Update footprint names to match library changes.* Schematic: Update header vendor and part numbers.* Schematic: Specify (arbitrary) value for PDN# net.* Schematic: Remove fourth fiducial. Not standard practice, and was taking up valuable board space.* Schematic: Add reference oscillator -- options for clipped sine or HCMOS output.* Schematic: Update copyright year.* Schematic: Remove CLKOUT to CPLD. It was a half-baked idea.* Schematic: Add (experimental) GPS circuit.Add note about charging circuit.Update date and revision to match PCB.* PCB: Update from schematic change: now revision 20180819.Diff was extensive due to net renumbering...* PCB: Fix GPS courtyard to accommodate crazy solder paste recommendation in integration manual.PCB: Address DRC clearance violation between via and oscillator pad.* PCB: Update copyright on drawing.* Update schematic and PCB date and revision.* gitignore: Sublime Text editor project/workspace files* Power: Power up or power down peripheral clock at appropriate times, so firmware doesn't freeze...* Clocking: Fix incorrect shift for CGU IDIVx_CTRL.PD field.* LPC43xx: Add CGU IDIVx struct/union type.* Power: Switch off unused IDIV dividers. Make note of active IDIVs and their use.* HackRF Mode: Upgrade firmware to 2018.01.1 (API 1.02)* MAX V CPLD: Refactor class to look more like Xilinx CoolRunner II CPLD class.* MAX V CPLD: Add BYPASS, SAMPLE support.Rename enter_isp -> enable, exit_isp -> disable.Use SAMPLE at start of flash process, which somehow addresses the problem where CFM wouldn't load into SRAM (and become the active bitstream) after flashing.* MAX V CPLD: Reverse verify data checking logic to make it a little faster.* CPLD: After reprogramming flash, immediately clamp I/O signals, load to SRAM, and "execute" the new bitstream.* Si5351: Refactor code, make one of the registers more type-safe.Clock Manager: Track selected reference clock source for later use in user interface.* Clock Manager: Add note about PPM only affecting Si5351C PLLA, which always runs from the HackRF 25MHz crystal.It is assumed an external clock does not need adjustment, though I am open to being convinced otherwise...* PPM UI: Show "EXT" when showing PPM adjustment and reference clock is external.* CPLD: Add pins and logic for new PortaPack hardware feature(s).* CPLD: Bitstream to support new hardware features.* Clock Generator: Add a couple more setter methods for ClockControl registers.* Clock Manager: Use shared MCU CLKIN clock control configuration constant.* Clock Manager: Reduce MCU CLKIN driver current. 2mA should be plenty.* Clock Manager: Remove redundant clock generator output enable.* Bootstrap: Remove unnecessary ldscript hack to locate SPIFI mode change code in RAM.* Bootstrap: Get CPU operating at max frequency as soon as possible.Update SPIFI speed comment.Make some more LPC43xx types into unions with uint32_t.* Bootstrap: Explicitly configure IDIVB for SPIFI, despite LPC43xx bootloader setting it.* Clock Manager: Init peripherals before CPLD reconfig. Do the clock generator setup after, so we can check presence of PortaPack reference clock with the help of the latest CPLD bitstream.* Clock Manager: Reverse sense of conditional that determines crystal or non-crystal reference source. This is for an expected upcoming change where multiple external options can be differentiated.* Bootstrap: Consolidate clock configuration, update SPIFI rate comment.* Clock Manager: Use IDIVA for clock source for all peripherals, instead of PLL1. Should make switching easier going forward.Don't use IRC as clock during initial clock manager configuration. Until we switch to GP_CLKIN, we should go flat out...* ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution.* PortaPack IO: Expose method to set reference oscillator enable pin.* Pin configuration: Do SPIFI pin config with other pins, in preparation for eliminating separate bootloader.* Pin configuration: Disable input buffers on pins that are never read.* Revert "ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution."This reverts commit c0e2bb6cc4cc656769323bdbb8ee5a16d2d5bb03.* PCB: Change PCB stackup, Tg, clarify solder mask color, use more metric.* PCB: Move HackRF header P9 to B.CrtYd layer.* PCB: Change a Tg reference I missed.* PCB: Update footprints for parts with mismatched CAD->tape rotation.Adjust a few layer choice and line thickness bits.* PCB: Got cold feet, switched back to rectangular pads.* PCB: Add Eco layers to be visible and Gerber output.* PCB: Use aux origin for plotting, for tidier coordinates.* PCB: Output Gerber job file, because why not?* Schematic: Correct footprints for two reference-related components.* Schematic: Remove manfuacturer and part number for DNP component.* Schematic: Specify resistor value, manufacturer, part number for reference oscillator series termination.* PCB: Update netlist and footprints from schematic.* Netlist: Updated component values, footprints.* PCB: Nudge some components and traces to address DRC clearance violations.* PCB: Allow KiCad to update zone timestamps (again?!).* PCB: Generate *all* Gerber layers.* Schematic, PCB: Update revision to 20181025.* PCB: Adjust fab layer annotations orientation and font size.* PCB: Hide mounting hole reference designators on silk layer.* PCB: Shrink U1, U3 pads to get 0.2mm space between pads.* PCB: Set pad-to-mask clearance to zero, leave up to fab. Set minimum mask web to 0.2mm for non-black options.* PCB: Revise U1 pad shape, mask, paste, thermal drills.Clearance is improved at corner pads.* PCB: Tweak U3 for better thermal pad/drill/mask/paste design.* PCB: Change solder mask color to blue.* Schematic, PCB: Update revision to 20181029.* PCB: Bump minimum mask web down a tiny bit because KiCad is having trouble with math.* Update schematic* Remove unused board files.* Add LPC43xx functions.* chibios: Replace code with per-peripheral structs defining clocks, interrupts, and reset bits.* LPC43xx: Add MCPWM peripheral struct.* clock generator: Use recommended PLL reset register value.Datasheet recommends a value. AN619 is quiet on the topic, claims the low nibble is default 0b0000.* GPIO: Tweak masking of SCU function.I don't remember why I thought this was necessary...* HAL: Explicitly turn on timer peripheral clocks used as systicks, during init.* SCU: Add struct to hold pin configuration.* PAL: Add functions to address The Glitch.https://greatscottgadgets.com/2018/02-28-we-fixed-the-glitch/* PAL/board: New IO initialization codeDeclare initial state for SCU pin config, GPIOs. Apply initial state during PAL init. Perform VAA slow turn-on to address The Glitch.* Merge M0 and M4 to eliminate need for bootstrap firmwareDuring _early_init, detect if we're running on the M4 or M0.If M4: do M4-specific core initialization, reset peripherals, speed up SPIFI clock, start M0, go to sleep.If M0: do all the other things.* Pins: Miscellaneous SCU configuration tweaks.* Little code clarity improvement.* bootstrap: Remove, not necessary.* Clock Manager: Large re-working to support external references.* Clock Manager: Actually store chosen clock referenceSimilarly-named local was covering a member and discarding the value.* Clock Manager: Reference type which contains source, frequency.* Setup: Display reference source, frequency in frequency correction screen.* LPC43xx API: Add extern "C" for use from C++.* Use LPC43xx API for SGPIO, GPDMA, I2S initialization.* I2S: Add BASE_AUDIO_CLK management.* Add MOTOCON_PWM clock/reset structure.* Serial: Fix dumb typos.* Serial: Remove extra reference operator.* Serial: Cut-and-paste error in structure type name.* Move SCU structure from PAL to LPC43xx API.It'd be nice if I gave some thought to where code should live before I commit it.* VAA power: Move code to HackRF board fileIt doesn't belong in PAL.* MAX5 CPLD: Add SAMPLE and EXTEST methods.* Flash image: Change packing scheme to use flash more efficiently.Application is now a single image for both M4 bootstrap and M0.Baseband images come immediately after application binary. No need to align to large blocks (and waste lots of flash).* Clock Manager: Remove PLL1 power down function.* Move and rename peripherals reset function to board module.* Remove unused peripheral/clock management.* Clock Manager: Extract switch to IRC into separate function.* Clock Manager: More explicit shutdown of clocks, clock generator.* Move initialization to board module.* ChibiOS: Rename "application" board, add "baseband" board.There are now two ChibiOS "boards", one which runs the application and does the hardware setup. The other board, "baseband", does very little setup.* Clock Manager: Remove unused crystal enable/disable code.* Clock Manager: Restore clock configuration to SPIFI bootloader state before app shutdown.* Reset peripherals on app shutdown.Be careful not to reset M0APP (the core we're running on) or GPIO (which is holding the hardware in a stable state).* M4/baseband hal_lld_init: use IDIVA, which is configured earlier by M0.This was causing problems during restart into HackRF mode. Baseband hal_lld_init changed M4 clock from IDIVA (set by M0) to PLL1, which was unceremoniously turned off during shutdown.* Audio app: Stop audio PLL on shutdown.* M4 HAL: Make LPC43XX_M4_CLK_SRC optional.This was changing the BASE_M4_CLK when a baseband was run.* LPC43xx C++ layer: Fix IDIVx constructor IDIV narrow field width.* Application board: hide the peripherals_reset function, as it isn't useful except during hardware init.* Consolidate hardware init code to some degree.ClockManager is super-overloaded and murky in its purpose.Migrate audio from IDIVC to IDIVD, to more closely resemble initial clock scheme, so it's simpler to get back to it during shutdown.* Migrate some startup code to application board.* Si5351: Use correct methods for reset().update_output_enable_control() doesn't reset the enabled outputs to the reset state, unless the object is freshly initialized, which it isn't when performing firmware shutdown.For similar reasons, use set_clock_control() instead of setting internal state and then using the update function.* GPIO: Set SPIFI CS pin to match input buffer state coming out of bootloader.* Change application board.c to .cpp, with required dependent changes* Board: Clean up SCU configuration code/data.* I2S: Add shutdown code and use it.* LPC43xx: Consolidate a bunch of structures that had been scattered all over....because I'm an undisciplined coder.* I2S: Fix ordering of branch and base clock disable.Core was hanging, presumably because the register interface on the branch/peripheral was unresponsive after the base clock was disabled.* Controls: Save and expose raw navigation wheel switch stateI need to do some work on debouncing and ignoring simultaneous key presses.* Controls: Add debug view for switches state.* Controls: Ignore all key presses until all keys are released.This should address some mechanical quirks of the navigation wheel used on the PortaPack.* Clock Manager: Wait for only the necessary PLL to lock.Wasn't working on PortaPacks without a built-in clock reference, as that uses the other PLL.TODO: Switching PLLs may be kind of pointless now...* CMake: Pull HackRF project from GitHub and build.* CMake: Remove commented code.* CMake: Clone HackRF via HTTPS, not SSH.* CMake: Extra pause for slow post-DFU firmware boot-up.* CMake: TODO to fix SVF/XSVF file source.* CMake: Ask HackRF hackrf_usb to make DFU binary.* Travis-CI: Add dfu-util, now that HackRF firmware is being built for inclusion.* Travis-CI: Update build environment to Ubuntu xenialPreviously Trusty.* Travis-CI: Incorrectly structured my request for dfu-util package.I'm soooo talented.* ldscript: Mark flash, ram with correct R/W/X flags.* ldscript: Enlarge M0 flash region to 1Mbyte, the size of the HackRF SPI flash.* Receiver: Hide PPM adjustment if clock source is not HackRF crystal.* Documentation: Update product photos and README.* Documentation: Add TCXO feature to README description.* Application: Rearrange files to match HAVOC directory structure.* Map view in AIS (#213)* Added GeoMapView to AISRecentEntryDetailView* Added autoupdate in AIS map* Revert "Map view in AIS (#213)"This reverts commit 262c030224b9ea3e56ff1c8a66246e7ecf30e41f.This commit will be cherry-picked onto a clean branch, then re-committed after a troublesome pull request is reverted.* Revert "Upstream merge to make new revision of PortaPack work (#206)"This reverts commit 920b98f7c9a30371b643c42949066fb7d2441daf.This pull request was missing some changes and was preventing firmware from functioning on older PortaPacks.* CPLD: Pull bitstream from HackRF project.* SGPIO: Identify pins on CPLD by their new functions. Pull down HOST_SYNC_EN.* CPLD: Don't load HackRF CPLD bitstream into RAM.Trying to converge CPLD implementations, so this shouldn't be necesssary. HOWEVER, it would be good to *check* the CPLD contents and provide a way to update, if necessary.* CPLD: Tweak clock generator config to match CPLD timing changes in HackRF.* PinConfig: Drive CPLD pins correctly.* CMake: Use jboone/hackrf master branch, now that CPLD fixes are there.* CMake: Fix HackRF CPLD SVF dependency.Build would break on the first pass, but work if you restarted make.* CMake: Fix my misuse of the HackRF CMake configuration -- was building from too deep in the directory tree* CMake: Work-around for CMake 3.5 not supporting ExternalProject_Add SOURCE_SUBDIR.* CMake: Choose a CMP0005 policy to quiet CMake warnings.* Settings: Show active clock reference. Only show PPM adjustment for HackRF source.* Setup: Format clock reference frequency in MHz, not Hz.* Radio Settings: Change reference clock text color.Make consistent color with other un-editable text.TODO: This is a bit of a hack to get ui::Text objects to support custom colors, like the Label structures used elsewhere.* Pin config: VREGMODE=1, add other pins for completeness, comment detail* Pin setup: More useful comments.* Pin setup: Change some defaults, only set up PortaPack pins if detected.* Pin setup: Disable LPC pull-ups on PP CPLD data bus, as CPLD is pulling up.* Baseband: Allow larger HackRF firmware image.* HackRF: Remove USER_INTERFACE CMake variable.* CPLD: Make use of HackRF CPLD tool to generate code.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-02  
Contributor(s):  
furrtek  
Jared Boone  
>PortaPack Sync, take 2 (#215)* Power: Turn off additional peripheral clock branches.* Update schematic with new symbol table and KiCad standard symbols.Fix up wires.* Schematic: Update power net labels.* Schematic: Update footprint names to match library changes.* Schematic: Update header vendor and part numbers.* Schematic: Specify (arbitrary) value for PDN# net.* Schematic: Remove fourth fiducial. Not standard practice, and was taking up valuable board space.* Schematic: Add reference oscillator -- options for clipped sine or HCMOS output.* Schematic: Update copyright year.* Schematic: Remove CLKOUT to CPLD. It was a half-baked idea.* Schematic: Add (experimental) GPS circuit.Add note about charging circuit.Update date and revision to match PCB.* PCB: Update from schematic change: now revision 20180819.Diff was extensive due to net renumbering...* PCB: Fix GPS courtyard to accommodate crazy solder paste recommendation in integration manual.PCB: Address DRC clearance violation between via and oscillator pad.* PCB: Update copyright on drawing.* Update schematic and PCB date and revision.* gitignore: Sublime Text editor project/workspace files* Power: Power up or power down peripheral clock at appropriate times, so firmware doesn't freeze...* Clocking: Fix incorrect shift for CGU IDIVx_CTRL.PD field.* LPC43xx: Add CGU IDIVx struct/union type.* Power: Switch off unused IDIV dividers. Make note of active IDIVs and their use.* HackRF Mode: Upgrade firmware to 2018.01.1 (API 1.02)* MAX V CPLD: Refactor class to look more like Xilinx CoolRunner II CPLD class.* MAX V CPLD: Add BYPASS, SAMPLE support.Rename enter_isp -> enable, exit_isp -> disable.Use SAMPLE at start of flash process, which somehow addresses the problem where CFM wouldn't load into SRAM (and become the active bitstream) after flashing.* MAX V CPLD: Reverse verify data checking logic to make it a little faster.* CPLD: After reprogramming flash, immediately clamp I/O signals, load to SRAM, and "execute" the new bitstream.* Si5351: Refactor code, make one of the registers more type-safe.Clock Manager: Track selected reference clock source for later use in user interface.* Clock Manager: Add note about PPM only affecting Si5351C PLLA, which always runs from the HackRF 25MHz crystal.It is assumed an external clock does not need adjustment, though I am open to being convinced otherwise...* PPM UI: Show "EXT" when showing PPM adjustment and reference clock is external.* CPLD: Add pins and logic for new PortaPack hardware feature(s).* CPLD: Bitstream to support new hardware features.* Clock Generator: Add a couple more setter methods for ClockControl registers.* Clock Manager: Use shared MCU CLKIN clock control configuration constant.* Clock Manager: Reduce MCU CLKIN driver current. 2mA should be plenty.* Clock Manager: Remove redundant clock generator output enable.* Bootstrap: Remove unnecessary ldscript hack to locate SPIFI mode change code in RAM.* Bootstrap: Get CPU operating at max frequency as soon as possible.Update SPIFI speed comment.Make some more LPC43xx types into unions with uint32_t.* Bootstrap: Explicitly configure IDIVB for SPIFI, despite LPC43xx bootloader setting it.* Clock Manager: Init peripherals before CPLD reconfig. Do the clock generator setup after, so we can check presence of PortaPack reference clock with the help of the latest CPLD bitstream.* Clock Manager: Reverse sense of conditional that determines crystal or non-crystal reference source. This is for an expected upcoming change where multiple external options can be differentiated.* Bootstrap: Consolidate clock configuration, update SPIFI rate comment.* Clock Manager: Use IDIVA for clock source for all peripherals, instead of PLL1. Should make switching easier going forward.Don't use IRC as clock during initial clock manager configuration. Until we switch to GP_CLKIN, we should go flat out...* ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution.* PortaPack IO: Expose method to set reference oscillator enable pin.* Pin configuration: Do SPIFI pin config with other pins, in preparation for eliminating separate bootloader.* Pin configuration: Disable input buffers on pins that are never read.* Revert "ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution."This reverts commit c0e2bb6cc4cc656769323bdbb8ee5a16d2d5bb03.* PCB: Change PCB stackup, Tg, clarify solder mask color, use more metric.* PCB: Move HackRF header P9 to B.CrtYd layer.* PCB: Change a Tg reference I missed.* PCB: Update footprints for parts with mismatched CAD->tape rotation.Adjust a few layer choice and line thickness bits.* PCB: Got cold feet, switched back to rectangular pads.* PCB: Add Eco layers to be visible and Gerber output.* PCB: Use aux origin for plotting, for tidier coordinates.* PCB: Output Gerber job file, because why not?* Schematic: Correct footprints for two reference-related components.* Schematic: Remove manfuacturer and part number for DNP component.* Schematic: Specify resistor value, manufacturer, part number for reference oscillator series termination.* PCB: Update netlist and footprints from schematic.* Netlist: Updated component values, footprints.* PCB: Nudge some components and traces to address DRC clearance violations.* PCB: Allow KiCad to update zone timestamps (again?!).* PCB: Generate *all* Gerber layers.* Schematic, PCB: Update revision to 20181025.* PCB: Adjust fab layer annotations orientation and font size.* PCB: Hide mounting hole reference designators on silk layer.* PCB: Shrink U1, U3 pads to get 0.2mm space between pads.* PCB: Set pad-to-mask clearance to zero, leave up to fab. Set minimum mask web to 0.2mm for non-black options.* PCB: Revise U1 pad shape, mask, paste, thermal drills.Clearance is improved at corner pads.* PCB: Tweak U3 for better thermal pad/drill/mask/paste design.* PCB: Change solder mask color to blue.* Schematic, PCB: Update revision to 20181029.* PCB: Bump minimum mask web down a tiny bit because KiCad is having trouble with math.* Update schematic* Remove unused board files.* Add LPC43xx functions.* chibios: Replace code with per-peripheral structs defining clocks, interrupts, and reset bits.* LPC43xx: Add MCPWM peripheral struct.* clock generator: Use recommended PLL reset register value.Datasheet recommends a value. AN619 is quiet on the topic, claims the low nibble is default 0b0000.* GPIO: Tweak masking of SCU function.I don't remember why I thought this was necessary...* HAL: Explicitly turn on timer peripheral clocks used as systicks, during init.* SCU: Add struct to hold pin configuration.* PAL: Add functions to address The Glitch.https://greatscottgadgets.com/2018/02-28-we-fixed-the-glitch/* PAL/board: New IO initialization codeDeclare initial state for SCU pin config, GPIOs. Apply initial state during PAL init. Perform VAA slow turn-on to address The Glitch.* Merge M0 and M4 to eliminate need for bootstrap firmwareDuring _early_init, detect if we're running on the M4 or M0.If M4: do M4-specific core initialization, reset peripherals, speed up SPIFI clock, start M0, go to sleep.If M0: do all the other things.* Pins: Miscellaneous SCU configuration tweaks.* Little code clarity improvement.* bootstrap: Remove, not necessary.* Clock Manager: Large re-working to support external references.* Clock Manager: Actually store chosen clock referenceSimilarly-named local was covering a member and discarding the value.* Clock Manager: Reference type which contains source, frequency.* Setup: Display reference source, frequency in frequency correction screen.* LPC43xx API: Add extern "C" for use from C++.* Use LPC43xx API for SGPIO, GPDMA, I2S initialization.* I2S: Add BASE_AUDIO_CLK management.* Add MOTOCON_PWM clock/reset structure.* Serial: Fix dumb typos.* Serial: Remove extra reference operator.* Serial: Cut-and-paste error in structure type name.* Move SCU structure from PAL to LPC43xx API.It'd be nice if I gave some thought to where code should live before I commit it.* VAA power: Move code to HackRF board fileIt doesn't belong in PAL.* MAX5 CPLD: Add SAMPLE and EXTEST methods.* Flash image: Change packing scheme to use flash more efficiently.Application is now a single image for both M4 bootstrap and M0.Baseband images come immediately after application binary. No need to align to large blocks (and waste lots of flash).* Clock Manager: Remove PLL1 power down function.* Move and rename peripherals reset function to board module.* Remove unused peripheral/clock management.* Clock Manager: Extract switch to IRC into separate function.* Clock Manager: More explicit shutdown of clocks, clock generator.* Move initialization to board module.* ChibiOS: Rename "application" board, add "baseband" board.There are now two ChibiOS "boards", one which runs the application and does the hardware setup. The other board, "baseband", does very little setup.* Clock Manager: Remove unused crystal enable/disable code.* Clock Manager: Restore clock configuration to SPIFI bootloader state before app shutdown.* Reset peripherals on app shutdown.Be careful not to reset M0APP (the core we're running on) or GPIO (which is holding the hardware in a stable state).* M4/baseband hal_lld_init: use IDIVA, which is configured earlier by M0.This was causing problems during restart into HackRF mode. Baseband hal_lld_init changed M4 clock from IDIVA (set by M0) to PLL1, which was unceremoniously turned off during shutdown.* Audio app: Stop audio PLL on shutdown.* M4 HAL: Make LPC43XX_M4_CLK_SRC optional.This was changing the BASE_M4_CLK when a baseband was run.* LPC43xx C++ layer: Fix IDIVx constructor IDIV narrow field width.* Application board: hide the peripherals_reset function, as it isn't useful except during hardware init.* Consolidate hardware init code to some degree.ClockManager is super-overloaded and murky in its purpose.Migrate audio from IDIVC to IDIVD, to more closely resemble initial clock scheme, so it's simpler to get back to it during shutdown.* Migrate some startup code to application board.* Si5351: Use correct methods for reset().update_output_enable_control() doesn't reset the enabled outputs to the reset state, unless the object is freshly initialized, which it isn't when performing firmware shutdown.For similar reasons, use set_clock_control() instead of setting internal state and then using the update function.* GPIO: Set SPIFI CS pin to match input buffer state coming out of bootloader.* Change application board.c to .cpp, with required dependent changes* Board: Clean up SCU configuration code/data.* I2S: Add shutdown code and use it.* LPC43xx: Consolidate a bunch of structures that had been scattered all over....because I'm an undisciplined coder.* I2S: Fix ordering of branch and base clock disable.Core was hanging, presumably because the register interface on the branch/peripheral was unresponsive after the base clock was disabled.* Controls: Save and expose raw navigation wheel switch stateI need to do some work on debouncing and ignoring simultaneous key presses.* Controls: Add debug view for switches state.* Controls: Ignore all key presses until all keys are released.This should address some mechanical quirks of the navigation wheel used on the PortaPack.* Clock Manager: Wait for only the necessary PLL to lock.Wasn't working on PortaPacks without a built-in clock reference, as that uses the other PLL.TODO: Switching PLLs may be kind of pointless now...* CMake: Pull HackRF project from GitHub and build.* CMake: Remove commented code.* CMake: Clone HackRF via HTTPS, not SSH.* CMake: Extra pause for slow post-DFU firmware boot-up.* CMake: TODO to fix SVF/XSVF file source.* CMake: Ask HackRF hackrf_usb to make DFU binary.* Travis-CI: Add dfu-util, now that HackRF firmware is being built for inclusion.* Travis-CI: Update build environment to Ubuntu xenialPreviously Trusty.* Travis-CI: Incorrectly structured my request for dfu-util package.I'm soooo talented.* ldscript: Mark flash, ram with correct R/W/X flags.* ldscript: Enlarge M0 flash region to 1Mbyte, the size of the HackRF SPI flash.* Receiver: Hide PPM adjustment if clock source is not HackRF crystal.* Documentation: Update product photos and README.* Documentation: Add TCXO feature to README description.* Application: Rearrange files to match HAVOC directory structure.* Map view in AIS (#213)* Added GeoMapView to AISRecentEntryDetailView* Added autoupdate in AIS map* Revert "Map view in AIS (#213)"This reverts commit 262c030224b9ea3e56ff1c8a66246e7ecf30e41f.This commit will be cherry-picked onto a clean branch, then re-committed after a troublesome pull request is reverted.* Revert "Upstream merge to make new revision of PortaPack work (#206)"This reverts commit 920b98f7c9a30371b643c42949066fb7d2441daf.This pull request was missing some changes and was preventing firmware from functioning on older PortaPacks.* CPLD: Pull bitstream from HackRF project.* SGPIO: Identify pins on CPLD by their new functions. Pull down HOST_SYNC_EN.* CPLD: Don't load HackRF CPLD bitstream into RAM.Trying to converge CPLD implementations, so this shouldn't be necesssary. HOWEVER, it would be good to *check* the CPLD contents and provide a way to update, if necessary.* CPLD: Tweak clock generator config to match CPLD timing changes in HackRF.* PinConfig: Drive CPLD pins correctly.* CMake: Use jboone/hackrf master branch, now that CPLD fixes are there.* CMake: Fix HackRF CPLD SVF dependency.Build would break on the first pass, but work if you restarted make.* CMake: Fix my misuse of the HackRF CMake configuration -- was building from too deep in the directory tree* CMake: Work-around for CMake 3.5 not supporting ExternalProject_Add SOURCE_SUBDIR.* CMake: Choose a CMP0005 policy to quiet CMake warnings.* Settings: Show active clock reference. Only show PPM adjustment for HackRF source.* Radio Settings: Change reference clock text color.Make consistent color with other un-editable text.TODO: This is a bit of a hack to get ui::Text objects to support custom colors, like the Label structures used elsewhere.  
>Added LGE app, nothing to see hereUpdate button in signal gen now works for shape change  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-01  
Contributor(s):  
Maescool  
>Upstream merge to make new revision of PortaPack work (#206)* Power: Turn off additional peripheral clock branches.* Update schematic with new symbol table and KiCad standard symbols.Fix up wires.* Schematic: Update power net labels.* Schematic: Update footprint names to match library changes.* Schematic: Update header vendor and part numbers.* Schematic: Specify (arbitrary) value for PDN# net.* Schematic: Remove fourth fiducial. Not standard practice, and was taking up valuable board space.* Schematic: Add reference oscillator -- options for clipped sine or HCMOS output.* Schematic: Update copyright year.* Schematic: Remove CLKOUT to CPLD. It was a half-baked idea.* Schematic: Add (experimental) GPS circuit.Add note about charging circuit.Update date and revision to match PCB.* PCB: Update from schematic change: now revision 20180819.Diff was extensive due to net renumbering...* PCB: Fix GPS courtyard to accommodate crazy solder paste recommendation in integration manual.PCB: Address DRC clearance violation between via and oscillator pad.* PCB: Update copyright on drawing.* Update schematic and PCB date and revision.* gitignore: Sublime Text editor project/workspace files* Power: Power up or power down peripheral clock at appropriate times, so firmware doesn't freeze...* Clocking: Fix incorrect shift for CGU IDIVx_CTRL.PD field.* LPC43xx: Add CGU IDIVx struct/union type.* Power: Switch off unused IDIV dividers. Make note of active IDIVs and their use.* HackRF Mode: Upgrade firmware to 2018.01.1 (API 1.02)* MAX V CPLD: Refactor class to look more like Xilinx CoolRunner II CPLD class.* MAX V CPLD: Add BYPASS, SAMPLE support.Rename enter_isp -> enable, exit_isp -> disable.Use SAMPLE at start of flash process, which somehow addresses the problem where CFM wouldn't load into SRAM (and become the active bitstream) after flashing.* MAX V CPLD: Reverse verify data checking logic to make it a little faster.* CPLD: After reprogramming flash, immediately clamp I/O signals, load to SRAM, and "execute" the new bitstream.* Si5351: Refactor code, make one of the registers more type-safe.Clock Manager: Track selected reference clock source for later use in user interface.* Clock Manager: Add note about PPM only affecting Si5351C PLLA, which always runs from the HackRF 25MHz crystal.It is assumed an external clock does not need adjustment, though I am open to being convinced otherwise...* PPM UI: Show "EXT" when showing PPM adjustment and reference clock is external.* CPLD: Add pins and logic for new PortaPack hardware feature(s).* CPLD: Bitstream to support new hardware features.* Clock Generator: Add a couple more setter methods for ClockControl registers.* Clock Manager: Use shared MCU CLKIN clock control configuration constant.* Clock Manager: Reduce MCU CLKIN driver current. 2mA should be plenty.* Clock Manager: Remove redundant clock generator output enable.* Bootstrap: Remove unnecessary ldscript hack to locate SPIFI mode change code in RAM.* Bootstrap: Get CPU operating at max frequency as soon as possible.Update SPIFI speed comment.Make some more LPC43xx types into unions with uint32_t.* Bootstrap: Explicitly configure IDIVB for SPIFI, despite LPC43xx bootloader setting it.* Clock Manager: Init peripherals before CPLD reconfig. Do the clock generator setup after, so we can check presence of PortaPack reference clock with the help of the latest CPLD bitstream.* Clock Manager: Reverse sense of conditional that determines crystal or non-crystal reference source. This is for an expected upcoming change where multiple external options can be differentiated.* Bootstrap: Consolidate clock configuration, update SPIFI rate comment.* Clock Manager: Use IDIVA for clock source for all peripherals, instead of PLL1. Should make switching easier going forward.Don't use IRC as clock during initial clock manager configuration. Until we switch to GP_CLKIN, we should go flat out...* ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution.* PortaPack IO: Expose method to set reference oscillator enable pin.* Pin configuration: Do SPIFI pin config with other pins, in preparation for eliminating separate bootloader.* Pin configuration: Disable input buffers on pins that are never read.* Revert "ChibiOS M0: Change default clock speed to 204MHz, since bootstrap now maxes out clock speed before starting M0 execution."This reverts commit c0e2bb6cc4cc656769323bdbb8ee5a16d2d5bb03.* Remove unused board files.* Add LPC43xx functions.* chibios: Replace code with per-peripheral structs defining clocks, interrupts, and reset bits.* LPC43xx: Add MCPWM peripheral struct.* clock generator: Use recommended PLL reset register value.Datasheet recommends a value. AN619 is quiet on the topic, claims the low nibble is default 0b0000.* GPIO: Tweak masking of SCU function.I don't remember why I thought this was necessary...* HAL: Explicitly turn on timer peripheral clocks used as systicks, during init.* SCU: Add struct to hold pin configuration.* PAL: Add functions to address The Glitch.https://greatscottgadgets.com/2018/02-28-we-fixed-the-glitch/* PAL/board: New IO initialization codeDeclare initial state for SCU pin config, GPIOs. Apply initial state during PAL init. Perform VAA slow turn-on to address The Glitch.* Merge M0 and M4 to eliminate need for bootstrap firmwareDuring _early_init, detect if we're running on the M4 or M0.If M4: do M4-specific core initialization, reset peripherals, speed up SPIFI clock, start M0, go to sleep.If M0: do all the other things.* Pins: Miscellaneous SCU configuration tweaks.* Little code clarity improvement.* bootstrap: Remove, not necessary.* Clock Manager: Large re-working to support external references.* Fix merge conflicts  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-12  
Contributor(s):  
furrtek  
>Bias-T now works in capture modeSimplified soundboard app, still some work to doMerge remote-tracking branch 'upstream/master'  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-06  
Contributor(s):  
furrtek  
Furrtek  
>Testing external clock detection and auto-switchSimplified audio spectrum computation and transferACARS RX in debug modeDisabled ABI warningsUpdated binary  
>Started work on ACARS RXAdded ACARS frequencies fileMoved non-implemented apps menu items down  
>Ext clock detect bugfix attempt  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-05  
Contributor(s):  
furrtek  
>Added tone key mix ratio in Settings -> AudioRenamed Setup to SettingsUpdated binary  
>Finally found what was eating all the RAM :DRe-enabled the tone key selector in SoundboardSoundboard now uses OutputStream, like ReplayConstexpr'd a bunch of consts which were going to BSS sectionExiting an app now goes back to main menuCleaned up Message array  
>Added cursor to audio spectrum view  
>Added an audio FFT view in Wideband FM receiveTried speeding up fill_rectangle for clearing the waveform widget  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-03  
Contributor(s):  
furrtek  
>Added some skeletonsRenamed "Scanner" to "Search"Modified splash bitmapDisabled Nuoptix TX  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-02  
Contributor(s):  
furrtek  
>Added basic APRS transmitAdded goertzel algoUpdated binary  
>Added support for multiple sample rates in IQ recordSupport for any sample rate <= 500k in IQ replayFixed bias-t power not activating in TXRemoved RSSI pitch output option (awful code)Udated binary  
>Fixed mic tx not working the first time it was enteredFixed SD card FAT wipe (buffer size too big)Cleared some warnings from ADSB rxUpdated binary  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-01  
Contributor(s):  
furrtek  
>Added Bias-T toggle confirmationBacklight setting save bugfixUpdated binary  
>Added back scanning in BHT TXAdded file creation date display in File Manager  
>Added bias-T status iconMerged radio settings in one screen  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-12  
Contributor(s):  
furrtek  
Furrtek  
>Replay of IQ files ! :DAdded icons and colors for commonly used files in FilemanFileman can filter by file extensionBugfix: Fileman doesn't crash anymore on renaming long file namesUpdated binary  
>Added RF amp setting in ADS-B RXRemoved frequency step, added LNA setting in ReplayAnother menu bugfix :(Updated binary  
>Added loop option in Replay appUpdated binary  
>Update README.md  
>Added range file and range type to frequency manager (mainly for jammer)Made MenuView use less widgets, hopefully preventing crashes with largelistsFixed M10 sonde crash on packet receiveUpdated about screenUpdated binary  
>Added back frequency display for CTCSSAttempted to fix replay, just fixed StreamBuffer read() and addedwaterfall display...Updated binary  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-11  
Contributor(s):  
furrtek  
Furrtek  
>Tone generator class  
>Added tone keys for some wireless mic brandsRenamed CTCSS stuff to Tone keyChanged PTT key in mic TX (was left, now right) to allow easier exitMic samplerate bumped to 48kHzUpdated binary  
>Added CTCSS decoder in NFM RXRSSI output is now pitch instead of PWMDisabled RSSI output in WBFM mode  
>Added workaround for the CPLD overlay issue in tx modeSet back mic samplerate to 24kHz because 48kHz was poop :(  
>Fixed freeze in TouchTunes scanMade adsb_map.py compatible with Python 3  
>Update README.md  
>Fusion !  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-10  
Contributor(s):  
furrtek  
Furrtek  
>Added keyfob UI and debug functionsFixed hex display truncated to 32 bits instead of 64Updated binary  
>Radiosonde RX now understands Meteomodem's M10 correctlyUpdated binary  
>Updated main.cpp  
>Added wav file viewerFileman open now allows going into subdirectoriesUpdated binary  
>Started working on radiosonde RXRemoved some warningsBetter handling of absent map file in GeoMap ?  
>Updated ui_widget.cpp  
>Added "test app" as a draft zone for... stuffAdded second signature for M2K2 radiosonde  
>Added cursors in waveform widgetAuto unit string format function  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-09  
Contributor(s):  
furrtek  
Furrtek  
>AFSK RX works (only Bell202 for now)AFSK RX always logs to fileRemoved frequency and bw settings from modem setup viewUpdated binaryBugfix: Binary display shifted one bitBugfix: Frequency manager views freezing if SD card not presentBugfix: Menuview blinking arrow not showing up at the right positionBugfix: Freeze if console written to while it's hiddenBroken: LCR TX, needs UI update  
>Added file manager  
>Update README.md  
>Date and time display widgetDisabled handwriting text input (not that useful for now)Bugfix: Trim long filenames in filemanSlight cleanup of 7-seg display widget  
>Added remaining buttons for TouchTunes remoteLCR transmit UI cleanupCC1101 data whitening functionUniformized tx progress message handling  
>Added TouchTunes remote  
>Fixed EPAR transmit  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-08  
Contributor(s):  
furrtek  
Furrtek  
>Started writing (copying...) AFSK RXEncoders: removed bit duration setting (frame duration is more useful)  
>Added tabs to BHT TX and JammerUpdated firmware binary  
>Added tabs to RDS tx appFixed callsign not updating in ADSB tx  
>Fixed ADSB TX frame rotation  
>ADSB position decodingDate and time string format functionBinary update  
>Updated firmware binary  
>Made a GeoPos widget for lon/lat/alt entry and display (APRS...)Cleaned up the GeoMap view, can be used as input  
>BW setting in TX view should now be used everywhereJammer center and width value editing bugfix  
>Update README.md  
>ADSB RX now works \o/Added tabs in RDS TX, multiple groups can be sent at onceBugfix: text not updating on UI after text prompt  
>ADSB RX text color bugfixADSB RX entries now "age" after 10 and 20 seconds  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-07  
Contributor(s):  
furrtek  
Furrtek  
>Added velocity/bearing ADS-B frame for txAdded compass widgetManchester encoder  
>ADS-B TX works well enough for dump1090 and gr-air-modesHooked ADS-B RX to baseband instead of debug IQ file, not tested  
>Added map display view (GeoMapView)SigGen duration bugfix  
>Started writing TabViewLoopable NumberField  
>Update README.md  
>ADS-B receive app debug code  
>"CW generator" and "Whistle" merged in "Signal generator"Added wave shape selection and tone frequency auto-updateConverted color icons to B&W  
>Moved screenshots  
>Merge remote-tracking branch 'upstream/master'  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-06  
Contributor(s):  
furrtek  
>Replay buffer size and samplerate adjustment  
>Frequency manager listsScanner approximately 6.3% less buggy with wide ranges  
>Merge remote-tracking branch 'upstream/master'Base class for text entry  
>Text entry should be more stableText entry now allows for strings greater than 28 charsFrequency manager save with name bugfix  
>OutputStream (file M0 -> M4 radio) now worksDisabled numbers station for now (too buggy, low priority)  
>Using new CPLD data (fixes spectrum mirroring)Scanner bugfix for wide rangesAdded squelch parameter for NFM receiverAdjustment to Vumeter widget rendering  
>Support for frequency manager categories (as files)Base class for frequency manager viewsMenuview clear/add bugfix  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-05  
Contributor(s):  
furrtek  
>Started work on APRS (AX25)  
>Reorganized menus  
>More ADS-B TX experimentationLots of junk added in Numbers Station regarding voice filesRemoved warnings caused by unfinished ADS-B function  
>Scanner: Added last locked frequencies listAdded back squelch to NFM receiverScanner: cleanupWidgets: VU-meter cleanup  
>Missing image files  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-04  
Contributor(s):  
furrtek  
Furrtek  
>POCSAG address filter now ignores alpha messagesExperimenting with FIFOs for replay...  
>Added address filter in POCSAG RXChanged POCSAG log formatConsole widget knows red, green and blue now  
>POCSAG RX saves ignored addressMade AFSK code more generic (not tied to LCR)Added modem presets and more options in AFSK setupString-ized and simplified LCR UI codeSimplified AFSK baseband code, made to always work on 16bit words  
>Whistle now worksMoved BW widget in txviewString-ized LCR and AFSK message generator  
>Update README.md  
>Shameful commit. Fixed HackRF mode not working...Sync'd with Sharebrained's repo, no more SIMD warnings  
>Reverted to SIMD macros to fix FM RX (again)  
>Started adding coaster pager/EZRadioPro TXBHT XY TX sequencer  
>Made back button always focusable with left key  
>Commented out Play Dead screen on startup  
>Simplified LCR code a bitSplit modem into modem and serializerFrequency string formatter  
>Coaster pager address scanMerged tone setups  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-03  
Contributor(s):  
furrtek  
Furrtek  
>Added DCS parity table and generator tool  
>Added microphone TX (very basic for now)  
>Bitmap preview in SSTV TX  
>Reduced audio tx FIFO refill sizeLast received POCSAG address is auto loaded in POCSAG tx  
>SSTV transmit beta (320x256 24bpp Scottie 2 only)  
>Microphone tx is mostly working, Voice activation, PTT, CTCSS...Transmit bandwidth bugfixTX LED is now only lit when using rf ampVU-meter widgetAdded gain parameter for baseband audio TX  
>Tones bugfix, numbers station voice files search  
>Update README.md  
>Added more SSTV modesA bit more work done on Replay (still not enabled)  
>Added roger beep option in mic TX  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-02  
Contributor(s):  
furrtek  
Furrtek  
>Morse TX foxhunt codes are working  
>Morse special chars and tx duration indication  
>Finished jammer modesShaved off a few kBs by using the Labels widget  
>CTCSS in soundboard. 24 jammer chs instead of 9.Soundboard random mode now cares about loop option.Started documenting UI.  
>Encoders, Nuoptix DTMF and RDS transmitters now use TransmitterViewBigger buttons in AlphanumView, 3 pagesScary yellow stripes around TransmitterView  
>Jammer manual set range 2 & 3 bugfixMenu capture/replay confusion bugfix  
>Close Call should be more accurateMerged close call and wideband spectrum baseband processors  
>Icons and icon tool update  
>Added categories for Frequency ManagerVery bad memory leak fix in MenuView  
>POCSAG TX (with fixed message for testing)  
>Improved POCSAG receiver reliability  
>Symfield widget auto-initsADS-B emergency frame  
>Utility: CW generator  
>"Labels" widget  
>Reverted WFM mode to working stateTXView in ADSB TXLockable TXViewPOCSAG TX bugfix with Alphanum and Numeric onlyTesting Labels widget  
>POCSAG TX text and bitrate can be changedModal view message can be multiline now  
>Update README.md  
>POCSAG RX now runs at 3.072MHz, like NFM audio  
>Wiki link  
>POCSAG TX: Support for numeric only and address only messages  
>Save before cleanup  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-01  
Contributor(s):  
furrtek  
Furrtek  
>POCSAG bitrate selection and logging toggleSmall checkboxes  
>Jammer bugfix: now produces all the right channels  
>Udpdated jammer baseband code, should work again  
>Digital mode for waveform widget, 2.4GHZ WLAN channels in jammer  
>Added SD card wiper toolFrequency manager now creates FREQMAN.TXT if not foundMoved graphics files  
>Frequency manager empty file bugfix  
>More pretty icons, BW setting change in BHT TX  
>Update README.md  
>Transmitter config widgetFrequency manager duplicate alertTone sets  
>Added waveform widget and a frequency field in encoders tx  
>Fixed encoders TX locking up, more icons  
>Commit replay stuff before sync  
>Merge 'upstream/master' - At least it builds...  
>Added bitrate option for POCSAG baseband, PWMRSSI frequency optionSplit SD card wiper appCleanup for -Weffc++  
>Reverted to original CPLD data  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-12  
Contributor(s):  
furrtek  
Furrtek  
>Re-enabled closecall even if it's still not working wellRDS PSN works again but update issue (UI ?)Moved CTCSS stuff to dedicated file  
>Fixed proc_tones skipping last toneSplit ui_bht to bht  
>Pretty icons  
>Started work on ADS-B TX baseband processor  
>Added utilities > Frequency manager + load/save  
># This is a combination of 2 commits.# The first commit's message is:Updated RDS transmitter: flags, PI and date/timeMerging baseband audio tone generatorsMerging DTMF baseband with "tones" basebandAdded stealth transmit modeApp flash section bumped to 512kRX and TX LEDs are now usedPlay dead should work again, added login optionMorse frame gen. for letters and fox hunt codesMerged EPAR with XylosMade EPAR use encoders for frame gen.Moved OOK encoders data in encoders.hppSimplified about screen, ui_about_demo.* files are still thereBHT city DB, keywords removedBHT cities DB, keywords removedUpdate README.mdRDS radiotext and time group generators# This is the 2nd commit message:Update README.md  
>Screenshot mosaïque  
>Splash was stuck  
>Update README.md  
>3D buttons, to make UI clearer  
>Numbers station works, very basicAdded utilities, whip antenna length calculatorModal errors/abort  
>Restoring jammer and RDS functionalities, please wait...Started work on frequency manager and numbers station simulator  
>Wavfile class  
>Playdead default sequence and validity check  
>Fixed messup after last squashed commits  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-11  
Contributor(s):  
furrtek  
>Started ADS-B TX UI and frame encoding  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-09  
Contributor(s):  
furrtek  
Furrtek  
>Fixed xx2262 remote encoder defSymField now shows symbol chars  
>Update README.md  
>Added CTCSS in Soundboard  
>Added Nuoptix DTMF sync transmit (Disney parades, light shows...)Soundboard ignores stereo files  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-08  
Contributor(s):  
furrtek  
Furrtek  
>Added basic POCSAG receiverAdded Yes/no modal screen (for future tx warnings)  
>Manual frequency input in POCSAG RXChanged firmware file name  
>Soundboard: Arbitrary samplerate support for wave filesScreenshots  
>Bugfix: POCSAG alphanum messages not showingBugfix: Range limit for afsk config  
>Added raw ASCII char field in keyboard view  
>OOK transmit is mostly working, bit durations are wrongSimplified messages carrying data (uses shared_memory instead)Added SymField widget (bitfield, symbol field...)Added some space for baseband codeBMP palette loading bugfix  
>AudioTX, fixed about screen and an LCR address list bug  
>Update README.md  
>Testing OOK TX baseband module  
>Added Soundboardfile.cpp: scan_root_filesproc_audiotx.cpp: bandwidth settingui_widget.cpp: button on_focus  
>Wrote most of the Encoders TX app (lacks baseband module)Fixed menu scroll glitchAdded set_range to NumberField widget  
>Merge remote-tracking branch 'upstream/master'# Conflicts:#	firmware/application/bitmap.hpp#	firmware/application/receiver_model.cpp#	firmware/application/receiver_model.hpp#	firmware/application/touch.hpp#	firmware/application/ui_setup.cpp#	firmware/baseband/proc_ais.hpp#	firmware/baseband/proc_ert.hpp#	firmware/bootstrap/CMakeLists.txt#	firmware/common/portapack_persistent_memory.cpp#	firmware/common/portapack_persistent_memory.hpp  
>More AFSK options, scan lists,  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-07  
Contributor(s):  
furrtek  
Furrtek  
>Merge remote-tracking branch 'upstream/master'Conflicts:	firmware/Makefile	firmware/application/Makefile	firmware/application/event_m0.cpp	firmware/application/ui_setup.cpp	firmware/application/ui_setup.hpp	firmware/baseband/baseband_thread.cpp	firmware/baseband/baseband_thread.hpp	firmware/bootstrap/CMakeLists.txt	firmware/common/message.hpp	firmware/common/portapack_shared_memory.hpp	hardware/.gitignore  
>Cleaned up Xylos TX, J/N works again  
>Scrollable menuview  
>Fixed LCR transmit and AFSK baseband module  
>Fixed LCR scan and alt format, console widget, text input autotrim  
>Update README.md  
>Added repeat setting for AFSK TX, fixed LCR scan, cleaned up LCRAdded max setting for progressbars, default = 100  
>Added PWM RSSI output for NBFM and WFM  
>Sync with Sharebrained's fw, only Xylos TX works for now  
>Added frequency manager skeleton, LCR alt encoding, GPS jammer  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-05  
Contributor(s):  
furrtek  
Furrtek  
>LCR scanner and bugfixes, keyboard bugfixes  
>Improved close call precision  
>Added EPAR transmit (slow FSK), bit bug for now  
>Started close call dev  
>Added carrier wake up in Xylos TX and cute icons  
>Testing Messagepack for saving/loading stuff from SD card  
>Handwriting fixes  
>Merge  
>Update README.md  
>RDS radiotext transmit (group 2A)Keyboard/Unistroke text input method selection  
>Fixed LCR and Xylos transmitters  
>Close call tuning fix, more UI elements  
>More messagepack  
>Handwriting recognition  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-04  
Contributor(s):  
furrtek  
Furrtek  
>"At least it builds !"  
>Fixed module loading (again), only audio tx works for now  
>Update README.md  
>Merge remote-tracking branch 'upstream/master'Conflicts:	firmware/application/Makefile	firmware/application/core_control.cpp	firmware/application/touch.cpp	firmware/application/ui_debug.cpp	firmware/application/ui_debug.hpp	firmware/application/ui_navigation.cpp	firmware/baseband/baseband_thread.cpp  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-02  
Contributor(s):  
furrtek  
Furrtek  
>"At least it builds, now"  
>Merge remote-tracking branch 'upstream/master'Conflicts:	firmware/application/Makefile	firmware/application/analog_audio_app.cpp	firmware/application/analog_audio_app.hpp	firmware/application/event.cpp	firmware/application/irq_ipc.hpp	firmware/application/portapack.hpp	firmware/application/receiver_model.cpp	firmware/application/receiver_model.hpp	firmware/application/recent_entries.cpp	firmware/application/string_format.hpp	firmware/application/ui_debug.cpp	firmware/application/ui_debug.hpp	firmware/application/ui_menu.cpp	firmware/application/ui_navigation.cpp	firmware/application/ui_navigation.hpp	firmware/application/ui_receiver.cpp	firmware/application/ui_receiver.hpp	firmware/application/ui_sd_card_status_view.cpp	firmware/application/ui_sd_card_status_view.hpp	firmware/application/ui_setup.cpp	firmware/application/ui_setup.hpp	firmware/application/ui_spectrum.hpp	firmware/baseband-tx/dsp_fir_taps.cpp	firmware/baseband-tx/dsp_fir_taps.hpp	firmware/baseband-tx/irq_ipc_m4.cpp	firmware/baseband-tx/irq_ipc_m4.hpp	firmware/baseband-tx/proc_audiotx.cpp	firmware/baseband/Makefile	firmware/baseband/audio_output.cpp	firmware/baseband/audio_output.hpp	firmware/baseband/block_decimator.hpp	firmware/baseband/dsp_decimate.cpp	firmware/baseband/dsp_decimate.hpp	firmware/baseband/dsp_demodulate.cpp	firmware/baseband/dsp_demodulate.hpp	firmware/baseband/dsp_fir_taps.cpp	firmware/baseband/irq_ipc_m4.cpp	firmware/baseband/irq_ipc_m4.hpp	firmware/baseband/proc_am_audio.cpp	firmware/baseband/proc_am_audio.hpp	firmware/baseband/proc_nfm_audio.cpp	firmware/baseband/proc_nfm_audio.hpp	firmware/baseband/proc_wfm_audio.cpp	firmware/baseband/proc_wfm_audio.hpp	firmware/baseband/spectrum_collector.hpp	firmware/common/dsp_fir_taps.cpp	firmware/common/dsp_fir_taps.hpp	firmware/common/event.hpp	firmware/common/message.hpp	firmware/common/ui_painter.cpp	firmware/common/ui_painter.hpp  
>Update README.md  
>Merge fixing, commit to catch up on recent files  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-01  
Contributor(s):  
furrtek  
Furrtek  
>Module loading should work againModules won't load if already loaded (dirty footprint hack)  
>Added missing files, ENUMed modulation modes  
>Thanks  
>UI options: backlight auto-off, splash screen toggle  
>SYNC  
>Xylos (CCIR tones) TX, jammer update, SD card mod loadXylos TX (CCIR tones) ;)Jammer update, still buggy and inefficientSD card module loader update  
>Update README.md  
>Completely useless "about" screenPaved road for talking Xylos RX and loggerAdded test button for Xylos TXFixed jammer crashing after loading second time  
>Sync  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-11  
Contributor(s):  
furrtek  
Furrtek  
>Jammer ramp modeJammer range splitting, hopping. Only 1MHz wide splits for now.  
>Update README.md  
>Dynamic baseband module loading from SD card  
>Merged remote-tracking branch 'upstream/master'  
>Jammer UIDrew jammer UIAdded presets for French GSM operators and a few other funny frequenciesAdded minimalist jamming baseband code  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-09  
Contributor(s):  
furrtek  
Furrtek  
>Play dead init  
>Splash screen and Play Dead functionality  
>Started adding AFSK modulator options, cleaning up LCR TX  
>Checkboxes, more AFSK options  
>Update README.md  
>Play dead actually works, fixed 7bit AFSK, AFSK repeat, started whistle mode  
>Added AFSK BW and repeat parameters  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-08  
Contributor(s):  
furrtek  
Furrtek  
>Integerized the waveform table  
>Update README.md  
>Savestate ! RDS (only PSN) tx  
>LCR in TEDI 1200/2400 AFSK transmit  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 

