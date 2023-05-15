# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
r"""A drop-in replacement for the re module.

It uses RE2 under the hood, of course, so various PCRE features
(e.g. backreferences, look-around assertions) are not supported.
See https://github.com/google/re2/wiki/Syntax for the canonical
reference, but known syntactic "gotchas" relative to Python are:

  * PCRE supports \Z and \z; RE2 supports \z; Python supports \z,
    but calls it \Z. You must rewrite \Z to \z in pattern strings.

Known differences between this module's API and the re module's API:

  * The error class does not provide any error information as attributes.
  * The Options class replaces the re module's flags with RE2's options as
    gettable/settable properties. Please see re2.h for their documentation.
  * The pattern string and the input string do not have to be the same type.
    Any str will be encoded to UTF-8.
  * The pattern string cannot be str if the options specify Latin-1 encoding.

This module's LRU cache contains a maximum of 128 regular expression objects.
Each regular expression object's underlying RE2 object uses a maximum of 8MiB
of memory (by default). Hence, this module's LRU cache uses a maximum of 1GiB
of memory (by default), but in most cases, it should use much less than that.
"""

import codecs
import functools
import itertools

import _re2


class error(Exception):
  pass


class Options(_re2.RE2.Options):

  __slots__ = ()

  NAMES = (
      'max_mem',
      'encoding',
      'posix_syntax',
      'longest_match',
      'log_errors',
      'literal',
      'never_nl',
      'dot_nl',
      'never_capture',
      'case_sensitive',
      'perl_classes',
      'word_boundary',
      'one_line',
  )


def compile(pattern, options=None):
  if isinstance(pattern, _Regexp):
    if options:
      raise error('pattern is already compiled, so '
                  'options may not be specified')
    pattern = pattern._pattern
  options = options or Options()
  values = tuple(getattr(options, name) for name in Options.NAMES)
  return _Regexp._make(pattern, values)


def search(pattern, text, options=None):
  return compile(pattern, options=options).search(text)


def match(pattern, text, options=None):
  return compile(pattern, options=options).match(text)


def fullmatch(pattern, text, options=None):
  return compile(pattern, options=options).fullmatch(text)


def finditer(pattern, text, options=None):
  return compile(pattern, options=options).finditer(text)


def findall(pattern, text, options=None):
  return compile(pattern, options=options).findall(text)


def split(pattern, text, maxsplit=0, options=None):
  return compile(pattern, options=options).split(text, maxsplit)


def subn(pattern, repl, text, count=0, options=None):
  return compile(pattern, options=options).subn(repl, text, count)


def sub(pattern, repl, text, count=0, options=None):
  return compile(pattern, options=options).sub(repl, text, count)


def _encode(t):
  return t.encode(encoding='utf-8')


def _decode(b):
  return b.decode(encoding='utf-8')


def escape(pattern):
  if isinstance(pattern, str):
    encoded_pattern = _encode(pattern)
    escaped = _re2.RE2.QuoteMeta(encoded_pattern)
    decoded_escaped = _decode(escaped)
    return decoded_escaped
  else:
    escaped = _re2.RE2.QuoteMeta(pattern)
    return escaped


def purge():
  return _Regexp._make.cache_clear()


_Anchor = _re2.RE2.Anchor
_NULL_SPAN = (-1, -1)


