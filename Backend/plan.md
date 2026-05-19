# Football Scouting Web App - Requirements Plan

## 1) Vision
Build a football scouting web app where scouts can visit schools, evaluate players, and track prospects for professional pathways.

The implementation order is:
1. Backend first
2. Frontend second

## 2) Core Product Goals
- Allow scouts to create and manage school visits.
- Allow scouts to record player profiles, metrics, and observations.
- Support fast player lookup, ranking, and filtering.
- Maintain historical scouting records for long-term development tracking.
- Allow users to build hypothetical teams from scouted players for simulation and comparison.
- Provide secure authentication/login and role-based access for protected operations.

## 3) Non-Negotiable Technical Rule (DSA-First)
Before major feature development, create foundational data structures in a dedicated folder:

- `src/structures/LinkedList.*`
- `src/structures/AVLTree.*`
- `src/structures/HashTable.*`

These must be project-owned implementations (not only STL wrappers), and will be used by services/models.

## 4) Data Access Strategy
The project will **not** rely on direct SQL query usage in route/controllers for app behavior.
Instead:

1. Fetch raw records from SQL via repository/data-access layer.
2. Load/transform records into in-memory structures (`LinkedList`, `AVLTree`, `HashTable`).
3. Perform existence checks, indexing, search, sorting/ranking, and retrieval through those structures.
4. Return structured model/view DTO responses to API callers.

This creates explicit algorithmic behavior and enforces DSA fundamentals in core logic.

## 5) Proposed Backend Architecture
- `src/routes/` - API route registration.
- `src/controller/` - request/response handlers.
- `src/service/` - business logic.
- `src/repository/` - SQL fetch/update layer.
- `src/model/` - domain models (`Player`, `Scout`, `School`, `Visit`, `Report`).
- `src/structures/` - custom DSA implementations.
- `src/index/` (optional) - structure adapters/index managers.

## 6) Domain Entities (Initial)
- **Player**: id, name, age, schoolId, preferredSpot, shoot, tackle, speed, accuracy, awareness, potentialScore.
- **School**: id, name, location, contact info.
- **Scout**: id, name, region, team affiliation.
- **Visit**: id, scoutId, schoolId, date, notes.
- **ScoutingReport**: id, playerId, scoutId, visitId, ratings, strengths, weaknesses, recommendation.
- **HypotheticalTeam**: id, ownerUserId, name, formation, createdAt.
- **TeamPlayer**: id, teamId, playerId, assignedSpot, orderIndex.

### Player Preferred Spot Enum
Define an enum for preferred football position, e.g.:
- `GK`
- `RB`, `CB`, `LB`
- `CDM`, `CM`, `CAM`
- `RW`, `LW`
- `ST`

Store this enum on `Player.preferredSpot` and validate assignment rules when adding players to hypothetical teams.

## 7) DSA Usage Mapping
- **Linked List**
  - Maintain ordered scouting report history per player.
  - Queue-like traversal of visit logs.

- **AVL Tree**
  - Balanced ranking index for players by score/potential.
  - Fast sorted retrieval for top prospects in region/school.

- **Hash Table**
  - O(1)-style lookup by IDs (`playerId`, `schoolId`, `scoutId`).
  - Fast existence checks before insert/update actions.
  - Prevent duplicate player inserts within a hypothetical team.

## 8) Backend Milestones
1. **M1 - Foundation**
   - Create folder layout.
   - Implement and test `LinkedList`, `AVLTree`, `HashTable`.
   - Define core model classes.

2. **M2 - Repository + Structure Hydration**
   - Build DB connection/repository layer.
   - Fetch entities from SQL and hydrate structures.
   - Add synchronization strategy (reload, incremental updates).

3. **M3 - Service Layer**
   - Business flows using structures only:
     - add/update player
     - check existence
     - rank players
     - fetch history
     - create hypothetical teams
     - add/remove team players and validate position fit

4. **M4 - API Layer**
   - Expose endpoints for school visits, player scouting, ranking, and search.
   - Validate requests and return consistent JSON responses.

5. **M5 - Quality**
   - Unit tests for each structure.
   - Service tests for ranking/search logic.
   - Integration tests for repository + hydration + API behavior.

## 9) Initial API Requirement Draft
- `POST /auth/register`
- `POST /auth/login`
- `POST /auth/logout`
- `GET /auth/me`
- `POST /schools`
- `POST /players`
- `POST /visits`
- `POST /reports`
- `GET /players/:id`
- `GET /players/top`
- `GET /players/potential`
- `GET /schools/:id/players`
- `GET /reports/player/:playerId`
- `POST /teams`
- `POST /teams/:teamId/players`
- `DELETE /teams/:teamId/players/:playerId`
- `GET /teams/:teamId`
- `GET /teams/:teamId/analysis`

### Player Potential Filters
`GET /players/potential` should support:
- By school: `schoolId`
- By preferred position: `preferredSpot`
- By stat range: min/max for one or more of:
  - `shoot`
  - `tackle`
  - `speed`
  - `accuracy`
  - `awareness`

## 10) Frontend (After Backend Stabilization)
- Build scout dashboard to:
  - browse schools
  - add visits
  - create scouting reports
  - view rankings and search results powered by backend structures

## 11) Risks and Controls
- **Risk**: Structure state diverges from SQL state.
  - **Control**: define strict write-through or reload policy.

- **Risk**: Manual structure mapping complexity grows.
  - **Control**: repository + mapper boundaries and tests.

- **Risk**: Performance bottlenecks with large datasets.
  - **Control**: benchmark structure operations and cache strategy.

## 12) Input/Output Contracts
- All request and response payloads are JSON.
- Set `Content-Type: application/json` for API communication.
- Standard response envelope:
  - success:
    - `{ "success": true, "data": { ... }, "message": "..." }`
  - error:
    - `{ "success": false, "error": { "code": "...", "message": "..." } }`
- Validation errors return structured JSON with field-level details.

## 13) Validation Rules
- Player stat fields must be integers in range `1-100`:
  - `shoot`, `tackle`, `speed`, `accuracy`, `awareness`
- Reject invalid payloads before service/repository operations.
- Prevent duplicate player assignment in the same hypothetical team.

## 14) Authentication and Authorization
- Implement auth endpoints for register/login/logout/session lookup.
- Use token-based auth (JWT) or secure session tokens.
- Enforce role-based access control:
  - `admin`: full access
  - `scout`: create/update visits, reports, teams
  - `viewer`: read-only access
- Team/report modifications must require ownership or admin rights.

## 15) Immediate Next Step
Implement `src/structures/` with:
- `LinkedList.h/.cpp`
- `AVLTree.h/.cpp`
- `HashTable.h/.cpp`

Then add unit tests to validate insertion, deletion, search, traversal, balancing, and collision handling.
