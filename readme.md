# GitBranch Far Manager plugin

Plugin helps Far Manager to show git branch at command prompt. It setups `%SH_G_BRANCH%,%SH_G_COMMIT%,%SH_G_STATE%,%SH_G_STATUS%`  and `%SH_DIR%` to git info for repository and directory on active panel. You should customize "Command line settings" to insert for example `(b)%SH_DIR%(d)%SH_G_BRANCH%(a)%SH_G_COMMIT%(e)%SH_G_STATE%(c)%SH_G_STATUS%(a)❯` in command line prompt format string. Branch name will be shown at command prompt:

![Far Manager plugin show git branch](git-branch-name.png)

**Git is not needed**



![build](https://github.com/smithx/far/workflows/build/badge.svg)
