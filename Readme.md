# ESP32-S3 USB to BLE Keyboard Bridge# ESP32-S3 USB to BLE Keyboard Bridge



A complete USB Host to Bluetooth Low Energy (BLE) HID bridge for the **ESP32-S3 DevKitC-1**. Forwards keyboard, mouse, and media controls from USB devices to Bluetooth hosts like Windows, macOS, Android, and iOS.Transform any USB keyboard into a Bluetooth wireless keyboard using ESP32-S3's native USB-OTG hardware.



Specifically optimized for the **Keychron Q1 Wireless Keyboard** with rotary knob support, but works with any standard USB HID keyboard and mouse.## What This Does



## Features```

┌─────────────┐     USB-C OTG     ┌────────────┐     Bluetooth     ┌──────────────┐

✅ **Full USB HID Support**│ USB Keyboard│ ◄──────────────►  │ ESP32-S3   │ ◄───────────────► │ PC / Phone   │

- USB Keyboard (boot protocol with 6-key rollover)└─────────────┘                   └────────────┘                   └──────────────┘

- USB Mouse (boot protocol with 3 buttons + wheel)```

- Consumer Control (media keys, knob rotation, knob press)

Plug a USB keyboard into ESP32-S3's USB-C port and it becomes a Bluetooth keyboard that can connect to any BLE-compatible device (Windows, macOS, Linux, iOS, Android).

✅ **Complete BLE HID Device**

- Keyboard with modifiers and 6 simultaneous key support[![Demo](https://img.youtube.com/vi/dVUMYTfJw0s/maxresdefault.jpg)](https://youtu.be/dVUMYTfJw0s)

- Mouse with movement, 3 buttons, and scroll wheel

- Media controls (mute, volume up/down, next/previous track, play/pause)## Features



✅ **Keychron Q1 Specific Features**- **Native USB Host** - Uses ESP32-S3's hardware USB-OTG (no software emulation)

- Rotary knob volume control (left = volume down, right = volume up)- **Multi-Device Support** - Switch between 3 paired devices with a key combo

- Knob press detection (mute/unmute toggle)- **Low Latency** - Direct HID report forwarding

- Full Bluetooth name visibility- **Universal Compatibility** - Works with Windows, macOS, Linux, iOS, Android, Smart TVs

- Smooth, responsive input forwarding

## Multi-Device Switching

✅ **Production Ready**

- Proper HID descriptors for Windows/Mac/Linux/iOS/Android compatibilityYou can pair with up to **3 different devices** (e.g., PC, Laptop, Tablet) and switch between them using your keyboard.

- NimBLE-based implementation (lightweight, efficient BLE stack)

- Boot protocol support for maximum device compatibility| Key Combo | Action | Device Name |

- Single device persistent bonding mode|-----------|--------|-------------|

| **Scroll Lock + 1** | Switch to Device 1 | `USB-BLE Dev 1` |

## Hardware Requirements| **Scroll Lock + 2** | Switch to Device 2 | `USB-BLE Dev 2` |

| **Scroll Lock + 3** | Switch to Device 3 | `USB-BLE Dev 3` |

- **ESP32-S3 DevKitC-1** with native USB Host OTG capability

  - USB-C port in Host mode (GPIO 19-20 for D+/D-)**How it works:**

  1. Press `Scroll Lock + 1`. Pair "USB-BLE Dev 1" with your first computer.

- **Keychron Q1 Wireless** keyboard (or any USB HID keyboard/mouse)2. Press `Scroll Lock + 2`. The connection drops. Pair "USB-BLE Dev 2" with your second device.

3. Switch back and forth instantly using the key combos!

- **Powered USB Hub** (required - USB-C port doesn't output 5V)4. The active slot is **saved** and restored on reboot.

  - Connect ESP32-S3 USB-C to hub5. The **LED** on GPIO 2 blinks to indicate the current slot (1, 2, or 3 blinks).

  - Hub provides power to keyboard

## USB Keyboard Power

- **Serial Connection** (optional - for debugging)

  - CH340 or similar USB-to-Serial for monitoring**The USB-C port on most ESP32-S3 boards does NOT output 5V!**



## Project StructureEven if you power the ESP32-S3 from the 5V pin, that power is NOT routed to the USB-C VBUS line.



```### Solutions

src/

├── main.cpp           # Application entry point#### Option 1: Powered USB Hub (Recommended)

├── Bridge.h/cpp       # USB to BLE bridge (USB → BLE forwarding)

├── BleDevice.h/cpp    # BLE HID device (NimBLE-based)Use a powered USB hub between ESP32-S3 and keyboard:

