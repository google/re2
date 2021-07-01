# RE2 regular expression syntax reference

This page lists the regular expression syntax accepted by RE2.

It also lists syntax accepted by PCRE, PERL, and VIM.

~~Strike out~~ expressions are not supported by RE2.

## Single characters:

| Syntax         | Description                                        |
| -------------- | -------------------------------------------------- |
| `.`            | any character, possibly including newline (s=true) |
| `[xyz]`        | character class                                    |
| `[^xyz]`       | negated character class                            |
| `\d`           | Perl character class                               |
| `\D`           | negated Perl character class                       |
| `[[:alpha:]]`  | ASCII character class                              |
| `[[:^alpha:]]` | negated ASCII character class                      |
| `\pN`          | Unicode character class (one-letter name)          |
| `\p{Greek}`    | Unicode character class                            |
| `\PN`          | negated Unicode character class (one-letter name)  |
| `\P{Greek}`    | negated Unicode character class                    |

## Composites:

| Syntax | Description             |
| ------ | ----------------------- |
| `xy`   | `x` followed by `y`     |
| `x\|y` | `x` or `y` (prefer `x`) |

## Repetitions:

| Syntax      | Description                                  |
| ----------- | -------------------------------------------- |
| `x*`        | zero or more `x`, prefer more                |
| `x+`        | one or more `x`, prefer more                 |
| `x?`        | zero or one `x`, prefer one                  |
| `x{n,m}`    | `n` or `n`+1 or ... or `m` `x`, prefer more  |
| `x{n,}`     | `n` or more `x`, prefer more                 |
| `x{n}`      | exactly `n` `x`                              |
| `x*?`       | zero or more `x`, prefer fewer               |
| `x+?`       | one or more `x`, prefer fewer                |
| `x??`       | zero or one `x`, prefer zero                 |
| `x{n,m}?`   | `n` or `n`+1 or ... or `m` `x`, prefer fewer |
| `x{n,}?`    | `n` or more `x`, prefer fewer                |
| `x{n}?`     | exactly `n` `x`                              |
| ~~`x{}`~~   | (≡ `x*`) VIM                                 |
| ~~`x{-}`~~  | ≡ `x*?`) VIM                                 |
| ~~`x{-n}`~~ | (≡ `x{n}?`) VIM                              |
| ~~`x=`~~    | (≡ `x?`) VIM                                 |

Implementation restriction: The counting forms `x{n,m}`, `x{n,}`, and
`x{n}`

reject forms that create a minimum or maximum repetition count above 1000.

Unlimited repetitions are not subject to this restriction.

## Possessive repetitions:

| Syntax        | Description                       |
| ------------- | --------------------------------- |
| ~~`x*+`~~     | zero or more `x`, possessive      |
| ~~`x++`~~     | one or more `x`, possessive       |
| ~~`x?+`~~     | zero or one `x`, possessive       |
| ~~`x{n,m}+`~~ | `n` or ... or `m` `x`, possessive |
| ~~`x{n,}+`~~  | `n` or more `x`, possessive       |
| ~~`x{n}+`~~   | exactly `n` `x`, possessive       |

## Grouping:

| Syntax             | Description                                   |
| ------------------ | --------------------------------------------- |
| `(re)`             | numbered capturing group (submatch)           |
| `(?P<name>re)`     | named & numbered capturing group (submatch)   |
| ~~`(?<name>re)`~~  | named & numbered capturing group (submatch)   |
| ~~`(?'name're)`~~  | named & numbered capturing group (submatch)   |
| `(?:re)`           | non-capturing group                           |
| `(?flags)`         | set flags within current group; non-capturing |
| `(?flags:re)`      | set flags during re; non-capturing            |
| ~~`(?#text)`~~     | comment                                       |
| ~~`(?\|x\|y\|z)`~~ | branch numbering reset                        |
| ~~`(?>re)`~~       | possessive match of `re`                      |
| ~~`re@>`~~         | possessive match of `re` VIM                  |
| ~~`%(re)`~~        | non-capturing group VIM                       |

## Flags:

| Syntax | Description                                                                                     |
| ------ | ----------------------------------------------------------------------------------------------- |
| `i`    | case-insensitive (default false)                                                                |
| `m`    | multi-line mode: `^` and `$` match begin/end line in addition to begin/end text (default false) |
| `s`    | let `.` match `\n` (default false)                                                              |
| `U`    | ungreedy: swap meaning of `x*` and `x*?`, `x+` and `x+?`, etc (default false)                   |

