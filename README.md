# PortaPack Mayhem

[![Build Status](https://travis-ci.com/eried/portapack-mayhem.svg?branch=master)](https://travis-ci.com/eried/portapack-mayhem) [![buddy pipeline](https://app.buddy.works/eried/portapack/pipelines/pipeline/252276/badge.svg?token=48cd59d53de0589a8fbe26bc751d77a59a011cf72581da049343879402991c34 "buddy pipeline")](https://app.buddy.works/eried/portapack/pipelines/pipeline/252276) [![CodeScene Code Health](https://codescene.io/projects/8381/status-badges/code-health)](https://codescene.io/projects/8381) [![Docker Hub Pulls](https://img.shields.io/docker/pulls/eried/portapack.svg)](https://hub.docker.com/r/eried/portapack)

This is a fork of the [Havoc](https://github.com/furrtek/portapack-havoc/), which was a fork of the [PortaPack](https://github.com/sharebrained/portapack-hackrf) firmware, an add-on for the [HackRF](http://greatscottgadgets.com/hackrf/). A fork means it is a derivated, so it is basically a clone that has extra features and fixes over the older versions.

[<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_front.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview) [<img src="https://raw.githubusercontent.com/wiki/eried/portapack-mayhem/img/hw_overview_h2_inside.png" height="400">](https://github.com/eried/portapack-mayhem/wiki/Hardware-overview#portapack-internals)

*[PortaPack H2](https://s.click.aliexpress.com/e/_dSMPvNo) (clone) with a custom [3d printed case](https://github.com/eried/portapack-mayhem/wiki/H2-Enclosure)*

## FAQ

The intention of the current repository is to try to get all the latest features and cleanest documentation for them, embracing any colaboration. To clarify any basic question, please read the following common ones:

### Does it work on H1/H2 PortaPack?

Yes, those are the [same](https://github.com/eried/portapack-mayhem/wiki/First-steps). The one I am using to test all changes is this [Portapack H2+HackRF+battery](https://s.click.aliexpress.com/e/_dSMPvNo), which is a clone that includes everything you need. Sadly, the people making the H2 never published the updated PCB, so it is not "optimal" for the community.

### Where is the latest firmware?

The current stable release is on the [Release](https://github.com/eried/portapack-mayhem/releases/latest) page. Follow the instructions on the description of the release itself to get it on your device. 

There is also [nightly builds](https://github.com/eried/portapack-mayhem/releases/tag/nightly) generated frequently, but these may contain incomplete functionality.

### Is this the newest firmware for my PortaPack? 
Probably **YES**.

### Which one is actually the newest?
There is a lot of confusion of which is the latest version because no old version used any actual "version number". Additionally, since the files were distributed on facebook groups, github issue links and similar temporal sources, then there was no central location for them. 

This fork (**Mayhem**) uses *major.minor.release* [semantic versioning](https://en.wikipedia.org/wiki/Software_versioning), so you can always compare your current version with the latest from [Releases](https://github.com/eried/portapack-mayhem/releases/latest).

### What about Havoc/GridRF/jamesshao8/jboone's?
* jboone's PortaPack: the [vainilla](https://en.wikipedia.org/wiki/Vanilla_software) experience
* Havoc: It was the most popular fork of jboone's PortaPack, right now is readonly so it is not being developed
* jamesshao8: He keeps his own version of the fork, while not attached as a fork to anything
* GridRF: They sell PortaPack clones with their own firmware based on a old version, which has no sourcecode available

### How can I colaborate
You can write [documentation](https://github.com/eried/portapack-mayhem/wiki), fix bugs and [answer issues](https://github.com/eried/portapack-mayhem/issues) or add new functionality. Please check the following [guide](https://github.com/eried/portapack-mayhem/wiki/How-to-colaborate) with details.

Consider that the hardware and firmware has been created and maintain by a [lot](https://github.com/mossmann/hackrf/graphs/contributors) of [people](https://github.com/eried/portapack-mayhem/graphs/contributors), so always try colaborating your time and effort first. 

As a last option, if you want to send money directly to me for getting more boards, antennas and such, please use:
[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CBPQA4HRRPJQ6&source=url)

### What if I need help?
First, check the [documentation](https://github.com/eried/portapack-mayhem/wiki). If you find a bug or you think the problem is related with the current repository, please open an [issue](https://github.com/eried/portapack-mayhem/issues/new/choose).

You can reach the [official community](https://www.facebook.com/groups/177623356165819) in Facebook. 
