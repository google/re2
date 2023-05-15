# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
"""Tests for google3.third_party.re2.python.re2."""

import collections
import pickle
import re

from absl.testing import absltest
from absl.testing import parameterized
import re2


class OptionsTest(parameterized.TestCase):

  @parameterized.parameters(*re2.Options.NAMES)
  def test_option(self, name):
    options = re2.Options()
    value = getattr(options, name)
    if isinstance(value, re2.Options.Encoding):
      value = next(v for v in type(value).__members__.values() if v != value)
    elif isinstance(value, bool):
      value = not value
    elif isinstance(value, int):
      value = value + 1
    else:
      raise TypeError('option {!r}: {!r} {!r}'.format(name, type(value), value))
    setattr(options, name, value)
    self.assertEqual(value, getattr(options, name))


class Re2CompileTest(parameterized.TestCase):
  """Contains tests that apply to the re2 module only.

  We disagree with Python on the string types of group names,
  so there is no point attempting to verify consistency.
  """

  @parameterized.parameters(
      (u'(foo*)(?P<bar>qux+)', 2, [(u'bar', 2)]),
      (b'(foo*)(?P<bar>qux+)', 2, [(b'bar', 2)]),
      (u'(foo*)(?P<中文>qux+)', 2, [(u'中文', 2)]),
  )
  def test_compile(self, pattern, expected_groups, expected_groupindex):
    regexp = re2.compile(pattern)
    self.assertIs(regexp, re2.compile(pattern))  # cached
    self.assertIs(regexp, re2.compile(regexp))  # cached
    with self.assertRaisesRegex(re2.error,
                                ('pattern is already compiled, so '
                                 'options may not be specified')):
      options = re2.Options()
      options.log_errors = not options.log_errors
      re2.compile(regexp, options=options)
    self.assertIsNotNone(regexp.options)
    self.assertEqual(expected_groups, regexp.groups)
    self.assertDictEqual(dict(expected_groupindex), regexp.groupindex)

  def test_compile_with_options(self):
    options = re2.Options()
    options.max_mem = 100
    with self.assertRaisesRegex(re2.error, 'pattern too large'):
      re2.compile('.{1000}', options=options)

  def test_programsize_reverseprogramsize(self):
    regexp = re2.compile('a+b')
    self.assertEqual(7, regexp.programsize)
    self.assertEqual(7, regexp.reverseprogramsize)

  def test_programfanout_reverseprogramfanout(self):
    regexp = re2.compile('a+b')
    self.assertListEqual([1, 1], regexp.programfanout)
    self.assertListEqual([3], regexp.reverseprogramfanout)

  @parameterized.parameters(
      (u'abc', 0, None),
      (b'abc', 0, None),
      (u'abc', 10, (b'abc', b'abc')),
      (b'abc', 10, (b'abc', b'abc')),
      (u'ab*c', 10, (b'ab', b'ac')),
      (b'ab*c', 10, (b'ab', b'ac')),
      (u'ab+c', 10, (b'abb', b'abc')),
      (b'ab+c', 10, (b'abb', b'abc')),
      (u'ab?c', 10, (b'abc', b'ac')),
      (b'ab?c', 10, (b'abc', b'ac')),
      (u'.*', 10, (b'', b'\xf4\xbf\xbf\xc0')),
      (b'.*', 10, None),
      (u'\\C*', 10, None),
      (b'\\C*', 10, None),
  )
  def test_possiblematchrange(self, pattern, maxlen, expected_min_max):
    # For brevity, the string type of pattern determines the encoding.
    # It would otherwise be possible to have bytes with UTF8, but as per
    # the module docstring, it isn't permitted to have str with LATIN1.
    options = re2.Options()
    if isinstance(pattern, str):
      options.encoding = re2.Options.Encoding.UTF8
    else:
      options.encoding = re2.Options.Encoding.LATIN1
    regexp = re2.compile(pattern, options=options)
    if expected_min_max:
      self.assertEqual(expected_min_max, regexp.possiblematchrange(maxlen))
    else:
      with self.assertRaisesRegex(re2.error, 'failed to compute match range'):
        regexp.possiblematchrange(maxlen)


Params = collections.namedtuple(
    'Params', ('pattern', 'text', 'spans', 'search', 'match', 'fullmatch'))