Flag syntax is `xyz` (set) or `-xyz` (clear) or `xy-z` (set `xy`, clear
`z`).

## Empty strings:

| Syntax        | Description                                                                    |
| ------------- | ------------------------------------------------------------------------------ |
| `^`           | at beginning of text or line (`m`=true)                                        |
| `$`           | at end of text (like `\z` not `\Z`) or line (`m`=true)                         |
| `\A`          | at beginning of text                                                           |
| `\b`          | at ASCII word boundary (`\w` on one side and `\W`, `\A`, or `\z` on the other) |
| `\B`          | not at ASCII word boundary                                                     |
| ~~`\G`~~      | at beginning of subtext being searched PCRE                                    |
| ~~`\G`~~      | at end of last match PERL                                                      |
| ~~`\Z`~~      | at end of text, or before newline at end of text                               |
| `\z`          | at end of text                                                                 |
| ~~`(?=re)`~~  | before text matching `re`                                                      |
| ~~`(?!re)`~~  | before text not matching `re`                                                  |
| ~~`(?<=re)`~~ | after text matching `re`                                                       |
| ~~`(?<!re)`~~ | after text not matching `re`                                                   |
| ~~`re&`~~     | before text matching `re` VIM                                                  |
| ~~`re@=`~~    | before text matching `re` VIM                                                  |
| ~~`re@!`~~    | before text not matching `re` VIM                                              |
| ~~`re@<=`~~   | after text matching `re` VIM                                                   |
| ~~`re@<!`~~   | after text not matching `re` VIM                                               |
| ~~`\zs`~~     | sets start of match (`= \K`) VIM                                               |
| ~~`\ze`~~     | sets end of match VIM                                                          |
| ~~`\%^`~~     | beginning of file VIM                                                          |
| ~~`\%$`~~     | end of file VIM                                                                |
| ~~`\%V`~~     | on screen VIM                                                                  |
| ~~`\%#`~~     | cursor position VIM                                                            |
| ~~`\%'m`~~    | mark `m` position VIM                                                          |
| ~~`\%23l`~~   | in line 23 VIM                                                                 |
| ~~`\%23c`~~   | in column 23 VIM                                                               |
| ~~`\%23v`~~   | in virtual column 23 VIM                                                       |

## Escape sequences:

| Syntax            | Description                                      |
| ----------------- | ------------------------------------------------ |
| `\a`              | bell (≡ `\007`)                                  |
| `\f`              | form feed (≡ `\014`)                             |
| `\t`              | horizontal tab (≡ `\011`)                        |
| `\n`              | newline (≡ `\012`)                               |
| `\r`              | carriage return (≡ `\015`)                       |
| `\v`              | vertical tab character (≡ `\013`)                |
| `\*`              | literal `*`, for any punctuation character `*`   |
| `\123`            | octal character code (up to three digits)        |
| `\x7F`            | hex character code (exactly two digits)          |
| `\x{10FFFF}`      | hex character code                               |
| `\C`              | match a single byte even in UTF-8 mode           |
| `\Q...\E`         | literal text `...` even if `...` has punctuation |
| ~~`\1`~~          | backreference                                    |
| ~~`\b`~~          | backspace (use `\010`)                           |
| ~~`\cK`~~         | control char \^K (use `\001` etc)                |
| ~~`\e`~~          | escape (use `\033`)                              |
| ~~`\g1`~~         | backreference                                    |
| ~~`\g{1}`~~       | backreference                                    |
| ~~`\g{+1}`~~      | backreference                                    |
| ~~`\g{-1}`~~      | backreference                                    |
| ~~`\g{name}`~~    | named backreference                              |
| ~~`\g<name>`~~    | subroutine call                                  |
| ~~`\g'name`~~`    | subroutine call                                  |
| ~~`\k<name>`~~    | named backreference                              |
| ~~`\k'name`~~`    | named backreference                              |
| ~~`\lX`~~         | lowercase `X`                                    |
| ~~`\ux`~~         | uppercase `x`                                    |
| ~~`\L...\E`~~     | lowercase text `...`                             |
| ~~`\K`~~          | reset beginning of `$0`                          |
| ~~`\N{name}`~~    | named Unicode character                          |
| ~~`\R`~~          | line break                                       |
| ~~`\U...\E`~~     | upper case text `...`                            |
| ~~`\X`~~          | extended Unicode sequence                        |
| ~~`\%d123`~~      | decimal character 123 VIM                        |
| ~~`\%xFF`~~       | hex character FF VIM                             |
| ~~`\%o123`~~      | octal character 123 VIM                          |
| ~~`\%u1234`~~     | Unicode character 0x1234 VIM                     |
| ~~`\%U12345678`~~ | Unicode character 0x12345678 VIM                 |

