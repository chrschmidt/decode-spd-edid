BEGIN {
	bank = 1;
	printf ("#define 
	printf ("static const struct jedec_vendor jedec_vendors[] = {\n");
}

/The following numbers are all in bank/ { bank++; }

/[0-9]+ .* [01] [01] [01] [01] [01] [01] [01] [01] [0-9A-F]/ {
	printf ("  { { ");
	for (i=1; i<bank; i++) printf ("0x7F, ");
	printf ("0x%s%s ", $NF, bank < maxbanks ? "," : "");
	for (i=bank; i<maxbanks; i++) printf ("0x00%s ", i < (maxbanks-1) ? ",": "");
	printf ("}, \"");
	maxind = NF - 9;
	for (i=2; i<maxind; i++) printf ("%s ", $i);
	printf ("%s\" },\n", $maxind);
}


static const struct jedec_vendor jedec_vendors[] = {
#include "vendortable.h"
    {{0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F}, 0}
};
