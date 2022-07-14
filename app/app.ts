// Copyright 2022 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

import {css, html, LitElement, render} from 'lit';
import {customElement, state} from 'lit/decorators.js';

import /*default*/ loadModule from './_re2';
import {Info, MainModule} from './_re2';

var _re2: MainModule;
loadModule().then((module: MainModule) => {
  _re2 = module;
  render(html`<title>re2-dev</title><re2-dev></re2-dev>`, document.body);
});

@customElement('re2-dev')
export class RE2Dev extends LitElement {
  @state() private _info: Info|null = null;

  private _update(pattern: string) {
    if (pattern) {
      this._info = _re2.getInfo(pattern);
    } else {
      this._info = null;
    }
    this.requestUpdate();
  }

  constructor() {
    super();
    var pattern: string = decodeURIComponent(window.location.hash.slice(1));
    this._update(pattern);
  }

  static override styles = css`
    .error {
      color: red;
      font-family: monospace;
      font-weight: bold;
      white-space: pre-line;
    }

    .info {
      color: darkgreen;
      font-family: monospace;
      font-weight: bold;
      white-space: pre-line;
    }
  `;

  override render() {
    return html`
      <div>
        <input type="text" size="64" @change=${this._onChange}/>
      </div>
      ${this._renderInfo(this._info)}
    `;
  }

  private _onChange = (e: Event) => {
    var pattern: string = (e.target as HTMLInputElement).value;
    this._update(pattern);
    window.location.hash = '#' + encodeURIComponent(pattern);
  };

  private _renderInfo(info: Info|null) {
    if (info === null) {
      return html`
      `;
    } else if (info.error) {
      return html`
        <div>
          pattern:
          <span class="info">${info.pattern}</span>
        </div>
        <div>
          error:
          <span class="error">${info.error}</span>
        </div>
      `;
    } else {
      return html`
        <div>
          pattern:
          <span class="info">${info.pattern}</span>
        </div>
        <div>
          prefix:
          <span class="info">${info.prefix}</span>
          _foldcase:
          <span class="info">${info.prefix_foldcase}</span>
        </div>
        <div>
          accel_prefix:
          <span class="info">${info.accel_prefix}</span>
          _foldcase:
          <span class="info">${info.accel_prefix_foldcase}</span>
        </div>
        <div>
          num_captures:
          <span class="info">${info.num_captures}</span>
        </div>
        <div>
          is_one_pass:
          <span class="info">${info.is_one_pass}</span>
        </div>
        <div>
          can_bit_state:
          <span class="info">${info.can_bit_state}</span>
        </div>
        <div>
          bytecode:
          <br/>
          <span class="info">${info.bytecode}</span>
        </div>
        <div>
          bytemap:
          <br/>
          <span class="info">${info.bytemap}</span>
        </div>
      `;
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    're2-dev': RE2Dev;
  }
}
