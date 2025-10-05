
function create_env(obj) {
  return new Proxy(obj, {
    get(target, property) {
      if (property in target) return target[property];
      return (...args) => {
	const args_str = args.join(', ');
	console.error(`UnimplementedException: ${property}(${args_str})`);
	return 0;
      };
    }
  });
}

class MemoryView extends DataView {
  offset = 0;

  shift_bool   = () => {
    const n = this.getUint8(this.offset);
    this.offset += 1;
    return n == 1;
  }
  get_bool = () => this.getUint8(0) == 1;
  set_bool = (value) => {
    if (typeof value != 'boolean') {
      throw new TypeError('Expected boolean');
    }
    this.setUint8(0, value);
  };

  shift_uint8  = () => {
    this.offset = mem.align_up(this.offset, 4);
    const n = this.getUint8(this.offset);
    this.offset += 4;
    return n;
  };
  get_uint8    = () => this.getUint8(0);
  set_uint8    = (value) => this.setUint8(0, value);

  shift_int8   = () => {
    this.offset = mem.align_up(this.offset, 4);
    const n = this.getUint8(this.offset);
    this.offset += 4;
    return n;
  };
  get_int8     = () => this.getInt8(0);
  set_int8     = (value) => this.setInt8(0, value);

  shift_uint32 = () => {
    this.offset = mem.align_up(this.offset, 4);
    const n = this.getUint32(this.offset, true);
    this.offset += 4;
    return n;
  };
  get_uint32   = () => this.getUint32(0, true);
  set_uint32   = (value) => this.setUint32(0, value, true);

  shift_int32  = () => {
    this.offset = mem.align_up(this.offset, 4);
    const n = this.getInt32(this.offset, true);
    this.offset += 4;
    return n;
  };
  get_int32    = () => this.getInt32(0, true);
  set_int32    = (value) => this.setInt32(0, value, true);

  shift_flt32  = () => {
    this.offset = mem.align_up(this.offset, 4);
    const n = this.getFloat32(this.offset, true);
    this.offset += 4;
    return n;
  };
  get_flt32    = () => this.getFlt32(0, true);
  set_flt32    = (value) => this.setFlt32(0, value, true);

  shift_flt64  = () => {
    this.offset = mem.align_up(this.offset, 8);
    const n = this.getFloat64(this.offset, true);
    this.offset += 8;
    return n;
  };
  get_flt64    = () => this.getFloat64(0, true);
  set_flt64    = (value) => this.setFloat64(0, value, true);

  shift_uint64 = () => {
    this.offset = mem.align_up(this.offset, 8);
    const n = this.getBigUint64(this.offset, true);
    this.offset += 8;
    return n;
  };
  get_uint64   = () => this.getBigUint64(0, true);
  set_uint64   = (value) => this.setBigUint64(0, value, true);

  shift_int64  = () => {
    this.offset = mem.align_up(this.offset, 8);
    const n = this.getBigInt64(this.offset, true);
    this.offset += 8;
    return n;
  };
  get_int64    = () => this.getBigInt64(0, true);
  set_int64    = (value) => this.setBigInt64(0, value, true);


  get_offset_ptr() { return mem.align_up(this.byteOffset + this.offset); }
  get_offset_as_size() { return mem.align_up(this.offset, 4) }
  shift_ptr = this.shift_uint32
  reset_offset = () => { this.offset = 0; }
}

const Utf8 = Object.freeze({
  __encoder: new TextEncoder(),
  __decoder: new TextDecoder(),
  encode: (...strings) => Utf8.__encoder.encode(strings.join('')),
  decode: (buffer) => Utf8.__decoder.decode(buffer),
});

