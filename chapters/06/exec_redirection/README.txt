1. Run `make`. This will create
	- execve_hook.ko
	- /tmp/good
	- /tmp/evil
2. Execute `/tmp/good` to make sure there are no problems
3. Load the module: `kldload ./execve_hook.ko`
4. Execute `/tmp/good` again to check that execution is redirected
