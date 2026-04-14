# Code example on how to extend MariaDB using the Type_Handler framework

This is the code of part4. This time we convert our data type from Field_double to Field_real to have the possibility to
modify the sort order. We implemented a reverse sort order.

```
MariaDB [test]> install soname 'type_money';
Query OK, 0 rows affected (0.012 sec)

MariaDB [test]> create table t1 (id int auto_increment primary key, amount money(10,2)) engine=innodb;
Query OK, 0 rows affected (0.008 sec)

MariaDB [test]> insert into t1 (amount) values (15678.309);
Query OK, 1 row affected (0.016 sec)

MariaDB [test]> insert into t1 (amount) values (24.4);
Query OK, 1 row affected (0.003 sec)

MariaDB [test]> insert into t1 (amount) values (41578.4);
Query OK, 1 row affected (0.003 sec)

MariaDB [test]> select * from t1 order by amount;
+----+-----------+
| id | amount    |
+----+-----------+
|  3 | $41578.40 |
|  1 | $15678.31 |
|  2 | $24.40    |
+----+-----------+
3 rows in set (0.002 sec)
```



