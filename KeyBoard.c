/** @file
  Helper functions for the USB Xbox 360 controller to keyboard driver.

Copyright (c) 2025, Chenx Dust. All rights reserved.<BR>
Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "KeyBoard.h"

//
// Xbox 360 button bit definitions
//
#define XBOX360_BUTTON_DPAD_UP         BIT0
#define XBOX360_BUTTON_DPAD_DOWN       BIT1
#define XBOX360_BUTTON_DPAD_LEFT       BIT2
#define XBOX360_BUTTON_DPAD_RIGHT      BIT3
#define XBOX360_BUTTON_START           BIT4
#define XBOX360_BUTTON_BACK            BIT5
#define XBOX360_BUTTON_LEFT_THUMB      BIT6
#define XBOX360_BUTTON_RIGHT_THUMB     BIT7
#define XBOX360_BUTTON_LEFT_SHOULDER   BIT8
#define XBOX360_BUTTON_RIGHT_SHOULDER  BIT9
#define XBOX360_BUTTON_GUIDE           BIT10
#define XBOX360_BUTTON_A               BIT12
#define XBOX360_BUTTON_B               BIT13
#define XBOX360_BUTTON_X               BIT14
#define XBOX360_BUTTON_Y               BIT15

//
// Known Xbox 360 protocol compatible devices
// All devices verified against Linux kernel xpad driver (XTYPE_XBOX360)
// Reference: linux/drivers/input/joystick/xpad.c
//
STATIC CONST XBOX360_COMPATIBLE_DEVICE  mXbox360BuiltinDevices[] = {
  //
  // Microsoft Official Controllers
  //
  { 0x045E, 0x028E, L"Xbox 360 Wired Controller" },
  { 0x045E, 0x028F, L"Xbox 360 Wired Controller v2" },
  { 0x045E, 0x0719, L"Xbox 360 Wireless Receiver" },
  
  //
  // Handheld Gaming Devices (High Priority)
  //
  { 0x0079, 0x18D4, L"GPD Win 2 Controller" },
  { 0x2563, 0x058D, L"OneXPlayer Gamepad" },
  { 0x17EF, 0x6182, L"Lenovo Legion Go" },
  { 0x1A86, 0xE310, L"Legion Go S" },
  { 0x0DB0, 0x1901, L"MSI Claw" },
  { 0x2993, 0x2001, L"TECNO Pocket Go" },
  { 0x1EE9, 0x1590, L"ZOTAC Gaming Zone" },
  
  //
  // 8BitDo Controllers
  //
  { 0x2DC8, 0x3106, L"8BitDo Ultimate / Pro 2 Wired" },
  { 0x2DC8, 0x3109, L"8BitDo Ultimate Wireless" },
  { 0x2DC8, 0x310A, L"8BitDo Ultimate 2C Wireless" },
  { 0x2DC8, 0x310B, L"8BitDo Ultimate 2 Wireless" },
  { 0x2DC8, 0x6001, L"8BitDo SN30 Pro" },
  
  //
  // Logitech
  //
  { 0x046D, 0xC21D, L"Logitech F310" },
  { 0x046D, 0xC21E, L"Logitech F510" },
  { 0x046D, 0xC21F, L"Logitech F710" },
  { 0x046D, 0xC242, L"Logitech Chillstream" },
  
  //
  // HyperX
  //
  { 0x03F0, 0x038D, L"HyperX Clutch (wired)" },
  { 0x03F0, 0x048D, L"HyperX Clutch (wireless)" },
  
  //
  // Other Popular Brands
  //
  { 0x1038, 0x1430, L"SteelSeries Stratus Duo" },
  { 0x1038, 0x1431, L"SteelSeries Stratus Duo (alt)" },
  { 0x2345, 0xE00B, L"Machenike G5 Pro" },
  { 0x3537, 0x1004, L"GameSir T4 Kaleid" },
  { 0x37D7, 0x2501, L"Flydigi Apex 5" },
  { 0x413D, 0x2104, L"Black Shark Green Ghost" },
  { 0x1949, 0x041A, L"Amazon Game Controller" },
  
  //
  // Razer
  //
  { 0x1689, 0xFD00, L"Razer Onza Tournament" },
  { 0x1689, 0xFD01, L"Razer Onza Classic" },
  { 0x1689, 0xFE00, L"Razer Sabertooth" },
  
  //
  // Add more devices here as needed
  // Format: { VID, PID, L"Description" },
  //
};

#define XBOX360_BUILTIN_DEVICE_COUNT \
  (sizeof(mXbox360BuiltinDevices) / sizeof(XBOX360_COMPATIBLE_DEVICE))

//
// Dynamic device list (built-in + custom from config)
//
STATIC XBOX360_COMPATIBLE_DEVICE  *mXbox360DeviceList = NULL;
STATIC UINTN                       mXbox360DeviceCount = 0;
STATIC BOOLEAN                     mDeviceListInitialized = FALSE;

//
// Global configuration
//
STATIC XBOX360_CONFIG  mGlobalConfig;

//
// Button to keyboard mapping structure
//
typedef struct {
  UINT16    ButtonMask;
  UINT8     UsbKeyCode;
} XBOX360_BUTTON_MAP;

STATIC CONST XBOX360_BUTTON_MAP  mXbox360ButtonMap[] = {
  { XBOX360_BUTTON_START,          0x2C }, // Space
  { XBOX360_BUTTON_BACK,           0x2B }, // Tab
  { XBOX360_BUTTON_A,              0x28 }, // Enter
  { XBOX360_BUTTON_B,              0x29 }, // Escape
  { XBOX360_BUTTON_X,              0x2A }, // Backspace
  { XBOX360_BUTTON_Y,              0x2B }, // Tab
  { XBOX360_BUTTON_LEFT_THUMB,     0xE0 }, // Left Control
  { XBOX360_BUTTON_RIGHT_THUMB,    0xE2 }, // Left Alt
  { XBOX360_BUTTON_LEFT_SHOULDER,  0x4B }, // Page Up
  { XBOX360_BUTTON_RIGHT_SHOULDER, 0x4E }, // Page Down
  { XBOX360_BUTTON_GUIDE,          0xE1 }, // Left Shift
  { XBOX360_BUTTON_DPAD_UP,        0x52 }, // Up Arrow
  { XBOX360_BUTTON_DPAD_DOWN,      0x51 }, // Down Arrow
  { XBOX360_BUTTON_DPAD_LEFT,      0x50 }, // Left Arrow
  { XBOX360_BUTTON_DPAD_RIGHT,     0x4F }  // Right Arrow
};

STATIC
VOID
QueueButtonTransition (
  IN USB_KB_DEV  *UsbKeyboardDevice,
  IN UINT8       KeyCode,
  IN BOOLEAN     IsPressed
  );

STATIC
VOID
ProcessButtonChanges (
  IN USB_KB_DEV  *UsbKeyboardDevice,
  IN UINT16      OldButtons,
  IN UINT16      NewButtons
  );

//
// Configuration file functions (forward declarations)
//
STATIC
VOID
SetDefaultConfig (
  OUT XBOX360_CONFIG  *Config
  );

STATIC
VOID
TrimString (
  IN OUT CHAR8  *Str
  );

STATIC
BOOLEAN
ParseBool (
  IN CHAR8  *Value
  );

STATIC
BOOLEAN
ParseDeviceString (
  IN  CHAR8                      *DeviceStr,
  OUT XBOX360_COMPATIBLE_DEVICE  *Device
  );

STATIC
VOID
ParseIniConfig (
  IN  CHAR8           *IniData,
  OUT XBOX360_CONFIG  *Config
  );

STATIC
UINT16
ParseConfigVersion (
  IN CHAR8  *ConfigData
  );

STATIC
VOID
ValidateAndSanitizeConfig (
  IN OUT XBOX360_CONFIG  *Config
  );

STATIC
EFI_STATUS
TryReadConfigFromVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  OUT CHAR8                            **ConfigData,
  OUT UINTN                            *ConfigSize
  );

STATIC
EFI_STATUS
FindAndReadConfig (
  OUT CHAR8  **ConfigData,
  OUT UINTN  *ConfigSize
  );

STATIC
CHAR8 *
GenerateConfigTemplate (
  VOID
  );

STATIC
EFI_STATUS
TryWriteConfigToVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  );

STATIC
EFI_STATUS
GenerateDefaultConfigFile (
  VOID
  );

STATIC
EFI_STATUS
LoadConfigWithMigration (
  OUT XBOX360_CONFIG  *Config
  );

USB_KEYBOARD_LAYOUT_PACK_BIN  mUsbKeyboardLayoutBin = {
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN),   // Binary size

  //
  // EFI_HII_PACKAGE_HEADER
  //
  {
    sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_KEYBOARD_LAYOUT
  },
  1,                                                                                                                               // LayoutCount
  sizeof (USB_KEYBOARD_LAYOUT_PACK_BIN) - sizeof (UINT32) - sizeof (EFI_HII_PACKAGE_HEADER) - sizeof (UINT16),                     // LayoutLength
  USB_KEYBOARD_LAYOUT_KEY_GUID,                                                                                                    // KeyGuid
  sizeof (UINT16) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (UINT8) + (USB_KEYBOARD_KEY_COUNT * sizeof (EFI_KEY_DESCRIPTOR)), // LayoutDescriptorStringOffset
  USB_KEYBOARD_KEY_COUNT,                                                                                                          // DescriptorCount
  {
    //
    // EFI_KEY_DESCRIPTOR (total number is USB_KEYBOARD_KEY_COUNT)
    //
    { EfiKeyC1,         'a',  'A',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB5,         'b',  'B',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB3,         'c',  'C',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC3,         'd',  'D',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD3,         'e',  'E',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC4,         'f',  'F',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC5,         'g',  'G',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC6,         'h',  'H',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD8,         'i',  'I',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC7,         'j',  'J',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC8,         'k',  'K',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC9,         'l',  'L',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB7,         'm',  'M',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB6,         'n',  'N',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD9,         'o',  'O',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD10,        'p',  'P',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD1,         'q',  'Q',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD4,         'r',  'R',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyC2,         's',  'S',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD5,         't',  'T',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD7,         'u',  'U',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB4,         'v',  'V',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD2,         'w',  'W',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB2,         'x',  'X',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyD6,         'y',  'Y',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyB1,         'z',  'Z',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_CAPS_LOCK },
    { EfiKeyE1,         '1',  '!',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE2,         '2',  '@',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE3,         '3',  '#',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE4,         '4',  '$',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE5,         '5',  '%',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE6,         '6',  '^',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE7,         '7',  '&',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE8,         '8',  '*',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE9,         '9',  '(',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE10,        '0',  ')',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyEnter,      0x0d, 0x0d, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyEsc,        0x1b, 0x1b, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyBackSpace,  0x08, 0x08, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyTab,        0x09, 0x09, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeySpaceBar,   ' ',  ' ',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyE11,        '-',  '_',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE12,        '=',  '+',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD11,        '[',  '{',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD12,        ']',  '}',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyD13,        '\\', '|',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC12,        '\\', '|',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC10,        ';',  ':',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyC11,        '\'', '"',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyE0,         '`',  '~',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB8,         ',',  '<',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB9,         '.',  '>',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyB10,        '/',  '?',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT                             },
    { EfiKeyCapsLock,   0x00, 0x00, 0,   0,   EFI_CAPS_LOCK_MODIFIER,           0                                                          },
    { EfiKeyF1,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_ONE_MODIFIER,    0                                                          },
    { EfiKeyF2,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TWO_MODIFIER,    0                                                          },
    { EfiKeyF3,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_THREE_MODIFIER,  0                                                          },
    { EfiKeyF4,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_FOUR_MODIFIER,   0                                                          },
    { EfiKeyF5,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_FIVE_MODIFIER,   0                                                          },
    { EfiKeyF6,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_SIX_MODIFIER,    0                                                          },
    { EfiKeyF7,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_SEVEN_MODIFIER,  0                                                          },
    { EfiKeyF8,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_EIGHT_MODIFIER,  0                                                          },
    { EfiKeyF9,         0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_NINE_MODIFIER,   0                                                          },
    { EfiKeyF10,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TEN_MODIFIER,    0                                                          },
    { EfiKeyF11,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_ELEVEN_MODIFIER, 0                                                          },
    { EfiKeyF12,        0x00, 0x00, 0,   0,   EFI_FUNCTION_KEY_TWELVE_MODIFIER, 0                                                          },
    { EfiKeyPrint,      0x00, 0x00, 0,   0,   EFI_PRINT_MODIFIER,               0                                                          },
    { EfiKeySLck,       0x00, 0x00, 0,   0,   EFI_SCROLL_LOCK_MODIFIER,         0                                                          },
    { EfiKeyPause,      0x00, 0x00, 0,   0,   EFI_PAUSE_MODIFIER,               0                                                          },
    { EfiKeyIns,        0x00, 0x00, 0,   0,   EFI_INSERT_MODIFIER,              0                                                          },
    { EfiKeyHome,       0x00, 0x00, 0,   0,   EFI_HOME_MODIFIER,                0                                                          },
    { EfiKeyPgUp,       0x00, 0x00, 0,   0,   EFI_PAGE_UP_MODIFIER,             0                                                          },
    { EfiKeyDel,        0x00, 0x00, 0,   0,   EFI_DELETE_MODIFIER,              0                                                          },
    { EfiKeyEnd,        0x00, 0x00, 0,   0,   EFI_END_MODIFIER,                 0                                                          },
    { EfiKeyPgDn,       0x00, 0x00, 0,   0,   EFI_PAGE_DOWN_MODIFIER,           0                                                          },
    { EfiKeyRightArrow, 0x00, 0x00, 0,   0,   EFI_RIGHT_ARROW_MODIFIER,         0                                                          },
    { EfiKeyLeftArrow,  0x00, 0x00, 0,   0,   EFI_LEFT_ARROW_MODIFIER,          0                                                          },
    { EfiKeyDownArrow,  0x00, 0x00, 0,   0,   EFI_DOWN_ARROW_MODIFIER,          0                                                          },
    { EfiKeyUpArrow,    0x00, 0x00, 0,   0,   EFI_UP_ARROW_MODIFIER,            0                                                          },
    { EfiKeyNLck,       0x00, 0x00, 0,   0,   EFI_NUM_LOCK_MODIFIER,            0                                                          },
    { EfiKeySlash,      '/',  '/',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyAsterisk,   '*',  '*',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyMinus,      '-',  '-',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyPlus,       '+',  '+',  0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyEnter,      0x0d, 0x0d, 0,   0,   EFI_NULL_MODIFIER,                0                                                          },
    { EfiKeyOne,        '1',  '1',  0,   0,   EFI_END_MODIFIER,                 EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyTwo,        '2',  '2',  0,   0,   EFI_DOWN_ARROW_MODIFIER,          EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyThree,      '3',  '3',  0,   0,   EFI_PAGE_DOWN_MODIFIER,           EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyFour,       '4',  '4',  0,   0,   EFI_LEFT_ARROW_MODIFIER,          EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyFive,       '5',  '5',  0,   0,   EFI_NULL_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeySix,        '6',  '6',  0,   0,   EFI_RIGHT_ARROW_MODIFIER,         EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeySeven,      '7',  '7',  0,   0,   EFI_HOME_MODIFIER,                EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyEight,      '8',  '8',  0,   0,   EFI_UP_ARROW_MODIFIER,            EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyNine,       '9',  '9',  0,   0,   EFI_PAGE_UP_MODIFIER,             EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyZero,       '0',  '0',  0,   0,   EFI_INSERT_MODIFIER,              EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyPeriod,     '.',  '.',  0,   0,   EFI_DELETE_MODIFIER,              EFI_AFFECTED_BY_STANDARD_SHIFT | EFI_AFFECTED_BY_NUM_LOCK  },
    { EfiKeyA4,         0x00, 0x00, 0,   0,   EFI_MENU_MODIFIER,                0                                                          },
    { EfiKeyLCtrl,      0,    0,    0,   0,   EFI_LEFT_CONTROL_MODIFIER,        0                                                          },
    { EfiKeyLShift,     0,    0,    0,   0,   EFI_LEFT_SHIFT_MODIFIER,          0                                                          },
    { EfiKeyLAlt,       0,    0,    0,   0,   EFI_LEFT_ALT_MODIFIER,            0                                                          },
    { EfiKeyA0,         0,    0,    0,   0,   EFI_LEFT_LOGO_MODIFIER,           0                                                          },
    { EfiKeyRCtrl,      0,    0,    0,   0,   EFI_RIGHT_CONTROL_MODIFIER,       0                                                          },
    { EfiKeyRShift,     0,    0,    0,   0,   EFI_RIGHT_SHIFT_MODIFIER,         0                                                          },
    { EfiKeyA2,         0,    0,    0,   0,   EFI_RIGHT_ALT_MODIFIER,           0                                                          },
    { EfiKeyA3,         0,    0,    0,   0,   EFI_RIGHT_LOGO_MODIFIER,          0                                                          },
  },
  1,                                                  // DescriptionCount
  { 'e', 'n',  '-',  'U', 'S' },  // RFC4646 language code
  ' ',                                                // Space
  u"English Keyboard",                                // DescriptionString[]
};

//
// EFI_KEY to USB Keycode conversion table
// EFI_KEY is defined in UEFI spec.
// USB Keycode is defined in USB HID Firmware spec.
//
UINT8  EfiKeyToUsbKeyCodeConvertionTable[] = {
  0xe0,  //  EfiKeyLCtrl
  0xe3,  //  EfiKeyA0
  0xe2,  //  EfiKeyLAlt
  0x2c,  //  EfiKeySpaceBar
  0xe6,  //  EfiKeyA2
  0xe7,  //  EfiKeyA3
  0x65,  //  EfiKeyA4
  0xe4,  //  EfiKeyRCtrl
  0x50,  //  EfiKeyLeftArrow
  0x51,  //  EfiKeyDownArrow
  0x4F,  //  EfiKeyRightArrow
  0x62,  //  EfiKeyZero
  0x63,  //  EfiKeyPeriod
  0x28,  //  EfiKeyEnter
  0xe1,  //  EfiKeyLShift
  0x64,  //  EfiKeyB0
  0x1D,  //  EfiKeyB1
  0x1B,  //  EfiKeyB2
  0x06,  //  EfiKeyB3
  0x19,  //  EfiKeyB4
  0x05,  //  EfiKeyB5
  0x11,  //  EfiKeyB6
  0x10,  //  EfiKeyB7
  0x36,  //  EfiKeyB8
  0x37,  //  EfiKeyB9
  0x38,  //  EfiKeyB10
  0xe5,  //  EfiKeyRShift
  0x52,  //  EfiKeyUpArrow
  0x59,  //  EfiKeyOne
  0x5A,  //  EfiKeyTwo
  0x5B,  //  EfiKeyThree
  0x39,  //  EfiKeyCapsLock
  0x04,  //  EfiKeyC1
  0x16,  //  EfiKeyC2
  0x07,  //  EfiKeyC3
  0x09,  //  EfiKeyC4
  0x0A,  //  EfiKeyC5
  0x0B,  //  EfiKeyC6
  0x0D,  //  EfiKeyC7
  0x0E,  //  EfiKeyC8
  0x0F,  //  EfiKeyC9
  0x33,  //  EfiKeyC10
  0x34,  //  EfiKeyC11
  0x32,  //  EfiKeyC12
  0x5C,  //  EfiKeyFour
  0x5D,  //  EfiKeyFive
  0x5E,  //  EfiKeySix
  0x57,  //  EfiKeyPlus
  0x2B,  //  EfiKeyTab
  0x14,  //  EfiKeyD1
  0x1A,  //  EfiKeyD2
  0x08,  //  EfiKeyD3
  0x15,  //  EfiKeyD4
  0x17,  //  EfiKeyD5
  0x1C,  //  EfiKeyD6
  0x18,  //  EfiKeyD7
  0x0C,  //  EfiKeyD8
  0x12,  //  EfiKeyD9
  0x13,  //  EfiKeyD10
  0x2F,  //  EfiKeyD11
  0x30,  //  EfiKeyD12
  0x31,  //  EfiKeyD13
  0x4C,  //  EfiKeyDel
  0x4D,  //  EfiKeyEnd
  0x4E,  //  EfiKeyPgDn
  0x5F,  //  EfiKeySeven
  0x60,  //  EfiKeyEight
  0x61,  //  EfiKeyNine
  0x35,  //  EfiKeyE0
  0x1E,  //  EfiKeyE1
  0x1F,  //  EfiKeyE2
  0x20,  //  EfiKeyE3
  0x21,  //  EfiKeyE4
  0x22,  //  EfiKeyE5
  0x23,  //  EfiKeyE6
  0x24,  //  EfiKeyE7
  0x25,  //  EfiKeyE8
  0x26,  //  EfiKeyE9
  0x27,  //  EfiKeyE10
  0x2D,  //  EfiKeyE11
  0x2E,  //  EfiKeyE12
  0x2A,  //  EfiKeyBackSpace
  0x49,  //  EfiKeyIns
  0x4A,  //  EfiKeyHome
  0x4B,  //  EfiKeyPgUp
  0x53,  //  EfiKeyNLck
  0x54,  //  EfiKeySlash
  0x55,  //  EfiKeyAsterisk
  0x56,  //  EfiKeyMinus
  0x29,  //  EfiKeyEsc
  0x3A,  //  EfiKeyF1
  0x3B,  //  EfiKeyF2
  0x3C,  //  EfiKeyF3
  0x3D,  //  EfiKeyF4
  0x3E,  //  EfiKeyF5
  0x3F,  //  EfiKeyF6
  0x40,  //  EfiKeyF7
  0x41,  //  EfiKeyF8
  0x42,  //  EfiKeyF9
  0x43,  //  EfiKeyF10
  0x44,  //  EfiKeyF11
  0x45,  //  EfiKeyF12
  0x46,  //  EfiKeyPrint
  0x47,  //  EfiKeySLck
  0x48   //  EfiKeyPause
};

//
// Keyboard modifier value to EFI Scan Code conversion table
// EFI Scan Code and the modifier values are defined in UEFI spec.
//
UINT8  ModifierValueToEfiScanCodeConvertionTable[] = {
  SCAN_NULL,       // EFI_NULL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_CONTROL_MODIFIER
  SCAN_NULL,       // EFI_LEFT_ALT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_ALT_MODIFIER
  SCAN_NULL,       // EFI_ALT_GR_MODIFIER
  SCAN_INSERT,     // EFI_INSERT_MODIFIER
  SCAN_DELETE,     // EFI_DELETE_MODIFIER
  SCAN_PAGE_DOWN,  // EFI_PAGE_DOWN_MODIFIER
  SCAN_PAGE_UP,    // EFI_PAGE_UP_MODIFIER
  SCAN_HOME,       // EFI_HOME_MODIFIER
  SCAN_END,        // EFI_END_MODIFIER
  SCAN_NULL,       // EFI_LEFT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_SHIFT_MODIFIER
  SCAN_NULL,       // EFI_CAPS_LOCK_MODIFIER
  SCAN_NULL,       // EFI_NUM_LOCK_MODIFIER
  SCAN_LEFT,       // EFI_LEFT_ARROW_MODIFIER
  SCAN_RIGHT,      // EFI_RIGHT_ARROW_MODIFIER
  SCAN_DOWN,       // EFI_DOWN_ARROW_MODIFIER
  SCAN_UP,         // EFI_UP_ARROW_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_MODIFIER
  SCAN_NULL,       // EFI_NS_KEY_DEPENDENCY_MODIFIER
  SCAN_F1,         // EFI_FUNCTION_KEY_ONE_MODIFIER
  SCAN_F2,         // EFI_FUNCTION_KEY_TWO_MODIFIER
  SCAN_F3,         // EFI_FUNCTION_KEY_THREE_MODIFIER
  SCAN_F4,         // EFI_FUNCTION_KEY_FOUR_MODIFIER
  SCAN_F5,         // EFI_FUNCTION_KEY_FIVE_MODIFIER
  SCAN_F6,         // EFI_FUNCTION_KEY_SIX_MODIFIER
  SCAN_F7,         // EFI_FUNCTION_KEY_SEVEN_MODIFIER
  SCAN_F8,         // EFI_FUNCTION_KEY_EIGHT_MODIFIER
  SCAN_F9,         // EFI_FUNCTION_KEY_NINE_MODIFIER
  SCAN_F10,        // EFI_FUNCTION_KEY_TEN_MODIFIER
  SCAN_F11,        // EFI_FUNCTION_KEY_ELEVEN_MODIFIER
  SCAN_F12,        // EFI_FUNCTION_KEY_TWELVE_MODIFIER
  //
  // For Partial Keystroke support
  //
  SCAN_NULL,       // EFI_PRINT_MODIFIER
  SCAN_NULL,       // EFI_SYS_REQUEST_MODIFIER
  SCAN_NULL,       // EFI_SCROLL_LOCK_MODIFIER
  SCAN_PAUSE,      // EFI_PAUSE_MODIFIER
  SCAN_NULL,       // EFI_BREAK_MODIFIER
  SCAN_NULL,       // EFI_LEFT_LOGO_MODIFIER
  SCAN_NULL,       // EFI_RIGHT_LOGO_MODIFER
  SCAN_NULL,       // EFI_MENU_MODIFER
};

/**
  Initialize Key Convention Table by using default keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

  @retval EFI_SUCCESS          The default keyboard layout was installed successfully
  @retval Others               Failure to install default keyboard layout.
**/
EFI_STATUS
InstallDefaultKeyboardLayout (
  IN OUT USB_KB_DEV  *UsbKeyboardDevice
  )
{
  EFI_STATUS                 Status;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;
  EFI_HII_HANDLE             HiiHandle;

  //
  // Locate Hii database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Keyboard Layout package to HII database
  //
  HiiHandle = HiiAddPackages (
                &gUsbKeyboardLayoutPackageGuid,
                UsbKeyboardDevice->ControllerHandle,
                &mUsbKeyboardLayoutBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set current keyboard layout
  //
  Status = HiiDatabase->SetKeyboardLayout (HiiDatabase, &gUsbKeyboardLayoutKeyGuid);

  return Status;
}

//
// =============================================================================
// Configuration File Support Functions
// =============================================================================
//

/**
  Trim leading and trailing whitespace from a string.

  @param  Str  String to trim (modified in place).
**/
STATIC
VOID
TrimString (
  IN OUT CHAR8  *Str
  )
{
  CHAR8  *End;

  if (Str == NULL || *Str == '\0') {
    return;
  }

  // Trim leading whitespace
  while (*Str == ' ' || *Str == '\t' || *Str == '\r' || *Str == '\n') {
    Str++;
  }

  if (*Str == '\0') {
    return;
  }

  // Trim trailing whitespace
  End = Str + AsciiStrLen(Str) - 1;
  while (End > Str && (*End == ' ' || *End == '\t' || *End == '\r' || *End == '\n')) {
    *End = '\0';
    End--;
  }
}

/**
  Parse a boolean value from a string.

  @param  Value  String to parse (true/false/yes/no/1/0).

  @retval TRUE   Value is true.
  @retval FALSE  Value is false or unrecognized.
**/
STATIC
BOOLEAN
ParseBool (
  IN CHAR8  *Value
  )
{
  if (Value == NULL) {
    return FALSE;
  }

  if ((AsciiStrCmp(Value, "true") == 0) ||
      (AsciiStrCmp(Value, "TRUE") == 0) ||
      (AsciiStrCmp(Value, "yes") == 0) ||
      (AsciiStrCmp(Value, "YES") == 0) ||
      (AsciiStrCmp(Value, "1") == 0)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Parse device string in format: VID:PID:Description
  VID and PID can be hex (0x1234 or 1234).

  @param  DeviceStr  String to parse (e.g., "0x1234:0x5678:My Controller").
  @param  Device     Output device structure.

  @retval TRUE   Successfully parsed.
  @retval FALSE  Parse error.
**/
STATIC
BOOLEAN
ParseDeviceString (
  IN  CHAR8                      *DeviceStr,
  OUT XBOX360_COMPATIBLE_DEVICE  *Device
  )
{
  CHAR8  *VidStr;
  CHAR8  *PidStr;
  CHAR8  *DescStr;
  CHAR8  *Colon1;
  CHAR8  *Colon2;
  UINTN  DescLen;
  UINTN  i;

  if (DeviceStr == NULL || Device == NULL) {
    return FALSE;
  }

  // Find first colon
  Colon1 = AsciiStrStr(DeviceStr, ":");
  if (Colon1 == NULL) {
    return FALSE;
  }
  *Colon1 = '\0';

  // Find second colon
  Colon2 = AsciiStrStr(Colon1 + 1, ":");
  if (Colon2 == NULL) {
    return FALSE;
  }
  *Colon2 = '\0';

  VidStr = DeviceStr;
  PidStr = Colon1 + 1;
  DescStr = Colon2 + 1;

  // Trim strings
  TrimString(VidStr);
  TrimString(PidStr);
  TrimString(DescStr);

  // Parse VID (support both 0x1234 and 1234 format)
  if ((AsciiStrnCmp(VidStr, "0x", 2) == 0) || (AsciiStrnCmp(VidStr, "0X", 2) == 0)) {
    Device->VendorId = (UINT16)AsciiStrHexToUintn(VidStr + 2);
  } else {
    Device->VendorId = (UINT16)AsciiStrHexToUintn(VidStr);
  }

  // Parse PID
  if ((AsciiStrnCmp(PidStr, "0x", 2) == 0) || (AsciiStrnCmp(PidStr, "0X", 2) == 0)) {
    Device->ProductId = (UINT16)AsciiStrHexToUintn(PidStr + 2);
  } else {
    Device->ProductId = (UINT16)AsciiStrHexToUintn(PidStr);
  }

  // Convert description from ASCII to Unicode
  DescLen = AsciiStrLen(DescStr);
  if (DescLen > 63) {
    DescLen = 63;  // Limit length
  }

  // Allocate memory for description
  Device->Description = AllocateZeroPool((DescLen + 1) * sizeof(CHAR16));
  if (Device->Description == NULL) {
    return FALSE;
  }

  for (i = 0; i < DescLen; i++) {
    Device->Description[i] = (CHAR16)DescStr[i];
  }
  Device->Description[DescLen] = L'\0';

  // Validate VID/PID (must not be 0x0000)
  if ((Device->VendorId == 0) || (Device->ProductId == 0)) {
    if (Device->Description != NULL) {
      FreePool(Device->Description);
      Device->Description = NULL;
    }
    return FALSE;
  }

  return TRUE;
}

/**
  Set default configuration values.

  @param  Config  Pointer to configuration structure to initialize.
**/
STATIC
VOID
SetDefaultConfig (
  OUT XBOX360_CONFIG  *Config
  )
{
  UINTN  i;

  if (Config == NULL) {
    return;
  }

  ZeroMem(Config, sizeof(XBOX360_CONFIG));

  Config->Version = XBOX360_CONFIG_VERSION_CURRENT;
  Config->StickDeadzone = 8000;
  Config->TriggerThreshold = 128;
  Config->LeftTriggerKey = 0x4C;   // Delete
  Config->RightTriggerKey = 0x4D;  // End

  // Default button mappings (from existing mXbox360ButtonMap)
  Config->ButtonMap[0] = 0x52;   // DPAD_UP -> Up Arrow
  Config->ButtonMap[1] = 0x51;   // DPAD_DOWN -> Down Arrow
  Config->ButtonMap[2] = 0x50;   // DPAD_LEFT -> Left Arrow
  Config->ButtonMap[3] = 0x4F;   // DPAD_RIGHT -> Right Arrow
  Config->ButtonMap[4] = 0x2C;   // START -> Space
  Config->ButtonMap[5] = 0x2B;   // BACK -> Tab
  Config->ButtonMap[6] = 0xE0;   // LEFT_THUMB -> Left Control
  Config->ButtonMap[7] = 0xE2;   // RIGHT_THUMB -> Left Alt
  Config->ButtonMap[8] = 0x4B;   // LEFT_SHOULDER -> Page Up
  Config->ButtonMap[9] = 0x4E;   // RIGHT_SHOULDER -> Page Down
  Config->ButtonMap[10] = 0xE1;  // GUIDE -> Left Shift
  Config->ButtonMap[11] = 0xFF;  // Reserved
  Config->ButtonMap[12] = 0x28;  // A -> Enter
  Config->ButtonMap[13] = 0x29;  // B -> Escape
  Config->ButtonMap[14] = 0x2A;  // X -> Backspace
  Config->ButtonMap[15] = 0x2B;  // Y -> Tab

  Config->CustomDeviceCount = 0;
}

/**
  Parse version from config file.
  Format: Version=1.0 or Version=0x0100

  @param  ConfigData  Configuration file content.

  @retval Version number, or 0 if not found.
**/
STATIC
UINT16
ParseConfigVersion (
  IN CHAR8  *ConfigData
  )
{
  CHAR8   *VersionLine;
  UINT16  Major;
  UINT16  Minor;
  CHAR8   *Dot;

  if (ConfigData == NULL) {
    return 0;
  }

  // Look for "Version=" line
  VersionLine = AsciiStrStr(ConfigData, "Version=");
  if (VersionLine == NULL) {
    return 0;
  }

  VersionLine += 8; // Skip "Version="

  // Trim leading whitespace
  while (*VersionLine == ' ' || *VersionLine == '\t') {
    VersionLine++;
  }

  // Support hex format (0x0100)
  if ((VersionLine[0] == '0') && ((VersionLine[1] == 'x') || (VersionLine[1] == 'X'))) {
    return (UINT16)AsciiStrHexToUintn(VersionLine);
  }

  // Parse decimal "major.minor" format
  Major = (UINT16)AsciiStrDecimalToUintn(VersionLine);
  Dot = AsciiStrStr(VersionLine, ".");
  Minor = 0;
  if (Dot != NULL) {
    Minor = (UINT16)AsciiStrDecimalToUintn(Dot + 1);
  }

  return (Major << 8) | Minor;
}

/**
  Parse INI configuration file.

  @param  IniData  Configuration file content (will be modified).
  @param  Config   Configuration structure to populate.
**/
STATIC
VOID
ParseIniConfig (
  IN  CHAR8           *IniData,
  OUT XBOX360_CONFIG  *Config
  )
{
  CHAR8  *Line;
  CHAR8  *NextLine;
  CHAR8  *Key;
  CHAR8  *Value;
  CHAR8  *Equals;
  UINTN  DeviceIndex;

  if ((IniData == NULL) || (Config == NULL)) {
    return;
  }

  Line = IniData;
  DeviceIndex = 0;

  while ((Line != NULL) && (*Line != '\0')) {
    // Find next line
    NextLine = AsciiStrStr(Line, "\n");
    if (NextLine != NULL) {
      *NextLine = '\0';
      NextLine++;
    }

    // Trim line
    TrimString(Line);

    // Skip empty lines, comments, and section headers
    if ((*Line == '\0') || (*Line == '#') || (*Line == ';') || (*Line == '[')) {
      Line = NextLine;
      continue;
    }

    // Find '=' separator
    Equals = AsciiStrStr(Line, "=");
    if (Equals == NULL) {
      Line = NextLine;
      continue;
    }

    *Equals = '\0';
    Key = Line;
    Value = Equals + 1;

    // Trim key and value
    TrimString(Key);
    TrimString(Value);

    // Skip empty values
    if (*Value == '\0') {
      Line = NextLine;
      continue;
    }

    // Parse known configuration keys
    if (AsciiStrCmp(Key, "Version") == 0) {
      // Already parsed separately
    }
    else if (AsciiStrCmp(Key, "Deadzone") == 0) {
      Config->StickDeadzone = (UINT16)AsciiStrDecimalToUintn(Value);
    }
    else if (AsciiStrCmp(Key, "TriggerThreshold") == 0) {
      Config->TriggerThreshold = (UINT8)AsciiStrDecimalToUintn(Value);
    }
    else if (AsciiStrCmp(Key, "LeftTrigger") == 0) {
      if ((AsciiStrnCmp(Value, "0x", 2) == 0) || (AsciiStrnCmp(Value, "0X", 2) == 0)) {
        Config->LeftTriggerKey = (UINT8)AsciiStrHexToUintn(Value + 2);
      } else {
        Config->LeftTriggerKey = (UINT8)AsciiStrHexToUintn(Value);
      }
    }
    else if (AsciiStrCmp(Key, "RightTrigger") == 0) {
      if ((AsciiStrnCmp(Value, "0x", 2) == 0) || (AsciiStrnCmp(Value, "0X", 2) == 0)) {
        Config->RightTriggerKey = (UINT8)AsciiStrHexToUintn(Value + 2);
      } else {
        Config->RightTriggerKey = (UINT8)AsciiStrHexToUintn(Value);
      }
    }
    // Parse custom devices (Device1=, Device2=, etc.)
    else if ((AsciiStrnCmp(Key, "Device", 6) == 0) && (DeviceIndex < MAX_CUSTOM_DEVICES)) {
      if (ParseDeviceString(Value, &Config->CustomDevices[DeviceIndex])) {
        DeviceIndex++;
      }
    }
    // Button mappings (ButtonA=, ButtonB=, etc. - can be expanded later)

    Line = NextLine;
  }

  Config->CustomDeviceCount = DeviceIndex;
}

/**
  Validate and sanitize configuration values.

  @param  Config  Configuration structure to validate (modified in place).
**/
STATIC
VOID
ValidateAndSanitizeConfig (
  IN OUT XBOX360_CONFIG  *Config
  )
{
  UINTN  i;

  if (Config == NULL) {
    return;
  }

  // Clamp deadzone to valid range
  if (Config->StickDeadzone > 32767) {
    DEBUG((DEBUG_WARN, "Xbox360: Deadzone %d out of range, clamping to 32767\n", Config->StickDeadzone));
    Config->StickDeadzone = 32767;
  }

  // Validate trigger keys (USB HID scan codes should be <= 0xE7)
  if ((Config->LeftTriggerKey > 0xE7) && (Config->LeftTriggerKey != 0xFF)) {
    DEBUG((DEBUG_WARN, "Xbox360: Invalid LeftTriggerKey 0x%02X, using default\n", Config->LeftTriggerKey));
    Config->LeftTriggerKey = 0x4C;
  }

  if ((Config->RightTriggerKey > 0xE7) && (Config->RightTriggerKey != 0xFF)) {
    DEBUG((DEBUG_WARN, "Xbox360: Invalid RightTriggerKey 0x%02X, using default\n", Config->RightTriggerKey));
    Config->RightTriggerKey = 0x4D;
  }

  // Validate button mappings
  for (i = 0; i < 16; i++) {
    if ((Config->ButtonMap[i] > 0xE7) && (Config->ButtonMap[i] != 0xFF)) {
      DEBUG((DEBUG_WARN, "Xbox360: Invalid scan code 0x%02X for button %d, disabling\n", Config->ButtonMap[i], i));
      Config->ButtonMap[i] = 0xFF;
    }
  }

  // Clamp custom device count
  if (Config->CustomDeviceCount > MAX_CUSTOM_DEVICES) {
    DEBUG((DEBUG_WARN, "Xbox360: Custom device count %d exceeds maximum, clamping to %d\n", 
      Config->CustomDeviceCount, MAX_CUSTOM_DEVICES));
    Config->CustomDeviceCount = MAX_CUSTOM_DEVICES;
  }

  // Update version to current
  Config->Version = XBOX360_CONFIG_VERSION_CURRENT;
}

/**
  Generate default configuration file template.

  @retval Pointer to configuration template string (static storage).
**/
STATIC
CHAR8 *
GenerateConfigTemplate (
  VOID
  )
{
  STATIC CHAR8 Template[] = 
    "# Xbox 360 Controller Driver Configuration\r\n"
    "# =========================================\r\n"
    "# Edit this file and reboot to apply changes\r\n"
    "# This file was auto-generated on first boot\r\n"
    "\r\n"
    "Version=1.0\r\n"
    "\r\n"
    "# Analog Stick Settings\r\n"
    "# Deadzone: 0-32767 (default: 8000)\r\n"
    "Deadzone=8000\r\n"
    "\r\n"
    "# Trigger Settings\r\n"
    "# TriggerThreshold: 0-255 (default: 128)\r\n"
    "TriggerThreshold=128\r\n"
    "\r\n"
    "# Trigger key mappings (USB HID scan codes)\r\n"
    "LeftTrigger=0x4C          # Delete\r\n"
    "RightTrigger=0x4D         # End\r\n"
    "\r\n"
    "# Custom Device Support\r\n"
    "# Add your own Xbox 360 compatible devices here\r\n"
    "# Format: DeviceN=VID:PID:Description\r\n"
    "# Example: Device1=0x1234:0x5678:My Custom Controller\r\n"
    "#\r\n"
    "# [CustomDevices]\r\n"
    "# Device1=\r\n"
    "# Device2=\r\n"
    "\r\n"
    "# End of configuration\r\n";
  
  return Template;
}

/**
  Try to read configuration file from a specific volume.

  @param  FileSystem  File system protocol instance.
  @param  ConfigData  Output pointer to allocated config data.
  @param  ConfigSize  Output size of config data.

  @retval EFI_SUCCESS      Config file read successfully.
  @retval EFI_NOT_FOUND    Config file not found on this volume.
  @retval Other            Error reading file.
**/
STATIC
EFI_STATUS
TryReadConfigFromVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  OUT CHAR8                            **ConfigData,
  OUT UINTN                            *ConfigSize
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *Root;
  EFI_FILE_PROTOCOL  *ConfigFile;
  EFI_FILE_INFO      *FileInfo;
  UINTN              InfoSize;
  UINTN              BufferSize;
  CHAR8              *Buffer;
  CHAR16             *ConfigPaths[] = {
    L"EFI\\Xbox360\\config.ini",
    L"EFI\\BOOT\\xbox360.ini",
    L"xbox360.ini",
    NULL
  };
  UINTN              PathIndex;

  if ((FileSystem == NULL) || (ConfigData == NULL) || (ConfigSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FileSystem->OpenVolume(FileSystem, &Root);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Try multiple possible paths
  for (PathIndex = 0; ConfigPaths[PathIndex] != NULL; PathIndex++) {
    Status = Root->Open(
      Root,
      &ConfigFile,
      ConfigPaths[PathIndex],
      EFI_FILE_MODE_READ,
      0
    );

    if (!EFI_ERROR(Status)) {
      // Found config file, read it
      InfoSize = SIZE_OF_EFI_FILE_INFO + 256;
      FileInfo = AllocatePool(InfoSize);
      if (FileInfo == NULL) {
        ConfigFile->Close(ConfigFile);
        Root->Close(Root);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = ConfigFile->GetInfo(
        ConfigFile,
        &gEfiFileInfoGuid,
        &InfoSize,
        FileInfo
      );

      if (EFI_ERROR(Status)) {
        FreePool(FileInfo);
        ConfigFile->Close(ConfigFile);
        Root->Close(Root);
        return Status;
      }

      BufferSize = (UINTN)FileInfo->FileSize;
      Buffer = AllocateZeroPool(BufferSize + 1);
      if (Buffer == NULL) {
        FreePool(FileInfo);
        ConfigFile->Close(ConfigFile);
        Root->Close(Root);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = ConfigFile->Read(ConfigFile, &BufferSize, Buffer);
      Buffer[BufferSize] = '\0'; // Null terminate

      FreePool(FileInfo);
      ConfigFile->Close(ConfigFile);
      Root->Close(Root);

      if (!EFI_ERROR(Status)) {
        *ConfigData = Buffer;
        *ConfigSize = BufferSize;
        return EFI_SUCCESS;
      } else {
        FreePool(Buffer);
        return Status;
      }
    }
  }

  Root->Close(Root);
  return EFI_NOT_FOUND;
}

/**
  Find and read configuration file from any available volume.

  @param  ConfigData  Output pointer to allocated config data.
  @param  ConfigSize  Output size of config data.

  @retval EFI_SUCCESS      Config file found and read.
  @retval EFI_NOT_FOUND    Config file not found on any volume.
  @retval Other            Error.
**/
STATIC
EFI_STATUS
FindAndReadConfig (
  OUT CHAR8  **ConfigData,
  OUT UINTN  *ConfigSize
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            HandleCount;
  UINTN                            Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  if ((ConfigData == NULL) || (ConfigSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Locate all file system handles
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
  );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Try each file system
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol(
      HandleBuffer[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&FileSystem
    );

    if (!EFI_ERROR(Status)) {
      Status = TryReadConfigFromVolume(FileSystem, ConfigData, ConfigSize);
      if (!EFI_ERROR(Status)) {
        // Found and loaded successfully
        FreePool(HandleBuffer);
        return EFI_SUCCESS;
      }
    }
  }

  FreePool(HandleBuffer);
  return EFI_NOT_FOUND;
}

/**
  Try to write configuration file to a specific volume.

  @param  FileSystem  File system protocol instance.

  @retval EFI_SUCCESS  Config file written successfully.
  @retval Other        Error writing file.
**/
STATIC
EFI_STATUS
TryWriteConfigToVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *Root;
  EFI_FILE_PROTOCOL  *Dir;
  EFI_FILE_PROTOCOL  *ConfigFile;
  CHAR8              *ConfigTemplate;
  UINTN              ConfigSize;

  if (FileSystem == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FileSystem->OpenVolume(FileSystem, &Root);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Try to open EFI directory (must exist for this to be valid ESP)
  Status = Root->Open(
    Root,
    &Dir,
    L"EFI",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
    EFI_FILE_DIRECTORY
  );

  if (EFI_ERROR(Status)) {
    Root->Close(Root);
    return Status;
  }
  Dir->Close(Dir);

  // Create Xbox360 directory
  Status = Root->Open(
    Root,
    &Dir,
    L"EFI\\Xbox360",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
    EFI_FILE_DIRECTORY
  );

  if (EFI_ERROR(Status)) {
    Root->Close(Root);
    return Status;
  }

  // Create config file
  Status = Dir->Open(
    Dir,
    &ConfigFile,
    L"config.ini",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
    0
  );

  Dir->Close(Dir);

  if (EFI_ERROR(Status)) {
    Root->Close(Root);
    return Status;
  }

  // Write config template
  ConfigTemplate = GenerateConfigTemplate();
  ConfigSize = AsciiStrLen(ConfigTemplate);

  Status = ConfigFile->Write(ConfigFile, &ConfigSize, ConfigTemplate);

  ConfigFile->Close(ConfigFile);
  Root->Close(Root);

  return Status;
}

/**
  Generate default configuration file on first run.

  @retval EFI_SUCCESS  Config file created successfully.
  @retval Other        Error creating file (not critical).
**/
STATIC
EFI_STATUS
GenerateDefaultConfigFile (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            HandleCount;
  UINTN                            Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  // Locate all file system handles
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
  );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Try each file system until successful
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol(
      HandleBuffer[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&FileSystem
    );

    if (!EFI_ERROR(Status)) {
      Status = TryWriteConfigToVolume(FileSystem);
      if (!EFI_ERROR(Status)) {
        // Successfully created config
        FreePool(HandleBuffer);
        return EFI_SUCCESS;
      }
    }
  }

  FreePool(HandleBuffer);
  return EFI_NOT_FOUND;
}

/**
  Load configuration with version migration support.

  @param  Config  Pointer to configuration structure to populate.

  @retval EFI_SUCCESS  Configuration loaded (or defaults used).
**/
STATIC
EFI_STATUS
LoadConfigWithMigration (
  OUT XBOX360_CONFIG  *Config
  )
{
  EFI_STATUS  Status;
  CHAR8       *ConfigData;
  UINTN       ConfigSize;
  UINT16      FileVersion;

  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Set all defaults
  SetDefaultConfig(Config);

  // Step 2: Try to read config file
  Status = FindAndReadConfig(&ConfigData, &ConfigSize);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG((DEBUG_WARN, "Xbox360: Config file not found, generating template...\n"));
      
      Status = GenerateDefaultConfigFile();
      if (!EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "Xbox360: Config template created at \\EFI\\Xbox360\\config.ini\n"));
        DEBUG((DEBUG_INFO, "Xbox360: Edit and reboot to customize\n"));
      } else {
        DEBUG((DEBUG_WARN, "Xbox360: Could not create config file (using defaults)\n"));
      }
    }
    
    // Use defaults
    return EFI_SUCCESS;
  }

  // Step 3: Parse version
  FileVersion = ParseConfigVersion(ConfigData);
  
  DEBUG((DEBUG_INFO, "Xbox360: Config file found, version: %d.%d\n",
    (FileVersion >> 8), (FileVersion & 0xFF)));

  // Step 4: Parse configuration
  ParseIniConfig(ConfigData, Config);

  // Step 5: Validate and sanitize
  ValidateAndSanitizeConfig(Config);

  FreePool(ConfigData);

  DEBUG((DEBUG_INFO, "Xbox360: Configuration loaded successfully\n"));
  return EFI_SUCCESS;
}

//
// =============================================================================
// Dynamic Device List Management
// =============================================================================
//

/**
  Initialize the Xbox 360 compatible device list.
  Combines built-in devices with custom devices from config file.

  @param  Config  Pointer to configuration structure containing custom devices.

  @retval EFI_SUCCESS  Device list initialized successfully.
**/
EFI_STATUS
InitializeDeviceList (
  IN XBOX360_CONFIG  *Config
  )
{
  UINTN  TotalDevices;
  UINTN  Index;

  if (mDeviceListInitialized) {
    return EFI_SUCCESS;
  }

  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Calculate total device count
  TotalDevices = XBOX360_BUILTIN_DEVICE_COUNT + Config->CustomDeviceCount;

  // Allocate memory for combined list
  mXbox360DeviceList = AllocateZeroPool(sizeof(XBOX360_COMPATIBLE_DEVICE) * TotalDevices);
  
  if (mXbox360DeviceList == NULL) {
    // Fallback to built-in only
    mXbox360DeviceList = (XBOX360_COMPATIBLE_DEVICE*)mXbox360BuiltinDevices;
    mXbox360DeviceCount = XBOX360_BUILTIN_DEVICE_COUNT;
    mDeviceListInitialized = TRUE;
    DEBUG((DEBUG_WARN, "Xbox360: Failed to allocate device list, using built-in only\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy built-in devices
  CopyMem(
    mXbox360DeviceList,
    mXbox360BuiltinDevices,
    sizeof(XBOX360_COMPATIBLE_DEVICE) * XBOX360_BUILTIN_DEVICE_COUNT
  );

  // Append custom devices
  for (Index = 0; Index < Config->CustomDeviceCount; Index++) {
    CopyMem(
      &mXbox360DeviceList[XBOX360_BUILTIN_DEVICE_COUNT + Index],
      &Config->CustomDevices[Index],
      sizeof(XBOX360_COMPATIBLE_DEVICE)
    );

    DEBUG((DEBUG_INFO, 
      "Xbox360: Added custom device: %s (VID:0x%04X PID:0x%04X)\n",
      Config->CustomDevices[Index].Description,
      Config->CustomDevices[Index].VendorId,
      Config->CustomDevices[Index].ProductId
    ));
  }

  mXbox360DeviceCount = TotalDevices;
  mDeviceListInitialized = TRUE;

  DEBUG((DEBUG_INFO, 
    "Xbox360: Device list initialized (%d built-in + %d custom = %d total)\n",
    XBOX360_BUILTIN_DEVICE_COUNT,
    Config->CustomDeviceCount,
    TotalDevices
  ));

  return EFI_SUCCESS;
}

/**
  Cleanup device list when driver unloads.
**/
VOID
CleanupDeviceList (
  VOID
  )
{
  UINTN  i;

  if (!mDeviceListInitialized) {
    return;
  }

  // Free custom device descriptions
  if (mXbox360DeviceList != NULL && 
      mXbox360DeviceList != (XBOX360_COMPATIBLE_DEVICE*)mXbox360BuiltinDevices) {
    // Free allocated descriptions for custom devices
    for (i = XBOX360_BUILTIN_DEVICE_COUNT; i < mXbox360DeviceCount; i++) {
      if (mXbox360DeviceList[i].Description != NULL) {
        FreePool(mXbox360DeviceList[i].Description);
      }
    }
    FreePool(mXbox360DeviceList);
  }

  mXbox360DeviceList = NULL;
  mXbox360DeviceCount = 0;
  mDeviceListInitialized = FALSE;
}

/**
  Uses USB I/O to check whether the device is an Xbox 360 compatible controller.

  This function checks the device's VID/PID against a list of known Xbox 360
  protocol compatible devices verified in the Linux kernel xpad driver.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is an Xbox 360 compatible controller.
  @retval FALSE    Device is not compatible.

**/
BOOLEAN
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                  Status;
  EFI_USB_DEVICE_DESCRIPTOR   DeviceDescriptor;
  UINTN                       Index;

  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DeviceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Initialize device list if not already done
  if (!mDeviceListInitialized) {
    // Use built-in devices only as fallback
    mXbox360DeviceList = (XBOX360_COMPATIBLE_DEVICE*)mXbox360BuiltinDevices;
    mXbox360DeviceCount = XBOX360_BUILTIN_DEVICE_COUNT;
  }

  //
  // Check against combined device list (built-in + custom)
  //
  for (Index = 0; Index < mXbox360DeviceCount; Index++) {
    if ((DeviceDescriptor.IdVendor == mXbox360DeviceList[Index].VendorId) &&
        (DeviceDescriptor.IdProduct == mXbox360DeviceList[Index].ProductId))
    {
      DEBUG ((
        DEBUG_INFO,
        "Xbox360Dxe: Found compatible device: %s (VID:0x%04X PID:0x%04X)%a\n",
        mXbox360DeviceList[Index].Description,
        DeviceDescriptor.IdVendor,
        DeviceDescriptor.IdProduct,
        (Index >= XBOX360_BUILTIN_DEVICE_COUNT) ? " [CUSTOM]" : ""
        ));
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Get current keyboard layout from HII database.

  @return Pointer to HII Keyboard Layout.
          NULL means failure occurred while trying to get keyboard layout.

**/
EFI_HII_KEYBOARD_LAYOUT *
GetCurrentKeyboardLayout (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;
  EFI_HII_KEYBOARD_LAYOUT    *KeyboardLayout;
  UINT16                     Length;

  //
  // Locate HII Database Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get current keyboard layout from HII database
  //
  Length         = 0;
  KeyboardLayout = NULL;
  Status         = HiiDatabase->GetKeyboardLayout (
                                  HiiDatabase,
                                  NULL,
                                  &Length,
                                  KeyboardLayout
                                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    KeyboardLayout = AllocatePool (Length);
    ASSERT (KeyboardLayout != NULL);
    if (KeyboardLayout != NULL) {
      Status = HiiDatabase->GetKeyboardLayout (
                              HiiDatabase,
                              NULL,
                              &Length,
                              KeyboardLayout
                              );
      if (EFI_ERROR (Status)) {
        FreePool (KeyboardLayout);
        KeyboardLayout = NULL;
      }
    }
  }

  return KeyboardLayout;
}

/**
  Find Key Descriptor in Key Convertion Table given its USB keycode.

  @param  UsbKeyboardDevice   The USB_KB_DEV instance.
  @param  KeyCode             USB Keycode.

  @return The Key Descriptor in Key Convertion Table.
          NULL means not found.

**/
EFI_KEY_DESCRIPTOR *
GetKeyDescriptor (
  IN USB_KB_DEV  *UsbKeyboardDevice,
  IN UINT8       KeyCode
  )
{
  UINT8  Index;

  //
  // Make sure KeyCode is in the range of [0x4, 0x65] or [0xe0, 0xe7]
  //
  if ((!USBKBD_VALID_KEYCODE (KeyCode)) || ((KeyCode > 0x65) && (KeyCode < 0xe0)) || (KeyCode > 0xe7)) {
    return NULL;
  }

  //
  // Calculate the index of Key Descriptor in Key Convertion Table
  //
  if (KeyCode <= 0x65) {
    Index = (UINT8)(KeyCode - 4);
  } else {
    Index = (UINT8)(KeyCode - 0xe0 + NUMBER_OF_VALID_NON_MODIFIER_USB_KEYCODE);
  }

  return &UsbKeyboardDevice->KeyConvertionTable[Index];
}

/**
  Find Non-Spacing key for given Key descriptor.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyDescriptor        Key descriptor.

  @return The Non-Spacing key corresponding to KeyDescriptor
          NULL means not found.

**/
USB_NS_KEY *
FindUsbNsKey (
  IN USB_KB_DEV          *UsbKeyboardDevice,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  LIST_ENTRY  *Link;
  LIST_ENTRY  *NsKeyList;
  USB_NS_KEY  *UsbNsKey;

  NsKeyList = &UsbKeyboardDevice->NsKeyList;
  Link      = GetFirstNode (NsKeyList);
  while (!IsNull (NsKeyList, Link)) {
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);

    if (UsbNsKey->NsKey[0].Key == KeyDescriptor->Key) {
      return UsbNsKey;
    }

    Link = GetNextNode (NsKeyList, Link);
  }

  return NULL;
}

/**
  Find physical key definition for a given key descriptor.

  For a specified non-spacing key, there are a list of physical
  keys following it. This function traverses the list of
  physical keys and tries to find the physical key matching
  the KeyDescriptor.

  @param  UsbNsKey          The non-spacing key information.
  @param  KeyDescriptor     The key descriptor.

  @return The physical key definition.
          If no physical key is found, parameter KeyDescriptor is returned.

**/
EFI_KEY_DESCRIPTOR *
FindPhysicalKey (
  IN USB_NS_KEY          *UsbNsKey,
  IN EFI_KEY_DESCRIPTOR  *KeyDescriptor
  )
{
  UINTN               Index;
  EFI_KEY_DESCRIPTOR  *PhysicalKey;

  PhysicalKey = &UsbNsKey->NsKey[1];
  for (Index = 0; Index < UsbNsKey->KeyCount; Index++) {
    if (KeyDescriptor->Key == PhysicalKey->Key) {
      return PhysicalKey;
    }

    PhysicalKey++;
  }

  //
  // No children definition matched, return original key
  //
  return KeyDescriptor;
}

/**
  The notification function for EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID.

  This function is registered to event of EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID
  group type, which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  It tries to get current keyboard layout from HII database.

  @param  Event        Event being signaled.
  @param  Context      Points to USB_KB_DEV instance.

**/
VOID
EFIAPI
SetKeyboardLayoutEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  USB_KB_DEV               *UsbKeyboardDevice;
  EFI_HII_KEYBOARD_LAYOUT  *KeyboardLayout;
  EFI_KEY_DESCRIPTOR       TempKey;
  EFI_KEY_DESCRIPTOR       *KeyDescriptor;
  EFI_KEY_DESCRIPTOR       *TableEntry;
  EFI_KEY_DESCRIPTOR       *NsKey;
  USB_NS_KEY               *UsbNsKey;
  UINTN                    Index;
  UINTN                    Index2;
  UINTN                    KeyCount;
  UINT8                    KeyCode;

  UsbKeyboardDevice = (USB_KB_DEV *)Context;
  if (UsbKeyboardDevice->Signature != USB_KB_DEV_SIGNATURE) {
    return;
  }

  //
  // Try to get current keyboard layout from HII database
  //
  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout == NULL) {
    return;
  }

  //
  // Re-allocate resource for KeyConvertionTable
  //
  ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_USB_KEYCODE)*sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  //
  // Traverse the list of key descriptors following the header of EFI_HII_KEYBOARD_LAYOUT
  //
  KeyDescriptor = (EFI_KEY_DESCRIPTOR *)(((UINT8 *)KeyboardLayout) + sizeof (EFI_HII_KEYBOARD_LAYOUT));
  for (Index = 0; Index < KeyboardLayout->DescriptorCount; Index++) {
    //
    // Copy from HII keyboard layout package binary for alignment
    //
    CopyMem (&TempKey, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // Fill the key into KeyConvertionTable, whose index is calculated from USB keycode.
    //
    KeyCode    = EfiKeyToUsbKeyCodeConvertionTable[(UINT8)(TempKey.Key)];
    TableEntry = GetKeyDescriptor (UsbKeyboardDevice, KeyCode);
    if (TableEntry == NULL) {
      ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
      FreePool (KeyboardLayout);
      return;
    }

    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));

    //
    // For non-spacing key, create the list with a non-spacing key followed by physical keys.
    //
    if (TempKey.Modifier == EFI_NS_KEY_MODIFIER) {
      UsbNsKey = AllocateZeroPool (sizeof (USB_NS_KEY));
      ASSERT (UsbNsKey != NULL);

      //
      // Search for sequential children physical key definitions
      //
      KeyCount = 0;
      NsKey    = KeyDescriptor + 1;
      for (Index2 = (UINT8)Index + 1; Index2 < KeyboardLayout->DescriptorCount; Index2++) {
        CopyMem (&TempKey, NsKey, sizeof (EFI_KEY_DESCRIPTOR));
        if (TempKey.Modifier == EFI_NS_KEY_DEPENDENCY_MODIFIER) {
          KeyCount++;
        } else {
          break;
        }

        NsKey++;
      }

      UsbNsKey->Signature = USB_NS_KEY_SIGNATURE;
      UsbNsKey->KeyCount  = KeyCount;
      UsbNsKey->NsKey     = AllocateCopyPool (
                              (KeyCount + 1) * sizeof (EFI_KEY_DESCRIPTOR),
                              KeyDescriptor
                              );
      InsertTailList (&UsbKeyboardDevice->NsKeyList, &UsbNsKey->Link);

      //
      // Skip over the child physical keys
      //
      Index         += KeyCount;
      KeyDescriptor += KeyCount;
    }

    KeyDescriptor++;
  }

  //
  // There are two EfiKeyEnter, duplicate its key descriptor
  //
  TableEntry    = GetKeyDescriptor (UsbKeyboardDevice, 0x58);
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, 0x28);

  if ((TableEntry != NULL) && (KeyDescriptor != NULL)) {
    CopyMem (TableEntry, KeyDescriptor, sizeof (EFI_KEY_DESCRIPTOR));
  }

  FreePool (KeyboardLayout);
}

/**
  Destroy resources for keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
ReleaseKeyboardLayoutResources (
  IN OUT USB_KB_DEV  *UsbKeyboardDevice
  )
{
  USB_NS_KEY  *UsbNsKey;
  LIST_ENTRY  *Link;

  if (UsbKeyboardDevice->KeyConvertionTable != NULL) {
    FreePool (UsbKeyboardDevice->KeyConvertionTable);
  }

  UsbKeyboardDevice->KeyConvertionTable = NULL;

  while (!IsListEmpty (&UsbKeyboardDevice->NsKeyList)) {
    Link     = GetFirstNode (&UsbKeyboardDevice->NsKeyList);
    UsbNsKey = USB_NS_KEY_FORM_FROM_LINK (Link);
    RemoveEntryList (&UsbNsKey->Link);

    FreePool (UsbNsKey->NsKey);
    FreePool (UsbNsKey);
  }
}

/**
  Initialize USB keyboard layout.

  This function initializes Key Convertion Table for the USB keyboard device.
  It first tries to retrieve layout from HII database. If failed and default
  layout is enabled, then it just uses the default layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization succeeded.
  @retval EFI_NOT_READY          Keyboard layout cannot be retrieve from HII
                                 database, and default layout is disabled.
  @retval Other                  Fail to register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group.

**/
EFI_STATUS
InitKeyboardLayout (
  OUT USB_KB_DEV  *UsbKeyboardDevice
  )
{
  EFI_HII_KEYBOARD_LAYOUT  *KeyboardLayout;
  EFI_STATUS               Status;

  UsbKeyboardDevice->KeyConvertionTable = AllocateZeroPool ((NUMBER_OF_VALID_USB_KEYCODE)*sizeof (EFI_KEY_DESCRIPTOR));
  ASSERT (UsbKeyboardDevice->KeyConvertionTable != NULL);

  InitializeListHead (&UsbKeyboardDevice->NsKeyList);
  UsbKeyboardDevice->CurrentNsKey        = NULL;
  UsbKeyboardDevice->KeyboardLayoutEvent = NULL;

  //
  // Register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group,
  // which will be triggered by EFI_HII_DATABASE_PROTOCOL.SetKeyboardLayout().
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SetKeyboardLayoutEvent,
                  UsbKeyboardDevice,
                  &gEfiHiiKeyBoardLayoutGuid,
                  &UsbKeyboardDevice->KeyboardLayoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyboardLayout = GetCurrentKeyboardLayout ();
  if (KeyboardLayout != NULL) {
    //
    // If current keyboard layout is successfully retrieved from HII database,
    // force to initialize the keyboard layout.
    //
    gBS->SignalEvent (UsbKeyboardDevice->KeyboardLayoutEvent);
  } else {
    if (FeaturePcdGet (PcdDisableDefaultKeyboardLayoutInUsbKbDriver)) {
      //
      // If no keyboard layout can be retrieved from HII database, and default layout
      // is disabled, then return EFI_NOT_READY.
      //
      return EFI_NOT_READY;
    }

    //
    // If no keyboard layout can be retrieved from HII database, and default layout
    // is enabled, then load the default keyboard layout.
    //
    InstallDefaultKeyboardLayout (UsbKeyboardDevice);
  }

  return EFI_SUCCESS;
}

/**
  Initialize USB keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitUSBKeyboard (
  IN OUT USB_KB_DEV  *UsbKeyboardDevice
  )
{
  UINT16      ConfigValue;
  EFI_STATUS  Status;
  UINT32      TransferResult;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Load configuration from file (or use defaults)
  //
  LoadConfigWithMigration(&mGlobalConfig);

  //
  // Initialize dynamic device list with custom devices
  //
  InitializeDeviceList(&mGlobalConfig);

  InitQueue (&UsbKeyboardDevice->UsbKeyQueue, sizeof (USB_KEY));
  InitQueue (&UsbKeyboardDevice->EfiKeyQueue, sizeof (EFI_KEY_DATA));
  InitQueue (&UsbKeyboardDevice->EfiKeyQueueForNotify, sizeof (EFI_KEY_DATA));

  //
  // Use the config out of the descriptor
  // Assumed the first config is the correct one and this is not always the case
  //
  Status = UsbGetConfiguration (
             UsbKeyboardDevice->UsbIo,
             &ConfigValue,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    ConfigValue = 0x01;
    //
    // Uses default configuration to configure the USB Keyboard device.
    //
    Status = UsbSetConfiguration (
               UsbKeyboardDevice->UsbIo,
               ConfigValue,
               &TransferResult
               );
    if (EFI_ERROR (Status)) {
      //
      // If configuration could not be set here, it means
      // the keyboard interface has some errors and could
      // not be initialized
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR),
        UsbKeyboardDevice->DevicePath
        );

      return EFI_DEVICE_ERROR;
    }
  }

  UsbKeyboardDevice->CtrlOn    = FALSE;
  UsbKeyboardDevice->AltOn     = FALSE;
  UsbKeyboardDevice->ShiftOn   = FALSE;
  UsbKeyboardDevice->NumLockOn = FALSE;
  UsbKeyboardDevice->CapsOn    = FALSE;
  UsbKeyboardDevice->ScrollOn  = FALSE;

  UsbKeyboardDevice->LeftCtrlOn   = FALSE;
  UsbKeyboardDevice->LeftAltOn    = FALSE;
  UsbKeyboardDevice->LeftShiftOn  = FALSE;
  UsbKeyboardDevice->LeftLogoOn   = FALSE;
  UsbKeyboardDevice->RightCtrlOn  = FALSE;
  UsbKeyboardDevice->RightAltOn   = FALSE;
  UsbKeyboardDevice->RightShiftOn = FALSE;
  UsbKeyboardDevice->RightLogoOn  = FALSE;
  UsbKeyboardDevice->MenuKeyOn    = FALSE;
  UsbKeyboardDevice->SysReqOn     = FALSE;

  UsbKeyboardDevice->AltGrOn = FALSE;

  UsbKeyboardDevice->CurrentNsKey = NULL;

  //
  // Initialize cached controller state used for key translation.
  //
  ZeroMem (&UsbKeyboardDevice->XboxState, sizeof (UsbKeyboardDevice->XboxState));

  //
  // Create event for repeat keys' generation.
  //
  if (UsbKeyboardDevice->RepeatTimer != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
    UsbKeyboardDevice->RepeatTimer = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_CALLBACK,
         USBKeyboardRepeatHandler,
         UsbKeyboardDevice,
         &UsbKeyboardDevice->RepeatTimer
         );

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    UsbKeyboardDevice->DelayedRecoveryEvent = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         USBKeyboardRecoveryHandler,
         UsbKeyboardDevice,
         &UsbKeyboardDevice->DelayedRecoveryEvent
         );

  return EFI_SUCCESS;
}

STATIC
VOID
QueueButtonTransition (
  IN USB_KB_DEV  *UsbKeyboardDevice,
  IN UINT8       KeyCode,
  IN BOOLEAN     IsPressed
  )
{
  USB_KEY  UsbKey;

  UsbKey.KeyCode = KeyCode;
  UsbKey.Down    = IsPressed;
  Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

  if (!IsPressed && (UsbKeyboardDevice->RepeatKey == KeyCode)) {
    UsbKeyboardDevice->RepeatKey = 0;
  }
}

STATIC
VOID
ProcessButtonChanges (
  IN USB_KB_DEV  *UsbKeyboardDevice,
  IN UINT16      OldButtons,
  IN UINT16      NewButtons
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mXbox360ButtonMap); Index++) {
    UINT16   Mask;
    BOOLEAN  WasPressed;
    BOOLEAN  IsPressed;

    Mask       = mXbox360ButtonMap[Index].ButtonMask;
    WasPressed = ((OldButtons & Mask) != 0);
    IsPressed  = ((NewButtons & Mask) != 0);

    if (WasPressed == IsPressed) {
      continue;
    }

    QueueButtonTransition (
      UsbKeyboardDevice,
      mXbox360ButtonMap[Index].UsbKeyCode,
      IsPressed
      );
  }
}

/**
  Handler function for Xbox 360 controller asynchronous interrupt transfer.

  The wired Xbox 360 controller sends a fixed length vendor specific report. This handler
  maps the controller state into synthetic USB keyboard scan codes so the device can drive
  the UEFI Simple Text Input (Ex) protocols.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
KeyboardHandler (
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Result
  )
{
  USB_KB_DEV           *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL  *UsbIo;
  UINT8                *Report;
  UINT16               OldButtons;
  UINT16               NewButtons;
  UINT32               UsbStatus;

  ASSERT (Context != NULL);

  UsbKeyboardDevice = (USB_KB_DEV *)Context;
  UsbIo             = UsbKeyboardDevice->UsbIo;

  //
  // Analyzes Result and performs corresponding action.
  //
  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR),
      UsbKeyboardDevice->DevicePath
      );

    //
    // Stop the repeat key generation if any
    //
    UsbKeyboardDevice->RepeatKey = 0;

    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerCancel,
           USBKBD_REPEAT_RATE
           );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      UsbClearEndpointHalt (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        &UsbStatus
        );
    }

    //
    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt.
    //
    UsbIo->UsbAsyncInterruptTransfer (
             UsbIo,
             UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
             FALSE,
             0,
             0,
             NULL,
             NULL
             );
    //
    // EFI_USB_INTERRUPT_DELAY is defined in USB standard for error handling.
    //
    gBS->SetTimer (
           UsbKeyboardDevice->DelayedRecoveryEvent,
           TimerRelative,
           EFI_USB_INTERRUPT_DELAY
           );

    return EFI_DEVICE_ERROR;
  }

  if ((Data == NULL) || (DataLength < 4)) {
    return EFI_SUCCESS;
  }

  Report = (UINT8 *)Data;

  //
  // Parse button state (bytes 2-3)
  //
  OldButtons = UsbKeyboardDevice->XboxState.Buttons;
  NewButtons = (UINT16)(Report[2] | ((UINT16)Report[3] << 8));
  if (OldButtons != NewButtons) {
    ProcessButtonChanges (UsbKeyboardDevice, OldButtons, NewButtons);
    UsbKeyboardDevice->XboxState.Buttons = NewButtons;
  }

  //
  // Parse trigger state (bytes 4-5)
  //
  if (DataLength >= 6) {
    UINT8    LeftTrigger;
    UINT8    RightTrigger;
    BOOLEAN  LeftTriggerPressed;
    BOOLEAN  RightTriggerPressed;
    BOOLEAN  OldLeftTrigger;
    BOOLEAN  OldRightTrigger;

    LeftTrigger = Report[4];
    RightTrigger = Report[5];

    // Check triggers against threshold
    LeftTriggerPressed = (LeftTrigger > mGlobalConfig.TriggerThreshold);
    RightTriggerPressed = (RightTrigger > mGlobalConfig.TriggerThreshold);

    // Get previous trigger states
    OldLeftTrigger = UsbKeyboardDevice->XboxState.LeftTriggerActive;
    OldRightTrigger = UsbKeyboardDevice->XboxState.RightTriggerActive;

    // Handle left trigger state change
    if (LeftTriggerPressed != OldLeftTrigger) {
      if (mGlobalConfig.LeftTriggerKey != 0xFF) {
        QueueButtonTransition(
          UsbKeyboardDevice,
          mGlobalConfig.LeftTriggerKey,
          LeftTriggerPressed
        );
      }
      UsbKeyboardDevice->XboxState.LeftTriggerActive = LeftTriggerPressed;
    }

    // Handle right trigger state change
    if (RightTriggerPressed != OldRightTrigger) {
      if (mGlobalConfig.RightTriggerKey != 0xFF) {
        QueueButtonTransition(
          UsbKeyboardDevice,
          mGlobalConfig.RightTriggerKey,
          RightTriggerPressed
        );
      }
      UsbKeyboardDevice->XboxState.RightTriggerActive = RightTriggerPressed;
    }
  }

  UsbKeyboardDevice->RepeatKey = 0;
  if (UsbKeyboardDevice->RepeatTimer != NULL) {
    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerCancel,
           USBKBD_REPEAT_RATE
           );
  }

  return EFI_SUCCESS;
}

/**
  Retrieves a USB keycode after parsing the raw data in keyboard buffer.

  This function parses keyboard buffer. It updates state of modifier key for
  USB_KB_DEV instancem, and returns keycode for output.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.
  @param  KeyCode              Pointer to the USB keycode for output.

  @retval EFI_SUCCESS          Keycode successfully parsed.
  @retval EFI_NOT_READY        Keyboard buffer is not ready for a valid keycode

**/
EFI_STATUS
USBParseKey (
  IN OUT  USB_KB_DEV  *UsbKeyboardDevice,
  OUT  UINT8          *KeyCode
  )
{
  USB_KEY             UsbKey;
  EFI_KEY_DESCRIPTOR  *KeyDescriptor;

  *KeyCode = 0;

  while (!IsQueueEmpty (&UsbKeyboardDevice->UsbKeyQueue)) {
    //
    // Pops one raw data off.
    //
    Dequeue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

    KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, UsbKey.KeyCode);
    if (KeyDescriptor == NULL) {
      continue;
    }

    if (!UsbKey.Down) {
      //
      // Key is released.
      //
      switch (KeyDescriptor->Modifier) {
        //
        // Ctrl release
        //
        case EFI_LEFT_CONTROL_MODIFIER:
          UsbKeyboardDevice->LeftCtrlOn = FALSE;
          UsbKeyboardDevice->CtrlOn     = FALSE;
          break;
        case EFI_RIGHT_CONTROL_MODIFIER:
          UsbKeyboardDevice->RightCtrlOn = FALSE;
          UsbKeyboardDevice->CtrlOn      = FALSE;
          break;

        //
        // Shift release
        //
        case EFI_LEFT_SHIFT_MODIFIER:
          UsbKeyboardDevice->LeftShiftOn = FALSE;
          UsbKeyboardDevice->ShiftOn     = FALSE;
          break;
        case EFI_RIGHT_SHIFT_MODIFIER:
          UsbKeyboardDevice->RightShiftOn = FALSE;
          UsbKeyboardDevice->ShiftOn      = FALSE;
          break;

        //
        // Alt release
        //
        case EFI_LEFT_ALT_MODIFIER:
          UsbKeyboardDevice->LeftAltOn = FALSE;
          UsbKeyboardDevice->AltOn     = FALSE;
          break;
        case EFI_RIGHT_ALT_MODIFIER:
          UsbKeyboardDevice->RightAltOn = FALSE;
          UsbKeyboardDevice->AltOn      = FALSE;
          break;

        //
        // Left Logo release
        //
        case EFI_LEFT_LOGO_MODIFIER:
          UsbKeyboardDevice->LeftLogoOn = FALSE;
          break;

        //
        // Right Logo release
        //
        case EFI_RIGHT_LOGO_MODIFIER:
          UsbKeyboardDevice->RightLogoOn = FALSE;
          break;

        //
        // Menu key release
        //
        case EFI_MENU_MODIFIER:
          UsbKeyboardDevice->MenuKeyOn = FALSE;
          break;

        //
        // SysReq release
        //
        case EFI_PRINT_MODIFIER:
        case EFI_SYS_REQUEST_MODIFIER:
          UsbKeyboardDevice->SysReqOn = FALSE;
          break;

        //
        // AltGr release
        //
        case EFI_ALT_GR_MODIFIER:
          UsbKeyboardDevice->AltGrOn = FALSE;
          break;

        default:
          break;
      }

      continue;
    }

    //
    // Analyzes key pressing situation
    //
    switch (KeyDescriptor->Modifier) {
      //
      // Ctrl press
      //
      case EFI_LEFT_CONTROL_MODIFIER:
        UsbKeyboardDevice->LeftCtrlOn = TRUE;
        UsbKeyboardDevice->CtrlOn     = TRUE;
        break;
      case EFI_RIGHT_CONTROL_MODIFIER:
        UsbKeyboardDevice->RightCtrlOn = TRUE;
        UsbKeyboardDevice->CtrlOn      = TRUE;
        break;

      //
      // Shift press
      //
      case EFI_LEFT_SHIFT_MODIFIER:
        UsbKeyboardDevice->LeftShiftOn = TRUE;
        UsbKeyboardDevice->ShiftOn     = TRUE;
        break;
      case EFI_RIGHT_SHIFT_MODIFIER:
        UsbKeyboardDevice->RightShiftOn = TRUE;
        UsbKeyboardDevice->ShiftOn      = TRUE;
        break;

      //
      // Alt press
      //
      case EFI_LEFT_ALT_MODIFIER:
        UsbKeyboardDevice->LeftAltOn = TRUE;
        UsbKeyboardDevice->AltOn     = TRUE;
        break;
      case EFI_RIGHT_ALT_MODIFIER:
        UsbKeyboardDevice->RightAltOn = TRUE;
        UsbKeyboardDevice->AltOn      = TRUE;
        break;

      //
      // Left Logo press
      //
      case EFI_LEFT_LOGO_MODIFIER:
        UsbKeyboardDevice->LeftLogoOn = TRUE;
        break;

      //
      // Right Logo press
      //
      case EFI_RIGHT_LOGO_MODIFIER:
        UsbKeyboardDevice->RightLogoOn = TRUE;
        break;

      //
      // Menu key press
      //
      case EFI_MENU_MODIFIER:
        UsbKeyboardDevice->MenuKeyOn = TRUE;
        break;

      //
      // SysReq press
      //
      case EFI_PRINT_MODIFIER:
      case EFI_SYS_REQUEST_MODIFIER:
        UsbKeyboardDevice->SysReqOn = TRUE;
        break;

      //
      // AltGr press
      //
      case EFI_ALT_GR_MODIFIER:
        UsbKeyboardDevice->AltGrOn = TRUE;
        break;

      case EFI_NUM_LOCK_MODIFIER:
        //
        // Toggle NumLock
        //
        UsbKeyboardDevice->NumLockOn = (BOOLEAN)(!(UsbKeyboardDevice->NumLockOn));
        SetKeyLED (UsbKeyboardDevice);
        break;

      case EFI_CAPS_LOCK_MODIFIER:
        //
        // Toggle CapsLock
        //
        UsbKeyboardDevice->CapsOn = (BOOLEAN)(!(UsbKeyboardDevice->CapsOn));
        SetKeyLED (UsbKeyboardDevice);
        break;

      case EFI_SCROLL_LOCK_MODIFIER:
        //
        // Toggle ScrollLock
        //
        UsbKeyboardDevice->ScrollOn = (BOOLEAN)(!(UsbKeyboardDevice->ScrollOn));
        SetKeyLED (UsbKeyboardDevice);
        break;

      default:
        break;
    }

    //
    // When encountering Ctrl + Alt + Del, then warm reset.
    //
    if (KeyDescriptor->Modifier == EFI_DELETE_MODIFIER) {
      if ((UsbKeyboardDevice->CtrlOn) && (UsbKeyboardDevice->AltOn)) {
        gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
      }
    }

    *KeyCode = UsbKey.KeyCode;
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

/**
  Initialize the key state.

  @param  UsbKeyboardDevice     The USB_KB_DEV instance.
  @param  KeyState              A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  USB_KB_DEV     *UsbKeyboardDevice,
  OUT EFI_KEY_STATE  *KeyState
  )
{
  KeyState->KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState->KeyToggleState = EFI_TOGGLE_STATE_VALID;

  if (UsbKeyboardDevice->LeftCtrlOn) {
    KeyState->KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  if (UsbKeyboardDevice->RightCtrlOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }

  if (UsbKeyboardDevice->LeftAltOn) {
    KeyState->KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }

  if (UsbKeyboardDevice->RightAltOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }

  if (UsbKeyboardDevice->LeftShiftOn) {
    KeyState->KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }

  if (UsbKeyboardDevice->RightShiftOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }

  if (UsbKeyboardDevice->LeftLogoOn) {
    KeyState->KeyShiftState |= EFI_LEFT_LOGO_PRESSED;
  }

  if (UsbKeyboardDevice->RightLogoOn) {
    KeyState->KeyShiftState |= EFI_RIGHT_LOGO_PRESSED;
  }

  if (UsbKeyboardDevice->MenuKeyOn) {
    KeyState->KeyShiftState |= EFI_MENU_KEY_PRESSED;
  }

  if (UsbKeyboardDevice->SysReqOn) {
    KeyState->KeyShiftState |= EFI_SYS_REQ_PRESSED;
  }

  if (UsbKeyboardDevice->ScrollOn) {
    KeyState->KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }

  if (UsbKeyboardDevice->NumLockOn) {
    KeyState->KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }

  if (UsbKeyboardDevice->CapsOn) {
    KeyState->KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }

  if (UsbKeyboardDevice->IsSupportPartialKey) {
    KeyState->KeyToggleState |= EFI_KEY_STATE_EXPOSED;
  }
}

/**
  Converts USB Keycode ranging from 0x4 to 0x65 to EFI_INPUT_KEY.

  @param  UsbKeyboardDevice     The USB_KB_DEV instance.
  @param  KeyCode               Indicates the key code that will be interpreted.
  @param  KeyData               A pointer to a buffer that is filled in with
                                the keystroke information for the key that
                                was pressed.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER KeyCode is not in the range of 0x4 to 0x65.
  @retval EFI_INVALID_PARAMETER Translated EFI_INPUT_KEY has zero for both ScanCode and UnicodeChar.
  @retval EFI_NOT_READY         KeyCode represents a dead key with EFI_NS_KEY_MODIFIER
  @retval EFI_DEVICE_ERROR      Keyboard layout is invalid.

**/
EFI_STATUS
UsbKeyCodeToEfiInputKey (
  IN  USB_KB_DEV    *UsbKeyboardDevice,
  IN  UINT8         KeyCode,
  OUT EFI_KEY_DATA  *KeyData
  )
{
  EFI_KEY_DESCRIPTOR             *KeyDescriptor;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;

  //
  // KeyCode must in the range of  [0x4, 0x65] or [0xe0, 0xe7].
  //
  KeyDescriptor = GetKeyDescriptor (UsbKeyboardDevice, KeyCode);
  if (KeyDescriptor == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (KeyDescriptor->Modifier == EFI_NS_KEY_MODIFIER) {
    //
    // If this is a dead key with EFI_NS_KEY_MODIFIER, then record it and return.
    //
    UsbKeyboardDevice->CurrentNsKey = FindUsbNsKey (UsbKeyboardDevice, KeyDescriptor);
    return EFI_NOT_READY;
  }

  if (UsbKeyboardDevice->CurrentNsKey != NULL) {
    //
    // If this keystroke follows a non-spacing key, then find the descriptor for corresponding
    // physical key.
    //
    KeyDescriptor                   = FindPhysicalKey (UsbKeyboardDevice->CurrentNsKey, KeyDescriptor);
    UsbKeyboardDevice->CurrentNsKey = NULL;
  }

  //
  // Make sure modifier of Key Descriptor is in the valid range according to UEFI spec.
  //
  if (KeyDescriptor->Modifier >= (sizeof (ModifierValueToEfiScanCodeConvertionTable) / sizeof (UINT8))) {
    return EFI_DEVICE_ERROR;
  }

  KeyData->Key.ScanCode    = ModifierValueToEfiScanCodeConvertionTable[KeyDescriptor->Modifier];
  KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_STANDARD_SHIFT) != 0) {
    if (UsbKeyboardDevice->ShiftOn) {
      KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;

      //
      // Need not return associated shift state if a class of printable characters that
      // are normally adjusted by shift modifiers. e.g. Shift Key + 'f' key = 'F'
      //
      if ((KeyDescriptor->Unicode != CHAR_NULL) && (KeyDescriptor->ShiftedUnicode != CHAR_NULL) &&
          (KeyDescriptor->Unicode != KeyDescriptor->ShiftedUnicode))
      {
        UsbKeyboardDevice->LeftShiftOn  = FALSE;
        UsbKeyboardDevice->RightShiftOn = FALSE;
      }

      if (UsbKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedAltGrUnicode;
      }
    } else {
      //
      // Shift off
      //
      KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;

      if (UsbKeyboardDevice->AltGrOn) {
        KeyData->Key.UnicodeChar = KeyDescriptor->AltGrUnicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_CAPS_LOCK) != 0) {
    if (UsbKeyboardDevice->CapsOn) {
      if (KeyData->Key.UnicodeChar == KeyDescriptor->Unicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->ShiftedUnicode;
      } else if (KeyData->Key.UnicodeChar == KeyDescriptor->ShiftedUnicode) {
        KeyData->Key.UnicodeChar = KeyDescriptor->Unicode;
      }
    }
  }

  if ((KeyDescriptor->AffectedAttribute & EFI_AFFECTED_BY_NUM_LOCK) != 0) {
    //
    // For key affected by NumLock, if NumLock is on and Shift is not pressed, then it means
    // normal key, instead of original control key. So the ScanCode should be cleaned.
    // Otherwise, it means control key, so preserve the EFI Scan Code and clear the unicode keycode.
    //
    if ((UsbKeyboardDevice->NumLockOn) && (!(UsbKeyboardDevice->ShiftOn))) {
      KeyData->Key.ScanCode = SCAN_NULL;
    } else {
      KeyData->Key.UnicodeChar = CHAR_NULL;
    }
  }

  //
  // Translate Unicode 0x1B (ESC) to EFI Scan Code
  //
  if ((KeyData->Key.UnicodeChar == 0x1B) && (KeyData->Key.ScanCode == SCAN_NULL)) {
    KeyData->Key.ScanCode    = SCAN_ESC;
    KeyData->Key.UnicodeChar = CHAR_NULL;
  }

  //
  // Not valid for key without both unicode key code and EFI Scan Code.
  //
  if ((KeyData->Key.UnicodeChar == 0) && (KeyData->Key.ScanCode == SCAN_NULL)) {
    if (!UsbKeyboardDevice->IsSupportPartialKey) {
      return EFI_NOT_READY;
    }
  }

  //
  // Save Shift/Toggle state
  //
  InitializeKeyState (UsbKeyboardDevice, &KeyData->KeyState);

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK
      // while current TPL is TPL_NOTIFY. It will be invoked in
      // KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      Enqueue (&UsbKeyboardDevice->EfiKeyQueueForNotify, KeyData, sizeof (*KeyData));
      gBS->SignalEvent (UsbKeyboardDevice->KeyNotifyProcessEvent);
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Create the queue.

  @param  Queue     Points to the queue.
  @param  ItemSize  Size of the single item.

**/
VOID
InitQueue (
  IN OUT  USB_SIMPLE_QUEUE  *Queue,
  IN      UINTN             ItemSize
  )
{
  UINTN  Index;

  Queue->ItemSize = ItemSize;
  Queue->Head     = 0;
  Queue->Tail     = 0;

  if (Queue->Buffer[0] != NULL) {
    FreePool (Queue->Buffer[0]);
  }

  Queue->Buffer[0] = AllocatePool (sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]) * ItemSize);
  ASSERT (Queue->Buffer[0] != NULL);

  for (Index = 1; Index < sizeof (Queue->Buffer) / sizeof (Queue->Buffer[0]); Index++) {
    Queue->Buffer[Index] = ((UINT8 *)Queue->Buffer[Index - 1]) + ItemSize;
  }
}

/**
  Destroy the queue

  @param Queue    Points to the queue.
**/
VOID
DestroyQueue (
  IN OUT USB_SIMPLE_QUEUE  *Queue
  )
{
  FreePool (Queue->Buffer[0]);
}

/**
  Check whether the queue is empty.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is empty.
  @retval FALSE     Queue is not empty.

**/
BOOLEAN
IsQueueEmpty (
  IN  USB_SIMPLE_QUEUE  *Queue
  )
{
  //
  // Meet FIFO empty condition
  //
  return (BOOLEAN)(Queue->Head == Queue->Tail);
}

/**
  Check whether the queue is full.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is full.
  @retval FALSE     Queue is not full.

**/
BOOLEAN
IsQueueFull (
  IN  USB_SIMPLE_QUEUE  *Queue
  )
{
  return (BOOLEAN)(((Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1)) == Queue->Head);
}

/**
  Enqueue the item to the queue.

  @param  Queue     Points to the queue.
  @param  Item      Points to the item to be enqueued.
  @param  ItemSize  Size of the item.
**/
VOID
Enqueue (
  IN OUT  USB_SIMPLE_QUEUE  *Queue,
  IN      VOID              *Item,
  IN      UINTN             ItemSize
  )
{
  ASSERT (ItemSize == Queue->ItemSize);
  //
  // If keyboard buffer is full, throw the
  // first key out of the keyboard buffer.
  //
  if (IsQueueFull (Queue)) {
    Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);
  }

  CopyMem (Queue->Buffer[Queue->Tail], Item, ItemSize);

  //
  // Adjust the tail pointer of the FIFO keyboard buffer.
  //
  Queue->Tail = (Queue->Tail + 1) % (MAX_KEY_ALLOWED + 1);
}

