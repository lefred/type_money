# Code example on how to extend MariaDB using the Type_Handler framework

This is the code of part2. We just add a simple data type that behaves like a double:

```
MariaDB [test]> install soname 'type_money';
Query OK, 0 rows affected (0.014 sec)

MariaDB [test]> create table t1 (id int auto_increment primary key, amount money(10,2)) engine=innodb;
Query OK, 0 rows affected (0.014 sec)

MariaDB [test]> show create table t1\G
*************************** 1. row ***************************
       Table: t1
Create Table: CREATE TABLE `t1` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `amount` money(10,2) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci
1 row in set (0.002 sec)

MariaDB [test]> insert into t1 (amount) values (15678.309);
Query OK, 1 row affected (0.006 sec)

MariaDB [test]> select * from t1 order by amount desc;
+----+----------+
| id | amount   |
+----+----------+
|  1 | 15678.31 |
+----+----------+
1 row in set (0.007 sec)
```
