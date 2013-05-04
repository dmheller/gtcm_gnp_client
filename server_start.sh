#!/bin/sh
. /etc/profile.d/gtmprofile.sh
$gtm_dist/gtcm_gnp_server -log=~/GTCM.log -service=30000
