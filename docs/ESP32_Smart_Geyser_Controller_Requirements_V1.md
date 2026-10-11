# ESP32 Smart Geyser Controller
**Disclaimer:** This document is a result of AI chat and generated after refining the requirements. The document was generated to keep track of initial objectives and document any modifications as development progresses. 
## Software & System Requirements Specification — V1.0

**Project status:** Hardware baseline closed; software development pending  
**Primary controller:** ESP32-WROOM  
**Development framework:** ESP-IDF  
**Development IDE:** VS Code with ESP-IDF extension  
**Initial load:** 5 V LED  
**Final load:** Domestic electric geyser switched through an appropriately rated contactor  
**Primary communication:** Wi-Fi  
**Mobile application:** Flutter/Dart (planned)  
**Control model:** The ESP32 operates autonomously; the phone is a configuration and control interface.

---

## 1. Project Objective

The system shall automate a domestic electric water geyser using an ESP32 controller.

The system shall allow the user to:

- Turn the geyser ON/OFF manually.
- Create ON/OFF schedules.
- View the current geyser state.
- Configure schedules from a mobile phone.
- Monitor status over the home Wi-Fi network.
- Automatically turn the geyser OFF at defined safety times.
- Continue scheduled operation when the mobile phone is unavailable.
- Handle primary power failures safely.
- Maintain controller time during temporary power failures using the backup-power architecture.
- Support future alerts for unexpected or prolonged geyser operation.

The ESP32 shall be the primary decision-making device. The mobile application shall not be required for normal scheduled operation.

## 2. High-Level Architecture

```text
Mobile App (Flutter/Dart)
          |
        Wi-Fi
          |
     Home Router
          |
        Wi-Fi
          |
         ESP32
   ┌──────┼─────────┐
   |      |         |
Scheduler Time    HTTP API
   |                |
   └────── GPIO ────┘
             |
       V1: 5 V LED
             |
    Future: Driver/Relay
             |
          Contactor
             |
           Geyser
```

For the first software-development phase, a 5 V LED represents the geyser output. Later, the output will drive a suitable relay/driver and high-power contactor.

**The ESP32 GPIO must never directly switch geyser mains voltage.**

## 3. Hardware Baseline

Hardware is considered provisionally complete for software development.

- ESP32-WROOM development board.
- Board has a VIN input and onboard AMS1117-based 3.3 V regulation.
- Existing power architecture is accepted as the development baseline.
- A 5 V LED will be used as the initial controlled load.
- Final mains deployment remains subject to verification of the complete electrical installation and source-selection/power-path behavior.

## 4. Backup Power

### Battery

Selected battery: **3.7 V nominal, 5000 mAh Li-ion/LiPo battery with built-in protection/BMS**.

- Nominal voltage: 3.7 V.
- Fully charged voltage: approximately 4.2 V.
- Capacity: 5000 mAh.
- Built-in battery protection.

### Charger/boost module

Selected module: **HW-357 / J5019**, combining TP4056-based charging and MT3608 boost conversion.

The backup battery is intended to keep the ESP32 control system alive during a primary power failure. It must not power the geyser itself.

The power arrangement must prevent the primary supply and boosted battery supply from driving each other. The exact power-path design should be verified before final deployment.

## 5. Power-Failure Philosophy

### Normal operation

- Primary supply is available.
- ESP32 operates normally.
- Wi-Fi is active when configured.
- Schedules are active.
- Geyser control is available.

### Primary power failure

- ESP32 remains powered from backup, if the backup system is functioning.
- Controlled geyser output is turned OFF.
- Relevant state is recorded where practical.
- Unnecessary peripherals and Wi-Fi may be disabled to conserve battery.
- The controller enters a low-power operating mode while maintaining time.
- The controller waits for primary power to return.

### Primary power restoration

- Resume normal power operation.
- Reconnect Wi-Fi.
- Synchronize time with NTP/SNTP when available.
- Safely resume scheduler operation.

The system must not turn the geyser ON merely because primary power has returned. The scheduler and safety rules must determine the resulting output state.

## 6. Primary-Power Detection

Because the ESP32 may remain powered by the battery when the primary supply disappears, the firmware needs a way to detect that loss independently.

The final hardware shall provide a safe `MAINS_PRESENT` or `PRIMARY_POWER_PRESENT` signal to an ESP32 GPIO. The GPIO must not be exposed directly to an unsafe voltage.

## 7. Time Management

- Primary time synchronization: NTP/SNTP over Wi-Fi.
- The ESP32 shall maintain system time between synchronizations while its timekeeping domain remains powered.
- The controller shall track whether time is valid.
- Scheduler configuration shall be stored in non-volatile memory.
- The current time should not be continuously written to flash.

