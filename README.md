# Code example on how to extend MariaDB using the Type_Handler framework

This is the code of part5, our final article of the series. This time we use the `COMMENT` attribute of the column to specify the output format:

```
MariaDB [test]> install soname 'type_money';
Query OK, 0 rows affected (0.012 sec

MariaDB [test]> use test;
Database changed

MariaDB [test]> create table t1 (id int auto_increment primary key, amount money(10,2) comment 'currency=EUR;format=currenty_last;decimal=,;thousands=space', `credit` money(10,2) DEFAULT NULL COMMENT 'currency=$;format=currency_first;decimal=.;thousands=,') engine=innodb;
Query OK, 0 rows affected (0.006 sec)

MariaDB [test]> show create table t1\G
*************************** 1. row ***************************
       Table: t1
Create Table: CREATE TABLE `t1` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `amount` money(10,2) DEFAULT NULL COMMENT 'currency=EUR;format=currenty_last;decimal=,;thousands=space',
  `credit` money(10,2) DEFAULT NULL COMMENT 'currency=$;format=currency_first;decimal=.;thousands=,',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci
1 row in set (0.011 sec)


MariaDB [test]> insert into t1 (amount,credit) values (41578.4,2342);
Query OK, 1 row affected (0.011 sec)

MariaDB [test]> insert into t1 (amount,credit) values (24.4,12.50);
Query OK, 1 row affected (0.013 sec)

MariaDB [test]> insert into t1 (amount,credit) values (15678.309,-1234.299);
Query OK, 1 row affected (0.006 sec)


MariaDB [test]> select * from t1;
+----+---------------+------------+
| id | amount        | credit     |
+----+---------------+------------+
|  1 | 41 578,40 EUR | $2,342.00  |
|  2 | 15 678,31 EUR | -$1,234.30 |
|  3 | 24,40 EUR     | $12.50     |
+----+---------------+------------+
3 rows in set (0.010 sec)
```



