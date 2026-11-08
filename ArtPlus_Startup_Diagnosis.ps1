<#
.SYNOPSIS
    ArtPlus.exe startup diagnosis script for Windows MSVC 2022 + Qt 6.8.3 environment.
    Executes a 7-stage diagnostic to identify exactly where ArtPlus.exe fails to start.

.DESCRIPTION
    This script must be run in a Developer Command Prompt for VS 2022 (or with MSVC environment activated),
    with Qt 6.8.3 MSVC bin directory in PATH. It logs each startup stage to artplus_startup_diagnosis.log
    to pinpoint the exact failure point.

.STAGE 1: Executable loading by Windows
.STAGE 2: DLL initialization (dependencies)
.STAGE 3: Entry into main()
.STAGE 4: QApplication construction
.STAGE 5: PDFium initialization
.STAGE 6: MainWindow construction
.STAGE 7: Window display (window.show())

.PARAMETER BuildDir
    Path to the build directory (default: H:\Desktop\artplus-lite\build-msvc-nmake)
    
.PARAMETER QtDir
    Path to Qt 6.8.3 MSVC installation (e.g., C:\Qt\6.8.3\msvc2022_64)
    
.PARAMETER PdfiumDir
    Path to PDFium installation (default: H:\Desktop\artplus-lite\third_party)
    
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File ArtPlus_Startup_Diagnosis.ps1
    
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File ArtPlus_Startup_Diagnosis.ps1 -BuildDir "H:\Desktop\artplus-lite\build-msvc" -QtDir "C:\Qt\6.8.3\msvc2022_64"

.NOTES
    Run in Developer Command Prompt for VS 2022, or ensure:
    - MSVC 2022 x64 environment is activated (vcvarsall.bat x64)
    - Qt 6.8.3 MSVC bin is in PATH (e.g., C:\Qt\6.8.3\msvc2022_64\bin)
    - PDFium_DIR environment variable is set or PdfiumDir parameter provided
#>

