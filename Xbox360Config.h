/** @file
  Xbox 360 Driver Configuration System Header

  This module provides configuration file parsing, validation, and management
  for the Xbox 360 controller driver.

  Copyright (c) 2024-2025. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _XBOX360_CONFIG_H_
#define _XBOX360_CONFIG_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include "Xbox360Device.h"

//
// Configuration version
//
#define XBOX360_CONFIG_VERSION_CURRENT  0x0100  // Version 1.0

//
// Configuration file path
//
#define XBOX360_CONFIG_PATH  L"\\EFI\\Xbox360\\config.ini"

//
// Analog stick mode
//
typedef enum {
  STICK_MODE_DISABLED = 0,
  STICK_MODE_KEYS = 1,
  STICK_MODE_MOUSE = 2,
  STICK_MODE_SCROLL = 3
} STICK_MODE;

//
// Analog stick configuration
//
typedef struct {
  STICK_MODE  Mode;                   // Stick mode (keys/mouse/scroll/disabled)
  UINT16      Deadzone;               // Deadzone value (0-32767)
  UINT16      Saturation;             // Saturation value (0-32767)
  UINT8       MouseSensitivity;       // Mouse sensitivity (1-100)
  UINT8       MouseMaxSpeed;          // Maximum mouse speed (pixels/poll)
  UINT8       MouseCurve;             // Response curve (1=linear, 2=square, 3=s-curve)
  UINT8       DirectionMode;          // Direction mode for keys (4 or 8 way)
  UINT8       UpMapping;              // Key mapping for up direction
  UINT8       DownMapping;            // Key mapping for down direction
  UINT8       LeftMapping;            // Key mapping for left direction
  UINT8       RightMapping;           // Key mapping for right direction
  UINT8       ScrollSensitivity;      // Scroll sensitivity (1-100)
  UINT16      ScrollDeadzone;         // Scroll deadzone (0 = use standard deadzone)
} STICK_CONFIG;

//
// Main configuration structure
//
typedef struct _XBOX360_CONFIG {
  UINT16                        Version;              // Configuration version
  UINT16                        StickDeadzone;        // Global stick deadzone
  UINT8                         TriggerThreshold;     // Trigger activation threshold
  UINT8                         LeftTriggerKey;       // Left trigger key mapping
  UINT8                         RightTriggerKey;      // Right trigger key mapping
  UINT8                         ButtonMap[16];        // Button to key mappings
  UINTN                         CustomDeviceCount;    // Number of custom devices
  XBOX360_COMPATIBLE_DEVICE     CustomDevices[MAX_CUSTOM_DEVICES]; // Custom device list
  STICK_CONFIG                  LeftStick;            // Left analog stick configuration
  STICK_CONFIG                  RightStick;           // Right analog stick configuration
  UINT8                         Reserved[32];         // Reserved for future expansion
} XBOX360_CONFIG;

/**
  Load configuration with version migration support.
  
  If no configuration file is found, defaults are used and a template
  is generated. Always returns success with defaults if loading fails.

  @param  Config   Pointer to configuration structure to populate

  @retval EFI_SUCCESS            Configuration loaded (or defaults used)
  @retval EFI_INVALID_PARAMETER  Config pointer is NULL
**/
EFI_STATUS
LoadConfigWithMigration (
  OUT XBOX360_CONFIG  *Config
  );

/**
  Get pointer to global configuration.
  
  The global configuration should be loaded once at driver initialization.

  @retval Pointer to global configuration structure
**/
XBOX360_CONFIG *
GetGlobalConfig (
  VOID
  );

#endif // _XBOX360_CONFIG_H_

