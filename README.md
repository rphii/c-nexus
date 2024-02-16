# c-nexus

Small code footprint.

Create notes and link them, however you can think of. Once created you can browse the nodes.

## Normal View
- ``h`` : go back in history (can be: viewed notes, searches)
- ``j`` : down
- ``k`` : up
- ``l`` : follow the arrow
- ``q`` : quit
- ``Q`` : rebuild (certainly works on linux)
- ``[space]`` : fold/unfold descriptions
- ``f`` : enter search view

## Search View
- ``[type something]`` : search for something, case insensitive
- ``[enter]`` : switch from editing search string to selecting found nodes; in this mode ..
    - .. `hjkl` : behaves as it would in normal view
    - .. `f`, `[enter]` : puts you back to editing the search string
    - .. `F` : clears the editing string and puts you back to editing it
    - .. `[escape]` : goes back to node you've watched before searching; as if nothing was
      selected (it goes back in the history)

## Command Line Arguments

- **Print help and exit** : `--help`
- **Print version and exit** : `--version`
- **Change default view** : `--view`
- **Change entry note** : `--node`

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

## Notes on Searching
- if you have many notes, you might benefit from parallelized searching. you can activate it via
  specifing the number of threads with the preprocessor token `PROC_COUNT` (e.g. if you want to
  search with 8 threads, in the Makefile add `-D PROC_COUNT=8` to the `CFLAGS`)
- if multithreaded searching is enabled, I at the moment do not bother to sort the found results, so
  that's that (equal searches of something might spit out randomized notes)
- even if you're not searching with multithreading, the results can seem unordered, because we're
  searching through the items in a hash table, after all. and I do not bother to sort them as well
- Q: why not sort the results?
    - A: 1) I don't know what to sort for, except maybe how likely a string is present...
    - A: 2) ...which I don't quite know how to do. I'd have to rework the searching algorithm...
    - A: 3) ...so when it bothers me too much, I'll rework it, eventually, maybe

