/** @file
  ASUS ROG Ally Device Implementation
  
  This module provides DirectInput support for ASUS ROG Ally devices.
  Implementation is based on Linux kernel's hid-asus.c driver.
  
  References:
  - Linux kernel: drivers/hid/hid-asus.c
  - Linux kernel: drivers/input/joystick/xpad.c
  - https://github.com/torvalds/linux/blob/master/drivers/hid/hid-asus.c

  Copyright (c) 2024-2025. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AsusAllyDevice.h"
#include "Xbox360Log.h"
#include "KeyBoard.h"

/**
  Check if the given USB device is an ASUS ROG Ally device.
  
  For Ally X, we need to bind to the gamepad interface (endpoint 0x87).
  The device has multiple interfaces (keyboard/mouse/config/gamepad),
  we only want the gamepad one.
  
  @param  UsbIo    Pointer to USB I/O Protocol
  
  @retval TRUE     Device is ASUS ROG Ally gamepad interface
  @retval FALSE    Device is not an ASUS ROG Ally or wrong interface
**/
BOOLEAN
IsAsusAlly (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                     Status;
  EFI_USB_DEVICE_DESCRIPTOR      DeviceDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR   InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR    EndpointDescriptor;
  UINT8                          Index;
  UINT8                          EndpointAddr;
  BOOLEAN                        FoundGamepadEndpoint;

  if (UsbIo == NULL) {
    return FALSE;
  }

  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DeviceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check for ASUS vendor ID and known Ally PIDs
  if (DeviceDescriptor.IdVendor != ASUS_VENDOR_ID) {
    return FALSE;
  }

  // Only support Ally X (0x1B4C)
  // Original Ally (0x1ABE) has XInput mode and doesn't need DirectInput conversion
  if (DeviceDescriptor.IdProduct != ASUS_ALLY_X_PID) {
    return FALSE;
  }

  LOG_INFO ("ASUS ROG Ally X detected: VID:0x%04X PID:0x%04X",
            DeviceDescriptor.IdVendor,
            DeviceDescriptor.IdProduct);

  //
  // Get interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    LOG_WARN ("Failed to get interface descriptor: %r", Status);
    return FALSE;
  }

  //
  // Ally X gamepad uses endpoint 0x87
  // Check all endpoints to find it
  //
  FoundGamepadEndpoint = FALSE;
  for (Index = 0; Index < InterfaceDescriptor.NumEndpoints; Index++) {
    Status = UsbIo->UsbGetEndpointDescriptor (UsbIo, Index, &EndpointDescriptor);
    if (EFI_ERROR (Status)) {
      continue;
    }

    EndpointAddr = EndpointDescriptor.EndpointAddress;
    
    // Check if this is endpoint 0x87 (Ally X gamepad)
    if (EndpointAddr == HID_ALLY_X_INTF_IN) {
      FoundGamepadEndpoint = TRUE;
      LOG_INFO ("Found Ally X gamepad interface at endpoint 0x%02X", EndpointAddr);
      break;
    }
  }

  if (!FoundGamepadEndpoint) {
    LOG_INFO ("This interface does not have gamepad endpoint, skipping");
    return FALSE;
  }

  LOG_INFO ("ASUS ROG Ally gamepad interface confirmed");
  return TRUE;
}

