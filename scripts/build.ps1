[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [switch]$RunTests
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $repoRoot 'build'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

$sdlRoot = Join-Path $repoRoot 'SDL2-devel-2.30.11-mingw/SDL2-2.30.11/x86_64-w64-mingw32'
$imageRoot = Join-Path $repoRoot 'SDL2_image-devel-2.8.4-mingw/SDL2_image-2.8.4/x86_64-w64-mingw32'
$mixerRoot = Join-Path $repoRoot 'SDL2_mixer-devel-2.8.0-mingw/SDL2_mixer-2.8.0/x86_64-w64-mingw32'
$ttfRoot = Join-Path $repoRoot 'SDL2_ttf-devel-2.24.0-mingw/SDL2_ttf-2.24.0/x86_64-w64-mingw32'

$commonArguments = @(
    '-std=c11',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wshadow',
    '-Wconversion',
    '-Wformat=2',
    '-I', (Join-Path $sdlRoot 'include'),
    '-I', (Join-Path $sdlRoot 'include/SDL2'),
    '-I', (Join-Path $imageRoot 'include'),
    '-I', (Join-Path $mixerRoot 'include'),
    '-I', (Join-Path $ttfRoot 'include'),
    '-I', $repoRoot,
    '-L', (Join-Path $sdlRoot 'lib'),
    '-L', (Join-Path $imageRoot 'lib'),
    '-L', (Join-Path $mixerRoot 'lib'),
    '-L', (Join-Path $ttfRoot 'lib')
)

if ($Configuration -eq 'Debug') {
    $commonArguments += @('-O0', '-g3')
} else {
    $commonArguments += @('-O2', '-DNDEBUG')
}

$libraries = @(
    '-lmingw32',
    '-lSDL2main',
    '-lSDL2',
    '-lSDL2_image',
    '-lSDL2_mixer',
    '-lSDL2_ttf'
)

$appSources = @(
    'Pipboy3000.c',
    'animations.c',
    'core.c',
    'events.c',
    'input.c',
    'inventory.c',
    'pipboy.c',
    'render.c',
    'resources.c',
    'save.c',
    'state.c',
    'ui.c',
    'video.c',
    'MAP/map.c',
    'third_party/cjson/cjson_impl.c'
) | ForEach-Object { Join-Path $repoRoot $_ }

$appOutput = Join-Path $buildDirectory 'PipBoy3000.exe'
& gcc @commonArguments '-o' $appOutput @appSources @libraries '-mwindows'
if ($LASTEXITCODE -ne 0) {
    throw "PipBoy3000 build failed with exit code $LASTEXITCODE."
}

$testSources = @(
    'tests/core_tests.c',
    'events.c',
    'core.c',
    'input.c',
    'inventory.c',
    'save.c',
    'state.c',
    'MAP/map.c',
    'third_party/cjson/cjson_impl.c'
) | ForEach-Object { Join-Path $repoRoot $_ }

$testOutput = Join-Path $buildDirectory 'pipboy_core_tests.exe'
& gcc @commonArguments '-DSDL_MAIN_HANDLED' '-o' $testOutput @testSources @libraries
if ($LASTEXITCODE -ne 0) {
    throw "Core test build failed with exit code $LASTEXITCODE."
}

$videoTestSources = @(
    'tests/video_tests.c',
    'video.c'
) | ForEach-Object { Join-Path $repoRoot $_ }

$videoTestOutput = Join-Path $buildDirectory 'pipboy_video_tests.exe'
& gcc @commonArguments '-DSDL_MAIN_HANDLED' '-o' $videoTestOutput @videoTestSources @libraries
if ($LASTEXITCODE -ne 0) {
    throw "Video test build failed with exit code $LASTEXITCODE."
}

foreach ($runtimeLibrary in @('SDL2.dll', 'SDL2_image.dll', 'SDL2_mixer.dll', 'SDL2_ttf.dll')) {
    Copy-Item -LiteralPath (Join-Path $repoRoot $runtimeLibrary) -Destination $buildDirectory -Force
}

Write-Host "Built $appOutput"
Write-Host "Built $testOutput"
Write-Host "Built $videoTestOutput"

if ($RunTests) {
    Push-Location $repoRoot
    try {
        & $testOutput
        if ($LASTEXITCODE -ne 0) {
            throw "Core tests failed with exit code $LASTEXITCODE."
        }
        Write-Host 'Core tests passed.'

        $previousVideoDriver = $env:SDL_VIDEODRIVER
        $previousAudioDriver = $env:SDL_AUDIODRIVER
        try {
            $env:SDL_VIDEODRIVER = 'dummy'
            $env:SDL_AUDIODRIVER = 'dummy'
            & $videoTestOutput
            if ($LASTEXITCODE -ne 0) {
                throw "Video tests failed with exit code $LASTEXITCODE."
            }
            Write-Host 'Video tests passed.'
        } finally {
            $env:SDL_VIDEODRIVER = $previousVideoDriver
            $env:SDL_AUDIODRIVER = $previousAudioDriver
        }
    } finally {
        Pop-Location
    }
}