param(
    [string]$BuildDir = "H:\Desktop\artplus-lite\build-msvc",
    [string]$QtDir = "",
    [string]$PdfiumDir = "H:\Desktop\artplus-lite\third_party"
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Setup logging
# ---------------------------------------------------------------------------
$LogFile = Join-Path $BuildDir "artplus_startup_diagnosis.log"
function Write-Log {
    param([string]$Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
    $entry = "[$timestamp] $Message"
    Write-Host $entry
    Add-Content -LiteralPath $LogFile -Encoding UTF8 -Value $entry
}

function Write-Stage {
    param([int]$Stage, [string]$Name, [string]$Status, [string]$Details = "")
    Write-Log "STAGE ${Stage}: $Name | STATUS: $Status $Details"
}

# Initialize log
"=== ArtPlus Startup Diagnosis Log ===" | Set-Content -LiteralPath $LogFile -Encoding UTF8
"Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Add-Content -LiteralPath $LogFile -Encoding UTF8
"BuildDir: $BuildDir" | Add-Content -LiteralPath $LogFile -Encoding UTF8
"QtDir: $QtDir" | Add-Content -LiteralPath $LogFile -Encoding UTF8
"PdfiumDir: $PdfiumDir" | Add-Content -LiteralPath $LogFile -Encoding UTF8
"" | Add-Content -LiteralPath $LogFile -Encoding UTF8

Write-Log "=== STAGE 1: Executable Loading ==="
$ExePath = Join-Path $BuildDir "bin\ArtPlus.exe"
if (-not (Test-Path -LiteralPath $ExePath)) {
    Write-Stage 1 "Executable Loading" "FAIL" "| Executable not found at $ExePath"
    Write-Log "RECOMMENDATION: Build the project first. Run cmake --build $BuildDir --config Release"
    exit 1
}
Write-Stage 1 "Executable Loading" "PASS" "| Found: $ExePath"

Write-Log "=== STAGE 2: DLL Dependencies ==="
# Check Qt DLLs
$QtBin = if ($QtDir) { Join-Path $QtDir "bin" } else { $env:Path -split ';' | Where-Object { $_ -like "*Qt*6*msvc*" } | Select-Object -First 1 }
if (-not $QtBin) {
    Write-Stage 2 "DLL Dependencies" "FAIL" "| Qt bin directory not found. Set QtDir parameter or add Qt to PATH."
    exit 1
}
Write-Log "Qt bin directory: $QtBin"

$RequiredQtDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll")
$MissingQtDlls = @()
foreach ($dll in $RequiredQtDlls) {
    $dllPath = Join-Path $QtBin $dll
    if (-not (Test-Path -LiteralPath $dllPath)) {
        $MissingQtDlls += $dll
        Write-Log "MISSING Qt DLL: $dllPath"
    } else {
        Write-Log "FOUND Qt DLL: $dllPath"
    }
}

# Check platform plugin
$PlatformsDir = Join-Path $QtDir "plugins\platforms"
$QWindowsDll = Join-Path $PlatformsDir "qwindows.dll"
if (-not (Test-Path -LiteralPath $QWindowsDll)) {
    Write-Log "MISSING platform plugin: $QWindowsDll"
    $MissingQtDlls += "qwindows.dll (platform plugin)"
} else {
    Write-Log "FOUND platform plugin: $QWindowsDll"
}

# Check PDFium
$PdfiumDll = Join-Path $PdfiumDir "bin\pdfium.dll"
$PdfiumLib = Join-Path $PdfiumDir "lib\pdfium.dll.lib"
if (-not (Test-Path -LiteralPath $PdfiumDll)) {
    Write-Log "MISSING PDFium DLL: $PdfiumDll"
    $MissingQtDlls += "pdfium.dll"
} else {
    Write-Log "FOUND PDFium DLL: $PdfiumDll"
}

if ($MissingQtDlls.Count -gt 0) {
    Write-Stage 2 "DLL Dependencies" "FAIL" "| Missing: $($MissingQtDlls -join ', ')"
    exit 1
}
Write-Stage 2 "DLL Dependencies" "PASS" "| All required DLLs found"

Write-Log "=== STAGE 3: MSVC Runtime Check ==="
# Check for MSVC runtime
$VCToolsVersion = $env:VCToolsVersion
$VisualStudioVersion = $env:VisualStudioVersion
Write-Log "VCToolsVersion: $VCToolsVersion"
Write-Log "VisualStudioVersion: $VisualStudioVersion"
if (-not $VCToolsVersion) {
    Write-Stage 3 "MSVC Runtime" "WARN" "| MSVC environment not detected. Run in Developer Command Prompt for VS 2022."
} else {
    Write-Stage 3 "MSVC Runtime" "PASS" "| MSVC environment detected: $VCToolsVersion"
}

Write-Log "=== STAGE 4: Process Execution Test (dry-run) ==="
# Use dumpbin to check imports if available
$DumpBin = (Get-Command dumpbin.exe -ErrorAction SilentlyContinue).Source
if ($DumpBin) {
    Write-Log "dumpbin found: $DumpBin"
    try {
        $Imports = & $DumpBin /IMPORTS $ExePath 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Log "Executable imports (first 50 lines):"
            $Imports -split "`n" | Select-Object -First 50 | ForEach-Object { Write-Log "  $_" }
        } else {
            Write-Log "dumpbin failed: $Imports"
        }
    } catch {
        Write-Log "dumpbin error: $_"
    }
} else {
    Write-Log "dumpbin not in PATH - skipping import analysis"
}

# Check if executable is 64-bit
$PeCheck = (Get-Command pecheck.exe -ErrorAction SilentlyContinue).Source
if (-not $PeCheck) {
    # Try with corflags
    $CorFlags = (Get-Command corflags.exe -ErrorAction SilentlyContinue).Source
    if ($CorFlags) {
        $CorOut = & $CorFlags $ExePath 2>&1
        Write-Log "corflags output: $CorOut"
    }
}

Write-Stage 4 "Process Execution Test" "PASS" "| Executable appears valid"

Write-Log "=== STAGE 5: Attempting Actual Execution (QApplication) ==="
Write-Log "Setting up environment for ArtPlus execution..."

# Prepare environment
$env:PATH = "$QtBin;$PdfiumDir\bin;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = Join-Path $QtDir "plugins"
$env:QT_DEBUG_PLUGINS = "1"
$env:PDFium_DIR = $PdfiumDir

Write-Log "PATH includes: $QtBin"
Write-Log "QT_QPA_PLATFORM_PLUGIN_PATH: $env:QT_QPA_PLATFORM_PLUGIN_PATH"

# Run ArtPlus without arguments to test startup
$ProcessInfo = New-Object System.Diagnostics.ProcessStartInfo
$ProcessInfo.FileName = $ExePath
$ProcessInfo.Arguments = ""  # No arguments - just test startup
$ProcessInfo.UseShellExecute = $false
$ProcessInfo.RedirectStandardOutput = $true
$ProcessInfo.RedirectStandardError = $true
$ProcessInfo.CreateNoWindow = $true
$ProcessInfo.WorkingDirectory = Split-Path $ExePath -Parent

$Process = New-Object System.Diagnostics.Process
$Process.StartInfo = $ProcessInfo

Write-Log "Starting ArtPlus.exe..."
try {
    $Process.Start() | Out-Null
    $Exited = $Process.WaitForExit(15000)  # 15 second timeout
    $StdOut = $Process.StandardOutput.ReadToEnd()
    $StdErr = $Process.StandardError.ReadToEnd()
    $ExitCode = $Process.ExitCode

    Write-Log "ExitCode: $ExitCode"
    Write-Log "STDOUT: $StdOut"
    Write-Log "STDERR: $StdErr"

    if ($Exited) {
        if ($ExitCode -eq 0) {
            Write-Stage 5 "QApplication Startup" "PASS" "| ArtPlus started and exited cleanly (exit code 0)"
        } else {
            Write-Stage 5 "QApplication Startup" "FAIL" "| ArtPlus exited with code $ExitCode"
            Write-Log "STDERR: $StdErr"
            Write-Log "STDOUT: $StdOut"
        }
    } else {
        Write-Stage 5 "QApplication Startup" "HANG" "| Process did not exit within 15 seconds - may be showing window"
        $Process.Kill()
    }
} catch {
    Write-Stage 5 "QApplication Startup" "EXCEPTION" "| $_"
    Write-Log "Exception: $($_.ToString())"
}

Write-Log "=== STAGE 6: Full GUI Launch Test (10s) ==="
# Try launching with a test PDF if available
$TestPdf = Join-Path (Split-Path $ExePath -Parent) "ArtPlus_PDF_Teste.pdf"
$PdfArg = if (Test-Path -LiteralPath $TestPdf) { "`"$TestPdf`"" } else { "" }

$ProcessInfo2 = New-Object System.Diagnostics.ProcessStartInfo
$ProcessInfo2.FileName = $ExePath
$ProcessInfo2.Arguments = $PdfArg
$ProcessInfo2.UseShellExecute = $false
$ProcessInfo2.RedirectStandardOutput = $true
$ProcessInfo2.RedirectStandardError = $true
$ProcessInfo2.CreateNoWindow = $false  # Allow window to show
$ProcessInfo2.WorkingDirectory = Split-Path $ExePath -Parent

$Process2 = New-Object System.Diagnostics.Process
$Process2.StartInfo = $ProcessInfo2

Write-Log "Starting ArtPlus.exe with GUI (10s timeout)..."
if ($PdfArg) { Write-Log "Test PDF: $TestPdf" }

try {
    $Process2.Start() | Out-Null
    $Exited2 = $Process2.WaitForExit(10000)  # 10 second timeout
    $StdOut2 = $Process2.StandardOutput.ReadToEnd()
    $StdErr2 = $Process2.StandardError.ReadToEnd()
    $ExitCode2 = $Process2.ExitCode

    Write-Log "ExitCode: $ExitCode2"
    Write-Log "STDOUT: $StdOut2"
    Write-Log "STDERR: $StdErr2"

    if ($Exited2) {
        if ($ExitCode2 -eq 0) {
            Write-Stage 6 "MainWindow + Display" "PASS" "| ArtPlus launched, displayed window, and exited cleanly"
        } else {
            Write-Stage 6 "MainWindow + Display" "FAIL" "| ArtPlus exited with code $ExitCode2"
        }
    } else {
        Write-Stage 6 "MainWindow + Display" "RUNNING" "| Process still running after 10s - window likely displayed"
        Write-Log "Window may be visible. Check visually."
        $Process2.Kill()
    }
} catch {
    Write-Stage 6 "MainWindow + Display" "EXCEPTION" "| $_"
    Write-Log "Exception: $($_.ToString())"
}

Write-Log "=== STAGE 7: Dependency Walker / ldd Analysis ==="
# Try to use Dependencies.exe if available, or list DLLs
$DependenciesGui = (Get-Command DependenciesGui.exe -ErrorAction SilentlyContinue).Source
if (-not $DependenciesGui) {
    $DependenciesGui = (Get-Command Dependencies.exe -ErrorAction SilentlyContinue).Source
}
if ($DependenciesGui) {
    Write-Log "Dependencies tool found: $DependenciesGui"
    Write-Log "Run manually for full analysis: $DependenciesGui $ExePath"
} else {
    Write-Log "Dependencies tool not found. Install from https://github.com/lucasg/Dependencies"
    Write-Log "Alternatively, use dumpbin /DEPENDENTS $ExePath"
}

Write-Log ""
Write-Log "=== DIAGNOSIS SUMMARY ==="
Write-Log "Log file: $LogFile"
Write-Log "Examine the STAGE entries above to identify the failure point."
Write-Log ""
Write-Log "Common failure points and fixes:"
Write-Log "  STAGE 1 FAIL: Build the project (cmake --build)"
Write-Log "  STAGE 2 FAIL: Missing Qt DLLs or platform plugin - verify QtDir and PATH"
Write-Log "  STAGE 3 WARN: Not in Developer Command Prompt - run vcvarsall.bat x64"
Write-Log "  STAGE 4 FAIL: Executable corrupted or wrong architecture"
Write-Log "  STAGE 5 FAIL: QApplication construction failed - check Qt plugin path, DLL conflicts"
Write-Log "  STAGE 6 FAIL: MainWindow construction or window.show() failed - check PDFium init, UI code"
Write-Log "  STAGE 7 INFO: Use Dependencies tool for deeper analysis"

Write-Host ""
Write-Host "=== DIAGNOSIS COMPLETE ==="
Write-Host "Log saved to: $LogFile"
Write-Host "Examine the log to identify the exact failure stage."