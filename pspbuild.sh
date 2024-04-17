#!/bin/bash

exec docker run --rm \
     -v ./:/source \
     -w /source \
     pspdev/pspdev:latest \
     /bin/bash /source/pspcompile.sh
