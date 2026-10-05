# Development Process

## Summary

This document explains what Katela is, why it is worth contributing to, how a contribution travels from an idea to the `main` branch, and under which license the code is shared.

- **What Katela is** describes the project and the way it is designed.
- **Why contribute** lists what you get from working on a kernel this size.
- **How contributions work** describes the process step by step.
- **Licensing** covers the license and what it means for your code.

This document does not replace `README.md` or `CONTRIBUTING.md`. Read both first. If anything here disagrees with them, they win.

## What Katela is

Katela is a standalone kernel written from scratch in C and x86 assembly. Development began on April 26, 2026. It has its own core, its own shell, its own filesystem, and its own drivers.

Katela is not built as a learning exercise that stops at "hello world", and it is not a collection of disconnected experiments. Every subsystem, from the shell to the filesystem to the drivers, is designed as part of one cohesive kernel.

Katela does not follow UNIX or POSIX conventions. It does not aim for POSIX compliance, UNIX-like semantics, or compatibility with existing UNIX tooling. It has its own system call conventions and its own shell behavior.

This matters for contributors. A proposal whose only argument is "UNIX does it this way" is unlikely to fit. A proposal that explains why the change makes Katela better on its own terms is much more likely to be accepted.

## Why contribute

- **The codebase is small enough to understand.** The kernel is currently a few thousand lines. You can read all of it, so your change can be made with the whole picture in mind.
- **Your work shapes the design.** Katela is young, and many major decisions have not been made yet. Contributors can influence them.
- **Authorship is preserved.** Every commit keeps the name of the person who wrote it in the git history.
- **Review improves the code.** Code that is reviewed before it is merged has fewer bugs. This matters even more in a kernel, where a single mistake can crash the whole machine or open a security hole.
- **You get real kernel experience.** Boot code, interrupts, memory management, scheduling, drivers, and filesystems are all here, and they are real, not simplified for teaching.

## How contributions work

The process is intentionally simple.

1. **Fork** the repository on GitHub.
2. **Build it** and run it in QEMU before you change anything, so you know what normal behavior looks like. The requirements and commands are in `README.md`.
3. **Make your change.** Keep the code simple and readable, and follow the existing project structure. Match the style of the file you are editing, and respect `.editorconfig`.
4. **Test it.** Run `make clean` and `make`, then boot the result in QEMU. A change that builds but was never run is not finished.
5. **Commit it** with a clear message. The existing history uses short messages in the past tense, such as `Added more syscalls` or `Moved GDT, IDT and interrupt code into src/boot`.
6. **Submit a pull request** against `main`. Describe what the change does and why, and how you tested it.

When you open a pull request, a GitHub Actions workflow builds the project automatically. The build must pass. First-time contributors also get a welcome message that points back to `README.md` and `CONTRIBUTING.md`.

### Using AI

The use of AI is allowed with conditions, which are set out in the AI section of `README.md`. In short: you must understand the code, you must say in the pull request that you used AI and which model, and you take full responsibility for the result.

### Keep changes focused

One pull request should do one thing. A bug fix, a new driver, and a formatting cleanup belong in three separate pull requests. Small changes are easier to review, easier to test, and easier to undo.

### Before you start something big

If you plan a large change, such as a new subsystem, a change to the filesystem format, or a new syscall convention, open an issue first and describe the idea. This avoids the situation where you spend weeks on work that does not fit the direction of the project.

### After you submit

Submitting is not the end. Expect questions and requests for changes. Answer them, update your branch, and be patient: Katela does not have a large team. A review comment is about the code, not about you.

## Licensing

Katela is released under the GNU General Public License, version 3. The full text is in the `LICENSE` file.

All code that is contributed must be compatible with that license. In practice this means:

- Only submit code that you wrote yourself, or code that you have the right to share under a compatible license.
- Do not copy code from projects with incompatible licenses, and do not include code derived from reverse engineering that was done without proper safeguards.
- If AI helped you, make sure its output does not reproduce code from a source you cannot legally reuse.

Questions about licensing and copyright come up often. Nothing in this document is legal advice. If you have a real legal question about your code, talk to a lawyer who understands this area.

## Acknowledgements

The structure of this document is based on the introduction to the Linux kernel development process written by Jonathan Corbet. The content has been rewritten for Katela.
