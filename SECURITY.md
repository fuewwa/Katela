# Security Policy

## Supported Versions

Katela is under active development and has no stable releases yet. Only the latest commit on the `main` branch receives security fixes.

| Version         | Supported |
| --------------- | --------- |
| `main` (latest) | Yes       |
| Older commits   | No        |
| Tagged releases | Only the most recent one, once releases exist |

## Reporting a Vulnerability

Please do not report security vulnerabilities through public GitHub issues, discussions, or pull requests.

Send a report by email to **kurwa.offc@proton.me** with the subject line `[Katela Security]`.

Include as much of the following as you can:

- A description of the vulnerability and its impact
- The affected component (kernel core, memory manager, filesystem, a specific driver, syscall interface, build system, CI workflow)
- The commit hash or version you tested
- Steps to reproduce, or a proof of concept
- How you ran it (QEMU, VirtualBox, VMware, or real hardware) and with which options
- Any suggested fix or mitigation

If you want to send sensitive details, ask for an encrypted channel in your first message and one will be arranged.

## What to Expect

- **Response:** Katela is maintained by a single person, so there is no guaranteed response time. Reports are read and answered as time allows.
- **Assessment:** the report will be reviewed and you will be told whether it is accepted as a vulnerability or declined.
- **Fix:** accepted issues are fixed on `main` when possible. Timing depends on severity and complexity.
- **Disclosure:** please keep the details private until a fix is available. After the fix is published, the issue can be disclosed publicly and you will be credited if you wish.

## Scope

In scope:

- Memory corruption, out-of-bounds access, and use-after-free in kernel code and drivers
- Privilege escalation from usermode to kernel mode
- Flaws in the syscall interface, including missing argument validation
- Filesystem bugs that allow data corruption or reading data that should not be accessible
- Flaws in the build system or CI workflows that could compromise the produced ISO

Out of scope:

- Issues that require modifying the kernel source or the build toolchain on the victim's machine
- Bugs in third-party software such as GRUB, QEMU, or NASM (please report those upstream)
- Missing hardening features that are not yet implemented and are listed on the roadmap
- Denial of service through resource exhaustion in a single-user development environment

## Safe Harbor

Good-faith research and responsible reporting under this policy are welcome. Please avoid accessing, modifying, or destroying data that does not belong to you, and test only against your own systems and virtual machines.