/**
  Dequeue a item from the queue.

  @param  Queue     Points to the queue.
  @param  Item      Receives the item.
  @param  ItemSize  Size of the item.

  @retval EFI_SUCCESS        Item was successfully dequeued.
  @retval EFI_DEVICE_ERROR   The queue is empty.

**/
EFI_STATUS
Dequeue (
  IN OUT  USB_SIMPLE_QUEUE  *Queue,
  OUT  VOID                 *Item,
  IN      UINTN             ItemSize
  )
{
  ASSERT (Queue->ItemSize == ItemSize);

  if (IsQueueEmpty (Queue)) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Item, Queue->Buffer[Queue->Head], ItemSize);
  ZeroMem (Queue->Buffer[Queue->Head], ItemSize);
  //
  // Adjust the head pointer of the FIFO keyboard buffer.
  //
  Queue->Head = (Queue->Head + 1) % (MAX_KEY_ALLOWED + 1);

  return EFI_SUCCESS;
}

/**
  Sets USB keyboard LED state.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

**/
VOID
SetKeyLED (
  IN  USB_KB_DEV  *UsbKeyboardDevice
  )
{
  //
  // The Xbox 360 controller interface does not expose keyboard LED output reports.
  // Consume the parameter to avoid compiler warnings and intentionally do nothing.
  //
  (VOID)UsbKeyboardDevice;
}