├── USBManager.h/cpp   # USB Host HID driver wrapper

├── Config.h           # Configuration constants```

├── NVSUtils.h/cpp     # Non-volatile storage (legacy)ESP32-S3 USB-C ──► [Powered USB Hub] ──► USB Keyboard

└── BLEManager.*       # Deprecated (merged into BleDevice)                         ▲

```                    External 5V

```

## Data Flow Architecture

#### Option 2: External Power to Keyboard (untested)

```

┌────────────────────────────────────────────────────────────┐Power the keyboard directly, use USB-C only for data:

│                    USB Keyboard/Mouse/Knob                 │

└────────────────────┬─────────────────────────────────────┘```

                     │ USB HID Reports                      ┌──────────────┐

                     ↓    5V Power ─────────┤ USB Breakout ├──── USB Keyboard

        ┌────────────────────────────┐    Supply    GND ────┤    Board     │

        │      USBManager            │                      └──────┬───────┘

        │  (USB Host HID Driver)     │                             │ D+/D- only (data)

        └────────────────┬───────────┘                      ┌──────┴───────┐

                         │ Raw HID Data via Callbacks                      │   ESP32-S3   │

                         ↓                      │   USB-C port │

        ┌────────────────────────────┐                      └──────────────┘

        │       Bridge               │```

        │  • Parse reports           │

        │  • Handle modifiers        │**Steps:**

        │  • Throttle mouse          │1. Cut a USB cable or get a USB breakout board

        │  • Map consumer codes      │2. Connect **5V and GND** from your power supply directly to keyboard's USB power

        └────────────────┬───────────┘3. Connect only **D+ and D-** (data lines) through the ESP32-S3 USB-C port

                         │ Structured Data

                         ↓#### Option 3: ESP32-S3-USB-OTG Board

        ┌────────────────────────────┐

        │      BleDevice             │The **ESP32-S3-USB-OTG** development board has a dedicated USB-A host port with proper 5V output - no modifications needed.

        │  • Format HID reports      │

        │  • Manage BLE connection   │#### Option 4: Modify DevKit (Advanced)

        │  • Send via GATT notify    │

        └────────────────┬───────────┘Some boards have solder pads to enable 5V on USB-C. Check your board's schematic for pads labeled "USB_OTG" or similar.

                         │ BLE Notifications

                         ↓## Quick Start

        ┌────────────────────────────┐

        │   BLE Host (Windows/Mac)   │### 1. Clone and Build

        │  • Receives HID reports    │

        │  • Updates UI              │```bash

        └────────────────────────────┘git clone https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE

```cd ESP32S3-USB-Keyboard-To-BLE



## Installation# Build with PlatformIO

pio run

### Prerequisites

- PlatformIO (VS Code extension or CLI)# Upload to ESP32-S3

- Python 3.7+pio run -t upload

- CH340 USB-to-Serial driver (optional, for serial monitoring)```



### Steps## Configuration



1. **Clone Repository**### Device Name

   ```bash

   git clone https://github.com/KoStard/ESP32S3-USB-Keyboard-To-BLE.gitEdit `src/Config.h`:

   cd ESP32S3-USB-Keyboard-To-BLE

   ``````cpp

#define DEVICE_NAME_1 "USB-BLE Dev 1"

2. **Open in VS Code**#define DEVICE_MANUFACTURER "Custom"

   - Install PlatformIO extension```

   - Open workspace folder

   - PlatformIO auto-detects configuration### Board Selection



3. **Build & Upload**The project is configured for `esp32-s3-devkitc-1`. For other boards, edit `platformio.ini`:

   ```bash

   # Build firmware```ini

   pio run -e esp32s3_usb_bleboard = esp32-s3-devkitc-1  ; Change to your board

   ```

   # Upload to device

   pio run -e esp32s3_usb_ble -t uploadYour board needs to have USB-OTG.

   

   # Monitor serial output (optional)## Hardware Limitations and Known Issues

   pio device monitor -e esp32s3_usb_ble -b 115200

   ```### USB Host Channel Limits

The ESP32-S3 has a hardware limitation on the number of USB Host channels (pipes).

4. **Connect Hardware**

   - Connect powered USB hub to ESP32-S3 USB-C port**Consequence:** If you connect a complex hub with multiple devices (e.g., a "Gaming" keyboard that shows up as 3-4 different HID interfaces + a mouse + a hub), you may run out of hardware channels.

   - Connect Keychron Q1 to USB hub- **Symptoms:** You will see `No more HCD channels available` in the Serial logs, and some devices or interfaces will fail to initialize.

   - Power on keyboard- **Recommendation:** Use a simple USB hub and avoid devices that present too many virtual interfaces if you plan to use them simultaneously.