const mem = {
  blocks: new Map(),
  get BYTE_ALIGNMENT() { return 8; },
  align_up: (x, alignment = mem.BYTE_ALIGNMENT) => ((x + (alignment - 1)) & ~(alignment - 1)),

  create_view: (offset = 0) => new MemoryView(mem.buffer, offset),
  

  // The following line of variables are set once the WASM module is loaded
  memory: null, heap_base: 0, heap_end: 0, index: 0,


  // ============================================
  // |
  // | Principal memory related functions
  // |
  // -----------------------------------------

  free(ptr) { // We are using a bump allocator so we don't really implement free
    mem.blocks.delete(ptr);
  },

  alloc(bytes_count) {
    bytes_count = mem.align_up(bytes_count);

    if (this.index + bytes_count > this.heap_end) {
      try {
	while (this.index + bytes_count > this.heap_end) this.memory.grow(1);
      } catch (e) {
	console.error('[WASM] Out of Memory: Failed to expand wasm memory');
	return 0;
      }
    }
    const ptr = this.index;
    this.index += bytes_count;
    this.blocks.set(ptr, bytes_count);
    return ptr;
  },

  realloc(base_ptr, bytes_count) {
    bytes_count = mem.align_up(bytes_count);

    if (base_ptr == 0) return this.alloc(bytes_count);
    if (!this.blocks.has(base_ptr)) {
      throw new Error('[WASM] Illegal Memory Access: Attempting to reallocate a pointer that was not formally allocated before');
    }
    if (this.index + bytes_count > this.heap_end) {
      try {
	while (this.index + bytes_count > this.heap_end) this.memory.grow(1);
      } catch (e) {
	console.error('[WASM] Out of Memory');
	console.error(e);
	return 0;
      }
    }

    const ptr = this.index;
    this.index += bytes_count;

    const old_block = new Uint8Array(this.memory.buffer, base_ptr, this.blocks.get(base_ptr));
    const new_block = new Uint8Array(this.memory.buffer, ptr, bytes_count);
    for (let i = 0; i < Math.min(old_block.length, new_block.length); ++i) {
      new_block[i] = old_block[i];
    }
    this.blocks.delete(base_ptr);
    this.blocks.set(ptr, bytes_count);

    return ptr;
  },


  // ============================================
  // |
  // | Helper memory data reading functions
  // |
  // -----------------------------------------

  read_uint32(ptr) {
    if (ptr == 0) {
      throw new Error('SIGSEV: Attempting to read uint32 from NULL pointer');
    }
    const view = mem.create_view();
    return view.getUint32(ptr);
  },

  read_zstr(ptr) {
    if (ptr == 0) return Object.freeze({ buffer: new Uint8Array(0), ptr, len: 0, str: '', });
    const buf = new Uint8Array(this.memory.buffer, ptr);
    let len = 0, i = 0;
    let c = buf[i++];
    while (c != 0) {
      if (i > buf.length) {
	throw new Error('Illegal memory access: Trying to read zstr that never hit an end');
      }
      len++;
      c = buf[i++];
    }
    const data = buf.slice(0, len);
    return Object.freeze({
      buffer: data,
      ptr, len,
      str: Utf8.decode(data),
    });
  },

  read_str(ptr, len) {
    if (ptr == 0) return Object.freeze({ buffer: new Uint8Array(0), ptr, len: 0, str: '', });
    const buffer = new Uint8Array(this.memory.buffer, ptr, len);
    return Object.freeze({
      buffer,
      ptr, len,
      str: Utf8.decode(buffer),
    });
  },

  read_sv(ptr) {
    if (ptr == 0) return this.read_str(0, 0);
    const view = new DataView(this.memory.buffer);
    const data_ptr   = view.getUint32(ptr + 0, true);
    const length = view.getUint32(ptr + 4, true);
    return this.read_str(data_ptr, length);
  },

  // shape: StructShape; type StructShape = Array<[Name: String, Type: String] | [Name: String, StructShape]>
  generate_struct_view(shape, ptr, view = mem.create_view(ptr)) {
    const props = {};
    for (const [prop_name, prop_type] of shape) {
      if (prop_name === "ptr" || prop_name === "sizeof") {
	throw new Error("Struct shape cannot have properties with name " + prop_name + " as it conflicts with provided properties");
      }

      if (Array.isArray(prop_type)) {
	const sub_ptr = mem.align_up(ptr + view.offset, 4);
	props[prop_name] = mem.generate_struct_view(prop_type, sub_ptr, view);
	continue;
      }

      switch (prop_type) {
      case 'bool': {
	const offset = mem.align_up(view.offset, 1);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_bool(); },
	  set value(value) { mem.create_view(this.ptr).set_bool(value); },
	};
	view.shift_bool();
      } break;

      case 'char':
      case 'int8': {
	const offset = mem.align_up(view.offset, 4);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_int8(); },
	  set value(value) { mem.create_view(this.ptr).set_int8(value); },
	};
	view.shift_int8();
      } break;

      case 'uint8': {
	const offset = mem.align_up(view.offset, 4);
	props[prop_name] = {
	  get offset() { return offset },
	  get ptr() { return ptr + offset },
	  get value() { return mem.create_view(this.ptr).get_uint8(); },
	  set value(value) { mem.create_view(this.ptr).set_uint8(value); },
	};
	view.shift_uint8();
      } break;

      case 'int':
      case 'enum':
      case 'int32': {
	const offset = mem.align_up(view.offset, 4);
	props[prop_name] = {
	  get offset() { return offset },
	  get ptr() { return ptr + offset },
	  get value() { return mem.create_view(this.ptr).get_uint32(); },
	  set value(value) { mem.create_view(this.ptr).set_uint32(value); },
	};
	view.shift_int32();
      } break;

      case 'uint32':
      case 'size_t':
      case 'ptr': {
	const offset = mem.align_up(view.offset, 4);
	props[prop_name] = {
	  get offset() { return offset },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_uint32(); },
	  set value(value) { mem.create_view(this.ptr).set_uint32(value); },
	};
	view.shift_uint32();
      } break;

      case 'float32':
      case 'float': {
	const offset = mem.align_up(view.offset, 4);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_flt32(); },
	  set value(value) { mem.create_view(this.ptr).set_flt32(value); },
	};
	view.shift_flt32();
      } break;

      case 'int64': {
	const offset = mem.align_up(view.offset, 8);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_uint32(); },
	  set value(value) { mem.create_view(this.ptr).set_uint32(value); },
	};
	view.shift_int64();
      } break;

      case 'uint64': {
	const offset = mem.align_up(view.offset, 8);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_uint64(); },
	  set value(value) { mem.create_view(this.ptr).set_uint64(value); },
	};
	view.shift_uint64();
      } break;

      case 'float64':
      case 'double': {
	const offset = mem.align_up(view.offset, 8);
	props[prop_name] = {
	  get offset() { return offset; },
	  get ptr() { return ptr + offset; },
	  get value() { return mem.create_view(this.ptr).get_flt64(); },
	  set value(value) { mem.create_view(this.ptr).set_flt64(value); },
	};
	view.shift_flt64();
      } break;

      default:
	throw new Error('Unhandled type when generating struct viewer: ' + prop_type);
      }

      const prop_offset = props[prop_name].offset;
      const prop_ptr    = props[prop_name].ptr;
      Object.defineProperties(props[prop_name], {
	offset: {
	  configurable: false,
	  enumerable: true,
	  writable: true,
	  value: prop_offset,
	},
	ptr: {
	  configurable: false,
	  enumerable: true,
	  writable: true,
	  value: prop_ptr,
	},
      });
    }

    Object.defineProperties(props, {
      ptr: {
	configurable: false,
	enumerable: true,
	writable: true,
	value: ptr,
      },
      sizeof: {
	configurable: false,
	enumerable: true,
	writable: true,
	value: view.get_offset_as_size(),
      },
    });

    return Object.freeze(props);
  },
};



