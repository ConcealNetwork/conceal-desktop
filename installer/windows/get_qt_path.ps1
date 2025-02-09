# Read CMakeLists.txt
$content = Get-Content "../../CMakeLists.txt" -Raw

# Extract Qt path using regex
$pattern = 'set\(CMAKE_PREFIX_PATH\s+"([^"]+)"'
if ($content -match $pattern) {
    $qtPath = $matches[1]
    # Convert to proper path format and remove \lib\cmake\
    $qtPath = $qtPath -replace '\\\\', '\' -replace '\\lib\\cmake\\$', ''
    
    # Output in INI format
    Write-Output "[Path]`nQtDir=$qtPath"
} else {
    Write-Error "Qt path not found in CMakeLists.txt"
    exit 1
} 