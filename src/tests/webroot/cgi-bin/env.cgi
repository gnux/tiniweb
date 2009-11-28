#!/bin/bash
#
# Minimalistic bash "CGI" script to show the environment.
#
echo -e -n "Content-Type: text/plain\r\n\r\n"
env
