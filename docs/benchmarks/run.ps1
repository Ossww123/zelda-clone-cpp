param(
    [ValidateSet("single", "multi")]
    [string]$Mode = "single",
    [int]$WorkerCount = 8,
    [int]$BotCount = 100,
    [int]$DummyIocpThreads = 4,
    [int]$MoveMs = 120,
    [int]$AttackMs = 700,
    [int]$DurationSec = 180
)

$ErrorActionPreference = "Stop"

# Adjust these if your output folders differ.
$serverDir = Join-Path $PSScriptRoot "..\..\Zelda-Winapi\Binaries\x64\Server"
$dummyDir = Join-Path $PSScriptRoot "..\..\Zelda-Winapi\Binaries\x64\DummyClient"

Write-Host "[Benchmark] Mode=$Mode Workers=$WorkerCount Bots=$BotCount Duration=${DurationSec}s"
Write-Host "[Benchmark] ServerDir=$serverDir"
Write-Host "[Benchmark] DummyDir=$dummyDir"

Push-Location $serverDir
try {
    if ($Mode -eq "multi") {
        $serverArgs = @("multi", "$WorkerCount")
    }
    else {
        $serverArgs = @()
    }

    $serverProc = Start-Process -FilePath ".\Server.exe" -ArgumentList $serverArgs -PassThru
    Start-Sleep -Seconds 2
}
finally {
    Pop-Location
}

Push-Location $dummyDir
try {
    $dummyArgs = @("$BotCount", "$DummyIocpThreads", "$MoveMs", "$AttackMs")
    $dummyProc = Start-Process -FilePath ".\DummyClient.exe" -ArgumentList $dummyArgs -PassThru

    Start-Sleep -Seconds $DurationSec
}
finally {
    Pop-Location
}

Write-Host "[Benchmark] Stopping processes..."
if ($dummyProc -and -not $dummyProc.HasExited) { Stop-Process -Id $dummyProc.Id -Force }
if ($serverProc -and -not $serverProc.HasExited) { Stop-Process -Id $serverProc.Id -Force }

Write-Host "[Benchmark] Done. Check Binaries/x64/logs for generated logs."
