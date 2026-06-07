# Chesscord

A multiplayer Chess and real-time Chat platform built in C++  powered by a Qt6 TCP server and a plain terminal CMD client. Every data structure is implemented from scratch ;) no STD containers used.

---

## Features

- **Multiplayer Chess:** Play chess in real time against another player over TCP server.
- **Real time Chat:** Send and receive messages during a live game session.
- **Terminal UI:** Plain CMD terminal interface just run the exe and play.
- **Auth System:** Token based authentication with a 10 second timeout for unverified connections.
- **Reconnect Support:** Returning users (same IP address) are recognized and welcomed back automatically.
- **Matchmaking Queue:** Players are paired automatically once two are waiting.
- **Friend Graph:** Adjacency matrix tracks opponent relationships after every completed game.
- **Move Validation:** Full chess move validation runs on both client and server independently.
- **Pawn Promotion:** Prompted automatically when a pawn reaches the last rank.
- **Disconnect Handling:** Surviving player is declared winner and re-queued if opponent disconnects mid game.
- **GCP Deployed:** Server runs live on Google Cloud Platform (e2-micro, Ubuntu 22.04, port 8080).
- **No STL Containers:** Every data structure is hand built from scratch in C++.

---

## Data Structures, Why Each One Was Chosen

| Structure | File | Used For | Why |
|---|---|---|---|
| **Doubly Linked List** | `LinkedList.h` | Pending (unauthenticated) connections | Fast insert/remove from both ends and middle; nodes store socket + IP + token buffer for clients not yet verified |
| **HashMap (Chaining)** | `HashMap.h` | Active authenticated users | O(1) average lookup by `userId`; multiplicative hash over a 100-bucket table |
| **Linked Queue** | `Queue.h` | Matchmaking queue | FIFO ordering ensures fair pairing; supports `remove(userId)` for mid queue disconnects |
| **AVL Tree** | `AVLTree.h` | Persistent user storage | Self balancing BST keeps all registered users sorted by `userId`; supports `findByIp()` for reconnect detection |
| **Stack** | `Stack.h` | Move history (generic template) | LIFO structure for tracking game moves per session |
| **Adjacency Matrix Graph** | `FriendGraph.h` | Friend/opponent relationship tracking | 1000×1000 matrix records which users have played each other; updated after every completed game |
| **4-Directional Linked Grid** | `ChessBoard.h` | Chess board representation | Each cell links to its up/down/left/right neighbor, making path clear validation a natural pointer traversal |

---

## Protocol

All messages follow a `TAG|data\n` format defined in `Protocol.h`.

| Tag | Direction | Description |
|---|---|---|
| `AUTH\|SEND_TOKEN` | Server → Client | Server requests auth token on connect |
| `AUTH\|CHESS123456PLEASEAUTH` | Client → Server | Client sends the auth token |
| `AUTH\|OK\|<id>` | Server → Client | Auth accepted, user ID assigned |
| `AUTH\|RECONNECT` | Server → Client | Returning user recognized by IP |
| `AUTH\|DUPLICATE` | Server → Client | User already connected, rejected |
| `WAIT\|<message>` | Server → Client | Status messages (looking for opponent, welcome, etc.) |
| `START\|<oppId>\|<color>\|<board>` | Server → Client | Match found, board state and color assigned |
| `GAME\|<move>` | Client → Server | Player submits a move (e.g. `e2e4`) |
| `GAME\|<move>\|<board>` | Server → Client | Move verified, updated board state sent |
| `GAME\|INVALID` | Server → Client | Move rejected by server validation |
| `GAME\|CHECK` | Server → Client | King is in check warning |
| `GAME\|PROMOTE\|<move>` | Server → Client | Pawn promotion required |
| `CHAT\|<message>` | Client → Server | Send a chat message |
| `CHAT\|<message>` | Server → Client | Receive opponent's chat message |
| `WIN\|` | Server → Client | You won |
| `LOSS\|` | Server → Client | You lost |
| `TIE\|` | Server → Client | Draw |
| `END\|` | Server → Client | Game session closed |

---

## Architecture

