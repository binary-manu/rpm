$DosBoxConf = New-TemporaryFile

@"
[autoexec]
mount s s:\
mount t t:\
S:
T:\BC5\BIN\${args}
exit
"@ | Out-File -FilePath $DosBoxConf.FullName -Encoding ASCII

Start-Process -Wait -FilePath dosbox -ArgumentList "-conf", $DosBoxConf.FullName

$DosBoxConf.Delete()
