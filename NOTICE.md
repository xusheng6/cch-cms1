# Attribution and provenance

This unofficial project preserves behavior of CMS1, the engine used by
*Chinese Chess Master III: Jiangzu* (象棋大師 III／將族), associated with original
developer Yu Hsi-Shun / 虞希舜 and T-Time Technology / 光譜資訊.
It is not endorsed by those developers and is not their original source release.

The rewrite was developed using disassembly, runtime traces, differential
testing, and AI coding assistance. It was **not** a separated clean-room process.
Credit for the original engine's design and behavior belongs with its developers.

## Included and excluded material

Included: newly written implementation and tests; recovered evaluation tables,
constants, opponent names and control values; a small generated regression
fixture set; and public validation documentation. In particular,
`port/src/original_constants.h` contains data recovered from the original engine.

Excluded: original executables, `OPENING.LIB`, artwork, music, manuals, memory
images, instruction traces, and disassembler databases.
The progress diagram is newly generated from research checkpoint counts.

The MIT license covers contributors' licensable contributions. It does not
assert ownership of third-party material, grant rights the contributors do
not hold, or license the original game, its assets, or its opening library.

No original-author permission or rights-clearance document is recorded here.
Recovered tables are not asserted to be free of third-party rights, and
preservation intent does not provide blanket legal clearance.
