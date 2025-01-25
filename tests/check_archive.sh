#!/bin/sh

set -eu

archive=${1:-libbuffered_line_reader.a}
manifest_dir=tests/manifests
temporary_dir=$(mktemp -d)
trap 'rm -rf "$temporary_dir"' EXIT HUP INT TERM

ar t "$archive" |
	sed '/^__.SYMDEF/d' |
	LC_ALL=C sort >"$temporary_dir/archive-members.txt"

nm -g "$archive" |
	awk 'NF >= 3 && $2 != "U" {print $3}' |
	sed 's/^_//' |
	LC_ALL=C sort -u >"$temporary_dir/api-symbols.txt"

nm -g "$archive" |
	awk 'NF == 2 && $1 == "U" {print $2}' |
	sed -e 's/^___error$/errno_accessor/' \
		-e 's/^__errno_location$/errno_accessor/' \
		-e 's/^_//' |
	LC_ALL=C sort -u >"$temporary_dir/allowed-undefined.txt"

diff -u "$manifest_dir/archive-members.txt" \
	"$temporary_dir/archive-members.txt"
diff -u "$manifest_dir/api-symbols.txt" \
	"$temporary_dir/api-symbols.txt"
diff -u "$manifest_dir/allowed-undefined.txt" \
	"$temporary_dir/allowed-undefined.txt"
