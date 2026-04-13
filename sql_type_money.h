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
  const Type_collection *type_collection() const override;

  Field *make_table_field_from_def(TABLE_SHARE *share,
                                   MEM_ROOT *root,
                                   const LEX_CSTRING *name,
                                   const Record_addr &rec,
                                   const Bit_addr &bit,
                                   const Column_definition_attributes *attr,
                                   uint32 flags) const override;
};

class Field_money : public Field_real
{
public:
  Field_money(const LEX_CSTRING &name, const Record_addr &addr,
              enum utype unireg_check_arg, uint32 len_arg,
              decimal_digits_t dec_arg, bool zero_arg, bool unsigned_arg)
    : Field_real(addr.ptr(), len_arg, addr.null_ptr(), addr.null_bit(),
                 unireg_check_arg, &name, dec_arg, zero_arg, unsigned_arg)
  {}

  const Type_handler *type_handler() const override;

  // Still needed to make the class concrete.
  int store(const char *to, size_t length, CHARSET_INFO *charset) override;
  int store(double nr) override;
  int store(longlong nr, bool unsigned_val) override;

  double val_real() override;
  longlong val_int() override;
  String *val_str(String *to, String *tmp) override;

  int cmp(const uchar *a, const uchar *b) const override;
  void sort_string(uchar *buff, uint length) override;

};

