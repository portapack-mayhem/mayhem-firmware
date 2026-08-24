# External apps description
External apps has 3 tiers.
Tier 0 is external in all builds.
Tier 1 is internal in PortaRf AND HackRf Pro
Tier 2 is internal only for HackRf Pro.

Put all joke or not necesarry apps to T0. Also Harmful apps (like jammer)
Put The most important and most interesting apps to T1.
Put other apps to T2.

# external_tierN.ld files
If you include anything in these files, those will be external for that tier. So if you want the given app to be external for the given tier, add it to the .ld file.

# ui_navigation.cpp
Include the tier specific headers in there, and add it to the corresponding menu entry. These will be INTERNAL for the given tier. 

# baseband iamges
Open the baseband/CmakeLists.txt and read the instructions there. There will be multi tiered places too. Put the baseband image to the right place in it. 
If you can make the baseband image external for the given tier if the app is external too. If multiple apps uses the same baseband, set the baseband to the correct tier. Like It is used ba a T1 and a T2 app, then the baseband must be T1!

# baseband::run_prepared_image(portapack::memory::map::m4_code.base());
Don't use this. This is for T0 tiers only. But since tiers can be changed, and devs may forget to change it, just use the baseband::run_image(portapack::spi_flash::image_tag_tpms);  It'll fall back for tiers that doesn't include it, so it'll be loaded from the ext app then.
