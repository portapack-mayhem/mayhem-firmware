# PortaPack Mayhem

[![Build Status](https://travis-ci.com/eried/portapack-mayhem.svg?branch=master)](https://travis-ci.com/eried/portapack-mayhem) [![CodeScene Code Health](https://codescene.io/projects/8381/status-badges/code-health)](https://codescene.io/projects/8381) [![GitHub All Releases](https://img.shields.io/github/downloads/eried/portapack-mayhem/total)](https://github.com/eried/portapack-mayhem/releases) [![GitHub Releases](https://img.shields.io/github/downloads/eried/portapack-mayhem/latest/total)](https://github.com/eried/portapack-mayhem/releases/latest) [![Docker Hub Pulls](https://img.shields.io/docker/pulls/eried/portapack.svg)](https://hub.docker.com/r/eried/portapack) [![Discord Chat](https://img.shields.io/discord/719669764804444213.svg)](https://discord.gg/tuwVMv3)  [![Check bounties!](https://img.shields.io/bountysource/team/portapack-mayhem/activity?color=%2333ccff&label=bountysource%20%28USD%29&style=plastic)](https://www.bountysource.com/teams/portapack-mayhem/issues)

This is a fork of the [Havoc](https://github.com/furrtek/portapack-havoc/) firmware, which itself was a fork of the [PortaPack](https://github.com/sharebrained/portapack-hackrf) firmware, an add-on for the [HackRF](http://greatscottgadgets.com/hackrf/). A fork is a derivate, in this case one that has extra features and fixes when compared to the older versions.

[<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_front.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview) [<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_inside.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview#portapack-internals)

*[PortaPack H2+HackRF+battery](https://s.click.aliexpress.com/e/_9zbXMk) (clone) with a custom [3d printed case](https://github.com/eried/portapack-mayhem/wiki/H2-Enclosure)*

# Quick overview

If you are new to *HackRF+PortaPack+Mayhem*, there is an awesome introductory video by [Tech Minds](https://www.youtube.com/channel/UC9a8Z6Sp6eb2s3O79pX5Zvg) available:

[![Setup and overview](https://img.youtube.com/vi/kjFB58Y1TAo/0.jpg)](https://www.youtube.com/watch?v=kjFB58Y1TAo)

# Frequently Asked Questions

This repository expands upon the previous work by many people and aims to constantly add new features, bugfixes and generate documentation to make further development easier.  [Collaboration](https://github.com/eried/portapack-mayhem/wiki/How-to-collaborate) is always welcomed and appreciated.

## Does it work on H1/H2 PortaPack?

Yes, both devices are the [same](https://github.com/eried/portapack-mayhem/wiki/First-steps). The one I am using to test all changes is this [PortaPack H2+HackRF+battery](https://s.click.aliexpress.com/e/_9zbXMk), which is a kit that includes everything you need. Sadly, the people making the H2 never made the updated schematics available, which is not ideal (and goes against the terms of the license). Most members of the community are using a clone of the [PortaPack H1+HackRF+metal case](https://s.click.aliexpress.com/e/_dS6liw4), which does not include any battery functionality, but it is a cheaper alternative.

To support the people behind the hardware, please buy a genuine [HackRF](https://greatscottgadgets.com/hackrf/) and [PortaPack](https://store.sharebrained.com/products/portapack-for-hackrf-one-kit).

## Where is the latest firmware?

The **latest (nightly) release** can be found [here](https://github.com/eried/portapack-mayhem/releases/).

The current **stable release** is on the [![GitHub release (latest by date)](https://img.shields.io/github/v/release/eried/portapack-mayhem?label=Releases&style=social)](https://github.com/eried/portapack-mayhem/releases/latest) page. Follow the instructions you can find in the release description. 

## Is this the newest firmware for my PortaPack? 
Most probably: **YES**. *If you find new features somewhere else, please [suggest](https://github.com/eried/portapack-mayhem/issues/new/choose) them*.

## Which one is actually the newest?
There is a lot of confusion of which is the latest version because no old version used any actual "version number". Additionally, since the files were distributed on facebook groups, github issue links and similar temporal sources, then there was no central location for them. 

This fork (**Mayhem**) uses *major.minor.release* [semantic versioning](https://en.wikipedia.org/wiki/Software_versioning), so you can always compare your current version with the latest from [Releases](https://github.com/eried/portapack-mayhem/releases/latest).

## What about Havoc/GridRF/jamesshao8/jboone's?
* jboone's PortaPack: the [vanilla](https://en.wikipedia.org/wiki/Vanilla_software) experience
* Havoc: It was the most popular fork of jboone's PortaPack, currrently, it is not being maintained nor updated
* jamesshao8: He keeps his own version of the fork, while not attached as a fork to anything. Latest functions do not follow the license and are not being published with source code, so keep this in mind
* GridRF: They sell PortaPack clones with their own firmware based on a old version, which has no sourcecode available

## How can I collaborate
You can write [documentation](https://github.com/eried/portapack-mayhem/wiki), fix bugs and [answer issues](https://github.com/eried/portapack-mayhem/issues) or add new functionality. Please check the following [guide](https://github.com/eried/portapack-mayhem/wiki/How-to-collaborate) with details.

Consider that the hardware and firmware has been created and maintain by a [lot](https://github.com/mossmann/hackrf/graphs/contributors) of [people](https://github.com/eried/portapack-mayhem/graphs/contributors), so always try colaborating your time and effort first. For coding related questions, if something does not fit as an issue, please join our Discord by clicking the chat badge on [top](#portapack-mayhem).

As a last option, if you want to send money directly to me for getting more boards, antennas and such:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CBPQA4HRRPJQ6&source=url)

## What if I really want to pay for something?
You can create a bounty and invite people to your own bounty. This will incentivize coders to work on a new feature, solving a bug or even writting documentation. Start a bounty by [creating](https://github.com/eried/portapack-mayhem/issues/new/choose) or [choosing](https://github.com/eried/portapack-mayhem/issues/) an existing issue. Then, go to [Bountysource](https://www.bountysource.com/) and post a bounty using the link to that specific [issue](https://www.bountysource.com/teams/portapack-mayhem/issues).

Promote your bounty over our Discord by clicking the chat badge on [top](#portapack-mayhem).

## What if I need help?
First, check the [documentation](https://github.com/eried/portapack-mayhem/wiki). If you find a bug or you think the problem is related to the current repository, please open an [issue](https://github.com/eried/portapack-mayhem/issues/new/choose).

You can reach the [official community](https://www.facebook.com/groups/177623356165819) in Facebook, and our Discord by clicking the chat badge on [top](#portapack-mayhem).

## What if I find incongruencies, or grammatical errors in the text?
If is on the [Wiki](https://github.com/eried/portapack-mayhem/wiki), you can modify it directly. If is on files of the repository, you can send corrections as [pull requests](https://github.com/eried/portapack-mayhem/wiki/How-to-collaborate#coding-new-stuff-or-fixing-bugs). As a last resource, open an [issue](https://github.com/eried/portapack-mayhem/issues/new/choose).
