#! /bin/sh


../darts-clone-test KeyFile
if [ $? -eq 0 ]
then
	echo "Done! darts-clone-test"
else
	echo "Error: darts-clone-test failed"
fi


if ../mkdarts-clone KeyFile IndexFile
then
	if cmp IndexFile CorrectIndexFile
	then
		echo "Done! mkdarts-clone"
		if ../darts-clone IndexFile < TextFile > ResultFile
		then
			if diff --strip-trailing-cr ResultFile CorrectResultFile
			then
				echo "Done! darts-clone"
				rm -f IndexFile ResultFile
			else
				echo "Error: invalid matching results"
			fi
		else
			echo "Error: darts-clone failed"
		fi
	else
		echo "Error: invalid index file"
	fi
else
	echo "Error: mkdarts-clone failed"
fi
