# Design Principles

This document describes the principles behind Katela's design and how they are applied when a change is reviewed. It explains the reasons behind decisions, not the current state of the code, so it should rarely need to change.

For what Katela is, read `README.md` and `docs/process/DEVELOPMENT_PROCESS.md`. For how to build, test and submit a change, read `CONTRIBUTING.md`. If anything here disagrees with `README.md` or `CONTRIBUTING.md`, they win.

## One kernel, not a collection of parts

Every subsystem is designed as part of one cohesive kernel. A new feature should fit the existing structure, naming and conventions instead of living next to them. If a feature needs special cases in unrelated code, the design is probably wrong, and the fix is to change the design, not to add the special cases.

## Katela's own terms

Katela does not follow UNIX or POSIX conventions. "UNIX does it this way" is not an argument for a change, and it is not an argument against one. A good proposal states the problem, what the solution costs, and which alternatives were considered. It explains why the result is better for Katela itself.

## Simple and readable

Code must be understandable by someone who was not there when it was written. Prefer the plain solution. Complexity is accepted only when it pays for itself with a concrete benefit that can be named.

The goal is that one person can still read and understand the whole kernel. Every addition makes that harder, so every addition has to earn its place.

## Everything the kernel needs lives in the repository

Katela has no hosted environment and no standard library. When the kernel needs an algorithm, a protocol or a format, it is implemented in the repository. Third-party code is not added without the maintainer's approval, and it must be compatible with the license in `LICENSE`.

## Logic apart from hardware

Code that only transforms data, such as parsers, packet builders, checksums and hash functions, should not touch I/O ports, interrupts or the scheduler. Hardware-facing code lives in drivers and calls into it.

This keeps the logic easy to read and review on its own, and it keeps drivers thin, so the part that is hard to test is as small as possible.

## Correct first

When principles pull in different directions, the order is:

1. Correctness and safety. Do not corrupt data or memory, and do not open security holes.
2. Simplicity and readability.
3. Features.
4. Speed.

Neither a feature nor a performance gain is a reason to merge code that is hard to understand or that nobody can explain.

## Failures are visible

Errors are reported to the caller and, where it makes sense, to the user. They are not ignored, and they are not hidden. When the kernel cannot continue safely, it stops with a clear message instead of carrying on in an unknown state.

## Say what was tested

Support means tested. A change states where it was run, such as which emulator or which hardware, and says plainly which paths were not exercised. Untested code is described as untested. A successful build is not evidence that something works.

## Small steps that can be undone

Changes are small, focused and easy to revert. Anything other code depends on, such as the on-disk layout or the system call conventions, changes only on purpose, with a migration path, and after the idea has been discussed in an issue.

## What Katela is not trying to be

- A POSIX or UNIX compatible system, or a host for existing UNIX tooling.
- A demonstration that stops at "hello world". It is meant to be a real, usable kernel.
- A kernel for every piece of hardware. A driver is accepted when it can be understood, tested and maintained.

## Using this document in review

These principles are for judgment, not for mechanical rule-following. When a change is declined, the reviewer names the principle involved. When a change is proposed, the author addresses the principles that apply. An exception is possible when the reasons for it are stated and accepted.
