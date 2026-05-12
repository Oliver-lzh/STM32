declare -i P=100*$1/$2
if [ $P -lt 10 ]; then S1=" "; fi
if [ $P -lt 100 ]; then S2=" "; fi
echo "[$S1$S2$P%] $3"