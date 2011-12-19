#!/bin/bash
base=/u/c/s/cs537-1/ta/tests/6
python $base/runtests.py --test-path $base $@
exit $?

