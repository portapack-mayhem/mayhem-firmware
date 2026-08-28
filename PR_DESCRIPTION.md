# Warn Before Fast Hopping

## Summary

Add a confirmation dialog before starting Jammer or Hopper with a hopping interval below 50 ms.

## Problem

Very short hopping intervals generate retune events faster than the M0 MCU can process them. This can make the UI unresponsive and require a hardware reset.

Hopper already displayed a warning for its fastest hopping mode. Jammer did not have equivalent protection, and Hopper did not warn for every unsafe interval.

## Changes

- Added matching fast-hopping warnings to Jammer and Hopper.
- Show a confirmation dialog when the selected hop interval is below 50 ms.
- Preserve the existing 50 ms UI delay after confirmation so the dialog is rendered before transmission starts.
- Keep 50 ms and slower hopping intervals unchanged.

## Validation

- Rebuilt the application target successfully.
- Regenerated the Jammer and Hopper external application images.