class _Regexp(object):

  __slots__ = ('_pattern', '_regexp')

  @classmethod
  @functools.lru_cache(typed=True)
  def _make(cls, pattern, values):
    options = Options()
    for name, value in zip(Options.NAMES, values):
      setattr(options, name, value)
    return cls(pattern, options)

  def __init__(self, pattern, options):
    self._pattern = pattern
    if isinstance(self._pattern, str):
      if options.encoding == Options.Encoding.LATIN1:
        raise error('string type of pattern is str, but '
                    'encoding specified in options is LATIN1')
      encoded_pattern = _encode(self._pattern)
      self._regexp = _re2.RE2(encoded_pattern, options)
    else:
      self._regexp = _re2.RE2(self._pattern, options)
    if not self._regexp.ok():
      raise error(self._regexp.error())

  def __getstate__(self):
    options = {name: getattr(self.options, name) for name in Options.NAMES}
    return self._pattern, options

  def __setstate__(self, state):
    pattern, options = state
    values = tuple(options[name] for name in Options.NAMES)
    other = _Regexp._make(pattern, values)
    self._pattern = other._pattern
    self._regexp = other._regexp

  def _match(self, anchor, text, pos=None, endpos=None):
    pos = 0 if pos is None else max(0, min(pos, len(text)))
    endpos = len(text) if endpos is None else max(0, min(endpos, len(text)))
    if pos > endpos:
      return
    if isinstance(text, str):
      encoded_text = _encode(text)
      encoded_pos = _re2.CharLenToBytes(encoded_text, 0, pos)
      if endpos == len(text):
        # This is the common case.
        encoded_endpos = len(encoded_text)
      else:
        encoded_endpos = encoded_pos + _re2.CharLenToBytes(
            encoded_text, encoded_pos, endpos - pos)
      decoded_offsets = {0: 0}
      last_offset = 0
      while True:
        spans = self._regexp.Match(anchor, encoded_text, encoded_pos,
                                   encoded_endpos)
        if spans[0] == _NULL_SPAN:
          break

        # This algorithm is linear in the length of encoded_text. Specifically,
        # no matter how many groups there are for a given regular expression or
        # how many iterations through the loop there are for a given generator,
        # this algorithm uses a single, straightforward pass over encoded_text.
        offsets = sorted(set(itertools.chain(*spans)))
        if offsets[0] == -1:
          offsets = offsets[1:]
        # Discard the rest of the items because they are useless now - and we
        # could accumulate one item per str offset in the pathological case!
        decoded_offsets = {last_offset: decoded_offsets[last_offset]}
        for offset in offsets:
          decoded_offsets[offset] = (
              decoded_offsets[last_offset] +
              _re2.BytesToCharLen(encoded_text, last_offset, offset))
          last_offset = offset

        def decode(span):
          if span == _NULL_SPAN:
            return span
          return decoded_offsets[span[0]], decoded_offsets[span[1]]

        decoded_spans = [decode(span) for span in spans]
        yield _Match(self, text, pos, endpos, decoded_spans)
        if encoded_pos == encoded_endpos:
          break
        elif encoded_pos == spans[0][1]:
          # We matched the empty string at encoded_pos and would be stuck, so
          # in order to make forward progress, increment the str offset.
          encoded_pos += _re2.CharLenToBytes(encoded_text, encoded_pos, 1)
        else:
          encoded_pos = spans[0][1]
    else:
      while True:
        spans = self._regexp.Match(anchor, text, pos, endpos)
        if spans[0] == _NULL_SPAN:
          break
        yield _Match(self, text, pos, endpos, spans)
        if pos == endpos:
          break
        elif pos == spans[0][1]:
          # We matched the empty string at pos and would be stuck, so in order
          # to make forward progress, increment the bytes offset.
          pos += 1
        else:
          pos = spans[0][1]

  def search(self, text, pos=None, endpos=None):
    return next(self._match(_Anchor.UNANCHORED, text, pos, endpos), None)

  def match(self, text, pos=None, endpos=None):
    return next(self._match(_Anchor.ANCHOR_START, text, pos, endpos), None)

  def fullmatch(self, text, pos=None, endpos=None):
    return next(self._match(_Anchor.ANCHOR_BOTH, text, pos, endpos), None)

  def finditer(self, text, pos=None, endpos=None):
    return self._match(_Anchor.UNANCHORED, text, pos, endpos)

  def findall(self, text, pos=None, endpos=None):
    empty = type(text)()
    items = []
    for match in self.finditer(text, pos, endpos):
      if not self.groups:
        item = match.group()
      elif self.groups == 1:
        item = match.groups(default=empty)[0]
      else:
        item = match.groups(default=empty)
      items.append(item)
    return items

  def _split(self, cb, text, maxsplit=0):
    if maxsplit < 0:
      return [text], 0
    elif maxsplit > 0:
      matchiter = itertools.islice(self.finditer(text), maxsplit)
    else:
      matchiter = self.finditer(text)
    pieces = []
    end = 0
    numsplit = 0
    for match in matchiter:
      pieces.append(text[end:match.start()])
      pieces.extend(cb(match))
      end = match.end()
      numsplit += 1
    pieces.append(text[end:])
    return pieces, numsplit

  def split(self, text, maxsplit=0):
    cb = lambda match: [match[group] for group in range(1, self.groups + 1)]
    pieces, _ = self._split(cb, text, maxsplit)
    return pieces

  def subn(self, repl, text, count=0):
    cb = lambda match: [repl(match) if callable(repl) else match.expand(repl)]
    empty = type(text)()
    pieces, numsplit = self._split(cb, text, count)
    joined_pieces = empty.join(pieces)
    return joined_pieces, numsplit

  def sub(self, repl, text, count=0):
    joined_pieces, _ = self.subn(repl, text, count)
    return joined_pieces

  @property
  def pattern(self):
    return self._pattern

  @property
  def options(self):
    return self._regexp.options()

  @property
  def groups(self):
    return self._regexp.NumberOfCapturingGroups()

  @property
  def groupindex(self):
    groups = self._regexp.NamedCapturingGroups()
    if isinstance(self._pattern, str):
      decoded_groups = [(_decode(group), index) for group, index in groups]
      return dict(decoded_groups)
    else:
      return dict(groups)

  @property
  def programsize(self):
    return self._regexp.ProgramSize()

  @property
  def reverseprogramsize(self):
    return self._regexp.ReverseProgramSize()

  @property
  def programfanout(self):
    return self._regexp.ProgramFanout()

  @property
  def reverseprogramfanout(self):
    return self._regexp.ReverseProgramFanout()

  def possiblematchrange(self, maxlen):
    ok, min, max = self._regexp.PossibleMatchRange(maxlen)
    if not ok:
      raise error('failed to compute match range')
    return min, max