5. **Pair Bluetooth Device**
   - Open Settings → Bluetooth on your target device
   - Scan for "Keychron Q1 Wireless"
   - Select and pair
   - Start typing - input should appear!

## Usage

### Serial Debug Output

All operations logged with prefixes:

```
[System] Initializing USB-to-BLE Bridge...
[System] Starting BLE device...
[System] Starting USB host...
[System] Bridge initialized - waiting for connections...
[USB] KEYBOARD connected!
[System] BLE Status: CONNECTED

[KEYBOARD] Modifier: 0x00 | Keys: [04 00 00 00 00 00]   # 'A' key pressed
[BLE] Sending keyboard report...

[MOUSE] Buttons: 0x01 | X: 5 | Y: -3 | Wheel: 0         # Left click + movement
[BLE] Sending mouse report...

[CONSUMER] Code: 0xE9 (VOLUME_UP)                       # Knob right
[BLE] Media: Volume Up
[BLE] Sending media report: 0x0020 (data: 20 00)
```

### Normal Operation

1. USB keyboard connects automatically to ESP32-S3
2. First BLE connection pairing required
3. Keyboard/mouse/knob inputs forwarded instantly to connected device
4. Disconnect/reconnect handled automatically
5. Bonding persists across power cycles

## Configuration

### Device Name

Edit `src/Config.h`:

```cpp
#define DEVICE_NAME "Keychron Q1 Wireless"  // Bluetooth device name
#define DEVICE_MANUFACTURER "Espressif"
#define BATTERY_LEVEL 100
```

### Feature Flags

In `src/Config.h`:

```cpp
#define ENABLE_DEVICE_SWITCHING 0  // Single device mode (1 = multi-device)
```

### Build Configuration

In `platformio.ini`:

```ini
[env:esp32s3_usb_ble]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
build_flags = 
    -DARDUINO_USB_MODE=0         ; Host mode (critical!)
    -DARDUINO_USB_CDC_ON_BOOT=0  ; No serial on USB (critical!)
    -DCORE_DEBUG_LEVEL=4         ; Debug logging
    -DUSE_NIMBLE                 ; Lightweight BLE
```

## Technical Details

### USB HID Boot Protocol

**Keyboard Report (8 bytes)**
```c
struct {
  uint8_t modifier;  // Shift, Ctrl, Alt, Gui (bits 0-3: left, 4-7: right)
  uint8_t reserved;  // Always 0x00
  uint8_t keys[6];   // Up to 6 simultaneous key codes
}
```

**Mouse Report (3-4 bytes)**
```c
struct {
  uint8_t buttons;  // Bit 0: Left, Bit 1: Right, Bit 2: Middle
  int8_t x;         // Relative X (-127 to +127)
  int8_t y;         // Relative Y (-127 to +127)
  int8_t wheel;     // Scroll wheel (optional)
}
```

### Consumer Control (Media Keys)

**Keychron Q1 Knob Report (3 bytes)**
```c
struct {
  uint8_t report_id;       // Always 0x04
  uint8_t consumer_code;   // 0xE2=Mute, 0xE9=Vol+, 0xEA=Vol-
  uint8_t reserved;        // Always 0x00
}
```

**Mapped to BLE 16-bit Values**
```
0xE2 (Mute) → 0x0010 (bit 4)
0xE9 (Vol+) → 0x0020 (bit 5)
0xEA (Vol-) → 0x0040 (bit 6)
```

### Performance Optimizations

- **Mouse Throttling:** Reports sent max every 5ms (200Hz refresh rate)
- **Movement Accumulation:** Smooth mouse motion from rapid USB updates
- **Non-blocking USB:** Callback-based design prevents blocking
- **Efficient BLE:** NimBLE stack vs. classic Bluetooth for 50% less RAM

## Troubleshooting

### Device Not Detected
**Symptom:** No `[USB] KEYBOARD connected!` message

**Solutions:**
1. Verify USB hub has power (LED indicator on)
2. Check USB cable connections
3. Try keyboard in wired mode (if supported)
4. Test with different USB hub
5. Verify serial shows `[USB] Installing HID driver...`

### Phantom Key Presses
**Symptom:** Left Ctrl always pressed, or random keys

**Solution:** Ensure BleDevice implementation is current (v1.0+)
- Not using deprecated BLEManager
- Report ID not included in payload (handled by NimBLE)

### Knob Not Detected
**Symptom:** Knob press/rotation does nothing, no `[GENERIC]` logs

