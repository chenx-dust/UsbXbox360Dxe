# UEFI Driver for Xbox 360 Controller

This driver is modified from [edk2](https://github.com/tianocore/edk2) USB keyboard driver, with AI-assistance. It maps gamepad keys to keyboard keys.

## Key Map

```c
STATIC CONST XBOX360_BUTTON_MAP  mXbox360ButtonMap[] = {
  { XBOX360_BUTTON_START,          0x2B }, // Tab
  { XBOX360_BUTTON_BACK,           0x2C }, // Space
  { XBOX360_BUTTON_A,              0x28 }, // Enter
  { XBOX360_BUTTON_B,              0x29 }, // Escape
  { XBOX360_BUTTON_X,              0x4B }, // Page Up
  { XBOX360_BUTTON_Y,              0x4E }, // Page Down
  { XBOX360_BUTTON_LEFT_SHOULDER,  0xE0 }, // Left Control
  { XBOX360_BUTTON_RIGHT_SHOULDER, 0xE2 }, // Left Alt
  { XBOX360_BUTTON_LEFT_THUMB,     0xE1 }, // Left Shift
  { XBOX360_BUTTON_RIGHT_THUMB,    0xE5 }, // Right Shift
  { XBOX360_BUTTON_GUIDE,          0x4A }, // Home
  { XBOX360_BUTTON_DPAD_UP,        0x52 }, // Up Arrow
  { XBOX360_BUTTON_DPAD_DOWN,      0x51 }, // Down Arrow
  { XBOX360_BUTTON_DPAD_LEFT,      0x50 }, // Left Arrow
  { XBOX360_BUTTON_DPAD_RIGHT,     0x4F }  // Right Arrow
};
```

## License

This project inherits the license of original driver, BSD-2-Clause-Patent.

Copyright (c) 2025, Chenx Dust. All rights reserved.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.
