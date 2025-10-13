$currentDir = Get-Location
if (Test-Path build) {
    Remove-Item -Recurse -Force build
}
New-Item -ItemType Directory -Path build

Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\2022\Community" -DevCmdArguments "-arch=amd64 -host_arch=amd64"

Set-Location $currentDir

cl "$currentDir\assn1.cpp" /EHsc /std:c++17 /D "NDEBUG" `
	/I "$currentDir\include" `
	/Fo"$currentDir\build\assn1.obj" `
	/Fe"$currentDir\build\build.exe" `
	/Fd"$currentDir\build\vc.pdb" `
	/link /LIBPATH:"$currentDir\lib" freeglut.lib glew32.lib opengl32.lib

$env:Path = $env:Path + ";$currentDir\bin"

Write-Host "Execution fild build successfully. Running..."
& "$currentDir\build\build.exe"
Write-Host "Execution exited."