## 8. RTC and Power-Outage Timekeeping

An external DS3231 RTC is not required for V1.

The current plan is to keep the ESP32 powered during a temporary outage so its internal timekeeping can continue. NTP/SNTP shall correct the clock when network access is available again.

If testing shows that time accuracy or retention is insufficient, an external RTC may be reconsidered.

## 9. Scheduler

Each schedule shall contain at least:

- Enabled/disabled state.
- ON time.
- OFF time.
- Applicable days of the week.

Example:

| Days | ON | OFF |
|---|---:|---:|
| Monday–Friday | 06:00 | 08:00 |
| Monday–Friday | 18:00 | 22:00 |

Schedules shall execute independently of the mobile application once stored on the ESP32.

The scheduler must continue operating when Wi-Fi or Internet access is unavailable, provided the ESP32 has valid time.

## 10. Hard Safety Cutoff

The ESP32 shall independently enforce a configurable safety cutoff.

Example requirement:

> If the current local time is at or after 10:00 and the geyser output is ON, turn it OFF.

This protection must be implemented inside the ESP32, not only in the mobile application.

The exact cutoff time and how it interacts with manual control and schedules shall be configurable and explicitly defined during implementation.

## 11. Manual Control

The system shall support manual ON/OFF control through the mobile application.

On receiving a request, the ESP32 shall:

1. Validate the request.
2. Apply the safety rules.
3. Change the output if permitted.
4. Update internal state.
5. Return the resulting status to the client.

Safety rules take precedence over manual commands.

## 12. State Model

The initial implementation may use `ON` and `OFF` output states plus independent flags:

- `mains_present`
- `wifi_connected`
- `time_valid`
- `schedule_enabled`
- `fault`

A richer state machine may later include `STARTING`, `POWER_FAILURE`, and `FAULT`.

The reported output state should reflect the controller's actual commanded output. Physical load feedback may be added later if required.

## 13. Output Control — V1

For V1:

```text
ESP32 GPIO → 5 V LED
```

- LED ON represents the commanded geyser ON state.
- LED OFF represents the commanded geyser OFF state.

The LED allows scheduler, API, Wi-Fi, and safety logic to be tested without switching mains voltage.

## 14. Final Geyser Switching

The intended final architecture is:

```text
ESP32 GPIO
    |
Relay/driver stage
    |
Contactor coil
    |
Contactor contacts
    |
AC geyser
```

The contactor remains **TBD**. Selection must consider:

- Geyser rated power and current.
- Mains voltage.
- Switching duty and appropriate rating margin.
- Contactor coil voltage and current.
- Driver-stage requirements.
- Enclosure, wiring, and circuit protection.
- Applicable local electrical requirements.

The design should ensure that loss of controller power or control signal causes the contactor to de-energize and the geyser to turn OFF. The geyser's original thermostat and thermal protection must remain functional. A qualified electrician should handle final mains wiring.

## 15. Communication Technology

**Wi-Fi is selected for V1.**

Reasons:

- Wi-Fi is already supported by the ESP32.
- It suits a fixed home appliance.
- It allows phone-to-device communication over the home network.
- It supports future remote monitoring and notification options.

BLE is not required for V1. It may be considered later for initial provisioning, device discovery, or local fallback.

## 16. Network Architecture

Initial architecture:

```text
Phone ↔ Home Router ↔ ESP32
```

Basic local control should not require Internet access. If the Internet is down but the home router and local Wi-Fi network are functioning, the phone should still be able to communicate with the ESP32.

## 17. ESP32 HTTP API

The initial communication protocol shall be **HTTP/REST**.

Proposed endpoints:

- `GET /api/status`
- `POST /api/control`
- `GET /api/schedules`
- `POST /api/schedules`
- `PUT /api/schedules/{id}`
- `DELETE /api/schedules/{id}`
- `GET /api/time`
- `GET /api/config`

These are proposed endpoint names; request/response schemas, authentication, and error codes will be defined during implementation.

Example status response:

```json
{
  "device": "geyser-controller",
  "output": "OFF",
  "mains": true,
  "wifi": true,
  "time_valid": true,
  "schedule_enabled": true
}
```

## 18. Mobile Application

Planned technology: **Flutter + Dart**.

Initial screens and features:

- Dashboard showing output state.
- Mains/power status.
- Wi-Fi connection status.
- Time validity/current time.
- Manual ON/OFF control.
- Schedule creation and management.

Future features may include push notifications, power-failure alerts, geyser-left-on alerts, vacation mode, usage history, temperature sensing, and remote access.

## 19. Persistent Storage

The ESP32 shall use NVS for persistent configuration.

Potential stored values:

- Wi-Fi configuration.
- Device configuration.
- Schedules.
- Safety cutoff settings.
- User preferences.
- Other configuration needed after restart.

