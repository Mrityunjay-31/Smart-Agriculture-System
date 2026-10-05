# Architecture Overview

## Deployment Architecture
- **Backend**: C++17 daemon via Crow HTTP Server
- **Data persistence**: Firebase RTDB
- **Serial Comms**: POSIX API directly reading from `/dev/ttyACM0`
- **Frontend**: Vanilla HTML/JS UI built inside `web/templates`

## Sub-Systems
1. **Serial I/O Layer**: Connects to the Arduino, reads line-by-line raw sensor text strings. Handled completely by low-level `select()`/`read()` blocking operations in POSIX.
2. **Parser & Validator Layer**: Regex transforms textual inputs to parsed properties in the `SensorData` struct. NaNs or boundary errors are rejected.
3. **Processor & Alert Layer**: Threshold engine assigns OPTIMAL, WARNING, CRITICAL labels. Triggers new entries to AlertManager which pushes events to Firebase.
4. **API Layer**: Exposes Crow REST methods (e.g., `/api/sensors/latest`, `/api/auth/session`) for the browser-client to consume.
5. **Dashboard Layer**: Dark mode inspired UI powered by native `fetch()` calls polling endpoints on intervals, removing dependency on Node/React.