PARAMS = [
    Params(u'\\d+', u'Hello, world.', None, False, False, False),
    Params(b'\\d+', b'Hello, world.', None, False, False, False),
    Params(u'\\s+', u'Hello, world.', [(6, 7)], True, False, False),
    Params(b'\\s+', b'Hello, world.', [(6, 7)], True, False, False),
    Params(u'\\w+', u'Hello, world.', [(0, 5)], True, True, False),
    Params(b'\\w+', b'Hello, world.', [(0, 5)], True, True, False),
    Params(u'(\\d+)?', u'Hello, world.', [(0, 0), (-1, -1)], True, True, False),
    Params(b'(\\d+)?', b'Hello, world.', [(0, 0), (-1, -1)], True, True, False),
    Params(u'youtube(_device|_md|_gaia|_multiday|_multiday_gaia)?',
           u'youtube_ads', [(0, 7), (-1, -1)], True, True, False),
    Params(b'youtube(_device|_md|_gaia|_multiday|_multiday_gaia)?',
           b'youtube_ads', [(0, 7), (-1, -1)], True, True, False),
]


def upper(match):
  return match.group().upper()


class ReRegexpTest(parameterized.TestCase):
  """Contains tests that apply to the re and re2 modules."""

  MODULE = re

  @parameterized.parameters((p.pattern,) for p in PARAMS)
  def test_pickle(self, pattern):
    regexp = self.MODULE.compile(pattern)
    rick = pickle.loads(pickle.dumps(regexp))
    self.assertEqual(regexp.pattern, rick.pattern)

  @parameterized.parameters(
      (p.pattern, p.text, (p.spans if p.search else None)) for p in PARAMS)
  def test_search(self, pattern, text, expected_spans):
    match = self.MODULE.search(pattern, text)
    if expected_spans is None:
      self.assertIsNone(match)
    else:
      spans = [match.span(group) for group in range(match.re.groups + 1)]
      self.assertListEqual(expected_spans, spans)

  def test_search_with_pos_and_endpos(self):
    regexp = self.MODULE.compile(u'.+')  # empty string NOT allowed
    text = u'I \u2665 RE2!'
    # Note that len(text) is the position of the empty string at the end of
    # text, so range() stops at len(text) + 1 in order to include len(text).
    for pos in range(len(text) + 1):
      for endpos in range(pos, len(text) + 1):
        match = regexp.search(text, pos=pos, endpos=endpos)
        if pos == endpos:
          self.assertIsNone(match)
        else:
          self.assertEqual(pos, match.pos)
          self.assertEqual(endpos, match.endpos)
          self.assertEqual(pos, match.start())
          self.assertEqual(endpos, match.end())
          self.assertTupleEqual((pos, endpos), match.span())

  def test_search_with_bogus_pos_and_endpos(self):
    regexp = self.MODULE.compile(u'.*')  # empty string allowed
    text = u'I \u2665 RE2!'

    match = regexp.search(text, pos=-100)
    self.assertEqual(0, match.pos)
    match = regexp.search(text, pos=100)
    self.assertEqual(8, match.pos)

    match = regexp.search(text, endpos=-100)
    self.assertEqual(0, match.endpos)
    match = regexp.search(text, endpos=100)
    self.assertEqual(8, match.endpos)

    match = regexp.search(text, pos=100, endpos=-100)
    self.assertIsNone(match)

  @parameterized.parameters(
      (p.pattern, p.text, (p.spans if p.match else None)) for p in PARAMS)
  def test_match(self, pattern, text, expected_spans):
    match = self.MODULE.match(pattern, text)
    if expected_spans is None:
      self.assertIsNone(match)
    else:
      spans = [match.span(group) for group in range(match.re.groups + 1)]
      self.assertListEqual(expected_spans, spans)

  @parameterized.parameters(
      (p.pattern, p.text, (p.spans if p.fullmatch else None)) for p in PARAMS)
  def test_fullmatch(self, pattern, text, expected_spans):
    match = self.MODULE.fullmatch(pattern, text)
    if expected_spans is None:
      self.assertIsNone(match)
    else:
      spans = [match.span(group) for group in range(match.re.groups + 1)]
      self.assertListEqual(expected_spans, spans)

  @parameterized.parameters(
      (u'', u'', [(0, 0)]),
      (b'', b'', [(0, 0)]),
      (u'', u'x', [(0, 0), (1, 1)]),
      (b'', b'x', [(0, 0), (1, 1)]),
      (u'', u'xy', [(0, 0), (1, 1), (2, 2)]),
      (b'', b'xy', [(0, 0), (1, 1), (2, 2)]),
      (u'.', u'xy', [(0, 1), (1, 2)]),
      (b'.', b'xy', [(0, 1), (1, 2)]),
      (u'x', u'xy', [(0, 1)]),
      (b'x', b'xy', [(0, 1)]),
      (u'y', u'xy', [(1, 2)]),
      (b'y', b'xy', [(1, 2)]),
      (u'z', u'xy', []),
      (b'z', b'xy', []),
      (u'\\w*', u'Hello, world.', [(0, 5), (5, 5), (6, 6), (7, 12), (12, 12),
                                   (13, 13)]),
      (b'\\w*', b'Hello, world.', [(0, 5), (5, 5), (6, 6), (7, 12), (12, 12),
                                   (13, 13)]),
  )
  def test_finditer(self, pattern, text, expected_matches):
    matches = [match.span() for match in self.MODULE.finditer(pattern, text)]
    self.assertListEqual(expected_matches, matches)

  @parameterized.parameters(
      (u'\\w\\w+', u'Hello, world.', [u'Hello', u'world']),
      (b'\\w\\w+', b'Hello, world.', [b'Hello', b'world']),
      (u'(\\w)\\w+', u'Hello, world.', [u'H', u'w']),
      (b'(\\w)\\w+', b'Hello, world.', [b'H', b'w']),
      (u'(\\w)(\\w+)', u'Hello, world.', [(u'H', u'ello'), (u'w', u'orld')]),
      (b'(\\w)(\\w+)', b'Hello, world.', [(b'H', b'ello'), (b'w', b'orld')]),
      (u'(\\w)(\\w+)?', u'Hello, w.', [(u'H', u'ello'), (u'w', u'')]),
      (b'(\\w)(\\w+)?', b'Hello, w.', [(b'H', b'ello'), (b'w', b'')]),
  )
  def test_findall(self, pattern, text, expected_matches):
    matches = self.MODULE.findall(pattern, text)
    self.assertListEqual(expected_matches, matches)

  @parameterized.parameters(
      (u'\\W+', u'Hello, world.', -1, [u'Hello, world.']),
      (b'\\W+', b'Hello, world.', -1, [b'Hello, world.']),
      (u'\\W+', u'Hello, world.', 0, [u'Hello', u'world', u'']),
      (b'\\W+', b'Hello, world.', 0, [b'Hello', b'world', b'']),
      (u'\\W+', u'Hello, world.', 1, [u'Hello', u'world.']),
      (b'\\W+', b'Hello, world.', 1, [b'Hello', b'world.']),
      (u'(\\W+)', u'Hello, world.', -1, [u'Hello, world.']),
      (b'(\\W+)', b'Hello, world.', -1, [b'Hello, world.']),
      (u'(\\W+)', u'Hello, world.', 0, [u'Hello', u', ', u'world', u'.', u'']),
      (b'(\\W+)', b'Hello, world.', 0, [b'Hello', b', ', b'world', b'.', b'']),
      (u'(\\W+)', u'Hello, world.', 1, [u'Hello', u', ', u'world.']),
      (b'(\\W+)', b'Hello, world.', 1, [b'Hello', b', ', b'world.']),
  )
  def test_split(self, pattern, text, maxsplit, expected_pieces):
    pieces = self.MODULE.split(pattern, text, maxsplit)
    self.assertListEqual(expected_pieces, pieces)

  @parameterized.parameters(
      (u'\\w+', upper, u'Hello, world.', -1, u'Hello, world.', 0),
      (b'\\w+', upper, b'Hello, world.', -1, b'Hello, world.', 0),
      (u'\\w+', upper, u'Hello, world.', 0, u'HELLO, WORLD.', 2),
      (b'\\w+', upper, b'Hello, world.', 0, b'HELLO, WORLD.', 2),
      (u'\\w+', upper, u'Hello, world.', 1, u'HELLO, world.', 1),
      (b'\\w+', upper, b'Hello, world.', 1, b'HELLO, world.', 1),
      (u'\\w+', u'MEEP', u'Hello, world.', -1, u'Hello, world.', 0),
      (b'\\w+', b'MEEP', b'Hello, world.', -1, b'Hello, world.', 0),
      (u'\\w+', u'MEEP', u'Hello, world.', 0, u'MEEP, MEEP.', 2),
      (b'\\w+', b'MEEP', b'Hello, world.', 0, b'MEEP, MEEP.', 2),
      (u'\\w+', u'MEEP', u'Hello, world.', 1, u'MEEP, world.', 1),
      (b'\\w+', b'MEEP', b'Hello, world.', 1, b'MEEP, world.', 1),
      (u'\\\\', u'\\\\\\\\', u'Hello,\\world.', 0, u'Hello,\\\\world.', 1),
      (b'\\\\', b'\\\\\\\\', b'Hello,\\world.', 0, b'Hello,\\\\world.', 1),
  )
  def test_subn_sub(self, pattern, repl, text, count, expected_joined_pieces,
                    expected_numsplit):
    joined_pieces, numsplit = self.MODULE.subn(pattern, repl, text, count)
    self.assertEqual(expected_joined_pieces, joined_pieces)
    self.assertEqual(expected_numsplit, numsplit)

    joined_pieces = self.MODULE.sub(pattern, repl, text, count)
    self.assertEqual(expected_joined_pieces, joined_pieces)


