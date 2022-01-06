#!/usr/bin/env bash
gcc -o yule_2021.so yule_2021.c $(yed --print-cflags) $(yed --print-ldflags)
