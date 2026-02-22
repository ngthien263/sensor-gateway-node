# Overview

A multi-threaded TCP gateway written in C that collects real-time temperature and humidity data from simulated sensor nodes, analyzes the data, and persists it to a SQLite database — all while logging every event to a file via a named FIFO pipe.

---

## Architecture

The project is split into two standalone executables:

```
┌─────────────────────────┐         TCP          ┌──────────────────────────┐
│      sensor_node        │ ───────────────────► │     sensor_gateway       │
│                         │                      │                          │
│  Parent: Menu UI        │                      │  Thread 1: Connection    │
│  Child:  Data sender    │                      │  Thread 2: Data Manager  │
│                         │                      │  Thread 3: Database      │
│  IPC: shared mem + sem  │                      │  Child process: Logger   │
└─────────────────────────┘                      └──────────────────────────┘
                                                          │
                                                  ┌───────┴────────┐
                                              sensors.db       gateway.log
```

### `sensor_gateway` — The Server

Spawns a **child process** dedicated to writing logs from a named FIFO (`log_fifo`) into `gateway.log`, then launches three POSIX threads:

| Thread | Role |
|---|---|
| `connect_thread` | Accepts incoming TCP connections and reads sensor packets |
| `data_manager_thread` | Computes running averages, detects temperature anomalies |
| `database_thread` | Persists sensor state to a SQLite3 database |

Threads communicate via a **mutex + condition variable** to safely share the sensor array.

### `sensor_node` — The Simulated Client

Uses `fork()` to create two processes:

- **Parent**: Presents a menu to add/remove virtual sensors by ID.
- **Child**: Reads commands from the parent via **POSIX shared memory + unnamed semaphore**, then periodically sends packed sensor data over TCP.

If the connection drops, the child automatically reconnects.

---

## Project Structure

```
.
├── sensor_gateway.c        # Gateway entry point (server)
├── sensor_node.c           # Sensor node simulator (client)
├── include/
│   ├── common.h            # Shared macros (handle_error)
│   ├── sensor_types.h      # Structs: sensor_info_t, sensor_packet_t, etc.
│   ├── sensor_reader.h     # Gateway-side packet reader
│   ├── sensor_sender.h     # Node-side packet sender + linked list
│   ├── shared_data.h       # Shared memory struct for IPC
│   ├── socket.h            # TCP socket abstraction
│   ├── thread_handle.h     # Thread handler declarations
│   └── database.h          # SQLite3 database API
├── source/
│   ├── sensor_reader.c
│   ├── sensor_sender.c
│   ├── socket.c
│   ├── thread_handle.c
│   └── database.c
├── Makefile
└── README.md
```

---

## Build

### Prerequisites

- GCC
- POSIX-compliant OS (Linux recommended)
- **SQLite3** development library

```bash
# Ubuntu / Debian
sudo apt install libsqlite3-dev
```

### Compile

```bash
make
```

This produces two binaries inside the `bin/` directory:

```
bin/sensor_gateway
bin/sensor_node
```

### Clean

```bash
make clean
```

---

## Usage

### 1. Start the Gateway (Server)

```bash
./bin/sensor_gateway <port>
```

**Example:**
```bash
./bin/sensor_gateway 5000
```

The gateway will:
- Listen for incoming sensor connections on the given port.
- Log all events to `gateway.log`.
- Store sensor readings in `sensors.db`.

### 2. Start a Sensor Node (Client)

```bash
./bin/sensor_node <server_ip> <port>
```

**Example:**
```bash
./bin/sensor_node 127.0.0.1 5000
```

You will be presented with an interactive menu:

```
  ╔═════════════════════════════════════╗
  ║           SENSOR MAIN MENU          ║
  ╠═════════════════════════════════════╣
  ║ 1. Add new sensor                   ║
  ║ 2. Remove sensors                   ║
  ║ 3. Exit                             ║
  ╚═════════════════════════════════════╝
```

Each virtual sensor you add will start transmitting randomized temperature and humidity readings every second.

---

## Database Schema

Sensor state is stored in `sensors.db` using the following table:

```sql
CREATE TABLE IF NOT EXISTS sensors_state (
    id         INTEGER PRIMARY KEY,
    first_seen TEXT    NOT NULL,
    cur_temp   REAL,
    cur_humid  REAL,
    avg_temp   REAL
);
```

Readings are **upserted** on every update — inserting new sensors on first contact and updating existing ones on subsequent reads.

---

## Logging

All runtime events are written to `gateway.log` via a named FIFO pipe. Example entries:

```
1. 10:45:02 A sensor node with ID 3 has opened a new connection
[Data] Sensor 3: Too hot (avg=52.30)
[Data] Sensor 7: Normal temperature (avg = 34.10)
[Data] Sensor 2: Too cold (avg=17.80)
```

Temperature thresholds:

| Status | Condition |
|---|---|
| ❄️ Too cold | Average temperature < **20°C** |
| ✅ Normal | 20°C ≤ Average temperature ≤ 50°C |
| 🔥 Too hot | Average temperature > **50°C** |

---

## Configuration

Key constants are defined in `include/sensor_types.h`:

| Constant | Default | Description |
|---|---|---|
| `MAX_SENSORS` | `10` | Maximum number of concurrent sensors |
| `TEMP_UPPER_LIMIT` | `50` | High temperature alert threshold (°C) |
| `TEMP_LOWER_LIMIT` | `20` | Low temperature alert threshold (°C) |
| `BUFF_SIZE` | `256` | General buffer size |

---

## Key Concepts & IPC Mechanisms

| Mechanism | Used For |
|---|---|
| **POSIX Threads (`pthreads`)** | Concurrent connection, data analysis, and DB write threads in gateway |
| **Mutex + Condition Variable** | Safe sharing of the sensor array across threads |
| **`fork()` + Named FIFO** | Dedicated log-writer child process in gateway |
| **`fork()` + Shared Memory (`mmap`)** | IPC between parent menu and child sender in sensor node |
| **Unnamed Semaphore** | Signaling commands from parent to child in sensor node |
| **TCP Sockets** | Communication between gateway and sensor nodes |
| **SQLite3** | Persistent sensor state storage |

---

## License
