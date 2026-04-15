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

#include <string.h>

static bool money_equals_ci_n(const char *a, size_t a_len,
                              const char *b, size_t b_len)
{
  if (a_len != b_len)
    return false;
  for (size_t i= 0; i < a_len; i++)
  {
    if (toupper((unsigned char) a[i]) != toupper((unsigned char) b[i]))
      return false;
  }
  return true;
}

static const char *money_trim_left(const char *p, const char *end)
{
  while (p < end && my_isspace(&my_charset_latin1, *p))
    p++;
  return p;
}

static const char *money_trim_right_begin(const char *p, const char *end)
{
  while (end > p && my_isspace(&my_charset_latin1, end[-1]))
    end--;
  return end;
}

static decimal_digits_t money_effective_decimals(decimal_digits_t dec)
{
  return dec >= FLOATING_POINT_DECIMALS ? 2 : (dec == 0 ? 2 : dec);
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
