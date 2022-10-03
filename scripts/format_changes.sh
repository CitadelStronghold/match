#!/bin/bash

for FILE in $(git diff --cached --name-only | grep -E '.*\.(cxx|txx|hxx)')
do
    if [ -f "$FILE" ]; then
        clang-format -i $FILE
    fi
done

for FILE in $(git diff --name-only | grep -E '.*\.(cxx|txx|hxx)')
do
    if [ -f "$FILE" ]; then
        clang-format -i $FILE
    fi
done

for FILE in $(git ls-files --others --exclude-standard | grep -E '.*\.(cxx|txx|hxx)')
do
    if [ -f "$FILE" ]; then
        clang-format -i $FILE
    fi
done