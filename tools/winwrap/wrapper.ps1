$DosBoxConf = New-TemporaryFile
$Env:TMP = "S:\"
$DosBoxOutput = New-TemporaryFile
$DosBoxResult = New-TemporaryFile

@"
[autoexec]
set EC=0
mount s s:\
mount t t:\
S:
T:\BC5\BIN\$args > $DosBoxOutput
if ERRORLEVEL 1 set EC=1
echo %EC% > $DosBoxResult
exit
"@ | Out-File -FilePath $DosBoxConf.FullName -Encoding ASCII

Start-Process -Wait -FilePath dosbox -ArgumentList "-conf", $DosBoxConf.FullName, "-noconsole"

Get-Content $DosBoxOutput
$ExitCode = Get-Content $DosBoxResult

$DosBoxConf.Delete()
$DosBoxOutput.Delete()
$DosBoxResult.Delete()

exit [int]$ExitCode
