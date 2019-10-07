# Ranish Partition Manager, improved

This is an improved version of RPM, based on the historical version 2.44.

![RPM GUI][rpm-gui]

![RPM CLI][rpm-cli]

The goal was to add a couple of features to RPM and allow it to be easily built
on \*NIX systems, despite its build system depending on an ancient
Windows-based toolchain.

_LEGAL DISCLAIMER 1: this repository includes parts of the Borland C++ 5.02
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

## License

The original code didn't come with a license. My original work is distributed
under the MIT license.

## Ease of compilation

RPM depends on a Borland toolchain, part of which requires a DOS environment,
while other tools run under a Windows environment. To make building the code as
easy as possible on \*NIX systems, I have made the following:

* parts of the Borland C++ 5.02 toolchain have been included within the
  repository. Only the file sets requires for the build are included and there
  is no setup, so if you plan to grab a full copy of the development
  environment, you should _not_ copy files from here. Instead, go to some
  abandonware site, it is pretty easy to find this tool;
* the build phase is run under Wine in order to execute Windows tools. When
  running real mode programs that require a DOS environment, Wine transparently
  delegates the execution to DosBox. Thanks to this two tools, the original build
  system can still be run without the need to patch or port it to a different
  toolchain;
* the entire compilation takes place inside a Docker container, which can be
  built using `docker/Dockerfile`. Putting all the tools inside a container
  eases the setup of the build environment: you don't have to install Wine,
  DosBox and other tools on your host.

For convenience, there is an helper shell script `docker/build.sh` which takes
care of creating the Docker container, if it does not exists, and then uses it
to build RPM. Additional arguments to `build.sh` are passed down to Borland
Make.

To build RPM, simply run:

    cd docker
    ./build.sh

To clean the working directory, run:

    cd docker
    ./build.sh clean

Keep in mind that this script may have to create the container from scratch,
which involves downloading the base image for the container and all the tools
the must be installed inside it. Depending on your connection, this may take
some time and some GiB's of space.

### Project structure

    .
    ├── docker
    ├── README.md
    ├── src
    └── tools

`src` contains the source code and the Makefiles to build it. The build system
is in-tree, so all object files and output artifacts will be placed here. In
particular, the final executable is `src/PART.EXE`.

`tools` contains a (partial) copy of the Borland C++ 5.02 toolchain.

`docker` contains files needed to build the container hosting the build system
and ancillary tools.

While compiling, the output from the various tools (Turbo Assembler, compiler,
linker, ...) will be written to `src/BUILD.LOG`. Output produced by the
invocation of `docker/build.sh` to standard output and standard error is
extremely noisy and not very useful, consisting mainly of DosBox complaining
about audio/video rendering which has been disabled. I decided to keep such
output showing in order to avoid hiding things from users, but unless there is
a problem to analyze I recommend redirecting output to `/dev/null`. Important
information, like errors in source files, will still be logged to
`src/BUILD.LOG`.

### Intercepting output from DosBox invocations

Some tools run under DosBox, which doesn't produce textual output of what
programs write on the emulated screen. In order to obtain such output, commands
run under DosBox must have their standard output redirected to a file from
within DosBox itself, as in `FOO.EXE > MYLOG.TXT`.

When Wine detects that a program must be run in real mode, as opposed to under
a Win32 environment, it creates a configuration file for DosBox containing the
command to execute, plus some boilerplate to mount drive letters, and calls
DosBox to execute it. Such file needs to be patched in order to add appropriate
redirection requests so that output from real mode tools can be saved to the
log file.

This is implemented by making Wine call a fake DosBox executable
(`docker/dosbox`) which patches the configuration file and then calls the
real DosBox. While this solution works, it depends on the actual structure of
the configuration file Wine produces, so it is potentially prone to breakage
if that file format changes.

## Extra features

With respect to v2.44, this version adds:

* the ability to save and restore the entire MBR to file, which was
  unimplemented in the original code (the corresponding keyboard shortcuts
  returned a "not implemented" error);
* the ability to save and restore the CMOS data (the information stored in the
  real time clock memory). This only covers the first 128 bytes of such data;
* command line versions of the operations above.

### Command line options

The `/MBS`, `/MBL`, `/CMS` and `/CML` switches are used to save or load either
the MBR or CMOS data to or from a file. All of them take an extra `/F` options
which specifies the file used as the target of a store operation or as the
source for a load operation.

Additionally, `/CML` accepts a `/T` siwtch which defines how the date/time
information int the CMOS memory must be handled:

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

<!-- vi: set fo=crotn et sts=-1 sw=4 :-->
