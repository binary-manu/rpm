# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed

* The Docker container used to run the toolchain is now based on
  `opensuse/leap:15.2`, as the Arch Linux based one used so far has
  issue with running DOS executables via Wine.

## [v2.46] - 2019-11-02

### Changed

* Replace literal magic numbers with constants.

### Added

* Wine/DosBox based build system.
* Dockerfile for building helper container.
* Added contributor information.
* MBR can be saved and restored either from the GUI or from the command line.
* CMOS data can be saved and restored either from the GUI or from the command
  line.
