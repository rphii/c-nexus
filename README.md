# c-nexus

Small code footprint.

Create notes and link them, however you can think of. Once created you can browse the nodes.

- ``h`` : go back in history
- ``j`` : down
- ``k`` : up
- ``l`` : follow the arrow
- ``q`` : quit
- ``[space]`` : fold/unfold descriptions
- ``f`` : enter search mode
    - ``[type something]`` : search for something, case insensitive
    - ``[enter]`` : switch from editing search string to selecting found nodes; in this mode ..
        - .. `hjkl` behaves as stated above
        - .. `f` `[enter]` puts you back to editing the search string
        - .. `[escape]` and `q` goes back to node you've watched before searching; as if nothing was
          selected

## How to add notes?

Edit the [source file](src/nexus.c).

There's a handy macro called `NEXUS_INSERT`. See the example(s) provided.

### Upsides?
- impossible to link a note to an unexisting one
- at startup, all notes are stored in RAM (fast lookup)
- compile checked code

### Downsides?
- impossible to link a note to an unexisting one
- at startup, all notes are stored in RAM (amount limited through RAM)
- no editing of notes while browsing them _(at least not yet)_
- terminal interface doesn't yet handle the displaying of very long notes

### Notes on Searching
- it searches with multiple cores, as you type. if you wish to not have multiple cores, or a
  different amount of threads, you can specify so in the [Makefile](Makefile) by specifying the
  number for `DPROC_COUNT` (one core searches one entire node, or rather, transforms the node string
  into a searchable string, searches for a substring and stores the result)
- if multithreaded searching is enabled, I at the moment do not bother to sort the found results, so
  that's that