class Re2RegexpTest(ReRegexpTest):
  """Contains tests that apply to the re2 module only."""

  MODULE = re2

  def test_compile_with_latin1_encoding(self):
    options = re2.Options()
    options.encoding = re2.Options.Encoding.LATIN1
    with self.assertRaisesRegex(re2.error,
                                ('string type of pattern is str, but '
                                 'encoding specified in options is LATIN1')):
      re2.compile(u'.?', options=options)

    # ... whereas this is fine, of course.
    re2.compile(b'.?', options=options)

  @parameterized.parameters(
      (u'\\p{Lo}', u'\u0ca0_\u0ca0', [(0, 1), (2, 3)]),
      (b'\\p{Lo}', b'\xe0\xb2\xa0_\xe0\xb2\xa0', [(0, 3), (4, 7)]),
  )
  def test_finditer_with_utf8(self, pattern, text, expected_matches):
    matches = [match.span() for match in self.MODULE.finditer(pattern, text)]
    self.assertListEqual(expected_matches, matches)

  def test_purge(self):
    re2.compile('Goodbye, world.')
    self.assertGreater(re2._Regexp._make.cache_info().currsize, 0)
    re2.purge()
    self.assertEqual(re2._Regexp._make.cache_info().currsize, 0)


class Re2EscapeTest(parameterized.TestCase):
  """Contains tests that apply to the re2 module only.

  We disagree with Python on the escaping of some characters,
  so there is no point attempting to verify consistency.
  """

  @parameterized.parameters(
      (u'a*b+c?', u'a\\*b\\+c\\?'),
      (b'a*b+c?', b'a\\*b\\+c\\?'),
  )
  def test_escape(self, pattern, expected_escaped):
    escaped = re2.escape(pattern)
    self.assertEqual(expected_escaped, escaped)


