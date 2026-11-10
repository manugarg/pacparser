# Security Policy

If you have discovered a security vulnerability in this project, please report it privately. **Do not disclose it as a public issue.** This gives us time to work with you to fix the issue before public exposure, reducing the chance that the exploit will be used before a patch is released.

Please disclose it at [security advisory](https://github.com/manugarg/pacparser/security/advisories/new).

This project is maintained by a team of volunteers on a reasonable-effort basis. As such, please give us at least 90 days to work on a fix before public exposure.

## Scope

Pacparser evaluates PAC scripts and resolves proxies for URLs. We are
interested in issues that a remote party can trigger through those inputs:
for example, memory corruption or code execution caused by a crafted PAC
file, URL, or host name.

The following are **out of scope**. Reports about them will be closed
without a security advisory, CVE, or credit:

- Build-time and install-time issues that require a local attacker to plant
  files on the build machine and a victim to then build or install
  pacparser with elevated privileges. Where practical, we still fix these as
  ordinary hardening.
- Local-only issues that depend on the victim taking unusual or unsafe
  actions, such as building or running pacparser as root from a directory
  that other users can write to.
- Issues in the build scripts, CI configuration, test code, or examples
  that do not affect the shipped library, `pactester`, or Python module.
- Findings from automated scanners or AI tools without a realistic,
  demonstrated attack path against a typical use of pacparser.
- Denial of service from a PAC script that is slow or never terminates.
  PAC files are code, so only load PAC files you trust to run.

Hardening suggestions are welcome as regular issues or pull requests.
