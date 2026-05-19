# Football Scouting Platform

A full-stack scouting and team-management application built for a C++ (semester 3) coursework project. The **backend** is a C++17 REST API using [Crow](https://github.com/CrowCpp/Crow) with custom data structures; the **frontend** is a React + Vite single-page app.

## Features

- **Authentication** — register, login, logout, and role-based access (`admin`, `scout`, `viewer`)
- **Schools** — browse and manage academies; list players per school
- **Players** — CRUD, stat filters, scouting visits and reports
- **Rankings** — top players and potential rankings backed by an in-memory **AVL tree**
- **Teams** — create squads, assign lineups, roster analysis
- **Formations** — predefined templates with lineup suggestions
- **Match simulation** — pass-chain attack simulation on a formation **graph**, using a **max-heap** to pick the best next pass

## Tech stack

| Layer    | Stack |
|----------|--------|
| Backend  | C++17, Crow (standalone Asio), CMake |
| Frontend | React 19, Vite 8 |
| Storage  | CSV files under `Backend/data/` (demo / coursework scope) |

## Project structure

```
project/
├── Backend/
│   ├── main.cpp              # HTTP server entry (port 8080)
│   ├── CMakeLists.txt
│   ├── data/                 # CSV persistence
│   ├── external/Crow/        # HTTP framework (git submodule / vendored)
│   └── src/
│       ├── controller/       # Request handlers
│       ├── model/            # Domain types & CSV stores
│       ├── routes/           # Crow route registration
│       ├── service/          # Business logic & in-memory indexes
│       ├── structures/       # AVL tree, hash table, linked list, max-heap
│       └── view/             # JSON response builders
└── Frontend/
    └── src/
        ├── api/              # API client & scouting endpoints
        ├── components/
        ├── context/          # Auth state
        └── pages/
```

## Custom data structures

Implemented in `Backend/src/structures/` and used by `ScoutingService`:

| Structure   | Role |
|-------------|------|
| **AVLTree** | Sorted player rankings (`/players/top`, `/players/potential`) |
| **HashTable** | O(1) lookups for players, schools, teams, sessions |
| **LinkedList** | School lists, rosters, visit log, report chains |
| **MaxHeap** | Next-pass selection during attack simulation |
| **FormationGraph** | Pitch nodes, adjacency, and pass/shoot simulation (`model/formation_graph.cpp`) |

## Prerequisites

- **Backend:** CMake 3.16+, a C++17 compiler (GCC or Clang)
- **Frontend:** Node.js 18+ and npm or [pnpm](https://pnpm.io/)

## Getting started

### 1. Build and run the backend

```bash
cd Backend
mkdir -p build && cd build
cmake ..
cmake --build .
```

Run the server from **`Backend/build`** (resolves `../data/*.csv`) or from **`Backend`** (uses `data/*.csv`):

```bash
# from Backend/build
./server

# or from Backend
./build/server
```

The API listens on **http://127.0.0.1:8080**.

Health checks:

- `GET /health`
- `GET /db-health`

### 2. Run the frontend

```bash
cd Frontend
npm install    # or: pnpm install
npm run dev    # or: pnpm dev
```

Vite proxies API routes to `http://127.0.0.1:8080` (see `Frontend/vite.config.js`). Open the URL printed in the terminal (typically **http://localhost:5173**).

### 3. Sign in

Create an account from the login screen or add a row to `Backend/data/users.csv`. Passwords are stored in **plain text** — suitable only for local demos, not production.

## API overview

| Area | Endpoints |
|------|-----------|
| Auth | `POST /auth/register`, `POST /auth/login`, `POST /auth/logout`, `GET /auth/me` |
| Schools | `GET/POST /schools`, `GET/PUT/DELETE /schools/:id`, `GET /schools/:id/players`, `POST /visits` |
| Players | `GET /players/top`, `GET /players/potential`, `GET/POST /players/:id`, `POST /reports`, `GET /reports/player/:id` |
| Teams | `POST /teams`, `GET/PUT/DELETE /teams/:id`, roster & lineup routes, `GET /teams/:id/analysis`, `GET /teams/:id/simulate` |
| Formations | `GET /formations`, `GET /formations/:id/suggest-lineup` |

Authenticated requests send `Authorization: Bearer <token>` (token from login response).

## Simulation

`GET /teams/:id/simulate` runs an attack simulation on the team’s current lineup:

- Query params: `startPlayerId`, `maxSteps` (default 12), `riskSeed` (optional)
- Builds a formation graph from the team’s formation and positions
- Steps through passes using heap-based choice of the best adjacent option, with shoot vs pass decisions

The Teams page in the frontend exposes this via the formation simulator UI.

## Data files

| File | Contents |
|------|----------|
| `users.csv` | User accounts |
| `sessions.csv` | Auth tokens |
| `schools.csv` | Academies |
| `players.csv` | Player profiles and stats |
| `teams.csv` | Squads |
| `team_players.csv` | Roster slots and preferred positions |

Visits are kept in memory only (not written to disk).

## Development notes

- Backend follows an MVC-style layout: **routes → controllers → service → repository/CSV**.
- `ScoutingService::hydrate()` loads CSV data and rebuilds indexes at startup; failed hydrate exits the process.
- Crow is configured with `CROW_USE_BOOST OFF` (standalone Asio) in `Backend/CMakeLists.txt`.

## License

Coursework / educational project. Crow is vendored under its own license in `Backend/external/Crow/`.