class _Match(object):

  __slots__ = ('_regexp', '_text', '_pos', '_endpos', '_spans')

  def __init__(self, regexp, text, pos, endpos, spans):
    self._regexp = regexp
    self._text = text
    self._pos = pos
    self._endpos = endpos
    self._spans = spans

  # Python prioritises three-digit octal numbers over group escapes.
  # For example, \100 should not be handled the same way as \g<10>0.
  _OCTAL_RE = compile('\\\\[0-7][0-7][0-7]')

  # Python supports \1 through \99 (inclusive) and \g<...> syntax.
  _GROUP_RE = compile('\\\\[1-9][0-9]?|\\\\g<\\w+>')

  @classmethod
  @functools.lru_cache(typed=True)
  def _split(cls, template):
    if isinstance(template, str):
      backslash = '\\'
    else:
      backslash = b'\\'
    empty = type(template)()
    pieces = [empty]
    index = template.find(backslash)
    while index != -1:
      piece, template = template[:index], template[index:]
      pieces[-1] += piece
      octal_match = cls._OCTAL_RE.match(template)
      group_match = cls._GROUP_RE.match(template)
      if (not octal_match) and group_match:
        index = group_match.end()
        piece, template = template[:index], template[index:]
        pieces.extend((piece, empty))
      else:
        # 2 isn't enough for \o, \x, \N, \u and \U escapes, but none of those
        # should contain backslashes, so break them here and then fix them at
        # the beginning of the next loop iteration or right before returning.
        index = 2
        piece, template = template[:index], template[index:]
        pieces[-1] += piece
      index = template.find(backslash)
    pieces[-1] += template
    return pieces

  def expand(self, template):
    if isinstance(template, str):
      unescape = codecs.unicode_escape_decode
    else:
      unescape = codecs.escape_decode
    empty = type(template)()
    # Make a copy so that we don't clobber the cached pieces!
    pieces = list(self._split(template))
    for index, piece in enumerate(pieces):
      if not index % 2:
        pieces[index], _ = unescape(piece)
      else:
        if len(piece) <= 3:  # \1 through \99 (inclusive)
          group = int(piece[1:])
        else:  # \g<...>
          group = piece[3:-1]
          try:
            group = int(group)
          except ValueError:
            pass
        pieces[index] = self.__getitem__(group) or empty
    joined_pieces = empty.join(pieces)
    return joined_pieces

  def __getitem__(self, group):
    if not isinstance(group, int):
      try:
        group = self._regexp.groupindex[group]
      except KeyError:
        raise IndexError('bad group name')
    if not 0 <= group <= self._regexp.groups:
      raise IndexError('bad group index')
    span = self._spans[group]
    if span == _NULL_SPAN:
      return None
    return self._text[span[0]:span[1]]

  def group(self, *groups):
    if not groups:
      groups = (0,)
    items = (self.__getitem__(group) for group in groups)
    return next(items) if len(groups) == 1 else tuple(items)

  def groups(self, default=None):
    items = []
    for group in range(1, self._regexp.groups + 1):
      item = self.__getitem__(group)
      items.append(default if item is None else item)
    return tuple(items)

  def groupdict(self, default=None):
    items = []
    for group, index in self._regexp.groupindex.items():
      item = self.__getitem__(index)
      items.append((group, default) if item is None else (group, item))
    return dict(items)

  def start(self, group=0):
    if not 0 <= group <= self._regexp.groups:
      raise IndexError('bad group index')
    return self._spans[group][0]

  def end(self, group=0):
    if not 0 <= group <= self._regexp.groups:
      raise IndexError('bad group index')
    return self._spans[group][1]

  def span(self, group=0):
    if not 0 <= group <= self._regexp.groups:
      raise IndexError('bad group index')
    return self._spans[group]

  @property
  def re(self):
    return self._regexp

  @property
  def string(self):
    return self._text

  @property
  def pos(self):
    return self._pos

  @property
  def endpos(self):
    return self._endpos

  @property
  def lastindex(self):
    max_end = -1
    max_group = None
    # We look for the rightmost right parenthesis by keeping the first group
    # that ends at max_end because that is the leftmost/outermost group when
    # there are nested groups!
    for group in range(1, self._regexp.groups + 1):
      end = self._spans[group][1]
      if max_end < end:
        max_end = end
        max_group = group
    return max_group

  @property
  def lastgroup(self):
    max_group = self.lastindex
    if not max_group:
      return None
    for group, index in self._regexp.groupindex.items():
      if max_group == index:
        return group
    return None


