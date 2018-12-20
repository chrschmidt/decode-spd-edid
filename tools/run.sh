#!/bin/sh

pdf=$1
txt=$(basename -s pdf "${pdf}")txt

pdftotext -nopgbrk -layout "${pdf}"
sed -i -E 's/[[:blank:]]+/ /g' "${txt}"

BANKS=$(awk '
BEGIN { bank = 1; }
/The following numbers are all in bank/ { bank++; }
END { print bank; }
' "${txt}")

echo -en '#pragma once\n#define JEDEC_BANKS '${BANKS}'\n' > ../jedecbanks.h

awk -f jed2c.awk -v maxbanks=${BANKS} "${txt}" > ../vendortable.h
