#requires -version 5.0
param(
    [string]$TargetDir = $null
)
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $TargetDir) { $TargetDir = $ScriptDir }

$Deark = Join-Path $ScriptDir "deark.exe"

function Write-Title($text) {
    Write-Host ""
    Write-Host "== $text ==" -ForegroundColor Cyan
}

function Find-7Zip {
    $candidates = @(
        "$env:ProgramFiles\7-Zip\7z.exe",
        "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    $cmd = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

Write-Host ""
Write-Host "  Johnny Castaway - Ficheiros do jogo original" -ForegroundColor Yellow
Write-Host "  -----------------------------------------------"
Write-Host "  RESOURCE.MAP e RESOURCE.001 sao propriedade da Sierra/Dynamix"
Write-Host "  e nao vem incluidos neste instalador."
Write-Host ""
Write-Host "  Se ainda nao tens a imagem da disquete original, descarrega-a"
Write-Host "  por exemplo em: https://archive.org/details/000580-ScreenAnticsJohnnyCastaway"
Write-Host ""
Write-Host "  Podes indicar aqui:"
Write-Host "   - um ficheiro .7z (como descarregado do Archive.org)"
Write-Host "   - um ficheiro .img (imagem de disquete, ja extraida)"
Write-Host "   - uma pasta que ja contenha RESOURCE.MAP e RESOURCE.00`$"
Write-Host ""
Write-Host "  (Podes fechar esta janela e fazer isto mais tarde a partir do" -ForegroundColor DarkGray
Write-Host "   atalho 'Obter ficheiros do jogo original' no Menu Iniciar.)" -ForegroundColor DarkGray
Write-Host ""

if (-not (Test-Path $Deark)) {
    Write-Host "ERRO: deark.exe nao encontrado ao lado deste script." -ForegroundColor Red
    exit 1
}

$srcPath = Read-Host "Caminho do ficheiro/pasta (ou deixa vazio para saltar)"
$srcPath = $srcPath.Trim('"').Trim()

if ($srcPath -eq "") {
    Write-Host "Saltado. Podes voltar a correr isto mais tarde pelo Menu Iniciar." -ForegroundColor Yellow
    exit 0
}

if (-not (Test-Path $srcPath)) {
    Write-Host "ERRO: '$srcPath' nao existe." -ForegroundColor Red
    exit 1
}

$work = Join-Path $env:TEMP ("jc_install_" + [guid]::NewGuid().ToString("N").Substring(0,8))
New-Item -ItemType Directory -Path $work | Out-Null

try {
    $imgPath = $null
    $item = Get-Item $srcPath

    if ($item.PSIsContainer) {
        $direct = Join-Path $srcPath "RESOURCE.MAP"
        if (Test-Path $direct) {
            $imgPath = $null
            $floppyDir = $srcPath
        }
        else {
            $found7z = Get-ChildItem $srcPath -Filter *.7z -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            $foundImg = Get-ChildItem $srcPath -Filter *.img -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($foundImg) { $srcPath = $foundImg.FullName; $item = Get-Item $srcPath }
            elseif ($found7z) { $srcPath = $found7z.FullName; $item = Get-Item $srcPath }
            else {
                Write-Host "ERRO: nao encontrei RESOURCE.MAP, nem .img, nem .7z dentro dessa pasta." -ForegroundColor Red
                exit 1
            }
        }
    }

    if (-not $floppyDir) {
        if ($item.Extension -ieq ".7z") {
            Write-Title "A extrair o .7z"
            $sevenZip = Find-7Zip
            if (-not $sevenZip) {
                Write-Host "ERRO: precisas do 7-Zip instalado para abrir ficheiros .7z." -ForegroundColor Red
                Write-Host "Instala-o gratuitamente em https://www.7-zip.org/ e volta a tentar," -ForegroundColor Red
                Write-Host "ou extrai o .7z manualmente e aponta este instalador para o .img resultante." -ForegroundColor Red
                exit 1
            }
            & $sevenZip x -y "-o$work" $srcPath | Out-Null
            $imgFile = Get-ChildItem $work -Filter *.img -Recurse | Select-Object -First 1
            if (-not $imgFile) {
                Write-Host "ERRO: o .7z nao continha nenhum ficheiro .img." -ForegroundColor Red
                exit 1
            }
            $imgPath = $imgFile.FullName
        }
        elseif ($item.Extension -ieq ".img") {
            $imgPath = $item.FullName
        }
        else {
            Write-Host "ERRO: tipo de ficheiro nao reconhecido ($($item.Extension)). Usa .7z, .img, ou uma pasta." -ForegroundColor Red
            exit 1
        }

        Write-Title "A ler a imagem da disquete (FAT12)"
        $floppyDir = Join-Path $work "floppy"
        New-Item -ItemType Directory -Path $floppyDir | Out-Null
        & $Deark -m fat -od $floppyDir $imgPath | Out-Null

        Get-ChildItem $floppyDir | ForEach-Object {
            $newName = $_.Name -replace '^output\.\d+\.', ''
            if ($newName -ne $_.Name) {
                Rename-Item $_.FullName $newName
            }
        }
    }

    $resourceMap = Join-Path $floppyDir "RESOURCE.MAP"
    $resourceComp = Join-Path $floppyDir 'RESOURCE.00$'

    if (-not (Test-Path $resourceMap) -or -not (Test-Path $resourceComp)) {
        Write-Host "ERRO: nao encontrei RESOURCE.MAP / RESOURCE.00`$ nos dados fornecidos." -ForegroundColor Red
        exit 1
    }

    Write-Title "A descomprimir RESOURCE.001 (formato TSComp)"
    $decompDir = Join-Path $work "decomp"
    New-Item -ItemType Directory -Path $decompDir | Out-Null
    & $Deark -m tscomp -od $decompDir $resourceComp | Out-Null

    $resource001 = Get-ChildItem $decompDir -Filter "*RESOURCE.001" | Select-Object -First 1
    if (-not $resource001) {
        Write-Host "ERRO: a descompressao do RESOURCE.00`$ falhou." -ForegroundColor Red
        exit 1
    }

    Copy-Item $resourceMap (Join-Path $TargetDir "RESOURCE.MAP") -Force
    Copy-Item $resource001.FullName (Join-Path $TargetDir "RESOURCE.001") -Force

    $mapSize = (Get-Item (Join-Path $TargetDir "RESOURCE.MAP")).Length
    $resSize = (Get-Item (Join-Path $TargetDir "RESOURCE.001")).Length

    Write-Title "Concluido"
    Write-Host "RESOURCE.MAP : $mapSize bytes (esperado: 1461)"
    Write-Host "RESOURCE.001 : $resSize bytes (esperado: 1175645)"

    if ($mapSize -ne 1461 -or $resSize -ne 1175645) {
        Write-Host ""
        Write-Host "AVISO: os tamanhos nao batem certo com o esperado. O jogo pode nao funcionar bem." -ForegroundColor Yellow
    }
    else {
        Write-Host ""
        Write-Host "Tudo certo!" -ForegroundColor Green
    }

    Write-Host ""
    $wantSound = Read-Host "Queres tambem os sons originais? Sao descarregados do repositorio publico JCOS no GitHub (s/n)"
    if ($wantSound -match '^[sSyY]') {
        Write-Title "A descarregar sons do JCOS (github.com/nivs1978/Johnny-Castaway-Open-Source)"
        $soundIndexes = @(0,1,2,3,4,5,6,7,8,9,10,12,14,15,16,17,18,19,20,21,22,23,24)
        $baseUrl = "https://raw.githubusercontent.com/nivs1978/Johnny-Castaway-Open-Source/master/JCOS/Resources"
        $ok = 0
        foreach ($i in $soundIndexes) {
            $name = "sound$i.wav"
            try {
                Invoke-WebRequest -Uri "$baseUrl/$name" -OutFile (Join-Path $TargetDir $name) -UseBasicParsing
                $ok++
            }
            catch {
                Write-Host "  falhou: $name" -ForegroundColor Yellow
            }
        }
        Write-Host "$ok / $($soundIndexes.Count) sons descarregados." -ForegroundColor Green
    }

    Write-Host ""
    Write-Host "Podes fechar esta janela." -ForegroundColor Cyan
    Start-Sleep -Seconds 2
}
finally {
    Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
}
