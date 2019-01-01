BEGIN {
	bank = 0;
        bank_par = 0x80;
	printf ("#pragma once\n");
        printf ("#include <stdint.h>\n");
        printf ("struct jedec_vendor { const uint16_t id; const char * name; };\n");
	printf ("static const struct jedec_vendor jedec_vendors[] = {\n");
}

/The following numbers are all in bank/ {
        bank++;
        bank_par = 0x80;
        for (i=0; i<7; i++)
                if (and (bank, lshift (1, i)))
                        bank_par = xor (bank_par, 0x80);
        bank_par += bank;
}

/[0-9]+ .* [01] [01] [01] [01] [01] [01] [01] [01] [0-9A-F]/ {
        printf ("    { 0x%s%02x, \"", $NF, bank_par);
	maxind = NF - 9;
	for (i=2; i<maxind; i++) printf ("%s ", $i);
	printf ("%s\" },\n", $maxind);
}

END {
	printf ("    { 0xffff, 0 }\n");
	printf ("};\n");
}
