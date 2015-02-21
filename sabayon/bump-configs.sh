#!/bin/bash

CONFIGS_DIR="sabayon/config"
if [ ! -d "${CONFIGS_DIR}" ]; then
	echo "${CONFIGS_DIR} not found" >&2
	exit 1
fi

get_arch() {
	local conf="${1}"
	if grep -rq "^CONFIG_ARM=y" "${conf}"; then
		echo "arm"
	elif grep -rq "^CONFIG_X86_64=y" "${conf}"; then
		echo "x86_64"
	elif grep -rq "^CONFIG_X86_32=y" "${conf}"; then
		echo "x86"
	fi
}

main() {
	local conf= conf_arch=
	for conf in "${CONFIGS_DIR}"/*.config; do
		echo "Working on ${conf}"

		conf_arch=$(get_arch "${conf}")
		[ -z "${conf_arch}" ] && {
			echo "Cannot determine ARCH" >&2
			exit 1;
		}
		cp "${conf}" ".config" || exit 1
		make ARCH="${conf_arch}" oldconfig || exit 1 # interactive
		cp ".config" "${conf}" || exit 1
	done
}

main
