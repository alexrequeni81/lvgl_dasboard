#!/bin/sh
# Tiny Core Linux dependencies for v1_dashboard
# Run as root on the target:  sh tce_deps.sh

tce-load -wi curl.tcz
tce-load -wi openssl.tcz
tce-load -wi ca-certificates.tcz
tce-load -wi mpg123.tcz
tce-load -wi alsa-utils.tcz
tce-load -wi libasound.tcz
