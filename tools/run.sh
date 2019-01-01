#!/bin/sh

pdf=$1
txt=$(basename -s pdf "${pdf}")txt

pdftotext -nopgbrk -layout "${pdf}"
sed -i -E 's/[[:blank:]]+/ /g' "${txt}"

awk -f jed2c.awk "${txt}" > ../vendortable.h
