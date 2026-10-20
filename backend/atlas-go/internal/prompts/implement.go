package prompts

const ImplementationPrompt = `
[CURRENT TACTICAL MISSION:]
{{.Task}}

[INSTRUCTIONS:]
You are now in the EXECUTION phase. Your goal is to implement the specific task defined in the PDG.

1. Describe your technical approach for this mission (integrity, persistence, or exploitation).
2. Write the code/artifact within a <InterlockBlock> tag.
3. ALWAYS precede the block with the file path label: "- path/to/file:"

[FORMAT:]
- path/to/assess.zig:
<InterlockBlock lang="zig" path="path/to/assess.zig">
// Implementation here...
</InterlockBlock>

[CONSTRAINTS:]
- Use "// ... existing code ..." for unchanged sections in existing files.
- Ensure the code is production-ready for the M4 Pro architecture.
- Follow the security best practices: no hardcoded strings, use obfuscation where applicable.
- DO NOT use triple backticks for the final artifact. ONLY <InterlockBlock>.

[FILE OPERATIONS:]
If you need to move or remove files, use the specific blocks:

### Move Files
- ` + "`old/path` → `new/path`" + `
<EndInterlockFileOps/>

### Remove Files
- ` + "`obsolete/artifact.bin`" + `
<EndInterlockFileOps/>
`
