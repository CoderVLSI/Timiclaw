# WROOM Brain Scaffold

This is a first-pass ESP-IDF scaffold for an ESP32-WROOM-centric "agent brain" design.

Design choices:
- OpenClaw influence: command-driven tool execution model.
- ZeroClaw influence: modular transport/tool/scheduler split.
- Mimiclaw influence: ESP-IDF firmware-first architecture.

Current state:
- Boots and runs an agent loop on device.
- Executes only strict local allowlisted tools.
- Telegram transport is a stub (next step).
- Scheduler runs autonomous `status` every 30 seconds.

## Build

```bash
idf.py set-target esp32
idf.py build
idf.py -p <PORT> flash monitor
```

## Next Steps

1. Implement Wi-Fi init and reconnect handling.
2. Implement Telegram `getUpdates` polling and `sendMessage`.
3. Add persistent queue and crash-safe command replay guard.
4. Add HMAC command signatures and nonce window.
5. Add hardware tool drivers (relay, sensor, pwm).
