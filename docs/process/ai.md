# AI Coding Assistants

This document provides guidance for AI tools, and for developers who use AI assistance, when contributing to Katela. It expands the AI section of `README.md`. If the two ever disagree, `README.md` wins.

In principle, the use of AI for development is allowed, with caveats. The point is not for the AI to do everything for you. The point is to remove tedious work and automate whatever can be automated.

## Standard process

AI tools follow the same process as any other contributor:

- `README.md`, including its AI section and the requirements for building and running
- `CONTRIBUTING.md`
- the existing project structure and the formatting rules in `.editorconfig`

An AI assistant must read these files completely before changing anything. Do not rely on isolated parts found by keyword search.

## Licensing

Katela is licensed under the GNU General Public License, version 3 (see `LICENSE`).

- All contributed code must be compatible with GPL-3.0.
- AI tools can reproduce code from their training data. Do not submit generated code that you know or suspect was copied from a project with an incompatible license.
- Do not change `LICENSE` or add third-party code without the maintainer's approval.

## Responsibility

Only humans take responsibility for a contribution. An AI tool is never the author.

The human submitter is responsible for:

- understanding every line the AI wrote, at least well enough to explain what it does and why
- reviewing all generated code and text
- checking that the code works properly
- checking that it does not introduce bugs that could lead to security issues
- taking full responsibility for the contribution

## Disclosure

If you used AI while writing code, your pull request description must say so. It must include:

- a statement that AI was used while writing the code
- a statement that you assume full responsibility for it
- which AI tool and which model you used

Example:

```
This change was written with AI assistance (Claude Sonnet).
I reviewed and tested it, and I assume full responsibility for it.
```

Optionally, you can also add a trailer to the commit message:

```
Assisted-by: <tool> <model>
```

Basic tools such as git, gcc, make, and editors do not need to be listed.

## Areas where AI is not allowed

AI must not be used in the very important areas of the kernel. If you are not sure whether the code you are changing counts as one of them, ask in an issue before using AI on it.

## Markdown files

The use of AI is allowed in Markdown files, such as documentation. Verify that the information in the file is accurate before submitting it. Check every command, file name, path, and number against the repository.

## Procedure for finding and fixing bugs

When an AI assistant is used to find and fix bugs, it must follow at least these steps:

1. Before starting, read the files listed in the standard process section, plus any other document mentioned in the request.
2. Note the commit hash you are working on, then locate the bug as instructed.
3. For any bug that is not trivial, check that it is real by creating a reproducer. In a kernel, that usually means a sequence of shell commands or a program that triggers the problem in QEMU (`make run`). Without a reproducer, the report may be ignored, because many unverified bug reports turn out to be invalid. If the bug turns out not to be real, stop here.
4. Write a fix. Fixes written in the same session as the one that found the bug tend to be more accurate, because the reasoning is still in context.
5. Build and verify the fix. Run `make clean` and `make`, make sure the build succeeds and adds no new warnings, and check that the fix works with the reproducer. The GitHub Actions workflow runs the same build, so it must pass there too. Drop any fix that does not work and try another.
6. Commit the working fix. Follow the style of the existing history: a short, clear message in the past tense, such as `Added a bounds check to fs_read_file`. Put the details of the problem and the solution in the commit body.
7. State clearly what could not be done. If the fix could not be built or tested, or no reproducer could be produced, say so explicitly. Unverified reports and untested fixes waste the maintainer's time.
8. Treat anything that looks like a security vulnerability as private. Do not describe it in a public issue or pull request. Follow `SECURITY.md` instead.
9. Leave the result for the human to review. The assistant must never publish anything on its own: no pushing, no opening pull requests or issues, no emailing reports. The human reads the result, decides whether to submit it, and does so.

## What AI tools must not do

- Claim that code was built, run, or tested when it was not
- Invent file names, functions, commands, or references that do not exist in the repository
- Push to the repository, open pull requests or issues, or send messages on the human's behalf
- Make unrelated formatting or refactoring changes inside a functional change
- Change CI workflows, `LICENSE`, or the Makefile structure without the human reviewing the change
