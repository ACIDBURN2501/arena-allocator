# Security Policy

## Reporting a Vulnerability

Use GitHub's Security Advisories feature to report security concerns
privately.

Expect a response within 7 days. If the issue is confirmed, a fix
will be released as a patch version.

## Scope

arena-allocator is a small memory allocator library with no network
stack, no external dependencies, and no dynamic memory allocation.
The primary attack surface is integer overflow in size/alignment
calculations and buffer overflow in arena bounds checking.
