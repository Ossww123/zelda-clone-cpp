# Benchmark Guide

이 폴더는 부하 테스트 재현 방법과 결과 요약을 저장합니다.
원본 런타임 로그(`Binaries/x64/logs`)는 용량/노이즈 문제로 저장소에 포함하지 않고 로컬에 보관합니다.

## Test Scenario
- Server mode:
  - Single: `Server.exe`
  - Multi: `Server.exe multi 8`
- DummyClient: `100 bots`, `move interval 120ms`, `attack interval 700ms`
- Duration: `180s`
- Aggregation rule: 안정구간(`S_EnterGame=0`) 평균
- Repeats: Single 3회 / Multi 3회

## Commands (PowerShell)
자세한 템플릿은 `docs/benchmarks/run.ps1` 참고.

```powershell
# Single
.\Server.exe

# Multi (worker 8)
.\Server.exe multi 8

# DummyClient (bots iocpThreads moveMs attackMs)
.\DummyClient.exe 100 4 120 700
```

## Result Files
- `results.csv`: README에 반영된 평균값(3회 평균) 저장
- `run.ps1`: 실행/수집 템플릿 스크립트

## Notes
- 추후 p95/p99 latency, CPU 사용률을 같은 포맷으로 추가 권장
- 필요 시 `raw/` 폴더를 만들어 로컬 로그 파일명만 인덱싱 가능
