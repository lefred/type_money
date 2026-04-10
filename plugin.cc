/* Copyright (c) 2026, MariaDB Foundation

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#include "sql_type_money.h"
#include <mysql/plugin_data_type.h>
#include <mysql/plugin_function.h>

static Type_handler_money type_handler_money;

const Type_collection *Type_handler_money::type_collection() const
{
  return Type_handler_double::type_collection();
}

Field *Type_handler_money::make_table_field_from_def(TABLE_SHARE *share,
                                                     MEM_ROOT *root,
                                                     const LEX_CSTRING *name,
                                                     const Record_addr &rec,
                                                     const Bit_addr &bit,
                                                     const Column_definition_attributes *attr,
                                                     uint32 flags) const
{
  return new (root) Field_money(*name, rec,
                                Field::NONE,
                                attr->length,
                                attr->decimals,
                                false,
                                f_is_dec(attr->pack_flag) == 0);
}

const Type_handler *Field_money::type_handler() const
{
  return &type_handler_money;
}

int Field_money::store(const char *to, size_t length, CHARSET_INFO *charset)
{
  char buf[128];
  size_t n= length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
  memcpy(buf, to, n);
  buf[n]= '\0';
  return store(my_strtod(buf, nullptr, &errno));
}

int Field_money::store(double nr)
{
  memcpy(ptr, &nr, sizeof(nr));
  return 0;
}

int Field_money::store(longlong nr, bool unsigned_val)
{
  double d= unsigned_val ? (double) (ulonglong) nr : (double) nr;
  return store(d);
}

double Field_money::val_real()
{
  double d;
  memcpy(&d, ptr, sizeof(d));
  return d;
}

longlong Field_money::val_int()
{
  return (longlong) val_real();
}

String *Field_money::val_str(String *to, String *tmp)
{
  char buf[64];
  my_snprintf(buf, sizeof(buf), "%.*f", dec, val_real());
  to->copy(buf, strlen(buf), &my_charset_latin1);
  return to;
}

int Field_money::cmp(const uchar *a, const uchar *b) const
{
  double da, db;
  memcpy(&da, a, sizeof(da));
  memcpy(&db, b, sizeof(db));
  if (da < db) return -1;
  if (da > db) return 1;
  return 0;
}

void Field_money::sort_string(uchar *buff, uint length)
{
  double d= val_real();
  size_t copy_len= length < sizeof(d) ? length : sizeof(d);
  memcpy(buff, &d, copy_len);
  if (length > copy_len)
    bzero(buff + copy_len, length - copy_len);
}

static struct st_mariadb_data_type plugin_descriptor_type_money=
{
  MariaDB_DATA_TYPE_INTERFACE_VERSION,
  &type_handler_money
};

maria_declare_plugin(type_money)
{
  MariaDB_DATA_TYPE_PLUGIN,
  &plugin_descriptor_type_money,
  "money",
  "lefred",
  "Data type MONEY",
  PLUGIN_LICENSE_GPL,
  0,
  0,
  0x0001,
  NULL,
  NULL,
  "0.1",
  MariaDB_PLUGIN_MATURITY_EXPERIMENTAL
}
maria_declare_plugin_end;
