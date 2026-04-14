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

#include "field.h"
#include "sql_type.h"

class Type_handler_money : public Type_handler_double
{
public:
  protocol_send_type_t protocol_send_type() const override
  {
    return PROTOCOL_SEND_STRING;
  }

  const Type_collection *type_collection() const override;
  bool Column_definition_data_type_info_image(Binary_string *to,
                                              const Column_definition &def)
                                              const override;

  Field *make_table_field_from_def(TABLE_SHARE *share,
                                   MEM_ROOT *root,
                                   const LEX_CSTRING *name,
                                   const Record_addr &rec,
                                   const Bit_addr &bit,
                                   const Column_definition_attributes *attr,
                                   uint32 flags) const override;
};

extern Type_handler_money type_handler_money;

class Field_money : public Field_double
{
public:
  Field_money(const LEX_CSTRING &name, const Record_addr &addr,
              enum utype unireg_check_arg, uint32 len_arg,
              decimal_digits_t dec_arg, bool zero_arg, bool unsigned_arg)
    : Field_double(addr.ptr(), len_arg, addr.null_ptr(), addr.null_bit(),
                   unireg_check_arg, &name, dec_arg, zero_arg, unsigned_arg)
  {}

  const Type_handler *type_handler() const override { return &type_handler_money; }
  bool send(Protocol *protocol) override;
  void make_send_field(Send_field *field) override;
};

