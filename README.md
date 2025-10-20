# UEFI Driver for Xbox 360 Controller

This driver is modified from [edk2](https://github.com/tianocore/edk2) USB keyboard driver, with AI-assistance. It maps Xbox 360 controller buttons and triggers to keyboard keys, enabling controller use in UEFI environments (BIOS, bootloaders, etc.).

## Features

- **40+ Built-in Device Support**: Xbox 360 controllers, handheld gaming devices (GPD, OneXPlayer, Legion Go, MSI Claw, etc.), 8BitDo, Logitech, HyperX, and more
- **Trigger Button Support**: Left and Right triggers can be mapped to keyboard keys
- **Configuration File Support**: Customize settings via INI file on ESP partition
- **Custom Device Support**: Add your own Xbox 360 protocol compatible devices
- **Auto-configuration**: Driver creates default config on first boot

## Default Key Mappings

### D-Pad & Face Buttons
- **D-Pad Up/Down/Left/Right**: Arrow Keys
- **A Button**: Enter
- **B Button**: Escape
- **X Button**: Backspace
- **Y Button**: Tab

### Shoulder Buttons & Triggers
- **Left/Right Shoulder (LB/RB)**: Page Up/Down
- **Left/Right Thumb Click**: Left Control/Alt
- **Left Trigger (LT)**: Delete (default threshold: 128/255)
- **Right Trigger (RT)**: End (default threshold: 128/255)

### Special Buttons
- **Start**: Space
- **Back**: Tab
- **Guide**: Left Shift

## Configuration

The driver supports configuration via an INI file on your ESP partition. On first boot, the driver will automatically create a default configuration file at `\EFI\Xbox360\config.ini`.

### Configuration File Location

- **Linux**: `/boot/efi/EFI/Xbox360/config.ini`
- **Windows**: `X:\EFI\Xbox360\config.ini` (where X: is your ESP mount point)

### Editing Configuration

1. Mount your ESP partition
2. Edit `\EFI\Xbox360\config.ini` (see `config.ini.example` for all options)
3. Reboot to apply changes

### Configuration Options

- **Deadzone**: Analog stick deadzone (0-32767, default: 8000)
- **TriggerThreshold**: Trigger activation threshold (0-255, default: 128)
- **LeftTrigger/RightTrigger**: Key mappings for triggers (USB HID scan codes)
- **Custom Devices**: Add your own Xbox 360 compatible devices

Example configuration:
```ini
Version=1.0
Deadzone=8000
TriggerThreshold=128
LeftTrigger=0x4C    # Delete
RightTrigger=0x4D   # End

# Add custom devices
# Device1=0x1234:0x5678:My Custom Controller
```

## Adding Custom Devices

If your Xbox 360 compatible controller isn't recognized:

1. Find your device's VID and PID:
   - **Linux**: `lsusb`
   - **Windows**: Device Manager → Properties → Hardware Ids
   - **macOS**: System Information → USB

2. Add to config file:
   ```ini
   [CustomDevices]
   Device1=0x1234:0x5678:My Custom Xbox Controller
   ```

3. Reboot

The driver will log detected devices to debug output.

## Supported Devices

### Built-in Support (40+ devices)

- **Microsoft**: Xbox 360 Wired/Wireless controllers
- **Handheld Gaming Devices**:
  - GPD Win 2
  - OneXPlayer
  - Lenovo Legion Go / Legion Go S
  - MSI Claw
  - TECNO Pocket Go
  - ZOTAC Gaming Zone
- **8BitDo**: Ultimate, Pro 2, SN30 Pro, Ultimate 2/2C
- **Logitech**: F310, F510, F710, Chillstream
- **HyperX**: Clutch (wired/wireless)
- **SteelSeries**: Stratus Duo
- **Razer**: Onza Tournament/Classic, Sabertooth
- And many more...

See `KeyBoard.c` for the complete list of supported devices.

## Troubleshooting

**Q: My controller isn't recognized**
A: Add it as a custom device in the config file (see above).

**Q: Triggers aren't working**
A: Check that `TriggerThreshold` is appropriate for your controller (try lowering it).

**Q: Changes don't take effect**
A: Ensure:
1. The config file is in the correct location
2. The file is saved in UTF-8 or ASCII format
3. You've rebooted after making changes

**Q: How do I know if the driver is working?**
A: Try using the controller in your UEFI BIOS menu. The controller should work like a keyboard.

## License

This project inherits the license of original driver, BSD-2-Clause-Patent.

Copyright (c) 2025, Chenx Dust. All rights reserved.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.
