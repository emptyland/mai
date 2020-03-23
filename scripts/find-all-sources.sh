#!/bin/bash

cd src/base

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${BASE_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/core

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${CORE_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/db

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${DB_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/port

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${PORT_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/table

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${TABLE_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/txn

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${TXN_SOURCE_DIR}/$name"
done

cd - > /dev/null

cd src/lang

for name in $(ls *.cc | grep -v  -e "test.cc"); do
    echo "\${LANG_SOURCE_DIR}/$name"
done

cd - > /dev/null

echo "------------tests------------"

cd src

for name in $(find . -name  "*-test.cc" | sed -e "s/^\.//" | grep -v -e "^/sql" | grep -v -e "^/nyaa"); do
	echo "\${PROJECT_SOURCE_DIR}/src$name"
done

cd - > /dev/null