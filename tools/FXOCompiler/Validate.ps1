param(
    [Parameter(Mandatory=$true)][string]$Compiler,
    [Parameter(Mandatory=$true)][string]$Manifest
)
$ErrorActionPreference = 'Stop'
$compilerPath = (Resolve-Path -LiteralPath $Compiler).Path
$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$manifestBase = Split-Path -Parent $manifestPath
$manifestText = Get-Content -LiteralPath $manifestPath -Raw
$firstJob = ($manifestText -split '\[fxo\]')[1] -split '\[' | Select-Object -First 1
$templateRelative = [regex]::Match($firstJob, '(?m)^template=(.+)').Groups[1].Value.Trim()
$templatePath = (Resolve-Path -LiteralPath (Join-Path $manifestBase $templateRelative)).Path
$templateHash = (Get-FileHash -LiteralPath $templatePath -Algorithm SHA256).Hash.ToLowerInvariant()
$testRoot = Join-Path (Split-Path -Parent $compilerPath) 'validation'
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
function Run-Case([string]$Name, [string]$Shader, [string]$Hash, [bool]$ExpectSuccess, [string]$ExpectedError) {
    $caseRoot = Join-Path $testRoot $Name
    New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
    $sourcePath = Join-Path $caseRoot 'test.hlsl'
    $outputPath = Join-Path $caseRoot 'output.fxo_pc'
    $caseManifest = Join-Path $caseRoot 'build.ini'
    Set-Content -LiteralPath $sourcePath -Value $Shader -Encoding utf8
    $caseText = "[fxo]`ntemplate=$templatePath`nsha256=$Hash`noutput=$outputPath`nps=0|$sourcePath|PS_Main`n"
    Set-Content -LiteralPath $caseManifest -Value $caseText -Encoding utf8
    # UTF-8 without BOM: matches checked-in manifests and supported CLI input.
    [IO.File]::WriteAllText($caseManifest, $caseText, [Text.UTF8Encoding]::new($false))
    $log = & $compilerPath build $caseManifest 2>&1
    $result = $LASTEXITCODE
    $log | Set-Content -LiteralPath (Join-Path $caseRoot 'compiler.log')
    if (($result -eq 0) -ne $ExpectSuccess) { throw "$Name returned $result`n$log" }
    if (-not $ExpectSuccess -and ($log -join "`n") -notmatch $ExpectedError) { throw "$Name failed for an unexpected reason`n$log" }
    if ($ExpectSuccess -and (Get-Item -LiteralPath $outputPath).Length -ne (Get-Item -LiteralPath $templatePath).Length) { throw "$Name changed the FXO size." }
    if (-not $ExpectSuccess -and (Test-Path -LiteralPath $outputPath)) { throw "$Name wrote invalid output." }
    Write-Output "$Name passed"
}
$plain = 'float4 PS_Main(float2 uv:TEXCOORD0):COLOR0 {return float4(uv,.5,1);}'
Run-Case 'matching-template' $plain $templateHash $true ''
Run-Case 'wrong-template-hash' $plain ('0'*64) $false 'Template hash mismatch'
Run-Case 'invalid-hlsl' 'this is not HLSL' $templateHash $false 'failed'
Run-Case 'unbound-constant' 'float4 data:register(c220);float4 PS_Main(float2 uv:TEXCOORD0):COLOR0{return data;}' $templateHash $false 'Unbound new external shader constant'
Run-Case 'incompatible-input' 'float4 PS_Main(float2 uv:TEXCOORD8):COLOR0{return float4(uv,.5,1);}' $templateHash $false 'incompatible input semantic'
Run-Case 'incompatible-sampler' 'samplerCUBE data:register(s0);float4 PS_Main(float2 uv:TEXCOORD0):COLOR0{return texCUBE(data,float3(uv,1));}' $templateHash $false 'incompatible sampler binding'
Write-Output 'Standalone native FXO compiler validation passed.'

$global:LASTEXITCODE = 0