/**
  Handler for Repeat Key event.

  This function is the handler for Repeat Key event triggered
  by timer.
  After a repeatable key is pressed, the event would be triggered
  with interval of USBKBD_REPEAT_DELAY. Once the event is triggered,
  following trigger will come with interval of USBKBD_REPEAT_RATE.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardRepeatHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;
  USB_KEY     UsbKey;

  UsbKeyboardDevice = (USB_KB_DEV *)Context;

  //
  // Do nothing when there is no repeat key.
  //
  if (UsbKeyboardDevice->RepeatKey != 0) {
    //
    // Inserts the repeat key into keyboard buffer,
    //
    UsbKey.KeyCode = UsbKeyboardDevice->RepeatKey;
    UsbKey.Down    = TRUE;
    Enqueue (&UsbKeyboardDevice->UsbKeyQueue, &UsbKey, sizeof (UsbKey));

    //
    // Set repeat rate for next repeat key generation.
    //
    gBS->SetTimer (
           UsbKeyboardDevice->RepeatTimer,
           TimerRelative,
           USBKBD_REPEAT_RATE
           );
  }
}

/**
  Handler for Delayed Recovery event.

  This function is the handler for Delayed Recovery event triggered
  by timer.
  After a device error occurs, the event would be triggered
  with interval of EFI_USB_INTERRUPT_DELAY. EFI_USB_INTERRUPT_DELAY
  is defined in USB standard for error handling.

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardRecoveryHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  )
{
  USB_KB_DEV           *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL  *UsbIo;
  UINT8                PacketSize;

  UsbKeyboardDevice = (USB_KB_DEV *)Context;

  UsbIo = UsbKeyboardDevice->UsbIo;

  PacketSize = (UINT8)(UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbKeyboardDevice->IntEndpointDescriptor.Interval,
           PacketSize,
           KeyboardHandler,
           UsbKeyboardDevice
           );
}
