param([string[]]$Path = @('build/Release/DesktopPet.exe','build/Release/Updater.exe'))
$ErrorActionPreference='Stop'
foreach ($file in $Path) {
 $b=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($file))
 function Bound([long]$o,[long]$n) { if ($o -lt 0 -or $n -lt 0 -or $o -gt $b.Length-$n) { throw 'Malformed/truncated PE.' } }
 function U16([long]$o) { Bound $o 2; return [BitConverter]::ToUInt16($b,[int]$o) }
 function U32([long]$o) { Bound $o 4; return [BitConverter]::ToUInt32($b,[int]$o) }
 function U64([long]$o) { Bound $o 8; return [BitConverter]::ToUInt64($b,[int]$o) }
 if ((U16 0) -ne 0x5a4d) { throw 'Not a PE executable.' }
 $pe=U32 0x3c
 if ((U32 $pe) -ne 0x4550 -or (U16 ($pe+4)) -ne 0x8664) { throw 'Windows x64 PE required.' }
 $opt=$pe+24;$sections=U16 ($pe+6);$optionalSize=U16 ($pe+20)
 if ((U16 $opt) -ne 0x20b -or $optionalSize -lt 240) { throw 'PE32+ headers required.' }
 $flags=U16 ($opt+70)
 if (($flags -band 0x4160) -ne 0x4160) { throw 'ASLR/HIGHENTROPYVA/DEP/CFG missing.' }
 $sectionOffset=$opt+$optionalSize
 function Offset([uint32]$rva) {
  for ($i=0;$i -lt $sections;$i++) {
   $s=$sectionOffset+40*$i;$start=U32 ($s+12);$raw=U32 ($s+16)
   if ($rva -ge $start -and ([long]$rva-$start) -lt $raw) { return [long](U32 ($s+20))+[long]$rva-$start }
  }
  throw 'RVA has no file-backed section.'
 }
 if (!(U32 ($opt+112+5*8)) -or !(U32 ($opt+116+5*8))) { throw 'ASLR relocation directory missing.' }
 $loadRva=U32 ($opt+112+10*8);if (!$loadRva) { throw 'Load configuration missing.' };$load=Offset $loadRva
 if ((U32 $load) -lt 148) { throw 'Security load configuration incomplete.' }
 if ((U16 ($load+78)) -ne 0x800) { throw 'Static DLL dependencies must resolve only in System32.' }
 if (!(U64 ($load+88))) { throw '/GS security cookie missing.' }
 if (!(U64 ($load+112)) -or !(U64 ($load+128)) -or !(U64 ($load+136)) -or ((U32 ($load+144)) -band 0x500) -ne 0x500) { throw 'CFG instrumentation/FID table missing.' }
 $imports=U32 ($opt+112+8);if (!$imports) { throw 'Expected Windows import table.' };$descriptor=Offset $imports;$dlls=@()
 for ($n=0;$n -lt 256;$n++) {
  $rva=U32 ($descriptor+12);if (!$rva) { break };$nameOffset=Offset $rva;$end=$nameOffset
  while ($end -lt $b.Length -and $b[$end]) { $end++;if ($end-$nameOffset -gt 256) { throw 'Invalid DLL name.' } }
  if ($end -eq $b.Length) { throw 'Truncated DLL name.' }
  $name=[Text.Encoding]::ASCII.GetString($b,[int]$nameOffset,[int]($end-$nameOffset));$dlls+=$name
  if ($name -notmatch '(?i)^(kernel32|user32|gdi32|shell32|ole32|oleaut32|advapi32|winhttp|crypt32|bcrypt|comdlg32|comctl32|gdiplus|shlwapi|version|imm32|ntdll|secur32|ws2_32|rpcrt4|api-ms-win-[a-z0-9-]+|ext-ms-win-[a-z0-9-]+)\.dll$') { throw "Non-allowlisted runtime DLL: $name" }
  $descriptor+=20
 }
 if ($n -eq 256) { throw 'Unterminated import table.' }
 Write-Output "$file : PASS x64, ASLR/high entropy, DEP, CFG table, GS cookie, System32 dependent-load policy; imports: $($dlls -join ', ')"
}