export const example_code = `proc ascii_E [] -> [number]
    34 35 +
end

580
ascii_E
print_char
print_stack
160 rot2 -
print_number
`;

let buffered_output = '';
let output_target;
function print(message) {
  if (typeof message !== 'string') {
    throw new TypeError("Message passed onto print function must be a string");
  }
  if (output_target) {
    output_target.write(message);
    return;
  }
  if (message == '\n') {
    console.log(buffered_output);
    buffered_output = '';
    return;
  }
  const idx = message.lastIndexOf('\n');
  if (idx > -1) {
    buffered_output += message.substring(0, idx);
    console.log(buffered_output);
    buffered_output = message.substring(idx + 1);
  } else {
    buffered_output += message;
  }
}

const List_Shape = (name, props = []) => [name, [
  ["items", "ptr"],  // List<T>
  ["len", "size_t"],
  ["cap", "size_t"],
  ...props,
]];

const String_View_Shape = (name) => [name, [
  ["data", "ptr"],    // char*
  ["len", "size_t"],
]];

const Proc_Shape = (name) => [name, [
  ["items", "ptr"],   // List<Token>
  ["len", "size_t"],
  ["cap", "size_t"],

  String_View_Shape("name_sv"),

  List_Shape("inputs"),  // List<Value_Kind>
  List_Shape("outputs"), // List<Value_Kind>
]];

