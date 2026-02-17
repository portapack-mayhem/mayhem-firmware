# Keystore File Specification
## Introduction
This directory contains the KeeLoq keystore file needed by SubGhzD to decrypt KeeLoq signals and used by the KeeLoq TX app to emulate KeeLoq transmitters.
## Where to get more KeeLoq keys
To gather more KeeLoq keys you can either extract them directly from the receiver's firmware or you can grab them from Flipper Zero firmwares, such as Unleashed and Momentum.

To do the latter a Flipper Zero device is required, since the `keeloq_mfcodes` file on the flipper is encrypted using the factory AES-128 keys stored in the its security enclave chip. In order to decrypt them and have them exported for use in Mayhem, you can use [Rocket God's SubGHz toolkit app](https://github.com/RocketGod-git/RocketGods-SubGHz-Toolkit).

## File structure
Each line in the MFCODES file contains three fields, each separated by a semicolon: Manufacturer, key and key type. For example:
```
Stilmatic;0123456789ABCDEF;2
```
The type field can be either 1 or 2 currently (KeeLoq simple learning and normal learning, respectively) since those are the currently supported learning modes.
