# so what the heck is going on!??

- The stack sometimes doesn't copy? We return from fork() as child, to NULL, causing #PF
- PID is out of whack. In working instances, it's not. That's a good lead.
- It looks like PID 1 is returning to PID 2 state sometimes? huh?


# unrelated problems
- Oops, calling exit() doesn't move the stack to the right place. #PF...