class ReMatchTest(parameterized.TestCase):
  """Contains tests that apply to the re and re2 modules."""

  MODULE = re

  def test_expand(self):
    pattern = u'(?P<S>[\u2600-\u26ff]+).*?(?P<P>[^\\s\\w]+)'
    text = u'I \u2665 RE2!\n'
    match = self.MODULE.search(pattern, text)

    self.assertEqual(u'\u2665\n!', match.expand(u'\\1\\n\\2'))
    self.assertEqual(u'\u2665\n!', match.expand(u'\\g<1>\\n\\g<2>'))
    self.assertEqual(u'\u2665\n!', match.expand(u'\\g<S>\\n\\g<P>'))
    self.assertEqual(u'\\1\\2\n\u2665!', match.expand(u'\\\\1\\\\2\\n\\1\\2'))

  def test_expand_with_octal(self):
    pattern = u'()()()()()()()()()(\\w+)'
    text = u'Hello, world.'
    match = self.MODULE.search(pattern, text)

    self.assertEqual(u'Hello\n', match.expand(u'\\g<0>\\n'))
    self.assertEqual(u'Hello\n', match.expand(u'\\g<10>\\n'))

    self.assertEqual(u'\x00\n', match.expand(u'\\0\\n'))
    self.assertEqual(u'\x00\n', match.expand(u'\\00\\n'))
    self.assertEqual(u'\x00\n', match.expand(u'\\000\\n'))
    self.assertEqual(u'\x000\n', match.expand(u'\\0000\\n'))

    self.assertEqual(u'\n', match.expand(u'\\1\\n'))
    self.assertEqual(u'Hello\n', match.expand(u'\\10\\n'))
    self.assertEqual(u'@\n', match.expand(u'\\100\\n'))
    self.assertEqual(u'@0\n', match.expand(u'\\1000\\n'))

  def test_getitem_group_groups_groupdict(self):
    pattern = u'(?P<S>[\u2600-\u26ff]+).*?(?P<P>[^\\s\\w]+)'
    text = u'Hello, world.\nI \u2665 RE2!\nGoodbye, world.\n'
    match = self.MODULE.search(pattern, text)

    self.assertEqual(u'\u2665 RE2!', match[0])
    self.assertEqual(u'\u2665', match[1])
    self.assertEqual(u'!', match[2])
    self.assertEqual(u'\u2665', match[u'S'])
    self.assertEqual(u'!', match[u'P'])

    self.assertEqual(u'\u2665 RE2!', match.group())
    self.assertEqual(u'\u2665 RE2!', match.group(0))
    self.assertEqual(u'\u2665', match.group(1))
    self.assertEqual(u'!', match.group(2))
    self.assertEqual(u'\u2665', match.group(u'S'))
    self.assertEqual(u'!', match.group(u'P'))

    self.assertTupleEqual((u'\u2665', u'!'), match.group(1, 2))
    self.assertTupleEqual((u'\u2665', u'!'), match.group(u'S', u'P'))
    self.assertTupleEqual((u'\u2665', u'!'), match.groups())
    self.assertDictEqual({u'S': u'\u2665', u'P': u'!'}, match.groupdict())

  def test_bogus_group_start_end_and_span(self):
    pattern = u'(?P<S>[\u2600-\u26ff]+).*?(?P<P>[^\\s\\w]+)'
    text = u'I \u2665 RE2!\n'
    match = self.MODULE.search(pattern, text)

    self.assertRaises(IndexError, match.group, -1)
    self.assertRaises(IndexError, match.group, 3)
    self.assertRaises(IndexError, match.group, 'X')

    self.assertRaises(IndexError, match.start, -1)
    self.assertRaises(IndexError, match.start, 3)

    self.assertRaises(IndexError, match.end, -1)
    self.assertRaises(IndexError, match.end, 3)

    self.assertRaises(IndexError, match.span, -1)
    self.assertRaises(IndexError, match.span, 3)

  @parameterized.parameters(
      (u'((a)(b))((c)(d))', u'foo bar qux', None, None),
      (u'(?P<one>(a)(b))((c)(d))', u'foo abcd qux', 4, None),
      (u'(?P<one>(a)(b))(?P<four>(c)(d))', u'foo abcd qux', 4, 'four'),
  )
  def test_lastindex_lastgroup(self, pattern, text, expected_lastindex,
                               expected_lastgroup):
    match = self.MODULE.search(pattern, text)
    if expected_lastindex is None:
      self.assertIsNone(match)
    else:
      self.assertEqual(expected_lastindex, match.lastindex)
      self.assertEqual(expected_lastgroup, match.lastgroup)


