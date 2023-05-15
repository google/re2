// Copyright 2022 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

export type Info = {
  pattern: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
  error: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
  prefix: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
  prefix_foldcase: boolean,
  accel_prefix: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
  accel_prefix_foldcase: boolean,
  num_captures: number,
  is_one_pass: boolean,
  can_bit_state: boolean,
  bytecode: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
  bytemap: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string,
};

export interface MainModule {
  getInfo(pattern: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string): Info;
}

export default function loadModule(): Promise<MainModule>;
