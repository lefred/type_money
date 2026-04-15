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