Avoid unnecessary frequent writes to flash, especially for rapidly changing values such as current time.

## 20. Device Boot Behavior

Proposed boot sequence:

1. Initialize hardware.
2. Initialize NVS.
3. Initialize GPIO.
4. Set controlled output to OFF.
5. Determine primary-power status.
6. Load configuration.
7. Initialize timekeeping and assess time validity.
8. Start Wi-Fi if appropriate.
9. Synchronize time when possible.
10. Start scheduler.
11. Start HTTP/API server.

The output shall default to OFF until the controller has established that changing it is safe.

## 21. Power-Failure and Resume Behavior

### Failure

1. Detect primary-power loss.
2. Turn the controlled output OFF.
3. Record power-failure state where practical.
4. Disable unnecessary peripherals.
5. Enter low-power operation.
6. Wait for primary power to return.

### Restoration

1. Detect primary-power restoration.
2. Return to normal power operation.
3. Reconnect Wi-Fi.
4. Synchronize time when possible.
5. Evaluate the schedule and safety rules.
6. Apply the resulting output state without unexpected activation.

Post-restoration behavior must be explicitly tested, including cases where the outage spans a scheduled ON or OFF time.

## 22. Safety Requirements

- **SR-01 — Default OFF:** Reset or loss of control should lead to a safe OFF output.
- **SR-02 — Power failure:** Turn the controlled output OFF when primary-power failure is detected.
- **SR-03 — Hard cutoff:** Enforce the safety cutoff in ESP32 firmware.
- **SR-04 — No direct mains switching:** ESP32 GPIO must never directly switch geyser mains voltage.
- **SR-05 — Isolation:** Control and mains switching circuits must have appropriate isolation.
- **SR-06 — Independent protection:** Preserve the geyser's original thermostat and thermal safety mechanisms; ESP32 is not the only safety layer.
- **SR-07 — Wi-Fi loss:** Wi-Fi loss must not stop the local scheduler or leave the geyser indefinitely ON.
- **SR-08 — Phone failure:** The phone being unavailable must not stop autonomous operation.
- **SR-09 — Internet failure:** Internet failure must not stop basic local operation.
- **SR-10 — Firmware failure:** Hardware should be designed so a firmware failure results in a safe output state. A watchdog should be considered.
- **SR-11 — Time validity:** If time is invalid, scheduled ON actions should not be executed blindly; the system should use a defined safe fallback, normally OFF, until time is valid.
- **SR-12 — Conflicting commands:** Safety rules and hard cutoff must take precedence over manual commands and schedules.

## 23. Security Requirements

- Do not expose the ESP32 directly to the public Internet in the initial architecture.
- Do not configure router port forwarding to the ESP32.
- Authenticate control requests.
- Use secure Wi-Fi.
- Protect Wi-Fi credentials and other secrets.
- Validate incoming commands and configuration.
- Keep local control working without a cloud dependency.

Later hardening may include HTTPS/TLS, encrypted NVS, flash encryption, Secure Boot, cloud authentication, and certificate management. These are not prerequisites for the first LED prototype but should be considered before real-world deployment.

## 24. Alerts and Notifications

The eventual system should be able to notify the user when the geyser has remained ON too long or when a safety cutoff turns it OFF.

Example:

> Geyser has been ON since 08:15. The 10:00 safety cutoff has turned it OFF.

Push notifications require a future notification path, likely through an Internet-connected service. They are not required for the initial local prototype.

## 25. Core Design Principle

> **The phone controls the ESP32; the ESP32 controls the geyser.**

The phone is a user interface and configuration tool. The ESP32 is responsible for autonomous scheduling and local safety behavior.

## 26. V1 Functional Requirements

| ID | Requirement | Priority |
|---|---|---|
| FR-01 | ESP32 boots reliably | Must |
| FR-02 | GPIO controls 5 V LED | Must |
| FR-03 | LED represents commanded geyser state | Must |
| FR-04 | ESP32 connects to Wi-Fi | Must |
| FR-05 | ESP32 provides local HTTP API | Must |
| FR-06 | Phone can query device status | Must |
| FR-07 | Phone can turn output ON/OFF | Must |
| FR-08 | Scheduler supports ON/OFF times | Must |
| FR-09 | Schedule is stored in NVS | Must |
| FR-10 | Schedule survives ESP32 restart | Must |
| FR-11 | Time synchronizes using NTP/SNTP | Must |
| FR-12 | Safety cutoff exists | Must |
| FR-13 | Output defaults OFF on boot | Must |
| FR-14 | Primary-power state can be detected | Must |
| FR-15 | Power-failure handling exists | Must |
| FR-16 | Backup operation keeps controller alive | Must |
| FR-17 | Wi-Fi loss does not stop scheduler | Must |
| FR-18 | Mobile Flutter application | Phase 2 |
| FR-19 | Push notifications | Phase 3 |
| FR-20 | Real geyser/contactor integration | Final hardware phase |

