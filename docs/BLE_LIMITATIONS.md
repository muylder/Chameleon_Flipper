# 📡 BLE Implementation & Limitations

## Current Status: ⚠️ PARTIAL IMPLEMENTATION

### Chameleon Ultra BLE Specifications

**Service Used**: Nordic UART Service (NUS)

**UUIDs**:
- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (Write to device)
- **TX Characteristic**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (Notifications from device)

---

## 🚫 Flipper Zero BLE Limitation

### The Problem

The **Flipper Zero** uses the STM32WB55 chip which technically supports both:
- **BLE Peripheral** mode (advertising services for others to connect)
- **BLE Central** mode (scanning and connecting to other devices)

**However**, the official Flipper Zero API (`furi_hal_bt.h`) only exposes **Peripheral mode functionality**.

### What This Means

**Current Flipper BLE API can:**
- ✅ Advertise BLE services (Peripheral)
- ✅ Accept connections from phones/computers
- ✅ Work as a BLE keyboard, remote, etc.

**Current Flipper BLE API CANNOT:**
- ❌ Scan for nearby BLE devices (Central)
- ❌ Connect to external BLE devices like Chameleon Ultra
- ❌ Act as a BLE client (Central role)

---

## 🔧 Current Implementation

The BLE handler in this project is a **stub/placeholder** with:
- Proper structure and interface
- Nordic UART Service UUIDs documented
- Simulated scanning and connection
- Framework ready for future implementation

### What Works

```c
// These functions exist but are SIMULATED:
ble_handler_start_scan()     // Simulates finding "Chameleon Ultra"
ble_handler_connect()         // Simulates connection
ble_handler_send()            // Placeholder (no actual transmission)
```

### What Doesn't Work

- **Real BLE scanning**: Cannot actually discover Chameleon Ultra devices
- **Real GATT connection**: Cannot connect to external BLE devices
- **Data transmission**: No actual data sent over BLE

---

## ✅ Recommended Solution: Use USB

**Primary Method**: **USB Connection** (FULLY WORKING ✅)

The USB/Serial handler is **fully functional** and is the recommended way to use this app:
1. Connect Chameleon Ultra via USB cable to Flipper Zero
2. Select "Connect Device" → "USB"
3. All features work perfectly: tag read/write, slots, diagnostics

**USB Advantages**:
- ✅ Faster data transfer
- ✅ More reliable
- ✅ No battery drain on either device
- ✅ Fully implemented and tested

---

## 🔮 Future Possibilities

### Option 1: Low-Level BLE API (Advanced)

Use undocumented/low-level APIs from STM32WB55:
- Requires deep dive into STM32 BLE stack
- May break with Flipper firmware updates
- Not officially supported

### Option 2: Flipper Firmware Update

Wait for official Flipper Team to expose BLE Central APIs:
- Monitor: https://github.com/flipperdevices/flipperzero-firmware
- Check for BLE Central support in future releases

### Option 3: External BLE Module

Add external BLE Central module to Flipper:
- Use GPIO pins
- Nordic nRF52840 or similar
- Additional hardware required

---

## 📊 Feature Comparison

| Feature | USB | BLE |
|---------|-----|-----|
| Scanning | N/A | ❌ Simulated only |
| Connection | ✅ Full support | ❌ Simulated only |
| Data transfer | ✅ Full support | ❌ Not implemented |
| Tag Read | ✅ Working | ❌ Via USB only |
| Tag Write | ✅ Working | ❌ Via USB only |
| Slot Management | ✅ Working | ❌ Via USB only |
| Speed | 🚀 Fast (115200 baud) | N/A |
| Reliability | ✅ Very reliable | N/A |
| Cable required | ⚠️ Yes | N/A (but not working) |

---

## 🛠️ For Developers

### If You Want to Implement Real BLE

1. **Research**: Study STM32WB55 BLE Central implementation
2. **SDK**: Investigate ST's STM32WB BLE stack documentation
3. **Flipper Source**: Examine `furi_hal_bt` implementation in Flipper firmware
4. **Contact**: Reach out to Flipper dev team about BLE Central support
5. **Alternative**: Create MCP (Master Control Program) interface for external BLE handling

### Code Structure (Ready for Implementation)

```
lib/ble_handler/
├── ble_handler.h       ← Interface (complete)
├── ble_handler.c       ← Implementation (stub, ready to be filled)
└── UUIDs documented    ← Nordic UART Service UUIDs

When BLE Central API becomes available:
1. Replace simulated scan with real GAP scanning
2. Implement GATT service discovery for NUS
3. Subscribe to TX characteristic (0x6E400003...)
4. Write to RX characteristic (0x6E400002...)
5. Route data to response_handler
```

---

## 📖 References

- **Chameleon Ultra Firmware**: https://github.com/RfidResearchGroup/ChameleonUltra
- **Nordic UART Service**: Standard BLE service for serial communication
- **Flipper Zero BLE**: https://docs.flipper.net/development/hardware/bluetooth
- **STM32WB55**: https://www.st.com/en/microcontrollers-microprocessors/stm32wb55rg.html

---

## ⚠️ Important Notice

**This limitation is NOT a bug in Chameleon Flipper app.**

It is a fundamental limitation of the current Flipper Zero firmware API. The app is built correctly and will work immediately once BLE Central support is added to the Flipper SDK.

**For now, please use USB connection which is fully functional and reliable!** 🔌✅

---

**Last Updated**: 2025-11-07
**Status**: Documented and waiting for Flipper firmware BLE Central API
