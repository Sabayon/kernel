#!/bin/bash

www_host="$(whoami)@sabayon.org"
www_port=2222

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

ssh -t -p "${www_port}" "${www_host}" screen /home/bin/kernel-tag-version "${tag}"