const env = create_env({
  web_parse_number(buf_ptr, buf_sz) {
    if (buf_ptr == 0) {
      throw new Error('Invalid pointer used for float parse request');
    }
    const str = mem.read_str(buf_ptr, buf_sz).str;
    return parseFloat(str);
  },

  web_malloc(sz) {
    return mem.alloc(sz);
  },

  web_mfree(ptr) {
    mem.free(ptr);
  },

  web_mrealloc(ptr, sz) {
    return mem.realloc(ptr, sz);
  },

  web_printf(pFmt, pVargs) {
    const view = mem.create_view(pVargs);

    const fmt = mem.read_zstr(pFmt).str;
    for (let i = 0; i < fmt.length; ++i) {
      if (fmt[i] != '%') {
	print(fmt[i]);
	continue;
      }

      let ss = fmt.substring(i);
      if (ss.startsWith('%zu')) {
	i += 2;
	const zu = view.shift_uint32();
	print(zu.toString(10));
	continue;
      }

      if (ss.startsWith('%d')) {
	i++;
	const d = view.shift_int32();
	print(d.toString(10));
	continue;
      }

      if (ss.startsWith('%s')) {
	i++;
	const data_ptr = view.shift_ptr();
	const { str } = mem.read_zstr(data_ptr);
	print(str);
	continue;
      }

      if (ss.startsWith('%c')) {
	i++;
	const val = view.shift_int8();
	const str = String.fromCharCode(val);
	print(str);
	continue;
      }

      if (ss.startsWith('%.')) {
	i += 2;
	ss = ss.substring(2);
	let n = 0;
	if (ss[0] == '*') {
	  n = view.shift_uint32();
	  ss = ss.substring(1);
	  if (ss[0] != 's') {
	    throw new Error('Arbitrary variable defined precision is only allowed for strings');
	  }
	  const data_ptr = view.shift_ptr();
	  const { str } = mem.read_str(data_ptr, n);
	  print(str);
	  continue;
	} else if ("0123456789".includes(ss[0])) {
	  let num_buf = '';
	  while ("0123456789".includes(ss[0])) {
	    num_buf += ss[0];
	    ++i;
	    ss = ss.substring(1);
	  }
	  n = Number(num_buf);
	  if (ss[0] == 'f') {
	    const flt = view.shift_flt64();
	    print(flt.toFixed(n));
            continue;
	  }
	  throw new Error('Format string defined precision is only allowed for float');
	}
	throw new Error(`Unsupported flag/format: %.${ss[0]}`);
      }

      throw new Error(`Unsupported flag/format: %${ss[1]}`);
    }
  },

  web_printfn(pFmt, pVargs) {
    env.web_printf(pFmt, pVargs);
    print('\n');
  },
});

const wait_frame = () => new Promise((resolve) => setTimeout(resolve, 0));

export async function load_interpreter() {
  const wasm = await WebAssembly.instantiateStreaming(fetch("./qleei.wasm"), { env });

  const exports = wasm.instance.exports;
  console.log('Instance.exports:', exports);
  Object.defineProperties(mem, {
    memory: {
      get() { return exports.memory; },
    },
    heap_base: {
      get() { return exports.__heap_base.value; },
    },
    heap_end: {
      get() { return exports.__heap_end.value; },
    },
    buffer: {
      get() { return exports.memory.buffer; },
    },
  });
  mem.index = mem.heap_base;
  console.log('Memory Manager:', mem);
  // window.mem = mem;

  const mod = {
    interpret_buffer: (buf_ptr, buf_sz) => exports.interpret_buffer(buf_ptr, buf_sz) == 1,
  };

  const CODE_BUF_CAP = mem.align_up(1024*8);
  const code_ptr = mem.alloc(CODE_BUF_CAP);
  if (code_ptr == 0) {
    throw new Error('Failed to allocate the wanted space for code of' + CODE_BUF_CAP.toString(10) + 'bytes');
  }

  const save_point = mem.index;

  const interpret_code = async (code) => {
    mem.index = save_point;

    const bytes = Utf8.encode(code);
    if (bytes.length > CODE_BUF_CAP) {
      throw new Error('Code exceeds allowed space for transfer to interpreter');
    }
    const buf = { ptr: code_ptr, len: bytes.byteLength };
    const view = mem.create_view(code_ptr);
    for (let i = 0; i < CODE_BUF_CAP; ++i) {
      const value = i < bytes.length ? bytes[i] : 0;
      view.setUint8(i, value);
    }

    return mod.interpret_buffer(buf.ptr, buf.len);
  };

  return {
    exec: interpret_code,
    set_output: (target) => {
      if (typeof target !== 'object') {
	throw new TypeError('Output must be an object with writeln property or null');
      }
      if (target) {
	if (typeof target.write != 'function') {
	  throw new TypeError('Output is required to have a write property that is a function');
	}
      }
      output_target = target;
      if (buffered_output.length) {
	target.write(buffered_output);
	buffered_output = '';
      }
    },
  };
}

