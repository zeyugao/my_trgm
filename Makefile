
my_trgm.so: my_trgm.c
	gcc -O2 -o my_trgm.so -shared my_trgm.c `mysql_config --include`

copy:
	cp my_trgm.so `mysql_config --plugindir`

install:
	echo "CREATE FUNCTION trgm_similarity RETURNS REAL SONAME 'my_trgm.so';" | mysql