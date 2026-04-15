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
#include "money.h"

#ifndef DOUBLE_TO_STRING_CONVERSION_BUFFER_SIZE
#define DOUBLE_TO_STRING_CONVERSION_BUFFER_SIZE 320
#endif

Type_handler_money type_handler_money;
static constexpr Name type_name={STRING_WITH_LEN("money")};

static void money_set_default_display_config(Field_money::money_display_config *cfg)
{
  bzero(cfg, sizeof(*cfg));
  cfg->decimal_sep= '.';
  cfg->thousands_sep= ',';
  cfg->allow_grouping= false;
  cfg->style= Field_money::MONEY_FMT_CURRENCY_LAST;
}

static void money_apply_thousands_option(const char *v, size_t v_len,
                                         Field_money::money_display_config *cfg)
{
  if (money_equals_ci_n(v, v_len, "none", 4))
  {
    cfg->allow_grouping= false;
    cfg->thousands_sep= 0;
  }
  else if (money_equals_ci_n(v, v_len, "space", 5) ||
           money_equals_ci_n(v, v_len, "blank", 5) ||
           (v_len == 1 && v[0] == '_'))
  {
    cfg->allow_grouping= true;
    cfg->thousands_sep= ' ';
  }
  else if (money_equals_ci_n(v, v_len, "comma", 5) ||
           (v_len == 1 && v[0] == ','))
  {
    cfg->allow_grouping= true;
    cfg->thousands_sep= ',';
  }
  else if (money_equals_ci_n(v, v_len, "dot", 3) ||
           money_equals_ci_n(v, v_len, "period", 6) ||
           (v_len == 1 && v[0] == '.'))
  {
    cfg->allow_grouping= true;
    cfg->thousands_sep= '.';
  }
  else if (v_len >= 1)
  {
    cfg->allow_grouping= true;
    cfg->thousands_sep= v[0];
  }
}

static void money_apply_comment_option(const char *k, size_t k_len,
                                       const char *v, size_t v_len,
                                       Field_money::money_display_config *cfg)
{
  if (money_equals_ci_n(k, k_len, "currency", 8))
  {
    size_t n= v_len < sizeof(cfg->currency) - 1 ? v_len : sizeof(cfg->currency) - 1;
    memcpy(cfg->currency, v, n);
    cfg->currency[n]= '\0';
  }
  else if (money_equals_ci_n(k, k_len, "format", 6))
  {
    if (money_equals_ci_n(v, v_len, "currency_first", 14))
      cfg->style= Field_money::MONEY_FMT_CURRENCY_FIRST;
    else if (money_equals_ci_n(v, v_len, "currency_last", 13))
      cfg->style= Field_money::MONEY_FMT_CURRENCY_LAST;
  }
  else if (money_equals_ci_n(k, k_len, "decimal", 7))
  {
    if (v_len >= 1 && (v[0] == '.' || v[0] == ','))
      cfg->decimal_sep= v[0];
  }
  else if (money_equals_ci_n(k, k_len, "thousands", 9))
    money_apply_thousands_option(v, v_len, cfg);
}

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

bool Field_money::parse_comment_config(money_display_config *cfg) const
{
  money_set_default_display_config(cfg);

  if (!comment.str || !comment.length)
    return true;

  const char *p= comment.str;
  const char *end= comment.str + comment.length;

  while (p < end)
  {
    const char *entry_end= (const char*) memchr(p, ';', (size_t) (end - p));
    if (!entry_end)
      entry_end= end;

    const char *eq= (const char*) memchr(p, '=', (size_t) (entry_end - p));
    if (eq)
    {
      const char *k= money_trim_left(p, eq);
      const char *k_end= money_trim_right_begin(k, eq);
      const char *v= money_trim_left(eq + 1, entry_end);
      const char *v_end= money_trim_right_begin(v, entry_end);
      size_t k_len= (size_t) (k_end - k);
      size_t v_len= (size_t) (v_end - v);
      money_apply_comment_option(k, k_len, v, v_len, cfg);
    }

    p= entry_end < end ? entry_end + 1 : end;
  }

  if (cfg->decimal_sep == cfg->thousands_sep)
  {
    if (cfg->decimal_sep == ',')
      cfg->thousands_sep= '.';
    else
      cfg->thousands_sep= ',';
  }

  return true;
}

static bool money_append_currency(String *to,
                                   const Field_money::money_display_config &cfg,
                                   bool before_number)
{
  if (!cfg.currency[0])
    return false;
  
  size_t currency_len= strlen(cfg.currency);
  bool is_alpha= isalpha((unsigned char) cfg.currency[0]);
  
  if (before_number)
  {
    if (to->append(cfg.currency, (uint32) currency_len))
      return true;
    if (is_alpha && to->append(' '))
      return true;
  }
  else  // after_number
  {
    if (is_alpha && to->append(' '))
      return true;
    if (to->append(cfg.currency, (uint32) currency_len))
      return true;
  }
  return false;
}

void Field_money::format_money_string(String *to, double nr,
                                      const money_display_config &cfg) const
{
  decimal_digits_t out_dec= money_effective_decimals(dec);
  char raw[DOUBLE_TO_STRING_CONVERSION_BUFFER_SIZE];
  size_t raw_len= my_fcvt(nr, out_dec, raw, NULL);
  raw[raw_len]= '\0';
  const char *p= raw;
  bool negative= false;

  if (*p == '-')
  {
    negative= true;
    p++;
  }

  const char *dot= strchr(p, '.');
  size_t int_len= dot ? (size_t) (dot - p) : strlen(p);
  const char *frac= dot ? dot + 1 : "";
  size_t frac_len= dot ? strlen(frac) : 0;

  to->length(0);
  to->set_charset(system_charset_info);

  if (negative && to->append('-'))
    return;

  if (cfg.style == MONEY_FMT_CURRENCY_FIRST && money_append_currency(to, cfg, true))
    return;

  for (size_t i= 0; i < int_len; i++)
  {
    if (cfg.allow_grouping && cfg.thousands_sep && i > 0 && ((int_len - i) % 3) == 0)
    {
      if (to->append(cfg.thousands_sep))
        return;
    }
    if (to->append(p[i]))
      return;
  }

  if (out_dec > 0)
  {
    if (to->append(cfg.decimal_sep))
      return;
    for (uint i= 0; i < out_dec; i++)
    {
      if (to->append((i < frac_len) ? frac[i] : '0'))
        return;
    }
  }

  if (cfg.style == MONEY_FMT_CURRENCY_LAST && money_append_currency(to, cfg, false))
    return;
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

  String money_buf;

  money_display_config cfg;
  parse_comment_config(&cfg);
  format_money_string(&money_buf, Field_money::val_real(), cfg);

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
