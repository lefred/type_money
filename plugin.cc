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
#include <mysql/plugin_data_type.h>
#include <mysql/plugin_function.h>

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

Field *Type_handler_money::make_table_field(MEM_ROOT *root,
                                            const LEX_CSTRING *name,
                                            const Record_addr &rec,
                                            const Type_all_attributes &attr,
                                            TABLE_SHARE *share) const
{
  Column_definition_attributes dattr(attr);
  return make_table_field_from_def(share, root, name, rec,
                                   Bit_addr(), &dattr, 0);
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

bool Field_money::send(Protocol *protocol)
{
  DBUG_ASSERT(marked_for_read());

  String numeric_buf;
  String money_buf;
  String *numeric= Field_double::val_str(&numeric_buf, &numeric_buf);

  money_buf.set_charset(numeric->charset());
  if (money_buf.append('$') || money_buf.append(*numeric))
    return true;

  return protocol->store(&money_buf);
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
  "0.2",
  MariaDB_PLUGIN_MATURITY_EXPERIMENTAL
}
maria_declare_plugin_end;
