#!/bin/bash

# Check target tag
[[ -z "$1" ]] && echo "usage: $0 <new-version>" >&2 && exit 1

# Validate new version
tag="$1"
for cur_tag in $(git tag); do
    [[ "$cur_tag" == "$tag" ]] && echo "$tag already tagged" >&2 && exit 1
done

# tag version
echo "Tagging version: $tag"
git tag "$tag" && git push --quiet origin HEAD \
    && git push --quiet --tags || exit 1


this_dir=$(dirname "$0")
exec "${this_dir}/make-dist.sh" "${tag}"
