#!/bin/bash

exec docker run -it --rm -v ./:/source -w /source pspdev/pspdev:latest
