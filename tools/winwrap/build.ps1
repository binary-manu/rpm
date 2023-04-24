$here = Get-Location

subst.exe S: $here/../../src
subst.exe T: $here/../../tools

$oldPath = $Env:Path

$Env:Path += ";T:\winwrap;T:\BC5\BIN"
S:
make > S:\BUILD.LOG
C:

$Env:Path = $oldpath
subst S: /D
subst T: /D
