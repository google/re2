# Copyright 2024 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
"""Type stubs for re2.py."""

from typing import Any, Callable, ClassVar, Generic, Iterable, Iterator, Type, TypeVar, overload

import _re2


error = _re2.Error


class Options(_re2.RE2.Options):

  def __init__(self) -> None:
    ...

  NAMES: ClassVar[tuple[str, ...]]


_PatternType = TypeVar('_PatternType', str, bytes)
_Span = tuple[int, int]


def compile(
  pattern: _Regexp[_PatternType] | _PatternType,
  options: Options | None = ...,
) -> _Regexp[_PatternType]:
  ...

def search(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  options: Options | None = ...,
) -> _Match[_PatternType] | None:
  ...

def match(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  options: Options | None = ...,
) -> _Match[_PatternType] | None:
  ...

def fullmatch(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  options: Options | None = ...,
) -> _Match[_PatternType] | None:
  ...

def finditer(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  options: Options | None = ...,
) -> Iterator[_Match[_PatternType]]:
  ...

def findall(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  options: Options | None = ...,
) -> list[_PatternType]:
  ...

def split(
  pattern: _Regexp[_PatternType] | _PatternType,
  text: _PatternType,
  maxsplit: int = 0,
  options: Options | None = ...,
) -> list[_PatternType]:
  ...

def subn(
  pattern: _Regexp[_PatternType] | _PatternType,
  repl: Callable[[_Match[_PatternType]], _PatternType] | _PatternType,
  text: _PatternType,
  count: int = 0,
  options: Options | None = ...,
) -> tuple[_PatternType, int]:
  ...

def sub(
  pattern: _Regexp[_PatternType] | _PatternType,
  repl: Callable[[_Match[_PatternType]], _PatternType] | _PatternType,
  text: _PatternType,
  count: int = 0,
  options: Options | None = ...,
) -> _PatternType:
  ...


def escape(pattern: _PatternType) -> _PatternType:
  ...


def purge() -> None:
  ...


_Anchor = _re2.RE2.Anchor
_NULL_SPAN: _Span


class _Regexp(Generic[_PatternType]):

  def __init__(self, pattern: _PatternType, options: Options) -> None:
    ...

  def __getstate__(self) -> tuple[_PatternType, dict[str, Any]]:
    ...

  def __setstate__(self, state: tuple[_PatternType, dict[str, Any]]) -> None:
    ...

  def search(
    self,
    text: _PatternType,
    pos: int | None = ...,
    endpos: int | None = ...,
  ) -> _Match[_PatternType] | None:
    ...

  def match(
    self,
    text: _PatternType,
    pos: int | None = ...,
    endpos: int | None = ...,
  ) -> _Match[_PatternType] | None:
    ...

  def fullmatch(
    self,
    text: _PatternType,
    pos: int | None = ...,
    endpos: int | None = ...,
  ) -> _Match[_PatternType] | None:
    ...

  def finditer(
    self,
    text: _PatternType,
    pos: int | None = ...,
    endpos: int | None = ...,
  ) -> Iterator[_Match[_PatternType]]:
    ...

  def findall(
    self,
    text: _PatternType,
    pos: int | None = ...,
    endpos: int | None = ...,
  ) -> list[_PatternType]:
    ...

  def split(
    self,
    text: _PatternType,
    maxsplit: int = 0,
  ) -> list[_PatternType]:
    ...

  def subn(
    self,
    repl: Callable[[_Match[_PatternType]], _PatternType] | _PatternType,
    text: _PatternType,
    count: int = 0,
  ) -> tuple[_PatternType, int]:
    ...

  def sub(
    self,
    repl: Callable[[_Match[_PatternType]], _PatternType] | _PatternType,
    text: _PatternType,
    count: int = 0,
  ) -> _PatternType:
    ...

  @property
  def pattern(self) -> _PatternType: ...

  @property
  def options(self) -> Options: ...

  @property
  def groups(self) -> int: ...

  @property
  def groupindex(self) -> dict[_PatternType, int]: ...

  @property
  def programsize(self) -> int: ...

  @property
  def reverseprogramsize(self) -> int: ...

  @property
  def programfanout(self) -> list[int]: ...

  @property
  def reverseprogramfanout(self) -> list[int]: ...

  def possiblematchrange(self, maxlen: int) -> _Span:
    ...


class _Match(Generic[_PatternType]):

  def __init__(
    self,
    regexp: _Regexp[_PatternType],
    text: _PatternType,
    pos: int,
    endpos: int,
    spans: list[_Span],
  ) -> None:
    ...

  def expand(self, template: _PatternType) -> _PatternType:
    ...

  def __getitem__(
    self,
    group: int | _PatternType,
  ) -> _PatternType | None:
    ...

  @overload
  def group(self) -> _PatternType:
    ...

  @overload
  def group(self, group: int | _PatternType) -> _PatternType | None:
    ...

  @overload
  def group(
    self,
    group1: int | _PatternType,
    *groups: int | _PatternType,
  ) -> tuple[_PatternType, ...] | None:
    ...

  def groups(
    self,
    default: _PatternType | None,
  ) -> tuple[_PatternType | None, ...]:
    ...

  def groupdict(
    self,
    default: _PatternType | None,
  ) -> dict[_PatternType, _PatternType | None]:
    ...

  def start(self, group: int = 0) -> int:
    ...

  def end(self, group: int = 0) -> int:
    ...

  def span(self, group: int = 0) -> _Span:
    ...

  @property
  def re(self) -> _Regexp[_PatternType]: ...

  @property
  def string(self) -> _PatternType: ...

  @property
  def pos(self) -> int: ...

  @property
  def endpos(self) -> int: ...

  @property
  def lastindex(self) -> int | None: ...

  @property
  def lastgroup(self) -> _PatternType | None: ...


class Set(Generic[_PatternType]):

  def __init__(
    self,
    anchor: _Anchor,
    options: Options | None = ...,
  ) -> None:
    ...

  @classmethod
  def SearchSet(
    cls: Type[Set[_PatternType]],
    options: Options | None = ...,
  ) -> Set[_PatternType]:
    ...

  @classmethod
  def MatchSet(
    cls: Type[Set[_PatternType]],
    options: Options | None = ...,
  ) -> Set[_PatternType]:
    ...

  @classmethod
  def FullMatchSet(
    cls: Type[Set[_PatternType]],
    options: Options | None = ...,
  ) -> Set[_PatternType]:
    ...

  def Add(self, pattern: _PatternType) -> int:
    ...

  def Compile(self) -> None:
    ...

  def Match(self, text: _PatternType) -> list[int] | None:
    ...


class Filter(Generic[_PatternType]):

  def __init__(self) -> None:
    ...

  def Add(
    self,
    pattern: _PatternType,
    options: Options | None = ...,
  ) -> int:
    ...

  def Compile(self) -> None:
    ...

  def Match(
    self,
    text: _PatternType,
    potential: bool = False,
  ) -> list[int] | None:
    ...

  def re(self, index: int) -> _Regexp[_PatternType]:
    ...
