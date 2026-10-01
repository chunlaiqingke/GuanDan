// 极简 protobuf wire-format 读写助手。
// 与 guandan-server/src/proto/messages.proto（protoc 生成代码）二进制兼容。
// 之所以手写而非用 protobufjs：本环境无法从 GitHub 安装 protobufjs，
// 但保证与服务端字节流严格一致，后续可无缝替换为 pbjs 生成代码。

const enum WireType {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimited = 2,
  Fixed32 = 5,
}

// ---- 手写 UTF-8（不依赖 TextEncoder/TextDecoder，兼容 Cocos 原生 jsb）----
export function utf8Encode(s: string): Uint8Array {
  const out: number[] = [];
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    if (c < 0x80) {
      out.push(c);
    } else if (c < 0x800) {
      out.push(0xc0 | (c >> 6), 0x80 | (c & 0x3f));
    } else if (c < 0xd800 || c >= 0xe000) {
      out.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f));
    } else {
      const c2 = s.charCodeAt(++i);
      const cp = 0x10000 + ((c - 0xd800) << 10) + (c2 - 0xdc00);
      out.push(0xf0 | (cp >> 18), 0x80 | ((cp >> 12) & 0x3f), 0x80 | ((cp >> 6) & 0x3f),
               0x80 | (cp & 0x3f));
    }
  }
  return new Uint8Array(out);
}

export function utf8Decode(b: Uint8Array): string {
  let out = '';
  let i = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x80) {
      out += String.fromCharCode(c);
    } else if (c < 0xe0) {
      out += String.fromCharCode(((c & 0x1f) << 6) | (b[i++] & 0x3f));
    } else if (c < 0xf0) {
      out += String.fromCharCode(((c & 0x0f) << 12) | ((b[i++] & 0x3f) << 6) | (b[i++] & 0x3f));
    } else {
      const cp = ((c & 0x07) << 18) | ((b[i++] & 0x3f) << 12) | ((b[i++] & 0x3f) << 6) |
                 (b[i++] & 0x3f);
      const u = cp - 0x10000;
      out += String.fromCharCode(0xd800 + (u >> 10), 0xdc00 + (u & 0x3ff));
    }
  }
  return out;
}

export class Writer {
  private chunks: number[] = [];
  private size = 0;

  private varint(v: number): void {
    let x = Math.trunc(v);
    while (x >= 0x80) {
      this.chunks.push((x % 128) | 0x80);
      this.size++;
      x = Math.floor(x / 128);
    }
    this.chunks.push(x);
    this.size++;
  }

  private tag(fieldNo: number, wt: WireType): void {
    this.varint((fieldNo << 3) | wt);
  }

  writeInt32(fieldNo: number, v: number): void {
    if (v === 0) return; // proto3 默认值省略
    this.tag(fieldNo, WireType.Varint);
    this.varint(v >= 0 ? v : v + 0x100000000); // 负 int32 符号扩展为 64 位补码
  }

  writeInt64(fieldNo: number, v: number): void {
    if (v === 0) return;
    this.tag(fieldNo, WireType.Varint);
    this.varint(v);
  }

  writeBool(fieldNo: number, v: boolean): void {
    if (!v) return;
    this.tag(fieldNo, WireType.Varint);
    this.varint(1);
  }

  writeString(fieldNo: number, v: string): void {
    if (v.length === 0) return;
    const bytes = utf8Encode(v);
    this.tag(fieldNo, WireType.LengthDelimited);
    this.varint(bytes.length);
    this.raw(bytes);
  }

  writeMessage(fieldNo: number, bytes: Uint8Array): void {
    if (bytes.byteLength === 0) return;
    this.tag(fieldNo, WireType.LengthDelimited);
    this.varint(bytes.byteLength);
    this.raw(bytes);
  }

  private raw(b: Uint8Array): void {
    for (let i = 0; i < b.length; i++) this.chunks.push(b[i]);
    this.size += b.length;
  }

  finish(): Uint8Array {
    const out = new Uint8Array(this.size);
    let off = 0;
    for (const c of this.chunks) out[off++] = c;
    return out;
  }
}

export class Reader {
  private pos = 0;

  constructor(private readonly data: Uint8Array) {}

  get eof(): boolean {
    return this.pos >= this.data.length;
  }

  readVarint(): number {
    let result = 0;
    let shift = 0;
    while (true) {
      if (this.pos >= this.data.length) throw new Error('varint overflow');
      const b = this.data[this.pos++];
      result += (b & 0x7f) * 2 ** shift;
      if ((b & 0x80) === 0) break;
      shift += 7;
    }
    return result;
  }

  /** 返回 [fieldNo, wireType]；到末尾返回 null。 */
  readTag(): [number, number] | null {
    if (this.eof) return null;
    const key = this.readVarint();
    return [key >>> 3, key & 0x7];
  }

  readBytes(): Uint8Array {
    const len = this.readVarint();
    const out = this.data.slice(this.pos, this.pos + len);
    this.pos += len;
    return out;
  }

  readString(): string {
    return utf8Decode(this.readBytes());
  }

  skip(wireType: number): void {
    switch (wireType) {
      case WireType.Varint:
        this.readVarint();
        break;
      case WireType.Fixed64:
        this.pos += 8;
        break;
      case WireType.LengthDelimited:
        this.readBytes();
        break;
      case WireType.Fixed32:
        this.pos += 4;
        break;
      default:
        throw new Error('unknown wire type ' + wireType);
    }
  }
}
