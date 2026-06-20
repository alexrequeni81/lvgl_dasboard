param(
    [string]$Panel = "tc@192.168.1.246",
    [string]$ZipUrl = "https://nightly.link/alexrequeni81/lvgl_dasboard/workflows/build.yml/main/dashboard_app-linux-i686.zip"
)

Write-Host "==> Descargando artifact..." -ForegroundColor Cyan
$tmp = "$env:TEMP\dashboard_app"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$zip = "$tmp\dashboard_app.zip"

curl.exe -sL -o $zip $ZipUrl
if (!(Test-Path $zip)) {
    Write-Host "ERROR: No se pudo descargar el artifact" -ForegroundColor Red
    exit 1
}

Write-Host "==> Extrayendo..." -ForegroundColor Cyan
tar -xf $zip -C $tmp
$bin = Get-ChildItem -Path $tmp -Recurse -File | Where-Object { $_.Length -gt 1MB } | Select-Object -First 1
if (!$bin) {
    Write-Host "ERROR: No se encontro el binario en el zip" -ForegroundColor Red
    exit 1
}

Write-Host "==> Matando app en el panel..." -ForegroundColor Cyan
ssh $Panel "killall dashboard_app 2>/dev/null; sleep 1; echo '[OK] Detenida'"

Write-Host "==> Subiendo nuevo binario a $Panel..." -ForegroundColor Cyan
scp $bin.FullName "${Panel}:/home/tc/dashboard_app"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: SCP fallo" -ForegroundColor Red
    exit 1
}

Write-Host "==> Reiniciando en el panel..." -ForegroundColor Cyan
ssh $Panel @'
    chmod +x /home/tc/dashboard_app
    nohup /home/tc/dashboard_app </dev/null >/dev/null 2>&1 &
    echo "[OK] App reiniciada"
'@

Write-Host "==> Listo!" -ForegroundColor Green
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
