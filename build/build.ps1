#############################
# Cleanup building terminal #
#############################
Clear-Host;

########################################################
# Required download locations for 3rd party components #
########################################################
$ProgressPreference = 'SilentlyContinue' 
$url_dosboxx = "https://github.com/joncampbell123/dosbox-x/releases/download/dosbox-x-v2023.03.31/dosbox-x-vsbuild-win64-20230401023040.zip";
$url_pkzip = "http://cd.textfiles.com/darkdomain/programs/dos/archivers/pkz204g.exe";
$url_pklite = "http://cd.textfiles.com/darkdomain/programs/dos/archivers/pklts201.exe";
$url_unxutils = "https://deac-ams.dl.sourceforge.net/project/unxutils/unxutils/current/UnxUtils.zip";

#######################
# Installing DOSBox-X #
#######################
Write-Host "- Installing DOSBox-X";
$file_dosboxx = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + $(Split-Path -Path $url_dosboxx -Leaf));
$folder_dosboxx = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools','dosbox-x'));
if(-not(Test-path $folder_dosboxx -PathType Container)) {
	Invoke-WebRequest -Uri $url_dosboxx -OutFile $file_dosboxx;
	$zip_dosboxx = [IO.Compression.ZipFile]::OpenRead($file_dosboxx);
	$zip_dosboxx.Entries | Where-Object { $_.FullName -like "$('bin/x64/Release SDL2' -replace '\\','/')/*.*" } | ForEach-Object {
		$file = $folder_dosboxx + ($_.FullName -replace 'bin/x64/Release SDL2/','/');
		$parent = Split-Path -Parent $file;
		if (-not (Test-Path -LiteralPath $parent)) { New-Item -Path $parent -Type Directory | Out-Null; }
		[IO.Compression.ZipFileExtensions]::ExtractToFile($_, $file, $true);
	}
	$zip_dosboxx.Dispose();
	if(Test-path $file_dosboxx) { Remove-Item $file_dosboxx; }
}

####################
# Installing PKZip #
####################
Write-Host "- Installing PKZip";
$file_pkzip = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + $(Split-Path -Path $url_pkzip -Leaf));
$folder_pkzip = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools','pkzip'));
if(-not(Test-path $folder_pkzip -PathType Container)) {
	Invoke-WebRequest -Uri $url_pkzip -OutFile $file_pkzip;
	$zip_pkzip = [IO.Compression.ZipFile]::OpenRead($file_pkzip);
	$zip_pkzip.Entries | Where-Object { $_.FullName -like "*.*" } | ForEach-Object {
		$file = $folder_pkzip + '/' + $_.FullName;
		$parent = Split-Path -Parent $file;
		if (-not (Test-Path -LiteralPath $parent)) { New-Item -Path $parent -Type Directory | Out-Null; }
		[IO.Compression.ZipFileExtensions]::ExtractToFile($_, $file, $true);
	}
	$zip_pkzip.Dispose();
	if(Test-path $file_pkzip) { Remove-Item $file_pkzip; }
}
 
#####################
# Installing PKLite #
#####################
Write-Host "- Installing PKLite";
$file_pklite = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + $(Split-Path -Path $url_pklite -Leaf));
$folder_pklite = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools','pklite'));
if(-not(Test-path $folder_pklite -PathType Container)) {
	Invoke-WebRequest -Uri $url_pklite -OutFile $file_pklite;
	$zip_pklite = [IO.Compression.ZipFile]::OpenRead($file_pklite);
	$zip_pklite.Entries | Where-Object { $_.FullName -like "*.*" } | ForEach-Object {
		$file = $folder_pklite + '/' + $_.FullName;
		$parent = Split-Path -Parent $file;
		if (-not (Test-Path -LiteralPath $parent)) { New-Item -Path $parent -Type Directory | Out-Null; }
		[IO.Compression.ZipFileExtensions]::ExtractToFile($_, $file, $true);
	}
	$zip_pklite.Dispose();
	if(Test-path $file_pklite) { Remove-Item $file_pklite; }
}

###################################
# Installing "UnxUtils for Win32" #
###################################
Write-Host "- Installing UnxUtils for Win32";
$file_unxutils = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + $(Split-Path -Path $url_unxutils -Leaf));
$folder_unxutils = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools','unxutils'));
if(-not(Test-path $folder_unxutils -PathType Container)) {
	Invoke-WebRequest -Uri $url_unxutils -OutFile $file_unxutils;
	$zip_unxutils = [IO.Compression.ZipFile]::OpenRead($file_unxutils);
	$zip_unxutils.Entries | Where-Object { $_.FullName -like "$('usr/local/wbin' -replace '\\','/')/*.*" } | ForEach-Object {
		$file = $folder_unxutils + ($_.FullName -replace 'usr/local/wbin/','/');
		$parent = Split-Path -Parent $file;
		if (-not (Test-Path -LiteralPath $parent)) { New-Item -Path $parent -Type Directory | Out-Null; }
		[IO.Compression.ZipFileExtensions]::ExtractToFile($_, $file, $true);
	}
	$zip_unxutils.Dispose();
	if(Test-path $file_unxutils) { Remove-Item $file_unxutils; }
}