**Solutions:**
1. Check for `[USB] Proto: NONE` in serial output
2. Verify consumer codes: should see `[CONSUMER] Code: 0xE9/0xEA/0xE2`
3. Confirm Keychron is in Bluetooth mode (some models have switch)
4. Try different USB port on hub

### BLE Pairing Fails
**Symptom:** Device seen but won't pair, or keeps disconnecting

**Solutions:**
1. Unpair and re-scan for device
2. Check BleDevice initialization: `advertising->setScanResponse(true)`
3. Verify no interference from nearby BLE devices
4. Ensure target device Bluetooth is fully enabled

### Serial Monitor Shows Garbage
**Symptom:** Random characters in terminal

**Solutions:**
1. Set baud rate to exactly **115200**
2. Reinstall CH340 driver (if using generic adapter)
3. Try different USB cable
4. Power cycle ESP32-S3

### USB Out of Channels
**Symptom:** `[USB] No more HCD channels available`

**Cause:** Complex USB device using multiple HID interfaces
- Some gaming keyboards report as keyboard + mouse + media + RGB = 4+ interfaces

**Solutions:**
1. Use simpler USB keyboard (Keychron Q1 = 3 interfaces, typical limit is 4-8)
2. Disconnect other USB devices
3. Use USB hub with device filtering

## Dependencies

**Platform:** PlatformIO, Arduino-ESP32 framework

**Libraries:**
- `ESP32_USB_Host_HID` (USB Host HID wrapper)
- `NimBLE-Arduino @ 1.4.1` (Lightweight BLE stack)
- `Arduino.h` (Standard Arduino core)

**Auto-installed:** All dependencies in `platformio.ini`

## Known Limitations

- **Single Device:** Only one BLE host connection at a time
- **6-Key Rollover:** Keyboard limited to 6 simultaneous keys (standard HID limitation)
- **Mouse Report Rate:** Max 200Hz (5ms throttle)
- **Boot Protocol:** Limited to standard HID; vendor-specific features not supported
- **Battery Reporting:** Fixed at 100% (no real battery level)
- **No LED Feedback:** Num/Caps/Scroll Lock LEDs not synchronized

## Future Enhancements

- [ ] Multi-device switching with keyboard shortcuts
- [ ] LED feedback for lock keys
- [ ] Persistent bonding from NVS
- [ ] Real battery level reporting
- [ ] Additional consumer codes (email, calculator, etc.)
- [ ] OTA firmware updates
- [ ] Web-based configuration portal
- [ ] Support for Keychron multimedia keys

## Development

### Code Style
- Clear component separation
- Extensive inline documentation
- Consistent logging with prefixes
- No external dependencies (standard Arduino APIs only)

### Adding New Features
1. **New USB Report Types:** Extend USBManager callbacks
2. **New BLE Reports:** Add methods to BleDevice
3. **Report Parsing:** Extend Bridge onGenericReport()

Example - adding WiFi connectivity:
```cpp
// In Bridge.cpp
void Bridge::sendReportToCloud(const uint8_t *data) {
  // Implement WiFi forwarding
}
```

### Debugging
Enable debug logging in platformio.ini:
```ini
build_flags = -DCORE_DEBUG_LEVEL=5  ; Maximum verbosity
```

Serial output shows exact USB/BLE/Bridge flow.

## References

- [ESP-IDF USB Host Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/usb_host.html)
- [NimBLE-Arduino GitHub](https://github.com/h2zero/NimBLE-Arduino)
- [USB HID Report Descriptors](https://www.usb.org/sites/default/files/documents/hid1_11.pdf)
- [Bluetooth HID Spec](https://www.bluetooth.com/wp-content/uploads/2020/01/Component_Specification_v1_2_0.pdf)

## License

[Specify license: MIT, Apache 2.0, GPL, etc.]

## Support & Contact

For issues or questions:
1. Check [Troubleshooting](#troubleshooting) section
2. Review serial debug output with `-DCORE_DEBUG_LEVEL=5`
3. Create GitHub issue with:
   - Hardware details (board model, USB hub model)
   - Serial output logs
   - Steps to reproduce
   - Expected vs. actual behavior

## Acknowledgments

- ESP32 USB Host HID library authors
- NimBLE-Arduino maintainers
- Keychron for excellent keyboard design
- Community feedback and testing

---

**Last Updated:** January 22, 2026  
**Version:** 1.0.0  
**Status:** ✅ Production Ready  
**Tested Devices:** ESP32-S3 DevKitC-1, Keychron Q1, Windows 11, macOS 13+, Android 12+
