for file in *.{1,3}
do
  rman  -f  html  -r  '%s.%s.html'  $file > html/$file.html
done
