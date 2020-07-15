#/bin/bash

cc -std=c11 -g -ggdb $1
if (($?))
then
      echo ERROR && exit 1
fi
./a.out