/**
  Initialize ASUS ROG Ally device for input.
  
  Based on Linux kernel: drivers/hid/asus-ally-hid/asus-ally-hid-core.c
  Sends EC initialization string and checks device ready status.
  
  @param  UsbIo    Pointer to USB I/O Protocol
  
  @retval EFI_SUCCESS     Device initialized successfully
  @retval Other           Initialization failed
**/
EFI_STATUS
InitializeAsusAlly (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  UsbStatus;
  UINT8                   Buffer[64];
  UINTN                   Retry;
  
  // EC_INIT_STRING from Linux kernel
  STATIC CONST UINT8 EcInitString[] = { 
    0x5A, 'A', 'S', 'U', 'S', ' ', 'T', 'e', 'c', 'h', '.', 'I', 'n', 'c', '.', '\0' 
  };

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  LOG_INFO ("Initializing ASUS ROG Ally device...");

  //
  // Send EC initialization string (HID Feature Report)
  // Report ID: 0x5A
  //
  ZeroMem (Buffer, sizeof(Buffer));
  CopyMem (Buffer, EcInitString, sizeof(EcInitString));
  
  Request.RequestType = 0x21;  // Host to Device, Class, Interface
  Request.Request     = 0x09;  // SET_REPORT
  Request.Value       = 0x035A; // Report Type (Feature=0x03) | Report ID (0x5A)
  Request.Index       = 0;      // Interface 0
  Request.Length      = sizeof(Buffer);

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataOut,
                    200,  // 200ms timeout
                    Buffer,
                    sizeof(Buffer),
                    &UsbStatus
                    );

  if (EFI_ERROR (Status)) {
    LOG_ERROR ("Failed to send EC init string: %r", Status);
    return Status;
  }
  
  LOG_INFO ("EC init string sent successfully");
  
  // Small delay after init
  gBS->Stall (50000);  // 50ms

  //
  // Check device ready status (CMD_CHECK_READY)
  //
  for (Retry = 0; Retry < 3; Retry++) {
    ZeroMem (Buffer, sizeof(Buffer));
    Buffer[0] = 0x5A;  // Report ID
    Buffer[1] = 0xD1;  // Feature code page
    Buffer[2] = 0x0A;  // CMD_CHECK_READY
    Buffer[3] = 0x01;  // Length
    
    Request.RequestType = 0x21;
    Request.Request     = 0x09;
    Request.Value       = 0x035A;
    Request.Index       = 0;
    Request.Length      = sizeof(Buffer);
    
    Status = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &Request,
                      EfiUsbDataOut,
                      100,
                      Buffer,
                      sizeof(Buffer),
                      &UsbStatus
                      );
    
    if (!EFI_ERROR (Status)) {
      // Try to read response
      Request.RequestType = 0xA1;  // Device to Host
      Request.Request     = 0x01;  // GET_REPORT
      Request.Value       = 0x030D; // Report ID 0x0D
      
      Status = UsbIo->UsbControlTransfer (
                        UsbIo,
                        &Request,
                        EfiUsbDataIn,
                        100,
                        Buffer,
                        sizeof(Buffer),
                        &UsbStatus
                        );
      
      if (!EFI_ERROR (Status) && Buffer[2] == 0x0A) {
        LOG_INFO ("ASUS ROG Ally ready check passed");
        return EFI_SUCCESS;
      }
    }
    
    gBS->Stall (2000);  // 2ms delay between retries
  }

  LOG_WARN ("ASUS ROG Ally ready check failed, continuing anyway");
  return EFI_SUCCESS;  // Non-critical, device might still work
}

