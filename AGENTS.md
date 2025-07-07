Rules:
Do not commit if you have deleted files, or large contents of files. Ask user dorectly if changes are ok to push and if you have verified already.
Do not run any Destructive git commnd unless directly asked.
Do not add placeholders for code to be put in later. Never have examples or placeholders.
When commit, Do not label your release as a prerelease, as the kernel is in alpha pre release and also provide release notes like bug fixes and improvements and new features if any along with the release, formatted in md.

## Build Number Legend

| Field  | Options            | Description                                   |
|------- |-------------------|----------------------------------------------|
| Major  | 0-9, A, etc.      | Major project version                         |
| Type   | I, R, T, D        | Internal, Release, Test, Debug                |
| Build  | 0001-9999         | Build number, auto-incremented                |
| Update | M, S, B, F        | Minor, Substantial, Bugfix, Feature           |

In debug mode, model numbers use this table to encode the version string.
