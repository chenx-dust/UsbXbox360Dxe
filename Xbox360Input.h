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
// Forward declarations
//
typedef struct _USB_KB_DEV USB_KB_DEV;

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

