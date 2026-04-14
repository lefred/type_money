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
#include "protocol.h"
#include <float.h>
#include <mysql/plugin_data_type.h>
#include <mysql/plugin_function.h>
#include "filesort.h"

Type_handler_money type_handler_money;
static constexpr Name type_name={STRING_WITH_LEN("money")};

const Type_collection *Type_handler_money::type_collection() const
{
  return Type_handler_double::type_collection();
}

bool Type_handler_money::Column_definition_data_type_info_image(
  Binary_string *to, const Column_definition &def) const
{
  (void) def;
  return to->append(Type_handler_money::name().lex_cstring());
}

Field *Type_handler_money::make_table_field_from_def(TABLE_SHARE *share,
                                                     MEM_ROOT *root,
                                                     const LEX_CSTRING *name,
                                                     const Record_addr &rec,
                                                     const Bit_addr &bit,
                                                     const Column_definition_attributes *attr,
                                                     uint32 flags) const
{
  (void) share;
  (void) bit;
  (void) flags;
  return new (root) Field_money(*name,
                                rec,
                                attr->unireg_check,
                                attr->length,
                                attr->decimals,
                                f_is_zerofill(attr->pack_flag) != 0,
                                f_is_dec(attr->pack_flag) == 0);
}

void Field_money::make_send_field(Send_field *field)
{
  Field::make_send_field(field);
  field->set_handler(&type_handler_varchar);
  field->set_data_type_name(LEX_CSTRING{STRING_WITH_LEN("money")});
  field->length= MY_MAX(field->length, (ulong) (field_length + 32));
  field->decimals= 0;
}

int Field_money::store(const char *from, size_t len, CHARSET_INFO *cs)
{
  int error;
  store(get_double(from, len, cs, &error));
  return error;
}

int Field_money::store(double nr)
{
  DBUG_ASSERT(marked_for_write_or_computed());
  int error= truncate_double(&nr, field_length,
                             not_fixed ? NOT_FIXED_DEC : dec,
                             unsigned_flag, DBL_MAX);
  if (unlikely(error))
  {
    set_warning(ER_WARN_DATA_OUT_OF_RANGE, 1);
    if (error < 0)
    {
      error= 1;
      set_null();
    }
  }

  float8store(ptr, nr);
  return error;
}

int Field_money::store(longlong nr, bool unsigned_val)
{
  return store(unsigned_val ? ulonglong2double((ulonglong) nr) : (double) nr);
}

double Field_money::val_real()
{
  DBUG_ASSERT(marked_for_read());
  double value;
  float8get(value, ptr);
  return value;
}

longlong Field_money::val_int()
{
  Converter_double_to_longlong_with_warn conv(get_thd(), val_real(), false);
  return conv.result();
}

String *Field_money::val_str(String *val_buffer,
                             String *val_ptr __attribute__((unused)))
{
  DBUG_ASSERT(marked_for_read());
  DBUG_ASSERT(!zerofill || field_length <= MAX_FIELD_CHARLENGTH);

  const double value= val_real();
  uint to_length= 128;
  if (val_buffer->alloc(to_length))
    return val_buffer;

  char *to= (char *) val_buffer->ptr();
  size_t len;
  if (dec >= FLOATING_POINT_DECIMALS)
    len= my_gcvt(value, MY_GCVT_ARG_DOUBLE, to_length - 1, to, NULL);
  else
    len= my_fcvt(value, dec, to, NULL);

  val_buffer->length((uint) len);
  if (zerofill)
    prepend_zeros(val_buffer);
  val_buffer->set_charset(&my_charset_numeric);
  return val_buffer;
}

bool Field_money::send(Protocol *protocol)
{
  DBUG_ASSERT(marked_for_read());

  String numeric_buf;
  String money_buf;
  String *numeric= val_str(&numeric_buf, &numeric_buf);

  money_buf.set_charset(numeric->charset());
  if (money_buf.append('$') || money_buf.append(*numeric))
    return true;

  return protocol->store(&money_buf);
}

int Field_money::cmp(const uchar *a_ptr, const uchar *b_ptr) const
{
  double a, b;
  float8get(a, a_ptr);
  float8get(b, b_ptr);
  return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void Field_money::sort_string(uchar *buff, uint length __attribute__((unused)))
{
  double nr;
  float8get(nr,ptr);
  change_double_for_sort(nr, buff);

  // Invert key bytes so lexicographic ascending comparison yields descending order.
  for (uint i= 0; i < sizeof(double); i++)
    buff[i]^= 0xFF;
}

Binlog_type_info Field_money::binlog_type_info() const
{
  DBUG_ASSERT(Field_money::type() == binlog_type());
  return Binlog_type_info(Field_money::type(), pack_length(), 1,
                          binlog_signedness());
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
  type_name.ptr(),
  "lefred",
  "Data type MONEY",
  PLUGIN_LICENSE_GPL,
  0,
  0,
  0x0002,
  NULL,
  NULL,
  "0.3",
  MariaDB_PLUGIN_MATURITY_EXPERIMENTAL
}
maria_declare_plugin_end;
