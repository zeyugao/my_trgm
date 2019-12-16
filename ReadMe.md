# MyTrgm

A limited implement of 3-GRAM algorithm for calculating the similarity between two strings for MySQL.

From pg_trgm

# Usage

## Install

Compile the library
```shell
(Unix x86) gcc -o my_trgm.so -shared my_trgm.c `mysql_config --include`
(Unix x64) gcc -o my_trgm.so -fPIC -shared my_trgm.c `mysql_config --include`
(macOS) gcc -bundle -o my_trgm.so my_trgm.c `mysql_config --include`
cp my_trgm.so `mysql_config --plugindir` # Maybe sudo required
```

Run this in MySQL
```sql
CREATE FUNCTION trgm_similarity RETURNS REAL SONAME 'my_trgm.so';
```

### Install by Make command

```
make my_trgm.so && make copy && make install # Maybe sudo required
```

## Example

```sql
mysql> select trgm_similarity('123', '1234');
+--------------------------------+
| trgm_similarity('123', '1234') |
+--------------------------------+
|                            0.5 |
+--------------------------------+
1 row in set (0.00 sec)

mysql> select trgm_similarity('1 23 321','12 413 234');
+------------------------------------------+
| trgm_similarity('1 23 321','12 413 234') |
+------------------------------------------+
|                      0.17647058823529413 |
+------------------------------------------+
1 row in set (0.00 sec)

```
## Test (Run as a standalone program)
```shell
gcc my_trgm.c -DSTANDALONE -o my_trgm
```

## Uninstall
Run this in MySQL
```sql
DROP FUNCTION trgm_similarity;
```

Then
```shell
rm `mysql_config --plugindir`/my_trgm.so # Maybe sudo required
```