```
chesscord/
├── server/
│   ├── server.cpp               # Main server: auth, matchmaking, game lifecycle
│   ├── CMakeLists.txt
│   └── utils/
│       ├── Protocol.h           # TAG|data protocol
│       ├── Logger.h             # Server event logger
│       ├── UserIdManager.h      # Auto-incrementing user ID assignment
│       ├── LinkedList.h         # Pending unauthenticated users
│       ├── HashMap.h            # Active authenticated users
│       ├── Queue.h              # Matchmaking queue
│       ├── AVLTree.h            # Persistent user storage
│       ├── Stack.h              # Move history
│       ├── FriendGraph.h        # Opponent/friend adjacency matrix
│       ├── BoardNode.h          # Single chess cell node
│       ├── ChessBoard.h         # 4 directional linked grid chess board
│       ├── ChessValidator.h     # Server side move validation
│       ├── ChatSession.h        # In game chat handling
│       ├── GameSession.h        # Active game state between two players
│       ├── GameRoom.h           # Room wrapper: routes messages, tracks result
│       └── GameRoomMap.h        # Map of all active game rooms
│
└── client/
    ├── client.cpp               # Terminal client: auth, input loop, board render
    ├── CMakeLists.txt
    └── utils/
        ├── Protocol.h           # Shared protocol tags
        ├── Chess.h              # Client side move validation & game state
        ├── ChessBoard.h         # Client board representation
        └── BoardNode.h          # Single chess cell node
```

---

## Requirements

- **Qt6** (Core + Network modules)
- **CMake** 3.16 or higher
- **C++17** or higher
- **VS Code** with the CMake Tools extension

---

## How to Build & Run

### Step 1: Clone the Repository

```bash
git clone https://github.com/Huzaifa-Jamil/ChessCord.git
cd ChessCord
```

---

### Step 2: Build the Server

Open the `server/` folder as its own VS Code workspace:

```bash
code server
```

In VS Code:

1. Press `Ctrl + Shift + P` → **CMake: Configure**
2. Select your compiler (e.g. GCC or MSVC with Qt6)
3. Press `Ctrl + Shift + P` → **CMake: Build**

The compiled binary will be placed in:

```
server/build/server.exe       # Windows
server/build/server           # Linux / macOS
```

To run the server:

**Windows (CMD):**
```cmd
server\build\server.exe
```

**Linux**
```bash
./server/build/server
```

You should see:
```
[INFO] Server is waiting for clients all over the world on port 8080
```

---

### Step 3: Build the Client

Open the `client/` folder as its own VS Code workspace *(separate window from the server)*:

```bash
code client
```

In VS Code:

1. Press `Ctrl + Shift + P` → **CMake: Configure**
2. Select your compiler
3. Press `Ctrl + Shift + P` → **CMake: Build**

The compiled binary will be placed in:

```
client/build/client.exe       # Windows
client/build/client           # Linux / macOS
```

To run the client:

**Windows (CMD):**
```cmd
client\build\client.exe
```

**Linux:**
```bash
./client/build/client
```

> **Note:** The client connects to the live GCP server at `104.198.200.12:8080` by default.  
> To connect to a local server instead, change the IP in `client/client.cpp`:
> ```cpp
> pSocket->connectToHost("127.0.0.1", 8080);
> ```
> Then rebuild the client.

---

## In-Game Commands

Once connected and matched, type in the terminal:

| Input | Action |
|---|---|
| `e2e4/` | Submit a chess move (always end with `/`) |
| `board` | Reprint the current board |
| `resign` | Resign the current game |
| `q` or `r` or `b` or `n` | Choose pawn promotion piece when prompted |
| Any other text | Sends as a chat message to your opponent |
| `quit` | Disconnect and exit |

---

## Deploying the Server on GCP (Ubuntu 22.04)

### Step 1: SSH into your instance and install dependencies

```bash
sudo apt update
sudo apt install -y cmake g++ qt6-base-dev
```

### Step 2: Clone and build

```bash
git clone https://github.com/Huzaifa-Jamil/ChessCord.git
cd ChessCord/server
mkdir build && cd build
cmake ..
make
```

### Step 3: Run in background

```bash
nohup ./server > server.log 2>&1 &
```

### Step 4: Open firewall port

In GCP Console → VPC Network → Firewall Rules → allow **TCP port 8080** for all sources (`0.0.0.0/0`).

---

## Notes

- The server uses **no STL containers** — all data structures are written from scratch.
- Move validation runs on **both sides**: the client validates before sending, the server re validates on receive. A mismatch results in a `GAME|INVALID` response.
- On opponent disconnect mid game, the server sends `WIN|` and `END|` to the surviving player and immediately re-queues them.
- Reconnecting from the same IP restores the previous user session automatically.
- All server events are logged via the `Logger` class with timestamped entries.