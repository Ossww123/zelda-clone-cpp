# Zelda-Clone Multiplayer Game

> **C++ 기반 2D 액션 RPG 멀티플레이어 게임**
> Client-Server 아키텍처를 활용한 실시간 동기화 게임 프로젝트

## 목차
- [프로젝트 개요](#프로젝트-개요)
- [기술 스택](#기술-스택)
- [아키텍처](#아키텍처)
- [주요 구현 시스템](#주요-구현-시스템)
- [핵심 기술 구현](#핵심-기술-구현)
- [학습 내용 및 트러블슈팅](#학습-내용-및-트러블슈팅)
- [빌드 및 실행](#빌드-및-실행)

---

## 프로젝트 개요

Zelda 스타일의 2D 액션 RPG를 멀티플레이어로 구현한 프로젝트입니다. 클라이언트-서버 아키텍처를 기반으로 **서버 권위 시스템(Server-Authoritative)** 을 적용하였습니다.

### 주요 특징
- **실시간 멀티플레이어**: 여러 플레이어가 동시에 접속하여 게임 플레이
- **마을/던전 시스템**: 공용 마을과 파티별 인스턴스 던전
- **채널 시스템**: 서버 부하 분산을 위한 채널 구조
- **서버 권위 전투**: 전투 판정 및 피해 계산을 서버에서 처리
- **몬스터 AI**: Aggro/Leash 메커니즘을 활용한 몬스터 행동 패턴

---

## 기술 스택

### Client
- **C++ / Win32 API**: 게임 클라이언트 렌더링 및 입력 처리
- **Custom Game Engine**: Component 기반 게임 오브젝트 시스템
  - Sprite/Flipbook 애니메이션
  - Collider 기반 충돌 감지
  - Scene 관리 시스템

### Server
- **C++ / ServerCore**: 커스텀 네트워킹 라이브러리
- **Protobuf**: 네트워크 패킷 직렬화
- **Multi-threaded Architecture**: IOCP 기반 비동기 네트워크 처리

### Core Technologies
- **IOCP (I/O Completion Port)**: 고성능 비동기 네트워크 통신
- **Job Queue**: 멀티스레드 환경 동기화
- **Object Pool**: 메모리 효율적인 객체 관리
- **A* Pathfinding**: 몬스터 이동 경로 탐색

---

## 아키텍처

### 전체 구조
```
┌─────────────┐                  ┌──────────────────┐
│   Client    │◄────Protobuf────►│     Server       │
│  (Win32 API)│                  │  (ServerCore)    │
└─────────────┘                  └──────────────────┘
      │                                   │
      │                                   ├─ GameRoomManager
      │                                   │   ├─ Town (Static Room)
      │                                   │   └─ Dungeon (Instance Room)
      │                                   │
      │                                   ├─ GameSession
      └───────────────────────────────────┴─ Packet Handler
```

### GameRoom 시스템

**GameRoom**은 게임 월드의 논리적 공간 단위입니다. 각 GameRoom은 독립적인 게임 로직 실행 환경을 제공하며, JobQueue를 통해 동기화 문제를 해결합니다.

#### 1. Static Room (마을)
- 모든 플레이어가 공유하는 공간
- 채널별로 구분 (Town_CH1, Town_CH2, ...)
- 몬스터 스폰 없음

#### 2. Instance Room (던전)
- 파티별로 독립적인 던전 생성
- 고유 Instance ID 할당
- 던전 내 몬스터 스폰/리스폰 관리
- 플레이어 전원 퇴장 시 던전 삭제

```cpp
// GameRoom 구조 (데이터 기반)
class GameRoom {
    map<uint64, PlayerRef> _players;
    map<uint64, MonsterRef> _monsters;
    map<uint64, GameObjectRef> _projectiles;

    Tilemap _tilemap;

    // 데이터 기반 설정
    const RoomConfigData* _config;
    const RoomSpawnConfig* _spawnConfig;

    queue<function<void()>> _jobs;  // JobQueue
};
```

---

## 주요 구현 시스템

### 1. Network Synchronization

**Server-Authoritative 방식**으로 모든 게임 로직을 서버에서 처리하고, 클라이언트는 입력과 렌더링만 담당합니다.

#### 이동 동기화
```cpp
// Client → Server
C_Move {
    DIR_TYPE dir;
    int32 targetx, targety;
}

// Server → All Clients
S_Move {
    ObjectInfo info;  // id, position, direction, state
}
```

#### 전투 동기화
```cpp
// Client → Server
C_Attack {
    DIR_TYPE dir;
    WEAPON_TYPE weaponType;  // Sword, Bow, Staff
}

// Server → All Clients
S_Attack { /* 공격 애니메이션 */ }
S_Damaged { /* 피해량 및 HP 업데이트 */ }
```

**서버 판정 로직**:
1. 클라이언트가 공격 입력 전송
2. 서버에서 공격 범위 내 타겟 검증
3. 피해 계산 (공격 배율, 방어력 등)
4. 모든 클라이언트에 결과 브로드캐스트

### 2. 전투 시스템

#### 무기별 공격 메커니즘

| 무기 | 타입 | 특징 |
|------|------|------|
| **Sword** | 근접 | 즉발 판정, 범위 공격 |
| **Bow** | 원거리 | 발사체(Arrow) 생성, 충돌 판정 |
| **Staff** | 마법 | 관통형 발사체, 다수 타격 |

```cpp
void GameRoom::Handle_C_Attack(GameSessionRef session, const Protocol::C_Attack& pkt) {
    PlayerRef attacker = FindPlayer(session->playerId);

    switch (pkt.weapontype()) {
        case WEAPON_TYPE::Sword:
            Handle_SwordAttack(attacker, pkt);  // 즉시 판정
            break;
        case WEAPON_TYPE::Bow:
            Handle_BowAttack(attacker, pkt);    // Arrow 생성
            break;
        case WEAPON_TYPE::Staff:
            Handle_StaffAttack(attacker, pkt);  // 관통 발사체 생성
            break;
    }
}
```

### 3. 몬스터 AI

#### State Machine 기반 행동 패턴

```cpp
enum class CreatureState {
    Idle,   // 대기 상태
    Move,   // 이동 (추격)
    Skill   // 공격 상태
};
```

#### Aggro & Leash 시스템

```cpp
class Monster {
    Vec2Int _homePos;      // 스폰 위치
    int32 _aggroRange;     // 인식 범위 (8칸)
    int32 _leashRange;     // 최대 추격 범위 (10칸)
};

void Monster::UpdateIdle() {
    // 인식 범위 내 플레이어 탐색
    PlayerRef target = _room->FindClosestPlayer(_cellPos);
    if (distance(target, _cellPos) <= _aggroRange) {
        _target = target;
        SetState(CreatureState::Move);
    }
}

void Monster::UpdateMove() {
    // Leash 범위 초과 시 귀환
    if (distance(_cellPos, _homePos) > _leashRange) {
        _target.reset();
        // A* 알고리즘으로 귀환 경로 탐색
        FindPath(_cellPos, _homePos);
    }
}
```

#### 던전 전용 스폰 시스템

- 몬스터는 **던전에서만 스폰**
- 사망 시 **5초 후 자동 리스폰**
- 홈 포지션 기반 행동 범위 제한

```cpp
void GameRoom::ReserveMonsterRespawn(Vec2Int homePos) {
    uint64 respawnTime = GetTickCount64() + 5000;  // 5초 후
    _respawnQueue.push_back({respawnTime, homePos});
}
```

### 4. GameRoom Job Queue

멀티스레드 환경에서 **Race Condition**을 방지하기 위해 각 GameRoom은 독립적인 JobQueue를 사용합니다.

```cpp
void GameRoom::PushJob(function<void()> job) {
    WRITE_LOCK;
    _jobs.push(job);
}

void GameRoom::Update() {
    // JobQueue 처리
    while (!_jobs.empty()) {
        auto job = _jobs.front();
        _jobs.pop();
        job();  // 순차 실행으로 동기화 보장
    }

    // 게임 로직 업데이트
    for (auto& monster : _monsters)
        monster->Update();
}
```

**적용 사례**:
- 플레이어 이동/공격 패킷 처리
- 몬스터 리스폰
- 발사체 충돌 판정

### 5. 맵 이동 및 채널 시스템

```cpp
void Handle_C_ChangeMap(GameSessionRef session, Protocol::C_ChangeMap pkt) {
    FieldId targetField = pkt.mapid();
    int32 channel = pkt.channel();

    // 현재 룸에서 퇴장
    currentRoom->LeaveRoom(session);

    if (targetField == FieldId::Dungeon) {
        // 던전: 새 인스턴스 생성
        uint64 instanceId = GRoomManager.CreateDungeonInstance();
        GameRoomRef dungeon = GRoomManager.GetDungeonInstance(instanceId);
        dungeon->EnterRoom(session);
    } else {
        // 마을: 정적 룸 입장
        GameRoomRef town = GRoomManager.GetStaticRoom(targetField, channel);
        town->EnterRoom(session);
    }
}
```

### 6. A* Pathfinding

몬스터의 스마트한 이동을 위한 경로 탐색 알고리즘 구현.

```cpp
bool GameRoom::FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth) {
    priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
    map<Vec2Int, int32> best;
    map<Vec2Int, Vec2Int> parent;

    // A* 알고리즘
    // F = G + H
    // G: 시작점으로부터의 실제 거리
    // H: 목표지점까지의 휴리스틱(맨하탄 거리)

    pq.push(PQNode(0, src));
    best[src] = 0;

    while (!pq.empty()) {
        PQNode node = pq.top();
        pq.pop();

        if (node.pos == dest) {
            // 경로 복원
            ReconstructPath(parent, dest, path);
            return true;
        }

        // 4방향 탐색
        for (Vec2Int delta : {Vec2Int{0,1}, {0,-1}, {1,0}, {-1,0}}) {
            Vec2Int next = node.pos + delta;

            if (!CanGo(next)) continue;

            int32 g = best[node.pos] + 1;
            int32 h = abs(dest.x - next.x) + abs(dest.y - next.y);

            if (best.find(next) == best.end() || g < best[next]) {
                best[next] = g;
                pq.push(PQNode(g + h, next));
                parent[next] = node.pos;
            }
        }
    }

    return false;
}
```

---

## 핵심 기술 구현

### 1. RoomLogic 분리

초기에는 GameRoom이 마을과 던전 로직을 모두 포함하고 있었으나, **Strategy Pattern**을 적용하여 로직을 분리했습니다.

```cpp
// Before: GameRoom에 모든 로직이 혼재
class GameRoom {
    void Update() {
        if (_fieldId == FieldId::Town) {
            // 마을 로직
        } else if (_fieldId == FieldId::Dungeon) {
            // 던전 로직 (몬스터 스폰 등)
        }
    }
};

// After: Strategy Pattern 적용
class IRoomLogic {
    virtual void Update(GameRoom* room) = 0;
};

class TownLogic : public IRoomLogic {
    void Update(GameRoom* room) override {
        // 마을 전용 로직
    }
};

class DungeonLogic : public IRoomLogic {
    void Update(GameRoom* room) override {
        // 몬스터 스폰/리스폰 처리
        room->ProcessRespawn();
    }
};
```

**장점**:
- 코드 가독성 향상
- 새로운 필드 타입 추가 용이 (보스전 룸, PvP 룸 등)
- 테스트 및 유지보수 편의성

### 2. Camera System

초기 하드코딩된 카메라를 Component 시스템으로 리팩토링.

```cpp
// Before: 하드코딩된 카메라 위치
void Render() {
    int32 cameraX = player->GetPos().x - SCREEN_WIDTH / 2;
    int32 cameraY = player->GetPos().y - SCREEN_HEIGHT / 2;
    // ...
}

// After: CameraComponent
class CameraComponent : public Component {
    void SetTarget(GameObject* target) { _target = target; }
    Vec2 GetCameraPos();
};
```

### 3. 공격 배율 시스템

무기별 데미지 밸런싱을 위한 공격 배율 파라미터 추가.

```cpp
void Creature::OnDamaged(CreatureRef attacker, int32 damage, float multiplier) {
    int32 finalDamage = static_cast<int32>(damage * multiplier);
    _stat.hp = max(0, _stat.hp - finalDamage);

    if (_stat.hp == 0) {
        OnDead(attacker);
    }
}

// 무기별 배율 설정
Handle_SwordAttack(...) {
    target->OnDamaged(attacker, damage, 1.0f);  // 기본 배율
}

Handle_StaffAttack(...) {
    target->OnDamaged(attacker, damage, 0.7f);  // 약한 대신 관통
}
```

---

## 학습 내용 및 트러블슈팅

### 1. 멀티스레드 환경에서의 동기화 문제

**문제**: 여러 스레드(네트워크 스레드, 게임 로직 스레드)에서 동시에 GameRoom의 데이터에 접근하여 **Race Condition** 발생

**해결**:
- **JobQueue 패턴** 도입
- 모든 GameRoom 작업을 Job으로 래핑하여 순차 실행
- Lock을 최소화하면서도 데이터 무결성 보장

**학습 내용**:
- Lock-based vs Lock-free 동기화 방식의 트레이드오프
- Job System의 실제 게임 적용 경험
- 멀티스레드 디버깅 기법

### 2. 네트워크 지연과 클라이언트 예측

**문제**: 네트워크 지연으로 인한 끊김 현상

**현재 구현**:
- 서버 권위 방식으로 정확성 우선
- 클라이언트는 서버 응답 대기

**향후 개선 방향**:
- Client-side Prediction 구현
- Server Reconciliation으로 정확성과 반응성 양립

**학습 내용**:
- 네트워크 게임의 근본적인 지연 문제 이해
- Authoritative Server의 장단점
- Prediction/Reconciliation 기법 이론 학습

### 3. 인스턴스 던전 관리

**문제**: 던전 인스턴스가 메모리에 계속 누적되는 문제

**해결**:
```cpp
void GameRoom::Update() {
    // 던전이고 플레이어가 모두 나갔을 경우
    if (IsDungeonInstance() && GetPlayerCount() == 0) {
        GRoomManager.RequestRemoveDungeonInstance(_instanceId);
    }
}

void GameRoomManager::Update() {
    // 삭제 요청된 던전 제거
    for (uint64 instanceId : _pendingRemoveDungeon) {
        _dungeonInstances.erase(instanceId);
    }
    _pendingRemoveDungeon.clear();
}
```

**학습 내용**:
- 인스턴스 기반 콘텐츠의 라이프사이클 관리
- 메모리 누수 디버깅 및 해결
- RAII와 스마트 포인터의 중요성

### 4. Protobuf 메시지 자동화

**문제**: 패킷 핸들러 작성 시 반복 코드 과다

**해결**:
- Python 스크립트로 .proto 파일 파싱
- PacketHandler 자동 생성 코드 작성
- 메시지 ID 매핑 자동화

**학습 내용**:
- 코드 제너레이션을 통한 생산성 향상
- 메타프로그래밍의 실용적 활용
- 빌드 파이프라인 자동화

---

## 빌드 및 실행

### 요구사항
- **Visual Studio 2022** (Platform Toolset v143)
- **Windows 10 SDK**
- **Protobuf 3.21.12** (포함됨)

### 빌드 방법

1. **솔루션 열기**
   ```
   Zelda-Winapi/Zelda-Winapi.sln
   ```

2. **빌드 순서**
   ```
   1) ServerCore (Static Library)
   2) Server (Application)
   3) Client (Application)
   4) DummyClient (Optional)
   ```

3. **실행**
   - Server 먼저 실행
   - Client 실행 (여러 개 동시 실행 가능)

### 프로젝트 구조
```
Zelda-Winapi/
├── Client/           # 게임 클라이언트
├── Server/           # 게임 서버
├── ServerCore/       # 네트워킹 라이브러리
├── DummyClient/      # 테스트용 더미 클라이언트
├── Common/           # Protobuf 정의 및 공유 코드
├── Libraries/        # 외부 라이브러리 (Protobuf)
└── Resources/        # 게임 리소스 (스프라이트, 사운드, 타일맵)
```

---

## 개발 환경

- **IDE**: Visual Studio 2022
- **언어**: C++17
- **플랫폼**: Windows
- **버전 관리**: Git

---

## 데이터 기반 GameRoom 시스템

최근 리팩토링을 통해 **데이터 기반 GameRoom 시스템**으로 전환하였습니다. 이를 통해 코드 수정 없이 새로운 룸 타입과 몬스터 스폰 설정을 추가할 수 있게 되었습니다.

### 아키텍처 변경

**Before (코드 기반)**:
```cpp
// 하드코딩된 로직 클래스
unique_ptr<IRoomLogic> _logic;  // TownLogic / DungeonLogic

// 스폰 위치 하드코딩
Vec2Int anchorUR = { 30, 5 };
Vec2Int anchorDR = { 30, 25 };
Vec2Int anchorDL = { 5, 25 };
```

**After (데이터 기반)**:
```cpp
// CSV/JSON 데이터 로딩
const RoomConfigData* _config;
const RoomSpawnConfig* _spawnConfig;

// 데이터 기반 초기화
room->InitFromConfig("dungeon_snake");
```

### 데이터 파일 구조

#### 1. RoomConfig.csv (간단한 룸 설정)
```csv
RoomId,RoomName,SkillEnabled,MonsterSpawnEnabled,RespawnTimeMs,TilemapPath,MaxPlayers,IsInstance
town_1,Town Square,false,false,0,../Resources/Tilemap/Tilemap_01.txt,50,false
dungeon_snake,Snake Dungeon,true,true,10000,../Resources/Tilemap/Tilemap_02.txt,4,true
```

#### 2. MonsterSpawn.json (복잡한 스폰 설정)
```json
{
  "dungeon_snake": {
    "spawns": [
      {
        "groupId": "entrance_right",
        "anchor": [30, 5],
        "offsets": [[0, 0], [1, 0], [0, 1]],
        "monsters": [
          {
            "templateId": 1001,
            "count": 3,
            "level": 1,
            "aggroRange": 8,
            "leashRange": 10
          }
        ]
      }
    ]
  }
}
```

#### 3. MonsterTemplate.csv (몬스터 스탯)
```csv
templateId,name,maxHp,attack,defence
1001,Snake,30,5,1
1002,Slime,50,6,1
1001,Snake,30,5,1
1004,Wolf,60,8,1
```

### 새로운 룸 추가 방법

**기존 방식**: 새로운 Logic 클래스 작성 + 컴파일 필요

**데이터 기반**: 데이터 파일만 수정
```csv
# RoomConfig.csv에 추가
boss_dragon,Dragon Lair,true,true,60000,../Resources/Tilemap/Boss_01.txt,4,true
```

```json
// MonsterSpawn.json에 추가
{
  "boss_dragon": {
    "spawns": [
      {
        "groupId": "boss",
        "anchor": [20, 15],
        "offsets": [[0, 0]],
        "monsters": [{"templateId": 2001, "count": 1, "level": 10}]
      }
    ]
  }
}
```

### 핵심 클래스

- **RoomDataManager**: 싱글톤 패턴으로 모든 룸 데이터 로딩 및 관리
- **RoomConfigData**: 룸 기본 설정 (스킬 허용, 몬스터 스폰 여부 등)
- **RoomSpawnConfig**: 몬스터 스폰 위치 및 개수
- **MonsterTemplateData**: 몬스터 종류별 스탯

### 장점

1. **확장성**: 새로운 룸/던전 추가가 데이터만으로 가능
2. **유지보수**: 스폰 위치/개수 변경 시 재컴파일 불필요
3. **기획자 친화적**: CSV/JSON 파일로 쉽게 설정 변경 가능
4. **일관성**: MonsterTemplate.csv와 통합하여 데이터 일관성 확보

---

## 향후 계획

- [ ] Client-side Prediction 구현
- [ ] 몬스터 종류 및 스킬 다양화
- [ ] 인벤토리 시스템
- [ ] 플레이어 스탯/레벨링 시스템
- [ ] 데이터베이스 연동 (플레이어 정보 저장)
- [ ] 파티 시스템

