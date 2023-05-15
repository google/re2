// Copyright 2022 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

import nodeResolve from '@rollup/plugin-node-resolve';
import terser from '@rollup/plugin-terser';
import html from '@web/rollup-plugin-html';
import {importMetaAssets} from '@web/rollup-plugin-import-meta-assets';

export default {
  input: 'index.html',
  output: {
    entryFileNames: '[hash].js',
    chunkFileNames: '[hash].js',
    assetFileNames: '[hash][extname]',
    format: 'es',
  },
  preserveEntrySignatures: false,
  plugins:
      [
        html({
          minify: true,
        }),
        nodeResolve(),
        terser(),
        importMetaAssets(),
      ],
};