class Re2MatchTest(ReMatchTest):
  """Contains tests that apply to the re2 module only."""

  MODULE = re2


class SetTest(absltest.TestCase):

  def test_search(self):
    s = re2.Set.SearchSet()
    self.assertEqual(0, s.Add('\\d+'))
    self.assertEqual(1, s.Add('\\s+'))
    self.assertEqual(2, s.Add('\\w+'))
    self.assertRaises(re2.error, s.Add, '(MEEP')
    s.Compile()
    self.assertItemsEqual([1, 2], s.Match('Hello, world.'))

  def test_match(self):
    s = re2.Set.MatchSet()
    self.assertEqual(0, s.Add('\\d+'))
    self.assertEqual(1, s.Add('\\s+'))
    self.assertEqual(2, s.Add('\\w+'))
    self.assertRaises(re2.error, s.Add, '(MEEP')
    s.Compile()
    self.assertItemsEqual([2], s.Match('Hello, world.'))

  def test_fullmatch(self):
    s = re2.Set.FullMatchSet()
    self.assertEqual(0, s.Add('\\d+'))
    self.assertEqual(1, s.Add('\\s+'))
    self.assertEqual(2, s.Add('\\w+'))
    self.assertRaises(re2.error, s.Add, '(MEEP')
    s.Compile()
    self.assertIsNone(s.Match('Hello, world.'))


class FilterTest(absltest.TestCase):

  def test_match(self):
    f = re2.Filter()
    self.assertEqual(0, f.Add('Hello, \\w+\\.'))
    self.assertEqual(1, f.Add('\\w+, world\\.'))
    self.assertEqual(2, f.Add('Goodbye, \\w+\\.'))
    self.assertRaises(re2.error, f.Add, '(MEEP')
    f.Compile()
    self.assertItemsEqual([0, 1], f.Match('Hello, world.', potential=True))
    self.assertItemsEqual([0, 1], f.Match('HELLO, WORLD.', potential=True))
    self.assertItemsEqual([0, 1], f.Match('Hello, world.'))
    self.assertIsNone(f.Match('HELLO, WORLD.'))

    self.assertRaises(IndexError, f.re, -1)
    self.assertRaises(IndexError, f.re, 3)
    self.assertEqual('Goodbye, \\w+\\.', f.re(2).pattern)
    # Verify whether the underlying RE2 object is usable.
    self.assertEqual(0, f.re(2).groups)


if __name__ == '__main__':
  absltest.main()
