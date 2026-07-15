
// Just some code i need to move here for now. (Uzis)


/*

uint32_t NewCOOPPlayerLim = 12;
// Experiment to give CO-OP a higher player limit.
patchByte((BYTE*)0x007F750D + 1, 0x01); // max gb count (we'll use this for modified co-op)
patchDWord((void*)(0x0086ACF5 + 6), NewCOOPPlayerLim); // overwrite the co-op maxplayerslobby to 12
//patchBytesM((BYTE*)0x007F7095, (BYTE*)"\xBD\xC", 2);
patchNop((BYTE*)0x007F7522, 5); // nop coop max and max lobby

patchBytesM((BYTE*)0x007F7A31, (BYTE*)"\xB8\x02", 2);

patchBytesM((BYTE*)0x0051E502, (BYTE*)"\x68\x00\x60\x77\x01", 5);
patchBytesM((BYTE*)0x00826B8B, (BYTE*)"\xB8\xC", 2);
patchBytesM((BYTE*)0x0051E535, (BYTE*)"\x68\xB0\xEB\x92\x00", 5);
patchBytesM((BYTE*)0x0051E8A7, (BYTE*)"\x68\00\x70\x24\x0B", 5);

//0051E502

*/