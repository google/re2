// Copyright 2022 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

import {css, html, LitElement, render} from 'lit';
import {customElement} from 'lit/decorators.js';

import /*default*/ loadModule from './_re2';
import {Info, MainModule} from './_re2';

var _re2: MainModule;
loadModule().then((module: MainModule) => {
  _re2 = module;
  render(html`<title>re2-dev</title><re2-dev></re2-dev>`, document.body);
});

@customElement('re2-dev')
export class RE2Dev extends LitElement {
  private _pattern: string = '';
  private _info: Info|null = null;

  constructor() {
    super();
    this._pattern = decodeURIComponent(window.location.hash.slice(1));
    this._info = this._pattern ? _re2.getInfo(this._pattern) : null;
    this.requestUpdate();
  }

  private _onChange = (e: Event) => {
    this._pattern = (e.target as HTMLInputElement).value;
    this._info = this._pattern ? _re2.getInfo(this._pattern) : null;
    this.requestUpdate();
    window.location.hash = '#' + encodeURIComponent(this._pattern);
  };

  static override styles = css`
.code {
  font-family: monospace;
  white-space: pre-line;
}
`;

  override render() {
    var fragments = [];
    fragments.push(html`
<div>
  <input type="text" size="48" @change=${this._onChange} .value=${this._pattern}>
</div>
`);

    if (this._info === null) {
      return html`${fragments}`;
    }

    if (this._info.error) {
      fragments.push(html`
<br>
<div>
  error:
  <span class="code">${this._info.error}</span>
</div>
`);
      return html`${fragments}`;
    }

    fragments.push(html`
<br>
<div>
  pattern:
  <span class="code">${this._info.pattern}</span>
  <br>
  prefix:
  <span class="code">${this._info.prefix}</span>
  ·
  _foldcase:
  <span class="code">${this._info.prefix_foldcase}</span>
  <br>
  accel_prefix:
  <span class="code">${this._info.accel_prefix}</span>
  ·
  _foldcase:
  <span class="code">${this._info.accel_prefix_foldcase}</span>
  <br>
  num_captures:
  <span class="code">${this._info.num_captures}</span>
  <br>
  is_one_pass:
  <span class="code">${this._info.is_one_pass}</span>
  <br>
  can_bit_state:
  <span class="code">${this._info.can_bit_state}</span>
  <br>
  <br>
  bytecode:
  <br>
  <span class="code">${this._info.bytecode}</span>
  <br>
  bytemap:
  <br>
  <span class="code">${this._info.bytemap}</span>
</div>
`);
    return html`${fragments}`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    're2-dev': RE2Dev;
  }
}
