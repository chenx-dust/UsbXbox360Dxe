/** @file
  Xbox 360 Controller Input Processing Header

  This module provides input processing for Xbox 360 controllers, including
  button mapping, analog stick processing, and USB interrupt handling.

  Copyright (c) 2024-2025. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _XBOX360_INPUT_H_
#define _XBOX360_INPUT_H_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UsbIo.h>

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
// Analog stick direction bit definitions
//
#define STICK_DIR_UP       BIT0
#define STICK_DIR_DOWN     BIT1
#define STICK_DIR_LEFT     BIT2
#define STICK_DIR_RIGHT    BIT3

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
  );

#endif // _XBOX360_INPUT_H_

