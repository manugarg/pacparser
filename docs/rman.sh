for file in *.3
do
  rman  -f  html  -r  '%s.%s.html'  $file > html/$file.html
done
