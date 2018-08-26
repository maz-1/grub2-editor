#!/bin/sh
sed -i -E '/^\s*echo\s+.*$/d' "$@"
