#!/bin/sh
for i in `find tests -name \*.diff `; do echo $i; cat $i; echo -e "\n\n\n"; done
