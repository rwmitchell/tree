# tree
## Background
This is a fork of:   ftp://mama.indstate.edu/linux/tree/

I've customized it for features I wanted.  The original code has also continued to evolve.  I recommend looking there first.

- add color to the level indicator lines
  - uses $TREE_COLORS to defines colors
- add glyphs to filenames/directories
  - uses $LS_ICONS

## Requirements
- OS X and C
- $LS_ICONS - this is from my els repo

## Screenshots
![tree](img/tree.png)
![vdir](img/vdir.png)
![vdir-size+date](img/vdir-size+date.png)

## Comparison
Comparison between the original tree (2.0.0) with this version.
Both run using ```tree -Ds```

![compare](img/compare.png)
