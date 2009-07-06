#! /bin/sh

bin_path="./darts-clone-test"
if [ ! -f "$bin_path" ]
then
	echo "Error: no such file: $bin_path"
	exit 1
fi

"$bin_path" KeyFile
if [ $? -ne 0 ]
then
	echo "Error: $bin_path failed"
	exit 1
fi

echo "Done! $bin_path"
