#############################
# Cleanup building terminal #
#############################
Clear-Host;

########################################################
# Required download locations for 3rd party components #
########################################################
$ProgressPreference = 'SilentlyContinue' 
$url_dosboxx = "https://github.com/joncampbell123/dosbox-x/releases/download/dosbox-x-v2023.03.31/dosbox-x-vsbuild-win64-20230401023040.zip";

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

####################################
# Starting cleaning source process #
####################################
Write-Host "- Cleaning up source";
$process_dosboxx = Start-Process -NoNewWindow -WorkingDirectory $(Get-Item $PSScriptRoot) -FilePath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\dosbox-x\dosbox-x.exe')) -ArgumentList "-conf",$([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\dosbox-x.conf')),"-fastlaunch","-log-con","-silent","-c ""T:\CLEAN.BAT""" -PassThru;
$process_dosboxx_instances = 1;

try {
	Write-Host "- Starting logging job";
	$file_buildlog = [IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'build') + '\build.log');
	if(Test-path $file_buildlog) { Remove-Item $file_buildlog; }
	New-Item $file_buildlog | Out-Null;
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

#################################
# Cleaning up tools #
#################################
Write-Host "- Cleaning up tools";
Remove-Item -LiteralPath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\dosbox-x')) -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\pklite')) -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\pkzip')) -Force -Recurse -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $([IO.Path]::GetFullPath([IO.Path]::Combine($((Get-Item $PSScriptRoot).Parent.FullName),'tools') + '\unxutils')) -Force -Recurse -ErrorAction SilentlyContinue

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