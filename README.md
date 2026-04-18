# Native Animation Framework

A multi-character animation framework for Fallout 4.
It is just a fork for blescent original [Deweh/Native-Animation-Framework](https://github.com/Deweh/Native-Animation-Framework)

## Features

- Custom face animation system with an in-game creator.
- Custom full-body animation system with an in-game creator (aka the Animation Studio).
- Arbitrary inverse kinematic chains with support for skipped-bones, integrated into the full-body animation creator.
- A configurable multi-character scene system for playing synchronized animations with precisely positioned actors, compatible with XMLs made for AAF.
- AI package override system that can apply any arbitrary AI package to an actor without being overriden by quests/etc.
- Extensive papyrus API functions for all aforementioned features.

## Technical Capibilities

- Cubic spline-based keyframe sampling for natural movement, with either simple float values or quaternions.
- Precise timing thread for delayed tasks, which can be serialized to the F4SE cosave to maintain state between game sessions.

## Build Requirements

- [Visual Studio 2022 with C++ Desktop Development Component](https://visualstudio.microsoft.com/vs/)
- [vcpkg](https://github.com/microsoft/vcpkg) with VCPKG_INSTALLATION_ROOT and VCPKG_ROOT environment variables.
- [Python 3](https://www.python.org/downloads/) added to PATH environment variable.
