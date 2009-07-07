#! /bin/sh

bin_dir="../src"
mkdarts_path="$bin_dir/mkdarts-clone"
darts_path="$bin_dir/darts-clone"

"$mkdarts_path" KeyFile IndexFile
if [ $? -ne 0 ]
then
	echo "Error: $mkdarts_path failed"
	exit 1
fi

cmp IndexFile CorrectIndexFile
if [ $? -ne 0 ]
then
	echo "Error: incorrect index file"
	exit 1
fi

echo "Done! $mkdarts_path"
	
"$darts_path" IndexFile < TextFile > ResultFile
if [ $? -ne 0 ]
then
	echo "Error: $darts_path failed"
	exit 1
fi

cat CorrectResultFile | cmp ResultFile
if [ $? -ne 0 ]
then
	echo "Error: incorrect result file"
	exit 1
fi

echo "Done! $darts_path"