## 27. Development Phases

### Phase 1 — ESP32 foundation
- Create ESP-IDF project structure.
- Implement GPIO output control.
- Control the LED.
- Add basic state handling and logging.
- Consider watchdog behavior.

### Phase 2 — Time
- Implement system time.
- Configure NTP/SNTP.
- Set local timezone.
- Track time validity and handle clock synchronization.

### Phase 3 — Scheduler
- Define schedule data structures.
- Store schedules in NVS.
- Implement scheduler task.
- Implement ON/OFF execution and hard cutoff.
- Test schedule behavior after reboot and time changes.

### Phase 4 — Wi-Fi
- Implement station mode.
- Handle connection status and reconnection.
- Ensure scheduler continues if Wi-Fi is lost.

### Phase 5 — HTTP API
- Implement status and control endpoints.
- Implement schedule CRUD endpoints.
- Add authentication, validation, and error responses.

### Phase 6 — Flutter application
- Implement dashboard.
- Add manual control.
- Add schedule management.
- Display device status and connection/time validity.

### Phase 7 — Power-failure handling
- Integrate primary-power detection.
- Test backup operation.
- Implement low-power behavior.
- Test recovery and NTP resynchronization.

### Phase 8 — Testing
Test at least:

- ESP32 restart and watchdog reset.
- Wi-Fi loss and router restart.
- Internet failure.
- Primary-power failure and restoration.
- Overlapping or invalid schedules.
- Missed schedule boundaries during an outage.
- Time synchronization and timezone behavior.
- Phone unavailable.
- Repeated ON/OFF commands.
- Invalid-time behavior.
- Safety cutoff behavior.

### Phase 9 — Final electrical hardware
Only after the software passes LED simulation tests, integrate the driver, contactor, and geyser. Have the final mains installation checked by a qualified electrician.

## 28. Out of Scope for V1

- Cloud platform.
- MQTT.
- Alexa/Google Home.
- BLE control.
- Temperature sensing.
- Water-level sensing.
- Energy metering.
- Multiple geysers.
- OTA firmware updates.
- Remote Internet access.
- Advanced analytics.
- AI-based scheduling.

These features may be added later without changing the core architecture.

## 29. Key Engineering Decisions — V1

| Area | Decision |
|---|---|
| MCU | ESP32-WROOM |
| Framework | ESP-IDF |
| IDE | VS Code + ESP-IDF |
| Communication | Wi-Fi |
| Local protocol | HTTP/REST |
| Mobile | Flutter/Dart |
| Time | ESP32 system time + NTP/SNTP |
| External RTC | Not required initially |
| Persistent storage | NVS |
| Backup battery | 3.7 V, 5000 mAh Li-ion |
| Charger/boost module | HW-357 / J5019 |
| Power architecture | Existing hardware arrangement accepted for development; power-path behavior must be verified before deployment |
| Power-failure strategy | Geyser output OFF; ESP32 backup operation |
| Primary-power detection | Required |
| Initial output | 5 V LED |
| Final output | High-power contactor |
| Geyser safety | Independent thermostat/thermal protection retained |
| Default output state | OFF |
| Internet dependency | None for basic local operation |
| Phone dependency | None for autonomous operation |
| Hard cutoff | Required |
| Cloud | Future |
| Notifications | Future |
| BLE | Not required for V1 |
| Contactor selection | TBD |

## 30. Definition of Success for V1

The prototype is successful when:

1. At a scheduled ON time, the ESP32 turns the LED ON.
2. At a scheduled OFF time, the ESP32 turns the LED OFF.
3. The phone can manually control the LED through the local Wi-Fi/API path.
4. After an ESP32 reboot, the output starts OFF, configuration is restored, time is synchronized when possible, and the scheduler resumes safely.
5. Wi-Fi/router or Internet failure does not stop local scheduling.
6. During a simulated primary-power failure, backup power keeps the controller alive while the controlled output is forced OFF.
7. After power restoration, the controller reconnects, synchronizes time when possible, and resumes without unintended activation.
8. The hard cutoff overrides manual control and normal schedules.
9. Invalid time does not trigger an unsafe scheduled ON action.

---

## Document Control

- **Document:** ESP32 Smart Geyser Controller — Software & System Requirements Specification
- **Version:** V1.0
- **Status:** Baseline for software-first prototype
- **Next step:** Begin Phase 1 with ESP-IDF project structure and a GPIO/LED output module.

Changes to the frozen decisions should be recorded in a new revision before implementation proceeds.