class Set(object):
  """A Pythonic wrapper around RE2::Set."""

  __slots__ = ('_set')

  def __init__(self, anchor, options=None):
    options = options or Options()
    self._set = _re2.Set(anchor, options)

  @classmethod
  def SearchSet(cls, options=None):
    return cls(_Anchor.UNANCHORED, options=options)

  @classmethod
  def MatchSet(cls, options=None):
    return cls(_Anchor.ANCHOR_START, options=options)

  @classmethod
  def FullMatchSet(cls, options=None):
    return cls(_Anchor.ANCHOR_BOTH, options=options)

  def Add(self, pattern):
    if isinstance(pattern, str):
      encoded_pattern = _encode(pattern)
      index = self._set.Add(encoded_pattern)
    else:
      index = self._set.Add(pattern)
    if index == -1:
      raise error('failed to add %r to Set' % pattern)
    return index

  def Compile(self):
    if not self._set.Compile():
      raise error('failed to compile Set')

  def Match(self, text):
    if isinstance(text, str):
      encoded_text = _encode(text)
      matches = self._set.Match(encoded_text)
    else:
      matches = self._set.Match(text)
    return matches or None


class Filter(object):
  """A Pythonic wrapper around FilteredRE2."""

  __slots__ = ('_filter', '_patterns')

  def __init__(self):
    self._filter = _re2.Filter()
    self._patterns = []

  def Add(self, pattern, options=None):
    options = options or Options()
    if isinstance(pattern, str):
      encoded_pattern = _encode(pattern)
      index = self._filter.Add(encoded_pattern, options)
    else:
      index = self._filter.Add(pattern, options)
    if index == -1:
      raise error('failed to add %r to Filter' % pattern)
    self._patterns.append(pattern)
    return index

  def Compile(self):
    if not self._filter.Compile():
      raise error('failed to compile Filter')

  def Match(self, text, potential=False):
    if isinstance(text, str):
      encoded_text = _encode(text)
      matches = self._filter.Match(encoded_text, potential)
    else:
      matches = self._filter.Match(text, potential)
    return matches or None

  def re(self, index):
    if not 0 <= index < len(self._patterns):
      raise IndexError('bad index')
    proxy = object.__new__(_Regexp)
    proxy._pattern = self._patterns[index]
    proxy._regexp = self._filter.GetRE2(index)
    return proxy
