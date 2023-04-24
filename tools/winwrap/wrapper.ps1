$DosBoxConf = New-TemporaryFile
$Env:TMP = "S:\"
$DosBoxOutput = New-TemporaryFile

@"
[autoexec]
mount s s:\
mount t t:\
S:
T:\BC5\BIN\$args > $DosBoxOutput
exit
"@ | Out-File -FilePath $DosBoxConf.FullName -Encoding ASCII

Start-Process -Wait -FilePath dosbox -ArgumentList "-conf", $DosBoxConf.FullName

Get-Content $DosBoxOutput

$DosBoxConf.Delete()
$DosBoxOutput.Delete()