## Character class elements:

| Syntax    | Description                                   |
| --------- | --------------------------------------------- |
| `x`       | single character                              |
| `A-Z`     | character range (inclusive)                   |
| `\d`      | Perl character class                          |
| `[:foo:]` | ASCII character class `foo`                   |
| `\p{Foo}` | Unicode character class `Foo`                 |
| `\pF`     | Unicode character class `F` (one-letter name) |

## Named character classes as character class elements:

| Syntax        | Description                                                          |
| ------------- | -------------------------------------------------------------------- |
| `[\d]`        | digits (≡ `\d`)                                                      |
| `[^\d]`       | not digits (≡ `\D`)                                                  |
| `[\D]`        | not digits (≡ `\D`)                                                  |
| `[^\D]`       | not not digits (≡ `\d`)                                              |
| `[[:name:]]`  | named ASCII class inside character class (≡ `[:name:]`)              |
| `[^[:name:]]` | named ASCII class inside negated character class (≡ `[:^name:]`)     |
| `[\p{Name}]`  | named Unicode property inside character class (≡ `\p{Name}`)         |
| `[^\p{Name}]` | named Unicode property inside negated character class (≡ `\P{Name}`) |

## Perl character classes (all ASCII-only):

| Syntax   | Description                             |
| -------- | --------------------------------------- |
| `\d`     | digits (≡ `[0-9]`)                      |
| `\D`     | not digits (≡ `[^0-9]`)                 |
| `\s`     | whitespace (≡ `[\t\n\f\r ]`)            |
| `\S`     | not whitespace (≡ `[^\t\n\f\r ]`)       |
| `\w`     | word characters (≡ `[0-9A-Za-z_]`)      |
| `\W`     | not word characters (≡ `[^0-9A-Za-z_]`) |
| ~~`\h`~~ | horizontal space                        |
| ~~`\H`~~ | not horizontal space                    |
| ~~`\v`~~ | vertical space                          |
| ~~`\V`~~ | not vertical space                      |

## ASCII character classes:

