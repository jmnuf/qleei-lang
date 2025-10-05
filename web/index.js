import { load_interpreter, example_code } from "./qleei.js";

class List extends Array {
  constructor(length) {
    typeof length === 'number' ? super(length) : super();
  }

  get last_index() {
    if (this.length < 1) return -1;
    return this.length-1;
  }

  last_item() {
    const index = this.length - 1;
    if (index == -1) return undefined;
    return this[index];
  }

  static from_array(array) {
    const list = new List(array.length);
    for (let i = 0; i < array.length; ++i) {
      list[i] = array[i];
    }
    return list;
  }
}

Array.prototype.last = function() {
};

const appDiv = document.querySelector('#app');

function setup_loading_screen() {
  appDiv.className = "flex flex-col w-full text-center";
  appDiv.innerHTML = `
<h1>QLeei Lang</h1>
<p>QLeei is a stack based inspired language and has been created for experimental purposes only.</p>
<div class="w-full flex flex-col justify-center items-center">
  <p>Loading Interpreter...</p>
</div>
`;
  return appDiv.querySelector('div');
}

function setup_input_screen(div, interpreter) {
  const start_code = localStorage.getItem('qleei:code') ?? example_code;

  div.innerHTML = `
<form id="code-form" class="flex flex-col text-center gap-1">
    <h2>Code</h2>
    <div>
        <label for="code-area">Source:</label>
        <button id="submit-btn" type="submit">Interpret</button>
    </div>
    <textarea id="code-area" cols="50" rows="25"></textarea>
</form>
<section id="log" class="px-2 flex flex-col text-align-left">
    <h2 class="pt-4">Logs</h2>
</section>
`;
  div.className = "w-full px-2 grid grid-cols-1 md:grid-cols-2 text-center justify-center";

  const textarea = div.querySelector('#code-area');
  const pLog     = div.querySelector('#log');
  const codeform = div.querySelector('#code-form');

  const logs = new List();
  logs.push('');

  const run_code = (code) => setTimeout(() => {
    interpreter
      .exec(code)
      .then((result) => {
	if (result) {
	  logs.push('[SYSTEM] OK');
	} else {
	  logs.push('[SYSTEM] FAILED');
	}
	render_logs();
      })
      // .then((result) => {
      // 	if (result.ok) {
      // 	  logs.push('[SYSTEM] OK');
      // 	} else if (result.error == 'failed') {
      // 	  logs.push('[SYSTEM] FAILED');
      // 	} else if (result.error == 'aborted') {
      // 	  return;
      // 	}
      // 	render_logs();
      // });
  }, 0);

  textarea.value = start_code;
  localStorage.setItem('qleei:code', start_code);
  codeform.addEventListener('submit', (event) => {
    event.preventDefault();

    const code = textarea.value;
    localStorage.setItem('qleei:code', code);
    
    logs.length = 0;
    render_logs();
    run_code(code);
  });

  const render_logs = () => {
    const html = logs.reduce((acc, line) => {
      acc += `<p>${line}</p>`;
      return acc;
    }, '');
    pLog.innerHTML = '<h2 class="pt-4">Logs</h2>' + html;
  };
  
  interpreter.set_output({
    write(content) {
      if (content === '\n') {
	render_logs();
	logs.push('');
	return;
      }
      if (!content.includes('\n')) {
	if (logs.length > 0) logs[logs.last_index] = logs[logs.last_index] + content;
	else logs.push(content);
	render_logs();
	return;
      }
      logs.push(...content.split('\n'));
      render_logs();
    },
  });

  run_code(start_code);
}

async function main() {
  const div = setup_loading_screen();
  const interpreter = await load_interpreter();
  setup_input_screen(div, interpreter);
}

main();

