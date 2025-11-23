# ⚡ Low-Latency Pub/Sub Broker Engine

This project is a demonstration of a simple Pub/Sub (Publisher/Subscriber) system designed for high throughput and low latency, implemented in C++20 using asynchronous Boost.Asio.

The system consists of three main components: the **Broker**, the **Publisher**, and the **Subscriber**. Communication relies on an optimized binary protocol.

## 🚀 System Architecture

The project is based on an architecture where the Broker acts as the central hub for routing binary messages based on subscription to specific topics (Topic ID).

| Component | Role | Key Technologies |
| :--- | :--- | :--- |
| **`Broker`** | Server (TCP Acceptor). Manages all client sessions and routes messages. | `boost::asio::io_context`, Multithreading, `ClientSession`, `SubscriptionManager`, `steady_timer` for cleanup. |
| **`Publisher`** | Client that sends a continuous stream of binary `TradeMessage` packets to the Broker. | **Asynchronous TCP connection**, C++ `std::random` for data generation. |
| **`Subscriber`** | Client that subscribes to specific topics. Receives and decodes the binary data stream asynchronously. | Asynchronous TCP connection, `boost::asio::async_read`. |

---

## 📦 Binary Protocol Specification

The system uses a minimalist binary protocol (Big-Endian) to minimize overhead. Every message starts with a **1-byte header** defining the message type.

### 1. Header (1 Byte)

| Message Type (`MsgType`) | Value | Description |
| :--- | :--- | :--- |
| `SUBSCRIBE` | `0x01` | Request to subscribe to a topic. |
| `DATA` | `0x02` | Actual binary payload of trade data. |

### 2. Payload (`TradeMessage`)

By using `#pragma pack(push, 1)`, the `TradeMessage` structure has a fixed size of **28 bytes**, eliminating memory padding.

| Field | Type | Size | Running Total | Description |
| :--- | :--- | :--- | :--- | :--- |
| `topic_id` | `int32_t` | 4 B | **4 B** | The topic the message relates to. |
| `timestamp_ms` | `uint64_t` | 8 B | **12 B** | Timestamp in milliseconds. |
| `price` | `double` | 8 B | **20 B** | Trade price. |
| `quantity` | `double` | 8 B | **28 B** | Trade quantity. |

---

## 🛠️ Project Build Instructions

The project uses **CMake** for build management and **Vcpkg** for handling Boost dependencies on Windows.

### 1. Prerequisites

- C++20 compatible compiler (MSVC 2022 recommended).
- CMake (version 3.16 or newer).
- **Boost Library** (required components: `system` and `asio`).
- **Vcpkg** (highly recommended for automatic download and configuration of Boost on Windows).

### 2. Configuration (Windows/Vcpkg)

```bash
mkdir build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

### 3. Compilation

```bash
cmake --build . --config Release
```

Executables will appear in:

```
build/Release/
```

---

## 🧪 Testing Instructions

Open **three terminals**, as each component is a separate process.

### 1. Start the Broker

```bash
.\broker.exe
```

### 2. Start a Subscriber

Example: subscribe to Topic 1

```bash
.\subscriber.exe 1
```

### 3. Start the Publisher

```bash
.\publisher.exe
```

### 4. Verification

- Subscriber terminal prints only messages with `topic=1`.
- Broker logs new connections, subscriptions, and cleanup cycles.

---

## 📸 Project Screenshots

Quickly check the files in the `/Screenshots` folder to see the engine in action!

The provided images confirm the full system operation, including:

* **Three Active Terminals:** Simultaneous operation of the Broker, Publisher, and Subscriber clients.
* **Asynchronous Communication:** The Broker logging successful routing of data (`routing to 1 subscribers.`).
* **Real-Time Data Flow:** The Subscriber terminal displaying continuously received and decoded `price` and `qty` messages, verifying the low-latency binary communication.

---
## 🧑‍💻 Author

Luka Mikulić