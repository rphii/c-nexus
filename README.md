# c-nexus

Small code footprint.

Create notes and link them, however you can think of. Once created you can browse the nodes.

- **h** : go back in history
- **j** : down
- **k** : up
- **l** : follow the arrow
- **q** : quit
- **[space]** : fold/unfold descriptions
- searching is not a feature yet :)

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

