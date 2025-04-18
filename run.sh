#! /bin/bash
for i in `seq 0 1 $(($3-1))`;
do
	./out/build/linux/src/test $1 -ttp $2 -n $3 -t $4 -id $i -m $5 -i $6 &

	# ./oringt1 $1 $2 $3 $i $4 $5&
	# echo "Running $i..." &
done