/**
  Parse ASUS ROG Ally HID report and convert to Xbox 360 format.
  
  This is the key function that translates ASUS ROG Ally DirectInput reports
  into Xbox 360 format, allowing the rest of the driver to work unchanged.
  
  Implementation based on Linux kernel: 
  drivers/hid/asus-ally-hid/asus-ally-hid-input.c
  
  @param  AllyReport   Pointer to ASUS Ally HID report data
  @param  ReportLen    Length of the report
  @param  XboxReport   Output buffer for Xbox 360 format report (20 bytes min)
  
  @retval EFI_SUCCESS     Report converted successfully
  @retval Other           Conversion failed
**/
EFI_STATUS
ConvertAsusAllyToXbox360 (
  IN  VOID    *AllyReport,
  IN  UINTN   ReportLen,
  OUT UINT8   *XboxReport
  )
{
  ASUS_ALLY_HID_REPORT  *Ally;
  UINT16                XboxButtons;
  UINT8                 DPadBits;
  INT16                 StickValue;

  if ((AllyReport == NULL) || (XboxReport == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Validate report length (17 bytes: 1 report ID + 16 data)
  if (ReportLen < 17) {
    LOG_WARN ("ASUS Ally report too short: %d bytes", (UINT32)ReportLen);
    return EFI_INVALID_PARAMETER;
  }

  Ally = (ASUS_ALLY_HID_REPORT *)AllyReport;

  // Check report ID (0x0B for Ally X gamepad report)
  if (Ally->ReportId != 0x0B) {
    LOG_INFO ("ASUS Ally unexpected report ID: 0x%02X", Ally->ReportId);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert to Xbox 360 report format:
  // Byte 0:    Message type (0x00)
  // Byte 1:    Packet size (0x14 = 20 bytes)
  // Byte 2-3:  Button state (16 bits)
  // Byte 4:    Left trigger (0-255)
  // Byte 5:    Right trigger (0-255)
  // Byte 6-7:  Left stick X (INT16)
  // Byte 8-9:  Left stick Y (INT16)
  // Byte 10-11: Right stick X (INT16)
  // Byte 12-13: Right stick Y (INT16)
  //
  ZeroMem (XboxReport, 20);
  XboxReport[0] = 0x00;  // Message type
  XboxReport[1] = 0x14;  // Packet size

  //
  // Convert buttons from ASUS format to Xbox 360 format
  // Xbox 360 button layout (16 bits):
  // Bit 0:  D-pad Up
  // Bit 1:  D-pad Down
  // Bit 2:  D-pad Left
  // Bit 3:  D-pad Right
  // Bit 4:  Start
  // Bit 5:  Back
  // Bit 6:  Left Stick
  // Bit 7:  Right Stick
  // Bit 8:  Left Bumper
  // Bit 9:  Right Bumper
  // Bit 10: Guide
  // Bit 11: (unused)
  // Bit 12: A
  // Bit 13: B
  // Bit 14: X
  // Bit 15: Y
  //
  XboxButtons = 0;

  // Map D-Pad (hatswitch in Buttons[2])
  DPadBits = 0;
  switch (Ally->Buttons[2]) {
    case ALLY_DPAD_UP:
      DPadBits = BIT0;  // Up
      break;
    case ALLY_DPAD_UP_RIGHT:
      DPadBits = BIT0 | BIT3;  // Up + Right
      break;
    case ALLY_DPAD_RIGHT:
      DPadBits = BIT3;  // Right
      break;
    case ALLY_DPAD_DOWN_RIGHT:
      DPadBits = BIT1 | BIT3;  // Down + Right
      break;
    case ALLY_DPAD_DOWN:
      DPadBits = BIT1;  // Down
      break;
    case ALLY_DPAD_DOWN_LEFT:
      DPadBits = BIT1 | BIT2;  // Down + Left
      break;
    case ALLY_DPAD_LEFT:
      DPadBits = BIT2;  // Left
      break;
    case ALLY_DPAD_UP_LEFT:
      DPadBits = BIT0 | BIT2;  // Up + Left
      break;
    case ALLY_DPAD_NEUTRAL:
    default:
      DPadBits = 0;
      break;
  }
  XboxButtons |= DPadBits;

  // Map face buttons (Buttons[0])
  if (Ally->Buttons[0] & ALLY_BTN_A)    XboxButtons |= BIT12;  // A
  if (Ally->Buttons[0] & ALLY_BTN_B)    XboxButtons |= BIT13;  // B
  if (Ally->Buttons[0] & ALLY_BTN_X)    XboxButtons |= BIT14;  // X
  if (Ally->Buttons[0] & ALLY_BTN_Y)    XboxButtons |= BIT15;  // Y

  // Map shoulder buttons (Buttons[0])
  if (Ally->Buttons[0] & ALLY_BTN_LB)   XboxButtons |= BIT8;   // Left Bumper
  if (Ally->Buttons[0] & ALLY_BTN_RB)   XboxButtons |= BIT9;   // Right Bumper

  // Map menu buttons (Buttons[0])
  if (Ally->Buttons[0] & ALLY_BTN_VIEW) XboxButtons |= BIT5;   // Back/View
  if (Ally->Buttons[0] & ALLY_BTN_MENU) XboxButtons |= BIT4;   // Start/Menu

  // Map stick buttons (Buttons[1])
  if (Ally->Buttons[1] & ALLY_BTN_L3)   XboxButtons |= BIT6;   // Left Stick
  if (Ally->Buttons[1] & ALLY_BTN_R3)   XboxButtons |= BIT7;   // Right Stick

  // Map Guide button (Buttons[1])
  if (Ally->Buttons[1] & ALLY_BTN_MODE) XboxButtons |= BIT10;  // Guide

  // Write button state
  XboxReport[2] = (UINT8)(XboxButtons & 0xFF);
  XboxReport[3] = (UINT8)((XboxButtons >> 8) & 0xFF);

  //
  // Convert triggers: Ally uses 0-1023 (10-bit), Xbox 360 uses 0-255 (8-bit)
  // Divide by 4 to convert
  //
  XboxReport[4] = (UINT8)(Ally->LeftTrigger >> 2);   // 1023/4 = 255
  XboxReport[5] = (UINT8)(Ally->RightTrigger >> 2);

  //
  // Convert analog sticks: Ally uses 0-65535, Xbox 360 uses -32768 to 32767
  // Subtract 32768 from Ally values to convert
  //
  StickValue = (INT16)(Ally->LeftStickX - 32768);
  CopyMem (&XboxReport[6], &StickValue, sizeof(INT16));
  
  StickValue = (INT16)(Ally->LeftStickY - 32768);
  CopyMem (&XboxReport[8], &StickValue, sizeof(INT16));
  
  StickValue = (INT16)(Ally->RightStickX - 32768);
  CopyMem (&XboxReport[10], &StickValue, sizeof(INT16));
  
  StickValue = (INT16)(Ally->RightStickY - 32768);
  CopyMem (&XboxReport[12], &StickValue, sizeof(INT16));

  return EFI_SUCCESS;
}

