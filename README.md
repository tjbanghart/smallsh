# Smallsh

This program is a "wrapper shell" around bash. The user is prompted with `: ` and allowed to enter any common bash command The commands `cd`, `exit`, and `status` are handled by this program. All other commands are passed to the bash shell via `execvp()` as a child process. Commands can be run in either "foreground" or "background" mode. Commands executed in the foreground cause the parent process to `wait()` until the child process terminates. Background commands cause the parent to wait(`..WNOHANG..`) so the user is open to enter another command as the child process runs.

This program was made for as part of the course requirements for CS 344: Into To Operating Systems taken at Oregon State Univeristy in Fall 2020. You can read the assignment description [here](/assignment_desc.md).

## Setup
- Clone repo
- `cd` into cloned directory
- Run `make`
- Start the shell with `./smallsh`
- The commands `cd`, `exit`, and `status` are handled by the `smallsh` process -- all other `bash` commands should work normally.
- `cd` changes the current directory (as in `bash`)
- `status` shows exit value of the last run command
- Type `exit` to exit the shell
- Type `&` and a command to run it in "background" mode. The user will be handed back the shell prompt (`:`) while the command runs via a forked processes.
- CONTROL-Z will toggle "foreground-only mode" which will prevent `&` commands from running in the background.
- CONTROL-C kills the current running process but does not exit the shell.


## Test
You can also run an included test script against `smallsh`. To do so:
- Run `chmod u+x testscript`
- Then run the script and capture the output in `results` with: `./testscript > results 2>&1`
