# PortaPack Mayhem

[![Build Status](https://travis-ci.com/eried/portapack-mayhem.svg?branch=master)](https://travis-ci.com/eried/portapack-mayhem) [![Nightly Release](https://github.com/eried/portapack-mayhem/actions/workflows/create_nightly_release.yml/badge.svg?branch=next)](https://github.com/eried/portapack-mayhem/actions/workflows/create_nightly_release.yml) [![CodeScene Code Health](https://codescene.io/projects/8381/status-badges/code-health)](https://codescene.io/projects/8381) [![GitHub All Releases](https://img.shields.io/github/downloads/eried/portapack-mayhem/total)](https://github.com/eried/portapack-mayhem/releases) [![GitHub Releases](https://img.shields.io/github/downloads/eried/portapack-mayhem/latest/total)](https://github.com/eried/portapack-mayhem/releases/latest) [![Docker Hub Pulls](https://img.shields.io/docker/pulls/eried/portapack.svg)](https://hub.docker.com/r/eried/portapack) [![Discord Chat](https://img.shields.io/discord/719669764804444213.svg)](https://discord.gg/tuwVMv3)  [![Check bounties!](https://img.shields.io/bountysource/team/portapack-mayhem/activity?color=%2333ccff&label=bountysource%20%28USD%29&style=plastic)](https://www.bountysource.com/teams/portapack-mayhem/issues)

This is a fork of the [Havoc](https://github.com/furrtek/portapack-havoc/) firmware, which itself was a fork of the [PortaPack](https://github.com/sharebrained/portapack-hackrf) firmware, an add-on for the [HackRF](http://greatscottgadgets.com/hackrf/). A fork is a derivate, in this case one that has extra features and fixes when compared to the older versions.

[<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_front.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview) [<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_inside.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview#portapack-internals)

*[PortaPack H2+HackRF+battery](https://s.click.aliexpress.com/e/_DmU7GQX) (clone) with a custom [3d printed case](https://github.com/eried/portapack-mayhem/wiki/H2-Enclosure)*

# Quick overview

If you are new to *HackRF+PortaPack+Mayhem*, there is an awesome introductory video by [Tech Minds](https://www.youtube.com/channel/UC9a8Z6Sp6eb2s3O79pX5Zvg) available:

[![Setup and overview](https://img.youtube.com/vi/kjFB58Y1TAo/0.jpg)](https://www.youtube.com/watch?v=kjFB58Y1TAo)

# Frequently Asked Questions

This repository expands upon the previous work by many people and aims to constantly add new features, bugfixes and generate documentation to make further development easier.  [Collaboration](https://github.com/eried/portapack-mayhem/wiki/How-to-collaborate) is always welcomed and appreciated.

## What to buy?

:heavy_check_mark: A recommended one is this [PortaPack H2](https://s.click.aliexpress.com/e/_DmU7GQX) pack, that includes everything you need. Sadly, the people making the H2 never made the updated schematics available (against the terms of the license). 

:heavy_check_mark: Another popular option is the clone of the [PortaPack H1](https://s.click.aliexpress.com/e/_Dkbqs2X).

:warning: Be cautious , *ask* the seller about compatibility with the latest releases. Look out for the description of the item, if they provide the firmware files for an older version or they have custom setup instructions, this means it might be **NOT compatible**, for example:

![image](https://user-images.githubusercontent.com/1091420/214579017-9ad970b9-0917-48f6-a550-588226d3f89b.png)

:warning: If it looks **too different**, this might mean that they are using their own recipe, check the [different models](https://github.com/eried/portapack-mayhem/wiki/PortaPack-Versions) in our wiki. For example all the H3 and clones of that version use their own version of the firmware. They do not contribute the changes back and eventually you will be left with a device that nobody maintains:

![image](https://user-images.githubusercontent.com/1091420/214581333-424900ee-26f8-4e96-be2f-69d8dc995ba9.png)

## Where is the latest version?

The current **stable release** is on the [![GitHub release (latest by date)](https://img.shields.io/github/v/release/eried/portapack-mayhem?label=Releases&style=social)](https://github.com/eried/portapack-mayhem/releases/latest) page. Follow the instructions you can find in the release description. The **latest (nightly) release** can be found [here](https://github.com/eried/portapack-mayhem/releases/).

## How can I collaborate
You can write [documentation](https://github.com/eried/portapack-mayhem/wiki), fix bugs and [answer issues](https://github.com/eried/portapack-mayhem/issues) or add new functionality. Please check the following [guide](https://github.com/eried/portapack-mayhem/wiki/How-to-collaborate) with details.

Consider that the hardware and firmware has been created and maintain by a [lot](https://github.com/mossmann/hackrf/graphs/contributors) of [people](https://github.com/eried/portapack-mayhem/graphs/contributors), so always try collaborating your time and effort first. For coding related questions, if something does not fit as an issue, please join our Discord by clicking the chat badge on [top](#portapack-mayhem).

[![Contributors](https://contrib.rocks/image?repo=eried/portapack-mayhem)](https://github.com/eried/portapack-mayhem/graphs/contributors)

To support the people behind the hardware, please buy a genuine [HackRF](https://greatscottgadgets.com/hackrf/) and [PortaPack](https://store.sharebrained.com/products/portapack-for-hackrf-one-kit).

## What if I really want something specific?
If what you need can be relevant in general, you can [request a feature](https://github.com/eried/portapack-mayhem/issues/new?labels=enhancement&template=feature_request.md).

You can create a bounty and invite people to your own bounty. This will incentivize coders to work on a new feature, solving a bug or even writting documentation. Start a bounty by [creating](https://github.com/eried/portapack-mayhem/issues/new/choose) or [choosing](https://github.com/eried/portapack-mayhem/issues/) an existing issue. Then, go to [Bountysource](https://www.bountysource.com/) and post a bounty using the link to that specific [issue](https://www.bountysource.com/teams/portapack-mayhem/issues).

Promote your bounty over our Discord by clicking the chat badge on [top](#portapack-mayhem).

## What if I need help?
First, check the [documentation](https://github.com/eried/portapack-mayhem/wiki). If you find a bug or you think the problem is related to the current repository, please open an [issue](https://github.com/eried/portapack-mayhem/issues/new/choose).

You can reach the [official community](https://www.facebook.com/groups/177623356165819) in Facebook, and our Discord by clicking the chat badge on [top](#portapack-mayhem).
