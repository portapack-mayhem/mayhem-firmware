> [!WARNING]
> __IF YOU'VE PAID FOR MAYHEM OR ANY PREPACKAGED VERSIONS, YOU'RE BEING SCAMMED.__ 
>
> The only legitimate link to our repositories is the [portapack-mayhem](https://github.com/portapack-mayhem/mayhem-firmware) organization on GitHub.

# PortaPack Mayhem

[![Nightly Release](https://github.com/portapack-mayhem/mayhem-firmware/actions/workflows/create_nightly_release.yml/badge.svg?branch=next)](https://github.com/portapack-mayhem/mayhem-firmware/actions/workflows/create_nightly_release.yml) [![CodeScene Code Health](https://codescene.io/projects/8381/status-badges/code-health)](https://codescene.io/projects/8381) [![GitHub All Releases](https://img.shields.io/github/downloads/portapack-mayhem/mayhem-firmware/total)](https://github.com/portapack-mayhem/mayhem-firmware/releases) [![GitHub Releases](https://img.shields.io/github/downloads/portapack-mayhem/mayhem-firmware/latest/total)](https://github.com/portapack-mayhem/mayhem-firmware/releases/latest) [![Docker Hub Pulls](https://img.shields.io/docker/pulls/eried/portapack.svg)](https://hub.docker.com/r/eried/portapack) [![Discord Chat](https://dcbadge.vercel.app/api/server/tuwVMv3?style=flat)](https://discord.gg/tuwVMv3)

This is a fork of the [Havoc](https://github.com/furrtek/portapack-havoc/) firmware, which itself was a fork of the [PortaPack](https://github.com/sharebrained/portapack-hackrf) firmware, an add-on for the [HackRF](http://greatscottgadgets.com/hackrf/). A fork is a derivate, in this case one that has extra features and fixes when compared to the older versions.

[<img src="https://raw.githubusercontent.com/wiki/portapack-mayhem/mayhem-firmware/img/hw_overview_h2_front.png" height="400">](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Hardware-overview) [<img src="https://raw.githubusercontent.com/wiki/portapack-mayhem/mayhem-firmware/img/hw_overview_h2_inside.png" height="400">](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Hardware-overview#portapack-internals)

*[PortaPack H2+HackRF+battery](https://grabify.link/ZYYPLB) (clone) with a custom [3d printed case](https://github.com/portapack-mayhem/mayhem-firmware/wiki/H2-Enclosure)*

# What is this?

If you are new to *HackRF+PortaPack+Mayhem*, check these:

[<img alt="The new HackRF Portapack H4M" src="https://img.youtube.com/vi/onQRdCITmuk/maxresdefault.jpg" width="701">](https://grabify.link/0JUHZ6)

[<img alt="It’s TOO Easy to Accidentally Do Illegal Stuff with This" src="https://img.youtube.com/vi/OPckpjBSAOw/maxresdefault.jpg" width="172">](https://grabify.link/X4D5TF) [<img alt="What is the HackRF One Portapack H2+?" src="https://img.youtube.com/vi/alrFbY5vxt4/maxresdefault.jpg" width="172">](https://grabify.link/9UZGEW) [<img alt="Beginner's Guide To The HackRF & Portapak With Mayhem" src="https://img.youtube.com/vi/H-bqdWfbhpg/maxresdefault.jpg" width="172">](https://grabify.link/5MU2VH) [<img alt="HackRF 101 : Everything You Need to Know to Get Started!" src="https://img.youtube.com/vi/xGR_PMD9LeU/maxresdefault.jpg" width="172">](https://grabify.link/C0J6ZR)

# Frequently Asked Questions

This repository expands upon the previous work by many people and aims to constantly add new features, bugfixes and generate documentation to make further development easier.  [Collaboration](https://github.com/portapack-mayhem/mayhem-firmware/wiki/How-to-collaborate) is always welcomed and appreciated.

## What to buy?

:heavy_check_mark: ![Static Badge](https://img.shields.io/badge/NEW-yellow) The fabulous [PortaPack H4M Mayhem](https://grabify.link/VPMPSL), featuring numerous improvements and accessories. Sold by our friends at [OpenSourceSDRLab](https://grabify.link/99SAMT). Join their giveaways on discord (check the badge on top).

:heavy_check_mark: A recommended one is this [PortaPack H2](https://grabify.link/ZYYPLB), that includes everything you need with the plastic case "inspired" on [this](https://github.com/portapack-mayhem/mayhem-firmware/wiki/H2-Enclosure).

:heavy_check_mark: Some individuals lean towards the [H2 with a metal enclosure](https://grabify.link/XNGTAP), but its advantages remain debated. Share your insights on our [wiki](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Hardware-overview). 

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
If what you need can be relevant in general, you can [request a feature](https://github.com/portapack-mayhem/mayhem-firmware/issues/new?labels=enhancement&template=feature_request.md). Alternatively, go to our Discord by clicking the chat badge on [top](#portapack-mayhem) and discuss there.

## What if I need help?
First, check the [documentation](https://github.com/portapack-mayhem/mayhem-firmware/wiki). If you find a bug or you think the problem is related to the current repository, please open an [issue](https://github.com/portapack-mayhem/mayhem-firmware/issues/new/choose).

You can reach the [official community](https://www.facebook.com/groups/177623356165819) in Facebook, and our Discord by clicking the chat badge on [top](#portapack-mayhem).
