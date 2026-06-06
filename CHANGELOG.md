# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed

- Standardised project metadata, SPDX headers, security policy, pkg-config generation, installed version header, contributor guidance, and CI coverage thresholds with the primitive family baseline.

## [1.1.8] - 2026-03-10

### Fixed

- Corrected the exported Meson library dependency and generated `arena_version.h` into the build directory for consumers.

## [1.1.7] - 2026-03-06

### Fixed

- Made the install smoke test detect Meson multiarch library directory layouts.

## [1.1.3] - 2026-03-03

### Fixed

- Resolved MISRA C:2012 audit findings around header guards, unused includes, NULL marker handling, const-correct prototypes, Rule 11.6 documentation, and marker/alignment regression tests.