| Syntax         | Description                                                                  |
| -------------- | ---------------------------------------------------------------------------- |
| `[[:alnum:]]`  | alphanumeric (≡ `[0-9A-Za-z]`)                                               |
| `[[:alpha:]]`  | alphabetic (≡ `[A-Za-z]`)                                                    |
| `[[:ascii:]]`  | ASCII (≡ `[\x00-\x7F]`)                                                      |
| `[[:blank:]]`  | blank (≡ `[\t ]`)                                                            |
| `[[:cntrl:]]`  | control (≡ `[\x00-\x1F\x7F]`)                                                |
| `[[:digit:]]`  | digits (≡ `[0-9]`)                                                           |
| `[[:graph:]]`  | graphical (≡ `` [!-~] == [A-Za-z0-9!"#$%&'()*+,\-./:;<=>?@[\\\]^_`{\|}~] ``) |
| `[[:lower:]]`  | lower case (≡ `[a-z]`)                                                       |
| `[[:print:]]`  | printable (≡ `[ -~] == [ [:graph:]]`)                                        |
| `[[:punct:]]`  | punctuation (≡ `` [!-/:-@[-`{-~] ``)                                         |
| `[[:space:]]`  | whitespace (≡ `[\t\n\v\f\r ]`)                                               |
| `[[:upper:]]`  | upper case (≡ `[A-Z]`)                                                       |
| `[[:word:]]`   | word characters (≡ `[0-9A-Za-z_]`)                                           |
| `[[:xdigit:]]` | hex digit (≡ `[0-9A-Fa-f]`)                                                  |

## Unicode character class names--general category:

| Syntax   | Description            |
| -------- | ---------------------- |
| `C`      | other                  |
| `Cc`     | control                |
| `Cf`     | format                 |
| ~~`Cn`~~ | unassigned code points |
| `Co`     | private use            |
| `Cs`     | surrogate              |
| `L`      | letter                 |
| ~~`LC`~~ | cased letter           |
| ~~`L&`~~ | cased letter           |
| `Ll`     | lowercase letter       |
| `Lm`     | modifier letter        |
| `Lo`     | other letter           |
| `Lt`     | titlecase letter       |
| `Lu`     | uppercase letter       |
| `M`      | mark                   |
| `Mc`     | spacing mark           |
| `Me`     | enclosing mark         |
| `Mn`     | non-spacing mark       |
| `N`      | number                 |
| `Nd`     | decimal number         |
| `Nl`     | letter number          |
| `No`     | other number           |
| `P`      | punctuation            |
| `Pc`     | connector punctuation  |
| `Pd`     | dash punctuation       |
| `Pe`     | close punctuation      |
| `Pf`     | final punctuation      |
| `Pi`     | initial punctuation    |
| `Po`     | other punctuation      |
| `Ps`     | open punctuation       |
| `S`      | symbol                 |
| `Sc`     | currency symbol        |
| `Sk`     | modifier symbol        |
| `Sm`     | math symbol            |
| `So`     | other symbol           |
| `Z`      | separator              |
| `Zl`     | line separator         |
| `Zp`     | paragraph separator    |
| `Zs`     | space separator        |

## Unicode character class names--scripts:

| Script Name            |
| ---------------------- |
| Adlam                  |
| Ahom                   |
| Anatolian_Hieroglyphs  |
| Arabic                 |
| Armenian               |
| Avestan                |
| Balinese               |
| Bamum                  |
| Bassa_Vah              |
| Batak                  |
| Bengali                |
| Bhaiksuki              |
| Bopomofo               |
| Brahmi                 |
| Braille                |
| Buginese               |
| Buhid                  |
| Canadian_Aboriginal    |
| Carian                 |
| Caucasian_Albanian     |
| Chakma                 |
| Cham                   |
| Cherokee               |
| Chorasmian             |
| Common                 |
| Coptic                 |
| Cuneiform              |
| Cypriot                |
| Cyrillic               |
| Deseret                |
| Devanagari             |
| Dives_Akuru            |
| Dogra                  |
| Duployan               |
| Egyptian_Hieroglyphs   |
| Elbasan                |
| Elymaic                |
| Ethiopic               |
| Georgian               |
| Glagolitic             |
| Gothic                 |
| Grantha                |
| Greek                  |
| Gujarati               |
| Gunjala_Gondi          |
| Gurmukhi               |
| Han                    |
| Hangul                 |
| Hanifi_Rohingya        |
| Hanunoo                |
| Hatran                 |
| Hebrew                 |
| Hiragana               |
| Imperial_Aramaic       |
| Inherited              |
| Inscriptional_Pahlavi  |
| Inscriptional_Parthian |
| Javanese               |
| Kaithi                 |
| Kannada                |
| Katakana               |
| Kayah_Li               |
| Kharoshthi             |
| Khitan_Small_Script    |
| Khmer                  |
| Khojki                 |
| Khudawadi              |
| Lao                    |
| Latin                  |
| Lepcha                 |
| Limbu                  |
| Linear_A               |
| Linear_B               |
| Lisu                   |
| Lycian                 |
| Lydian                 |
| Mahajani               |
| Makasar                |
| Malayalam              |
| Mandaic                |
| Manichaean             |
| Marchen                |
| Masaram_Gondi          |
| Medefaidrin            |
| Meetei_Mayek           |
| Mende_Kikakui          |
| Meroitic_Cursive       |
| Meroitic_Hieroglyphs   |
| Miao                   |
| Modi                   |
| Mongolian              |
| Mro                    |
| Multani                |
| Myanmar                |
| Nabataean              |
| Nandinagari            |
| New_Tai_Lue            |
| Newa                   |
| Nkov                   |
| Nushu                  |
| Nyiakeng_Puachue_Hmong |
| Ogham                  |
| Ol_Chiki               |
| Old_Hungarian          |
| Old_Italic             |
| Old_North_Arabian      |
| Old_Permic             |
| Old_Persian            |
| Old_Sogdian            |
| Old_South_Arabian      |
| Old_Turkic             |
| Oriya                  |
| Osage                  |
| Osmanya                |
| Pahawh_Hmong           |
| Palmyrene              |
| Pau_Cin_Hau            |
| Phags_Pa               |
| Phoenician             |
| Psalter_Pahlavi        |
| Rejang                 |
| Runic                  |
| Samaritan              |
| Saurashtra             |
| Sharada                |
| Shavian                |
| Siddham                |
| SignWriting            |
| Sinhala                |
| Sogdian                |
| Sora_Sompeng           |
| Soyombo                |
| Sundanese              |
| Syloti_Nagri           |
| Syriac                 |
| Tagalog                |
| Tagbanwa               |
| Tai_Le                 |
| Tai_Tham               |
| Tai_Viet               |
| Takri                  |
| Tamil                  |
| Tangut                 |
| Telugu                 |
| Thaana                 |
| Thai                   |
| Tibetan                |
| Tifinagh               |
| Tirhuta                |
| Ugaritic               |
| Vai                    |
| Wancho                 |
| Warang_Citi            |
| Yezidi                 |
| Yi                     |
| Zanabazar_Square       |

## Vim character classes:

| Syntax    | Description                                |
| --------- | ------------------------------------------ |
| ~~`\i`~~  | identifier character VIM                   |
| ~~`\I`~~  | `\i` except digits VIM                     |
| ~~`\k`~~  | keyword character VIM                      |
| ~~`\K`~~  | `\k` except digits VIM                     |
| ~~`\f`~~  | file name character VIM                    |
| ~~`\F`~~  | `\f` except digits VIM                     |
| ~~`\p`~~  | printable character VIM                    |
| ~~`\P`~~  | `\p` except digits VIM                     |
| ~~`\s`~~  | whitespace character (≡ `[ \t]`) VIM       |
| ~~`\S`~~  | non-white space character (≡ `[^ \t]`) VIM |
| `\d`      | digits (≡ `[0-9]`) VIM                     |
| `\D`      | not `\d` VIM                               |
| ~~`\x`~~  | hex digits (≡ `[0-9A-Fa-f]`) VIM           |
| ~~`\X`~~  | not `\x` VIM                               |
| ~~`\o`~~  | octal digits (≡ `[0-7]`) VIM               |
| ~~`\O`~~  | not `\o` VIM                               |
| `\w`      | word character VIM                         |
| `\W`      | not `\w` VIM                               |
| ~~`\h`~~  | head of word character VIM                 |
| ~~`\H`~~  | not `\h` VIM                               |
| ~~`\a`~~  | alphabetic VIM                             |
| ~~`\A`~~  | not `\a` VIM                               |
| ~~`\l`~~  | lowercase VIM                              |
| ~~`\L`~~  | not lowercase VIM                          |
| ~~`\u`~~  | uppercase VIM                              |
| ~~`\U`~~  | not uppercase VIM                          |
| ~~`\_x`~~ | `\x` plus newline, for any `x` VIM         |

## Vim flags:

| Syntax   | Description                                            |
| -------- | ------------------------------------------------------ |
| ~~`\c`~~ | ignore case VIM                                        |
| ~~`\C`~~ | match case VIM                                         |
| ~~`\m`~~ | magic VIM                                              |
| ~~`\M`~~ | nomagic VIM                                            |
| ~~`\v`~~ | verymagic VIM                                          |
| ~~`\V`~~ | verynomagic VIM                                        |
| ~~`\Z`~~ | ignore differences in Unicode combining characters VIM |

## Magic:

| Syntax                     | Description                                  |
| -------------------------- | -------------------------------------------- |
| ~~`(?{code})`~~            | arbitrary Perl code PERL                     |
| ~~`(??{code})`~~           | postponed arbitrary Perl code PERL           |
| ~~`(?n)`~~                 | recursive call to regexp capturing group `n` |
| ~~`(?+n)`~~                | recursive call to relative group `+n`        |
| ~~`(?-n)`~~                | recursive call to relative group `-n`        |
| ~~`(?C)`~~                 | PCRE callout PCRE                            |
| ~~`(?R)`~~                 | recursive call to entire regexp (≡ `(?0)`)   |
| ~~`(?&name)`~~             | recursive call to named group                |
| ~~`(?P=name)`~~            | named backreference                          |
| ~~`(?P>name)`~~            | recursive call to named group                |
| ~~`(?(cond)true\|false)`~~ | conditional branch                           |
| ~~`(?(cond)true)`~~        | conditional branch                           |
| ~~`(*ACCEPT)`~~            | make regexps more like Prolog                |
| ~~`(*COMMIT)`~~            |                                              |
| ~~`(*F)`~~                 |                                              |
| ~~`(*FAIL)`~~              |                                              |
| ~~`(*MARK)`~~              |                                              |
| ~~`(*PRUNE)`~~             |                                              |
| ~~`(*SKIP)`~~              |                                              |
| ~~`(*THEN)`~~              |                                              |
| ~~`(*ANY)`~~               | set newline convention                       |
| ~~`(*ANYCRLF)`~~           |                                              |
| ~~`(*CR)`~~                |                                              |
| ~~`(*CRLF)`~~              |                                              |
| ~~`(*LF)`~~                |                                              |
| ~~`(*BSR_ANYCRLF)`~~       | set \\R convention PCRE                      |
| ~~`(*BSR_UNICODE)`~~       | PCRE                                         |
