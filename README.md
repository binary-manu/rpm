# Ranish Partition Manager, improved

This is an improved version of RPM, based on the historical version 2.44.

![RPM GUI][rpm-gui]

![RPM CLI][rpm-cli]

The goal was to add a couple of features to RPM and allow it to be easily built
on recent windows systems, despite its build system depending on an ancient
windows-based toolchain.

_LEGAL DISCLAIMER 1: this repository includes parts of the Borland C++ 5.0
toolchain, which are necessary to build the program. Technically, they are
copyrighted tools, but it is pretty easy to find them as abandoware on the
web, and they date back to 1997, so I find unlikely somebody cares about them
anymore. However, if the copyright holder decides so, it may contact me for
removal from the source tree._

_LEGAL DISCLAIMER 2: the source code for RPM can no longer be found on the web.
However, it used to be publicly available at the original author's website, as
shown by this now gone
[link](http://web.archive.org/web/20180201141432/http://www.ranish.com:80/)
saved by the [Wayback Machine](https://archive.org/web/web.php). I tried
contacting the original author before releasing, but since this tool has been
discontinued since 2002 I may never get a response._

The main goal of this repository is to prevent this good, highly educational
piece of software from vanishing as time passes. While it is less relevant in
the era of UEFI and GPT, this program still has some uses and, more
importantly, provides a fantastic playground for people interested in
low-level, bare metal programming. As an example, this is one of those
program that handle protected mode switch by themselves.

I don't expect this repository to see further development, apart from keeping
the code base able to compile as the tools, it uses, evolve.

## License

The original code didn't come with a license. My original work is distributed
under the MIT license.

## Ease of compilation

RPM depends on a Borland C++ toolchain, part of which requires a DOS environment
(16bit binaries), while other tools run under a Windows environment (32bit
binaries). To make building the code as easy as possible on windows systems, I
have made the following:

* parts of the Borland C++ 5.0 toolchain have been included within the
  repository. Only the files which are strictly required for the build are
  included and there is no setup. So if you plan to grab a full copy of the
  development environment, you should _not_ copy files from here. Instead, go
  to some abandonware site, it is pretty easy to find these tools;
* the build phase is run under DOSBox-X in order to execute the building tools.
  Thanks to these tools, the original build system can still be run without the
  need to patch or port it to a different toolchain;

For convenience, there is a clean `build/clean.ps1` (for cleanup environment)
and a build powershell script `build/build.ps1` which take care of building
RPM.

These scripts require the most recent [Microsoft PowerShell-Framework (7.3)](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows?view=powershell-7.3#msi) installed (x64 or x86)
on the host machine. 

To build RPM, simply run:

    cd build
    ./build.ps1

To clean the working directories, run:

    cd build
    ./clean.ps1

Keep in mind that these script may have to download the required 3rd party
components and tools to complete the building environment. Depending on your
connection, this may take some time and some MiB's of space.

### Project structure

    .
    ├── build
    ├── docs
    ├── src
    ├── tools
    ├── CHANGELOG.md
    ├── LICENSE.txt
    └── README.md

`build` contains files needed to build and the final output of the build system
and ancillary tools.

`src` contains the source code and the Makefiles to build it. The build system
is in-tree, so all object files and output artifacts will be placed here. In
particular, the final executable is `src/PART.EXE`.

`tools` contains a (partial) copy of the Borland C++ 5.0 toolchain and the
other required components.

While compiling, the output from the various tools (Assembler, compiler, linker
and so on) will be written to `build/BUILD.LOG`. Important information, like
errors in source files, will be logged in there too.

## Extra features

With respect to v2.44, this version adds (without including upcoming changes):

* the ability to save and restore the entire MBR to file, which was
  unimplemented in the original code (the corresponding keyboard shortcuts
  returned a "not implemented" error);
* the ability to save and restore the CMOS data (the information stored in the
  real time clock memory). This only covers the first 128 bytes of such data;
* command line versions of the operations above.

CMOS save/restore was meant as a simple ways to backup the current BIOS
configuration before experimenting with settings or changing the battery.
However, while it works well with some BIOSes, it totally fails with others.
It is import to test it on a system-by-system basis before relying on it to
produce usable backups.

### Command line options

The `/MBS`, `/MBL`, `/CMS` and `/CML` switches are used to save or load either
the MBR or CMOS data to or from a file. All of them take an extra `/F` options
which specifies the file used as the target of a store operation or as the
source for a load operation.

Additionally, `/CML` accepts a `/T` switch, which defines how the date/time
information within CMOS memory must be handled:

* if `/T` is not given, the current CMOS date/time is preserved (i.e. the
  system time is not changed);
* if `/T:` is given (that is, the option is followed by an empty date/time)
  the time information that were saved in the CMOS file are restored,
  effectively bringing the system time back to the value when the backup was
  taken;
* if `/T` is followed by a full date/time specifier, that value is written to
  the CMOS as is, there is no notion of UTC or local timezone.

### GUI options

The `L` or `S` keys will bring on the MBR save/load dialog. Both keys are
equivalent, as the exact operation is determined by the chosen dialog button.
Type the filename from which the MBR should be loaded (or stored) and then
navigate to the appropriate button. Confirm with enter.

![RPM MBR DIALOG][rpm-mbr-dlg]

The `M` key will bring on the CMOS save/load dialog. Just as the command-line
version, when loading CMOS data the user must select how to handle date/time,
by choosing one of the provided options.

![RPM CMOS DIALOG][rpm-cmos-dlg]

[rpm-gui]: docs/rpm-gui.png
[rpm-cli]: docs/rpm-cli.png
[rpm-mbr-dlg]: docs/rpm-mbr-dlg.png
[rpm-cmos-dlg]: docs/rpm-cmos-dlg.png

### Building tools

`32rtm version 1.5 Copyright (c) 1992-94 Borland International`
`Borland C++ 5.0 for Win32 Copyright (c) 1993, 1996 Borland International`
`MAKE Version 4.0  Copyright (c) 1987, 1996 Borland International`
`RTM loader version 1.5 Copyright (c) 1990-94 Borland International`
`Turbo Assembler  Version 3.1  Copyright (c) 1988, 1992 Borland International`
`Turbo Link  Version 7.1.30.1. Copyright (c) 1987, 1996 Borland International`
`Turbo Link for Win32  Version 1.6.71.0 Copyright (c) 1993,1996 Borland International`
<!-- vi: set fo=crotn et sts=-1 sw=4 :-->