###############################
# Resetting build environment #
###############################
Write-Host "- Resetting build environment";
Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) *.zip | ForEach-Object { Remove-Item -Path $_.FullName }
Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) *.exe | ForEach-Object { Remove-Item -Path $_.FullName }
Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) *.org | ForEach-Object { Remove-Item -Path $_.FullName }
Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) *.log | ForEach-Object { Remove-Item -Path $_.FullName }
Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) xrpart*.xxf | ForEach-Object { Remove-Item -Path $_.FullName }
$file_buildlog = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\build.log');
if(Test-path $file_buildlog) { Remove-Item $file_buildlog; }
New-Item $file_buildlog | Out-Null;

##########################
# Starting build process #
##########################
Write-Host "- Starting build process";
$process_dosboxx = Start-Process -NoNewWindow -WorkingDirectory $(Get-Item $PSScriptRoot) -FilePath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\dosbox-x\dosbox-x.exe')) -ArgumentList "-conf",$([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\dosbox-x.conf')),"-fastlaunch","-log-con","-silent","-c ""T:\BUILD.BAT""" -PassThru;
$process_dosboxx_instances = 1;

try {
	Write-Host "- Starting logging job";
	$job_logging = Start-Job -Name "DOSBox-X Logging" -ArgumentList $file_buildlog -ScriptBlock { param($localfile_buildlog); Get-Content $localfile_buildlog -Wait | ForEach-Object { Write-Host $($_ -replace '[^\x20-\x7E]+', '');  if ($_ -Match "--END.OF.LOG--" -or $_ -Match "--PROCESS.ENDED/TERMINATED--") { break; } } };

	Write-Host "----------------------------------------------------------------------------------------------------";
	while ($null -ne $process_dosboxx -and $process_dosboxx_instances -gt 0) {
		try
		{
			if ($process_dosboxx.HasExited -ne $true) {
				$process_dosboxx = (Get-Process -inputObject $process_dosboxx -ErrorAction SilentlyContinue);
				if ($null -ne $process_dosboxx) { $process_dosboxx_instances = $process_dosboxx.count; }
			}
			else { $process_dosboxx = $null; }

			Receive-Job $job_logging;
		}
		catch { 
			$process_dosboxx = $null;
		}
	}
	Add-Content $file_buildlog -Value "`r`n--PROCESS ENDED/TERMINATED--`r`n";
	while ($job_logging.HasMoreData) { 
		Receive-Job $job_logging; 
	}
	Write-Host "----------------------------------------------------------------------------------------------------";
}
finally
{
	###################
	# Releasing files #
	###################
	Write-Host "- Releasing files";
	Move-Item -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'src') + '\part.exe')) -Destination $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\')) -Force -ErrorAction SilentlyContinue;
	Move-Item -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'src') + '\part.org')) -Destination $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\')) -Force -ErrorAction SilentlyContinue;
	Move-Item -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'src') + '\sources.zip')) -Destination $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\')) -Force -ErrorAction SilentlyContinue;
	Move-Item -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'src') + '\release.zip')) -Destination $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\')) -Force -ErrorAction SilentlyContinue;

	##############################
	# Preparing release for XOSL #
	##############################
	Write-Host "- Preparing release for XOSL";
	if (Test-Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\part.org'))) {
		$process_split = Start-Process -NoNewWindow -WorkingDirectory $(Get-Item $PSScriptRoot) -FilePath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\unxutils\split.exe')) -ArgumentList "-b","32768",$([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\part.org')),'XRPART' -PassThru;
		$process_split.WaitForExit();
	}
	[ref]$i = 0; Get-ChildItem -Path $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build'))) XRPART* | Rename-Item -NewName { '{0}{1:d2}{2}' -f "XRPART", $i.value++, ".XXF" };

	#########################
	# Cleaning up processes #
	#########################
	Write-Host "- Stopping process(es)";
	try { $process_dosboxx = (Get-Process -inputObject $process_dosboxx -ErrorAction SilentlyContinue); } catch { $process_dosboxx = $null; } finally { if ($null -ne $process_dosboxx) { Stop-Process -inputObject $process_dosboxx; } }
	try { $process_split = (Get-Process -inputObject $process_split -ErrorAction SilentlyContinue); } catch { $process_split = $null; } finally { if ($null -ne $process_split) { Stop-Process -inputObject $process_split; } }
}

####################
# Cleaning up jobs #
####################
Write-Host "- Stopping logging job(s)";
Stop-Job -Name "DOSBox-X Logging";
Remove-Job -Name "DOSBox-X Logging";