#!/bin/bash

# srv_host="fabio@sabayon.org"
# srv_port=2222
# kernel_tag_script="/home/bin/kernel-tag-version"

srv_host="fabio@sabayon.ing.unibs.it"
srv_port=8888
kernel_tag_script="/scripts/kernel/kernel-tag-version"

# Check target tag
[[ -z "$1" ]] && echo "usage: $0 <new-version>" >&2 && exit 1

# Validate new version
tag="$1"
for cur_tag in $(git tag); do
	[[ "$cur_tag" == "$tag" ]] && echo "$tag already tagged" >&2 && exit 1
done

# tag version
echo "Tagging version: $tag"
git tag "$tag" && git push origin HEAD && git push --tags || exit 1

ssh -t -t -p "${srv_port}" "${srv_host}" screen "${kernel_tag_script}" "${tag}"
