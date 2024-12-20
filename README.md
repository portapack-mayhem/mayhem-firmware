<!---
> [!WARNING]
> __IF YOU'VE PAID FOR MAYHEM OR ANY PREPACKAGED VERSIONS, YOU'RE BEING SCAMMED.__ 
>
> The only legitimate link to our repositories is the [portapack-mayhem](https://github.com/portapack-mayhem/mayhem-firmware) organization on GitHub.--->
# PortaPack Mayhem

[![Nightly Release](https://github.com/portapack-mayhem/mayhem-firmware/actions/workflows/create_nightly_release.yml/badge.svg?branch=next)](https://github.com/portapack-mayhem/mayhem-firmware/actions/workflows/create_nightly_release.yml) [![CodeScene Code Health](https://codescene.io/projects/8381/status-badges/code-health)](https://codescene.io/projects/8381) [![GitHub All Releases](https://img.shields.io/github/downloads/portapack-mayhem/mayhem-firmware/total)](https://github.com/portapack-mayhem/mayhem-firmware/releases) [![GitHub Releases](https://img.shields.io/github/downloads/portapack-mayhem/mayhem-firmware/latest/total)](https://github.com/portapack-mayhem/mayhem-firmware/releases/latest) [![Docker Hub Pulls](https://img.shields.io/docker/pulls/eried/portapack.svg)](https://hub.docker.com/r/eried/portapack) [![Discord Chat](https://dcbadge.vercel.app/api/server/tuwVMv3?style=flat)](https://discord.gg/tuwVMv3)

This is a fork of the [Havoc](https://github.com/furrtek/portapack-havoc/) firmware, which itself was a fork of the [PortaPack](https://github.com/sharebrained/portapack-hackrf) firmware, an add-on for the [HackRF](http://greatscottgadgets.com/hackrf/). A fork is a derivate, in this case one that has extra features and fixes when compared to the older versions.

[<img src="https://github.com/user-attachments/assets/dea337ab-fb64-4a2a-b419-69afd272e815" height="400">](https://github.com/portapack-mayhem/mayhem-firmware/wiki/PortaPack-Versions#new-h4m-mayhem-edition) [<img src="https://camo.githubusercontent.com/5c1f1da0688240ac7b2ccca0c8dbfd1d73f2540741ad8b1828ba4d5ea68af248/68747470733a2f2f6769746875622d70726f64756374696f6e2d757365722d61737365742d3632313064662e73332e616d617a6f6e6177732e636f6d2f343339333937392f3239353533323731382d38653562363631632d663934362d346365652d386232642d3061363135663737313566342e706e67" height="400">](https://github.com/portapack-mayhem/mayhem-firmware/wiki/PortaPack-Versions#h2m-mayhem-edition)

# What is this?

If you are new to *HackRF+PortaPack+Mayhem*, check these:

[<img alt="The Latest HackRF & Portapak Combo - H4M The Flipper Zero Killer?" src="https://img.youtube.com/vi/Ew2qDgm2hf0/maxresdefault.jpg" width="701">](https://share.hackrf.app/6HKX9A)

[<img alt="It’s TOO Easy to Accidentally Do Illegal Stuff with This" src="https://img.youtube.com/vi/OPckpjBSAOw/maxresdefault.jpg" width="172">](https://share.hackrf.app/X4D5TF) [<img alt="HackRF Portapack H4M - Getting Started Guide" src="https://img.youtube.com/vi/wzP0zWi85SI/maxresdefault.jpg" width="172">](https://share.hackrf.app/F9MPOO) [<img alt="The new HackRF Portapack H4M" src="https://img.youtube.com/vi/onQRdCITmuk/maxresdefault.jpg" width="172">](https://share.hackrf.app/0JUHZ6) [<img alt="HackRF 101 : Everything You Need to Know to Get Started!" src="https://img.youtube.com/vi/xGR_PMD9LeU/maxresdefault.jpg" width="172">](https://share.hackrf.app/C0J6ZR)

# Frequently Asked Questions

This repository expands upon the previous work by many people and aims to constantly add new features, bugfixes and generate documentation to make further development easier.  [Collaboration](https://github.com/portapack-mayhem/mayhem-firmware/wiki/How-to-collaborate) is always welcomed and appreciated.

## What to buy?

<!---not direct to h4m but to opensourcesdrlab https://share.hackrf.app/TUOLYI---> 
:heavy_check_mark: ![Static Badge](https://img.shields.io/badge/NEW-yellow) The fabulous H4M [complete](https://share.hackrf.app/TUOLYI) or [upgrade](https://share.hackrf.app/FPLM1H), featuring numerous improvements and accessories. Sold by our friends at [OpenSourceSDRLab](https://share.hackrf.app/99SAMT). Join their giveaways on discord (check the badge on top).

:heavy_check_mark: A recommended one is this [PortaPack H2](https://www.ebay.com/itm/116382397447), that includes everything you need with the plastic case "inspired" on [this](https://github.com/portapack-mayhem/mayhem-firmware/wiki/3d-printed-enclosure).

:heavy_check_mark: Some individuals lean towards the [H2 with a metal enclosure](https://share.hackrf.app/LZPBH9), but its advantages remain debated. Share your insights on our [wiki](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Hardware-overview). 

:warning: Be cautious , *ask* the seller about compatibility with the latest releases. Look out for the description of the item, if they provide the firmware files for an older version or they have custom setup instructions, this means it might be **NOT compatible**, for example:

![image](https://user-images.githubusercontent.com/1091420/214579017-9ad970b9-0917-48f6-a550-588226d3f89b.png)

:warning: If it looks **too different**, this might mean that they are using their own recipe, check the [different models](https://github.com/portapack-mayhem/mayhem-firmware/wiki/PortaPack-Versions) in our wiki. For example all the H3 and clones of that version use their own version of the firmware. They do not contribute the changes back and eventually you will be left with a device that nobody maintains:

![image](https://user-images.githubusercontent.com/1091420/214581333-424900ee-26f8-4e96-be2f-69d8dc995ba9.png)

## Where is the latest version?

The current **stable release** is on the [![GitHub release (latest by date)](https://img.shields.io/github/v/release/portapack-mayhem/mayhem-firmware?label=Releases&style=social)](https://github.com/portapack-mayhem/mayhem-firmware/releases/latest) page. Follow the instructions you can find in the release description. The **latest (nightly) release** can be found [here](https://github.com/portapack-mayhem/mayhem-firmware/releases/).

## How can I collaborate
You can write [documentation](https://github.com/portapack-mayhem/mayhem-firmware/wiki), fix bugs and [answer issues](https://github.com/portapack-mayhem/mayhem-firmware/issues) or add new functionality. Please check the following [guide](https://github.com/portapack-mayhem/mayhem-firmware/wiki/How-to-collaborate) with details.

Consider that the hardware and firmware has been created and maintain by a [lot](https://github.com/mossmann/hackrf/graphs/contributors) of [people](https://github.com/portapack-mayhem/mayhem-firmware/graphs/contributors), so always try collaborating your time and effort first. For coding related questions, if something does not fit as an issue, please join our Discord by clicking the chat badge on [top](#portapack-mayhem).

[![Contributors](https://contrib.rocks/image?repo=portapack-mayhem/mayhem-firmware)](https://github.com/portapack-mayhem/mayhem-firmware/graphs/contributors)

To support the people behind the hardware, please buy a genuine [HackRF](https://greatscottgadgets.com/hackrf/) and [PortaPack](https://store.sharebrained.com/products/portapack-for-hackrf-one-kit).

## What if I really want something specific?
If what you need can be relevant in general, you can [request a feature](https://github.com/portapack-mayhem/mayhem-firmware/issues/new?assignees=&labels=enhancement&projects=&template=02_feature_request.yml). Alternatively, go to our Discord by clicking the chat badge on [top](#portapack-mayhem) and discuss there.

## What if I need help?
First, check the [documentation](https://github.com/portapack-mayhem/mayhem-firmware/wiki). If you find a bug or you think the problem is related to the current repository, please open an [issue](https://github.com/portapack-mayhem/mayhem-firmware/issues/new/choose).

You can reach the [official community](https://www.facebook.com/groups/177623356165819) in Facebook, and our Discord by clicking the chat badge on [top](#portapack-mayhem).
