param(
    [Parameter(Mandatory = $true)]
    [string]$Port,
    [string]$Fqbn = "esp32:esp32:esp32c3",
    [int]$WebPort = 5501
)

$ErrorActionPreference = "Stop"
$workspace = Split-Path -Parent $PSScriptRoot
$dataPath = Join-Path $workspace "data"
$sketchPath = Join-Path $workspace "Reciver_WebSocket_dashboard.ino"
$url = "http://localhost:$WebPort/preview.html"

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    throw "arduino-cli was not found. Install Arduino CLI and add it to PATH."
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python was not found. Install Python to serve the preview."
}

$server = Start-Process -FilePath $python.Source `
    -ArgumentList "-m", "http.server", $WebPort, "--bind", "0.0.0.0", "--directory", $dataPath `
    -PassThru

try {
    Write-Host "Compiling $sketchPath ..."
    & arduino-cli compile --fqbn $Fqbn $sketchPath
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed."
    }

    Write-Host "Uploading to $Port ..."
    & arduino-cli upload --fqbn $Fqbn --port $Port $sketchPath
    if ($LASTEXITCODE -ne 0) {
        throw "Upload failed."
    }

    Start-Process $url
    $ipv4 = (Get-NetIPAddress -AddressFamily IPv4 -PrefixOrigin Dhcp,Manual |
        Where-Object { $_.IPAddress -notlike "127.*" -and $_.IPAddress -notlike "169.254.*" } |
        Select-Object -First 1 -ExpandProperty IPAddress)
    Write-Host "Preview opened locally: $url"
    if ($ipv4) {
        Write-Host "Open from another device: http://${ipv4}:$WebPort/preview.html"
    }
    Write-Host "The preview server stays running in the background. Stop the Python process when finished."
}
catch {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force
    }
    throw
}
