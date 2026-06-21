$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$activePaths = @(
    "$root/lib/M360-DRY/src",
    "$root/src/DRY/gateway/ngm",
    "$root/src/DRY/gateway/withLibDRY",
    "$root/src/DRY/nos"
)

$legacyCommand = Get-ChildItem $activePaths -Recurse -Include *.h,*.cpp |
    Select-String -SimpleMatch 'teste_do_gateway'
if ($legacyCommand) {
    throw "Comando legado encontrado: $($legacyCommand.Path):$($legacyCommand.LineNumber)"
}

$credentials = Join-Path $root 'include/M360Credentials.h'
$template = Join-Path $root 'include/M360Credentials.h.example'
if (-not (Test-Path $credentials)) {
    throw 'Crie include/M360Credentials.h a partir do template.'
}
if (-not (Test-Path $template)) {
    throw 'Template include/M360Credentials.h.example ausente.'
}

$ignored = git -C $root check-ignore 'include/M360Credentials.h'
if ($LASTEXITCODE -ne 0 -or -not $ignored) {
    throw 'include/M360Credentials.h precisa estar protegido pelo .gitignore.'
}

$required = @(
    'M360_AP_SSID', 'M360_AP_PASSWORD', 'M360_AP_IP_OCTETS',
    'M360_STA_SSID', 'M360_STA_PASSWORD', 'M360_MQTT_SERVER',
    'M360_MQTT_PORT', 'M360_MQTT_USER', 'M360_MQTT_PASSWORD',
    'M360_UF', 'M360_CAR_NUMBER'
)
$templateContent = Get-Content -Raw $template
foreach ($name in $required) {
    if ($templateContent -notmatch "#define\s+$name\b") {
        throw "Constante ausente no template: $name"
    }
}

Write-Output 'Contratos M360 validados.'
