if (-NOT ((Test-Path '.\ExternalLibs\FbxSDK\x64\debug\libfbxsdk.lib') -AND (Test-Path '.\ExternalLibs\FbxSDK\x64\release\libfbxsdk.lib') -AND (Test-Path '.\ExternalLibs\FbxSDK\x86\release\libfbxsdk.lib')  -AND (Test-Path '.\ExternalLibs\FbxSDK\x86\debug\libfbxsdk.lib'))) {
"Required binaries not found, downloading..."
(New-Object Net.WebClient).DownloadFile('https://github.com/csyonghe/SpireMiniEngineExtBinaries/raw/master/binaries.zip', 'binaries.zip')
Add-Type -assembly "system.io.compression.filesystem"
if (Test-Path 'C:\Projects\SpireMiniEngine\ExternalLibs') {
Remove-Item -Recurse -Force 'ExternalLibs'
}
[System.IO.Compression.ZipFile]::ExtractToDirectory("binaries.zip", '.\')
Remove-Item 'binaries.zip'
}
"Done."
cmd /c pause | out-null