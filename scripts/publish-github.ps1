param(
    [ValidateSet("public", "private")]
    [string]$Visibility = "public",
    [string]$RepositoryName = "Bellows-VST3"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Find-Gh {
    $command = Get-Command gh -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $installed = Join-Path $env:ProgramFiles "GitHub CLI\gh.exe"
    if (Test-Path $installed) { return $installed }

    if (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Host "GitHub CLI is missing. Installing it with winget..."
        winget install --id GitHub.cli --exact --accept-package-agreements --accept-source-agreements
        if (Test-Path $installed) { return $installed }
    }

    throw "GitHub CLI is required. Install it from https://cli.github.com/ and run this script again."
}

$Gh = Find-Gh

& $Gh auth status *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Sign in to GitHub in the browser window that opens."
    & $Gh auth login --web --git-protocol https
    if ($LASTEXITCODE -ne 0) {
        throw "GitHub authentication did not complete."
    }
}

& $Gh auth status
$Login = (& $Gh api user --jq .login).Trim()
$FullName = "$Login/$RepositoryName"

& $Gh repo view $FullName *> $null
if ($LASTEXITCODE -eq 0) {
    throw "Repository $FullName already exists. Choose another -RepositoryName value."
}

if (-not (Test-Path ".git")) {
    git init -b main
}

if (-not (git config user.name)) {
    git config user.name $Login
}
if (-not (git config user.email)) {
    git config user.email "$Login@users.noreply.github.com"
}

git add .
$hasCommit = git rev-parse --verify HEAD 2>$null
if (-not $hasCommit) {
    git commit -m "Create Bellows accordion VST3 prototype"
} elseif (git status --porcelain) {
    git commit -m "Update Bellows accordion VST3 prototype"
}

$VisibilityFlag = "--$Visibility"
& $Gh repo create $FullName `
    $VisibilityFlag `
    --source . `
    --remote origin `
    --push `
    --description "Expressive physically inspired accordion VST3 built with C++ and JUCE"

$Url = (& $Gh repo view $FullName --json url --jq .url).Trim()
Write-Host ""
Write-Host "Published: $Url"
Write-Host "GitHub Actions will build the Windows VST3 and attach it as a workflow artifact."
